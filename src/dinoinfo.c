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
//#include <assert.h>
//#include <stdio.h>

#include "../lib/libdino.h"

/* TODO: locale */
/* TODO: gettext */
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

/* Program version */
const char *argp_program_version = "dinoinfo 0.1";

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

static const char hexchars[] = "0123456789abcdef";

int key2hex(const Dino_Idx_Key *key, const size_t keysize, char *hex) {
    for (int i=0; i<keysize; i++) {
        hex[i<<1]     = hexchars[key[i] >> 4];
        hex[(i<<1)+1] = hexchars[key[i] & 0x0f];
    }
    hex[keysize<<1] = '\0';
    return keysize<<1;
}

char *akey2hex(const Dino_Idx_Key *key, const size_t keysize) {
    if (key == NULL || keysize == 0)
        return NULL;
    char *out = malloc((keysize<<1)+1);
    if (out == NULL)
        return NULL;
    key2hex(key, keysize, out);
    return out;
}

int main(int argc, char *argv[]) {
    int remaining, fd, rv = 0;
    struct argstruct args;
    Dino *dino;
    Dino_Dhdr *dhdr;
    Dino_Shdr *shdr;

    /* Set arg defaults */
    args.verbose = 0;

    /* Parse arguments */
    argp_parse(&argp, argc, argv, 0, &remaining, &args);

    fd = open(args.filename, O_RDONLY);
    if (fd == -1) {
        error(1, errno, N_("can't open '%s'"), args.filename);
    }

    dino = read_dino(fd);
    if (dino == NULL) {
        error(2, errno, N_("failed reading '%s'"), args.filename);
    }

    /* TODO: do I really have to do this manually? */
    if (load_indexes(dino) < 0) {
        error(2, errno, N_("failed reading indexes in '%s'"), args.filename);
    }

    dhdr = get_dhdr(dino);
    printf("section_count=%u sectab_size=%u namtab_size=%u\n",
            dhdr->section_count, dhdr->sectab_size, dhdr->namtab_size);

    printf("  %3s %-16s %-8s %-8s\n",
            N_("idx"), N_("name"), N_("size"), N_("type"));
    for (int i=0; i<dhdr->section_count; i++) {
        shdr = get_shdr(dino, i);
        printf("  %3u %-16s %08x %-8u\n",
                i, get_name(dino, shdr->name), shdr->size, shdr->type);
    }

    /* Dump info from the index */
    Dino_Secidx rpmidxsec = 0; /* FIXME: need section lookup... */
    uint8_t keysize = 32; /* FIXME: need to be able to get keysize too... */
    Dino_Index *rpmidx = get_index(dino, rpmidxsec);
    Dino_Idx_Cnt keycount = index_get_cnt(rpmidx);
    Dino_Idx_Key *k;
    Dino_Idx_Val *v;
    char *hexkey = malloc(keysize+1); /* FIXME: keysize... */
    for (int i=0; i<keycount; i++) {
        k = index_get_key(rpmidx, i);
        v = index_get_val(rpmidx, i);
        key2hex(k, 4, hexkey);
        printf(" rpm %s size %08x offset %08x\n",
                hexkey, v->size, v->offset);
    }
    const uint8_t partkey[] = { 0xcc, 0xa3, 0x91, 0x80 };
    key2hex(partkey, 4, hexkey);
    printf("\ntrying key match for partial key %s:\n", hexkey);
    k = index_key_match(rpmidx, partkey, 4);
    if (k == NULL)
        printf(" BOOOOO NO MATCH, BOGUS\n");
    else {
        key2hex(k, keysize, hexkey);
        printf(" %s\n", hexkey);
    }

    free(hexkey);


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

