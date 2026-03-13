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

wlroots    = dependency('wlroots-0.19')
wayland_server = dependency('wayland-server')
xkbcommon  = dependency('xkbcommon')

executable(
  'whey80land',
  files(
    'src/main.c',
    'src/server.c',
    'src/output.c',
  ),
  dependencies: [wlroots, wayland_server, xkbcommon],
  c_args: ['-DWLR_USE_UNSTABLE'],
  install: true,
)
