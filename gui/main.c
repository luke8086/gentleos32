/*
 * Copyright (c) 2025-2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: gui.c - GUI main function
 */

#include <gui.h>

global app_st *gui_apps[] = {
    &app_about,
    &app_clock,
    &app_calendar,
    &app_calc,
    &app_player,
    &app_sounds,
    &app_keys,
    &app_colors,
    &app_fonts,
    &app_settings,
    &app_snake,
    &app_mines,
    &app_tetris,
    &app_pairs,
    &app_mahjong,
    &app_freecell,
    &app_klondike,
    &app_blackjack,
};

global unsigned gui_apps_count = (sizeof(gui_apps) / sizeof(gui_apps[0]));

global bitmap_st *
gui_load_bitmap(const char *name)
{
    file_st *file = file_lookup(name);
    bitmap_st *bitmap;

    if (!file) {
        return NULL;
    }

    bitmap = (bitmap_st *)file->addr;
    bitmap->pixels = (uint8_t *)((uint32_t)file->addr + sizeof(bitmap_st));

    return bitmap;
}

static int
gui_do_launch_app(app_st *app)
{
    int err;

    if (app->main_window) {
        return gui_wm_add_window(app->main_window);
    }

    if (gui_wm_free_slots() == 0) {
        return E_TOO_MANY_WINDOWS;
    }

    err = app->init();

    if (err != E_OK) {
        return err;
    }

    ASSERT(app->main_window != 0);

    return gui_wm_add_window(app->main_window);
}

global void
gui_launch_app(app_st *app)
{
    int err;

    err = gui_do_launch_app(app);

    if (err != E_OK) {
        gui_status_set_error(err);
    }
}

global int
gui_handle_key(event_st event)
{
    int is_special = (event.key_mods & KEY_MOD_CTRL) && (event.key_mods & KEY_MOD_SHIFT);
    int key_code = event.key_code;
    int ok;

    if (!is_special) {
        return 0;
    }

    if (key_code == KEY_M) {
        heap_dump();
        return 1;
    }

    if (key_code == KEY_D) {
        gui_launch_app(&app_logview);
        return 1;
    }

    if (key_code == KEY_S) {
        ok = krn_uart_set_mode(UART_MODE_MOUSE);
        gui_status_set(ok ? "Serial mouse activated" : "Serial port unavailable");
        return 1;
    }

    if (key_code == KEY_A) {
        gui_launch_app(&app_launcher);
        return 1;
    }

    return 0;
}

global void
gui_main(void)
{
    event_st event;
    window_st *pressed_window = NULL;

    gui_fb_init();
    gui_planar_init();
    gui_pointer_init();
    gui_theme_set(GUI_THEME_DEFAULT);
    gui_wm_init();
    gui_fb_flush();

    krn_debug_status_cb = gui_status_set_alert;

#ifdef DEV_AUTOSTART
    gui_launch_app(&DEV_AUTOSTART);
#endif

    while (1) {
        if (krn_event_count() == 0) {
            krn_timer_is_cpu_idle = 1;
            cpu_hlt();
            krn_timer_is_cpu_idle = 0;
            continue;
        }

        if (krn_event_pop(&event) != 0) {
            continue;
        }

        if (event.type == EVENT_TIMER_TICK) {
            gui_wm_on_tick();
        } else if (event.type == EVENT_POINTER_DOWN) {
            gui_pointer_move(event.pointer_x, event.pointer_y);

            window_st *w = gui_wm_find_window(event.pointer_x, event.pointer_y);

            if (w) {
                pressed_window = w;
                gui_wm_raise_window(w);
                gui_window_on_pointer_down(w, event);
            }
        } else if (event.type == EVENT_POINTER_MOVE) {
            gui_pointer_move(event.pointer_x, event.pointer_y);

            if (pressed_window) {
                gui_window_on_pointer_move(pressed_window, event);
            }
        } else if (event.type == EVENT_POINTER_UP) {
            gui_pointer_move(event.pointer_x, event.pointer_y);

            if (pressed_window) {
                gui_window_on_pointer_up(pressed_window, event);
            }

            pressed_window = NULL;
        } else if (event.type == EVENT_POINTER_ALT) {
            gui_pointer_move(event.pointer_x, event.pointer_y);

            window_st *w = gui_wm_find_window(event.pointer_x, event.pointer_y);

            if (w && !pressed_window) {
                gui_window_on_pointer_alt(w, event);
            }
        } else if (event.type == EVENT_KEY_DOWN) {
            window_st *w = gui_wm_top_window();

            if (!gui_handle_key(event) && w && w->on_key_down) {
                w->on_key_down(w, event);
            }
        } else if (event.type == EVENT_KEY_UP) {
            window_st *w = gui_wm_top_window();

            if (w && w->on_key_up) {
                w->on_key_up(w, event);
            }
        }

        if (krn_event_count() == 0) {
            gui_fb_flush();
        }
    }
}
