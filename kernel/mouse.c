/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: mouse.c - Driver for PS/2 and MS serial mouse
 */

#include <kernel.h>

static struct {
    int16_t x;
    int16_t y;
    uint16_t btn_left;
    uint16_t btn_right;
} mouse_state = { 0 };

global void
krn_mouse_handle_abs_packet(int x, int y, int btn_left, int btn_right)
{
    system_info_st *si = &krn_system_info;
    int skip_event = 0;

    x = MAX(MIN(x, si->fb_width - 1), 0);
    y = MAX(MIN(y, si->fb_height - 1), 0);

    event_st event = {
        .type = EVENT_POINTER_MOVE,
        .pointer_x = x,
        .pointer_y = y,
    };

    if (btn_left && !mouse_state.btn_left) {
        event.type = EVENT_POINTER_DOWN;
        rand_add_entropy(krn_timer_get_counter_0());
    } else if (!btn_left && mouse_state.btn_left) {
        event.type = EVENT_POINTER_UP;
    } else if (btn_right && !mouse_state.btn_right) {
        event.type = EVENT_POINTER_ALT;
    }

    if (event.type == EVENT_POINTER_MOVE && x == mouse_state.x && y == mouse_state.y) {
        skip_event = 1;
    }

    mouse_state.x = x;
    mouse_state.y = y;
    mouse_state.btn_left = btn_left;
    mouse_state.btn_right = btn_right;

    if (skip_event) {
        return;
    }

    (void)krn_event_ipush(event);
}

global void
krn_mouse_handle_rel_packet(int dx, int dy, int btn_left, int btn_right)
{
    krn_mouse_handle_abs_packet(mouse_state.x + dx, mouse_state.y - dy, btn_left, btn_right);
}

global void
krn_mouse_handle_uart_data(uint8_t data)
{
    static uint8_t cycle = 0;
    static uint8_t p[3];

    /* Synchronize incoming data */
    if (data & 0x40) {
        cycle = 0;
    } else if (cycle == 0) {
        return;
    }

    p[cycle++] = data;

    if (cycle < 3) {
        return;
    }

    cycle = 0;

    int32_t dx = (int8_t)(((p[0] & 0x03) << 6) | (p[1] & 0x3F));
    int32_t dy = -(int8_t)(((p[0] & 0x0C) << 4) | (p[2] & 0x3F));
    uint8_t btn_left = p[0] & 0x20 ? 1 : 0;
    uint8_t btn_right = p[0] & 0x10 ? 1 : 0;

    krn_mouse_handle_rel_packet(dx, dy, btn_left, btn_right);
}

global void
krn_mouse_handle_ps2_data(uint8_t data)
{
    static uint8_t cycle = 0;
    static uint8_t p[3];
    int dx, dy, btn_left, btn_right;

    /* Synchronize incoming data */
    if (cycle == 0 && (data & 8) == 0) {
        return;
    }

    p[cycle++] = data;

    if (cycle < 3) {
        return;
    }

    cycle = 0;

    /* Ignore packet if overflow bits are set */
    if (p[0] & 0xc0) {
        return;
    }

    dx = (int32_t)p[1] - ((p[0] & 0x10) ? 256 : 0);
    dy = (int32_t)p[2] - ((p[0] & 0x20) ? 256 : 0);
    btn_left = p[0] & 0x01 ? 1 : 0;
    btn_right = p[0] & 0x02 ? 1 : 0;

    krn_mouse_handle_rel_packet(dx, dy, btn_left, btn_right);
}

global void
krn_mouse_init(void)
{
    system_info_st *si = &krn_system_info;

    mouse_state.x = si->fb_width / 2;
    mouse_state.y = si->fb_height / 2;
}
