#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "client.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void xdg_toplevel_configure(void *data,
                                   struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states) {
  (void)xdg_toplevel;
  (void)states;
  struct whey80_client_window *window = data;

  if (width > 0 && height > 0) {
    window->width = width;
    window->height = height;
    /* TODO: Reallocate */
  }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  (void)xdg_toplevel;
  struct whey80_client_window *window = data;
  window->client->running = false;
}

static void xdg_toplevel_configure_bounds(void *data,
                                          struct xdg_toplevel *xdg_toplevel,
                                          int32_t width, int32_t height) {
  /* TODO: clamp window to compositor-suggested bounds */
  (void)data;
  (void)xdg_toplevel;
  (void)width;
  (void)height;
}

static void xdg_toplevel_wm_capabilities(void *data,
                                         struct xdg_toplevel *xdg_toplevel,
                                         struct wl_array *capabilities) {
  /* TODO: inspect compositor capabilities */
  (void)data;
  (void)xdg_toplevel;
  (void)capabilities;
}

static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities = xdg_toplevel_wm_capabilities,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
  xdg_surface_ack_configure(xdg_surface, serial);
  struct whey80_client_window *window = data;

  if (!window->configured) {
    window->configured = true;

    window->buffer =
        whey80_shm_buffer_create(window->client, window->width, window->height);

    whey80_draw_frame(window);
    whey80_submit_frame(window);
    return;
  }
}

static const struct xdg_surface_listener surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base,
                             uint32_t serial) {
  (void)data;
  xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface,
                            uint32_t version) {
  struct whey80_client *client = data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    client->compositor = wl_registry_bind(
        registry, name, &wl_compositor_interface, version < 6 ? version : 6);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    client->shm = wl_registry_bind(registry, name, &wl_shm_interface,
                                   version < 1 ? version : 1);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    client->xdg_wm_base = wl_registry_bind(
        registry, name, &xdg_wm_base_interface, version < 6 ? version : 6);
    xdg_wm_base_add_listener(client->xdg_wm_base, &wm_base_listener, NULL);
  }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name) {
  (void)data;
  (void)registry;
  (void)name;
  fprintf(stderr, "registry_global_removed, this is unsupported");
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

struct whey80_client *whey80_client_connect(void) {
  struct whey80_client *client = calloc(1, sizeof(*client));
  if (!client)
    return NULL;

  client->display = wl_display_connect(NULL); /* reads $WAYLAND_DISPLAY */
  if (!client->display) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    free(client);
    return NULL;
  }

  client->registry = wl_display_get_registry(client->display);
  wl_registry_add_listener(client->registry, &registry_listener, client);

  wl_display_roundtrip(client->display);

  if (!client->compositor || !client->shm || !client->xdg_wm_base) {
    fprintf(stderr, "Missing required globals (compositor=%p shm=%p xdg=%p)\n",
            (void *)client->compositor, (void *)client->shm,
            (void *)client->xdg_wm_base);
    whey80_client_destroy(client);
    return NULL;
  }

  client->running = true;
  return client;
}

void whey80_client_destroy(struct whey80_client *client) {
  if (!client)
    return;
  if (client->xdg_wm_base) {
    xdg_wm_base_destroy(client->xdg_wm_base);
  }
  if (client->shm) {
    wl_shm_destroy(client->shm);
  }
  if (client->compositor) {
    wl_compositor_destroy(client->compositor);
  }
  if (client->registry) {
    wl_registry_destroy(client->registry);
  }
  if (client->display) {
    wl_display_disconnect(client->display);
  }
  free(client);
}

void whey80_client_run(struct whey80_client *client) {
  while (client->running) {
    if (wl_display_dispatch(client->display) < 0) {
      fprintf(stderr, "Display dispatch error: %s", strerror(errno));
    }
  }
}

struct whey80_client_window *whey80_window_create(struct whey80_client *client,
                                                  int width, int height,
                                                  const char *title) {
  struct whey80_client_window *window = calloc(1, sizeof(*window));
  if (!window)
    return NULL;

  window->client = client;
  window->width = width;
  window->height = height;
  window->title = title;

  /* Layer 1: wl_surface */
  window->surface = wl_compositor_create_surface(client->compositor);

  /* Layer 2: xdg_surface */
  window->xdg_surface =
      xdg_wm_base_get_xdg_surface(client->xdg_wm_base, window->surface);
  xdg_surface_add_listener(window->xdg_surface, &surface_listener, window);

  /* Layer 3: xdg_toplevel */
  window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
  xdg_toplevel_add_listener(window->xdg_toplevel, &toplevel_listener, window);
  xdg_toplevel_set_title(window->xdg_toplevel, title);

  /* Trigger the initial configure sequence.
     This empty commit tells the compositor "I'm ready for configuration."
     The compositor responds with configure events, which eventually call
     our xdg_surface_configure callback above. */
  wl_surface_commit(window->surface);

  client->window = window;
  return window;
}

void whey80_window_destroy(struct whey80_client_window *window) {
  if (!window)
    return;
  whey80_shm_buffer_destroy(window->buffer);
  if (window->xdg_toplevel)
    xdg_toplevel_destroy(window->xdg_toplevel);
  if (window->xdg_surface)
    xdg_surface_destroy(window->xdg_surface);
  if (window->surface)
    wl_surface_destroy(window->surface);
  free(window);
}
struct whey80_shm_buffer *whey80_shm_buffer_create(struct whey80_client *client,
                                                   int width, int height) {
  struct whey80_shm_buffer *buf = calloc(1, sizeof(*buf));
  if (!buf) {
    return NULL;
  }

  buf->width = width;
  buf->height = height;
  /* ARGB888 => 4 bytes per pixel */
  buf->stride = width * 4;
  buf->size = width * height * 4;

  int fd = memfd_create("whey80-shm-buffer", MFD_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "memfd_create failed: %s\n", strerror(errno));
    close(fd);
    whey80_shm_buffer_destroy(buf);
    return NULL;
  }
  if (ftruncate(fd, buf->size) < 0) {
    fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
    close(fd);
    whey80_shm_buffer_destroy(buf);
    return NULL;
  }

  buf->pixels =
      mmap(NULL, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (buf->pixels == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    close(fd);
    whey80_shm_buffer_destroy(buf);
    return NULL;
  }

  struct wl_shm_pool *pool = wl_shm_create_pool(client->shm, fd, buf->size);
  close(fd); /* Pool has a reference to fd */

  buf->buffer = wl_shm_pool_create_buffer(pool, 0, buf->width, buf->height,
                                          buf->stride, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool); /* Not needed anymore, buffer still valid */

  return buf;
}

void whey80_shm_buffer_destroy(struct whey80_shm_buffer *buf) {
  if (!buf)
    return;
  if (buf->pixels && buf->pixels != MAP_FAILED) {
    munmap(buf->pixels, buf->size);
  }
  if (buf->buffer) {
    wl_buffer_destroy(buf->buffer);
  }
  free(buf);
}

void whey80_draw_frame(struct whey80_client_window *window) {
  if (window->draw && window->buffer) {
    window->draw(window, window->user_data);
  }
}

static void frame_done(void *data, struct wl_callback *callback,
                       uint32_t time) {
  (void)time;
  struct whey80_client_window *window = data;
  wl_callback_destroy(callback);
  whey80_submit_frame(window);
}

static const struct wl_callback_listener frame_listener = {
    .done = frame_done,
};

void whey80_submit_frame(struct whey80_client_window *window) {
  wl_surface_attach(window->surface, window->buffer->buffer, 0, 0);
  wl_surface_damage_buffer(window->surface, 0, 0, window->width,
                           window->height);

  struct wl_callback *callback = wl_surface_frame(window->surface);
  wl_callback_add_listener(callback, &frame_listener, window);

  wl_surface_commit(window->surface);
}
