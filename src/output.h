#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>

struct whey_server;

struct whey_output {

  struct wl_list link;
  struct whey_server *server;
  struct wlr_output *wlr_output;
  struct wlr_scene_output *scene_output;
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
};

void handle_new_output(struct wl_listener *listener, void *data);
