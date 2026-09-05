/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: sudoku.c - Sudoku game
 */

#include <gui.h>

enum {
    GRID_ROWS = 9,
    GRID_COLS = 9,
    GRID_CELL_WIDTH = 20,
    GRID_CELL_HEIGHT = 20,
    GRID_CELL_COUNT = GRID_ROWS * GRID_COLS,
    GRID_BORDER = 1,
    GRID_BOX_SIZE = 3,
    GRID_BOX_GAP = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER) + 2 * GRID_BOX_GAP,
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER) + 2 * GRID_BOX_GAP,
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st cell_widgets[GRID_CELL_COUNT];
    widget_st *widgets[GRID_CELL_COUNT + 2];

    grid_st grid;
} app_state_st;

static app_state_st *app_state = NULL;

static void
update_status(void)
{
    gui_status_set("");
}

static void
draw_cell(widget_st *widget)
{
    app_state_st *a = app_state;
    uint8_t bg = COLOR_WIDGET_BG;
    uint8_t fg = COLOR_WIDGET_FG;
    rect_st rect = widget->rect;
    char str[2] = { 0, 0 };

    gui_surface_draw_rect(a->window.surface, rect, bg);

    str[0] = '0' + 5;
    gui_surface_draw_str_cc(a->window.surface, rect, font_8x16, str, fg, bg);

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
}

static void
on_active_change(window_st *window)
{
    if (window->active) {
        update_status();
    }
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_sudoku.main_window = NULL;

    heap_free(app_state);
    app_state = NULL;
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
    a->window.title = "Sudoku";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_active_change = on_active_change;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_grid(void)
{
    app_state_st *a = app_state;
    int i, col, row;
    rect_st rect;

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (i = 0; i < GRID_CELL_COUNT; ++i) {
        col = i % GRID_COLS;
        row = i / GRID_COLS;
        rect = gui_grid_cell_rect(&a->grid, col, row);

        rect.x += (col / GRID_BOX_SIZE) * GRID_BOX_GAP;
        rect.y += (row / GRID_BOX_SIZE) * GRID_BOX_GAP;

        gui_button_init(&a->cell_widgets[i]);
        a->cell_widgets[i].rect = rect;
        a->cell_widgets[i].tag1 = i;
        a->cell_widgets[i].draw = draw_cell;

        gui_window_add_widget(&a->window, &a->cell_widgets[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Sudoku app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    app_sudoku.main_window = &app_state->window;

    return E_OK;
}

global app_st app_sudoku = {
    .icon = &icon_sudoku,
    .init = init_app,
};
