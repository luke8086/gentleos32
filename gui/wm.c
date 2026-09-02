/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: wm.c - Window manager
 */

#include <gui.h>

enum {
    WINDOWS_COUNT_MAX = 10,
};

global rect_st gui_wm_container = { 0 };

static window_st *gui_wm_panel_window = NULL;
static window_st *gui_wm_status_window = NULL;
static window_st *gui_wm_windows[WINDOWS_COUNT_MAX];

global bitmap_st *gui_wm_wallpaper = NULL;
global bitmap_st *gui_wm_pattern = NULL;

global void
gui_wm_toggle_window_active(window_st *w, int active)
{
    if (w->active == active) {
        return;
    }

    w->active = active;

    if (active) {
        gui_status_set("");
    }

    gui_window_on_active_change(w);
}

global void
gui_wm_raise_window(struct window *w)
{
    unsigned i;

    for (i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        if (gui_wm_windows[i] == w) {
            break;
        }
    }

    if (i == WINDOWS_COUNT_MAX) {
        return;
    }

    if (i > 0) {
        for (; i > 0; --i) {
            gui_wm_windows[i] = gui_wm_windows[i - 1];
            gui_wm_toggle_window_active(gui_wm_windows[i], 0);
        }
    }

    gui_wm_windows[0] = w;
    gui_wm_toggle_window_active(w, 1);

    gui_wm_render_desktop_region(w->rect, w);
}

global int
gui_wm_free_slots(void)
{
    unsigned i;
    int count = 0;

    for (i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        if (gui_wm_windows[i] == NULL) {
            ++count;
        }
    }

    return count;
}

global int
gui_wm_add_window(struct window *w)
{
    unsigned i;

    w->draw(w);

    for (i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        if (gui_wm_windows[i] == w) {
            gui_wm_raise_window(w);
            return E_OK;
        }

        if (gui_wm_windows[i] == NULL) {
            gui_wm_windows[i] = w;
            w->visible = 1;
            gui_wm_raise_window(w);
            return E_OK;
        }
    }

    return E_TOO_MANY_WINDOWS;
}

global void
gui_wm_remove_window(struct window *w)
{
    unsigned i;

    for (i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        if (gui_wm_windows[i] == w) {
            w->visible = 0;
            gui_wm_windows[i] = NULL;
            gui_wm_toggle_window_active(w, 0);

            break;
        }
    }

    for (; i < WINDOWS_COUNT_MAX; ++i) {
        gui_wm_windows[i] = (i + 1 < WINDOWS_COUNT_MAX) ? gui_wm_windows[i + 1] : NULL;
    }

    if (gui_wm_windows[0]) {
        gui_wm_toggle_window_active(gui_wm_windows[0], 1);
    } else {
        gui_status_set("");
    }

    gui_wm_render_desktop_region(w->rect, NULL);
}

static void
gui_wm_render_wallpaper(rect_st rect)
{
    gui_fb_draw_start();

    if (gui_wm_pattern) {
        gui_fb_draw_pattern(rect, gui_wm_pattern, COLOR_DESKTOP_ALT, COLOR_DESKTOP);
    } else if (gui_wm_wallpaper && gui_wm_wallpaper->bpp == 1) {
        gui_fb_draw_pattern(rect, gui_wm_wallpaper, COLOR_DESKTOP_ALT, COLOR_DESKTOP);
    } else if (gui_wm_wallpaper && gui_wm_wallpaper->bpp == 8) {
        gui_fb_draw_wallpaper(rect, gui_wm_wallpaper);
    } else {
        gui_fb_draw_rect(rect, COLOR_DESKTOP);
    }

    gui_fb_draw_end();
}

global void
gui_wm_render_window_surface(window_st *window, rect_st desktop_reg)
{
    desktop_reg = gui_rect_clip(desktop_reg, window->rect);
    rect_st window_reg = gui_rect_translate_back(desktop_reg, window->rect.pos);

    gui_fb_draw_start();
    gui_fb_draw_surface(desktop_reg.x, desktop_reg.y, window->surface, window_reg);
    gui_fb_draw_end();
}

/*
 * Re-render a specified region of the desktop to the screen,
 * by rendering that region in all windows from the bottom up,
 * starting from a specified bottom window
 */
global void
gui_wm_render_desktop_region(rect_st rect, window_st *bottom_window)
{
    window_st *w;
    int started = (bottom_window == NULL);

    if (!bottom_window) {
        gui_wm_render_wallpaper(rect);
    }

    for (int i = WINDOWS_COUNT_MAX - 1; i >= 0; --i) {
        w = gui_wm_windows[i];

        if (w && w == bottom_window) {
            started = 1;
        }

        if (w && started) {
            gui_wm_render_window_surface(w, rect);
        }
    }
}

global void
gui_wm_render_window_region(window_st *window, rect_st window_reg)
{
    rect_st desktop_reg;

    if (!window->visible || gui_rect_is_empty(window_reg)) {
        return;
    }

    if (krn_system_info.fb_planar) {
        window_reg = gui_rect_snap_window_reg(window_reg);
    }

    desktop_reg = gui_rect_translate(window_reg, window->rect.pos);

    if (window == gui_wm_panel_window || window == gui_wm_status_window) {
        gui_wm_render_window_surface(window, desktop_reg);
    } else {
        gui_wm_render_desktop_region(desktop_reg, window);
    }
}

global window_st *
gui_wm_find_window(uint16_t x, uint16_t y)
{
    point_st p = { .x = x, .y = y };

    if (gui_wm_panel_window && gui_rect_contains_point(gui_wm_panel_window->rect, p)) {
        return gui_wm_panel_window;
    }

    for (size_t i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        window_st *w = gui_wm_windows[i];

        if (!w) {
            break;
        }

        if (gui_rect_contains_point(w->rect, p)) {
            return w;
        }
    }

    return NULL;
}

global window_st *
gui_wm_top_window(void)
{
    return gui_wm_windows[0];
}

global void
gui_wm_redraw_all(void)
{
    window_st *w;

    for (int i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        w = gui_wm_windows[i];

        if (w) {
            w->draw(w);
        }
    }

    gui_wm_render_desktop_region(gui_wm_container, NULL);

    w = gui_wm_panel_window;
    if (w) {
        w->draw(w);
        gui_wm_render_window_region(w, gui_window_area(w));
    }

    w = gui_wm_status_window;
    if (w) {
        w->draw(w);
        gui_wm_render_window_region(w, gui_window_area(w));
    }
}

global void
gui_wm_on_tick(void)
{
    for (size_t i = 0; i < WINDOWS_COUNT_MAX; ++i) {
        window_st *w = gui_wm_windows[i];

        if (!w) {
            break;
        }

        if (w->on_tick) {
            w->on_tick(w);
        }
    }
}

global void
gui_wm_set_panel_window(window_st *w)
{
    w->draw(w);
    gui_wm_panel_window = w;
    gui_wm_render_window_region(w, gui_window_area(w));
}

global void
gui_wm_set_status_window(window_st *w)
{
    w->draw(w);
    gui_wm_status_window = w;
    gui_wm_render_window_region(w, gui_window_area(w));
}

global void
gui_wm_init(void)
{
    system_info_st *si = &krn_system_info;

    gui_wm_container.x = 0;
    gui_wm_container.y = PANEL_HEIGHT;
    gui_wm_container.width = si->fb_width;
    gui_wm_container.height = si->fb_height - PANEL_HEIGHT - STATUS_HEIGHT;
    gui_wm_render_wallpaper(gui_wm_container);

    gui_status_init();
    gui_panel_init();
}
