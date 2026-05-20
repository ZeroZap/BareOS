/**
 * @file xy_gui.c
 * @brief Hierarchical menu / list UI — message queue + list renderer.
 *
 * Ported from Kohler JV545 UI_V3_List.c + T_Msg.c.
 *
 * Rendering layout (two items per screen):
 *
 *   y=0  ┌──────────────────────────────────────┐
 *        │  <item[first]>   [indicator]          │  top half
 *   y=H/2├──────────────────────────────────────┤
 *        │▌ <item[first+1]> [indicator]          │  bottom half (selected)
 *   y=H  └──────────────────────────────────────┘
 *
 * The selected half is pixel-inverted (black highlight bar).
 * Items wrap around cyclically (unless XY_GUI_F_NOWRAP).
 *
 * Slide animations:
 *   _GUI_SCROLL_DIRECT — no animation (immediate flip)
 *   _GUI_SCROLL_L2R    — new screen slides in from left  (BACK)
 *   _GUI_SCROLL_R2L    — new screen slides in from right (OK/forward)
 */

#include "xy_gui.h"
#include "xy_string.h"

/* ── Row geometry ────────────────────────────────────────────────────── */

#define _ROW_H   (XY_LCD_HEIGHT / 2u)   /* height of each half-screen row */
#define _ROW0_Y  0u
#define _ROW1_Y  _ROW_H

/* Text y-offset within each row (pixels from row top to text baseline) */
#define _TEXT_MARGIN_Y  3u

/* ── Scroll animation types ──────────────────────────────────────────── */

#define _GUI_SCROLL_DIRECT  0u
#define _GUI_SCROLL_L2R     1u   /* back / navigate up */
#define _GUI_SCROLL_R2L     2u   /* forward / navigate down */

/* ── String table ────────────────────────────────────────────────────── */

static const uint8_t _empty_str[] = { 0u };
static const uint8_t *_default_str_fn(uint8_t id) { (void)id; return _empty_str; }
static xy_gui_string_fn s_str_fn = _default_str_fn;

void xy_gui_set_string_fn(xy_gui_string_fn fn)
{
    s_str_fn = fn ? fn : _default_str_fn;
}

static inline const uint8_t *_S(uint8_t id)
{
    return s_str_fn(id);
}

/* ── Event queue ─────────────────────────────────────────────────────── */

typedef char _gui_pow2_chk[((XY_GUI_QUEUE_SIZE & (XY_GUI_QUEUE_SIZE - 1u)) == 0u) ? 1 : -1];
#define _QMASK  (XY_GUI_QUEUE_SIZE - 1u)

static xy_gui_msg_t     s_queue[XY_GUI_QUEUE_SIZE];
static volatile uint8_t s_qhead;   /* producer — written by ISR/post */
static volatile uint8_t s_qtail;   /* consumer — main loop only      */

/* ── Active list and proc ────────────────────────────────────────────── */

xy_gui_list_t *g_xy_gui_list;
static xy_gui_proc_t s_proc;

/* ── Info-mode scroll state ─────────────────────────────────────────── */

static int8_t  s_info_first;   /* first visible line index  */
static int8_t  s_info_lines;   /* total lines in current text */

/* ── Init ───────────────────────────────────────────────────────────── */

void xy_gui_init(void)
{
    g_xy_gui_list = NULL;
    s_proc        = NULL;
    s_qhead       = 0u;
    s_qtail       = 0u;
    memset(s_queue, 0, sizeof(s_queue));
}

/* ── Post ───────────────────────────────────────────────────────────── */

int xy_gui_post(xy_gui_ev_t ev, uint8_t key, void *data)
{
    uint8_t head = s_qhead;
    if (((head + 1u) & _QMASK) == (s_qtail & _QMASK) &&
        (head + 1u - s_qtail) > _QMASK)
        return -1;                         /* full */

    s_queue[head & _QMASK].ev   = ev;
    s_queue[head & _QMASK].key  = key;
    s_queue[head & _QMASK].data = data;
    s_qhead = (uint8_t)(head + 1u);
    return 0;
}

/* ── Switch ─────────────────────────────────────────────────────────── */

void xy_gui_switch(xy_gui_list_t *list)
{
    if (!list) return;
    xy_gui_msg_t msg;

    if (s_proc) {
        msg.ev = XY_GUI_EV_EXIT; msg.key = 0u; msg.data = NULL;
        s_proc(&msg);
    }

    g_xy_gui_list = list;
    s_proc        = list->proc;

    if (s_proc) {
        msg.ev = XY_GUI_EV_INIT; msg.key = 0u; msg.data = NULL;
        s_proc(&msg);
    }
}

/* ── Run ─────────────────────────────────────────────────────────────── */

int xy_gui_run(void)
{
    int n = 0;
    while (s_qtail != s_qhead) {
        xy_gui_msg_t msg = s_queue[s_qtail & _QMASK];
        s_qtail = (uint8_t)(s_qtail + 1u);
        n++;
        if (s_proc) s_proc(&msg);
    }
    return n;
}

/* ══════════════════════════════════════════════════════════════════════
 *  List rendering helpers
 * ══════════════════════════════════════════════════════════════════════ */

/* Advance index forward by 1, wrapping. */
static int8_t _next_idx(int8_t x, int8_t count)
{
    x++;
    return (x >= count) ? 0 : x;
}

/* Draw text and optional state indicator for one display row. */
static void _draw_row(uint8_t row_y, uint8_t str_id, uint8_t state)
{
    uint8_t text_y = (uint8_t)(row_y + _TEXT_MARGIN_Y);
    xy_gdi_text_out(XY_PSYSDC, 2u, text_y, _S(str_id), true);

    /* State indicator: codepoints 4 and 5 in the font (Kohler convention).
     * 1 = selected/checked mark, 2 = alternative indicator. */
    if (state > 0u) {
        uint16_t glyph_code = (state == 1u) ? 5u : 4u;
        xy_gdi_char_out(XY_PSYSDC,
                        (uint8_t)(XY_LCD_WIDTH - 14u),
                        (uint8_t)(text_y + 1u),
                        glyph_code, true);
    }
}

/* Invert one display row (selection highlight). */
static void _highlight_row(uint8_t row_y)
{
    xy_gdi_fill_rect(XY_PSYSDC,
                     1u, (uint8_t)(row_y + 1u),
                     (uint8_t)(XY_LCD_WIDTH - 2u),
                     (uint8_t)(_ROW_H - 2u),
                     2u);  /* c=2: invert */
}

/* ── Count lines in a UTF-8 string using the active font ─────────────── */

static uint8_t _info_count_lines(uint8_t str_id)
{
    const uint8_t *s = _S(str_id);
    uint8_t lines = 0u;
    while (*s) {
        /* Walk one display-width worth of characters */
        uint16_t x = 0u;
        while (*s) {
            const uint8_t *p = s;
            uint16_t c = xy_gdi_utf8_next(&p);
            xy_glyph_t g;
            uint8_t w = 0u;
            if (xy_gdi_get_glyph(c, true, &g)) w = g.advance;
            if (x + w > XY_LCD_WIDTH) break;
            x = (uint16_t)(x + w);
            s = p;
        }
        lines++;
    }
    return lines;
}

/* ── Render info (multi-line scrollable text) ────────────────────────── */

static void _draw_info(void)
{
    const uint8_t *s = _S(g_xy_gui_list->str_id);
    uint8_t y = _TEXT_MARGIN_Y;

    /* Skip 'first' lines */
    int8_t skip = s_info_first;
    while (*s && skip > 0) {
        uint16_t x = 0u;
        while (*s) {
            const uint8_t *p = s;
            uint16_t c = xy_gdi_utf8_next(&p);
            xy_glyph_t g;
            uint8_t w = 0u;
            if (xy_gdi_get_glyph(c, true, &g)) w = g.advance;
            if (x + w > XY_LCD_WIDTH) break;
            x = (uint16_t)(x + w);
            s = p;
        }
        skip--;
    }

    /* Draw up to 2 lines */
    uint8_t drawn = 0u;
    while (*s && drawn < 2u) {
        const uint8_t *line_start = s;
        uint16_t line_px = 0u;
        const uint8_t *line_end = s;
        while (*s) {
            const uint8_t *p = s;
            uint16_t c = xy_gdi_utf8_next(&p);
            xy_glyph_t g;
            uint8_t w = 0u;
            if (xy_gdi_get_glyph(c, true, &g)) w = g.advance;
            if (line_px + w > XY_LCD_WIDTH) break;
            line_px = (uint16_t)(line_px + w);
            line_end = p;
            s = p;
        }
        (void)line_end;
        xy_gdi_text_out(XY_PSYSDC, 0u, y, line_start, true);
        y = (uint8_t)(y + 14u);  /* 14px line height */
        drawn++;
    }

    /* Up arrow if scrolled down */
    if (s_info_first > 0) {
        xy_gdi_hline(XY_PSYSDC, 63u, 1u, 2u, 1u);
        xy_gdi_hline(XY_PSYSDC, 62u, 2u, 4u, 1u);
        xy_gdi_hline(XY_PSYSDC, 61u, 3u, 6u, 1u);
    }
    /* Down arrow if more content below */
    if (s_info_first + 2 < (int8_t)s_info_lines) {
        uint8_t ay = (uint8_t)(XY_LCD_HEIGHT - 2u);
        xy_gdi_hline(XY_PSYSDC, 63u, ay,              2u, 1u);
        xy_gdi_hline(XY_PSYSDC, 62u, (uint8_t)(ay-1), 4u, 1u);
        xy_gdi_hline(XY_PSYSDC, 61u, (uint8_t)(ay-2), 6u, 1u);
    }
}

/* ── Slide animation helpers ─────────────────────────────────────────── */

static void _slide_anim(uint8_t direction)
{
    /* Scratch frame was pre-rendered into g_xy_lcd_buf + XY_LCD_BUFSIZE.
     * Slide 8 steps: shift 2 bytes per step on a 16-byte-wide framebuffer
     * (row-major, W=128, each row = 16 bytes). */
    uint8_t *cur  = g_xy_lcd_buf;
    uint8_t *next = g_xy_lcd_buf + XY_LCD_BUFSIZE;
    uint8_t  row_bytes = (uint8_t)(XY_LCD_WIDTH / 8u);

    for (uint8_t step = 0u; step < (uint8_t)(row_bytes / 2u); step++) {
        if (direction == _GUI_SCROLL_R2L) {
            /* Shift current frame left by 2, pull in from next */
            for (uint8_t row = 0u; row < XY_LCD_HEIGHT; row++) {
                uint8_t *d = cur  + (unsigned)row * row_bytes;
                uint8_t *s = next + (unsigned)row * row_bytes;
                for (uint8_t b = 0u; b < (uint8_t)(row_bytes - 2u); b++)
                    d[b] = d[b + 2u];
                d[row_bytes - 2u] = s[step * 2u];
                d[row_bytes - 1u] = s[step * 2u + 1u];
            }
        } else {
            /* Shift current frame right by 2, pull in from next */
            uint8_t src_off = (uint8_t)((row_bytes / 2u - 1u - step) * 2u);
            for (uint8_t row = 0u; row < XY_LCD_HEIGHT; row++) {
                uint8_t *d = cur  + (unsigned)row * row_bytes;
                uint8_t *s = next + (unsigned)row * row_bytes;
                for (int8_t b = (int8_t)(row_bytes - 1); b >= 2; b--)
                    d[b] = d[b - 2];
                d[0] = s[src_off];
                d[1] = s[src_off + 1u];
            }
        }
        xy_lcd_flush();
    }

    /* Copy scratch to current to finish */
    memcpy(cur, next, XY_LCD_BUFSIZE);
    xy_lcd_flush();
}

/* ── Main draw function ──────────────────────────────────────────────── */

static void _list_draw(bool animate, uint8_t scroll_dir)
{
    xy_dc_t *draw_target = XY_PSYSDC;

    if (animate && scroll_dir != _GUI_SCROLL_DIRECT) {
        /* Draw into scratch buffer, then animate */
        draw_target = XY_PSYSDC;  /* we'll swap after */
        xy_dc_t scratch;
        xy_gdi_init_dc(&scratch, XY_LCD_WIDTH, XY_LCD_HEIGHT,
                       g_xy_lcd_buf + XY_LCD_BUFSIZE);
        /* Render new frame into scratch */
        memset(g_xy_lcd_buf + XY_LCD_BUFSIZE, 0, XY_LCD_BUFSIZE);
        xy_dc_t saved = g_xy_dc;
        g_xy_dc = scratch;

        if (g_xy_gui_list->items && g_xy_gui_list->state) {
            int8_t  a = g_xy_gui_list->state->first;
            int8_t  b = _next_idx(a, g_xy_gui_list->count);
            int8_t  sel = g_xy_gui_list->state->now;
            uint8_t xa = g_xy_gui_list->get ? (*g_xy_gui_list->get)((uint8_t)a) : 0u;
            uint8_t xb = g_xy_gui_list->get ? (*g_xy_gui_list->get)((uint8_t)b) : 0u;
            _draw_row(_ROW0_Y, g_xy_gui_list->items[a].str_id, xa);
            _draw_row(_ROW1_Y, g_xy_gui_list->items[b].str_id, xb);
            _highlight_row((sel == a) ? _ROW0_Y : _ROW1_Y);
        } else if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
            _draw_info();
        }

        g_xy_dc = saved;
        _slide_anim(scroll_dir);
        return;
    }

    /* Direct draw */
    xy_gdi_clear();

    if (g_xy_gui_list->items && g_xy_gui_list->state) {
        int8_t  a   = g_xy_gui_list->state->first;
        int8_t  b   = _next_idx(a, g_xy_gui_list->count);
        int8_t  sel = g_xy_gui_list->state->now;
        uint8_t xa  = g_xy_gui_list->get ? (*g_xy_gui_list->get)((uint8_t)a) : 0u;
        uint8_t xb  = g_xy_gui_list->get ? (*g_xy_gui_list->get)((uint8_t)b) : 0u;
        _draw_row(_ROW0_Y, g_xy_gui_list->items[a].str_id, xa);
        _draw_row(_ROW1_Y, g_xy_gui_list->items[b].str_id, xb);
        _highlight_row((sel == a) ? _ROW0_Y : _ROW1_Y);
    } else if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
        _draw_info();
    }

    xy_lcd_flush();
}

/* ── List init ───────────────────────────────────────────────────────── */

static void _list_init(void)
{
    xy_gdi_clear();
    xy_lcd_on_off(true);

    /* Radio: ensure selection is visible */
    if ((g_xy_gui_list->flags & XY_GUI_F_RADIO) &&
        g_xy_gui_list->items && g_xy_gui_list->state && g_xy_gui_list->get) {
        int8_t i;
        for (i = 0; i < g_xy_gui_list->count; i++) {
            if ((*g_xy_gui_list->get)((uint8_t)i) == 1u) break;
        }
        if (i == g_xy_gui_list->count) i = 0;
        if (i != g_xy_gui_list->state->first &&
            i != g_xy_gui_list->state->first + 1) {
            g_xy_gui_list->state->first = (i > 0) ? (int8_t)(i - 1) : 0;
            g_xy_gui_list->state->now   = i;
        }
    }

    /* Info mode: count lines */
    if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
        s_info_first = 0;
        s_info_lines = (int8_t)_info_count_lines(g_xy_gui_list->str_id);
        if (g_xy_gui_list->set) (*g_xy_gui_list->set)(0u);
    }

    _list_draw(false, _GUI_SCROLL_DIRECT);
}

/* ── Key handlers ────────────────────────────────────────────────────── */

static void _on_up(void)
{
    if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
        if (g_xy_gui_list->get) {
            (*g_xy_gui_list->get)(0u);
        } else if (s_info_first > 0) {
            s_info_first -= 2;
            if (s_info_first < 0) s_info_first = 0;
            _list_draw(false, _GUI_SCROLL_DIRECT);
        }
        return;
    }
    if (!g_xy_gui_list->state || g_xy_gui_list->count <= 1) return;

    int8_t x = g_xy_gui_list->state->now;
    if (g_xy_gui_list->flags & XY_GUI_F_NOWRAP) {
        if (x <= 0) return;
        x--;
    } else {
        x--;
        if (x < 0) x = (int8_t)(g_xy_gui_list->count - 1);
    }
    g_xy_gui_list->state->now = x;
    /* Keep 'first' in sync so selected item is always visible */
    if (x != g_xy_gui_list->state->first)
        g_xy_gui_list->state->first = x;
    if (g_xy_gui_list->count == 2) g_xy_gui_list->state->first = 0;

    _list_draw(true, _GUI_SCROLL_L2R);
}

static void _on_down(void)
{
    if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
        if (g_xy_gui_list->get) {
            (*g_xy_gui_list->get)(1u);
        } else if (s_info_first + 2 < (int8_t)s_info_lines) {
            s_info_first += 2;
            _list_draw(false, _GUI_SCROLL_DIRECT);
        }
        return;
    }
    if (!g_xy_gui_list->state || g_xy_gui_list->count <= 1) return;

    int8_t x = g_xy_gui_list->state->now;
    if (x != g_xy_gui_list->state->first)
        g_xy_gui_list->state->first = x;
    if (g_xy_gui_list->count == 2) g_xy_gui_list->state->first = 0;

    if (g_xy_gui_list->flags & XY_GUI_F_NOWRAP) {
        if (x + 1 >= g_xy_gui_list->count) return;
        x++;
    } else {
        x++;
        if (x >= g_xy_gui_list->count) x = 0;
    }
    g_xy_gui_list->state->now = x;

    _list_draw(true, _GUI_SCROLL_R2L);
}

static void _on_back(void)
{
    if (!g_xy_gui_list->back || g_xy_gui_list->back == g_xy_gui_list) return;
    /* Reset selection state before leaving */
    if (g_xy_gui_list->state) {
        g_xy_gui_list->state->first = 0;
        g_xy_gui_list->state->now   = 0;
    }
    xy_gui_list_t *target = g_xy_gui_list->back;
    xy_gui_switch(target);
}

static void _navigate_to(xy_gui_list_t *target)
{
    if (!target) return;
    xy_gui_switch(target);
}

static void _on_ok(void)
{
    if (g_xy_gui_list->flags & XY_GUI_F_INFO) {
        if (g_xy_gui_list->next) {
            if (!g_xy_gui_list->set ||
                (*g_xy_gui_list->set)(1u)) {
                _navigate_to(g_xy_gui_list->next);
            }
        }
        return;
    }

    /* Let the set callback veto navigation */
    if (g_xy_gui_list->set && g_xy_gui_list->state) {
        if (!(*g_xy_gui_list->set)((uint8_t)g_xy_gui_list->state->now)) {
            _list_draw(false, _GUI_SCROLL_DIRECT);
            return;
        }
    }

    /* Navigate to the item's target or list-wide next */
    xy_gui_list_t *tgt = NULL;
    if (g_xy_gui_list->items && g_xy_gui_list->state) {
        tgt = g_xy_gui_list->items[g_xy_gui_list->state->now].next_on_ok;
    } else {
        tgt = g_xy_gui_list->next;
    }
    if (tgt) _navigate_to(tgt);
}

/* ── Public proc ─────────────────────────────────────────────────────── */

bool xy_gui_proc_list(xy_gui_msg_t *msg)
{
    if (!msg) return false;
    switch (msg->ev) {
    case XY_GUI_EV_INIT:
        _list_init();
        return true;
    case XY_GUI_EV_EXIT:
        return true;
    case XY_GUI_EV_KEY:
        switch (msg->key) {
        case XY_GUI_KEY_UP:   _on_up();   return true;
        case XY_GUI_KEY_DOWN: _on_down(); return true;
        case XY_GUI_KEY_OK:   _on_ok();   return true;
        case XY_GUI_KEY_BACK: _on_back(); return true;
        default: break;
        }
        break;
    default:
        break;
    }
    return false;
}
