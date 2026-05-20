/**
 * @file xy_gdi.h
 * @brief Bare-metal Graphics Device Interface.
 *
 * Ported from the Kohler JV545 T_Gdi layer.  All PIC18-specific qualifiers
 * removed; standard C99 types used throughout.
 *
 * Framebuffer layout (default, row-major):
 *   buf[y * (W/8) + x/8],  bit = (0x80 >> (x & 7))
 *
 * Define XY_GDI_SCAN_HV for UC1701 / SSD1306 page layout:
 *   buf[(y/8) * W + x],    bit = (0x80 >> (y & 7))
 *
 * Usage:
 *   // BSP provides xy_lcd_hw_init / xy_lcd_flush / xy_lcd_on_off
 *   xy_gdi_set_font(my_font14, my_font12);
 *   xy_gdi_init();
 *   xy_gdi_clear();
 *   xy_gdi_text_out(XY_PSYSDC, 0, 0, (const uint8_t *)"Hello", true);
 *   xy_lcd_flush();
 *
 * Font binary format (compatible with JV545 font files):
 *   Offset  Size  Field
 *    0..15   16   Metadata (reserved)
 *   16..17    2   Character count (uint16 LE)
 *   18..63   46   Reserved
 *   64 + i*4  4   Index entry i (sorted by codepoint):
 *                   [0..1] glyph_data_offset (uint16 LE, from font base)
 *                   [2]    reserved
 *                   [3]    advance_px << 2
 *   At glyph_data_offset:
 *     [0..1]  Unicode codepoint (uint16 LE)
 *     [2]     bearing_x
 *     [3]     bearing_y
 *     [4]     glyph pixel width
 *     [5]     glyph pixel height
 *     [6+]    1-bit bitmap, row-major, MSB-first
 */

#ifndef XY_GDI_H
#define XY_GDI_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── LCD geometry (BSP may override via compiler flags) ──────────────── */

#ifndef XY_LCD_WIDTH
#define XY_LCD_WIDTH    128u
#endif
#ifndef XY_LCD_HEIGHT
#define XY_LCD_HEIGHT    64u
#endif

#define XY_LCD_BUFSIZE  ((XY_LCD_WIDTH * XY_LCD_HEIGHT) / 8u)

/* ── Device context ─────────────────────────────────────────────────── */

typedef struct {
    uint8_t  width;
    uint8_t  height;
    uint8_t *buf;
} xy_dc_t;

/* ── Glyph descriptor ───────────────────────────────────────────────── */

typedef struct {
    const uint8_t *bitmap;   /* 1-bit row-major, MSB-first pixel data */
    uint8_t        bearing_x;
    uint8_t        bearing_y;
    uint8_t        px_w;     /* glyph pixel width  */
    uint8_t        px_h;     /* glyph pixel height */
    uint8_t        advance;  /* total horizontal advance in px */
} xy_glyph_t;

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Set active font data.  Call before any text functions.
 * @param font_large  14px font binary blob (Kohler format).
 * @param font_small  12px font blob, or NULL to alias large font.
 */
void xy_gdi_set_font(const uint8_t *font_large, const uint8_t *font_small);

/** Initialise GDI — binds system DC to g_xy_lcd_buf and calls xy_lcd_hw_init(). */
void xy_gdi_init(void);

/** Initialise a device context pointing at an existing byte array. */
void xy_gdi_init_dc(xy_dc_t *dc, uint8_t w, uint8_t h, uint8_t *buf);

/** Zero the system framebuffer (g_xy_dc only). */
void xy_gdi_clear(void);

/** Set/clear/invert one pixel.  c: 0=off 1=on 2=invert. */
void xy_gdi_set_pixel(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t c);

/** Read one pixel (0 or 1). */
uint8_t xy_gdi_get_pixel(const xy_dc_t *dc, uint8_t x, uint8_t y);

/** Horizontal line, w pixels wide.  c: 0/1/2. */
void xy_gdi_hline(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t w, uint8_t c);

/** Vertical line, h pixels tall.  c: 0/1/2. */
void xy_gdi_vline(xy_dc_t *dc, uint8_t x, uint8_t y, uint8_t h, uint8_t c);

/** Solid filled rectangle.  c: 0/1/2. */
void xy_gdi_fill_rect(xy_dc_t *dc, uint8_t x, uint8_t y,
                      uint8_t w, uint8_t h, uint8_t c);

/** Hollow rectangle (border only).  c: 0/1/2. */
void xy_gdi_rect(xy_dc_t *dc, uint8_t x, uint8_t y,
                 uint8_t w, uint8_t h, uint8_t c);

/** Block-transfer region from src to dst device context. */
void xy_gdi_bitblt(xy_dc_t *dst, uint8_t dx, uint8_t dy,
                   uint8_t w, uint8_t h,
                   const xy_dc_t *src, uint8_t sx, uint8_t sy);

/** Render a 1-bit packed glyph bitmap (row-major, MSB-first). */
void xy_gdi_show_char(xy_dc_t *dc, uint8_t x, uint8_t y,
                      uint8_t px_w, uint8_t px_h, const uint8_t *bitmap);

/**
 * Look up a glyph by Unicode codepoint (binary search).
 * Falls back to '?' if the codepoint is not found.
 * @return true if found (out is filled), false if '?' also not found.
 */
bool xy_gdi_get_glyph(uint16_t c, bool large, xy_glyph_t *out);

/**
 * Decode one UTF-8 character from *s and advance the pointer.
 * Handles 1-, 2-, and 3-byte sequences.
 */
uint16_t xy_gdi_utf8_next(const uint8_t **s);

/** Render one character; return pixel advance (0 on error). */
uint8_t xy_gdi_char_out(xy_dc_t *dc, uint8_t x, uint8_t y,
                        uint16_t c, bool large);

/**
 * Render a NUL-terminated UTF-8 string.
 * Pass dc=NULL to measure width without drawing.
 * Rendering clips at dc->width.
 * @return Total pixel width of the string.
 */
uint8_t xy_gdi_text_out(xy_dc_t *dc, uint8_t x, uint8_t y,
                        const uint8_t *s, bool large);

/* ── System framebuffer + DC ────────────────────────────────────────── */

/**
 * Two-frame buffer:
 *   [0 .. BUFSIZE-1]         current display frame
 *   [BUFSIZE .. 2*BUFSIZE-1] off-screen / animation scratch
 */
extern uint8_t  g_xy_lcd_buf[XY_LCD_BUFSIZE * 2u];
extern xy_dc_t  g_xy_dc;

#define XY_PSYSDC  (&g_xy_dc)

/* ── BSP hooks (implement in xy_lcd_port.c) ─────────────────────────── */

/** Hardware display initialisation — called once by xy_gdi_init(). */
void xy_lcd_hw_init(void);

/** Flush g_xy_lcd_buf[0..BUFSIZE-1] to the physical display. */
void xy_lcd_flush(void);

/** Turn the display on (true) or off (false). */
void xy_lcd_on_off(bool on);

#ifdef __cplusplus
}
#endif

#endif /* XY_GDI_H */
