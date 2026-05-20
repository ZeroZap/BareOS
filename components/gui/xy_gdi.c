/**
 * @file xy_gdi.c
 * @brief Bare-metal GDI implementation.
 *
 * Ported from Kohler JV545 T_Gdi.c.  Supports both row-major (default) and
 * UC1701/SSD1306 page layout (XY_GDI_SCAN_HV).
 */

#include "xy_gdi.h"
#include "xy_string.h"

/* ── Global state ───────────────────────────────────────────────────── */

uint8_t g_xy_lcd_buf[XY_LCD_BUFSIZE * 2u];
xy_dc_t g_xy_dc;

static const uint8_t *s_font_large;
static const uint8_t *s_font_small;

/* ── Init ───────────────────────────────────────────────────────────── */

void xy_gdi_set_font(const uint8_t *font_large, const uint8_t *font_small)
{
    s_font_large = font_large;
    s_font_small = font_small ? font_small : font_large;
}

void xy_gdi_init(void)
{
    g_xy_dc.width  = XY_LCD_WIDTH;
    g_xy_dc.height = XY_LCD_HEIGHT;
    g_xy_dc.buf    = g_xy_lcd_buf;
    memset(g_xy_lcd_buf, 0, sizeof(g_xy_lcd_buf));
    xy_lcd_hw_init();
}

void xy_gdi_init_dc(xy_dc_t *dc, uint8_t w, uint8_t h, uint8_t *buf)
{
    dc->width  = w;
    dc->height = h;
    dc->buf    = buf;
}

void xy_gdi_clear(void)
{
    memset(g_xy_dc.buf, 0, XY_LCD_BUFSIZE);
}

/* ── Pixel helpers ──────────────────────────────────────────────────── */

static uint8_t *_pixel_ptr(const xy_dc_t *dc, uint8_t x, uint8_t y,
                            uint8_t *mask_out)
{
#ifdef XY_GDI_SCAN_HV
    *mask_out = (uint8_t)(0x80u >> (y & 7u));
    return &dc->buf[((unsigned)(y >> 3u)) * dc->width + x];
#else
    *mask_out = (uint8_t)(0x80u >> (x & 7u));
    return &dc->buf[((unsigned)y) * (dc->width / 8u) + (x / 8u)];
#endif
}

void xy_gdi_set_pixel(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t c)
{
    if (!dc || x >= dc->width || y >= dc->height) return;
    uint8_t m;
    uint8_t *p = _pixel_ptr(dc, x, y, &m);
    if      (c == 1u) *p |= m;
    else if (c == 0u) *p &= (uint8_t)~m;
    else              *p  = (uint8_t)((~*p & m) | (*p & ~m));
}

uint8_t xy_gdi_get_pixel(const xy_dc_t *dc, uint8_t x, uint8_t y)
{
    if (!dc || x >= dc->width || y >= dc->height) return 0u;
    uint8_t m;
    const uint8_t *p = _pixel_ptr((xy_dc_t *)(uintptr_t)dc, x, y, &m);
    return (*p & m) ? 1u : 0u;
}

/* ── Lines ──────────────────────────────────────────────────────────── */

void xy_gdi_hline(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t w, uint8_t c)
{
    for (uint8_t i = 0u; i < w; i++)
        xy_gdi_set_pixel(dc, (uint8_t)(x + i), y, c);
}

void xy_gdi_vline(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t h, uint8_t c)
{
    for (uint8_t i = 0u; i < h; i++)
        xy_gdi_set_pixel(dc, x, (uint8_t)(y + i), c);
}

void xy_gdi_fill_rect(xy_dc_t *dc, uint8_t x, uint8_t y,
                      uint8_t w, uint8_t h, uint8_t c)
{
    for (uint8_t j = 0u; j < h; j++)
        xy_gdi_hline(dc, x, (uint8_t)(y + j), w, c);
}

void xy_gdi_rect(xy_dc_t *dc, uint8_t x, uint8_t y,
                 uint8_t w, uint8_t h, uint8_t c)
{
    if (w < 2u || h < 2u) { xy_gdi_fill_rect(dc, x, y, w, h, c); return; }
    xy_gdi_hline(dc, x, y,                         w,       c);
    xy_gdi_hline(dc, x, (uint8_t)(y + h - 1u),     w,       c);
    xy_gdi_vline(dc, x,               (uint8_t)(y + 1u), (uint8_t)(h - 2u), c);
    xy_gdi_vline(dc, (uint8_t)(x + w - 1u), (uint8_t)(y + 1u), (uint8_t)(h - 2u), c);
}

/* ── Blit ───────────────────────────────────────────────────────────── */

void xy_gdi_bitblt(xy_dc_t *dst, uint8_t dx, uint8_t dy,
                   uint8_t w, uint8_t h,
                   const xy_dc_t *src, uint8_t sx, uint8_t sy)
{
    for (uint8_t j = 0u; j < h; j++)
        for (uint8_t i = 0u; i < w; i++)
            xy_gdi_set_pixel(dst, (uint8_t)(dx + i), (uint8_t)(dy + j),
                             xy_gdi_get_pixel(src, (uint8_t)(sx + i),
                                                   (uint8_t)(sy + j)));
}

/* ── Glyph rendering ────────────────────────────────────────────────── */

void xy_gdi_show_char(xy_dc_t *dc, uint8_t x, uint8_t y,
                      uint8_t px_w, uint8_t px_h, const uint8_t *bitmap)
{
    uint8_t bit  = 0u;
    uint8_t byte = *bitmap;
    for (uint8_t row = 0u; row < px_h; row++) {
        for (uint8_t col = 0u; col < px_w; col++) {
            if ((byte >> (7u - bit)) & 1u)
                xy_gdi_set_pixel(dc, (uint8_t)(x + col), (uint8_t)(y + row), 1u);
            if (++bit >= 8u) { bit = 0u; byte = *++bitmap; }
        }
    }
}

/* ── Font binary-search (Kohler format) ─────────────────────────────── */

bool xy_gdi_get_glyph(uint16_t c, bool large, xy_glyph_t *out)
{
    const uint8_t *font = large ? s_font_large : s_font_small;
    if (!font || !out) return false;

    uint16_t count = (uint16_t)(font[16u] | ((uint16_t)font[17u] << 8u));
    if (count == 0u) return false;

    uint16_t lo = 0u, hi = (uint16_t)(count - 1u);
    while (lo <= hi) {
        uint16_t       mid   = (uint16_t)((lo + hi) / 2u);
        const uint8_t *entry = &font[64u + (unsigned)mid * 4u];
        uint16_t       off   = (uint16_t)(entry[0] | ((uint16_t)entry[1] << 8u));
        uint8_t        adv   = (uint8_t)(entry[3] >> 2u);
        const uint8_t *gd    = &font[off];
        uint16_t       code  = (uint16_t)(gd[0] | ((uint16_t)gd[1] << 8u));

        if (code == c) {
            out->bearing_x = gd[2];
            out->bearing_y = gd[3];
            out->px_w      = gd[4];
            out->px_h      = gd[5];
            out->advance   = adv;
            out->bitmap    = &gd[6];
            return true;
        }
        if (c < code) {
            if (mid == 0u) break;
            hi = (uint16_t)(mid - 1u);
        } else {
            lo = (uint16_t)(mid + 1u);
        }
    }
    return false;
}

/* ── UTF-8 decoder ──────────────────────────────────────────────────── */

uint16_t xy_gdi_utf8_next(const uint8_t **s)
{
    uint8_t b = **s;
    if (b <= 0x7fu) { (*s)++; return (uint16_t)b; }
    if (b <= 0xdfu) {
        uint16_t c = (uint16_t)(((uint16_t)(b & 0x1fu) << 6u) |
                                 ((*s)[1] & 0x3fu));
        *s += 2; return c;
    }
    uint16_t c = (uint16_t)(((uint16_t)(b & 0x0fu) << 12u) |
                             ((uint16_t)((*s)[1] & 0x3fu) << 6u) |
                              ((*s)[2] & 0x3fu));
    *s += 3; return c;
}

/* ── Text output ────────────────────────────────────────────────────── */

uint8_t xy_gdi_char_out(xy_dc_t *dc, uint8_t x, uint8_t y,
                        uint16_t c, bool large)
{
    xy_glyph_t g;
    if (!xy_gdi_get_glyph(c, large, &g) && !xy_gdi_get_glyph('?', large, &g))
        return 0u;
    if (dc)
        xy_gdi_show_char(dc, (uint8_t)(x + g.bearing_x),
                             (uint8_t)(y + g.bearing_y),
                             g.px_w, g.px_h, g.bitmap);
    return g.advance;
}

uint8_t xy_gdi_text_out(xy_dc_t *dc, uint8_t x, uint8_t y,
                        const uint8_t *s, bool large)
{
    uint8_t total = 0u;
    while (*s) {
        uint16_t c = xy_gdi_utf8_next(&s);
        uint8_t  w = xy_gdi_char_out(dc, (uint8_t)(x + total), y, c, large);
        if (w == 0u) break;
        total = (uint8_t)(total + w);
        if (dc && (x + total) >= dc->width) break;
    }
    return total;
}

/* ── BSP weak stubs ─────────────────────────────────────────────────── */

__attribute__((weak)) void xy_lcd_hw_init(void)    {}
__attribute__((weak)) void xy_lcd_flush(void)      {}
__attribute__((weak)) void xy_lcd_on_off(bool on)  { (void)on; }
