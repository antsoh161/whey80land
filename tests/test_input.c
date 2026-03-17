#define _POSIX_C_SOURCE 200809L

#include <criterion/criterion.h>
#include <string.h>
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>

#include "../src/input.c"


Test(input, escape_keybinding_is_handled) {
  struct whey_server server = {0};
  server.wl_display = wl_display_create();
  cr_assert_not_null(server.wl_display);

  bool handled = handle_compositor_keybinding(&server, XKB_KEY_Escape);
  cr_assert(handled, "Alt+Escape should be handled by the compositor");

  wl_display_destroy(server.wl_display);
}

Test(input, regular_key_is_not_handled) {
  struct whey_server server = {0};
  server.wl_display = wl_display_create();
  cr_assert_not_null(server.wl_display);

  cr_assert_not(handle_compositor_keybinding(&server, XKB_KEY_a));
  cr_assert_not(handle_compositor_keybinding(&server, XKB_KEY_Return));
  cr_assert_not(handle_compositor_keybinding(&server, XKB_KEY_space));

  wl_display_destroy(server.wl_display);
}


Test(input, keyboard_destroy_removes_from_all_lists) {
  struct whey_keyboard *kb = calloc(1, sizeof(*kb));
  cr_assert_not_null(kb);

  struct wlr_keyboard mock_kb = {0};
  mock_kb.base.name = strdup("test-keyboard");
  kb->wlr_keyboard = &mock_kb;

  struct wl_list parent_list, key_list, mod_list, destroy_list;
  wl_list_init(&parent_list);
  wl_list_init(&key_list);
  wl_list_init(&mod_list);
  wl_list_init(&destroy_list);

  wl_list_insert(&parent_list, &kb->link);
  wl_list_insert(&key_list, &kb->key.link);
  wl_list_insert(&mod_list, &kb->modifiers.link);
  wl_list_insert(&destroy_list, &kb->destroy.link);

  handle_keyboard_destroy(&kb->destroy, NULL);

  cr_assert(wl_list_empty(&parent_list), "keyboard not removed from server list");
  cr_assert(wl_list_empty(&key_list), "key listener not removed");
  cr_assert(wl_list_empty(&mod_list), "modifiers listener not removed");
  cr_assert(wl_list_empty(&destroy_list), "destroy listener not removed");

  free(mock_kb.base.name);
}
