
#define _POSIX_C_SOURCE 200809L

#include "server.h"

#include <stdlib.h>
#include <unistd.h>
#include <wlr/util/log.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  wlr_log_init(WLR_DEBUG, NULL);

  struct whey_server server = {0};
  if (!server_init(&server)) {
    wlr_log(WLR_ERROR, "Failed to initialize server");
    return 1;
  }

  const char *socket = wl_display_add_socket_auto(server.wl_display);
  if (!socket) {
    wlr_log(WLR_ERROR, "Failed to create Wayland socket");
    server_finish(&server);
    return 1;
  }

  if (!wlr_backend_start(server.backend)) {
    wlr_log(WLR_ERROR, "Failed to start backend");
    server_finish(&server);
    return 1;
  }

  setenv("WAYLAND_DISPLAY", socket, true);

  wlr_log(WLR_INFO, "whey80land running on Wayland display '%s'", socket);

  wl_display_run(server.wl_display);

  wlr_log(WLR_INFO, "Shutting down whey80land");
  server_finish(&server);

  return 0;
}
