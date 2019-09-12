/* dino.h - define standard DINO types, structures, identifiers, etc.
 * Copyright 2019 Red Hat, Inc.
 * Author: Will Woods <wwoods@redhat.com>
 *
 * Released under the terms of the LGPLv2.1 (FIXME: proper boilerplate!)
 *
 */

#ifndef _DINO_H
#define _DINO_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Type of section indexes. 256 sections is good enough for Mach-O... */
typedef uint8_t Dino_Secidx;
#define DINO_SEC_MAXCNT 256
#define DINO_SEC_MAXIDX 255


/* Section offsets/sizes are 32 bits by default. */
typedef uint32_t Dino_Offset;
typedef Dino_Offset Dino_Size;

/* If DINO_ENCODING_SEC64 is set, then Dino_Size values in Shdr with their
 * MSB set are actually indexes into a table of Dino_Size64 values that
 * follows the Shdr table. */
#define DINO_SIZE_IS_64(x) (x & 0x80000000)
#define DINO_SIZE64_IDX(x) (x & 0x7fffffff)
#define DINO_SIZE64_INVALID (~0ULL)

/* 64-bit offsets/sizes. Uncommon, but valid. */
typedef uint64_t Dino_Off64;
typedef Dino_Off64 Dino_Size64;


/* Offset into the section name table.
 * Section names are limited to 256 bytes, so 16 bits is enough. */
typedef uint16_t Dino_NameOffset;


/* Size of the section table. We guarantee that section table entries will
 * always be <= 256 bytes, so 16 bits is enough here. */
typedef uint16_t Dino_SectabSize;


/* Compression algorithm identifiers */
typedef enum Dino_CompressID_e {
    DINO_COMPRESS_NONE = 0,
    DINO_COMPRESS_ZLIB = 1,
    DINO_COMPRESS_LZMA = 2,
    DINO_COMPRESS_LZO  = 3,
    DINO_COMPRESS_XZ   = 4,
    DINO_COMPRESS_LZ4  = 5,
    DINO_COMPRESS_ZSTD = 6,
    /* ... */
    DINO_COMPRESS_INVALID = 255,
} Dino_CompressID_e;
typedef uint8_t Dino_CompressID;
#define DINO_COMPRESSNUM 7

/* Digest algorithm identifiers.
 * The values match OpenPGP/RPM PGPHASHALGO_* values (RFC4880, rpmpgp.h).
 * MD5 and SHA1 are listed for compatibility with other tools and formats
 * that might still use them, but should not be used by DINO.
 * SHA256 and SHA512 are required; SHA256 is our default. */
typedef enum Dino_DigestID_e {
    DINO_DIGEST_UNKNOWN   = 0,
    DINO_DIGEST_MD5       = 1, /* Broken. Do not use. */
    DINO_DIGEST_SHA1      = 2, /* Insecure, deprecated. */
    DINO_DIGEST_RIPEMD160 = 3, /* RIPEMD160 support is optional */
    /* 4-7 were used for now-deprecated algorithms and are "Reserved" */
    DINO_DIGEST_SHA256    = 8,
    DINO_DIGEST_SHA384    = 9,
    DINO_DIGEST_SHA512    = 10,
    DINO_DIGEST_SHA224    = 11,
} Dino_DigestID_e;
typedef uint8_t Dino_DigestID;
#define DINO_DIGESTNUM 12


/* Architecture identifiers.
 * The values match ELF e_machine EM_* values (elf.h).
 * This isn't an exhaustive or authoritative list, but it covers everything
 * that RPM knows about. */
typedef enum Dino_Arch_e {
    DINO_ARCH_NONE    = 0,
    DINO_ARCH_SPARC   = 2,
    DINO_ARCH_X86     = 3,
    DINO_ARCH_MIPS    = 8,
    DINO_ARCH_PPC     = 20,
    DINO_ARCH_PPC64   = 21,
    DINO_ARCH_S390    = 22,
    DINO_ARCH_ARM     = 40,
    DINO_ARCH_ALPHA   = 41,
    DINO_ARCH_SH      = 42,
    DINO_ARCH_SPARCV9 = 43,
    DINO_ARCH_IA64    = 50,
    DINO_ARCH_MIPSX   = 51,
    DINO_ARCH_X86_64  = 62,
    DINO_ARCH_NDS32   = 167,
    DINO_ARCH_AARCH64 = 183,
    DINO_ARCH_RISCV   = 243,
} Dino_Arch_e;
typedef uint8_t Dino_Arch;


/* Encoding options.
 * This byte describes the encoding of the headers.
 * It's basically equivalent to ELF EI_CLASS and EI_DATA. */
typedef enum Dino_Encoding_e {
    /* Low two bits give endianness, same values as ELFDATA{LSB,MSB} */
    DINO_ENCODING_INVALID = 0, /* No value means this is an invalid file. */
    DINO_ENCODING_LSB     = 1, /* LSB = little-endian. Default encoding. */
    DINO_ENCODING_MSB     = 2, /* MSB = big-endian. */
    DINO_ENCODING_WTF     = 3, /* Also an invalid value! */
    DINO_ENCODING_SEC64   = 4, /* 64bit section size/counts are present */
    /* The rest is reserved for future use:
     * Variable-width integer encodings?
     * Section alignment/padding? */
} Dino_Encoding_e;
typedef uint8_t Dino_Encoding;


/* Object type identifiers. */
/* XXX NOTE: this is all aspirational; none of these are implemented, their
 * semantics are all totally undefined, the names might change, etc.
 * Just use ARCHIVE for now. */
typedef enum Dino_Objtype_e {
    DINO_TYPE_UNKNOWN     = 0, /* This should never be used in actual files */
    DINO_TYPE_ARCHIVE     = 1, /* Archive/data container */
    DINO_TYPE_COMPONENT   = 2, /* Shared objects used to build dynamic images */
    DINO_TYPE_DYNIMG      = 3, /* Dynamic image description */
    DINO_TYPE_APPLICATION = 4, /* Dynamic image with an entrypoint */
    DINO_TYPE_DUMP        = 5, /* Dump of a dynamic image */
} Dino_Objtype_e;
typedef uint8_t Dino_Objtype;


/* The main DINO header - Dhdr for short */
typedef struct
{
    uint8_t         magic[4];      /* Magic number (and file format major version) */
    uint8_t         version;       /* File format version (0=experimental) */
    Dino_Arch       arch;          /* Architecture */
    Dino_Encoding   encoding;      /* File encoding (byte order, 32/64 bit, etc.) */
    Dino_Objtype    type;          /* Object file type */
    Dino_CompressID compress_id;   /* Compression algorithm used in this object */
    Dino_Secidx     compress_opts; /* Section containing extra compressor options */
    uint8_t         reserved;      /* Pad byte, reserved for future expansion */
    Dino_Secidx     section_count; /* Number of sections in the section table */
    Dino_SectabSize sectab_size;   /* Size of the section table (inc. sec64) */
    Dino_NameOffset namtab_size;   /* Size of the section nametable */
} Dino_Dhdr;

/* Hey look it's our magic bytes. */
#define DINO_MAGIC_V0 "\xed\xab\xee\xf0"


/* Section flags common to all section types */
typedef enum Dino_Secflags_e {
    DINO_FLAG_COMPRESSED = 1 << 0, /* bit 0: is this section compressed? */
} Dino_Secflags_e;
typedef uint8_t Dino_Secflags;


/* Section-specific data/flags/etc. */
typedef uint32_t Dino_Secinfo;
/* TODO: could probably have a union here... */




/* Section type identifiers */
/* TODO: this is in progress - NULL and BLOB are okay but nothing else is
 * implemented or defined yet, so don't use them. */
typedef enum Dino_Sectype_e {
    DINO_SEC_NULL     = 0x00, /* Invalid/empty section */
    DINO_SEC_BLOB     = 0x01, /* Opaque blob of binary data */
    DINO_SEC_INDEX    = 0x02, /* A generic index over another section */
    DINO_SEC_STRTAB   = 0x03, /* A table of strings prefixed with lengths */
    DINO_SEC_CSTRTAB  = 0x04, /* Table of NUL-terminated UTF8 strings */
    DINO_SEC_NOTE     = 0x05, /* ELF-style NOTE section */

    DINO_SEC_PROVS    = 0x06, /* Symbols this object provides */
    DINO_SEC_DEPS     = 0x07, /* Symbols this object requires */

    DINO_SEC_FILESYS  = 0x08, /* A filesystem archive/image */

    /* TODO: DINO-specific filesystem/archive/packfile format.
     * Use separate sections for:
     *   FILEDATA: hashes+contents - just the data
     *   FILESTAT: basic filesystem metadata (mode, timestamps, etc)
     *   FILETREE: file names/directory entries/symbolic paths/tree objects
     *   FILEMETA: extended file info / attributes (tags, classes, etc)
     * Deduplicate data/stat/tree/meta objects as much as possible
     * Use deltas to store similar files
     * Each file object should be in its own compression frame so individual
     * files/objects can be fetched and decompressed
     * Put offsets etc. in indexes so network clients can range-request
     * Handle path ownership/visibility/etc. like ELF handles symbols */
    DINO_SEC_FILEDATA = 0x10, /* File contents */
    DINO_SEC_FILESTAT = 0x11, /* File stat data (mode, mtime(?), etc) */
    DINO_SEC_FILETREE = 0x12, /* File paths / directory entries */
    DINO_SEC_FILEMETA = 0x13, /* File metadata (tags, fileclass, etc) */

    /* TODO: pkginfo, pkgdata, build objects... */

    /* 0x60 - 0x6f reserved for OS-specific extensions */
    DINO_SEC_LOOS     = 0x60,
    DINO_SEC_HIOS     = 0x6f,

    /* 0x70 - 0x7f reserved for back/cross-compat with other tools */
    DINO_SEC_RPMHDR   = 0x7f, /* RPM header data */
    DINO_SEC_LOCOMPAT = 0x70,
    DINO_SEC_HICOMPAT = 0x7f,

    /* 0x80 - 0xff reserved for user extensions. Have fun, kids! */
    DINO_SEC_LOUSER   = 0x80,
    DINO_SEC_HIUSER   = 0xff,
} Dino_Sectype_e;
typedef uint8_t Dino_Sectype;


/* Section table entry, also called a Shdr.
 *
 * By default, sections are not padded/aligned.
 * Section 0 starts immediately after Dhdr+sectab+namtab and is `size` bytes
 * long, section 1 starts immediately after section 0, etc.
 *
 * If DINO_ENCODING_SEC64 is set, the on-disk sectab still consists of
 * Dino_Shdr structs, but there will also be a table of Dino_Size64 values
 * immediately following the Shdr entries. (The size of this table is included
 * in dhdr.sectab_size.) In this case, any `size` or `count` item that has its
 * MSB set is an index into that table.
 */
typedef struct
{
    Dino_NameOffset name;  /* Offset of this section's name in the nametable */
    Dino_Sectype    type;  /* Section type */
    Dino_Secflags   flags; /* Flags common to all section types (compression etc) */
    Dino_Secinfo    info;  /* Section-specific info */
    Dino_Size       size;  /* Size (in bytes) of this section */
    Dino_Size       count; /* Item count (or other section-specific data) */
} Dino_Shdr;

/* Shdr with 64-bit size/count values.
 * This is _not_ used on-disk - see note about DINO_ENCODING_SEC64 above. */
typedef struct
{
    Dino_NameOffset name;  /* Offset of this section's name in the nametable */
    Dino_Sectype    type;  /* Section type */
    Dino_Secflags   flags; /* Flags common to all section types (compression etc) */
    Dino_Secinfo    info;  /* Section-specific info */
    Dino_Size64     size;  /* Size (in bytes) of this section */
    Dino_Size64     count; /* Item count (or other section-specific data) */
} Dino_Shdr64;


/* A note about large files:
 *
 * Currently, each section is limited to a 32-bit size (4GB). We also have a
 * maximum of 256 sections, which gives us a maximum file size of 1TB.
 * The vast majority of use cases for DINO will not exceed these sizes.
 *
 * ELF supports fully 64-bit objects, but ELF is designed to be mapped directly
 * into memory, which makes using 64-bit offset/size fields sensible even if
 * the values use less than 32 bits 99.9% of the time.
 *
 * One of DINO's primary goals is to be a space-efficient on-disk format, so if
 * we only need 32 bits 99.9% of the time, then we should use a 32-bit value
 * 99.9% of the time.
 *
 * (In fact, most of the values we're storing in 32-bit fields use way less
 * than 32 bits. A future version of DINO could use variable-length integer
 * encoding to pack data even more efficiently, but fixed-width structures
 * have the advantage of being simpler and safer to parse, so I don't think
 * the complexity tradeoff is worth it. Yet.)
 */

/* File archive format, version 0!
 *
 * NOTES / CONSIDERATIONS:
 * 1. Given two builds of the same package, the only things likely to change are
 *    the file sizes and digests. All the rest of the data is probably unchanged.
 * 2. Every directory has an owner (package) and a parent (directory).
 * 3. Paths that are exported should have symbolic names so we can relocate.
 * 4. Nearly all of the stat / cpio header data is optional/predictable,
 *    so we don't need most of it for each file:
 *    - inode and dev aren't relevant
 *    - for 90+% of files the mtime is within seconds of the package timestamp
 *    - for ~99.3% of files, UID/GID is 0/0
 *    - ~99.5% of modes are 100644, 100755, 120777, or 040755
 *      - that's file, exe, link, dir
 *      - no blk, fifo, socket in F30; 8 chr devs though (??)
 *
 * Given all that, I suggest we store file data as follows:
 *       /---------------  index  -----------\/--  payloads --\
 * +-----+--------+---------+-------+--------+-----------...--+
 * | hdr | fanout | digests | sizes | modes? | filedata  ...  |
 * +-----+--------+---------+-------+--------+-----------...--+
 *
 * This is basically the git .idx/.pack format(s) smushed together, with some
 * bonus data.
 *
 * One bonus of having the index present means that each file in the content
 * store can be referred to by its index in the digest table, which is (at
 * most) a uint32_t. If you treat this like a filesystem, these are basically
 * our inode values.
 *
 * As with git packfiles, the filedata objects are concatenated, compressed
 * objects. Each object can be decompressed individually and has a
 * (compression-algorithm-specific) header that contains the uncompressed size
 * of this file.
 *
 * TODO: since 99.5% of files have the same mode we certainly don't need a
 * full 32-bit mode_t for each file.. but what *do* we need?
 *
 * TODO: it might make sense to have the uncompressed sizes in the index so
 * we can get those without requiring another seek/read?
 *
 * TODO: "Servable" encoding could use varints for compactness at the expense
 * of CPU time (+branchiness). Might be good to benchmark those things..
 */

/*
 * TODO: Once we've got the file contents stored, we need some Tree object(s)
 * to provide directory entries / give paths and names for the files.
 *
 * Every file or directory provided by a package has a parent directory.
 * That parent might be owned by a different package - for example,
 * xz provides /usr/bin/xz, but it doesn't own /usr/bin.
 *
 * Directories, therefore, are part of the "API" exported by a component.
 * They can be public (like /usr/bin) or private (like /var/lib/rpm).
 *
 * In ELF:
 *   3 symbol binding types:
 *     STB_LOCAL:  visible in only this object
 *     STB_GLOBAL: visible to all objects; error if multiple defs
 *     STB_WEAK:   visible to all objects; GLOBAL will override if same name
 *   4 symbol visibility types:
 *     STV_DEFAULT: symbol behaves as above
 *     STV_PROTECTED: GLOBAL will not override inside this object
 *     STV_HIDDEN: not visible to other objects
 *     STV_INTERNAL: reserved
 *
 * For DINO, we need to figure out some stuff:
 *   * How do we export / import / refer to external paths?
 *   * How do you mark a path as relocatable, or fixed/static?
 *
 *
 *
 */

/* DEPENDENCIES!!!
 *
 * (Today's fun RPM fact: 76% of all the dependency metadata in F30 was
 * automatically generated by RPM.)
 *
 * RPM's dependency system consists of three types of objects:
 * 1. **Provides**, which are items of one of the following forms:
 *   * Simple: `NAME`
 *   * Versioned: `NAME = VERSION`
 *   * Versionrange: `NAME CMP VERSION`
 * 2. **Dependencies**, which encompasses any of the simple RPM dependency
 *    items (`Requires`, `Obsoletes`, `Enhances`, etc.).
 *    These have the form: `REQTYPE: NAME CMP VERSION`
 * 3. **Boolean Operators**, which can be used to join dependency expressions
 *    in various ways
 */

/* TODO:
 *   Dino_Note struct for DINO_SEC_NOTE
 *   Dino_User & Dino_Group structs for DINO_SEC_USERGROUP
 *   DINO_SEC_FILEDATA + structs for DINO_TYPE_ARCHIVE
 *   DINO_SEC_PROVS etc + structs for DINO_TYPE_COMPONENT
 *   DINO_SEC_DEPS etc + structs for DINO_TYPE_DYNIMG
 */
#ifdef __cplusplus
}
#endif

#endif /* _DINO_H */
