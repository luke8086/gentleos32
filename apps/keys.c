/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: keys.c - Keyboard map
 */

#include <gui.h>

typedef struct {
    uint16_t code;
    uint8_t width;
    const char *label;
    int x;
    int y;
    int pressed;
} cell_st;

enum {
    CELL_H = 28,
    CELL_W = 28,

    KEYS_X = 0,
    KEYS_Y = TITLE_BAR_HEIGHT - 1,
    KEYS_W = 525,
    KEYS_H = 169,

    WINDOW_WIDTH = KEYS_X + KEYS_W,
    WINDOW_HEIGHT = KEYS_Y + KEYS_H,
};

static cell_st cells[] = {
    { .code = KEY_ESC,      .width = CELL_W + 3,    .label = "Esc" },
    { .code = KEY_F1,       .width = CELL_W + 5,    .label = "F1" },
    { .code = KEY_F2,       .width = CELL_W + 5,    .label = "F2" },
    { .code = KEY_F3,       .width = CELL_W + 5,    .label = "F3" },
    { .code = KEY_F4,       .width = CELL_W + 5,    .label = "F4" },
    { .code = KEY_F5,       .width = CELL_W + 5,    .label = "F5" },
    { .code = KEY_F6,       .width = CELL_W + 5,    .label = "F6" },
    { .code = KEY_F7,       .width = CELL_W + 5,    .label = "F7" },
    { .code = KEY_F8,       .width = CELL_W + 5,    .label = "F8" },
    { .code = KEY_F9,       .width = CELL_W + 5,    .label = "F9" },
    { .code = KEY_F10,      .width = CELL_W + 5,    .label = "F10" },
    { .code = KEY_F11,      .width = CELL_W + 5,    .label = "F11" },
    { .code = KEY_F12,      .width = CELL_W + 5,    .label = "F12" },
    { .code = KEY_BKTICK,   .width = CELL_W,        .label = "`" },
    { .code = KEY_1,        .width = CELL_W,        .label = "1" },
    { .code = KEY_2,        .width = CELL_W,        .label = "2" },
    { .code = KEY_3,        .width = CELL_W,        .label = "3" },
    { .code = KEY_4,        .width = CELL_W,        .label = "4" },
    { .code = KEY_5,        .width = CELL_W,        .label = "5" },
    { .code = KEY_6,        .width = CELL_W,        .label = "6" },
    { .code = KEY_7,        .width = CELL_W,        .label = "7" },
    { .code = KEY_8,        .width = CELL_W,        .label = "8" },
    { .code = KEY_9,        .width = CELL_W,        .label = "9" },
    { .code = KEY_0,        .width = CELL_W,        .label = "0" },
    { .code = KEY_MINUS,    .width = CELL_W,        .label = "-" },
    { .code = KEY_EQUAL,    .width = CELL_W,        .label = "=" },
    { .code = KEY_BKSP,     .width = 63,            .label = "Bksp" },
    { .code = KEY_TAB,      .width = 43,            .label = "Tab" },
    { .code = KEY_Q,        .width = CELL_W,        .label = "Q" },
    { .code = KEY_W,        .width = CELL_W,        .label = "W" },
    { .code = KEY_E,        .width = CELL_W,        .label = "E" },
    { .code = KEY_R,        .width = CELL_W,        .label = "R" },
    { .code = KEY_T,        .width = CELL_W,        .label = "T" },
    { .code = KEY_Y,        .width = CELL_W,        .label = "Y" },
    { .code = KEY_U,        .width = CELL_W,        .label = "U" },
    { .code = KEY_I,        .width = CELL_W,        .label = "I" },
    { .code = KEY_O,        .width = CELL_W,        .label = "O" },
    { .code = KEY_P,        .width = CELL_W,        .label = "P" },
    { .code = KEY_LBRCKT,   .width = CELL_W,        .label = "[" },
    { .code = KEY_RBRCKT,   .width = CELL_W,        .label = "]" },
    { .code = KEY_BKSLASH,  .width = 48,            .label = "\\" },
    { .code = KEY_CAPS,     .width = 50,            .label = "Caps" },
    { .code = KEY_A,        .width = CELL_W,        .label = "A" },
    { .code = KEY_S,        .width = CELL_W,        .label = "S" },
    { .code = KEY_D,        .width = CELL_W,        .label = "D" },
    { .code = KEY_F,        .width = CELL_W,        .label = "F" },
    { .code = KEY_G,        .width = CELL_W,        .label = "G" },
    { .code = KEY_H,        .width = CELL_W,        .label = "H" },
    { .code = KEY_J,        .width = CELL_W,        .label = "J" },
    { .code = KEY_K,        .width = CELL_W,        .label = "K" },
    { .code = KEY_L,        .width = CELL_W,        .label = "L" },
    { .code = KEY_SEMICOL,  .width = CELL_W,        .label = ";" },
    { .code = KEY_QUOTE,    .width = CELL_W,        .label = "'" },
    { .code = KEY_ENTER,    .width = 69,            .label = "Enter" },
    { .code = KEY_LSHIFT,   .width = 65,            .label = "Shift" },
    { .code = KEY_Z,        .width = CELL_W,        .label = "Z" },
    { .code = KEY_X,        .width = CELL_W,        .label = "X" },
    { .code = KEY_C,        .width = CELL_W,        .label = "C" },
    { .code = KEY_V,        .width = CELL_W,        .label = "V" },
    { .code = KEY_B,        .width = CELL_W,        .label = "B" },
    { .code = KEY_N,        .width = CELL_W,        .label = "N" },
    { .code = KEY_M,        .width = CELL_W,        .label = "M" },
    { .code = KEY_COMMA,    .width = CELL_W,        .label = "," },
    { .code = KEY_PERIOD,   .width = CELL_W,        .label = "." },
    { .code = KEY_SLASH,    .width = CELL_W,        .label = "/" },
    { .code = KEY_RSHIFT,   .width = 82,            .label = "Shift" },
    { .code = KEY_CTRL,     .width = 47,            .label = "Ctrl" },
    { .code = KEY_ALT,      .width = 49,            .label = "Alt" },
    { .code = KEY_SPACE,    .width = 235,           .label = "Space" },
    { .code = KEY_RALT,     .width = 49,            .label = "Alt" },
    { .code = KEY_RCTRL,    .width = 47,            .label = "Ctrl" },
    { .code = KEY_INS,      .width = CELL_W,        .label = "Ins" },
    { .code = KEY_HOME,     .width = CELL_W,        .label = "Hom" },
    { .code = KEY_PGUP,     .width = CELL_W,        .label = "PgU" },
    { .code = KEY_DEL,      .width = CELL_W,        .label = "Del" },
    { .code = KEY_END,      .width = CELL_W,        .label = "End" },
    { .code = KEY_PGDN,     .width = CELL_W,        .label = "PgD" },
    { .code = KEY_UP,       .width = CELL_W,        .label = "\x18" },
    { .code = KEY_LEFT,     .width = CELL_W,        .label = "\x1b" },
    { .code = KEY_DOWN,     .width = CELL_W,        .label = "\x19" },
    { .code = KEY_RIGHT,    .width = CELL_W,        .label = "\x1a" },
};

static int cells_initialized = 0;

#define CELL_COUNT (sizeof(cells) / sizeof(cells[0]))

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    int last_key_code;
} app_state_st;

static app_state_st *app_state = NULL;

static void
init_cells(void)
{
    size_t i;

    if (cells_initialized) {
        return;
    }

    for (i = 0; i < (int)CELL_COUNT; ++i) {
        cells[i].x = 0;
        cells[i].y = 0;
        cells[i].pressed = 0;
    }

    cells[13].y = CELL_H * 1;
    cells[27].y = CELL_H * 2;
    cells[41].y = CELL_H * 3;
    cells[54].y = CELL_H * 4;
    cells[66].y = CELL_H * 5;
    cells[71].x = 440;
    cells[71].y = CELL_H * 1;
    cells[74].x = 440;
    cells[74].y = CELL_H * 2;
    cells[77].x = 440 + CELL_W;
    cells[77].y = CELL_H * 4;
    cells[78].x = 440;
    cells[78].y = CELL_H * 5;

    for (i = 1; i < (int)CELL_COUNT; ++i) {
        if (!cells[i].x && !cells[i].y) {
            cells[i].x = cells[i - 1].x + cells[i - 1].width;
            cells[i].y = cells[i - 1].y;
        }
    }

    cells_initialized = 1;
}

static void
draw_cell(cell_st *key, int pressed)
{
    app_state_st *a = app_state;

    rect_st rect;
    uint8_t fg = pressed ? COLOR_WIDGET_SEL_FG : COLOR_WIDGET_FG;
    uint8_t bg = pressed ? COLOR_WIDGET_SEL_BG : COLOR_WIDGET_BG;

    rect = gui_rect_make(KEYS_X + key->x, KEYS_Y + key->y, key->width + 1, CELL_H + 1);

    gui_surface_draw_rect(a->window.surface, rect, bg);
    gui_surface_draw_border(a->window.surface, rect, COLOR_BORDER);
    gui_surface_draw_str_cc(a->window.surface, rect, font_8x8, key->label, fg, bg);

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_keyboard(void)
{
    int i;

    for (i = 0; i < (int)CELL_COUNT; ++i) {
        draw_cell(&cells[i], 0);
    }
}

static cell_st *
find_key(uint16_t code)
{
    int i;

    for (i = 0; i < (int)CELL_COUNT; ++i) {
        if (cells[i].code == code) {
            return &cells[i];
        }
    }

    return NULL;
}

static void
update_cell(uint16_t code, int escaped, int pressed)
{
    cell_st *key = NULL;

    if (escaped) {
        key = find_key(code | 0xe000);
    }

    if (!key) {
        key = find_key(code);
    }

    if (key && key->pressed != pressed) {
        draw_cell(key, pressed);
        key->pressed = pressed;
    }
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_keyboard();
}

static void
on_key_down(window_st *w _unsd, event_st event)
{
    app_state_st *a = app_state;

    int escaped = !!(event.key_mods & KEY_MOD_ESC);
    char key_char = key_char_for_code(event.key_code, event.key_mods);

    update_cell(event.key_code, escaped, 1);

    if (event.key_code != a->last_key_code) {
        gui_status_set("Last key:%02X  Mods:%02X  Char:%02X (%c)",
            event.key_code, event.key_mods, key_char, key_char ? key_char : ' ');
    }
}

static void
on_key_up(window_st *w _unsd, event_st event)
{
    int escaped = !!(event.key_mods & KEY_MOD_ESC);

    update_cell(event.key_code, escaped, 0);
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_keys.main_window = NULL;

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
    a->window.title = "Keys";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_key_down = on_key_down;
    a->window.on_key_up = on_key_up;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Keys app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_cells();
    init_window();

    app_keys.main_window = &app_state->window;

    return E_OK;
}

global app_st app_keys = {
    .icon = &icon_keys,
    .init = init_app,
};
