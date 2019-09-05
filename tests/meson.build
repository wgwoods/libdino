munit_dep = dependency('munit', fallback: ['munit', 'munit_dep'])

munit_src = [
    'test_bsearch.c',
    'test_munit.c',
]

munit_exe = executable('test_munit', munit_src,
                       dependencies: munit_dep,
                       link_with: libdino)
test('munit', munit_exe)
