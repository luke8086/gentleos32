/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: calendar.c - Calendar app
 */

#include <gui.h>

enum {
    GRID_CELL_WIDTH = 32,
    GRID_CELL_HEIGHT = 32,

    TOOL_BAR_Y = TITLE_BAR_HEIGHT - 1,
    TOOL_BAR_HEIGHT = GRID_CELL_HEIGHT + 2,

    WEEK_BAR_Y = (TOOL_BAR_Y + TOOL_BAR_HEIGHT - 1),
    WEEK_BAR_HEIGHT = 24,

    GRID_ROWS = 6,
    GRID_COLS = 7,
    GRID_CELLS_COUNT = GRID_ROWS * GRID_COLS,
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = WEEK_BAR_Y + WEEK_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,
};

enum {
    MIN_YEAR = 1900,
    MAX_YEAR = 2099,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st prev_button;
    widget_st next_button;
    widget_st day_buttons[GRID_CELLS_COUNT];
    widget_st *widgets[GRID_CELLS_COUNT + 4];

    int current_month;
    int current_year;
    int current_day;

    int selected_month;
    int selected_year;

    grid_st grid;
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_month_label(void)
{
    app_state_st *a = app_state;

    char buf[16];
    snprintf(buf, sizeof(buf), "%s %d", TIME_MONTH_NAMES_SHORT[a->selected_month - 1], a->selected_year);

    rect_st rect = {
        .x = TOOL_BAR_HEIGHT - 1,
        .y = TOOL_BAR_Y,
        .width = WINDOW_WIDTH - (2 * TOOL_BAR_HEIGHT) + 2,
        .height = TOOL_BAR_HEIGHT,
    };

    gui_surface_draw_border(a->window.surface, rect, COLOR_BORDER);
    gui_surface_draw_str_cc(a->window.surface, rect, font_8x16, buf,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_day_button(widget_st *widget)
{
    app_state_st *a = app_state;

    int day = widget->tag1;
    int num_days = time_get_days_in_month(a->selected_month, a->selected_year);

    int is_in_month = day >= 0 && day < num_days;
    int is_current = (day == a->current_day - 1 && a->selected_month == a->current_month
        && a->selected_year == a->current_year);
    int is_pressed = (widget == widget->window->pressed_widget) || widget->active;
    int fg, bg;

    if (!is_in_month) {
        gui_surface_draw_rect(widget->window->surface, widget->rect, COLOR_WIDGET_BG);
        gui_wm_render_window_region(widget->window, widget->rect);
        return;
    }

    if (is_pressed) {
        fg = COLOR_WIDGET_SEL_FG;
        bg = COLOR_WIDGET_SEL_BG;
    } else if (is_current) {
        fg = COLOR_TITLE_ACT_FG;
        bg = COLOR_TITLE_ACT_BG;
    } else {
        fg = COLOR_WIDGET_FG;
        bg = COLOR_WIDGET_BG;
    }

    gui_surface_draw_rect(widget->window->surface, widget->rect, bg);

    char buf[3];
    snprintf(buf, sizeof(buf), "%d", day + 1);
    gui_surface_draw_str_cc(
        widget->window->surface,
        widget->rect,
        widget->font ? widget->font : font_8x16,
        buf,
        fg,
        bg
    );

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
draw_selected_month(void)
{
    app_state_st *a = app_state;

    int day_of_week = time_get_day_of_week(1, a->selected_month, a->selected_year);

    for (size_t i = 0; i < GRID_CELLS_COUNT; ++i) {
        a->day_buttons[i].tag1 = i - day_of_week;
        gui_widget_draw(&a->day_buttons[i]);
    }

    draw_month_label();
}

static void
draw_week_bar(void)
{
    app_state_st *a = app_state;

    for (int y = 0; y < 7; y++) {
        rect_st rect = {
            .x = y * (GRID_CELL_WIDTH + 2) - y,
            .y = WEEK_BAR_Y,
            .width = GRID_CELL_WIDTH + 2,
            .height = WEEK_BAR_HEIGHT,
        };

        gui_surface_draw_border(a->window.surface, rect, COLOR_BORDER);
        gui_surface_draw_str_cc(a->window.surface, rect, font_8x16,
            TIME_DAY_NAMES_SHORT[y], COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    }
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    gui_window_draw(window, COLOR_WIDGET_BG);
    gui_grid_fill(&a->grid, window, COLOR_BORDER);
    draw_week_bar();
    draw_selected_month();
}

static void
on_prev_button(widget_st *widget _unsd, event_st event,
    point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    if (a->selected_month > 1) {
        a->selected_month -= 1;
    } else if (a->selected_year > MIN_YEAR) {
        a->selected_year -= 1;
        a->selected_month = 12;
    } else {
        return;
    }

    draw_selected_month();
}

static void
on_next_button(widget_st *widget _unsd, event_st event,
    point_st pos)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    if (a->selected_month < 12) {
        a->selected_month += 1;
    } else if (a->selected_year < MAX_YEAR) {
        a->selected_year += 1;
        a->selected_month = 1;
    } else {
        return;
    }

    draw_selected_month();
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_calendar.main_window = NULL;

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
    a->window.title = "Calendar";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
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
init_day_buttons(void)
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
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;

        gui_button_init(&a->day_buttons[i]);
        a->day_buttons[i].rect = gui_grid_cell_rect(&a->grid, col, row);
        a->day_buttons[i].draw = draw_day_button;
        a->day_buttons[i].font = font_8x16;
        a->day_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &a->day_buttons[i]);
    }
}

static void
init_current_date(void)
{
    app_state_st *a = app_state;

    time_st t;
    time_get(&t);

    a->current_month = t.month;
    a->current_year = t.year;
    a->current_day = t.day;

    a->selected_month = a->current_month;
    a->selected_year = a->current_year;
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Calendar app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_buttons();
    init_day_buttons();
    init_current_date();

    app_calendar.main_window = &app_state->window;

    return E_OK;
}

global app_st app_calendar = {
    .icon = &icon_calendar,
    .init = init_app,
};
