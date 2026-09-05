/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: fb.c - Framebuffer routines
 */

#include <gui.h>

static surface_st _gui_fb_vram_surface = { 0 };
global surface_st *gui_fb_vram_surface = &_gui_fb_vram_surface;
static surface_st gui_fb_surface;
global rect_st gui_fb_screen_rect = { 0 };

enum {
    DIRTY_RECTS_MAX = 8,
};

static rect_st dirty_rects[DIRTY_RECTS_MAX];
static int dirty_rects_count = 0;

global void
gui_fb_draw_start(void)
{
}

global void
gui_fb_draw_end(void)
{
}

global void
gui_fb_mark_dirty(rect_st rect)
{
    int i, absorbed, growth, min_growth, min_growth_idx;

    rect = gui_rect_clip(rect, gui_fb_screen_rect);

    if (gui_rect_is_empty(rect)) {
        return;
    }

    while (1) {
        /* Keep absorbing existing slots that the rect touches */
        do {
            absorbed = 0;

            for (i = 0; i < dirty_rects_count; ++i) {
                if (gui_rect_touches(rect, dirty_rects[i])) {
                    rect = gui_rect_enclose(rect, dirty_rects[i]);
                    dirty_rects[i] = dirty_rects[--dirty_rects_count];
                    absorbed = 1;
                }
            }
        } while (absorbed);

        /* Break if there is an empty slot to use */
        if (dirty_rects_count < DIRTY_RECTS_MAX) {
            break;
        }

        /* Otherwise find slot that will cause minimal growth when absorbed */
        min_growth_idx = 0;
        min_growth = 0;
        for (i = 0; i < dirty_rects_count; ++i) {
            growth = gui_rect_area(gui_rect_enclose(dirty_rects[i], rect))
                - gui_rect_area(dirty_rects[i]);

            if (i == 0 || growth < min_growth) {
                min_growth_idx = i;
                min_growth = growth;
            }
        }

        /* Absorb it and repeat the process in case the new rect touches existing ones */
        rect = gui_rect_enclose(rect, dirty_rects[min_growth_idx]);
        dirty_rects[min_growth_idx] = dirty_rects[--dirty_rects_count];
    }

    dirty_rects[dirty_rects_count++] = rect;
}

global void
gui_fb_draw_rect(rect_st rect, uint8_t color)
{
    if (krn_system_info.fb_planar) {
        gui_planar_draw_rect(rect, color);
    } else {
        gui_surface_draw_rect(&gui_fb_surface, rect, color);
    }

    gui_fb_mark_dirty(rect);
}

global void
gui_fb_draw_pattern(rect_st rect, bitmap_st *pattern, uint8_t c1, uint8_t c2)
{
    if (krn_system_info.fb_planar) {
        gui_planar_draw_pattern_abs(rect, pattern, c1, c2);
    } else {
        gui_surface_draw_pattern_abs(&gui_fb_surface, rect, pattern, c1, c2);
    }

    gui_fb_mark_dirty(rect);
}

global void
gui_fb_draw_surface(int dst_x, int dst_y, surface_st *src_sf, rect_st src_rect)
{
    if (krn_system_info.fb_planar) {
        gui_planar_draw_surface(dst_x, dst_y, src_sf, src_rect);
    } else {
        gui_surface_copy(&gui_fb_surface, dst_x, dst_y, src_sf, src_rect);
    }

    gui_fb_mark_dirty(gui_rect_make(dst_x, dst_y, src_rect.width, src_rect.height));
}

global void
gui_fb_draw_image(rect_st rect, bitmap_st *bitmap)
{
    surface_st surface;

    surface.size = bitmap->size;
    surface.pitch = bitmap->pitch;
    surface.pixels = (uint8_t *)bitmap->pixels;

    gui_fb_draw_surface(rect.x, rect.y, &surface, rect);
}

global void
gui_fb_draw_outline(rect_st rect)
{
    if (krn_system_info.fb_planar) {
        gui_planar_xor_corners(rect);
    } else {
        gui_surface_draw_border(gui_fb_vram_surface, rect, COLOR_BORDER);
    }
}

global void
gui_fb_flush(void)
{
    rect_st rects[DIRTY_RECTS_MAX];
    int count = dirty_rects_count;

    if (count == 0) {
        return;
    }

    gui_drag_clear_outline();

    /* Reset the list before flushing - gui_status_set_alert() may re-enter */
    memcpy(rects, dirty_rects, count * sizeof(rects[0]));
    dirty_rects_count = 0;

    for (int i = 0; i < count; ++i) {
        if (krn_system_info.fb_planar) {
            gui_planar_flush(rects[i]);
        } else {
            gui_surface_copy(gui_fb_vram_surface, rects[i].x, rects[i].y,
                &gui_fb_surface, rects[i]);
        }
    }

    gui_pointer_draw();
    gui_drag_draw_outline();
}

global void
gui_fb_init(void)
{
    system_info_st *si = &krn_system_info;

    ASSERT(si->fb_width >= GUI_MIN_WIDTH && si->fb_height >= GUI_MIN_HEIGHT);

    gui_fb_screen_rect.width = si->fb_width;
    gui_fb_screen_rect.height = si->fb_height;

    gui_fb_vram_surface->size = gui_fb_screen_rect.size;
    gui_fb_vram_surface->pitch = si->fb_pitch;
    gui_fb_vram_surface->pixels = si->fb_addr;

    if (!krn_system_info.fb_planar) {
        gui_fb_surface.size = gui_fb_screen_rect.size;
        gui_fb_surface.pitch = si->fb_width;
        gui_fb_surface.pixels = heap_alloc(si->fb_width * si->fb_height,
            "linear pixels", 1);
    }
}
