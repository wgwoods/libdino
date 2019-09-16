zstd = dependency('libzstd',   version: '>= 1.4.0', required: get_option('zstd'))
lzma = dependency('liblzma',   version: '>= 5.2.4', required: get_option('xz'))
zlib = dependency('zlib',      version: '>= 1.2.7', required: get_option('zlib'))

compress_deps = [zstd, lzma, zlib]

libcrypto = dependency('libcrypto', version: '>= 1.1.0')
crypto_deps = [libcrypto]

config = configuration_data()
config.set_quoted('LIBDINO_VERSION_STRING', meson.project_version())
config.set10('LIBDINO_XZ', lzma.found())
config.set10('LIBDINO_ZSTD', zstd.found())
config.set10('LIBDINO_ZLIB', zlib.found())
config_h = configure_file(input: 'libdino-config.h.in',
                          output: 'libdino-config.h',
                          configuration: config)

lib_sources = [
    'array.c',
    'bsearchn.c',
    'compression/compression.c',
    'compression/funcs.c',
    'dino_begin.c',
    'digest.c',
    'index.c',
    'memory.c',
    'namtab.c',
    'sectab.c',
]

if lzma.found()
  lib_sources += 'compression/xz.c'
endif

if zstd.found()
  lib_sources += 'compression/zstd.c'
endif

lib_headers = [
    'dino.h',
    'libdino.h',
]

libdino = library('dino', lib_sources,
                  dependencies: [compress_deps, crypto_deps],
                  install: true)

install_headers(lib_headers)
