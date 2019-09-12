/* NOTE: this file will be overwritten and ignored during build; it's just
 * here to allow syntax checking etc. to work during development.
 * If you actually want to add new config options etc, you should be editing
 * config.h.in. */

#pragma once

/* version string */
#define LIBDINO_VERSION_STRING "0.0.1"

/* support zstd compression */
#define LIBDINO_ZSTD 1

/* support xz/lzma compression */
#define LIBDINO_LZMA 0

/* support zlib/gzip compression */
#define LIBDINO_ZLIB 0

