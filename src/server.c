#include "server.h"
#include "output.h"
#include "src/xdg_shell.h"

#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>

bool server_init(struct whey_server *server) {
  server->wl_display = wl_display_create();
  if (!server->wl_display) {
    wlr_log(WLR_ERROR, "Failed to create wl_display");
    return false;
  }

  struct wl_event_loop *loop = wl_display_get_event_loop(server->wl_display);
  server->backend = wlr_backend_autocreate(loop, NULL);
  if (!server->backend) {
    wlr_log(WLR_ERROR, "Failed to create wlr_backend");
    goto err_display;
  }

  server->renderer = wlr_renderer_autocreate(server->backend);
  if (!server->renderer) {
    wlr_log(WLR_ERROR, "Failed to create wlr_renderer");
    goto err_backend;
  }

  wlr_renderer_init_wl_display(server->renderer, server->wl_display);

  server->allocator =
      wlr_allocator_autocreate(server->backend, server->renderer);
  if (!server->allocator) {
    wlr_log(WLR_ERROR, "Failed to create wlr_allocator");
    goto err_renderer;
  }

  wlr_compositor_create(server->wl_display, 6, server->renderer);
  wlr_subcompositor_create(server->wl_display);

  server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 6);
  if (!server->xdg_shell) {
    wlr_log(WLR_ERROR, "Failed to create wlr_xdg_shell");
    goto err_allocator;
  }

  server->scene = wlr_scene_create();
  if (!server->scene) {
    wlr_log(WLR_ERROR, "Failed to create wlr_scene");
    goto err_allocator;
  }

  server->output_layout = wlr_output_layout_create(server->wl_display);
  if (!server->output_layout) {
    wlr_log(WLR_ERROR, "Failed to create wlr_output_layout");
    goto err_scene;
  }

  server->scene_layout =
      wlr_scene_attach_output_layout(server->scene, server->output_layout);

  wl_list_init(&server->outputs);

  wl_list_init(&server->toplevels);
  server->new_output.notify = handle_new_output;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

  server->new_xdg_toplevel.notify = handle_new_toplevel;
  wl_signal_add(&server->xdg_shell->events.new_toplevel,
                &server->new_xdg_toplevel);

  return true;

err_scene:
  wlr_scene_node_destroy(&server->scene->tree.node);
err_allocator:
  wlr_allocator_destroy(server->allocator);
err_renderer:
  wlr_renderer_destroy(server->renderer);
err_backend:
  wlr_backend_destroy(server->backend);
err_display:
  wl_display_destroy(server->wl_display);
  return false;
}

void server_finish(struct whey_server *server) {
  wl_display_destroy_clients(server->wl_display);
  wlr_scene_node_destroy(&server->scene->tree.node);
  wlr_allocator_destroy(server->allocator);
  wlr_renderer_destroy(server->renderer);
  wlr_backend_destroy(server->backend);
  wl_display_destroy(server->wl_display);
}
