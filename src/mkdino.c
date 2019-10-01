/* mkdino - make DINO objects.
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

/* TODO: eventually this shouldn't require RPM - all the RPM-related stuff
 * should be in its own tool, like rpm-ostree.
 * Gotta start somewhere, tho. */
#include <rpm/header.h>

#include "dinotools.h"

/* TODO: using internal bits of libdino probably isn't great */
#include "../lib/fileio.h"
#include "../lib/array.h"
#include "../lib/buf.h"
#include "../lib/digest.h"
#include "../lib/compression/compression.h"

/* Program version */
const char *argp_program_version = "mkdino 0.1";

/* Short program description */
static const char doc[] = N_("\
Make a DINO object from a set of RPMs.");

/* String for program arguments, used in help text */
static const char args_doc[] = N_("DINOFILE RPM...");

enum long_opts_e {
    ARG_COMPRESS_LEVEL = 1,
    ARG_RPM_NODIGEST,
    ARG_RPM_NOFILEDIGEST,
    ARG_INDEX_VARINT,
    ARG_INDEX_UNCSIZE,
    ARG_INDEX_COMPRESS,
    ARG_INDEX_NOFANOUT,
};

/* The options we understand */
static const struct argp_option options[] =
{
    { "verbose",    'v', 0,          0, "Produce verbose output" },
    /* TODO: -vv or other option for "also gimme timing/profiling info" */
    /* TODO: quiet, silent */

    { 0,0,0,0, "RPM options:" },
    { "nodigest",     ARG_RPM_NODIGEST, 0, 0, "Don't check RPM header/payload digests" },
    { "nofiledigest", ARG_RPM_NOFILEDIGEST, 0, 0, "Don't check file digests" },
    /* TODO: nodigest */
    /* TODO: check-rpm-sigs */
    /* TODO: keep-lead */
    /* TODO: flag to ignore payload digest */
    /* TODO: flag to check filedigests */
    /* TODO: flag to allow packages without filedigests */
    /* TODO: option for behavior on digest mismatch (skip, abort) */

    { 0,0,0,0, "Compression options:" },
    { "compressor", 'c', "COMPNAME", 0, "Compress using COMPNAME" },
    /* TODO: show libdino_compression_available[] */
    { "compress-level", ARG_COMPRESS_LEVEL, "#", 0, "Compression level preset (typically 0-9)" },
    /* TODO figure out how to show '-#' rather than each individual number */
    {0,'1',0,OPTION_HIDDEN}, {0,'2',0,OPTION_HIDDEN}, {0,'3',0,OPTION_HIDDEN},
    {0,'4',0,OPTION_HIDDEN}, {0,'5',0,OPTION_HIDDEN}, {0,'6',0,OPTION_HIDDEN},
    {0,'7',0,OPTION_HIDDEN}, {0,'8',0,OPTION_HIDDEN}, {0,'9',0,OPTION_HIDDEN},

    { 0,0,0,0, "Index options:" },
    { "index-varint",   ARG_INDEX_VARINT,   0, 0, "Use variable-length integers for size/offset" },
    { "index-compress", ARG_INDEX_COMPRESS, 0, 0, "Compress index section (usually unhelpful)" },
    { "index-unc-size", ARG_INDEX_UNCSIZE,  0, 0, "Add \"unc_size\" field for uncompressed data" },
    { "index-nofanout", ARG_INDEX_NOFANOUT, 0, 0, "Do not include fanout table in index" },
    /* TODO: force-64bit? */

    /* FUTURE OPTIONS */
    /* Section ordering */
    /* Generate/store alternate digests */
    /* Generate/store delta of existing payload vs. reconstructed payload */
    /* CPU threads */
    /* Adding signatures? */

    { 0,0,0,0, "Help/usage switches:", -1 },
    /* Automagic options go here */

    { 0 }
};

/* TODO FIXME: rpmvs has all this already.. */
enum {
    RPM_DIGEST     = 1<<1,
    RPM_FILEDIGEST = 1<<2,
};

/* A struct to hold program options */
struct argstruct {
    int verbose;

    int compresslevel;
    Dino_CompressID compress_id;

    int rpmverify;           /* DIGEST, FILEDIGEST */

    Dino_Secflags idx_flags; /* VARINT, COMPRESS */
    Dino_Secinfo idx_info;   /* UNC_SIZE, FANOUT, 64BIT */

    char *filename;
    Array *rpms;
};

/* Prototype for option handler */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure for argp functions */
static struct argp argp = { options, parse_opt, args_doc, doc };

/* Actual option parsing */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct argstruct *args = state->input;
    unsigned long n;
    char *endp;

    switch (key) {
      case 'v':
        args->verbose = 1; break;

      case ARG_RPM_NODIGEST:
        args->rpmverify &= (~RPM_DIGEST); break;
      case ARG_RPM_NOFILEDIGEST:
        args->rpmverify &= (~RPM_FILEDIGEST); break;

      case ARG_INDEX_NOFANOUT:
        args->idx_info |= DINO_IDX_FLAG_NOFANOUT; break;
      case ARG_INDEX_UNCSIZE:
        args->idx_info |= DINO_IDX_FLAG_UNC_SIZE; break;

      case ARG_INDEX_VARINT:
        args->idx_flags |= DINO_FLAG_VARINT; break;
      case ARG_INDEX_COMPRESS:
        args->idx_flags |= DINO_FLAG_COMPRESSED; break;

      case ARG_COMPRESS_LEVEL:
        n = strtoul(arg, &endp, 10);
        if (*endp || n > 256)
            argp_error(state, N_("invalid --compress-level value '%s'"), arg);
        args->compresslevel = n;
        break;

      case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9':
        args->compresslevel = key;
        break;

      case ARGP_KEY_ARG:
        if (state->arg_num == 0)
            args->filename = arg;
        else
            array_append(args->rpms, &arg);
        break;
      case ARGP_KEY_END:
        if (state->arg_num < 2)
            argp_usage(state);
        break;
      default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* RPM header handling bits */
#define TAGSTR(hdr, tagname) headerGetString(h, RPMTAG##tagname)

/* RPM doesn't expose the internal buffer of a Header, and it doesn't export
 * the function it uses to _read_ that buffer (hdrblobRead()), but it _does_
 * export headerImport() - which does the same sanity checks as headerRead().
 * So, if we can read the header ourselves, we can pass it to headerImport()
 * and everything should be just fine. */
typedef struct HeaderBuf {
    void *buf;
    unsigned int size;
    Header hdr;
} HeaderBuf;

HeaderBuf *headerReadRaw(FD_t fd, int is_sig) {
    int32_t block[4];
    HeaderBuf *h = calloc(1, sizeof(HeaderBuf));
    if (!h)
        return NULL;

    if (Fread(block, sizeof(int32_t), 4, fd) <= 0)
        goto fail;
    if (memcmp(block, rpm_header_magic, sizeof(rpm_header_magic)))
        goto fail;

    int32_t il = be32toh(block[2]);
    int32_t dl = be32toh(block[3]);
    /* TODO: check ranges */
    size_t secsize = (il << 4) + dl;
    h->size = sizeof(il)+sizeof(dl)+secsize;
    h->buf = malloc(h->size);
    if (!h->buf)
        goto fail;
    memcpy(h->buf, &block[2], 8);
    if (Fread(h->buf+8, secsize, 1, fd) != secsize)
        goto fail;
    if (is_sig) {
        size_t padsize = (8 - (secsize % 8)) % 8;
        if (padsize && (Fread(block, padsize, 1, fd) != padsize)) {
            goto fail;
        }
    }
    return h;
fail:
    free(h->buf);
    free(h);
    return NULL;
}

void headerBufFree(HeaderBuf *h) {
    if (!h)
        return;
    if (h->hdr)
        headerFree(h->hdr);
    else
        free(h->buf);
    free(h);
}

#define PAD_PAGE(s) (((s)+0xfff)&~0xfff)
/* TODO: better logging than this.. */
#define VERBOSE_PRINTF(fmt, vargs...) (args.verbose ? printf(fmt, vargs) : 0)

int main(int argc, char *argv[]) {
    //int fd;
    int remaining, rv;
    struct argstruct args;

    /* Set argument defaults */
    args.verbose = 0;
    args.idx_info = 0;
    args.idx_flags = 0;
    args.compress_id = DINO_COMPRESS_ZSTD;
    args.compresslevel = 6;
    args.rpmverify = RPM_DIGEST | RPM_FILEDIGEST;
    args.rpms = array_with_capacity(sizeof(char*), argc);

    /* Default return value */
    rv = 0;

    /* Parse arguments */
    argp_parse(&argp, argc, argv, 0, &remaining, &args);

    VERBOSE_PRINTF("%s starting...\n", argp_program_version);
    VERBOSE_PRINTF("  files to read: %lu\n", array_len(args.rpms));
    VERBOSE_PRINTF("  compression: %s -%u\n", compress_name(args.compress_id), args.compresslevel);

    Dino_DigestID digestid = DINO_DIGEST_SHA256;
    Dino_Idx_Keysize keysize = digest_size(digestid);
    Hasher *hasher = hasher_create(digestid);
    uint8_t *digest = malloc(keysize);
    char *hexdigest = malloc(keysize<<1);

    Dino_CStream *cs = cstream_create(args.compress_id);
    //TODO: cstream_setlevel(cs, args.compresslevel);
    outBuf *outbuf = buf_init(cs->rec_outbuf_size);
    inBuf inbuf = {NULL, 0, 0};

    if (!(digest && hexdigest && outbuf && cs))
        error(ENOMEM, errno, N_("couldn't allocate memory"));

    /* TODO: progress indicator */
    for (unsigned i=0; i<array_len(args.rpms); i++) {
        char *rpmfn = *(char **)array_get(args.rpms, i);
        VERBOSE_PRINTF("%u/%lu %s\n", i+1, array_len(args.rpms), rpmfn);

        FD_t fd = Fopen(rpmfn, "r"); // FIXME: is this a good mode?
        Fseek(fd, 0x60, 0);
        VERBOSE_PRINTF("  rpm lead:        %8li bytes\n", Ftell(fd));

        HeaderBuf *sigbuf = headerReadRaw(fd, 1);
        VERBOSE_PRINTF("  rpm sig size:    %8u bytes\n", sigbuf->size+8);
        VERBOSE_PRINTF("  rpm sig padding: %8li bytes\n", Ftell(fd)-(0x60+sigbuf->size+8));
        VERBOSE_PRINTF("  rpm hdr offset:  %8li\n", Ftell(fd));

        HeaderBuf *hdrbuf = headerReadRaw(fd, 0);
        VERBOSE_PRINTF("  rpm hdr size:    %8u bytes\n", hdrbuf->size+8);
        VERBOSE_PRINTF("  payload offset:  %8li bytes\n", Ftell(fd));

        /* TODO: multithreaded hashing/compressing! */

        /* This is how you verify the digests in the sig hdr... */
        hasher_start(hasher);
        hasher_update(hasher, rpm_header_magic, 8);
        hasher_update(hasher, hdrbuf->buf, hdrbuf->size);
        hasher_finish(hasher, digest);
        key2hex(digest, keysize, hexdigest);

        /* And here's how we compress headers... */
        size_t input_size = hdrbuf->size + sigbuf->size;
        VERBOSE_PRINTF("  sig+hdr combined: %7lu bytes\n", input_size);
        size_t outbuf_size = PAD_PAGE(input_size);
        if (outbuf_size > outbuf->size)
            buf_realloc(outbuf, outbuf_size);
        cstream_compress_start(cs, input_size);
        inbuf.buf = sigbuf->buf;
        inbuf.pos = 0;
        inbuf.size = sigbuf->size;
        cstream_compress(cs, &inbuf, outbuf);
        inbuf.buf = hdrbuf->buf;
        inbuf.pos = 0;
        inbuf.size = hdrbuf->size;
        cstream_compress(cs, &inbuf, outbuf);
        cstream_compress_end(cs, outbuf);
        VERBOSE_PRINTF("  sig+hdr compressed: %5lu bytes\n", outbuf->pos);

        /* NOTE!! headerImport modifies the underlying buffer, which is why we
         * have to do the digest *before* this if we want it to match */
        sigbuf->hdr = headerImport(sigbuf->buf, sigbuf->size, 0);
        hdrbuf->hdr = headerImport(hdrbuf->buf, hdrbuf->size, 0);

        VERBOSE_PRINTF("  rpm ENVRA:         %lu:%s-%s-%s.%s\n",
                headerGetNumber(hdrbuf->hdr, RPMTAG_EPOCH),
                headerGetString(hdrbuf->hdr, RPMTAG_NAME),
                headerGetString(hdrbuf->hdr, RPMTAG_VERSION),
                headerGetString(hdrbuf->hdr, RPMTAG_RELEASE),
                headerGetString(hdrbuf->hdr, RPMTAG_ARCH));
        VERBOSE_PRINTF("  rpm SIGTAG_SHA256: %s\n", headerGetString(sigbuf->hdr, RPMSIGTAG_SHA256));
        VERBOSE_PRINTF("  rpm header digest: %s\n", hexdigest);

        /* We have the compressed headers (outbuf), their size (outbuf->size),
         * the uncompressed size (input_size), and their index key (digest). */

        /* TODO: write outbuf to RPMHdr section data */
        /* TODO: add digest + size/unc_size to index */
        /* TODO: iterate through package payload:
         *       ( uncompress, digest, compress, ...), finalize, index */

        /* Clean up before next RPM */
        headerBufFree(hdrbuf);
        headerBufFree(sigbuf);
        buf_clear(outbuf);

        Fclose(fd);
    }

    hasher_free(hasher);
    cstream_free(cs);
    buf_free(outbuf);
    free(digest);
    free(hexdigest);

    /* All finished - return and exit. */
    return rv;
}

