/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: panel.c - Top panel
 */

#include <gui.h>

static surface_st window_surface;
static window_st window;

static widget_st apps_button;
static widget_st *widgets[1];

static void
on_apps_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    gui_button_on_pointer_up(widget, event, pos);
    gui_launch_app(&app_launcher);
}

static void
draw_window(window_st *window)
{
    gui_surface_draw_rect(window->surface, gui_window_area(window), COLOR_WIDGET_BG);
    gui_surface_draw_h_seg(window->surface, 0, PANEL_HEIGHT - 1, window->rect.width,
        COLOR_BORDER);

    gui_widget_draw(&apps_button);
}

static void
init_window(void)
{
    system_info_st *si = &krn_system_info;
    int width = si->fb_width;

    window_surface.size.width = width;
    window_surface.size.height = PANEL_HEIGHT;
    window_surface.pitch = width;
    window_surface.pixels = heap_alloc(width * PANEL_HEIGHT, "Panel pixels", 1);

    window.rect.x = 0;
    window.rect.y = 0;
    window.rect.width = width;
    window.rect.height = PANEL_HEIGHT;
    window.surface = &window_surface;
    window.widgets = widgets;
    window.widgets_capacity = sizeof(widgets) / sizeof(widgets[0]);
    window.visible = 1;
    window.draw = draw_window;
}

static void
init_apps_button(void)
{
    font_st *font = font_8x16;

    gui_button_init(&apps_button);
    apps_button.font = font;
    apps_button.label = "Apps";
    apps_button.rect.x = 0;
    apps_button.rect.y = 0;
    apps_button.rect.width = (strlen(apps_button.label) + 2) * font->size.width;
    apps_button.rect.height = PANEL_HEIGHT - 1;
    apps_button.hide_border = 1;
    apps_button.on_pointer_up = on_apps_pointer_up;

    gui_window_add_widget(&window, &apps_button);
}

global void
gui_panel_init(void)
{
    init_window();
    init_apps_button();

    gui_wm_set_panel_window(&window);
}
