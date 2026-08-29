/* gui/button.c */
extern void gui_button_init(widget_st *widget);
extern void gui_button_on_pointer_down(widget_st *widget, event_st event, point_st pos);
extern void gui_button_on_pointer_up(widget_st *widget, event_st event, point_st pos);
extern void gui_button_on_pointer_out(widget_st *widget, event_st event, point_st pos);
extern void gui_button_draw(widget_st *widget);
/* gui/close_button.c */
extern void gui_close_button_init(widget_st *button, window_st *window);
/* gui/drag.c */
extern void gui_drag_start(window_st *window, event_st event);
extern void gui_drag_move(event_st event);
extern void gui_drag_end(void);
extern void gui_drag_draw_outline(void);
extern void gui_drag_clear_outline(void);
/* gui/fb.c */
extern surface_st *gui_fb_vram_surface;
extern rect_st gui_fb_screen_rect;
extern void gui_fb_draw_start(void);
extern void gui_fb_draw_end(void);
extern void gui_fb_mark_dirty(rect_st rect);
extern void gui_fb_draw_rect(rect_st rect, uint8_t color);
extern void gui_fb_draw_pattern(rect_st rect, bitmap_st *pattern, uint8_t c1, uint8_t c2);
extern void gui_fb_draw_surface(int dst_x, int dst_y, surface_st *src_sf, rect_st src_rect);
extern void gui_fb_draw_wallpaper(rect_st rect, bitmap_st *bitmap);
extern void gui_fb_draw_outline(rect_st rect);
extern void gui_fb_flush(void);
extern void gui_fb_init(void);
/* gui/grid.c */
extern rect_st gui_grid_rect(grid_st *grid);
extern rect_st gui_grid_cell_rect(grid_st *grid, int col, int row);
extern grid_pos_st gui_grid_cell_at(grid_st *grid, point_st pos);
extern void gui_grid_fill(grid_st *grid, window_st *window, uint8_t color);
/* gui/listwgt.c */
extern void gui_list_widget_init(list_widget_st *list);
extern void gui_list_widget_set_item_count(list_widget_st *list, int item_count);
extern void gui_list_widget_set_page(list_widget_st *list, int page);
extern void gui_list_widget_set_index(list_widget_st *list, int index);
extern void gui_list_widget_draw(widget_st *widget);
extern void gui_list_widget_on_pointer_down(widget_st *widget, event_st event, point_st pos);
extern void gui_list_widget_init_page_buttons(list_widget_st *list, widget_st *prev_btn, widget_st *next_btn);
/* gui/main.c */
extern app_st *gui_apps[];
extern unsigned gui_apps_count;
extern bitmap_st *gui_load_bitmap(const char *name);
extern void gui_launch_app(app_st *app);
extern int gui_handle_key(event_st event);
extern void gui_main(void);
/* gui/panel.c */
extern void gui_panel_init(void);
/* gui/planar.c */
extern void gui_planar_flush(rect_st rect);
extern void gui_planar_draw_rect(rect_st rect, uint8_t color);
extern void gui_planar_draw_pattern_abs(rect_st dst_rect, bitmap_st *pattern, uint8_t c1, uint8_t c2);
extern void gui_planar_draw_surface(int dst_x, int dst_y, surface_st *src, rect_st src_rect);
extern void gui_planar_draw_pointer(int dst_x, int dst_y);
extern void gui_planar_xor_corners(rect_st rect);
extern void gui_planar_init(void);
/* gui/pointer.c */
extern bitmap_st bitmap_pointer;
extern void gui_pointer_draw(void);
extern void gui_pointer_move(uint16_t x, uint16_t y);
extern void gui_pointer_init(void);
/* gui/progbar.c */
extern void gui_progress_bar_init(progress_bar_st *bar, int max_value, int cur_value);
extern void gui_progress_bar_draw(widget_st *widget);
extern void gui_progress_bar_set_values(progress_bar_st *bar, int max_value, int cur_value);
extern void gui_progress_bar_on_pointer_down(widget_st *widget, event_st event, point_st pos);
/* gui/rect.c */
extern int gui_rect_is_empty(rect_st r);
extern int gui_rect_area(rect_st r);
extern int gui_rect_touches(rect_st a, rect_st b);
extern rect_st gui_rect_make(int x, int y, int width, int height);
extern rect_st gui_rect_translate(rect_st r, point_st v);
extern rect_st gui_rect_translate_back(rect_st r, point_st v);
extern int gui_rect_contains_point(rect_st r, point_st p);
extern rect_st gui_rect_center(rect_st r, rect_st container);
extern rect_st gui_rect_limit(rect_st r, rect_st container);
extern rect_st gui_rect_snap_window_pos(rect_st r);
extern rect_st gui_rect_snap_window_reg(rect_st r);
extern rect_st gui_rect_shrink(rect_st r, int amount);
extern rect_st gui_rect_enclose(rect_st a, rect_st b);
extern rect_st gui_rect_clip(rect_st r, rect_st clipper);
extern void gui_rect_translate_diff(rect_st r1, rect_st r2, rect_st *hdiff, rect_st *vdiff);
extern const char *gui_rect_format(rect_st r);
/* gui/status.c */
extern void gui_status_set(const char *fmt, ...);
extern void gui_status_set_error(int err);
extern void gui_status_set_alert(const char *fmt, ...);
extern void gui_status_init(void);
/* gui/surface.c */
extern void gui_surface_copy(surface_st *dst_sf, int dst_x, int dst_y, surface_st *src_sf, rect_st src_rect);
extern void gui_surface_draw_h_seg(surface_st *surface, int x, int y, int w, uint8_t color);
extern void gui_surface_draw_v_seg(surface_st *surface, int x, int y, int h, uint8_t color);
extern void gui_surface_draw_border(surface_st *surface, rect_st r, uint8_t color);
extern void gui_surface_draw_rect(surface_st *surface, rect_st r, uint8_t color);
extern void gui_surface_draw_char(surface_st *surface, uint16_t x, uint16_t y, font_st *font, uint8_t ch, uint8_t fg, uint8_t bg);
extern void gui_surface_draw_str_at(surface_st *surface, uint16_t x, uint16_t y, font_st *font, const char *s, uint8_t fg, uint8_t bg);
extern void gui_surface_draw_str_cc(surface_st *surface, rect_st rect, font_st *font, const char *s, uint8_t fg, uint8_t bg);
extern void gui_surface_draw_str_cl(surface_st *surface, rect_st rect, int padding, font_st *font, const char *s, uint8_t fg, uint8_t bg);
extern void gui_surface_draw_str_cr(surface_st *surface, rect_st rect, int padding, font_st *font, const char *s, uint8_t fg, uint8_t bg);
extern void gui_surface_draw_bitmap_1bpp(surface_st *surface, rect_st src_rect, int dst_x, int dst_y, bitmap_st *bitmap, uint8_t fill);
extern void gui_surface_draw_bitmap(surface_st *surface, int dst_x, int dst_y, bitmap_st *bitmap, uint8_t fill);
extern void gui_surface_draw_bitmap_centered(surface_st *surface, rect_st rect, bitmap_st *bitmap, uint8_t fill);
extern void gui_surface_draw_pattern_abs(surface_st *surface, rect_st reg, bitmap_st *b, uint8_t col1, uint8_t col2);
extern void gui_surface_draw_pattern_rel(surface_st *surface, rect_st reg, bitmap_st *b, uint8_t col1, uint8_t col2);
/* gui/theme.c */
extern gui_theme_st gui_theme;
extern void gui_theme_set(int theme);
/* gui/title_bar.c */
extern void gui_title_bar_init(widget_st *bar, window_st *window);
/* gui/widget.c */
extern void gui_widget_draw(widget_st *widget);
/* gui/window.c */
extern rect_st gui_window_area(window_st *window);
extern void gui_window_init_frame(window_st *window, widget_st *title_bar, widget_st *close_button);
extern void gui_window_draw_frame(window_st *window, uint8_t bg_color);
extern void gui_window_draw_widgets(window_st *window);
extern int gui_window_add_widget(window_st *window, widget_st *widget);
extern void gui_window_draw(window_st *window, uint8_t bg_color);
extern widget_st *gui_window_find_widget_at(window_st *window, point_st pos);
extern void gui_window_on_pointer_out(window_st *window, event_st event, point_st pos);
extern void gui_window_on_pointer_down(window_st *window, event_st event);
extern void gui_window_on_pointer_move(window_st *window, event_st event);
extern void gui_window_on_pointer_up(window_st *window, event_st event);
extern void gui_window_on_pointer_alt(window_st *window, event_st event);
extern void gui_window_on_active_change(window_st *window);
/* gui/wm.c */
extern rect_st gui_wm_container;
extern bitmap_st *gui_wm_wallpaper;
extern bitmap_st *gui_wm_pattern;
extern void gui_wm_toggle_window_active(window_st *w, int active);
extern void gui_wm_raise_window(struct window *w);
extern int gui_wm_free_slots(void);
extern int gui_wm_add_window(struct window *w);
extern void gui_wm_remove_window(struct window *w);
extern void gui_wm_render_window_surface(window_st *window, rect_st desktop_reg);
extern void gui_wm_render_desktop_region(rect_st rect, window_st *bottom_window);
extern void gui_wm_render_window_region(window_st *window, rect_st window_reg);
extern window_st *gui_wm_find_window(uint16_t x, uint16_t y);
extern window_st *gui_wm_top_window(void);
extern void gui_wm_redraw_all(void);
extern void gui_wm_on_tick(void);
extern void gui_wm_set_panel_window(window_st *w);
extern void gui_wm_set_status_window(window_st *w);
extern void gui_wm_init(void);
