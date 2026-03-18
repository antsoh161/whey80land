#include "cursor.h"
#include "server.h"
#include "xdg_shell.h"

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

static struct whey_toplevel *toplevel_at(struct whey_server *server,
                                         double lx, double ly,
                                         struct wlr_surface **surface,
                                         double *sx, double *sy) {
  struct wlr_scene_node *node =
      wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return NULL;
  }

  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return NULL;
  }
  *surface = scene_surface->surface;

  /* Walk up the scene tree to find the toplevel's scene_tree */
  struct wlr_scene_tree *tree = node->parent;
  while (tree && !tree->node.data) {
    tree = tree->node.parent;
  }
  if (!tree || !tree->node.data) {
    return NULL;
  }
  return tree->node.data;
}

static void process_cursor_motion(struct whey_server *server, uint32_t time) {
  if (server->cursor_mode == WHEY_CURSOR_MOVE) {
    struct whey_toplevel *toplevel = server->grabbed_toplevel;
    wlr_scene_node_set_position(&toplevel->scene_tree->node,
                                server->cursor->x - server->grab_x,
                                server->cursor->y - server->grab_y);
    return;
  }

  if (server->cursor_mode == WHEY_CURSOR_RESIZE) {
    struct whey_toplevel *toplevel = server->grabbed_toplevel;
    double dx = server->cursor->x - server->grab_x;
    double dy = server->cursor->y - server->grab_y;

    double new_left = server->grab_geobox.x;
    double new_right = server->grab_geobox.x + server->grab_geobox.width;
    double new_top = server->grab_geobox.y;
    double new_bottom = server->grab_geobox.y + server->grab_geobox.height;

    if (server->resize_edges & WLR_EDGE_TOP) {
      new_top += dy;
      if (new_top >= new_bottom)
        new_top = new_bottom - 1;
    } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
      new_bottom += dy;
      if (new_bottom <= new_top)
        new_bottom = new_top + 1;
    }
    if (server->resize_edges & WLR_EDGE_LEFT) {
      new_left += dx;
      if (new_left >= new_right)
        new_left = new_right - 1;
    } else if (server->resize_edges & WLR_EDGE_RIGHT) {
      new_right += dx;
      if (new_right <= new_left)
        new_right = new_left + 1;
    }

    struct wlr_box geo_box = toplevel->xdg_toplevel->base->geometry;
    wlr_scene_node_set_position(&toplevel->scene_tree->node,
                                new_left - geo_box.x, new_top - geo_box.y);

    int new_width = (int)(new_right - new_left);
    int new_height = (int)(new_bottom - new_top);
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
    return;
  }

  /* PASSTHROUGH — find the surface under cursor and notify seat */
  struct wlr_surface *surface = NULL;
  double sx, sy;
  struct whey_toplevel *toplevel =
      toplevel_at(server, server->cursor->x, server->cursor->y,
                  &surface, &sx, &sy);

  if (!toplevel) {
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    wlr_seat_pointer_clear_focus(server->seat);
    return;
  }

  wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
  wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
}

void handle_cursor_motion(struct wl_listener *listener, void *data) {
  struct whey_server *server =
      wl_container_of(listener, server, cursor_motion);
  struct wlr_pointer_motion_event *event = data;

  wlr_cursor_move(server->cursor, &event->pointer->base,
                   event->delta_x, event->delta_y);
  process_cursor_motion(server, event->time_msec);
}

void handle_cursor_motion_absolute(struct wl_listener *listener, void *data) {
  struct whey_server *server =
      wl_container_of(listener, server, cursor_motion_absolute);
  struct wlr_pointer_motion_absolute_event *event = data;

  wlr_cursor_warp_absolute(server->cursor, &event->pointer->base,
                           event->x, event->y);
  process_cursor_motion(server, event->time_msec);
}

void handle_cursor_button(struct wl_listener *listener, void *data) {
  struct whey_server *server =
      wl_container_of(listener, server, cursor_button);
  struct wlr_pointer_button_event *event = data;

  wlr_seat_pointer_notify_button(server->seat,
                                 event->time_msec, event->button, event->state);

  if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
    if (server->cursor_mode != WHEY_CURSOR_PASSTHROUGH) {
      server->cursor_mode = WHEY_CURSOR_PASSTHROUGH;
      server->grabbed_toplevel = NULL;
      return;
    }
  }

  if (event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    struct wlr_surface *surface = NULL;
    double sx, sy;
    struct whey_toplevel *toplevel =
        toplevel_at(server, server->cursor->x, server->cursor->y,
                    &surface, &sx, &sy);

    if (toplevel) {
      wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);

      struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
      if (keyboard) {
        wlr_seat_keyboard_notify_enter(server->seat, surface,
                                       keyboard->keycodes,
                                       keyboard->num_keycodes,
                                       &keyboard->modifiers);
      }

      /* Alt+click shortcuts for move/resize */
      if (keyboard &&
          (wlr_keyboard_get_modifiers(keyboard) & WLR_MODIFIER_ALT)) {
        if (event->button == BTN_LEFT) {
          begin_interactive_move(server, toplevel);
        } else if (event->button == BTN_RIGHT) {
          begin_interactive_resize(server, toplevel,
                                   WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT);
        }
      }
    }
  }
}

void handle_cursor_axis(struct wl_listener *listener, void *data) {
  struct whey_server *server =
      wl_container_of(listener, server, cursor_axis);
  struct wlr_pointer_axis_event *event = data;

  wlr_seat_pointer_notify_axis(server->seat,
                               event->time_msec, event->orientation,
                               event->delta, event->delta_discrete,
                               event->source, event->relative_direction);
}

void handle_cursor_frame(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_server *server =
      wl_container_of(listener, server, cursor_frame);

  wlr_seat_pointer_notify_frame(server->seat);
}

void begin_interactive_move(struct whey_server *server,
                            struct whey_toplevel *toplevel) {
  server->grabbed_toplevel = toplevel;
  server->cursor_mode = WHEY_CURSOR_MOVE;
  server->grab_x = server->cursor->x - toplevel->scene_tree->node.x;
  server->grab_y = server->cursor->y - toplevel->scene_tree->node.y;
}

void begin_interactive_resize(struct whey_server *server,
                              struct whey_toplevel *toplevel,
                              uint32_t edges) {
  struct wlr_box geo_box = toplevel->xdg_toplevel->base->geometry;

  server->grabbed_toplevel = toplevel;
  server->cursor_mode = WHEY_CURSOR_RESIZE;
  server->resize_edges = edges;

  server->grab_x = server->cursor->x;
  server->grab_y = server->cursor->y;
  server->grab_geobox.x = toplevel->scene_tree->node.x + geo_box.x;
  server->grab_geobox.y = toplevel->scene_tree->node.y + geo_box.y;
  server->grab_geobox.width = geo_box.width;
  server->grab_geobox.height = geo_box.height;
}
