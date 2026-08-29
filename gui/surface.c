/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: surface.c - Surface drawing routines
 */

#include <gui.h>

global void
gui_surface_copy(surface_st *dst_sf, int dst_x, int dst_y,
    surface_st *src_sf, rect_st src_rect)
{
    if (dst_x + src_rect.width > dst_sf->size.width) {
        src_rect.width = dst_sf->size.width - dst_x;
        src_rect.width = MAX(src_rect.width, 0);
    }

    if (dst_y + src_rect.height > dst_sf->size.height) {
        src_rect.height = dst_sf->size.height - dst_y;
        src_rect.height = MAX(src_rect.height, 0);
    }

    if (dst_y < 0 || dst_y + src_rect.height > dst_sf->size.height) {
        krn_debug_printf("err: dst_y: %d, src_rect.height: %d, dst_sf->size.height: %d\n",
            dst_y, src_rect.height, dst_sf->size.height);
        return;
    }

    if (dst_x < 0 || dst_x + src_rect.width > dst_sf->size.width) {
        krn_debug_printf("err: dst_x: %d, src_rect.width: %d, dst_sf->size.width: %d\n",
            dst_x, src_rect.width, dst_sf->size.width);
        return;
    }

    for (int i = 0; i < src_rect.height; ++i) {
        memcpy(
            dst_sf->pixels + (dst_y + i) * dst_sf->pitch + dst_x,
            src_sf->pixels + (src_rect.y + i) * src_sf->pitch + src_rect.x,
            src_rect.width
        );
    }
}

global void
gui_surface_draw_h_seg(surface_st *surface, int x, int y, int w, uint8_t color)
{
    memset(surface->pixels + y * surface->pitch + x, color, w);
}

global void
gui_surface_draw_v_seg(surface_st *surface, int x, int y, int h, uint8_t color)
{
    for (int i = 0; i < h; i++) {
        surface->pixels[(y + i) * surface->pitch + x] = color;
    }
}

global void
gui_surface_draw_border(surface_st *surface, rect_st r, uint8_t color)
{
    gui_surface_draw_h_seg(surface, r.x, r.y, r.width, color);
    gui_surface_draw_h_seg(surface, r.x, r.y + r.height - 1, r.width, color);
    gui_surface_draw_v_seg(surface, r.x, r.y, r.height, color);
    gui_surface_draw_v_seg(surface, r.x + r.width - 1, r.y, r.height, color);
}

global void
gui_surface_draw_rect(surface_st *surface, rect_st r, uint8_t color)
{
    for (int i = 0; i < r.height; i++) {
        gui_surface_draw_h_seg(surface, r.x, r.y + i, r.width, color);
    }
}

global void
gui_surface_draw_char(surface_st *surface, uint16_t x, uint16_t y,
    font_st *font, uint8_t ch, uint8_t fg, uint8_t bg)
{
    const uint8_t *glyph;
    uint8_t active;
    size_t npixel;
    int i, j;

    if (!ch) {
        ch = ' ';
    }

    glyph = font->pixels + (ch * font->size.height);

    for (j = 0; j < font->size.height; ++j) {
        for (i = 0; i < font->size.width; ++i) {
            active = (glyph[j] & (1 << (7 - i)));
            npixel = (y + j) * surface->pitch + (x + i);
            surface->pixels[npixel] = fg * !!active + bg * !active;
        }
    }
}

global void
gui_surface_draw_str(surface_st *surface, uint16_t x, uint16_t y,
    font_st *font, const char *s, uint8_t fg, uint8_t bg)
{
    for (int i = 0; s[i]; i++) {
        gui_surface_draw_char(surface, x + i * font->size.width, y, font, s[i], fg, bg);
    }
}

global void
gui_surface_draw_str_cc(surface_st *surface, rect_st rect,
    font_st *font, const char *s, uint8_t fg, uint8_t bg)
{
    int text_width = strlen(s) * font->size.width;

    int x = rect.x + (rect.width - text_width) / 2;
    int y = rect.y + (rect.height - font->size.height) / 2;

    if (rect.height > font->size.height) {
        ++y;
    }

    gui_surface_draw_str(surface, x, y, font, s, fg, bg);
}

global void
gui_surface_draw_str_cl(surface_st *surface, rect_st rect, int padding,
    font_st *font, const char *s, uint8_t fg, uint8_t bg)
{
    int x = rect.x + padding;
    int y = rect.y + (rect.height - font->size.height) / 2;

    gui_surface_draw_str(surface, x, y, font, s, fg, bg);
}

global void
gui_surface_draw_str_cr(surface_st *surface, rect_st rect, int padding,
    font_st *font, const char *s, uint8_t fg, uint8_t bg)
{
    int x = rect.x + rect.width - padding - strlen(s) * font->size.width;
    int y = rect.y + (rect.height - font->size.height) / 2;

    gui_surface_draw_str(surface, x, y, font, s, fg, bg);
}

static void
gui_surface_draw_bitmap_8bpp(surface_st *surface, rect_st src_rect, int dst_x, int dst_y,
    bitmap_st *bitmap)
{
    uint8_t alpha = (uint8_t)bitmap->alpha;

    for (uint16_t i = 0; i < src_rect.height; i++) {
        for (uint16_t j = 0; j < src_rect.width; j++) {
            if (bitmap->pixels[i * bitmap->pitch + j] == alpha) {
                continue;
            }

            size_t src_pixel_no = i * bitmap->pitch + j;
            size_t dst_pixel_no = (dst_x + j) + (dst_y + i) * surface->pitch;

            uint8_t pixel = bitmap->pixels[src_pixel_no];
            surface->pixels[dst_pixel_no] = pixel;
        }
    }
}

global void
gui_surface_draw_bitmap_1bpp(surface_st *surface, rect_st src_rect, int dst_x, int dst_y,
    bitmap_st *bitmap, uint8_t fill)
{
    for (uint16_t i = 0; i < src_rect.height; i++) {
        for (uint16_t j = 0; j < src_rect.width; j++) {
            int byte_no = i * bitmap->pitch + j / 8;
            int bit_no = 7 - (j % 8);
            int active = (bitmap->pixels[byte_no] >> bit_no) & 1;

            if (!active) {
                continue;
            }

            size_t dst_pixel_no = (dst_x + j) + (dst_y + i) * surface->pitch;
            surface->pixels[dst_pixel_no] = fill;
        }
    }
}

global void gui_surface_draw_bitmap(surface_st *surface, int dst_x, int dst_y, bitmap_st *bitmap,
    uint8_t fill)
{
    rect_st src_rect = {
        .x = 0,
        .y = 0,
        .size = bitmap->size,
    };

    if (dst_x + src_rect.width > surface->size.width) {
        src_rect.width = surface->size.width - dst_x;
        src_rect.width = src_rect.width < 0 ? 0 : src_rect.width;
    }

    if (dst_y + src_rect.height > surface->size.height) {
        src_rect.height = surface->size.height - dst_y;
        src_rect.height = src_rect.height < 0 ? 0 : src_rect.height;
    }

    if (bitmap->bpp == 1) {
        gui_surface_draw_bitmap_1bpp(surface, src_rect, dst_x, dst_y, bitmap, fill);
    } else if (bitmap->bpp == 8) {
        gui_surface_draw_bitmap_8bpp(surface, src_rect, dst_x, dst_y, bitmap);
    }
}

global void
gui_surface_draw_bitmap_centered(surface_st *surface, rect_st rect, bitmap_st *bitmap,
    uint8_t fill)
{
    int x = rect.x + (rect.width - bitmap->size.width) / 2;
    int y = rect.y + (rect.height - bitmap->size.height) / 2;

    gui_surface_draw_bitmap(surface, x, y, bitmap, fill);
}

/* Draw pattern in specified region relative to surface origin */
global void
gui_surface_draw_pattern_abs(surface_st *surface, rect_st reg,
    bitmap_st *b, uint8_t col1, uint8_t col2)
{
    for (uint16_t y = reg.y; y < reg.y + reg.height; y++) {
        for (uint16_t x = reg.x; x < reg.x + reg.width; x++) {
            int tile_x = x % b->size.width;
            int tile_y = y % b->size.height;
            int byte_no = tile_y * b->pitch + tile_x / 8;
            int bit_no = 7 - (tile_x % 8);
            int src_bit = (b->pixels[byte_no] >> bit_no) & 1;
            size_t dst_pixel_no = y * surface->pitch + x;

            surface->pixels[dst_pixel_no] = src_bit ? col1 : col2;
        }
    }
}

/* Draw pattern in specified region relative to this region */
global void
gui_surface_draw_pattern_rel(surface_st *surface, rect_st reg,
    bitmap_st *b, uint8_t col1, uint8_t col2)
{
    for (int dy = 0; dy < reg.height; dy++) {
        for (int dx = 0; dx < reg.width; dx++) {
            int tile_x = dx % b->size.width;
            int tile_y = dy % b->size.height;
            int byte_no = tile_y * b->pitch + tile_x / 8;
            int bit_no = 7 - (tile_x % 8);
            int src_bit = (b->pixels[byte_no] >> bit_no) & 1;
            size_t dst_pixel_no = (reg.y + dy) * surface->pitch + (reg.x + dx);

            surface->pixels[dst_pixel_no] = src_bit ? col1 : col2;
        }
    }
}
