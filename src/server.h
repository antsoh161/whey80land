#pragma once
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/box.h>

#include "cursor.h"

struct whey_toplevel;

struct whey_server {
  struct wl_display *wl_display;

  struct wlr_backend *backend;

  struct wlr_renderer *renderer;

  struct wlr_allocator *allocator;

  struct wlr_scene *scene;

  struct wlr_scene_output_layout *scene_layout;

  struct wlr_output_layout *output_layout;

  struct wlr_xdg_shell *xdg_shell;

  struct wlr_seat *seat;

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;

  enum whey_cursor_mode cursor_mode;
  struct whey_toplevel *grabbed_toplevel;
  double grab_x, grab_y;
  struct wlr_box grab_geobox;
  uint32_t resize_edges;

  struct wl_listener new_output;

  struct wl_listener new_xdg_toplevel;

  struct wl_listener new_input;

  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;

  struct wl_list outputs;

  struct wl_list toplevels;

  struct wl_list keyboards;
};

bool server_init(struct whey_server *server);

void server_finish(struct whey_server *server);
