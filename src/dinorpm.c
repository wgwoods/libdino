/* dinoinfo - Displays information about DINO files.
 *
 * GPLv2+ etc.
 *
 * Author: Will Woods <wwoods@redhat.com>
 */

#include <argp.h>
#include <stdlib.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>

#include <rpm/header.h>

#include "dinotools.h"

/* Program version */
const char *argp_program_version = "dinorpm 0.1";

/* Short program description */
static const char doc[] = N_("\
Print information from DINO file in human-readable form.");

/* String for program arguments, used in help text */
static const char args_doc[] = N_("FILE");

/* The options we understand */
static const struct argp_option options[] =
{
    {"verbose", 'v', 0, 0, "Produce verbose output" },
    { 0 }
};

/* A struct to hold program options */
struct argstruct {
    int verbose;
    char *filename;
};

/* Prototype for option handler */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure for argp functions */
static struct argp argp = { options, parse_opt, args_doc, doc };

/* Some RPM header handling bits */

/* headerImport guarantees a few things about the Header and its
 * contents:
 *   - Header is <= 0x0fffffff bytes (256MB) long
 *   - There's at least one tag, but no more than 0xffff (65535)
 *   - All tag entries have valid tagid and type
 *   - All tag offsets fall inside the header data
 *   - All tag types match the expected type for the tagid
 *   - All tag offsets are correctly aligned for their value type
 * So we can be a _little_ fast-and-loose with bounds checking etc - strlen()
 * will _always_ return a value less than headerSizeof(h, 0), even if the data
 * _inside_ the header is corrupt, because there's guaranteed to be at least
 * one NUL byte in the header's region-trailer tag. So you might get garbage,
 * but you won't walk off the end of the header and cause a segfault.
 */
#include "../lib/buf.h"
#include "../lib/compression/compression.h"
#include "../lib/libdino_internal.h"

Header rpmhdr_import(Buf *hbuf) {
    return headerImport(hbuf->buf, hbuf->size, HEADERIMPORT_FAST);
}
#define TAGSTR(hdr, tagname) headerGetString(h, RPMTAG##tagname)

Buf *sec_readbuf_a(Dino_Sec *sec, Dino_Size64 size, Dino_Off64 off) {
    Buf *buf = buf_init(size);
    if (!buf)
        return NULL;
    ssize_t bytes_read = pread(sec->dino->fd, buf->buf, size, sec->offset);
    if (bytes_read == size)
        return buf;
    /* TODO proper error handling... */
    buf_free(buf);
    return NULL;
}

/* So the basic thing we want to provide is something like rpmlib's
 * rpmReadPackageFile, which basically does:
 * - Read sig_blob/hdr_blob from fd
 * - verify all signatures / digests
 * - hdrblobImport(sig_blob, 0, &sig, ...);
 * - hdrblobImport(hdr_blob, 0, &hdr, ...);
 * - legacy fixups, endianness retrofits, refcount++
 * etc. etc. - but since RPM keeps headers uncompressed at all times, it
 * won't know how to deal with our compressed headers unless we:
 * a) implement FDIO_s etc from rpmio so we present a RPM-friendly interface
 * b) just uncompress the data and hand it to RPM already in memory
 * The latter is easier to implement so let's do that.
 */

int main(int argc, char *argv[]) {
    int fd, remaining, rv;
    struct argstruct args;

    /* Set argument defaults */
    args.verbose = 0;

    /* Default return value */
    rv = 0;

    /* Parse arguments */
    argp_parse(&argp, argc, argv, 0, &remaining, &args);

    fd = open(args.filename, O_RDONLY);
    if (fd == -1) {
        error(1, errno, N_("can't open '%s'"), args.filename);
    }

    Dino *dino = read_dino(fd);
    if (dino == NULL) {
        error(2, errno, N_("failed reading '%s'"), args.filename);
    }

    Dino_Index *rpmidx = get_index_byname(dino, ".rpmhdr.idx");
    if (rpmidx == NULL) {
        error(2, errno, N_("failed to load RPMHdr index"));
    }

    /* TODO: look up key(s), do stuff */

    close(fd);
    /* All finished - return and exit. */
    return rv;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct argstruct *args = state->input;
    switch (key) {
      case 'v':
        args->verbose = 1; break;
      case ARGP_KEY_ARG:
        if (state->arg_num >= 1) {
          argp_usage(state);
        } else {
          args->filename = arg;
        }
        break;
      case ARGP_KEY_END:
        if (state->arg_num < 1) {
          argp_usage(state);
        }
        break;
      default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

