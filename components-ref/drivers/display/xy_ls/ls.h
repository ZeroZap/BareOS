#ifndef _LS_H_
#define _LS_H_
#include "ls_common.h"


/*********Common Setting************/
#define DEFAULT_BRIGHTNESS (ls_uint8_t)50
#define DEFAULT_EFFECT      (ls_uint8_t)0 //static
#define DEFAULT_SPEED       (ls_uint16_t)1000
#define DEFAULT_COLOR       (ls_uint32_t)0xFF0000
#define DEFAULT_ORDER       {RED, GREEN, BLUE}

typedef void *ls_object;

 // The order of primary colors in the NeoPixel data stream can vary among
 // device types, manufacturers and even different revisions of the same
 // item.  The third parameter to the Adafruit_NeoPixel constructor encodes
 // the per-pixel byte offsets of the red, green and blue primaries (plus
 // white, if present) in the data stream -- the following #defines provide
 // an easier-to-use named version for each permutation. e.g. PIXEL_GRB
 // indicates a NeoPixel-compatible device expecting three bytes per pixel,
 // with the first byte transmitted containing the green value, second
 // containing red and third containing blue. The in-memory representation
 // of a chain of NeoPixels is the same as the data-stream order; no
 // re-ordering of bytes is required when issuing data to the chain.
 // Most of these values won't exist in real-world devices, but it's done
 // this way so we're ready for it (also, if using the WS2811 driver IC,
 // one might have their pixels set up in any weird permutation).

 // Bits 5,4 of this value are the offset (0-3) from the first byte of a
 // pixel to the location of the red color byte.  Bits 3,2 are the green
 // offset and 1,0 are the blue offset.  If it is an RGBW-type device
 // (supporting a white primary in addition to R,G,B), bits 7,6 are the
 // offset to the white byte...otherwise, bits 7,6 are set to the same value
 // as 5,4 (red) to indicate an RGB (not RGBW) device.
 // i.e. binary representation:
 // 0bWWRRGGBB for RGBW devices
 // 0bRRRRGGBB for RGB

 // RGB NeoPixel permutations; white and red offsets are always same
 // Offset:         W        R        G        B
#define PIXEL_RGB ((0 << 6) | (0 << 4) | (1 << 2) | (2)) ///< Transmit as R,G,B
#define PIXEL_RBG ((0 << 6) | (0 << 4) | (2 << 2) | (1)) ///< Transmit as R,B,G
#define PIXEL_GRB ((1 << 6) | (1 << 4) | (0 << 2) | (2)) ///< Transmit as G,R,B
#define PIXEL_GBR ((2 << 6) | (2 << 4) | (0 << 2) | (1)) ///< Transmit as G,B,R
#define PIXEL_BRG ((1 << 6) | (1 << 4) | (2 << 2) | (0)) ///< Transmit as B,R,G
#define PIXEL_BGR ((2 << 6) | (2 << 4) | (1 << 2) | (0)) ///< Transmit as B,G,R

// RGBW NeoPixel permutations; all 4 offsets are distinct
// Offset:         W          R          G          B
#define PIXEL_WRGB ((0 << 6) | (1 << 4) | (2 << 2) | (3)) ///< Transmit as W,R,G,B
#define PIXEL_WRBG ((0 << 6) | (1 << 4) | (3 << 2) | (2)) ///< Transmit as W,R,B,G
#define PIXEL_WGRB ((0 << 6) | (2 << 4) | (1 << 2) | (3)) ///< Transmit as W,G,R,B
#define PIXEL_WGBR ((0 << 6) | (3 << 4) | (1 << 2) | (2)) ///< Transmit as W,G,B,R
#define PIXEL_WBRG ((0 << 6) | (2 << 4) | (3 << 2) | (1)) ///< Transmit as W,B,R,G
#define PIXEL_WBGR ((0 << 6) | (3 << 4) | (2 << 2) | (1)) ///< Transmit as W,B,G,R

#define PIXEL_RWGB ((1 << 6) | (0 << 4) | (2 << 2) | (3)) ///< Transmit as R,W,G,B
#define PIXEL_RWBG ((1 << 6) | (0 << 4) | (3 << 2) | (2)) ///< Transmit as R,W,B,G
#define PIXEL_RGWB ((2 << 6) | (0 << 4) | (1 << 2) | (3)) ///< Transmit as R,G,W,B
#define PIXEL_RGBW ((3 << 6) | (0 << 4) | (1 << 2) | (2)) ///< Transmit as R,G,B,W
#define PIXEL_RBWG ((2 << 6) | (0 << 4) | (3 << 2) | (1)) ///< Transmit as R,B,W,G
#define PIXEL_RBGW ((3 << 6) | (0 << 4) | (2 << 2) | (1)) ///< Transmit as R,B,G,W

#define PIXEL_GWRB ((1 << 6) | (2 << 4) | (0 << 2) | (3)) ///< Transmit as G,W,R,B
#define PIXEL_GWBR ((1 << 6) | (3 << 4) | (0 << 2) | (2)) ///< Transmit as G,W,B,R
#define PIXEL_GRWB ((2 << 6) | (1 << 4) | (0 << 2) | (3)) ///< Transmit as G,R,W,B
#define PIXEL_GRBW ((3 << 6) | (1 << 4) | (0 << 2) | (2)) ///< Transmit as G,R,B,W
#define PIXEL_GBWR ((2 << 6) | (3 << 4) | (0 << 2) | (1)) ///< Transmit as G,B,W,R
#define PIXEL_GBRW ((3 << 6) | (2 << 4) | (0 << 2) | (1)) ///< Transmit as G,B,R,W

#define PIXEL_BWRG ((1 << 6) | (2 << 4) | (3 << 2) | (0)) ///< Transmit as B,W,R,G
#define PIXEL_BWGR ((1 << 6) | (3 << 4) | (2 << 2) | (0)) ///< Transmit as B,W,G,R
#define PIXEL_BRWG ((2 << 6) | (1 << 4) | (3 << 2) | (0)) ///< Transmit as B,R,W,G
#define PIXEL_BRGW ((3 << 6) | (1 << 4) | (2 << 2) | (0)) ///< Transmit as B,R,G,W
#define PIXEL_BGWR ((2 << 6) | (3 << 4) | (1 << 2) | (0)) ///< Transmit as B,G,W,R
#define PIXEL_BGRW ((3 << 6) | (2 << 4) | (1 << 2) | (0)) ///< Transmit as B,G,R,W


ls_object ls_init(ls_uint8_t order, ls_uint16_t led_num);

ls_uint8_t ls_delete(ls_object obj);

ls_uint8_t ls_update(ls_object obj);

ls_uint32_t ls_rbg_2_color(ls_uint8_t r, ls_uint8_t g, ls_uint8_t b);

ls_uint32_t ls_rgbw_2_color(ls_uint8_t r, ls_uint8_t g, ls_uint8_t b, ls_uint8_t w);

// hue:色调，saturation:饱和度，value:明度(brightness) 设置
ls_uint32_t ls_hsv_2_color(ls_uint16_t hue, ls_uint8_t saturation, ls_uint8_t value);

ls_uint8_t ls_set_pixel_color(ls_object obj, ls_uint16_t i, ls_uint32_t c);
ls_uint32_t ls_get_pixel_color(ls_object obj, ls_uint16_t i);

// for invert ?
ls_uint8_t ls_set_pixel_brightness(ls_object obj, ls_uint16_t i, ls_uint8_t brn);
// there is no need for pixel brightness value save
// ls_uint8_t ls_get_pixel_brightness(ls_object obj, ls_uint16_t i);
ls_uint8_t ls_set_brightness(ls_object obj, ls_uint8_t brn);
ls_uint8_t ls_get_brightness(ls_object obj); // obj->brightness-1

ls_uint8_t ls_fill_color(ls_object obj, ls_uint32_t c, ls_uint16_t index, ls_uint16_t cnt);
ls_uint8_t ls_clear(ls_object obj, ls_uint32_t c); //memst = 0

ls_uint8_t ls_change_num(ls_object obj, ls_uint16_t num);
ls_uint16_t ls_get_num(ls_object obj);
ls_uint8_t ls_change_order(ls_object obj, ls_uint8_t order);
ls_uint8_t ls_get_order(ls_object obj);

// 判断时间
// return now - pre > 300

// ls_uint32_t gama32(ls_uint32_t x);

ls_uint8_t ls_rainbow(ls_object obj, ls_uint16_t first_hue, ls_uint8_t reps, ls_uint8_t saturation, \
				ls_uint8_t brightness, ls_uint8_t gammify);
#endif

