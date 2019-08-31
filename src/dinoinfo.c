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
    dhdr = get_dhdr(dino);

    printf("section_count=%u sectab_size=%u namtab_size=%u\n",
            dhdr->section_count, dhdr->sectab_size, dhdr->namtab_size);
    printf("  %3s %16s %8s %8s\n",
            N_("idx"), N_("name"), N_("size"), N_("type"));
    for (int i=0; i<dhdr->section_count; i++) {
        shdr = get_shdr(dino, i);
        printf("  %3u %16s %8u %8u\n",
                i, get_name(dino, shdr->name), shdr->size, shdr->type);
    }

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

