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
#include <string.h>

#include "dinotools.h"

/* Program version */
const char *argp_program_version = "dinoinfo 0.1";

/* Short program description */
static const char doc[] = N_("\
Print information from DINO file in human-readable form.");

/* String for program arguments, used in help text */
static const char args_doc[] = N_("FILE");

enum long_opt_keys {
    ARGP_KEY_ABBREV   = 1,
    ARGP_KEY_NOABBREV = 2,
};

/* The options we understand */
static const struct argp_option options[] =
{
    { 0,0,0,0, "At least one of the following switches must be given:" },
    { "file-headers",    'f', 0, 0, "Show the contents of the overall file header" },
    { "section-headers", 's', 0, 0, "Show the contents of the section header table" },
    { "sectab",           0,  0, OPTION_ALIAS|OPTION_HIDDEN },
    { "nametable",       'n', 0, 0, "Show the contents of the object nametable" },
    { "namtab",           0,  0, OPTION_ALIAS|OPTION_HIDDEN },
    /* TODO: index headers vs. index contents */
    { "indexes",         'i', 0, 0, "Show the contents of index sections" },
    { "all-headers",     'a', 0, 0, "Show the contents of all headers and indexes" },

    { 0,0,0,0, "Output format options:" },
    { "abbrev-key",       ARGP_KEY_ABBREV,   "N", 0, "Abbreviate keys to N characters (default: 8)" },
    { "no-abbrev-key",    ARGP_KEY_NOABBREV,  0,  0, "Show full hexadecimal keys" },
    { "verbose",         'v', 0, 0, "Produce verbose output" },

    { 0,0,0,0, "Filtering items:" },
    { "section",         'j', "NAME",    0, "Only show info for section NAME" },
    { "key",             'k', "KEY",     0, "Show index info for matching KEY" },
    { "key-exact",       'K', "FULLKEY", 0, "Show index info for FULLKEY" },

    { 0,0,0,0, "Help/usage switches:", -1 },
    /* Automagic options go in group -1 */

    { 0 }
};

enum section_select_enum {
    SHOW_FILEHEADERS    = 1<<0,
    SHOW_SECTIONHEADERS = 1<<1,
    SHOW_NAMETABLE      = 1<<2,
    SHOW_INDEXES        = 1<<3,
    SHOW_ALL            = 0xff, /* enh good enough */
};

enum key_match_enum {
    MATCH_PREFIX = 0,
    MATCH_SUBSTR = 1,
    MATCH_EXACT  = 2,
};

#define KEYSTR_MAXLEN 256

/* A struct to hold program options */
struct argstruct {
    int verbose;

    unsigned show;      /* SHOW_* flags */
    unsigned keymatch;  /* MATCH_* mode */

    unsigned abbrevkey; /* how many chars to abbreviate the key */

    char *keystr;
    unsigned keystrlen;
    char *secname;
    char *filename;
};

/* Prototype for option handler */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure for argp functions */
static struct argp argp = { options, parse_opt, args_doc, doc };

void print_dhdr_info(Dino_Dhdr *dhdr) {
    printf("  type: 0x%02x (%s), version %u, arch 0x%02x (%s)\n",
            dhdr->type, dino_objtype_name(dhdr->type),
            dhdr->version,
            dhdr->arch, dhdr->arch ? "ignored" : "none");
    printf("  encoding: %s, %s-bit section offsets\n",
            (dhdr->encoding & DINO_ENCODING_LSB) ? "little-endian" :
            (dhdr->encoding & DINO_ENCODING_MSB) ? "big-endian" :
            (dhdr->encoding & DINO_ENCODING_WTF) ? "INVALID" :
            "undefined byte order",
            (dhdr->encoding & DINO_ENCODING_SEC64) ? "64" : "32");
    printf("  sectab: offset %04x size %04x count %u\n"
           "  namtab: offset %04x size %04x\n",
           dhdr_secoff(dhdr), dhdr->sectab_size, dhdr->section_count,
           dhdr_namoff(dhdr), dhdr->namtab_size);
    printf("  compression format: %s\n", dino_compressid_name(dhdr->compress_id));
}

int main(int argc, char *argv[]) {
    int remaining, fd, rv = 0;
    struct argstruct args;
    Dino *dino;
    Dino_Dhdr *dhdr;
    Dino_Shdr *shdr;

    /* Set arg defaults */
    args.verbose = 0;
    args.abbrevkey = 8;
    args.show = 0;
    args.keymatch = MATCH_PREFIX;
    args.keystrlen = 0;
    args.secname = NULL;
    args.filename = NULL;

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

    dhdr = get_dhdr(dino);

    if (args.show & SHOW_FILEHEADERS) {
        printf("DINO v%u Object Header:\n", DINO_MAGIC_VER(dhdr->magic));
        print_dhdr_info(dhdr);
    }

    /* TODO: flag to control size/offset format width */
    if (args.show & SHOW_SECTIONHEADERS) {
        printf("\n%s\n", N_("Section Headers:"));
        printf("  %3s %-4s %-16s %-8s %8s %16s\n",
               N_("idx"), N_("type"), N_("name"), N_("size"), N_("flags"), N_("info"));
        for (int i=0; i<dhdr->section_count; i++) {
            shdr = get_shdr(dino, i);
            printf("  %3u   %02x %-16s %08x ------%c%c %016x\n",
                    i, shdr->type, dino_getname(dino, shdr->name), shdr->size,
                    shdr->flags & DINO_FLAG_VARINT     ? 'v' : '-',
                    shdr->flags & DINO_FLAG_COMPRESSED ? 'c' : '-',
                    shdr->info);
        }
    }

    if (args.show & SHOW_NAMETABLE) {
        printf("\n%s\n", N_("Nametable:"));

        Dino_NameOffset off = 0;
        while (off < dhdr->namtab_size) {
            const char *name = dino_getname(dino, off);
            int len = strnlen(name, dhdr->namtab_size - off);
            printf("  %04x-%04x \"%s%s\"\n", off, off+len, name,
                    name[len]==0 ? "\\0" : "");
            off += len + (!name[len]);
        }
    }

    /* Load index data into memory */
    if (load_indexes(dino) < 0) {
        error(2, errno, N_("failed reading indexes in '%s'"), args.filename);
    }

    if (args.show & SHOW_INDEXES) {
        printf("\n%s\n", N_("Indexes:"));
        /* TODO: need better section iterators... */
        Dino_Secidx i = 0;
        Dino_Sec *idxsec;
        for (i=0; (idxsec=dino_getsec(dino, i)); i++) {
            Dino_Index *idx = get_index(dino, i);
            if (!idx)
                continue;

            uint8_t keysize = index_get_keysize(idx);
            Dino_Idx_Cnt keycount = index_get_cnt(idx);
            Dino_Sec *othersec = dino_get_index_othersec(dino, idx);
            /* TODO: Index flags */
            printf("  Index section %s -> %s, %u keys, keysize %u\n",
                    dino_secname(idxsec), dino_secname(othersec),
                    keycount, keysize);

            char *hexkey = malloc((keysize*2)+1);
            Dino_Idx_Key *k;
            Dino_Idx_Val32 *v;
            Dino_Idx_Range showkeys;

            if (args.keystr) {
                /* Show matching keys */
                /* TODO: MATCH_EXACT */
                Dino_Idx_Key *partkey = hex2key_a(args.keystr, args.keystrlen);
                uint8_t partkeylen = args.keystrlen>>1;
                showkeys = index_key_match(idx, partkey, partkeylen);
                /* TODO: if args.keystrlen & 1, we ignored 4 bits of keystr */
                free(partkey);
            } else {
                /* Show all keys */
                showkeys.lo=0;
                showkeys.hi=index_get_cnt(idx)-1;
            }
            for (int i=showkeys.lo; i<=showkeys.hi; i++) {
                k = index_get_key(idx, i);
                /* FIXME: different val sizes / formats */
                v = index_get_val32(idx, i);
                key2hex(k, keysize, hexkey);
                printf("    key %.*s size %08x offset %08x\n",
                        args.abbrevkey ? MIN(args.abbrevkey, keysize<<1) : keysize<<1, hexkey,
                        v->size, v->offset);
            }
            printf("\n");
        }
    }


    close(fd);
    /* All finished - return and exit. */
    return rv;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct argstruct *args = state->input;
    char *endp;
    unsigned long n;
    switch (key) {
      case 'v':
        args->verbose = 1; break;
      case 'f':
        args->show |= SHOW_FILEHEADERS; break;
      case 's':
        args->show |= SHOW_SECTIONHEADERS; break;
      case 'i':
        args->show |= SHOW_INDEXES; break;
      case 'n':
        args->show |= SHOW_NAMETABLE; break;
      case 'a':
        args->show |= SHOW_ALL; break;
      case 'j':
        args->secname = arg; break;

      case 'K': args->keymatch = MATCH_EXACT;
        /* fallthru! */
      case 'k':
        /* TODO: work more like strtoul so we can point out the bad char */
        if (!canonicalize_hexstr(arg, &args->keystrlen))
            argp_error(state, N_("invalid hex key '%s'"), arg);
        args->keystr = arg;
        break;

      case ARGP_KEY_ABBREV:
        n = strtoul(arg, &endp, 10);
        if (*endp || n > 256)
            argp_error(state, N_("invalid --abbrev-key value '%s'"), arg);
        args->abbrevkey = n;
        break;
      case ARGP_KEY_NOABBREV:
        args->abbrevkey = 0;
        break;

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

