#define _POSIX_C_SOURCE 200809L

#include <criterion/criterion.h>
#include <wayland-server-core.h>
#include "../src/output.c"


Test(output, destroy_removes_all_listeners) {
  struct whey_output *output = calloc(1, sizeof(*output));
  cr_assert_not_null(output);

  struct wl_list lists[4];
  for (int i = 0; i < 4; i++)
    wl_list_init(&lists[i]);

  wl_list_insert(&lists[0], &output->frame.link);
  wl_list_insert(&lists[1], &output->request_state.link);
  wl_list_insert(&lists[2], &output->destroy.link);
  wl_list_insert(&lists[3], &output->link);

  handle_destroy(&output->destroy, NULL);

  const char *names[] = {"frame", "request_state", "destroy", "link"};
  for (int i = 0; i < 4; i++) {
    cr_assert(wl_list_empty(&lists[i]),
              "listener '%s' was not removed", names[i]);
  }
}
