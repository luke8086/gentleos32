/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: status.c - Status bar routines
 */

#include <gui.h>

enum {
    FONT_WIDTH = 8,
    FONT_HEIGHT = 16,

    TEXT_X = FONT_WIDTH,
    TEXT_Y = (STATUS_HEIGHT - FONT_HEIGHT) / 2 + 1,

    STATUS_TEXT_BUF_SIZE = ((GUI_MIN_WIDTH - PANEL_WIDTH) / FONT_WIDTH) - 2 + 1,
};

static surface_st window_surface;
static window_st window;

static char status_text_tmp[STATUS_TEXT_BUF_SIZE];
static char status_text[STATUS_TEXT_BUF_SIZE];

static uint8_t status_text_color = 0;
static size_t status_text_len = 0;
static uint8_t status_bg_color = 0;

static void
gui_status_set_bg_color(uint8_t color)
{
    if (color == status_bg_color) {
        return;
    }

    rect_st bg_rect = {
        .x = 0,
        .y = 1,
        .width = window.rect.width,
        .height = STATUS_HEIGHT - 1,
    };

    gui_surface_draw_rect(window.surface, bg_rect, color);
    gui_wm_render_window_region(&window, bg_rect);

    status_bg_color = color;
}

static void
gui_status_set_text(const char *text, uint8_t color)
{
    size_t len = strlen(text);
    font_st *font = font_8x16;

    strncpy(status_text, text, sizeof(status_text) - 1);
    status_text[sizeof(status_text) - 1] = 0;
    status_text_color = color;

    gui_surface_draw_str_at(window.surface, TEXT_X, TEXT_Y, font, text, color,
        status_bg_color);

    /* If the new text is shorter than previous, clear the remaining space */
    if (len < status_text_len) {
        rect_st clear_rect = {
            .x = TEXT_X + len * font->size.width,
            .y = TEXT_Y,
            .width = (status_text_len - len) * font->size.width,
            .height = font->size.height,
        };

        gui_surface_draw_rect(window.surface, clear_rect, status_bg_color);
    }

    rect_st text_rect = {
        .x = TEXT_X,
        .y = TEXT_Y,
        .width = window.rect.width - TEXT_X * 2,
        .height = font->size.height,
    };

    gui_wm_render_window_region(&window, text_rect);

    status_text_len = len;
}

global void
gui_status_set(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void) vsnprintf(status_text_tmp, sizeof(status_text_tmp), fmt, args);
    va_end(args);

    gui_status_set_bg_color(COLOR_WIDGET_BG);
    gui_status_set_text(status_text_tmp, COLOR_WIDGET_FG);
}

global void
gui_status_set_error(int err)
{
    const char *msg = error_message_for(err);

    if (msg) {
        gui_status_set("Error: %s", msg);
    }
}

global void
gui_status_set_alert(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void) vsnprintf(status_text_tmp, sizeof(status_text_tmp), fmt, args);
    va_end(args);

    gui_status_set_bg_color(COLOR_WIDGET_BG);
    gui_status_set_text(status_text_tmp, COLOR_ALERT_FG);

    /* Flush immediately on alerts */
    gui_fb_flush();
}

static void
draw_window(window_st *window)
{
    gui_surface_draw_h_seg(window->surface, 0, 0, window->rect.width, COLOR_BORDER);

    rect_st bg = { .x = 0, .y = 1, .width = window->rect.width, .height = STATUS_HEIGHT - 1 };
    status_bg_color = COLOR_WIDGET_BG;
    gui_surface_draw_rect(window->surface, bg, status_bg_color);

    status_text_len = 0;
    gui_status_set_text(status_text, status_text_color);
}

global void
gui_status_init(void)
{
    system_info_st *si = &krn_system_info;
    int width = si->fb_width - PANEL_WIDTH;

    window_surface.size.width = width;
    window_surface.size.height = STATUS_HEIGHT;
    window_surface.pitch = width;
    window_surface.pixels = heap_alloc(width * STATUS_HEIGHT, "Status pixels", 1);

    window.rect.x = 0;
    window.rect.y = si->fb_height - STATUS_HEIGHT;
    window.rect.width = width;
    window.rect.height = STATUS_HEIGHT;
    window.surface = &window_surface;
    window.visible = 1;
    window.draw = draw_window;

    gui_wm_set_status_window(&window);
}
