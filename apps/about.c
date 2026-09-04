/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: about.c - System info app
 */

#include <gui.h>

enum {
    LINE_DISPLAY,
    LINE_CPU_USAGE,
    LINE_MEM_TOTAL,
    LINE_MEM_USED,
    LINE_MEM_AVAIL,
    LINE_COUNT,
};

enum {
    TOP_BAR_Y = TITLE_BAR_HEIGHT,
    TOP_BAR_HEIGHT = 36,

    GRID_CELL_WIDTH = 7,
    GRID_CELL_HEIGHT = 15,
    GRID_ROWS = LINE_COUNT,
    GRID_COLS = 27,
    GRID_CELLS_COUNT = (GRID_ROWS * GRID_COLS),
    GRID_BORDER = 1,
    GRID_WIDTH = GRID_WIDTH_SPACED(GRID_CELL_WIDTH, GRID_COLS, GRID_BORDER),
    GRID_HEIGHT = GRID_HEIGHT_SPACED(GRID_CELL_HEIGHT, GRID_ROWS, GRID_BORDER) + 7,
    GRID_X = 0,
    GRID_Y = TOP_BAR_Y + TOP_BAR_HEIGHT + 15,

    WINDOW_WIDTH = GRID_X + GRID_WIDTH,
    WINDOW_HEIGHT = GRID_Y + GRID_HEIGHT + 1,

    LABEL_COL = 2,
    VALUE_COL = 11,
    VALUE_LEN = GRID_COLS - VALUE_COL - 2,

    REFRESH_TICKS = TICK_FREQUENCY, /* 1s */
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    widget_st *widgets[2];

    grid_st grid;
} app_state_st;

static app_state_st *app_state = NULL;

static void
draw_text_sm(int col, int row, const char *text)
{
    app_state_st *a = app_state;

    if (row >= GRID_COLS) {
        return;
    }

    rect_st r = gui_grid_cell_rect(&a->grid, col, row);
    r.width = strlen(text) * 8;

    gui_surface_draw_str_at(a->window.surface, r.x, r.y, font_8x8,
        text, COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, r);
}

static void
draw_cpu_usage(void)
{
    static char buf[8];

    snprintf(buf, sizeof(buf), "%u%%   ", krn_timer_get_cpu_usage());
    draw_text_sm(VALUE_COL, LINE_CPU_USAGE, buf);
}

static void
draw_mem_usage(void)
{
    static char buf[VALUE_LEN + 1];

    snprintf(buf, sizeof(buf), "%u KB   ", krn_system_get_used_mem() >> 10);
    draw_text_sm(VALUE_COL, LINE_MEM_USED, buf);

    snprintf(buf, sizeof(buf), "%u KB   ", krn_system_get_avail_mem() >> 10);
    draw_text_sm(VALUE_COL, LINE_MEM_AVAIL, buf);
}

static void
draw_top_bar(void)
{
    app_state_st *a = app_state;

    const char *text = "-=[ GENTLE OS / 32 ]=-";
    rect_st r = gui_rect_make(0, TOP_BAR_Y, WINDOW_WIDTH, TOP_BAR_HEIGHT);

    gui_surface_draw_h_seg(a->window.surface, r.x, r.y + r.height, r.width, COLOR_BORDER);

    r = gui_rect_shrink(r, 1);

    gui_surface_draw_str_cc(a->window.surface, r, font_8x16, text,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);
}

static void
draw_info(void)
{
    app_state_st *a = app_state;

    system_info_st *si = &krn_system_info;
    rect_st r = gui_grid_rect(&a->grid);
    static char buf[VALUE_LEN + 1];

    snprintf(buf, sizeof(buf), "%dx%dx%d", si->fb_width, si->fb_height, 1 << si->fb_bpp);

    draw_text_sm(LABEL_COL, LINE_DISPLAY, "Display:");
    draw_text_sm(VALUE_COL, LINE_DISPLAY, buf);

    draw_text_sm(LABEL_COL, LINE_CPU_USAGE, "CPU:");
    draw_cpu_usage();

    snprintf(buf, sizeof(buf), "%u KB", krn_system_get_total_mem() >> 10);
    draw_text_sm(LABEL_COL, LINE_MEM_TOTAL, "Mem:");
    draw_text_sm(VALUE_COL, LINE_MEM_TOTAL, buf);

    draw_text_sm(LABEL_COL, LINE_MEM_USED, "Used:");
    draw_text_sm(LABEL_COL, LINE_MEM_AVAIL, "Avail:");
    draw_mem_usage();

    draw_top_bar();

    gui_wm_render_window_region(&a->window, r);
}

static void
on_tick(window_st *window)
{
    static unsigned count = 0;

    if (!window->visible) {
        return;
    }

    ++count;

    if (count < REFRESH_TICKS) {
        return;
    }

    count = 0;
    draw_cpu_usage();
    draw_mem_usage();
}

static void
draw_window(window_st *window)
{
    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_info();
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_about.main_window = NULL;

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
    a->window.title = "About";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_tick = on_tick;
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
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "About app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_window();
    init_grid();

    app_about.main_window = &app_state->window;

    return E_OK;
}

global app_st app_about = {
    .icon = &icon_about,
    .init = init_app,
};
