#pragma once

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

struct whey_server;

struct whey_toplevel {
  /* List of toplevel windows */
  struct wl_list link;

  struct whey_server *server;

  struct wlr_xdg_toplevel *xdg_toplevel;

  /* Node for this toplevel window */
  struct wlr_scene_tree *scene_tree;

  /* Event listeners */
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener request_maximize;
  struct wl_listener request_fullscreen;
  struct wl_listener request_move;
  struct wl_listener request_resize;
};

void handle_new_toplevel(struct wl_listener *listener, void *data);
