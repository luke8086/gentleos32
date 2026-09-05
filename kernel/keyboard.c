/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: keyboard.c - Driver for PS/2 keyboard
 */

#include <kernel.h>

static int
krn_keyboard_handle_virtual_mouse(uint8_t key_code, int is_key_down, int is_active)
{
    const int step = 10;
    static int btn_left = 0;
    static int btn_right = 0;
    int dx = 0;
    int dy = 0;
    int go;

    if (!is_active && (btn_left || btn_right)) {
        btn_left = 0;
        btn_right = 0;
        krn_mouse_handle_rel_packet(0, 0, 0, 0);
    }

    if (!is_active) {
        return 0;
    }

    go = 1;

    switch (key_code) {
    case KEY_LEFT: dx = -step * is_key_down; break;
    case KEY_RIGHT: dx = step * is_key_down; break;
    case KEY_UP: dy = step * is_key_down; break;
    case KEY_DOWN: dy = -step * is_key_down; break;
    case KEY_SPACE: btn_left = is_key_down; break;
    case KEY_R: btn_right = is_key_down; break;
    default: go = 0;
    }

    if (go) {
        krn_mouse_handle_rel_packet(dx, dy, btn_left, btn_right);
    }

    return go;
}

static void
krn_keyboard_handle_scancode(uint8_t scancode)
{
    static uint8_t lshift = 0;
    static uint8_t rshift = 0;
    static uint8_t ctrl = 0;
    static uint8_t alt = 0;
    static int last_scan_was_e0 = 0;

    event_st ev;
    int is_key_down = !(scancode & 0x80);
    int is_key_escaped = last_scan_was_e0;
    uint8_t shift;
    uint8_t *current_mod;

    if (scancode == 0xe0) {
        last_scan_was_e0 = 1;
        return;
    }

    if (is_key_down) {
        rand_add_entropy(krn_timer_get_counter_0());
    }

    last_scan_was_e0 = 0;

    ev.type = is_key_down ? EVENT_KEY_DOWN : EVENT_KEY_UP;
    ev.key_code = scancode & 0x7f;

    switch (ev.key_code) {
    case KEY_LSHIFT: current_mod = &lshift; break;
    case KEY_RSHIFT: current_mod = &rshift; break;
    case KEY_CTRL: current_mod = &ctrl; break;
    case KEY_ALT: current_mod = &alt; break;
    default: current_mod = 0;
    }

    /* Ignore duplicate key presses of modifiers */
    if (current_mod && *current_mod == is_key_down) {
        return;
    }

    if (current_mod) {
        *current_mod = is_key_down;
    }

    shift = lshift || rshift;

    ev.key_mods =
        (KEY_MOD_ESC * is_key_escaped) |
        (KEY_MOD_SHIFT * shift) |
        (KEY_MOD_CTRL * ctrl) |
        (KEY_MOD_ALT * alt);

    if (krn_system_info.debug_keyboard) {
        krn_debug_printf("Key %s: code=%02X mods=%02X\n",
            is_key_down ? "down" : "up", ev.key_code, ev.key_mods);
    }

    if (ev.key_code == KEY_DEL && ctrl && alt && is_key_down) {
        krn_ps2_reboot();
    }

    if (krn_keyboard_handle_virtual_mouse(ev.key_code, is_key_down, ctrl && shift)) {
        return;
    }

    (void)krn_event_ipush(ev);
}

static void
krn_keyboard_handle_intr(isr_stack_st *isr_stack __attribute__((unused)))
{
    uint8_t scan = krn_ps2_read_data_with_timeout(0) >> 8;

    krn_keyboard_handle_scancode(scan);

    outb(0x20, 0x20);
}

global void
krn_keyboard_init(void)
{
    krn_debug_printf("Initializing keyboard... ");

    krn_intr_set_handler(0x21, krn_keyboard_handle_intr);

    krn_debug_printf("ok\n");
}
