munit_dep = dependency('munit', fallback: ['munit', 'munit_dep'])

munit_src = [
    'test_bsearch.c',
    'test_munit.c',
]

munit_exe = executable('test_munit', munit_src,
                       dependencies: munit_dep,
                       link_with: libdino)
array_exe = executable('test_array', 'test_array.c',
                       dependencies: munit_dep,
                       link_with: libdino)
test('array', array_exe)
test('munit', munit_exe)
