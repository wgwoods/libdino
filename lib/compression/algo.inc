/*
 * This is a hacky way to do conditional compilation things for each
 * algorithm we're building with.
 * If ALGO_DOINCLUDES is defined, it #includes the relevant headers.
 * If DECLARE_ALGONAME(id, name) is defined, it will do that once for each
 * supported Dino_CompressID / name pair.
 */

#ifndef DECLARE_ALGONAME
#define DECLARE_ALGONAME(id, name)
#endif

#include "../libdino-config.h"

#if LIBDINO_ZSTD
# if ALGO_DOINCLUDES
#  include "zstd.h"
# endif
DECLARE_ALGONAME(DINO_COMPRESS_ZSTD, "zstd")
#endif

#if LIBDINO_LZMA
# if ALGO_DOINCLUDES
#  include "xz.h"
# endif
DECLARE_ALGONAME(DINO_COMPRESS_XZ, "xz")
DECLARE_ALGONAME(DINO_COMPRESS_LZMA, "lzma")
#endif

#if LIBDINO_ZLIB
# if ALGO_DOINCLUDES
#  include "zlib.h"
# endif
DECLARE_ALGONAME(DINO_COMPRESS_ZLIB, "zlib")
#endif

#undef DECLARE_ALGONAME
#undef ALGO_DOINCLUDES
/* vim: set filetype=c : */
