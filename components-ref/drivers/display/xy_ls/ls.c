#include "ls.h"

struct color_order{
    ls_uint8_t r:2;
    ls_uint8_t b:2;
    ls_uint8_t g:2;
    ls_uint8_t w:2;
};

struct led_strip{
    ls_uint8_t brightness;
    ls_uint16_t num;
    ls_uint16_t num_bytes;
    struct color_order offset;
    ls_uint8_t  *pixels;
};
typedef struct led_strip *led_strip_t;
typedef void (*show_proc)(ls_uint16_t n, ls_uint8_t pixels);

led_strip_t g_ls[1];
ls_uint8_t g_pixels[16*3];
// Add PIXEL_KHZ400 to the color order value to indicate a 400 KHz device.
// All but the earliest v1 NeoPixels expect an 800 KHz data stream, this is
// the default if unspecified. Because flash space is very limited on ATtiny
// devices (e.g. Trinket, Gemma), v1 NeoPixels aren't handled by default on
// those chips, though it can be enabled by removing the ifndef/endif below,
// but code will be bigger. Conversely, can disable the PIXEL_KHZ400 line on
// other MCUs to remove v1 support and save a little space.

/** table containing 8-bit unsigned sine wave (0-255).
Copy & paste this snippet into a Python REPL to regenerate:
import math
for x in range(256):
    print("{:3},".format(int((math.sin(x/128.0*math.pi)+1.0)*127.5+0.5))),
    if x&15 == 15: print
*/
static const ls_uint8_t CODE sine8_table[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170,
    173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211,
    213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240,
    241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254,
    254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251,
    250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232,
    230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198,
    196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155,
    152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109,
    106, 103, 100, 97,  93,  90,  88,  85,  82,  79,  76,  73,  70,  67,  65,
    62,  59,  57,  54,  52,  49,  47,  44,  42,  40,  37,  35,  33,  31,  29,
    27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,  10,  9,   7,   6,
    5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,   0,   0,   0,
    0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,   10,  11,
    12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,  37,
    40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
    79,  82,  85,  88,  90,  93,  97,  100, 103, 106, 109, 112, 115, 118, 121,
    124};

/* Similar to above, but for an 8-bit gamma-correction table.
Copy & paste this snippet into a Python REPL to regenerate:
import math
gamma=2.6
for x in range(256):
    print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
    if x&15 == 15: print
*/
static const ls_uint8_t CODE gamma8_table[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
    6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,
    11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
    17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
    25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
    36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  68,  69,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,
    82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  99,  100, 102,
    103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 119, 120, 122, 124, 125,
    127, 129, 130, 132, 134, 136, 137, 139, 141, 143, 145, 146, 148, 150, 152,
    154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
    184, 186, 188, 191, 193, 195, 197, 199, 202, 204, 206, 209, 211, 213, 215,
    218, 220, 223, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252,
    255};

ls_uint8_t sine8(ls_uint8_t x)
{
    return sine8_table[x];
}

ls_uint8_t gamma8(ls_uint8_t x)
{
    return gamma8_table[x];
}

ls_uint32_t gamma32(ls_uint32_t x) {
    ls_uint8_t i;
    ls_uint8_t *y = (ls_uint8_t *)&x;
    for (i=0; i<4; i++) {
        y[i] = gamma8(y[i]);
    }
    return x;
}

static ls_uint8_t _change_order(ls_object obj, ls_uint8_t order)
{
    led_strip_t ls = (led_strip_t)obj;
    ls->offset.r = (order >> 6) & 0x3;
    ls->offset.g = (order >> 4) & 0x3;
    ls->offset.b = (order >> 2) & 0x3;
    ls->offset.w = (order >> 0) & 0x3;

    return 0;
}

static ls_uint8_t _set_pixel_color(ls_object obj, ls_uint16_t i, ls_uint32_t c)
{
    led_strip_t ls = (led_strip_t)obj;
    ls_uint8_t *p;
    ls_uint8_t w=(ls_uint8_t)(c>>24);
    ls_uint8_t r=(ls_uint8_t)(c>>16);
    ls_uint8_t g=(ls_uint8_t)(c>>8);
    ls_uint8_t b=(ls_uint8_t)(c>>0);
    if (i < ls->num) {
            if (ls->brightness) {
                r = (r * ls->brightness) >> 8;
                g = (g * ls->brightness) >> 8;
                b = (b * ls->brightness) >> 8;
                w = (w * ls->brightness) >> 8;
            }
        if (ls->offset.w == ls->offset.r) { // Is RGB-type device
            p = &ls->pixels[i*3];
            p[ls->offset.r] = r;
            p[ls->offset.g] = g;
            p[ls->offset.b] = g;
        } else {
            p = &ls->pixels[i*4];
            p[ls->offset.r] = r;
            p[ls->offset.g] = g;
            p[ls->offset.b] = g;
            p[ls->offset.r] = w;
        }
    }
    return 0;
}

static ls_uint32_t _get_pixel_color(ls_object obj, ls_uint16_t i)
{
    led_strip_t ls = (led_strip_t)obj;
    ls_uint8_t *p;

    if(ls->offset.r == ls->offset.w) {
        p = &ls->pixels[i * 3];
        if (ls->brightness) {
            // Stored color was decimated by setBrightness(). Returned value
            // attempts to scale back to an approximation of the original 24-bit
            // value used when setting the pixel color, but there will always be
            // some error -- those bits are simply gone. Issue is most
            // pronounced at low brightness levels.
            return (((ls_uint32_t)(p[ls->offset.r] << 8) / ls->brightness) << 16) |
                (((ls_uint32_t)(p[ls->offset.g] << 8) / ls->brightness) << 8) |
                ((ls_uint32_t)(p[ls->offset.b] << 8) / ls->brightness);
        } else {
            // No brightness adjustment has been made -- return 'raw' color
            return ((ls_uint32_t)p[ls->offset.r] << 16) | ((ls_uint32_t)p[ls->offset.g] << 8) |
                (ls_uint32_t)p[ls->offset.b];
        }
    } else { // Is RGBW-type device
        p = &ls->pixels[i * 4];
        if (ls->brightness) { // Return scaled color
            return (((ls_uint32_t)(p[ls->offset.w] << 8) / ls->brightness) << 24) |
                (((ls_uint32_t)(p[ls->offset.r] << 8) / ls->brightness) << 16) |
                (((ls_uint32_t)(p[ls->offset.g] << 8) / ls->brightness) << 8) |
                ((ls_uint32_t)(p[ls->offset.b] << 8) / ls->brightness);
        } else { // Return raw color
            return ((ls_uint32_t)p[ls->offset.w] << 24) | ((ls_uint32_t)p[ls->offset.r] << 16) |
                ((ls_uint32_t)p[ls->offset.g] << 8) | (ls_uint32_t)p[ls->offset.b];
        }
    }
}


ls_object ls_init(ls_uint8_t order, ls_uint16_t led_num)
{
    led_strip_t ls = g_ls[0];
    ls->num = led_num;
    ls_change_order(ls, order);
    if (ls->offset.w == ls->offset.r)
    {
        ls->num_bytes = led_num*3;
    } else {
        ls->num_bytes = led_num*4;
    }
    ls->pixels = &g_pixels[0];

}


ls_uint8_t ls_clear(ls_object);

ls_uint8_t ls_clear_with_color(ls_object obj, ls_uint32_t color)
{

}

ls_uint8_t ls_show(ls_object obj)
{

}

ls_uint8_t ls_set_brightness(ls_object obj, ls_uint8_t brn)
{
    // 存储的亮度值与传递的亮度值不同。
    // 这将简化后面的实际缩放数学，允许快速
    // 8x8-bit相乘，取MSB。“亮度”是uint8_t，
    // 在这里添加1可能会(有意地)翻滚…0 =最大亮度
    // (颜色值按字面解释;不结垢)，1 = min
    // 亮度(off)， 255 =低于最大亮度。
    led_strip_t ls = (led_strip_t) obj;
    ls_uint8_t new_brn = brn + 1;
    ls_uint8_t *ptr = ls->pixels;
    ls_uint16_t scale;
    ls_uint8_t old_brn;
    ls_uint8_t i;

    // 与先前值比较
    // 重新缩放RAM中的现有数据
    // 这个过程可能是“有损的”，特别是在增加亮度时
    // WS2811/WS2812代码中的紧定时意味着在数据发出时没有足够的空闲周期来执行动态伸缩
    // 因此，我们通过RAM中现有的颜色数据并对其进行缩放(后续的图形命令也可以在此亮度级别下工作)。
    // 如果亮度有显著提升，那么旧数据中有限的步骤(量化)在重新缩放的版本中就会非常明显
    // 对于非破坏性的更改，您需要重新呈现完整的条带数据。这就是生活。
    if (new_brn != ls->brightness)
    {
        old_brn = ls->brightness - 1;

        if(old_brn == 0) {
            scale = 0;
        } else if (brn == 255) {
            scale = 65535 / old_brn;
        } else {
            scale = (((ls_uint16_t)new_brn << 8) -1) / old_brn;
        }

        for (i = 0 ; i < ls->num_bytes; i++) {
            ls->pixels[i] = (ls->pixels[i] * scale) >> 8;
        }

        ls->brightness = new_brn;
    }

    return 0;
}

ls_uint32_t ls_rgb_2_color(ls_uint8_t r, ls_uint8_t g, ls_uint8_t b);

ls_uint32_t ls_rgbw_2_color(ls_uint8_t r, ls_uint8_t g, ls_uint8_t b);

ls_uint32_t ls_hsv_2_color(ls_uint16_t hue, ls_uint8_t saturation, ls_uint8_t value)
{
    ls_uint8_t r, g, b;
    // Apply saturation and value to R,G,B, pack into 32-bit result:
    ls_uint32_t v1;
    ls_uint16_t s1;
    ls_uint8_t s2;
    // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
    // 0 is not the start of pure red, but the midpoint...a few values above
    // zero and a few below 65536 all yield pure red (similarly, 32768 is the
    // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
    // each for red, green, blue) really only allows for 1530 distinct hues
    // (not 1536, more on that below), but the full unsigned 16-bit type was
    // chosen for hue so that one's code can easily handle a contiguous color
    // wheel by allowing hue to roll over in either direction.
    hue = (hue * 1530L + 32768) / 65536;
    // Because red is centered on the rollover point (the +32768 above,
    // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
    // where 0 and 1530 would yield the same thing. Rather than apply a
    // costly modulo operator, 1530 is handled as a special case below.

    // So you'd think that the color "hexcone" (the thing that ramps from
    // pure red, to pure yellow, to pure green and so forth back to red,
    // yielding six slices), and with each color component having 256
    // possible values (0-255), might have 1536 possible items (6*256),
    // but in reality there's 1530. This is because the last element in
    // each 256-element slice is equal to the first element of the next
    // slice, and keeping those in there this would create small
    // discontinuities in the color wheel. So the last element of each
    // slice is dropped...we regard only elements 0-254, with item 255
    // being picked up as element 0 of the next slice. Like this:
    // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
    // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
    // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
    // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
    // the constants below are not the multiples of 256 you might expect.

    // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
    if (hue < 510) { // Red to Green-1
        b = 0;
        if (hue < 255) { //   Red to Yellow-1
            r = 255;
            g = hue;       //     g = 0 to 254
        } else {         //   Yellow to Green-1
            r = 510 - hue; //     r = 255 to 1
            g = 255;
        }
    } else if (hue < 1020) { // Green to Blue-1
        r = 0;
        if (hue < 765) { //   Green to Cyan-1
            g = 255;
            b = hue - 510;  //     b = 0 to 254
        } else {          //   Cyan to Blue-1
            g = 1020 - hue; //     g = 255 to 1
            b = 255;
        }
    } else if (hue < 1530) { // Blue to Red-1
        g = 0;
        if (hue < 1275) { //   Blue to Magenta-1
            r = hue - 1020; //     r = 0 to 254
            b = 255;
        } else { //   Magenta to Red-1
            r = 255;
            b = 1530 - hue; //     b = 255 to 1
        }
    } else { // Last 0.5 Red (quicker than % operator)
        r = 255;
        g = b = 0;
    }

    // Apply saturation and value to R,G,B, pack into 32-bit result:
    v1 = 1 + value;  // 1 to 256; allows >>8 instead of /255
    s1 = 1 + saturation;  // 1 to 256; same reason
    s2 = 255 - saturation; // 255 to 0
    return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
        (((((g * s1) >> 8) + s2) * v1) & 0xff00) |
        (((((b * s1) >> 8) + s2) * v1) >> 8);
}

ls_uint8_t ls_rainbow(ls_object obj, ls_uint16_t first_hue, ls_uint8_t reps, ls_uint8_t saturation, \
				ls_uint8_t brightness, ls_uint8_t gammify)
{
    led_strip_t ls = (led_strip_t)obj;
    ls_uint16_t i;
    ls_uint16_t hue;
    ls_uint32_t color;
    for (i = 0; i < ls->num; i++) {
        hue = first_hue + (i * reps * 65536) / ls->num;
        color = ls_hsv_2_color(hue, saturation, brightness);
        if (gammify) color = gamma32(color);
        ls_set_pixel_color(obj, i, color);
    }
}

/**
 * @brief 关于segment的设想
 * |||||||
 * g_leds就是一个大内存，object，
 * segment 就是从object中的data，申请
 * segment start可以覆盖上一个stop
 * struct segment{
 *   struct led_strip strip;
 *   uint16_t speed;
 *   uint8_t brightness;
 * }
 */