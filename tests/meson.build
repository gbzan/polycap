tests = [
  'version',
  'error',
  'profile',
  'description',
  'capil',
  'photon',
  'source',
  'leaks',
]

test_c_args = core_c_args + ['-DEXAMPLE_DIR="@0@/"'.format(join_paths(project_source_root, 'example')), '-DTEST_BUILD']

foreach _test : tests
  _test_exec = executable(_test, files(_test + '.c'), c_args: test_c_args, dependencies: polycap_check_lib_dep)
  test(_test, _test_exec, timeout: 3600)
endforeach

if build_python_opt
  test_env = environment()
  test_env.prepend('PYTHONPATH', pydir)
  if host_system == 'windows'
    test_env.prepend('PATH', srcdir)
  endif

  test('python',
    python,
    args : files('python.py'),
    env : test_env,
    timeout: 1800,
    depends: [polycap_ext]
  )
endif
