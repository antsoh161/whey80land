#define _POSIX_C_SOURCE 200809L

#include "output.h"
#include "server.h"

#include <stdlib.h>
#include <time.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>

static void handle_frame(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_output *output = wl_container_of(listener, output, frame);
  if (!output->scene_output) {
    wlr_log(WLR_ERROR, "scene_output is null");
    return;
  }
  struct wlr_scene_output *scene_output = output->scene_output;

  wlr_log(WLR_DEBUG, "handle_frame called");
  /* Render the scene and submit the frame to the output */
  wlr_scene_output_commit(scene_output, NULL);

   wlr_output_schedule_frame(output->wlr_output);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}

static void handle_request_state(struct wl_listener *listener, void *data) {
  struct whey_output *output = wl_container_of(listener, output, request_state);
  const struct wlr_output_event_request_state *event = data;
  wlr_output_commit_state(output->wlr_output, event->state);
}

static void handle_destroy(struct wl_listener *listener, void *data) {
  (void)data;
  struct whey_output *output = wl_container_of(listener, output, destroy);

  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);

  free(output);
}

void handle_new_output(struct wl_listener *listener, void *data) {
  struct whey_server *server = wl_container_of(listener, server, new_output);
  struct wlr_output *wlr_output = data;

  wlr_output_init_render(wlr_output, server->allocator, server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
  if (mode) {
    wlr_output_state_set_mode(&state, mode);
  }

  /* Commit the initial state (mode + enabled) */
  wlr_output_commit_state(wlr_output, &state);
  wlr_output_state_finish(&state);

  /* Allocate and initialize our per-output tracking struct */
  struct whey_output *output = calloc(1, sizeof(*output));
  if (!output) {
    wlr_log(WLR_ERROR, "Failed to allocate whey_output");
    return;
  }

  output->server = server;
  output->wlr_output = wlr_output;

  /* Register event listeners. */
  output->frame.notify = handle_frame;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  output->request_state.notify = handle_request_state;
  wl_signal_add(&wlr_output->events.request_state, &output->request_state);

  output->destroy.notify = handle_destroy;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  wl_list_insert(&server->outputs, &output->link);

  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  output->scene_output = wlr_scene_get_scene_output(server->scene, wlr_output);
  wlr_xcursor_manager_load(server->cursor_mgr, wlr_output->scale);
  wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
}
