/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: calc.c - Calculator app
 */

#include <gui.h>

enum {
    BUTTON_WIDTH = 36,
    BUTTON_HEIGHT = 36,
    BUTTON_COLS = 4,
    BUTTON_ROWS = 4,
    BUTTONS_COUNT = BUTTON_COLS * BUTTON_ROWS,

    DISPLAY_X = 0,
    DISPLAY_Y = TITLE_BAR_HEIGHT - 1,
    DISPLAY_HEIGHT = BUTTON_HEIGHT,

    GRID_BORDER = 1,
    GRID_X = 0,
    GRID_Y = DISPLAY_Y + DISPLAY_HEIGHT - 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(BUTTON_WIDTH, BUTTON_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(BUTTON_HEIGHT, BUTTON_ROWS, GRID_BORDER),

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT,

    DISPLAY_WIDTH = GRID_WIDTH,
};

static const char *button_labels[BUTTONS_COUNT] = {
    "7", "8", "9", "/",
    "4", "5", "6", "*",
    "1", "2", "3", "-",
    "0", "C", "=", "+"
};

typedef int32_t val_t;

#define VAL_MAX INT32_MAX
#define VAL_MIN INT32_MIN

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st button_widgets[BUTTONS_COUNT];
    widget_st *widgets[BUTTONS_COUNT + 2];

    grid_st grid;

    val_t current_val;
    val_t stored_val;
    val_t last_operand;
    uint8_t current_op;
    uint8_t last_op;
    int new_number;
    int error;
} app_state_st;

static app_state_st *app_state = NULL;

static void
exec_add(void)
{
    app_state_st *a = app_state;

    val_t result;

    if (__builtin_add_overflow(a->stored_val, a->current_val, &result)) {
        a->error = 1;
        return;
    }

    a->current_val = result;
}

static void
exec_sub(void)
{
    app_state_st *a = app_state;

    val_t result;

    if (__builtin_sub_overflow(a->stored_val, a->current_val, &result)) {
        a->error = 1;
        return;
    }

    a->current_val = result;
}

static void
exec_mul(void)
{
    app_state_st *a = app_state;

    val_t result;

    if (__builtin_mul_overflow(a->current_val, a->stored_val, &result)) {
        a->error = 1;
        return;
    }

    a->current_val = result;
}

static void
exec_div(void)
{
    app_state_st *a = app_state;

    if (a->current_val == 0 || (a->stored_val == VAL_MIN && a->current_val == -1)) {
        a->error = 1;
        return;
    }

    a->current_val = a->stored_val / a->current_val;
}

static void
exec_current_op(void)
{
    app_state_st *a = app_state;

    switch (a->current_op) {
    case '+': exec_add(); break;
    case '-': exec_sub(); break;
    case '*': exec_mul(); break;
    case '/': exec_div(); break;
    }

    a->current_op = 0;
    a->new_number = 1;
}

static void
update_display(void)
{
    app_state_st *a = app_state;

    static char buf[32];

    if (a->error) {
        snprintf(buf, sizeof(buf), "ERR");
    } else {
        snprintf(buf, sizeof(buf), "%d", a->current_val);
    }

    rect_st rect = gui_rect_make(DISPLAY_X, DISPLAY_Y, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    rect = gui_rect_shrink(rect, 1);

    gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);

    font_st *font = font_8x16;
    int text_width = strlen(buf) * font->size.width;
    int text_x = rect.x + rect.width - text_width - 10;
    int text_y = rect.y + (rect.height - font_8x16->size.height) / 2 + 1;
    gui_surface_draw_str_at(a->window.surface, text_x, text_y, font,
        buf, COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_BORDER);
    update_display();
}

static void
on_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    gui_button_on_pointer_up(widget, event, pos);

    uint8_t op = widget->label[0];

    if (a->error && op != 'C') {
        return;
    }

    if (op >= '0' && op <= '9') {
        int val = op - '0';
        if (a->new_number) {
            a->current_val = val;
            a->new_number = 0;
        } else {
            val_t new_val;
            if (__builtin_mul_overflow(a->current_val, 10, &new_val) ||
                __builtin_add_overflow(new_val, val, &new_val)) {
                return;
            }
            a->current_val = new_val;
        }
    } else if (op == 'C') {
        a->current_val = 0;
        a->stored_val = 0;
        a->current_op = 0;
        a->last_op = 0;
        a->last_operand = 0;
        a->new_number = 1;
        a->error = 0;
    } else if (op == '=') {
        if (a->current_op) {
            a->last_op = a->current_op;
            a->last_operand = a->current_val;
            exec_current_op();
        } else if (a->last_op) {
            a->stored_val = a->current_val;
            a->current_val = a->last_operand;
            a->current_op = a->last_op;
            exec_current_op();
        }
    } else if (op == '+' || op == '-' || op == '*' || op == '/') {
        if (a->current_op && !a->new_number) {
            exec_current_op();
        }
        a->stored_val = a->current_val;
        a->current_op = op;
        a->last_op = 0;
        a->new_number = 1;
    }

    update_display();
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_calc.main_window = NULL;

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
    a->window.title = "Calculator";
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

    a->grid.cell_width = BUTTON_WIDTH;
    a->grid.cell_height = BUTTON_HEIGHT;
    a->grid.cols = BUTTON_COLS;
    a->grid.rows = BUTTON_ROWS;
    a->grid.border = GRID_BORDER;
    a->grid.x = GRID_X;
    a->grid.y = GRID_Y;

    for (int row = 0; row < BUTTON_ROWS; ++row) {
        for (int col = 0; col < BUTTON_COLS; ++col) {
            int idx = row * BUTTON_COLS + col;
            widget_st *button = &a->button_widgets[idx];

            gui_button_init(button);
            button->rect = gui_grid_cell_rect(&a->grid, col, row);
            button->hide_border = 1;
            button->window = &a->window;
            button->label = button_labels[idx];
            button->font = font_8x16;
            button->on_pointer_up = on_button_press;

            gui_window_add_widget(&a->window, button);
        }
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Calc app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_buttons();

    app_state->new_number = 1;

    app_calc.main_window = &app_state->window;

    return E_OK;
}

global app_st app_calc = {
    .icon = &icon_calc,
    .init = init_app,
};
