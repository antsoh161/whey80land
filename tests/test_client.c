
#include <criterion/criterion.h>

#include "../clients/client.c"

Test(client, configure_updates_dimensions_for_positive_values) {
  struct whey80_client client = {0};
  struct whey80_client_window window = {
      .client = &client, .width = 100, .height = 100};

  xdg_toplevel_configure(&window, NULL, 640, 480, NULL);

  cr_assert_eq(window.width, 640);
  cr_assert_eq(window.height, 480);
}

Test(client, configure_ignores_zero_dimensions) {
  struct whey80_client client = {0};
  struct whey80_client_window window = {
      .client = &client, .width = 800, .height = 600};

  xdg_toplevel_configure(&window, NULL, 0, 0, NULL);

  cr_assert_eq(window.width, 800, "width should remain unchanged");
  cr_assert_eq(window.height, 600, "height should remain unchanged");
}

Test(client, close_sets_running_false) {
  struct whey80_client client = {.running = true};
  struct whey80_client_window window = {.client = &client};

  xdg_toplevel_close(&window, NULL);

  cr_assert_not(client.running, "running should be false after close");
}

static bool draw_was_called = false;
static void *draw_received_data = NULL;

static void mock_draw(struct whey80_client_window *window, void *user_data) {
  (void)window;
  draw_was_called = true;
  draw_received_data = user_data;
}

Test(client, draw_frame_invokes_callback) {
  draw_was_called = false;
  draw_received_data = NULL;

  int sentinel = 42;
  struct whey80_shm_buffer buf = {0};
  struct whey80_client_window window = {
      .draw = mock_draw, .buffer = &buf, .user_data = &sentinel};

  whey80_draw_frame(&window);

  cr_assert(draw_was_called, "draw callback should have been invoked");
  cr_assert_eq(draw_received_data, &sentinel,
               "user_data should be passed through");
}

Test(client, draw_frame_safe_with_null_draw_or_buffer) {
  struct whey80_shm_buffer buf = {0};
  struct whey80_client_window window1 = {.draw = NULL, .buffer = &buf};
  whey80_draw_frame(&window1); /* should not crash */

  struct whey80_client_window window2 = {.draw = mock_draw, .buffer = NULL};
  whey80_draw_frame(&window2); /* should not crash */
}
