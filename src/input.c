
#include "input.h"
#include "server.h"

#include <stdlib.h>
#include <wayland-server-protocol.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

/* Keys for the compositor */
static bool handle_compositor_keybinding(struct whey_server *server,
                                         xkb_keysym_t sym) {
  switch (sym) {
  case XKB_KEY_Escape:
    /* Quit */
    wl_display_terminate(server->wl_display);
    return true;
  default:
    return false;
  }
}

static void handle_keyboard_key(struct wl_listener *listener, void *data) {
  struct whey_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct whey_server *server = keyboard->server;
  struct wlr_keyboard_key_event *event = data;

  /* Random offset? */
  uint32_t keycode = event->keycode + 8;

  const xkb_keysym_t *syms;
  int nsyms =
      xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

  bool handled = false;

  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED &&
      (modifiers & WLR_MODIFIER_ALT)) {
    for (int i = 0; i < nsyms; i++) {
      handled = handle_compositor_keybinding(server, syms[i]);
      if (handled)
        break;
    }
  }

  /* Not a compositor key, forward to client */
  if (!handled) {
    wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(server->seat, event->time_msec, event->keycode,
                                 event->state);
  }
}

static void handle_keyboard_modifiers(struct wl_listener *listener,
                                      void *data) {
  (void)data;
  struct whey_keyboard *keyboard =
      wl_container_of(listener, keyboard, modifiers);

  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                                     &keyboard->wlr_keyboard->modifiers);
}

static void handle_keyboard_destroy(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);

  wlr_log(WLR_INFO, "Keyboard destroyed: %s",
          keyboard->wlr_keyboard->base.name);

  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
  free(keyboard);
}

static void keyboard_create(struct whey_server *server,
                            struct wlr_keyboard *wlr_keyboard) {
  struct whey_keyboard *keyboard = calloc(1, sizeof(*keyboard));
  if (!keyboard) {
    wlr_log(WLR_ERROR, "Failed to allocate whey_keyboard");
    return;
  }

  keyboard->server = server;
  keyboard->wlr_keyboard = wlr_keyboard;

  /* Default for now */
  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!context) {
    wlr_log(WLR_ERROR, "Failed to create xkb context");
    free(keyboard);
    return;
  }

  struct xkb_keymap *keymap =
      xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    wlr_log(WLR_ERROR, "Failed to create xkb keymap");
    xkb_context_unref(context);
    free(keyboard);
    return;
  }

  wlr_keyboard_set_keymap(wlr_keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);

  wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

  keyboard->key.notify = handle_keyboard_key;
  wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);

  keyboard->modifiers.notify = handle_keyboard_modifiers;
  wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);

  keyboard->destroy.notify = handle_keyboard_destroy;
  wl_signal_add(&wlr_keyboard->base.events.destroy, &keyboard->destroy);

  wl_list_insert(&server->keyboards, &keyboard->link);
  wlr_seat_set_keyboard(server->seat, wlr_keyboard);

  wlr_log(WLR_INFO, "Keyboard added: %s", wlr_keyboard->base.name);
}

void handle_new_input(struct wl_listener *listener, void *data) {
  struct whey_server *server = wl_container_of(listener, server, new_input);
  struct wlr_input_device *device = data;

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    keyboard_create(server, wlr_keyboard_from_input_device(device));
    break;
  case WLR_INPUT_DEVICE_POINTER:
    wlr_log(WLR_INFO, "Pointer device detected (not yet handled): %s",
            device->name);
    break;
  default:
    wlr_log(WLR_DEBUG, "Unhandled input device type: %d", device->type);
    break;
  }

  uint32_t caps = WL_SEAT_CAPABILITY_KEYBOARD;
  /* caps |= WL_SEAT_CAPABILITY_POINTER; */
  wlr_seat_set_capabilities(server->seat, caps);
}
