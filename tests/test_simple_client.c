#include <criterion/criterion.h>

#define main simple_main
#include "../clients/simple-client.c"
#undef main

static uint32_t expected_pixel(int x, int y) {
  uint8_t r = x & 0xFF;
  uint8_t g = y & 0xFF;
  uint8_t b = ((x + y) * 2) & 0xFF;
  return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

Test(simple_client, draw_pixel_at_origin) {
  enum { W = 640, H = 480 };
  uint32_t pixels[W * H];
  struct whey80_shm_buffer buf = {.pixels = pixels, .width = W, .height = H};
  struct whey80_client_window window = {.width = W, .height = H, .buffer = &buf};

  draw(&window, NULL);

  cr_assert_eq(pixels[0], 0xFF000000,
               "pixel at (0,0) should be opaque black, got 0x%08X", pixels[0]);
}

Test(simple_client, draw_pixel_at_known_coordinates) {
  enum { W = 640, H = 480 };
  uint32_t pixels[W * H];
  struct whey80_shm_buffer buf = {.pixels = pixels, .width = W, .height = H};
  struct whey80_client_window window = {.width = W, .height = H, .buffer = &buf};

  draw(&window, NULL);

  struct {
    int x, y;
  } cases[] = {{0, 0}, {255, 0}, {0, 255}, {128, 64}, {639, 479}};

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    int x = cases[i].x, y = cases[i].y;
    uint32_t got = pixels[x + y * W];
    uint32_t want = expected_pixel(x, y);
    cr_assert_eq(got, want,
                 "pixel at (%d,%d): got 0x%08X, want 0x%08X", x, y, got, want);
  }
}

Test(simple_client, draw_fills_all_pixels_with_full_alpha) {
  enum { W = 64, H = 48 };
  uint32_t pixels[W * H];
  struct whey80_shm_buffer buf = {.pixels = pixels, .width = W, .height = H};
  struct whey80_client_window window = {.width = W, .height = H, .buffer = &buf};

  draw(&window, NULL);

  for (int i = 0; i < W * H; i++) {
    cr_assert_eq(pixels[i] >> 24, 0xFF,
                 "pixel %d should have full alpha", i);
  }
}
