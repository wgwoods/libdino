# eventually I'd rather have this be one big git-style multi-call binary,
# but for now we'll just build a bunch of binaries..

dinotools = [
  'dinotools.c',
]

executable('dinoinfo',
           'dinoinfo.c', dinotools,
           link_with: libdino, install: true)

rpm = dependency('rpm', version: '>= 4.14.0', required: get_option('rpm'))

if rpm.found()
  executable('dinorpm',
             'dinorpm.c', dinotools,
             dependencies: [rpm],
             link_with: libdino, install: true)
  executable('mkdino',
             'mkdino.c', dinotools, '../lib/fileio.c',
             dependencies: [rpm],
             link_with: libdino, install: true)
endif
