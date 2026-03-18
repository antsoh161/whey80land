project(
  'whey80land',
  'c',
  version: '0.1.0',
  license: 'MIT',
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ],
)

wlroots = dependency('wlroots-0.19')
wayland_server = dependency('wayland-server')
xkbcommon = dependency('xkbcommon')

wayland_protocols = dependency('wayland-protocols')
wayland_protocols_dir = wayland_protocols.get_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
  wayland_scanner_dep.get_variable('wayland_scanner'),
  native: true,
)

xdg_shell_xml = wayland_protocols_dir / 'stable' / 'xdg-shell' / 'xdg-shell.xml'

xdg_shell_server_header = custom_target(
  'xdg-shell-protocol-header',
  input: xdg_shell_xml,
  output: 'xdg-shell-protocol.h',
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

executable(
  'whey80land',
  files(
    'src/main.c',
    'src/output.c',
    'src/server.c',
    'src/xdg_shell.c',
    'src/input.c',
    'src/cursor.c',
  ),
  xdg_shell_server_header,
  dependencies: [wlroots, wayland_server, xkbcommon],
  c_args: ['-DWLR_USE_UNSTABLE'],
  install: true,
)

subdir('clients')
subdir('tests')
