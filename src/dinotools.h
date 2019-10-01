/* dinotools.h - functions/definitions common to all DINO tools */

#ifndef _DINOTOOLS_H
#define _DINOTOOLS_H

#include "../lib/libdino.h"
#include "../lib/common.h"

/* TODO: locale */
/* TODO: gettext */
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

/* These id->string conversions should probably be in libdino? */
extern const char *compressid_name[DINO_COMPRESSNUM+1];
extern const char *objtype_name[DINO_OBJTYPENUM+1];

#define dino_objtype_name(i) (objtype_name[MIN((i),DINO_OBJTYPENUM+1)])
#define dino_compressid_name(i) (compressid_name[MIN((i),DINO_COMPRESSNUM+1)])

#define dhdr_secoff(dhdr) ((Dino_Secidx)sizeof(*dhdr))
#define dhdr_namoff(dhdr) ((Dino_NameOffset)sizeof(*dhdr)+dhdr->sectab_size)

/* happy hex handling helpers hereafter */

int key2hex(const Dino_Idx_Key *key, const size_t keysize, char *hex);
int hex2key(const char *hex, const size_t hexsize, Dino_Idx_Key *key);
int hexchrval(const char hexchr);

/* like above but allocates space for the string and returns it.
 * caller is responsible for freeing the string. */
char *key2hex_a(const Dino_Idx_Key *key, const size_t keysize);
Dino_Idx_Key *hex2key_a(const char *hex, const size_t hexsize);

/* canonicalize a hexstr in-place (lowercase chars).
 * Returns 1 if successful or 0 if the string is not hex.
 * sets len to the length of the string.
 * TODO: better docs/NULL handling */
int canonicalize_hexstr(char *hex, unsigned *len);


#endif /* _DINOTOOLS_H */
