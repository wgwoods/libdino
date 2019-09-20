munit_dep = dependency('munit', fallback: ['munit', 'munit_dep'])

bsearch_exe = executable('test_bsearch', 'test_bsearch.c',
                       dependencies: munit_dep,
                       link_with: libdino)
array_exe = executable('test_array', 'test_array.c',
                       dependencies: munit_dep,
                       link_with: libdino)
compr_exe = executable('test_compress', 'test_compress.c',
                       dependencies: munit_dep,
                       link_with: libdino)
digest_exe = executable('test_digest', 'test_digest.c',
                       dependencies: munit_dep,
                       link_with: libdino)
misc_exe = executable('test_misc', 'test_misc.c',
                       dependencies: munit_dep,
                       link_with: libdino)

test('array', array_exe)
test('bsearch', bsearch_exe)
test('compr', compr_exe)
test('digest', digest_exe)
test('misc', misc_exe)
