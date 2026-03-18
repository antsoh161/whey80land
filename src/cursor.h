#pragma once

#include <wayland-server-core.h>

struct whey_server;
struct whey_toplevel;

enum whey_cursor_mode {
  WHEY_CURSOR_PASSTHROUGH,
  WHEY_CURSOR_MOVE,
  WHEY_CURSOR_RESIZE,
};

void handle_cursor_motion(struct wl_listener *listener, void *data);
void handle_cursor_motion_absolute(struct wl_listener *listener, void *data);
void handle_cursor_button(struct wl_listener *listener, void *data);
void handle_cursor_axis(struct wl_listener *listener, void *data);
void handle_cursor_frame(struct wl_listener *listener, void *data);

void begin_interactive_move(struct whey_server *server,
                            struct whey_toplevel *toplevel);
void begin_interactive_resize(struct whey_server *server,
                              struct whey_toplevel *toplevel, uint32_t edges);
