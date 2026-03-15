#include "client.h"
#include <stdio.h>

static void draw(struct whey80_client_window *window, void *user_data) {
  (void)user_data;
  for (int y = 0; y < window->height; y++) {
    for (int x = 0; x < window->width; x++) {
      uint8_t r = x & 0xFF;
      uint8_t g = y & 0xFF;
      uint8_t b = ((x + y) * 2) & 0xFF;

      window->buffer->pixels[x + y * window->width] =
          (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
  }
}

int main(void) {
  struct whey80_client *client = whey80_client_connect();
  if (!client) {
    fprintf(stderr, "Failed to connect to Wayland display\n");
    return 1;
  }

  struct whey80_client_window *window =
      whey80_window_create(client, 640, 480, "simple-client");
  if (!window) {
    fprintf(stderr, "Failed to create window\n");
    whey80_client_destroy(client);
    return 1;
  }

  window->draw = draw;
  whey80_client_run(client);

  whey80_window_destroy(window);
  whey80_client_destroy(client);
  return 0;
}
