criterion = dependency('criterion')

src_inc = include_directories('..', '../src')
clients_inc = include_directories('..', '../clients')


test('test_input',
  executable('test_input',
    'test_input.c',
    dependencies: [criterion, wlroots, wayland_server, xkbcommon],
    include_directories: src_inc,
    c_args: ['-DWLR_USE_UNSTABLE'],
  )
)

test('test_xdg_shell',
  executable('test_xdg_shell',
    'test_xdg_shell.c',
    dependencies: [criterion, wlroots, wayland_server],
    include_directories: src_inc,
    c_args: ['-DWLR_USE_UNSTABLE'],
  )
)

test('test_output',
  executable('test_output',
    'test_output.c',
    dependencies: [criterion, wlroots, wayland_server],
    include_directories: src_inc,
    c_args: ['-DWLR_USE_UNSTABLE'],
  )
)


test('test_client',
  executable('test_client',
    sources: ['test_client.c', xdg_shell_protocol_code],
    dependencies: [criterion, wayland_client],
    include_directories: clients_inc,
    c_args: ['-D_GNU_SOURCE'],
  )
)

test('test_simple_client',
  executable('test_simple_client',
    'test_simple_client.c',
    dependencies: [criterion, wayland_client],
    link_whole: whey80land_client,
    include_directories: clients_inc,
  )
)
