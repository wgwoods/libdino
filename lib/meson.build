compress_deps = [
    dependency('libzstd',   version: '>= 1.4.0'),
    dependency('zlib',      version: '>= 1.2.7', required: false),
]
crypto_deps = [
    dependency('libcrypto', version: '>= 1.1.0'),
]

lib_sources = [
    'dino_begin.c',
    'namtab.c',
    'sectab.c',
]

libdino = library('dino', lib_sources,
                  dependencies: [compress_deps, crypto_deps])
