/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: launcher.c - App launcher
 */

#include <gui.h>

enum {
    BUTTON_MARGIN = 8,
    BUTTON_SIZE = 48,
    BUTTON_STRIDE = BUTTON_SIZE + BUTTON_MARGIN,

    GRID_COLS = 5,
    GRID_ROWS = 4,
    GRID_CELLS_COUNT = GRID_COLS * GRID_ROWS,
    GRID_X = 1 + BUTTON_MARGIN,
    GRID_Y = TITLE_BAR_HEIGHT + BUTTON_MARGIN,
    GRID_WIDTH = GRID_COLS * BUTTON_STRIDE,
    GRID_HEIGHT = GRID_ROWS * BUTTON_STRIDE,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH + 1,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT + 1,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st app_buttons[GRID_CELLS_COUNT];

    widget_st *widgets[GRID_CELLS_COUNT + 2];
} app_state_st;

static app_state_st *app_state = NULL;

static void
close_window(window_st *window)
{
    gui_wm_remove_window(window);
    app_launcher.main_window = NULL;

    heap_free(app_state);
    app_state = NULL;
}

static void
on_app_button_pointer_up(widget_st *widget, event_st event, point_st pos)
{
    app_st *app = gui_apps[widget->tag1];

    gui_button_on_pointer_up(widget, event, pos);

    close_window(widget->window);

    gui_launch_app(app);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
}

static void
init_window(void)
{
    app_state_st *a = app_state;

    a->window_surface.size.width = WINDOW_WIDTH;
    a->window_surface.size.height = WINDOW_HEIGHT;
    a->window_surface.pitch = WINDOW_WIDTH;
    a->window_surface.pixels = a->window_pixels;

    a->window.surface = &a->window_surface;
    a->window.title = "Apps";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_app_buttons(void)
{
    app_state_st *a = app_state;
    widget_st *button;
    size_t i;

    for (i = 0; i < GRID_CELLS_COUNT; ++i) {
        button = &a->app_buttons[i];

        gui_button_init(button);
        button->rect.x = GRID_X + (i % GRID_COLS) * BUTTON_STRIDE;
        button->rect.y = GRID_Y + (i / GRID_COLS) * BUTTON_STRIDE;
        button->rect.width = BUTTON_SIZE;
        button->rect.height = BUTTON_SIZE;
        button->on_pointer_up = on_app_button_pointer_up;

        if (i < gui_apps_count) {
            button->bitmap = gui_apps[i]->icon;
            button->tag1 = i;
        } else {
            button->hidden = 1;
        }

        gui_window_add_widget(&a->window, button);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Launcher app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_app_buttons();

    app_launcher.main_window = &app_state->window;

    return E_OK;
}

global app_st app_launcher = {
    .init = init_app,
};
