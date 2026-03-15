#pragma once

#include <stdbool.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

struct whey80_client;
struct whey80_client_window;
struct whey80_shm_buffer;

typedef void (*whey80_draw_fn)(struct whey80_client_window *window,
                               void *user_data);

struct whey80_shm_buffer {
  struct wl_buffer *buffer;
  uint32_t *pixels;
  int width;
  int height;
  int stride;
  int size;
};

struct whey80_client_window {
  struct whey80_client *client;
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct whey80_shm_buffer *buffer;
  bool configured;
  int width;
  int height;
  const char *title;
  whey80_draw_fn draw;
  void *user_data;
};

struct whey80_client {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct xdg_wm_base *xdg_wm_base;
  struct whey80_client_window *window;
  bool running;
};

struct whey80_client *whey80_client_connect(void);
void whey80_client_destroy(struct whey80_client *client);
void whey80_client_run(struct whey80_client *client);

struct whey80_client_window *whey80_window_create(struct whey80_client *client,
                                                  int width, int height,
                                                  const char *title);
void whey80_window_destroy(struct whey80_client_window *window);

struct whey80_shm_buffer *whey80_shm_buffer_create(struct whey80_client *client,
                                                   int width, int height);
void whey80_shm_buffer_destroy(struct whey80_shm_buffer *buf);

void whey80_draw_frame(struct whey80_client_window *window);
void whey80_submit_frame(struct whey80_client_window *window);
