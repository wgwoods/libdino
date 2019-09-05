compress_deps = [
    dependency('libzstd',   version: '>= 1.4.0'),
    dependency('zlib',      version: '>= 1.2.7', required: false),
]
crypto_deps = [
    dependency('libcrypto', version: '>= 1.1.0'),
]

lib_sources = [
    'array.c',
    'bsearchn.c',
    'dino_begin.c',
    'index.c',
    'namtab.c',
    'sectab.c',
]

lib_headers = [
    'dino.h',
    'libdino.h',
]

libdino = library('dino', lib_sources,
                  dependencies: [compress_deps, crypto_deps],
                  install: true)

install_headers(lib_headers)
