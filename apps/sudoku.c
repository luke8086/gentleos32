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
    GRID_BOX_COUNT = GRID_BOX_SIZE * GRID_BOX_SIZE,
    GRID_BOX_GAP = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER) + 2 * GRID_BOX_GAP,
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER) + 2 * GRID_BOX_GAP,
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    UNIT_NONE = 0,
    UNIT_ROW = 1,
    UNIT_COL = 2,
    UNIT_BOX = 3,

    SOLVER_DIGIT_MASK_FULL = 0x1ff,
};

typedef struct {
    uint8_t digits[GRID_COLS][GRID_ROWS];

    uint16_t row_mask[GRID_ROWS];
    uint16_t col_mask[GRID_COLS];
    uint16_t box_mask[GRID_BOX_COUNT];
} solver_st;

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st cell_widgets[GRID_CELL_COUNT];
    widget_st *widgets[GRID_CELL_COUNT + 2];

    grid_st grid;

    uint8_t digits[GRID_COLS][GRID_ROWS];
    uint8_t givens[GRID_COLS][GRID_ROWS];
    solver_st solver;

    widget_st *active_cell;
} app_state_st;

static app_state_st *app_state = NULL;

static size_t
count_empty_cells(void)
{
    app_state_st *a = app_state;
    size_t count = 0;
    int row, col;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            if (!a->digits[col][row]) {
                ++count;
            }
        }
    }

    return count;
}

static int
find_conflicting_unit(int col, int row)
{
    app_state_st *a = app_state;
    uint8_t digit = a->digits[col][row];
    int i, dy, dx, box_col, box_row;

    if (!digit) {
        return UNIT_NONE;
    }

    for (i = 0; i < GRID_COLS; ++i) {
        if (i != col && a->digits[i][row] == digit) {
            return UNIT_ROW;
        }
    }

    for (i = 0; i < GRID_ROWS; ++i) {
        if (i != row && a->digits[col][i] == digit) {
            return UNIT_COL;
        }
    }

    for (dy = 0; dy < GRID_BOX_SIZE; ++dy) {
        for (dx = 0; dx < GRID_BOX_SIZE; ++dx) {
            box_col = col - col % GRID_BOX_SIZE + dx;
            box_row = row - row % GRID_BOX_SIZE + dy;

            if ((box_col != col || box_row != row) && a->digits[box_col][box_row] == digit) {
                return UNIT_BOX;
            }
        }
    }

    return UNIT_NONE;
}

static void
find_conflict(int *digit, int *unit)
{
    app_state_st *a = app_state;
    int row, col;

    *digit = 0;
    *unit = UNIT_NONE;

    for (row = 0; row < GRID_ROWS; ++row) {
        for (col = 0; col < GRID_COLS; ++col) {
            *unit = find_conflicting_unit(col, row);

            if (*unit) {
                *digit = a->digits[col][row];

                return;
            }
        }
    }
}

static int
is_solved(void)
{
    int digit, unit;

    if (count_empty_cells()) {
        return 0;
    }

    find_conflict(&digit, &unit);

    return unit == UNIT_NONE;
}

static void
update_status(void)
{
    static const char *ng = "N: New game";
    static const char *unit_labels[] = { NULL, "row", "column", "box" };

    int digit, unit;
    size_t count;

    find_conflict(&digit, &unit);

    if (unit != UNIT_NONE) {
        gui_status_set("Digit %d duplicated in a %s", digit, unit_labels[unit]);
        return;
    }

    count = count_empty_cells();

    if (count) {
        gui_status_set("Remaining: %u  \xb3  %s", count, ng);
        return;
    }

    gui_status_set("Solved, good job!  \xb3  %s", ng);
}

static void
shuffle_byte_array(uint8_t *values, int count)
{
    int i, j;
    uint8_t tmp;

    for (i = count - 1; i > 0; --i) {
        j = rand() % (i + 1);
        tmp = values[i];

        values[i] = values[j];
        values[j] = tmp;
    }
}

static int
solver_box_index(int col, int row)
{
    return (row / GRID_BOX_SIZE) * GRID_BOX_SIZE + col / GRID_BOX_SIZE;
}

static void
solver_set_digit(int col, int row, int digit)
{
    solver_st *s = &app_state->solver;
    int box = solver_box_index(col, row);
    uint16_t bit = 1 << (digit - 1);

    s->digits[col][row] = digit;
    s->row_mask[row] |= bit;
    s->col_mask[col] |= bit;
    s->box_mask[box] |= bit;
}

static void
solver_clear_digit(int col, int row, int digit)
{
    solver_st *s = &app_state->solver;
    int box = solver_box_index(col, row);
    uint16_t bit = 1 << (digit - 1);

    s->digits[col][row] = 0;
    s->row_mask[row] &= ~bit;
    s->col_mask[col] &= ~bit;
    s->box_mask[box] &= ~bit;
}

static uint16_t
solver_allowed_digits(int col, int row)
{
    solver_st *s = &app_state->solver;
    int box = solver_box_index(col, row);
    uint16_t used = s->row_mask[row] | s->col_mask[col] | s->box_mask[box];

    return ~used & SOLVER_DIGIT_MASK_FULL;
}

static int
solver_count_allowed_digits(uint16_t mask)
{
    int count = 0;

    while (mask) {
        mask &= mask - 1;
        ++count;
    }

    return count;
}

static void
generate_solved_board(void)
{
}

static void
clear_solution(void)
{
}

static void
generate_board(void)
{
    generate_solved_board();
    clear_solution();
}

static void
draw_cell(widget_st *widget)
{
    app_state_st *a = app_state;

    int idx = widget->tag1;
    int col = idx % GRID_COLS;
    int row = idx / GRID_COLS;
    int is_given = a->givens[col][row];
    uint8_t digit = a->digits[col][row];
    uint8_t bg = is_given ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG;
    uint8_t fg = is_given ? COLOR_WIDGET_SEL_FG : COLOR_WIDGET_FG;
    rect_st rect = widget->rect;
    char str[2] = { 0, 0 };

    if (widget == a->active_cell) {
        gui_surface_draw_rect(a->window.surface, rect, COLOR_BORDER);
        gui_surface_draw_rect(a->window.surface, gui_rect_shrink(rect, 1), bg);
    } else {
        gui_surface_draw_rect(a->window.surface, rect, bg);
    }

    if (is_given || digit) {
        str[0] = '0' + digit;
        gui_surface_draw_str_cc(a->window.surface, rect, font_8x16, str, fg, bg);
    }

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_all_cells(void)
{
    app_state_st *a = app_state;
    int i;

    for (i = 0; i < GRID_CELL_COUNT; ++i) {
        draw_cell(&a->cell_widgets[i]);
    }
}

static void
new_game(void)
{
    app_state_st *a = app_state;

    a->active_cell = &a->cell_widgets[0];
    generate_board();
    draw_all_cells();
    update_status();
}

static void
set_active_cell(widget_st *widget)
{
    app_state_st *a = app_state;
    widget_st *prev_active_cell = a->active_cell;

    if (widget == prev_active_cell) {
        return;
    }

    a->active_cell = widget;

    if (prev_active_cell) {
        gui_widget_draw(prev_active_cell);
    }

    gui_widget_draw(a->active_cell);
    update_status();
}

static void
update_active_cell(int col_step, int row_step)
{
    app_state_st *a = app_state;
    int col = 0;
    int row = 0;
    int idx;

    if (a->active_cell) {
        idx = a->active_cell->tag1;

        col = MAX(0, MIN(GRID_COLS - 1, idx % GRID_COLS + col_step));
        row = MAX(0, MIN(GRID_ROWS - 1, idx / GRID_COLS + row_step));
    }

    set_active_cell(&a->cell_widgets[row * GRID_COLS + col]);
}

static void
on_cell_pointer_down(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    set_active_cell(widget);
}

static void
on_key_down(window_st *window _unsd, event_st event)
{
    app_state_st *a = app_state;
    int digit, idx, col, row;

    switch (event.key_code) {
    case KEY_UP: update_active_cell(0, -1); return;
    case KEY_DOWN: update_active_cell(0, 1); return;
    case KEY_LEFT: update_active_cell(-1, 0); return;
    case KEY_RIGHT: update_active_cell(1, 0); return;
    case KEY_N: new_game(); return;
    case KEY_BKSP:
    case KEY_DEL:
    case KEY_SPACE: digit = 0; break;
    default: digit = key_number_for_code(event.key_code); break;
    }

    if (digit < 0 || is_solved()) {
        return;
    }

    idx = a->active_cell->tag1;
    col = idx % GRID_COLS;
    row = idx / GRID_COLS;

    if (a->givens[col][row] || a->digits[col][row] == digit) {
        return;
    }

    a->digits[col][row] = digit;

    gui_widget_draw(a->active_cell);
    update_status();
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
    a->window.on_key_down = on_key_down;
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
        a->cell_widgets[i].on_pointer_down = on_cell_pointer_down;

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
    new_game();

    app_sudoku.main_window = &app_state->window;

    return E_OK;
}

global app_st app_sudoku = {
    .icon = &icon_sudoku,
    .init = init_app,
};
