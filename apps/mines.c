/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mines.c - Minesweeper game
 */

#include <gui.h>

static void reveal_cell(int col, int row);

enum {
    GRID_CELL_WIDTH = 17,
    GRID_CELL_HEIGHT = 17,
    GRID_ROWS = 11,
    GRID_COLS = 18,
    GRID_CELL_COUNT = GRID_ROWS * GRID_COLS,
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER),
    GRID_X = 0,
    GRID_Y = TITLE_BAR_HEIGHT - 1,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    MINE_COUNT = 32,
};

enum {
    CELL_STATE_HIDDEN = 0,
    CELL_STATE_REVEALED = 1,
    CELL_STATE_FLAGGED = 2,
};

enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_MINE = 9,
};

enum {
    GAME_STATE_PLAYING = 0,
    GAME_STATE_WON = 1,
    GAME_STATE_LOST = 2,
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

    uint8_t cell_state[GRID_COLS][GRID_ROWS];
    uint8_t cell_type[GRID_COLS][GRID_ROWS];
} app_state_st;

static app_state_st *app_state = NULL;

static size_t
count_cells_by_state(uint8_t state)
{
    app_state_st *a = app_state;
    size_t count = 0;

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (a->cell_state[x][y] == state) {
                count++;
            }
        }
    }

    return count;
}

static size_t
count_adjacent_mines(int col, int row)
{
    app_state_st *a = app_state;
    size_t count = 0;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            int nx = col + dx;
            int ny = row + dy;

            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS) {
                if (a->cell_type[nx][ny] == CELL_TYPE_MINE) {
                    ++count;
                }
            }
        }
    }

    return count;
}

static void
draw_cell(widget_st *widget)
{
    app_state_st *a = app_state;
    int idx = widget->tag1;
    int col = idx % GRID_COLS;
    int row = idx / GRID_COLS;
    uint8_t state = a->cell_state[col][row];
    uint8_t type = a->cell_type[col][row];
    rect_st rect = widget->rect;
    rect_st dot_rect, num_rect;
    int pressed = widget->window->pressed_widget == widget;
    int activated = pressed && state == CELL_STATE_HIDDEN;
    char num_str[2] = { 0, 0 };

    gui_surface_draw_rect(a->window.surface, rect,
        activated ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG);

    if (state == CELL_STATE_FLAGGED) {
        gui_surface_draw_bitmap_centered(a->window.surface, rect, &sprite_flag,
            COLOR_WIDGET_FG);
    } else if (state == CELL_STATE_REVEALED && type == CELL_TYPE_MINE) {
        gui_surface_draw_bitmap_centered(a->window.surface, rect, &sprite_mine,
            COLOR_WIDGET_FG);
    } else if (state == CELL_STATE_REVEALED && type == CELL_TYPE_EMPTY) {
        dot_rect = gui_rect_make(rect.x + rect.width / 2 - 1, rect.y + rect.height / 2, 2, 1);
        gui_surface_draw_rect(a->window.surface, dot_rect, COLOR_WIDGET_FG);
    } else if (state == CELL_STATE_REVEALED) {
        num_str[0] = '0' + type;
        num_rect = gui_rect_make(rect.x + 1, rect.y, rect.width - 1, rect.height);

        gui_surface_draw_str_cc(a->window.surface, num_rect, font_8x8,
            num_str, COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    }

    gui_wm_render_window_region(&a->window, rect);
}

static void
update_cell(int col, int row, uint8_t type, uint8_t state)
{
    app_state_st *a = app_state;

    a->cell_type[col][row] = type;
    a->cell_state[col][row] = state;
    draw_cell(&a->cell_widgets[row * GRID_COLS + col]);
}

static void
update_all_mines(uint8_t state)
{
    app_state_st *a = app_state;

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            if (a->cell_type[col][row] == CELL_TYPE_MINE) {
                update_cell(col, row, CELL_TYPE_MINE, state);
            }
        }
    }
}

static void
clear_cells(void)
{
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            update_cell(col, row, CELL_TYPE_EMPTY, CELL_STATE_HIDDEN);
        }
    }
}

static void
place_mines(int except_col, int except_row)
{
    app_state_st *a = app_state;
    int remaining = MINE_COUNT;

    while (remaining > 0) {
        int col = rand() % GRID_COLS;
        int row = rand() % GRID_ROWS;

        if (col == except_col && row == except_row) {
            continue;
        }

        if (a->cell_type[col][row] != CELL_TYPE_MINE) {
            a->cell_type[col][row] = CELL_TYPE_MINE;
            --remaining;
        }
    }
}

static int
get_game_state(void)
{
    app_state_st *a = app_state;

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            if (a->cell_type[col][row] == CELL_TYPE_MINE &&
                a->cell_state[col][row] == CELL_STATE_REVEALED) {
                return GAME_STATE_LOST;
            }
        }
    }

    if (count_cells_by_state(CELL_STATE_REVEALED) == (GRID_CELL_COUNT - MINE_COUNT)) {
        return GAME_STATE_WON;
    }

    return GAME_STATE_PLAYING;
}

static void
update_status(void)
{
    int state = get_game_state();

    if (state == GAME_STATE_LOST) {
        gui_status_set("Game Over! Press any cell to start a new game");
    } else if (state == GAME_STATE_WON) {
        gui_status_set("You Win! Press any cell to start a new game");
    } else {
        size_t flagged_count = count_cells_by_state(CELL_STATE_FLAGGED);
        size_t remaining = MINE_COUNT > flagged_count ? MINE_COUNT - flagged_count : 0;

        gui_status_set("Remaining mines: %u", remaining);
    }
}

static void
restart_game(void)
{
    clear_cells();
    update_status();
}

static void
reveal_adjacent_cells(int col, int row)
{
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) {
                continue;
            }

            reveal_cell(col + dx, row + dy);
        }
    }
}

static void
reveal_cell(int col, int row)
{
    app_state_st *a = app_state;

    if (col < 0 || col >= GRID_COLS || row < 0 || row >= GRID_ROWS) {
        return;
    }

    if (a->cell_state[col][row] != CELL_STATE_HIDDEN) {
        return;
    }

    if (count_cells_by_state(CELL_STATE_REVEALED) == 0) {
        place_mines(col, row);
    }

    if (a->cell_type[col][row] == CELL_TYPE_MINE) {
        update_all_mines(CELL_STATE_REVEALED);
        update_status();
        return;
    };

    int adjacent_mine_count = count_adjacent_mines(col, row);
    update_cell(col, row, adjacent_mine_count, CELL_STATE_REVEALED);

    if (adjacent_mine_count == 0) {
        reveal_adjacent_cells(col, row);
    }

    if (get_game_state() == GAME_STATE_WON) {
        update_all_mines(CELL_STATE_FLAGGED);
    }

    update_status();
}

static void
on_cell_pointer_down(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    draw_cell(widget);
}

static void
on_cell_pointer_up(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    if (get_game_state() != GAME_STATE_PLAYING) {
        restart_game();
        return;
    }

    int idx = widget->tag1;
    int col = idx % GRID_COLS;
    int row = idx / GRID_COLS;

    if (a->cell_state[col][row] != CELL_STATE_HIDDEN) {
        return;
    }

    reveal_cell(col, row);
}

static void
on_cell_pointer_alt(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    if (get_game_state() != GAME_STATE_PLAYING) {
        return;
    }

    int idx = widget->tag1;
    int col = idx % GRID_COLS;
    int row = idx / GRID_COLS;

    if (a->cell_state[col][row] == CELL_STATE_HIDDEN) {
        update_cell(col, row, a->cell_type[col][row], CELL_STATE_FLAGGED);
        update_status();
    } else if (a->cell_state[col][row] == CELL_STATE_FLAGGED) {
        update_cell(col, row, a->cell_type[col][row], CELL_STATE_HIDDEN);
        update_status();
    }
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
    app_mines.main_window = NULL;

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
    a->window.title = "Mines";
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

    a->grid.cell_width = GRID_CELL_WIDTH;
    a->grid.cell_height = GRID_CELL_HEIGHT;
    a->grid.cols = GRID_COLS;
    a->grid.rows = GRID_ROWS;
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (int i = 0; i < GRID_CELL_COUNT; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;

        gui_button_init(&a->cell_widgets[i]);
        a->cell_widgets[i].rect = gui_grid_cell_rect(&a->grid, col, row);
        a->cell_widgets[i].draw = draw_cell;
        a->cell_widgets[i].tag1 = i;
        a->cell_widgets[i].on_pointer_down = on_cell_pointer_down;
        a->cell_widgets[i].on_pointer_up = on_cell_pointer_up;
        a->cell_widgets[i].on_pointer_alt = on_cell_pointer_alt;

        gui_window_add_widget(&a->window, &a->cell_widgets[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Mines app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    restart_game();

    app_mines.main_window = &app_state->window;

    return E_OK;
}

global app_st app_mines = {
    .icon = &icon_mines,
    .init = init_app,
};

