#include "xdg_shell.h"
#include "server.h"

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/util/log.h>

static void handle_toplevel_commit(struct wl_listener *listener, void *data) {
  (void)data;

  struct whey_toplevel *toplevel = wl_container_of(listener, toplevel, commit);
  if (toplevel->xdg_toplevel->base->initial_commit) {
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
  }
}

static void handle_toplevel_map(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_toplevel *toplevel = wl_container_of(listener, toplevel, map);

  wl_list_insert(&toplevel->server->toplevels, &toplevel->link);

  wlr_log(WLR_INFO, "Toplevel mapped: %s (%s)",
          toplevel->xdg_toplevel->title ? toplevel->xdg_toplevel->title
                                        : "(no title)",
          toplevel->xdg_toplevel->app_id ? toplevel->xdg_toplevel->app_id
                                         : "(no app_id)");
}

static void handle_toplevel_unmap(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

  wl_list_remove(&toplevel->link);

  wlr_log(WLR_INFO, "Toplevel unmapped");
}

static void handle_toplevel_destroy(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);

  wlr_log(WLR_INFO, "Toplevel destroyed");
  wl_list_remove(&toplevel->map.link);
  wl_list_remove(&toplevel->unmap.link);
  wl_list_remove(&toplevel->commit.link);
  wl_list_remove(&toplevel->destroy.link);
  wl_list_remove(&toplevel->request_maximize.link);
  wl_list_remove(&toplevel->request_fullscreen.link);

  free(toplevel);
}

static void handle_toplevel_request_maximize(struct wl_listener *listener,
                                             void *data) {
  (void)data;
  struct whey_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_maximize);
  wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

static void handle_toplevel_request_fullscreen(struct wl_listener *listener,
                                               void *data) {
  (void)data;
  struct whey_toplevel *toplevel =
      wl_container_of(listener, toplevel, request_fullscreen);
  wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void handle_new_toplevel(struct wl_listener *listener, void *data) {
  struct whey_server *server =
      wl_container_of(listener, server, new_xdg_toplevel);
  struct wlr_xdg_toplevel *xdg_toplevel = data;

  struct whey_toplevel *toplevel = calloc(1, sizeof(*toplevel));

  if (!toplevel) {
    wlr_log(WLR_ERROR, "Failed to allocate whey_toplevel");
    return;
  }

  toplevel->server = server;
  toplevel->xdg_toplevel = xdg_toplevel;

  toplevel->scene_tree =
      wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);

  toplevel->scene_tree->node.data = toplevel;

  struct wlr_xdg_surface *xdg_surface = xdg_toplevel->base;
  struct wlr_surface *surface = xdg_surface->surface;

  toplevel->map.notify = handle_toplevel_map;
  wl_signal_add(&surface->events.map, &toplevel->map);

  toplevel->unmap.notify = handle_toplevel_unmap;
  wl_signal_add(&surface->events.unmap, &toplevel->unmap);

  toplevel->commit.notify = handle_toplevel_commit;
  wl_signal_add(&surface->events.commit, &toplevel->commit);

  toplevel->destroy.notify = handle_toplevel_destroy;
  wl_signal_add(&surface->events.destroy, &toplevel->destroy);

  toplevel->request_maximize.notify = handle_toplevel_request_maximize;
  wl_signal_add(&xdg_toplevel->events.request_maximize,
                &toplevel->request_maximize);

  toplevel->request_fullscreen.notify = handle_toplevel_request_fullscreen;
  wl_signal_add(&xdg_toplevel->events.request_fullscreen,
                &toplevel->request_fullscreen);

  wlr_log(WLR_INFO, "New toplevel created");
}
