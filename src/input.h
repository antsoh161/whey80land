#pragma once

#include "server.h"
#include <wayland-server-core.h>
#include <wlr/types/wlr_keyboard.h>

struct whey_server;

struct whey_keyboard {
   struct wl_list link;
   struct whey_server* server;
   struct wlr_keyboard* wlr_keyboard;

   struct wl_listener key;
   struct wl_listener modifiers;
   struct wl_listener destroy;
};

void handle_new_input(struct wl_listener* listener, void* data);
