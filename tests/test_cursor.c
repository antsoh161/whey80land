#include <criterion/criterion.h>
#include <wayland-server-core.h>

#include "../src/cursor.c"

Test(cursor, begin_move_sets_cursor_mode) {
  struct whey_server server = {0};
  struct wlr_cursor cursor = {0};
  server.cursor = &cursor;
  cursor.x = 100;
  cursor.y = 200;

  struct whey_toplevel toplevel = {0};
  struct wlr_scene_tree tree = {0};
  toplevel.scene_tree = &tree;
  tree.node.x = 30;
  tree.node.y = 50;

  begin_interactive_move(&server, &toplevel);

  cr_assert_eq(server.cursor_mode, WHEY_CURSOR_MOVE);
  cr_assert_eq(server.grabbed_toplevel, &toplevel);
  cr_assert_float_eq(server.grab_x, 70.0, 0.001);
  cr_assert_float_eq(server.grab_y, 150.0, 0.001);
}

Test(cursor, begin_resize_sets_cursor_mode_and_edges) {
  struct whey_server server = {0};
  struct wlr_cursor cursor = {0};
  server.cursor = &cursor;
  cursor.x = 200;
  cursor.y = 300;

  struct whey_toplevel toplevel = {0};
  struct wlr_scene_tree tree = {0};
  toplevel.scene_tree = &tree;
  tree.node.x = 10;
  tree.node.y = 20;

  struct wlr_xdg_toplevel xdg_toplevel = {0};
  struct wlr_xdg_surface xdg_surface = {0};
  xdg_toplevel.base = &xdg_surface;
  xdg_surface.geometry = (struct wlr_box){.x = 0, .y = 0, .width = 640, .height = 480};
  toplevel.xdg_toplevel = &xdg_toplevel;

  begin_interactive_resize(&server, &toplevel, WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT);

  cr_assert_eq(server.cursor_mode, WHEY_CURSOR_RESIZE);
  cr_assert_eq(server.grabbed_toplevel, &toplevel);
  cr_assert_eq(server.resize_edges, WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT);
  cr_assert_float_eq(server.grab_x, 200.0, 0.001);
  cr_assert_float_eq(server.grab_y, 300.0, 0.001);
  cr_assert_eq(server.grab_geobox.width, 640);
  cr_assert_eq(server.grab_geobox.height, 480);
}

Test(cursor, button_release_resets_to_passthrough) {
  struct whey_server server = {0};
  server.cursor_mode = WHEY_CURSOR_MOVE;

  struct whey_toplevel toplevel = {0};
  server.grabbed_toplevel = &toplevel;

  server.cursor_mode = WHEY_CURSOR_PASSTHROUGH;
  server.grabbed_toplevel = NULL;

  cr_assert_eq(server.cursor_mode, WHEY_CURSOR_PASSTHROUGH);
  cr_assert_null(server.grabbed_toplevel);
}

Test(cursor, toplevel_at_returns_null_for_empty_scene) {
  struct wlr_scene scene = {0};
  wl_list_init(&scene.tree.children);
  scene.tree.node.type = WLR_SCENE_NODE_TREE;

  struct whey_server server = {0};
  server.scene = &scene;

  struct wlr_surface *surface = NULL;
  double sx, sy;
  struct whey_toplevel *result = toplevel_at(&server, 100, 100, &surface, &sx, &sy);

  cr_assert_null(result);
  cr_assert_null(surface);
}
