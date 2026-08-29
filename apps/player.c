/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: player.c - Music player app
 */

#include <gui.h>

enum {
    PADDING = 8,

    TRANSPORT_BUTTON_COUNT = 7,
    TRANSPORT_BUTTON_WIDTH = 25,
    TRANSPORT_BUTTON_HEIGHT = 23,
    TRANSPORT_BUTTON_GAP = 4,

    CONTENT_X = PADDING,
    CONTENT_Y = TITLE_BAR_HEIGHT + PADDING - 1,
    CONTENT_WIDTH = TRANSPORT_BUTTON_COUNT * TRANSPORT_BUTTON_WIDTH +
        (TRANSPORT_BUTTON_COUNT - 1) * TRANSPORT_BUTTON_GAP,

    TITLE_Y = CONTENT_Y,
    TITLE_HEIGHT = 8,

    PROGRESS_Y = TITLE_Y + TITLE_HEIGHT + PADDING,
    PROGRESS_HEIGHT = 12,

    TIME_Y = PROGRESS_Y + PROGRESS_HEIGHT + 6,
    TIME_HEIGHT = 8,

    TRANSPORT_BUTTON_Y = TIME_Y + TIME_HEIGHT + PADDING,

    PLAY_LIST_COLS = 1,
    PLAY_LIST_ROWS = 10,
    PLAY_LIST_BORDER = 1,
    PLAY_LIST_CELL_WIDTH = CONTENT_WIDTH - 2 * PLAY_LIST_BORDER,
    PLAY_LIST_CELL_HEIGHT = 16,
    PLAY_LIST_X = CONTENT_X,
    PLAY_LIST_Y = TRANSPORT_BUTTON_Y + TRANSPORT_BUTTON_HEIGHT + PADDING,
    PLAY_LIST_HEIGHT = GRID_HEIGHT_SPACED(PLAY_LIST_CELL_HEIGHT, PLAY_LIST_ROWS, PLAY_LIST_BORDER),

    PAGE_BUTTON_WIDTH = 15,
    PAGE_BUTTON_HEIGHT = 15,
    PAGE_BUTTON_GAP = 4,
    PAGE_BUTTON_Y = PLAY_LIST_Y + PLAY_LIST_HEIGHT + PADDING,
    PAGE_BUTTON_PREV_X = CONTENT_X + (CONTENT_WIDTH - 2 * PAGE_BUTTON_WIDTH - PAGE_BUTTON_GAP) / 2,
    PAGE_BUTTON_NEXT_X = PAGE_BUTTON_PREV_X + PAGE_BUTTON_WIDTH + PAGE_BUTTON_GAP,

    WINDOW_WIDTH = CONTENT_X + CONTENT_WIDTH + PADDING,
    WINDOW_HEIGHT = PAGE_BUTTON_Y + PAGE_BUTTON_HEIGHT + PADDING,

    SONG_MAX_COUNT = 32,
    REFRESH_TICKS = TICK_FREQUENCY * 25 / 100, /* 0.25s */
};

enum {
    TRANSPORT_BUTTON_PREV,
    TRANSPORT_BUTTON_PLAY,
    TRANSPORT_BUTTON_PAUSE,
    TRANSPORT_BUTTON_STOP,
    TRANSPORT_BUTTON_NEXT,
    TRANSPORT_BUTTON_SHUFFLE,
    TRANSPORT_BUTTON_LOOP,
};

enum {
    PLAY_STATE_STOPPED,
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSED,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;
    progress_bar_st progress_bar;
    widget_st transport_buttons[TRANSPORT_BUTTON_COUNT];
    list_widget_st play_list;
    widget_st prev_page_button;
    widget_st next_page_button;

    widget_st *widgets[TRANSPORT_BUTTON_COUNT + 6];

    file_st *songs[SONG_MAX_COUNT];
    uint32_t song_ticks[SONG_MAX_COUNT];
    int song_count;

    int play_state;
    uint32_t total_ticks;
    uint32_t last_elapsed_secs;
    uint32_t last_total_secs;
} app_state_st;

static app_state_st *app_state = NULL;

static bitmap_st *transport_button_icons[TRANSPORT_BUTTON_COUNT] = {
    &icon_player_prev,
    &icon_player_play,
    &icon_player_pause,
    &icon_player_stop,
    &icon_player_next,
    &icon_player_shuffle,
    &icon_player_loop,
};

static const char *
get_song_name(int index)
{
    app_state_st *a = app_state;

    if (index < 0 || index >= a->song_count) {
        return "-";
    }

    return a->songs[index]->name;
}

static const note_st *
get_song_notes(int index)
{
    static note_st stop_note = { 0, 0 };
    app_state_st *a = app_state;

    if (index < 0 || index >= a->song_count) {
        return &stop_note;
    }

    return (const note_st *)app_state->songs[index]->addr;
}

static int
get_play_state(const speaker_state_st *st)
{
    speaker_state_st local_st;

    if (!st) {
        krn_speaker_get_state(&local_st);
        st = &local_st;
    }

    if (st->song_owner != &app_player) {
        return PLAY_STATE_STOPPED;
    }

    switch (st->state) {
    case SPEAKER_STATE_PLAYING: return PLAY_STATE_PLAYING;
    case SPEAKER_STATE_PAUSED: return PLAY_STATE_PAUSED;
    default: return PLAY_STATE_STOPPED;
    }
}

static void
update_status(void)
{
    app_state_st *a = app_state;

    if (!a->window.active) {
        return;
    }

    if (a->song_count == 0) {
        gui_status_set("No songs found");
        return;
    }

    switch (a->play_state) {
    case PLAY_STATE_PLAYING:
        gui_status_set("Playing: %s", get_song_name(a->play_list.cur_index));
        break;
    case PLAY_STATE_PAUSED:
        gui_status_set("Paused: %s", get_song_name(a->play_list.cur_index));
        break;
    default:
        gui_status_set("Stopped: %s", get_song_name(a->play_list.cur_index));
        break;
    }
}

static void
draw_title(void)
{
    app_state_st *a = app_state;
    rect_st rect = gui_rect_make(CONTENT_X, TITLE_Y, CONTENT_WIDTH, TITLE_HEIGHT);
    const char *title = get_song_name(a->play_list.cur_index);

    gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
    gui_surface_draw_str_centered(a->window.surface, rect, font_5x8, title,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, rect);
}

static void
draw_time(uint32_t elapsed_ticks)
{
    app_state_st *a = app_state;
    rect_st rect = gui_rect_make(CONTENT_X, TIME_Y, CONTENT_WIDTH, TIME_HEIGHT);
    uint32_t elapsed_secs = elapsed_ticks / TICK_FREQUENCY;
    uint32_t total_secs = a->total_ticks / TICK_FREQUENCY;
    char time[16];

    snprintf(time, sizeof(time), "%02u:%02u / %02u:%02u",
        MIN(elapsed_secs / 60, 99u), elapsed_secs % 60,
        MIN(total_secs / 60, 99u), total_secs % 60);

    gui_surface_draw_rect(a->window.surface, rect, COLOR_WIDGET_BG);
    gui_surface_draw_str_centered(a->window.surface, rect, font_5x8, time,
        COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_wm_render_window_region(&a->window, rect);

    a->last_elapsed_secs = elapsed_secs;
    a->last_total_secs = total_secs;
}

static void
sync_playback_state(const speaker_state_st *st, int refresh_status)
{
    speaker_state_st local_st;
    uint32_t elapsed_ticks = 0;
    app_state_st *a = app_state;

    if (!st) {
        krn_speaker_get_state(&local_st);
        st = &local_st;
    }

    a->play_state = get_play_state(st);

    if (a->play_state != PLAY_STATE_STOPPED) {
        elapsed_ticks = MIN(st->song_elapsed_ticks, a->total_ticks);
    }

    if (elapsed_ticks / TICK_FREQUENCY != a->last_elapsed_secs ||
        a->total_ticks / TICK_FREQUENCY != a->last_total_secs) {
        draw_time(elapsed_ticks);
    }

    gui_progress_bar_set_values(&a->progress_bar, a->total_ticks, elapsed_ticks);

    if (refresh_status) {
        update_status();
    }
}

static void
select_song(int index)
{
    app_state_st *a = app_state;

    if (index < 0 || index >= a->song_count) {
        return;
    }

    gui_list_widget_set_index(&a->play_list, index);

    a->total_ticks = a->song_ticks[index];
    gui_progress_bar_set_values(&a->progress_bar, a->total_ticks, 0);

    draw_title();
}

static void
set_song(int index, int force_play)
{
    app_state_st *a = app_state;
    int play_state;

    if (a->song_count == 0) {
        return;
    }

    index = (index + a->song_count) % a->song_count;

    select_song(index);

    play_state = get_play_state(NULL);

    if (play_state == PLAY_STATE_PLAYING || force_play) {
        krn_speaker_play_song(get_song_notes(index), &app_player);
    } else if (play_state == PLAY_STATE_PAUSED) {
        krn_speaker_stop(&app_player);
    }

    sync_playback_state(NULL, 1);
}

static void
pause_song(void)
{
    krn_speaker_pause(&app_player);
    sync_playback_state(NULL, 1);
}

static void
resume_song(void)
{
    krn_speaker_resume(&app_player);
    sync_playback_state(NULL, 1);
}

static void
stop_song(void)
{
    krn_speaker_stop(&app_player);
    sync_playback_state(NULL, 1);
}

static void
seek_song(uint32_t ticks)
{
    app_state_st *a = app_state;
    int index = a->play_list.cur_index;

    if (a->total_ticks == 0) {
        return;
    }

    ticks = MIN(ticks, a->total_ticks - 1);

    if (get_play_state(NULL) == PLAY_STATE_STOPPED) {
        if (index < 0 || index >= a->song_count) {
            return;
        }

        krn_speaker_play_song(get_song_notes(index), &app_player);
    }

    krn_speaker_seek(&app_player, ticks);

    sync_playback_state(NULL, 1);
}

static void
advance_song(void)
{
    app_state_st *a = app_state;
    int cur_index = a->play_list.cur_index;
    int new_index;

    if (a->transport_buttons[TRANSPORT_BUTTON_LOOP].active) {
        set_song(cur_index, 1);
        return;
    }

    if (a->transport_buttons[TRANSPORT_BUTTON_SHUFFLE].active && a->song_count > 1) {
        do {
            new_index = (int)(rand() % (uint32_t)a->song_count);
        } while (new_index == cur_index);

        set_song(new_index, 1);
        return;
    }

    if (cur_index + 1 >= a->song_count) {
        select_song(0);
        stop_song();
        return;
    }

    set_song(cur_index + 1, 1);
}

static void
on_play_click(void)
{
    app_state_st *a = app_state;
    int play_state = get_play_state(NULL);

    if (play_state == PLAY_STATE_PLAYING) {
        return;
    }

    if (play_state == PLAY_STATE_PAUSED) {
        resume_song();
    } else {
        set_song(a->play_list.cur_index, 1);
    }
}

static void
on_shuffle_click(void)
{
    app_state_st *a = app_state;
    widget_st *shuffle_btn = &a->transport_buttons[TRANSPORT_BUTTON_SHUFFLE];
    widget_st *loop_btn = &a->transport_buttons[TRANSPORT_BUTTON_LOOP];

    shuffle_btn->active = !shuffle_btn->active;

    if (shuffle_btn->active) {
        loop_btn->active = 0;
        gui_widget_draw(loop_btn);
    }
}

static void
on_loop_click(void)
{
    app_state_st *a = app_state;
    widget_st *shuffle_btn = &a->transport_buttons[TRANSPORT_BUTTON_SHUFFLE];
    widget_st *loop_btn = &a->transport_buttons[TRANSPORT_BUTTON_LOOP];

    loop_btn->active = !loop_btn->active;

    if (loop_btn->active) {
        shuffle_btn->active = 0;
        gui_widget_draw(shuffle_btn);
    }
}

static void
on_progress_bar_down(progress_bar_st *bar _unsd, int value)
{
    seek_song((uint32_t)value);
}

static void
on_button_down(widget_st *widget, event_st event, point_st pos)
{
    app_state_st *a = app_state;

    switch (widget->tag1) {
    case TRANSPORT_BUTTON_PREV: set_song(a->play_list.cur_index - 1, 0); break;
    case TRANSPORT_BUTTON_PLAY: on_play_click(); break;
    case TRANSPORT_BUTTON_PAUSE: pause_song(); break;
    case TRANSPORT_BUTTON_STOP: stop_song(); break;
    case TRANSPORT_BUTTON_NEXT: set_song(a->play_list.cur_index + 1, 0); break;
    case TRANSPORT_BUTTON_SHUFFLE: on_shuffle_click(); break;
    case TRANSPORT_BUTTON_LOOP: on_loop_click(); break;
    }

    gui_button_on_pointer_down(widget, event, pos);
}

static void
on_play_list_select(list_widget_st *list _unsd, int index)
{
    set_song(index, 0);
}

static const char *
get_play_list_label(list_widget_st *list _unsd, int index)
{
    return get_song_name(index);
}

static const char *
get_play_list_right_label(list_widget_st *list _unsd, int index)
{
    static char buf[8];

    app_state_st *a = app_state;
    uint32_t secs;

    if (index < 0 || index >= a->song_count) {
        return NULL;
    }

    secs = a->song_ticks[index] / TICK_FREQUENCY;

    snprintf(buf, sizeof(buf), "  %02u:%02u", MIN(secs / 60, 99u), secs % 60);

    return buf;
}

static void
on_tick(window_st *window)
{
    static unsigned tick_count = 0;

    app_state_st *a = app_state;
    speaker_state_st st;
    int prev_state = a->play_state;

    krn_speaker_get_state(&st);
    a->play_state = get_play_state(&st);

    if (st.song_owner != &app_player) {
        if (prev_state != PLAY_STATE_STOPPED) {
            sync_playback_state(&st, 1);
        }

        return;
    }

    if (a->play_state == PLAY_STATE_STOPPED) {
        advance_song();
        return;
    }

    if (a->play_state != PLAY_STATE_PLAYING || !window->visible) {
        return;
    }

    if (++tick_count >= REFRESH_TICKS) {
        tick_count = 0;
        sync_playback_state(&st, 0);
    }
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    a->last_elapsed_secs = (uint32_t)-1;
    a->last_total_secs = (uint32_t)-1;

    gui_window_draw(window, COLOR_WIDGET_BG);
    draw_title();
    sync_playback_state(NULL, 0);
}

static void
on_active_change(window_st *window)
{
    if (window->active) {
        sync_playback_state(NULL, 1);
    }
}

static void
close_window(window_st *window)
{
    krn_speaker_stop(&app_player);

    gui_wm_remove_window(window);
    app_player.main_window = NULL;

    heap_free(app_state);
    app_state = NULL;
}

static void
init_songs(void)
{
    app_state_st *a = app_state;
    file_st *file;
    size_t i;

    for (i = 0; i < file_count() && a->song_count < SONG_MAX_COUNT; ++i) {
        file = file_get(i);

        if (file && file->type == FILE_TYPE_SONG) {
            a->song_ticks[a->song_count] = song_get_total_ticks((const note_st *)file->addr);
            a->songs[a->song_count++] = file;
        }
    }
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
    a->window.title = "Player";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_tick = on_tick;
    a->window.on_active_change = on_active_change;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_progress_bar(void)
{
    app_state_st *a = app_state;

    gui_progress_bar_init(&a->progress_bar, 0, 0);

    a->progress_bar.widget.rect = gui_rect_make(CONTENT_X, PROGRESS_Y,
        CONTENT_WIDTH, PROGRESS_HEIGHT);
    a->progress_bar.on_pointer_down = on_progress_bar_down;

    gui_window_add_widget(&a->window, &a->progress_bar.widget);
}

static void
init_transport_buttons(void)
{
    app_state_st *a = app_state;
    int i;
    widget_st *button;

    for (i = 0; i < TRANSPORT_BUTTON_COUNT; ++i) {
        button = &a->transport_buttons[i];

        gui_button_init(button);

        button->rect = gui_rect_make(
            CONTENT_X + i * (TRANSPORT_BUTTON_WIDTH + TRANSPORT_BUTTON_GAP),
            TRANSPORT_BUTTON_Y,
            TRANSPORT_BUTTON_WIDTH,
            TRANSPORT_BUTTON_HEIGHT
        );
        button->tag1 = i;
        button->bitmap = transport_button_icons[i];
        button->on_pointer_down = on_button_down;

        gui_window_add_widget(&a->window, button);
    }
}

static void
init_play_list(void)
{
    app_state_st *a = app_state;

    a->play_list.grid.cell_width = PLAY_LIST_CELL_WIDTH;
    a->play_list.grid.cell_height = PLAY_LIST_CELL_HEIGHT;
    a->play_list.grid.cols = PLAY_LIST_COLS;
    a->play_list.grid.rows = PLAY_LIST_ROWS;
    a->play_list.grid.border = PLAY_LIST_BORDER;
    a->play_list.grid.x = PLAY_LIST_X;
    a->play_list.grid.y = PLAY_LIST_Y;
    a->play_list.widget.font = font_5x8;

    a->play_list.get_label = get_play_list_label;
    a->play_list.get_right_label = get_play_list_right_label;
    a->play_list.on_select = on_play_list_select;

    gui_list_widget_init(&a->play_list);
    gui_list_widget_set_item_count(&a->play_list, a->song_count);

    gui_window_add_widget(&a->window, &a->play_list.widget);
}

static void
init_page_buttons(void)
{
    app_state_st *a = app_state;

    gui_list_widget_init_page_buttons(&a->play_list,
        &a->prev_page_button, &a->next_page_button);

    a->prev_page_button.rect = gui_rect_make(
        PAGE_BUTTON_PREV_X,
        PAGE_BUTTON_Y,
        PAGE_BUTTON_WIDTH,
        PAGE_BUTTON_HEIGHT
    );

    a->next_page_button.rect = gui_rect_make(
        PAGE_BUTTON_NEXT_X,
        PAGE_BUTTON_Y,
        PAGE_BUTTON_WIDTH,
        PAGE_BUTTON_HEIGHT
    );

    gui_window_add_widget(&a->window, &a->prev_page_button);
    gui_window_add_widget(&a->window, &a->next_page_button);
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Player app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    init_songs();
    init_window();
    init_progress_bar();
    init_transport_buttons();
    init_play_list();
    init_page_buttons();

    select_song(app_state->play_list.cur_index);

    app_player.main_window = &app_state->window;

    return E_OK;
}

global app_st app_player = {
    .icon = &icon_player,
    .init = init_app,
};
