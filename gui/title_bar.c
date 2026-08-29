/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: title_bar.c - Window title bar
 */

#include <gui.h>

static void
gui_title_bar_on_pointer_down(widget_st *widget, event_st event, point_st pos _unsd)
{
    gui_drag_start(widget->window, event);
}

static void
gui_title_bar_on_pointer_move(widget_st *widget _unsd, event_st event, point_st pos _unsd)
{
    gui_drag_move(event);
}

static void
gui_title_bar_on_pointer_up(widget_st *widget _unsd, event_st event _unsd,
    point_st pos _unsd)
{
    gui_drag_end();
}

static void
gui_title_bar_draw(widget_st *widget)
{
    window_st *win = widget->window;
    rect_st rect = widget->rect;
    int fg_color = win->active ? COLOR_TITLE_ACT_FG : COLOR_WIDGET_FG;
    int bg_color = win->active ? COLOR_TITLE_ACT_BG : COLOR_WIDGET_BG;
    char title[64];

    snprintf(title, sizeof(title), "   %s", widget->window->title);

    gui_surface_draw_border(win->surface, rect, COLOR_BORDER);

    if (win->active) {
        rect = gui_rect_shrink(rect, 1);
        gui_surface_draw_border(win->surface, rect, COLOR_TITLE_ACT_INNER_BORDER);
    }

    rect = gui_rect_shrink(rect, 1);
    gui_surface_draw_rect(win->surface, rect, bg_color);

    gui_surface_draw_str_cc(win->surface, widget->rect,
        font_8x16, title, fg_color, bg_color);

    gui_wm_render_window_region(widget->window, widget->rect);
}

global void
gui_title_bar_init(widget_st *bar, window_st *window)
{
    bar->rect = (rect_st) {
        .x = 0,
        .y = 0,
        .width = window->surface->size.width - TITLE_BAR_HEIGHT + 1,
        .height = TITLE_BAR_HEIGHT,
    };

    bar->press_sticky = 1;

    bar->draw = gui_title_bar_draw;
    bar->on_pointer_down = gui_title_bar_on_pointer_down;
    bar->on_pointer_move = gui_title_bar_on_pointer_move;
    bar->on_pointer_up = gui_title_bar_on_pointer_up;
}
