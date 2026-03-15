inc = include_directories('.')

# Generate client header
xdg_shell_client_header = custom_target(
  'xdg-shell-client-protocol-header',
  input: xdg_shell_xml,
  output: 'xdg-shell-client-protocol.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_protocol_code = custom_target(
  'xdg-shell-protocol-code',
  input: xdg_shell_xml,
  output: 'xdg-shell-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_client = dependency('wayland-client')

whey80land_client = static_library(
  'whey80land_client',
  sources: ['client.c', xdg_shell_protocol_code, xdg_shell_client_header],
  include_directories: inc,
  dependencies: [wayland_client],
  install: false,
)

# Build executable
executable(
  'simple-client',
  sources: ['simple-client.c'],
  include_directories: inc,
  dependencies: [wayland_client],
  link_whole: whey80land_client,
)
