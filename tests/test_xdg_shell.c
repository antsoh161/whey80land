#include <criterion/criterion.h>
#include <wayland-server-core.h>

#include <wlr/types/wlr_seat.h>

struct whey_server;
struct whey_toplevel;

void begin_interactive_move(struct whey_server *server,
                            struct whey_toplevel *toplevel) {
  (void)server;
  (void)toplevel;
}
void begin_interactive_resize(struct whey_server *server,
                              struct whey_toplevel *toplevel, uint32_t edges) {
  (void)server;
  (void)toplevel;
  (void)edges;
}

#include "../src/xdg_shell.c"

Test(xdg_shell, toplevel_destroy_removes_all_listeners) {
  struct whey_toplevel *toplevel = calloc(1, sizeof(*toplevel));
  cr_assert_not_null(toplevel);

  struct wl_list lists[8];
  for (int i = 0; i < 8; i++)
    wl_list_init(&lists[i]);

  wl_list_insert(&lists[0], &toplevel->map.link);
  wl_list_insert(&lists[1], &toplevel->unmap.link);
  wl_list_insert(&lists[2], &toplevel->commit.link);
  wl_list_insert(&lists[3], &toplevel->destroy.link);
  wl_list_insert(&lists[4], &toplevel->request_maximize.link);
  wl_list_insert(&lists[5], &toplevel->request_fullscreen.link);
  wl_list_insert(&lists[6], &toplevel->request_move.link);
  wl_list_insert(&lists[7], &toplevel->request_resize.link);

  handle_toplevel_destroy(&toplevel->destroy, NULL);

  const char *names[] = {"map",
                         "unmap",
                         "commit",
                         "destroy",
                         "request_maximize",
                         "request_fullscreen",
                         "request_move",
                         "request_resize"};
  for (int i = 0; i < 8; i++) {
    cr_assert(wl_list_empty(&lists[i]),
              "listener '%s' was not removed from its signal list", names[i]);
  }
}
