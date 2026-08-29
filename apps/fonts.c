/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: fonts.c - Font browser app
 */

#include <gui.h>

enum {
    TOOL_BAR_Y = TITLE_BAR_HEIGHT - 1,
    TOOL_BAR_HEIGHT = 30,

    GRID_CELL_WIDTH = 18,
    GRID_CELL_HEIGHT = 18,
    GRID_ROWS = 8,
    GRID_COLS = 16,
    GRID_CELLS_COUNT = (GRID_ROWS * GRID_COLS),
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = TOOL_BAR_Y + TOOL_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st prev_button;
    widget_st next_button;
    widget_st char_buttons[GRID_CELLS_COUNT];
    widget_st *widgets[GRID_CELLS_COUNT + 4];

    widget_st *active_char_button;
    size_t current_font;

    grid_st grid;
} app_state_st;

static app_state_st *app_state = NULL;

static void
update_status(void)
{
    app_state_st *a = app_state;

    if (!a->active_char_button) {
        gui_status_set("");
        return;
    }

    uint8_t n = a->active_char_button->tag2;

    gui_status_set("Char:%c  Hex:%02x  Dec:%d", n ? n : ' ', n, n);
}

static void
draw_char_button(widget_st *widget)
{
    app_state_st *a = app_state;

    char str[2] = { widget->tag2 ? widget->tag2 : ' ', 0 };
    int is_active = widget == a->active_char_button;
    rect_st rect = widget->rect;

    gui_surface_draw_rect(a->window.surface, rect,
        is_active ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG);

    --rect.height;

    gui_surface_draw_str_cc(
        a->window.surface,
        rect,
        &fonts[a->current_font],
        (const char *)str,
        is_active ? COLOR_WIDGET_SEL_FG : COLOR_WIDGET_FG,
        is_active ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG
    );

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
draw_all_char_buttons(void)
{
    app_state_st *a = app_state;

    for (size_t i = 0; i < GRID_CELLS_COUNT; ++i) {
        a->char_buttons[i].draw(&a->char_buttons[i]);
    }
}

static void
draw_font_label(void)
{
    app_state_st *a = app_state;

    rect_st r = {
        .x = TOOL_BAR_HEIGHT - 1,
        .y = TOOL_BAR_Y,
        .width = WINDOW_WIDTH - 2 * TOOL_BAR_HEIGHT + 2,
        .height = TOOL_BAR_HEIGHT,
    };

    gui_surface_draw_border(a->window.surface, r, COLOR_BORDER);
    gui_surface_draw_rect(a->window.surface, gui_rect_shrink(r, 1), COLOR_WIDGET_BG);
    gui_surface_draw_str_cc(a->window.surface, r, font_8x16,
        fonts[a->current_font].name, COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, r);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
    draw_font_label();
}

static void
on_prev_button(widget_st *widget _unsd, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    a->current_font = (a->current_font - 1) % FONT_COUNT;

    draw_font_label();
    draw_all_char_buttons();
}

static void
on_next_button(widget_st *widget _unsd, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    a->current_font = (a->current_font + 1) % FONT_COUNT;

    draw_font_label();
    draw_all_char_buttons();
}

static void
on_char_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev_active_char_button = a->active_char_button;

    a->active_char_button = widget;

    if (a->active_char_button == prev_active_char_button) {
        return;
    }

    if (prev_active_char_button) {
        gui_widget_draw(prev_active_char_button);
    }

    gui_widget_draw(a->active_char_button);

    update_status();
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
    app_fonts.main_window = NULL;

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
    a->window.title = "Fonts";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.on_active_change = on_active_change;
    a->window.draw = draw_window;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_buttons(void)
{
    app_state_st *a = app_state;

    gui_button_init(&a->prev_button);
    a->prev_button.rect.x = 0;
    a->prev_button.rect.y = TOOL_BAR_Y;
    a->prev_button.rect.width = TOOL_BAR_HEIGHT;
    a->prev_button.rect.height = TOOL_BAR_HEIGHT;
    a->prev_button.label = "<";
    a->prev_button.on_pointer_up = on_prev_button;

    gui_button_init(&a->next_button);
    a->next_button.rect.x = WINDOW_WIDTH - TOOL_BAR_HEIGHT;
    a->next_button.rect.y = TOOL_BAR_Y;
    a->next_button.rect.width = TOOL_BAR_HEIGHT;
    a->next_button.rect.height = TOOL_BAR_HEIGHT;
    a->next_button.label = ">";
    a->next_button.on_pointer_up = on_next_button;

    gui_window_add_widget(&a->window, &a->prev_button);
    gui_window_add_widget(&a->window, &a->next_button);
}

static void
init_char_buttons(void)
{
    app_state_st *a = app_state;

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (size_t i = 0; i < GRID_CELLS_COUNT; ++i) {
        int col = i % a->grid.cols;
        int row = i / a->grid.cols;

        gui_button_init(&a->char_buttons[i]);
        a->char_buttons[i].rect = gui_grid_cell_rect(&a->grid, col, row);
        a->char_buttons[i].tag2 = i;
        a->char_buttons[i].window = &a->window;
        a->char_buttons[i].draw = draw_char_button;
        a->char_buttons[i].on_pointer_down = on_char_button_press;
        a->char_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &a->char_buttons[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Fonts app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_buttons();
    init_char_buttons();

    app_fonts.main_window = &app_state->window;

    return E_OK;
}

global app_st app_fonts = {
    .icon = &icon_fonts,
    .init = init_app,
};
