/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: gui.h - GUI API
 */

#ifndef _GUI_H_
#define _GUI_H_

#include <kernel.h>

typedef struct {
    int x;
    int y;
} point_st;

typedef struct {
    int width;
    int height;
} size_st;

typedef struct {
    union {
        point_st pos;
        struct {
            int x;
            int y;
        };
    };

    union {
        size_st size;
        struct {
            int width;
            int height;
        };
    };
} rect_st;

enum {
    FONT_COUNT = 3,
};

#define font_8x16 (&fonts[0])
#define font_8x8 (&fonts[1])
#define font_5x8 (&fonts[2])

typedef struct {
    size_st size;
    const char *name;
    const uint8_t *pixels;
} font_st;

typedef struct {
    size_st size;
    int bpp;
    int pitch;
    int foreground;
    int alpha;
    const uint8_t *pixels;
} bitmap_st;

typedef struct {
    size_st size;
    int pitch;
    uint8_t *pixels;
} surface_st;

struct window;
typedef struct window window_st;

struct widget;
typedef struct widget widget_st;

struct widget {
    window_st *window;
    rect_st rect;

    int tag1;
    int tag2;
    void *data;

    int active;
    int press_on_move_in;
    int press_sticky;
    int hidden;
    int hide_border;

    void (*draw)(widget_st *);
    void (*on_pointer_down)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_up)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_out)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_move)(widget_st *, event_st event, point_st pos);
    void (*on_pointer_alt)(widget_st *, event_st event, point_st pos);

    const char *label;
    font_st *font;
    bitmap_st *bitmap;
};

struct window {
    rect_st rect;
    surface_st *surface;

    int visible;
    int active;

    const char *title;

    widget_st **widgets;
    size_t widgets_count;
    size_t widgets_capacity;
    widget_st *pressed_widget;

    void (*render_region)(window_st *, rect_st reg);
    void (*draw)(window_st *);
    void (*on_pointer)(window_st *, event_st event);
    void (*on_key_down)(window_st *, event_st event);
    void (*on_key_up)(window_st *, event_st event);
    void (*on_tick)(window_st *);
    void (*on_active_change)(window_st *);
    void (*on_close)(window_st *);
};

typedef struct {
    int cell_width;
    int cell_height;
    int cols;
    int rows;
    int border;
    int x;
    int y;
} grid_st;

typedef struct {
    int col;
    int row;
} grid_pos_st;

struct list_widget;
typedef struct list_widget list_widget_st;

struct list_widget {
    widget_st widget;
    grid_st grid;

    int item_count;
    int page_size;
    int page_count;
    int cur_page;
    int cur_index;

    const char *(*get_label)(list_widget_st *list, int index);
    const char *(*get_right_label)(list_widget_st *list, int index);
    void (*on_select)(list_widget_st *list, int index);
};

struct progress_bar;
typedef struct progress_bar progress_bar_st;

struct progress_bar {
    widget_st widget;

    int max_value;
    int cur_value;

    int _last_fill_width;

    void (*on_pointer_down)(progress_bar_st *bar, int value);
};

typedef struct {
    bitmap_st *icon;
    window_st *main_window;
    int (*init)(void);
} app_st;

typedef struct {
    int index;

    /* base */
    uint8_t widget_bg;
    uint8_t widget_fg;
    uint8_t widget_sel_bg;
    uint8_t widget_sel_fg;
    uint8_t title_act_bg;
    uint8_t title_act_fg;
    uint8_t title_act_inner_border;
    uint8_t title_sel_bg;
    uint8_t title_sel_fg;
    uint8_t alert_fg;
    uint8_t border;
    uint8_t desktop;
    uint8_t desktop_alt;
    uint8_t card_front_bg;

    /* derived */
    uint8_t card_back_bg_1;
    uint8_t card_back_bg_2;
    uint8_t card_red_fg;
    uint8_t card_black_fg;
    uint8_t card_sel_bg;
    uint8_t card_sel_fg;
    uint8_t mj_face_bg;
    uint8_t mj_face_fg;
    uint8_t mj_face_sel_bg;
    uint8_t mj_face_sel_fg;
    uint8_t mj_edge;
    uint8_t snake_floor;
    uint8_t snake_wall;
    uint8_t snake_snake;
    uint8_t snake_fruit;
    uint8_t tetris_block;
    uint8_t piano_key_black;
    uint8_t piano_key_white;
    uint8_t piano_key_sel;
} gui_theme_st;

enum {
    GUI_THEME_DEFAULT = 0,
    GUI_THEME_MONO,
    GUI_THEME_NEON,
    GUI_THEME_COUNT
};

#define COLOR_WIDGET_BG         gui_theme.widget_bg
#define COLOR_WIDGET_FG         gui_theme.widget_fg
#define COLOR_WIDGET_SEL_BG     gui_theme.widget_sel_bg
#define COLOR_WIDGET_SEL_FG     gui_theme.widget_sel_fg
#define COLOR_TITLE_ACT_BG      gui_theme.title_act_bg
#define COLOR_TITLE_ACT_FG      gui_theme.title_act_fg
#define COLOR_TITLE_ACT_INNER_BORDER gui_theme.title_act_inner_border
#define COLOR_TITLE_SEL_BG      gui_theme.title_sel_bg
#define COLOR_TITLE_SEL_FG      gui_theme.title_sel_fg
#define COLOR_ALERT_FG          gui_theme.alert_fg
#define COLOR_BORDER            gui_theme.border
#define COLOR_DESKTOP           gui_theme.desktop
#define COLOR_DESKTOP_ALT       gui_theme.desktop_alt
#define COLOR_CARD_FRONT_BG     gui_theme.card_front_bg

#define COLOR_CARD_BACK_BG_1    gui_theme.card_back_bg_1
#define COLOR_CARD_BACK_BG_2    gui_theme.card_back_bg_2
#define COLOR_CARD_RED_FG       gui_theme.card_red_fg
#define COLOR_CARD_BLACK_FG     gui_theme.card_black_fg
#define COLOR_CARD_SEL_BG       gui_theme.card_sel_bg
#define COLOR_CARD_SEL_FG       gui_theme.card_sel_fg
#define COLOR_MJ_FACE_BG        gui_theme.mj_face_bg
#define COLOR_MJ_FACE_FG        gui_theme.mj_face_fg
#define COLOR_MJ_FACE_SEL_BG    gui_theme.mj_face_sel_bg
#define COLOR_MJ_FACE_SEL_FG    gui_theme.mj_face_sel_fg
#define COLOR_MJ_EDGE           gui_theme.mj_edge
#define COLOR_SNAKE_FLOOR       gui_theme.snake_floor
#define COLOR_SNAKE_WALL        gui_theme.snake_wall
#define COLOR_SNAKE_SNAKE       gui_theme.snake_snake
#define COLOR_SNAKE_FRUIT       gui_theme.snake_fruit
#define COLOR_TETRIS_BLOCK      gui_theme.tetris_block
#define COLOR_PIANO_KEY_BLACK   gui_theme.piano_key_black
#define COLOR_PIANO_KEY_WHITE   gui_theme.piano_key_white
#define COLOR_PIANO_KEY_SEL     gui_theme.piano_key_sel

enum {
    TITLE_BAR_HEIGHT = 24,
    PANEL_HEIGHT = 24,
    STATUS_HEIGHT = 24,
};

#define GRID_WIDTH_SPACED(cell_width, cols, border) \
    ((cell_width) * (cols) + (cols) - 1 + 2 * (border))

#define GRID_HEIGHT_SPACED(cell_height, rows, border) \
    ((cell_height) * (rows) + (rows) - 1 + 2 * (border))

#include "p_gui.h"
#include "p_build.h"
#include "p_apps.h"

#endif /* _GUI_H_ */
