/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License
 *
 * File: card.c - Generic code for implementing card games
 */

#include <feat.h>

global const char *card_rank_str[CARD_RANK_COUNT] = {
    "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

global const char *card_suit_str[CARD_SUIT_COUNT] = {
    "\x03", "\x04", "\x05", "\x06"
};

global void
card_deck_init(card_t *deck, int n)
{
    int i;

    for (i = 0; i < n; ++i) {
        deck[i] = i;
    }
}

global void
card_deck_shuffle(card_t *deck, int n)
{
    card_t tmp;
    int i, j;

    for (i = n - 1; i > 0; --i) {
        j = rand() % (i + 1);
        tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
}

global void
card_pile_update_step(card_game_st *game, card_pile_st *p)
{
    if (!p->is_cascade || p->count <= 1) {
        p->step = game->card_step;
        return;
    }

    int avail = p->widget->rect.height - game->card_height;
    int step = avail / (p->count - 1);

    step = MAX(step, 1);
    step = MIN(step, game->card_step);

    p->step = step;
}

global int
card_pile_get_card_index_by_ypos(card_pile_st *p, int ypos)
{
    if (!p->is_cascade || p->count <= 1) {
        return p->count - 1;
    }

    if (ypos < 0) {
        return 0;
    }

    int top_y = (p->count - 1) * p->step;

    if (ypos >= top_y) {
        return p->count - 1;
    }

    int idx = ypos / p->step;

    if (idx >= p->count) {
        idx = p->count - 1;
    }

    return idx;
}

global card_t
card_pile_pop(card_pile_st *p)
{
    ASSERT(p->count > 0);
    return p->cards[--p->count];
}

global void
card_pile_push(card_pile_st *p, card_t card)
{
    if (p->replace_on_push) {
        p->cards[0] = card;
        p->count = 1;
        return;
    }

    ASSERT(p->count < p->capacity);
    p->cards[p->count++] = card;
}

global void
card_pile_uncover_top(card_pile_st *p)
{
    if (p->count > 0 && p->face_up_from > p->count - 1) {
        p->face_up_from = p->count - 1;
    }
}

global int
card_pile_top_y(card_pile_st *p)
{
    if (!p->is_cascade || p->count <= 1) {
        return p->widget->rect.y;
    }

    return p->widget->rect.y + (p->count - 1) * p->step;
}

global void
card_draw(card_game_st *game, int x, int y, card_t card, int selected)
{
    rect_st r = gui_rect_make(x, y, game->card_width, game->card_height);

    if (card == CARD_EMPTY) {
        gui_surface_draw_rect(game->surface, r, COLOR_WIDGET_BG);
        gui_surface_draw_border(game->surface, r, COLOR_BORDER);
        return;
    }

    int rank = CARD_RANK(card);
    int suit = CARD_SUIT(card);

    uint8_t fg = CARD_COLOR(card) ? COLOR_CARD_RED_FG : COLOR_CARD_BLACK_FG;
    uint8_t bg = COLOR_CARD_FRONT_BG;

    if (selected) {
        fg = COLOR_CARD_SEL_FG;
        bg = COLOR_CARD_SEL_BG;
    }

    gui_surface_draw_rect(game->surface, r, bg);
    gui_surface_draw_border(game->surface, r, COLOR_BORDER);

    gui_surface_draw_str(game->surface, x + 3, y + 2, font_8x8,
        card_rank_str[rank], fg, bg);

    gui_surface_draw_str_cc(game->surface, r, font_8x16,
        card_suit_str[suit], fg, bg);

    gui_surface_draw_str(game->surface,
        x + game->card_width - strlen(card_rank_str[rank]) * 8 - 3,
        y + game->card_height - 10,
        font_8x8, card_rank_str[rank], fg, bg);
}

global void
card_stub_draw(card_game_st *game, int x, int y, int height, card_t card, int selected)
{
    rect_st r = gui_rect_make(x, y, game->card_width, height);

    int rank = CARD_RANK(card);
    int suit = CARD_SUIT(card);
    uint8_t fg = CARD_COLOR(card) ? COLOR_CARD_RED_FG : COLOR_CARD_BLACK_FG;
    uint8_t bg = COLOR_CARD_FRONT_BG;

    if (selected) {
        fg = COLOR_CARD_SEL_FG;
        bg = COLOR_CARD_SEL_BG;
    }

    gui_surface_draw_rect(game->surface, r, bg);
    gui_surface_draw_h_seg(game->surface, x, y, game->card_width, COLOR_BORDER);
    gui_surface_draw_v_seg(game->surface, x, y, height, COLOR_BORDER);
    gui_surface_draw_v_seg(game->surface, x + game->card_width - 1, y, height, COLOR_BORDER);

    gui_surface_draw_str(game->surface, x + 3, y + 2, font_8x8,
        card_rank_str[rank], fg, bg);

    gui_surface_draw_str(game->surface,
        x + game->card_width - strlen(card_suit_str[suit]) * 8 - 3,
        y + 2,
        font_8x8, card_suit_str[suit], fg, bg);
}

global void
card_back_draw(card_game_st *game, int x, int y)
{
    rect_st r = gui_rect_make(x, y, game->card_width, game->card_height);

    gui_surface_draw_pattern_rel(game->surface, r, &bitmap_pattern_card,
        COLOR_CARD_BACK_BG_1, COLOR_CARD_BACK_BG_2);
    gui_surface_draw_border(game->surface, r, COLOR_BORDER);
}

global void
card_back_stub_draw(card_game_st *game, int x, int y, int height)
{
    rect_st r = gui_rect_make(x, y, game->card_width, height);

    gui_surface_draw_pattern_rel(game->surface, r, &bitmap_pattern_card,
        COLOR_CARD_BACK_BG_1, COLOR_CARD_BACK_BG_2);
    gui_surface_draw_h_seg(game->surface, x, y, game->card_width, COLOR_BORDER);
    gui_surface_draw_v_seg(game->surface, x, y, height, COLOR_BORDER);
    gui_surface_draw_v_seg(game->surface, x + game->card_width - 1, y, height, COLOR_BORDER);
}

global void
card_pile_draw(card_game_st *game, card_pile_st *p)
{
    rect_st rect = p->widget->rect;
    int x = rect.x;
    card_t top_card = CARD_PILE_TOP(p);
    int is_top_face_down = (p->count > 0 && p->face_up_from > p->count - 1);
    int sel_count = game->cur_move.src == p ? game->cur_move.count : 0;
    int seq_start = p->count - sel_count;
    int y, i, step, top_y;

    card_pile_update_step(game, p);
    top_y = card_pile_top_y(p);

    gui_surface_draw_rect(game->surface, rect, COLOR_WIDGET_BG);

    if (p->is_cascade && p->count > 1) {
        step = p->step;
        for (i = 0; i < p->count - 1; ++i) {
            y = rect.y + i * step;
            if (i < p->face_up_from) {
                card_back_stub_draw(game, x, y, step);
            } else {
                card_stub_draw(game, x, y, step, p->cards[i], i >= seq_start);
            }
        }
    }

    if (is_top_face_down) {
        card_back_draw(game, x, top_y);
    } else {
        card_draw(game, x, top_y, top_card, sel_count > 0);
    }

    gui_wm_render_window_region(p->widget->window, rect);
}

global void
card_game_exec_cur_move(card_game_st *game)
{
    int i;
    int count = game->cur_move.count;
    card_pile_st *src = game->cur_move.src;
    card_pile_st *dst = game->cur_move.dst;

    game->cur_move.src = NULL;

    ASSERT(src != NULL);
    ASSERT(dst != NULL);

    ASSERT(count <= src->count);
    ASSERT(dst->replace_on_push || dst->count + count <= dst->capacity);

    for (i = 0; i < count; ++i) {
        card_pile_push(dst, src->cards[src->count - count + i]);
    }

    src->count -= count;

    if (src->is_cascade) {
        card_pile_uncover_top(src);
    }

    card_pile_draw(game, src);
    card_pile_draw(game, dst);
}

