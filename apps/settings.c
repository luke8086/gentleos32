/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: settings.c - Settings app
 */

#include <gui.h>

enum {
    PADDING = 12,
    LABEL_HEIGHT = 8,
    LABEL_SPACING = 3,
    GRID_BORDER = 1,

    PATTERN_COLS = 6,
    PATTERN_ROWS = 3,
    PATTERN_CELL_WIDTH = 32,
    PATTERN_CELL_HEIGHT = 24,
    PATTERN_COUNT = (PATTERN_COLS * PATTERN_ROWS),
    PATTERN_GRID_WIDTH = GRID_WIDTH_SPACED(PATTERN_CELL_WIDTH, PATTERN_COLS, GRID_BORDER),
    PATTERN_GRID_HEIGHT = GRID_HEIGHT_SPACED(PATTERN_CELL_HEIGHT, PATTERN_ROWS, GRID_BORDER),

    COLOR_COLS = 8,
    COLOR_ROWS = 2,
    COLOR_CELL_WIDTH = 24,
    COLOR_CELL_HEIGHT = 18,
    COLOR_COUNT = (COLOR_COLS * COLOR_ROWS),
    COLOR_GRID_WIDTH = GRID_WIDTH_SPACED(COLOR_CELL_WIDTH, COLOR_COLS, GRID_BORDER),
    COLOR_GRID_HEIGHT = GRID_HEIGHT_SPACED(COLOR_CELL_HEIGHT, COLOR_ROWS, GRID_BORDER),

    THEME_COUNT = GUI_THEME_COUNT,
    THEME_COLS = 1,
    THEME_ROWS = THEME_COUNT,
    THEME_CELL_HEIGHT = 17,
    THEME_CELL_WIDTH = COLOR_GRID_WIDTH - 2 * GRID_BORDER,
    THEME_GRID_WIDTH = GRID_WIDTH_SPACED(THEME_CELL_WIDTH, THEME_COLS, GRID_BORDER),
    THEME_GRID_HEIGHT = GRID_HEIGHT_SPACED(THEME_CELL_HEIGHT, THEME_ROWS, GRID_BORDER),

    WALLPAPER_COLS = 1,
    WALLPAPER_ROWS = 7,
    WALLPAPER_CELL_HEIGHT = THEME_CELL_HEIGHT,
    WALLPAPER_CELL_WIDTH = THEME_CELL_WIDTH,
    WALLPAPER_GRID_WIDTH = GRID_WIDTH_SPACED(WALLPAPER_CELL_WIDTH, WALLPAPER_COLS, GRID_BORDER),
    WALLPAPER_GRID_HEIGHT = GRID_HEIGHT_SPACED(WALLPAPER_CELL_HEIGHT, WALLPAPER_ROWS, GRID_BORDER),

    COLUMN_TOP = TITLE_BAR_HEIGHT + PADDING,
    LEFT_COLUMN_X = PADDING,
    RIGHT_COLUMN_X = LEFT_COLUMN_X + THEME_GRID_WIDTH + PADDING,

    THEME_LABEL_Y = COLUMN_TOP,
    THEME_GRID_X = LEFT_COLUMN_X,
    THEME_GRID_Y = THEME_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    WALLPAPER_LABEL_Y = THEME_GRID_Y + THEME_GRID_HEIGHT + PADDING,
    WALLPAPER_GRID_X = LEFT_COLUMN_X,
    WALLPAPER_GRID_Y = WALLPAPER_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    PATTERN_LABEL_Y = COLUMN_TOP,
    PATTERN_GRID_X = RIGHT_COLUMN_X,
    PATTERN_GRID_Y = PATTERN_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    COLOR1_LABEL_Y = PATTERN_GRID_Y + PATTERN_GRID_HEIGHT + PADDING + 5,
    COLOR1_GRID_X = RIGHT_COLUMN_X,
    COLOR1_GRID_Y = COLOR1_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    COLOR2_LABEL_Y = COLOR1_GRID_Y + COLOR_GRID_HEIGHT + PADDING,
    COLOR2_GRID_X = RIGHT_COLUMN_X,
    COLOR2_GRID_Y = COLOR2_LABEL_Y + LABEL_HEIGHT + LABEL_SPACING,

    WINDOW_WIDTH = RIGHT_COLUMN_X + COLOR_GRID_WIDTH + PADDING,
    WINDOW_HEIGHT = WALLPAPER_GRID_Y + WALLPAPER_GRID_HEIGHT + PADDING,

    WIDGETS_COUNT = 2 + PATTERN_COUNT + COLOR_COUNT + COLOR_COUNT + 2,
};

typedef struct {
    uint8_t window_pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    surface_st window_surface;
    window_st window;

    widget_st title_bar;
    widget_st close_button;

    list_widget_st theme_list;
    list_widget_st wallpaper_list;
    widget_st pattern_buttons[PATTERN_COUNT];
    widget_st color1_buttons[COLOR_COUNT];
    widget_st color2_buttons[COLOR_COUNT];

    widget_st *widgets[WIDGETS_COUNT];

    grid_st pattern_grid;
    grid_st color1_grid;
    grid_st color2_grid;

    widget_st *active_pattern_button;
    widget_st *active_color1_button;
    widget_st *active_color2_button;

    bitmap_st *wallpapers[WALLPAPER_ROWS];
    const char *wallpaper_names[WALLPAPER_ROWS];
    int wallpaper_count;
} app_state_st;

static app_state_st *app_state = NULL;

static bitmap_st *patterns[] = {
    NULL,
    &bitmap_pattern_01,
    &bitmap_pattern_02,
    &bitmap_pattern_03,
    &bitmap_pattern_04,
    &bitmap_pattern_05,
    &bitmap_pattern_06,
    &bitmap_pattern_07,
    &bitmap_pattern_08,
    &bitmap_pattern_09,
    &bitmap_pattern_10,
    &bitmap_pattern_11,
    &bitmap_pattern_12,
    &bitmap_pattern_13,
    &bitmap_pattern_14,
    &bitmap_pattern_15,
    &bitmap_pattern_16,
    &bitmap_pattern_a,
};

static const char *theme_names[THEME_COUNT] = {
    "Default",
    "Mono",
    "Neon",
};

static void
select_active_pattern_button(void)
{
    app_state_st *a = app_state;
    widget_st *prev = a->active_pattern_button;

    a->active_pattern_button = NULL;

    for (int i = 0; i < PATTERN_COUNT; i++) {
        if (gui_wm_wallpaper == patterns[i]) {
            a->active_pattern_button = &a->pattern_buttons[i];
            break;
        }
    }

    if (a->active_pattern_button == prev) {
        return;
    }

    if (prev) {
        gui_widget_draw(prev);
    }

    if (a->active_pattern_button) {
        gui_widget_draw(a->active_pattern_button);
    }
}

static void
select_active_color_buttons(void)
{
    app_state_st *a = app_state;

    for (int i = 0; i < COLOR_COUNT; i++) {
        if (gui_theme.desktop == i) {
            a->active_color1_button = &a->color1_buttons[i];
        }

        if (gui_theme.desktop_alt == i) {
            a->active_color2_button = &a->color2_buttons[i];
        }
    }
}

static void
select_active_wallpaper_item(void)
{
    app_state_st *a = app_state;
    int index = 0;

    for (int i = 1; i < a->wallpaper_count; i++) {
        if (a->wallpapers[i] == gui_wm_wallpaper) {
            index = i;
            break;
        }
    }

    gui_list_widget_set_index(&a->wallpaper_list, index);
}

static void
load_wallpapers(void)
{
    app_state_st *a = app_state;

    a->wallpapers[0] = NULL;
    a->wallpaper_names[0] = "None";
    a->wallpaper_count = 1;

    for (size_t i = 0; i < file_count(); ++i) {
        file_st *file = file_get(i);
        bitmap_st *bitmap;

        if (a->wallpaper_count >= WALLPAPER_ROWS) {
            break;
        }

        if (!file || file->type != FILE_TYPE_BITMAP) {
            continue;
        }

        bitmap = gui_load_bitmap(file->name);

        if (bitmap && gui_wm_is_valid_wallpaper(bitmap)) {
            a->wallpapers[a->wallpaper_count] = bitmap;
            a->wallpaper_names[a->wallpaper_count] = file->name;
            ++a->wallpaper_count;
        }
    }
}

static void
set_wallpaper(bitmap_st *bitmap)
{
    if (bitmap == gui_wm_wallpaper) {
        return;
    }

    gui_wm_wallpaper = bitmap;

    select_active_pattern_button();
    select_active_wallpaper_item();

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static const char *
get_theme_label(list_widget_st *list _unsd, int index)
{
    return theme_names[index];
}

static void
on_theme_select(list_widget_st *list _unsd, int index)
{
    if (index == gui_theme.index) {
        return;
    }

    gui_theme_set(index);

    select_active_color_buttons();

    gui_wm_redraw_all();
}

static const char *
get_wallpaper_label(list_widget_st *list _unsd, int index)
{
    return app_state->wallpaper_names[index];
}

static void
on_wallpaper_select(list_widget_st *list _unsd, int index)
{
    set_wallpaper(app_state->wallpapers[index]);
}

static void
draw_pattern_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;
    surface_st *sf = widget->window->surface;
    int is_active = (widget == a->active_pattern_button);

    int idx = widget->tag1;

    if (widget->tag1 == 0) {
        gui_surface_draw_rect(sf, rect, COLOR_WIDGET_BG);
    } else {
        gui_surface_draw_pattern_rel(sf, rect, patterns[idx], COLOR_BORDER, COLOR_WIDGET_BG);
    }

    if (is_active) {
        gui_surface_draw_border(sf, rect, COLOR_BORDER);
        rect = gui_rect_shrink(rect, 1);
    }

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
on_pattern_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    set_wallpaper(patterns[widget->tag1]);
}

static void
draw_color_button(widget_st *widget)
{
    app_state_st *a = app_state;

    rect_st rect = widget->rect;
    surface_st *sf = widget->window->surface;

    if (widget == a->active_color1_button || widget == a->active_color2_button) {
        gui_surface_draw_rect(sf, rect, COLOR_BORDER);
        rect = gui_rect_shrink(rect, 1);
    }

    gui_surface_draw_rect(sf, rect, widget->tag2);

    gui_wm_render_window_region(widget->window, widget->rect);
}

static void
on_color1_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev = a->active_color1_button;
    a->active_color1_button = widget;

    gui_theme.desktop = widget->tag2;

    if (prev && prev != widget) {
        gui_widget_draw(prev);
    }

    gui_widget_draw(widget);

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static void
on_color2_button_press(widget_st *widget, event_st event _unsd, point_st pos _unsd)
{
    app_state_st *a = app_state;

    widget_st *prev = a->active_color2_button;
    a->active_color2_button = widget;

    gui_theme.desktop_alt = widget->tag2;

    if (prev && prev != widget) {
        gui_widget_draw(prev);
    }

    gui_widget_draw(widget);

    gui_wm_render_desktop_region(gui_wm_container, NULL);
}

static void
draw_window(window_st *window)
{
    app_state_st *a = app_state;

    gui_window_draw_frame(window, COLOR_WIDGET_BG);

    gui_surface_draw_str_at(window->surface, THEME_GRID_X, THEME_LABEL_Y, font_8x8,
        "Theme", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str_at(window->surface, WALLPAPER_GRID_X, WALLPAPER_LABEL_Y, font_8x8,
        "Wallpaper", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str_at(window->surface, PATTERN_GRID_X, PATTERN_LABEL_Y, font_8x8,
        "Desktop pattern", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str_at(window->surface, COLOR1_GRID_X, COLOR1_LABEL_Y, font_8x8,
        "Desktop color 1", COLOR_WIDGET_FG, COLOR_WIDGET_BG);
    gui_surface_draw_str_at(window->surface, COLOR2_GRID_X, COLOR2_LABEL_Y, font_8x8,
        "Desktop color 2", COLOR_WIDGET_FG, COLOR_WIDGET_BG);

    gui_grid_fill(&a->pattern_grid, window, COLOR_BORDER);
    gui_grid_fill(&a->color1_grid, window, COLOR_BORDER);
    gui_grid_fill(&a->color2_grid, window, COLOR_BORDER);

    gui_window_draw_widgets(window);
}

static void
close_window(window_st *window _unsd)
{
    gui_wm_remove_window(window);
    app_settings.main_window = NULL;

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
    a->window.title = "Settings";
    a->window.widgets = a->widgets;
    a->window.widgets_capacity = sizeof(a->widgets) / sizeof(a->widgets[0]);
    a->window.draw = draw_window;
    a->window.on_close = close_window;

    gui_window_init_frame(&a->window, &a->title_bar, &a->close_button);
}

static void
init_theme_list(void)
{
    app_state_st *a = app_state;

    a->theme_list.grid.cell_width = THEME_CELL_WIDTH;
    a->theme_list.grid.cell_height = THEME_CELL_HEIGHT;
    a->theme_list.grid.cols = THEME_COLS;
    a->theme_list.grid.rows = THEME_ROWS;
    a->theme_list.grid.border = GRID_BORDER;
    a->theme_list.grid.x = THEME_GRID_X;
    a->theme_list.grid.y = THEME_GRID_Y;

    a->theme_list.get_label = get_theme_label;
    a->theme_list.on_select = on_theme_select;

    gui_list_widget_init(&a->theme_list);
    gui_window_add_widget(&a->window, &a->theme_list.widget);

    gui_list_widget_set_item_count(&a->theme_list, THEME_COUNT);
    gui_list_widget_set_index(&a->theme_list, gui_theme.index);
}

static void
init_wallpaper_list(void)
{
    app_state_st *a = app_state;

    a->wallpaper_list.grid.cell_width = WALLPAPER_CELL_WIDTH;
    a->wallpaper_list.grid.cell_height = WALLPAPER_CELL_HEIGHT;
    a->wallpaper_list.grid.cols = WALLPAPER_COLS;
    a->wallpaper_list.grid.rows = WALLPAPER_ROWS;
    a->wallpaper_list.grid.border = GRID_BORDER;
    a->wallpaper_list.grid.x = WALLPAPER_GRID_X;
    a->wallpaper_list.grid.y = WALLPAPER_GRID_Y;

    a->wallpaper_list.get_label = get_wallpaper_label;
    a->wallpaper_list.on_select = on_wallpaper_select;

    gui_list_widget_init(&a->wallpaper_list);
    gui_window_add_widget(&a->window, &a->wallpaper_list.widget);

    gui_list_widget_set_item_count(&a->wallpaper_list, a->wallpaper_count);
}

static void
init_pattern_buttons(void)
{
    app_state_st *a = app_state;

    a->pattern_grid.cell_width = PATTERN_CELL_WIDTH;
    a->pattern_grid.cell_height = PATTERN_CELL_HEIGHT;
    a->pattern_grid.cols = PATTERN_COLS;
    a->pattern_grid.rows = PATTERN_ROWS;
    a->pattern_grid.border = GRID_BORDER;
    a->pattern_grid.x = PATTERN_GRID_X;
    a->pattern_grid.y = PATTERN_GRID_Y;

    for (int i = 0; i < PATTERN_COUNT; i++) {
        int col = i % PATTERN_COLS;
        int row = i / PATTERN_COLS;

        a->pattern_buttons[i].rect = gui_grid_cell_rect(&a->pattern_grid, col, row);
        a->pattern_buttons[i].tag1 = i;
        a->pattern_buttons[i].window = &a->window;
        a->pattern_buttons[i].draw = draw_pattern_button;
        a->pattern_buttons[i].on_pointer_down = on_pattern_button_press;
        a->pattern_buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &a->pattern_buttons[i]);
    }
}

static void
init_color_buttons(grid_st *grid, widget_st *buttons, int grid_x, int grid_y,
    void (*on_press)(widget_st *, event_st, point_st))
{
    app_state_st *a = app_state;

    grid->cell_width = COLOR_CELL_WIDTH;
    grid->cell_height = COLOR_CELL_HEIGHT;
    grid->cols = COLOR_COLS;
    grid->rows = COLOR_ROWS;
    grid->border = GRID_BORDER;
    grid->x = grid_x;
    grid->y = grid_y;

    for (int i = 0; i < COLOR_COUNT; i++) {
        int col = i % COLOR_COLS;
        int row = i / COLOR_COLS;

        buttons[i].rect = gui_grid_cell_rect(grid, col, row);
        buttons[i].tag2 = i;
        buttons[i].window = &a->window;
        buttons[i].draw = draw_color_button;
        buttons[i].on_pointer_down = on_press;
        buttons[i].press_on_move_in = 1;

        gui_window_add_widget(&a->window, &buttons[i]);
    }
}

static int
init_app(void)
{
    ASSERT(!app_state);

    app_state = heap_alloc(sizeof(app_state_st), "Settings app", 0);

    if (!app_state) {
        return E_NOT_ENOUGH_MEMORY;
    }

    load_wallpapers();

    init_window();
    init_theme_list();
    init_wallpaper_list();
    init_pattern_buttons();
    init_color_buttons(&app_state->color1_grid, app_state->color1_buttons,
        COLOR1_GRID_X, COLOR1_GRID_Y, on_color1_button_press);
    init_color_buttons(&app_state->color2_grid, app_state->color2_buttons,
        COLOR2_GRID_X, COLOR2_GRID_Y, on_color2_button_press);

    select_active_wallpaper_item();
    select_active_pattern_button();
    select_active_color_buttons();

    app_settings.main_window = &app_state->window;

    return E_OK;
}

global app_st app_settings = {
    .icon = &icon_settings,
    .init = init_app,
};
