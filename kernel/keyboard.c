/*
 * Copyright (c) 2014-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: keyboard.c - Driver for PS/2 keyboard
 */

#include <kernel.h>

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

    ev.key_mods =
        (KEY_MOD_ESC * is_key_escaped) |
        (KEY_MOD_SHIFT * lshift) |
        (KEY_MOD_SHIFT * rshift) |
        (KEY_MOD_CTRL * ctrl) |
        (KEY_MOD_ALT * alt);

    if (krn_system_info.debug_keyboard) {
        krn_debug_printf("Key %s: code=%02X mods=%02X\n",
            is_key_down ? "down" : "up", ev.key_code, ev.key_mods);
    }

    if (ev.key_code == KEY_DEL && ctrl && alt && is_key_down) {
        krn_ps2_reboot();
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
