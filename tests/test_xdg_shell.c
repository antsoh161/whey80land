#include <criterion/criterion.h>
#include <wayland-server-core.h>

#include <wlr/types/wlr_seat.h>

#include "../src/xdg_shell.c"

Test(xdg_shell, toplevel_destroy_removes_all_listeners) {
  struct whey_toplevel *toplevel = calloc(1, sizeof(*toplevel));
  cr_assert_not_null(toplevel);

  struct wl_list lists[6];
  for (int i = 0; i < 6; i++)
    wl_list_init(&lists[i]);

  wl_list_insert(&lists[0], &toplevel->map.link);
  wl_list_insert(&lists[1], &toplevel->unmap.link);
  wl_list_insert(&lists[2], &toplevel->commit.link);
  wl_list_insert(&lists[3], &toplevel->destroy.link);
  wl_list_insert(&lists[4], &toplevel->request_maximize.link);
  wl_list_insert(&lists[5], &toplevel->request_fullscreen.link);

  handle_toplevel_destroy(&toplevel->destroy, NULL);

  const char *names[] = {"map", "unmap", "commit", "destroy",
                         "request_maximize", "request_fullscreen"};
  for (int i = 0; i < 6; i++) {
    cr_assert(wl_list_empty(&lists[i]),
              "listener '%s' was not removed from its signal list", names[i]);
  }
}
