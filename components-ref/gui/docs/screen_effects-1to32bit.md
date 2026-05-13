## 完整点阵屏特效库（多色彩深度版）

### 1. 核心框架（基础结构与像素操作）

```c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*===========================================================================
 * 色彩格式定义
 *===========================================================================*/
typedef enum {
    COLOR_FORMAT_1BPP,      // 1位单色
    COLOR_FORMAT_2BPP,      // 2位灰度 (4级)
    COLOR_FORMAT_4BPP,      // 4位灰度/索引色 (16色)
    COLOR_FORMAT_8BPP,      // 8位灰度/索引色 (256色)
    COLOR_FORMAT_RGB565,    // 16位 RGB (5-6-5)
    COLOR_FORMAT_RGB888,    // 24位 RGB
    COLOR_FORMAT_ARGB8888,  // 32位 ARGB
    COLOR_FORMAT_RGBA8888,  // 32位 RGBA
} ColorFormat;

/*===========================================================================
 * 颜色结构（统一内部表示）
 *===========================================================================*/
typedef struct {
    uint8_t a, r, g, b;
} Color;

// 常用颜色预定义
#define COLOR_BLACK     (Color){255, 0, 0, 0}
#define COLOR_WHITE     (Color){255, 255, 255, 255}
#define COLOR_RED       (Color){255, 255, 0, 0}
#define COLOR_GREEN     (Color){255, 0, 255, 0}
#define COLOR_BLUE      (Color){255, 0, 0, 255}
#define COLOR_YELLOW    (Color){255, 255, 255, 0}
#define COLOR_CYAN      (Color){255, 0, 255, 255}
#define COLOR_MAGENTA   (Color){255, 255, 0, 255}
#define COLOR_TRANSPARENT (Color){0, 0, 0, 0}

/*===========================================================================
 * 点阵矩阵结构
 *===========================================================================*/
typedef struct {
    void *data;             // 像素数据
    int width;              // 宽度
    int height;             // 高度
    int stride;             // 每行字节数
    ColorFormat format;     // 色彩格式
    uint32_t *palette;      // 调色板
    int paletteSize;        // 调色板大小
} Matrix;

/*===========================================================================
 * 基础工具函数
 *===========================================================================*/

// 获取每像素位数
static inline int getBitsPerPixel(ColorFormat format) {
    static const int bppTable[] = {1, 2, 4, 8, 16, 24, 32, 32};
    return bppTable[format];
}

// 计算每行字节数
static inline int calcStride(int width, ColorFormat format, int alignment) {
    int bpp = getBitsPerPixel(format);
    int bytes = (width * bpp + 7) / 8;
    if (alignment > 1) {
        bytes = (bytes + alignment - 1) / alignment * alignment;
    }
    return bytes;
}

// 钳制值到范围
static inline int clamp(int value, int min, int max) {
    return (value < min) ? min : (value > max) ? max : value;
}

static inline float clampf(float value, float min, float max) {
    return (value < min) ? min : (value > max) ? max : value;
}

/*===========================================================================
 * 矩阵创建与销毁
 *===========================================================================*/

Matrix* createMatrix(int width, int height, ColorFormat format, int alignment) {
    Matrix *m = (Matrix*)malloc(sizeof(Matrix));
    if (!m) return NULL;
    
    m->width = width;
    m->height = height;
    m->format = format;
    m->stride = calcStride(width, format, alignment > 0 ? alignment : 1);
    m->data = calloc(m->stride * height, 1);
    m->palette = NULL;
    m->paletteSize = 0;
    
    if (!m->data) {
        free(m);
        return NULL;
    }
    return m;
}

void freeMatrix(Matrix *m) {
    if (m) {
        free(m->data);
        free(m->palette);
        free(m);
    }
}

Matrix* cloneMatrix(Matrix *src) {
    Matrix *dst = createMatrix(src->width, src->height, src->format, 1);
    if (!dst) return NULL;
    
    dst->stride = src->stride;
    memcpy(dst->data, src->data, src->stride * src->height);
    
    if (src->palette && src->paletteSize > 0) {
        dst->palette = (uint32_t*)malloc(src->paletteSize * sizeof(uint32_t));
        memcpy(dst->palette, src->palette, src->paletteSize * sizeof(uint32_t));
        dst->paletteSize = src->paletteSize;
    }
    return dst;
}

/*===========================================================================
 * 像素读写（原始值）
 *===========================================================================*/

uint32_t getPixelRaw(Matrix *m, int x, int y) {
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) return 0;
    
    uint8_t *row = (uint8_t*)m->data + y * m->stride;
    
    switch (m->format) {
        case COLOR_FORMAT_1BPP: {
            int byteIdx = x >> 3;
            int bitIdx = 7 - (x & 7);
            return (row[byteIdx] >> bitIdx) & 0x01;
        }
        case COLOR_FORMAT_2BPP: {
            int byteIdx = x >> 2;
            int bitIdx = (3 - (x & 3)) << 1;
            return (row[byteIdx] >> bitIdx) & 0x03;
        }
        case COLOR_FORMAT_4BPP: {
            int byteIdx = x >> 1;
            int bitIdx = (1 - (x & 1)) << 2;
            return (row[byteIdx] >> bitIdx) & 0x0F;
        }
        case COLOR_FORMAT_8BPP:
            return row[x];
        case COLOR_FORMAT_RGB565:
            return ((uint16_t*)row)[x];
        case COLOR_FORMAT_RGB888: {
            uint8_t *p = row + x * 3;
            return (p[0] << 16) | (p[1] << 8) | p[2];
        }
        case COLOR_FORMAT_ARGB8888:
        case COLOR_FORMAT_RGBA8888:
            return ((uint32_t*)row)[x];
        default:
            return 0;
    }
}

void setPixelRaw(Matrix *m, int x, int y, uint32_t value) {
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) return;
    
    uint8_t *row = (uint8_t*)m->data + y * m->stride;
    
    switch (m->format) {
        case COLOR_FORMAT_1BPP: {
            int byteIdx = x >> 3;
            int bitIdx = 7 - (x & 7);
            if (value)
                row[byteIdx] |= (1 << bitIdx);
            else
                row[byteIdx] &= ~(1 << bitIdx);
            break;
        }
        case COLOR_FORMAT_2BPP: {
            int byteIdx = x >> 2;
            int bitIdx = (3 - (x & 3)) << 1;
            row[byteIdx] = (row[byteIdx] & ~(0x03 << bitIdx)) | ((value & 0x03) << bitIdx);
            break;
        }
        case COLOR_FORMAT_4BPP: {
            int byteIdx = x >> 1;
            int bitIdx = (1 - (x & 1)) << 2;
            row[byteIdx] = (row[byteIdx] & ~(0x0F << bitIdx)) | ((value & 0x0F) << bitIdx);
            break;
        }
        case COLOR_FORMAT_8BPP:
            row[x] = value & 0xFF;
            break;
        case COLOR_FORMAT_RGB565:
            ((uint16_t*)row)[x] = value & 0xFFFF;
            break;
        case COLOR_FORMAT_RGB888: {
            uint8_t *p = row + x * 3;
            p[0] = (value >> 16) & 0xFF;
            p[1] = (value >> 8) & 0xFF;
            p[2] = value & 0xFF;
            break;
        }
        case COLOR_FORMAT_ARGB8888:
        case COLOR_FORMAT_RGBA8888:
            ((uint32_t*)row)[x] = value;
            break;
    }
}

/*===========================================================================
 * 颜色转换
 *===========================================================================*/

// 查找最近调色板索引
static int findNearestPaletteIndex(Matrix *m, Color c) {
    if (!m->palette || m->paletteSize == 0) return 0;
    
    int bestIdx = 0;
    int bestDist = 0x7FFFFFFF;
    
    for (int i = 0; i < m->paletteSize; i++) {
        uint32_t p = m->palette[i];
        int dr = c.r - ((p >> 16) & 0xFF);
        int dg = c.g - ((p >> 8) & 0xFF);
        int db = c.b - (p & 0xFF);
        int dist = dr * dr + dg * dg + db * db;
        
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
            if (dist == 0) break;
        }
    }
    return bestIdx;
}

// 原始值转Color
Color rawToColor(uint32_t raw, Matrix *m) {
    Color c = {255, 0, 0, 0};
    
    switch (m->format) {
        case COLOR_FORMAT_1BPP:
            c.r = c.g = c.b = raw ? 255 : 0;
            break;
        case COLOR_FORMAT_2BPP:
            c.r = c.g = c.b = raw * 85;
            break;
        case COLOR_FORMAT_4BPP:
            if (m->palette && raw < (uint32_t)m->paletteSize) {
                uint32_t p = m->palette[raw];
                c.a = (p >> 24) & 0xFF;
                c.r = (p >> 16) & 0xFF;
                c.g = (p >> 8) & 0xFF;
                c.b = p & 0xFF;
            } else {
                c.r = c.g = c.b = raw * 17;
            }
            break;
        case COLOR_FORMAT_8BPP:
            if (m->palette && raw < (uint32_t)m->paletteSize) {
                uint32_t p = m->palette[raw];
                c.a = (p >> 24) & 0xFF;
                c.r = (p >> 16) & 0xFF;
                c.g = (p >> 8) & 0xFF;
                c.b = p & 0xFF;
            } else {
                c.r = c.g = c.b = raw;
            }
            break;
        case COLOR_FORMAT_RGB565:
            c.r = ((raw >> 11) & 0x1F) << 3;
            c.g = ((raw >> 5) & 0x3F) << 2;
            c.b = (raw & 0x1F) << 3;
            break;
        case COLOR_FORMAT_RGB888:
            c.r = (raw >> 16) & 0xFF;
            c.g = (raw >> 8) & 0xFF;
            c.b = raw & 0xFF;
            break;
        case COLOR_FORMAT_ARGB8888:
            c.a = (raw >> 24) & 0xFF;
            c.r = (raw >> 16) & 0xFF;
            c.g = (raw >> 8) & 0xFF;
            c.b = raw & 0xFF;
            break;
        case COLOR_FORMAT_RGBA8888:
            c.r = (raw >> 24) & 0xFF;
            c.g = (raw >> 16) & 0xFF;
            c.b = (raw >> 8) & 0xFF;
            c.a = raw & 0xFF;
            break;
    }
    return c;
}

// Color转原始值
uint32_t colorToRaw(Color c, Matrix *m) {
    int gray;
    switch (m->format) {
        case COLOR_FORMAT_1BPP:
            return ((c.r * 299 + c.g * 587 + c.b * 114) / 1000) > 127 ? 1 : 0;
        case COLOR_FORMAT_2BPP:
            gray = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
            return gray >> 6;
        case COLOR_FORMAT_4BPP:
            if (m->palette) return findNearestPaletteIndex(m, c);
            gray = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
            return gray >> 4;
        case COLOR_FORMAT_8BPP:
            if (m->palette) return findNearestPaletteIndex(m, c);
            return (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
        case COLOR_FORMAT_RGB565:
            return ((c.r >> 3) << 11) | ((c.g >> 2) << 5) | (c.b >> 3);
        case COLOR_FORMAT_RGB888:
            return (c.r << 16) | (c.g << 8) | c.b;
        case COLOR_FORMAT_ARGB8888:
            return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
        case COLOR_FORMAT_RGBA8888:
            return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
        default:
            return 0;
    }
}

/*===========================================================================
 * 高级像素操作
 *===========================================================================*/

Color getPixel(Matrix *m, int x, int y) {
    return rawToColor(getPixelRaw(m, x, y), m);
}

void setPixel(Matrix *m, int x, int y, Color c) {
    setPixelRaw(m, x, y, colorToRaw(c, m));
}

// 带边界检查的安全获取（超出返回指定颜色）
Color getPixelSafe(Matrix *m, int x, int y, Color defaultColor) {
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) {
        return defaultColor;
    }
    return getPixel(m, x, y);
}

// 双线性插值获取
Color getPixelBilinear(Matrix *m, float x, float y) {
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    float fx = x - x0;
    float fy = y - y0;
    
    Color c00 = getPixelSafe(m, x0, y0, COLOR_BLACK);
    Color c10 = getPixelSafe(m, x0 + 1, y0, COLOR_BLACK);
    Color c01 = getPixelSafe(m, x0, y0 + 1, COLOR_BLACK);
    Color c11 = getPixelSafe(m, x0 + 1, y0 + 1, COLOR_BLACK);
    
    float w00 = (1 - fx) * (1 - fy);
    float w10 = fx * (1 - fy);
    float w01 = (1 - fx) * fy;
    float w11 = fx * fy;
    
    Color result;
    result.a = (uint8_t)(c00.a * w00 + c10.a * w10 + c01.a * w01 + c11.a * w11);
    result.r = (uint8_t)(c00.r * w00 + c10.r * w10 + c01.r * w01 + c11.r * w11);
    result.g = (uint8_t)(c00.g * w00 + c10.g * w10 + c01.g * w01 + c11.g * w11);
    result.b = (uint8_t)(c00.b * w00 + c10.b * w10 + c01.b * w01 + c11.b * w11);
    
    return result;
}

/*===========================================================================
 * 颜色工具函数
 *===========================================================================*/

// 创建颜色
static inline Color makeColor(uint8_t r, uint8_t g, uint8_t b) {
    return (Color){255, r, g, b};
}

static inline Color makeColorA(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (Color){a, r, g, b};
}

// 颜色混合
Color blendColor(Color src, Color dst, uint8_t alpha) {
    Color result;
    result.a = 255;
    result.r = (src.r * alpha + dst.r * (255 - alpha)) / 255;
    result.g = (src.g * alpha + dst.g * (255 - alpha)) / 255;
    result.b = (src.b * alpha + dst.b * (255 - alpha)) / 255;
    return result;
}

// Alpha混合
Color blendColorAlpha(Color src, Color dst) {
    if (src.a == 255) return src;
    if (src.a == 0) return dst;
    
    Color result;
    int alpha = src.a;
    result.a = 255;
    result.r = (src.r * alpha + dst.r * (255 - alpha)) / 255;
    result.g = (src.g * alpha + dst.g * (255 - alpha)) / 255;
    result.b = (src.b * alpha + dst.b * (255 - alpha)) / 255;
    return result;
}

// 亮度调整
Color adjustBrightness(Color c, float factor) {
    return (Color){
        c.a,
        clamp((int)(c.r * factor), 0, 255),
        clamp((int)(c.g * factor), 0, 255),
        clamp((int)(c.b * factor), 0, 255)
    };
}

// 转灰度
Color toGrayscale(Color c) {
    uint8_t gray = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
    return (Color){c.a, gray, gray, gray};
}

// 反色
Color invertColor(Color c) {
    return (Color){c.a, 255 - c.r, 255 - c.g, 255 - c.b};
}

// 颜色插值
Color lerpColor(Color a, Color b, float t) {
    t = clampf(t, 0.0f, 1.0f);
    return (Color){
        (uint8_t)(a.a + (b.a - a.a) * t),
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t)
    };
}

/*===========================================================================
 * 矩阵基础操作
 *===========================================================================*/

void clearMatrix(Matrix *m, Color c) {
    for (int y = 0; y < m->height; y++) {
        for (int x = 0; x < m->width; x++) {
            setPixel(m, x, y, c);
        }
    }
}

void copyMatrix(Matrix *src, Matrix *dst) {
    int minW = (src->width < dst->width) ? src->width : dst->width;
    int minH = (src->height < dst->height) ? src->height : dst->height;
    
    for (int y = 0; y < minH; y++) {
        for (int x = 0; x < minW; x++) {
            setPixel(dst, x, y, getPixel(src, x, y));
        }
    }
}

void fillRect(Matrix *m, int x, int y, int w, int h, Color c) {
    for (int j = y; j < y + h && j < m->height; j++) {
        for (int i = x; i < x + w && i < m->width; i++) {
            if (i >= 0 && j >= 0) {
                setPixel(m, i, j, c);
            }
        }
    }
}
```

------

### 2. 基础变换特效

```c
/*===========================================================================
 * 移动/滚动效果
 *===========================================================================*/

typedef enum {
    SCROLL_LEFT,
    SCROLL_RIGHT,
    SCROLL_UP,
    SCROLL_DOWN
} ScrollDirection;

void scrollEffect(Matrix *src, Matrix *dst, ScrollDirection dir, int offset, int wrap) {
    int ox = 0, oy = 0;
    
    switch (dir) {
        case SCROLL_LEFT:  ox = offset;  break;
        case SCROLL_RIGHT: ox = -offset; break;
        case SCROLL_UP:    oy = offset;  break;
        case SCROLL_DOWN:  oy = -offset; break;
    }
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            int srcX = x + ox;
            int srcY = y + oy;
            
            if (wrap) {
                srcX = ((srcX % src->width) + src->width) % src->width;
                srcY = ((srcY % src->height) + src->height) % src->height;
                setPixel(dst, x, y, getPixel(src, srcX, srcY));
            } else {
                setPixel(dst, x, y, getPixelSafe(src, srcX, srcY, COLOR_BLACK));
            }
        }
    }
}

// 对角线滚动
void scrollDiagonal(Matrix *src, Matrix *dst, int offsetX, int offsetY, int wrap) {
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            int srcX = x + offsetX;
            int srcY = y + offsetY;
            
            if (wrap) {
                srcX = ((srcX % src->width) + src->width) % src->width;
                srcY = ((srcY % src->height) + src->height) % src->height;
                setPixel(dst, x, y, getPixel(src, srcX, srcY));
            } else {
                setPixel(dst, x, y, getPixelSafe(src, srcX, srcY, COLOR_BLACK));
            }
        }
    }
}

/*===========================================================================
 * 翻转效果
 *===========================================================================*/

void flipHorizontal(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, src->width - 1 - x, y, getPixel(src, x, y));
        }
    }
}

void flipVertical(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, x, src->height - 1 - y, getPixel(src, x, y));
        }
    }
}

/*===========================================================================
 * 旋转效果
 *===========================================================================*/

// 90度旋转
void rotate90CW(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int newX = src->height - 1 - y;
            int newY = x;
            if (newX < dst->width && newY < dst->height) {
                setPixel(dst, newX, newY, getPixel(src, x, y));
            }
        }
    }
}

void rotate90CCW(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int newX = y;
            int newY = src->width - 1 - x;
            if (newX < dst->width && newY < dst->height) {
                setPixel(dst, newX, newY, getPixel(src, x, y));
            }
        }
    }
}

void rotate180(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, src->width - 1 - x, src->height - 1 - y, getPixel(src, x, y));
        }
    }
}

// 任意角度旋转
void rotateAngle(Matrix *src, Matrix *dst, float angleDeg, int smooth) {
    float angleRad = angleDeg * M_PI / 180.0f;
    float cosA = cosf(angleRad);
    float sinA = sinf(angleRad);
    
    int cx = src->width / 2;
    int cy = src->height / 2;
    int dcx = dst->width / 2;
    int dcy = dst->height / 2;
    
    clearMatrix(dst, COLOR_BLACK);
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            float dx = x - dcx;
            float dy = y - dcy;
            
            float srcX = dx * cosA + dy * sinA + cx;
            float srcY = -dx * sinA + dy * cosA + cy;
            
            if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                if (smooth) {
                    setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
                } else {
                    setPixel(dst, x, y, getPixel(src, (int)srcX, (int)srcY));
                }
            }
        }
    }
}

/*===========================================================================
 * 缩放效果
 *===========================================================================*/

void zoomEffect(Matrix *src, Matrix *dst, float scale, int centerX, int centerY, int smooth) {
    if (scale <= 0) {
        clearMatrix(dst, COLOR_BLACK);
        return;
    }
    
    if (centerX < 0) centerX = src->width / 2;
    if (centerY < 0) centerY = src->height / 2;
    
    int dcx = dst->width / 2;
    int dcy = dst->height / 2;
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            float srcX = (x - dcx) / scale + centerX;
            float srcY = (y - dcy) / scale + centerY;
            
            if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                if (smooth) {
                    setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
                } else {
                    setPixel(dst, x, y, getPixel(src, (int)srcX, (int)srcY));
                }
            } else {
                setPixel(dst, x, y, COLOR_BLACK);
            }
        }
    }
}

// 跳入效果
void zoomIn(Matrix *src, Matrix *dst, float progress, int smooth) {
    float scale = clampf(progress, 0.01f, 1.0f);
    zoomEffect(src, dst, scale, -1, -1, smooth);
}

// 跳出效果
void zoomOut(Matrix *src, Matrix *dst, float progress, int smooth) {
    float scale = clampf(1.0f - progress, 0.01f, 1.0f);
    zoomEffect(src, dst, scale, -1, -1, smooth);
}
```

------

### 3. 过渡特效

```c
/*===========================================================================
 * 淡入淡出效果
 *===========================================================================*/

void fadeIn(Matrix *src, Matrix *dst, float progress) {
    progress = clampf(progress, 0.0f, 1.0f);
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            setPixel(dst, x, y, adjustBrightness(c, progress));
        }
    }
}

void fadeOut(Matrix *src, Matrix *dst, float progress) {
    fadeIn(src, dst, 1.0f - progress);
}

// 交叉淡入淡出
void crossFade(Matrix *src1, Matrix *src2, Matrix *dst, float progress) {
    progress = clampf(progress, 0.0f, 1.0f);
    uint8_t alpha = (uint8_t)(progress * 255);
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            Color c1 = getPixelSafe(src1, x, y, COLOR_BLACK);
            Color c2 = getPixelSafe(src2, x, y, COLOR_BLACK);
            setPixel(dst, x, y, blendColor(c2, c1, alpha));
        }
    }
}

/*===========================================================================
 * 擦除效果
 *===========================================================================*/

typedef enum {
    WIPE_LEFT,
    WIPE_RIGHT,
    WIPE_UP,
    WIPE_DOWN,
    WIPE_FROM_CENTER,
    WIPE_TO_CENTER,
    WIPE_CIRCLE_OUT,
    WIPE_CIRCLE_IN
} WipeType;

void wipeEffect(Matrix *src, Matrix *dst, WipeType type, float progress) {
    progress = clampf(progress, 0.0f, 1.0f);
    int cx = src->width / 2;
    int cy = src->height / 2;
    float maxDist = sqrtf(cx * cx + cy * cy);
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int show = 0;
            
            switch (type) {
                case WIPE_LEFT:
                    show = x < (int)(src->width * progress);
                    break;
                case WIPE_RIGHT:
                    show = x >= (int)(src->width * (1.0f - progress));
                    break;
                case WIPE_UP:
                    show = y < (int)(src->height * progress);
                    break;
                case WIPE_DOWN:
                    show = y >= (int)(src->height * (1.0f - progress));
                    break;
                case WIPE_FROM_CENTER: {
                    int dx = abs(x - cx);
                    int dy = abs(y - cy);
                    show = (dx <= cx * progress) && (dy <= cy * progress);
                    break;
                }
                case WIPE_TO_CENTER: {
                    int dx = abs(x - cx);
                    int dy = abs(y - cy);
                    show = (dx > cx * (1.0f - progress)) || (dy > cy * (1.0f - progress));
                    break;
                }
                case WIPE_CIRCLE_OUT: {
                    float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    show = dist <= maxDist * progress;
                    break;
                }
                case WIPE_CIRCLE_IN: {
                    float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    show = dist >= maxDist * (1.0f - progress);
                    break;
                }
            }
            
            setPixel(dst, x, y, show ? getPixel(src, x, y) : COLOR_BLACK);
        }
    }
}

/*===========================================================================
 * 百叶窗效果
 *===========================================================================*/

void blindsHorizontal(Matrix *src, Matrix *dst, int blindCount, float progress) {
    int blindHeight = src->height / blindCount;
    if (blindHeight < 1) blindHeight = 1;
    int openHeight = (int)(blindHeight * progress);
    
    clearMatrix(dst, COLOR_BLACK);
    
    for (int y = 0; y < src->height; y++) {
        int posInBlind = y % blindHeight;
        if (posInBlind < openHeight) {
            for (int x = 0; x < src->width; x++) {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

void blindsVertical(Matrix *src, Matrix *dst, int blindCount, float progress) {
    int blindWidth = src->width / blindCount;
    if (blindWidth < 1) blindWidth = 1;
    int openWidth = (int)(blindWidth * progress);
    
    clearMatrix(dst, COLOR_BLACK);
    
    for (int x = 0; x < src->width; x++) {
        int posInBlind = x % blindWidth;
        if (posInBlind < openWidth) {
            for (int y = 0; y < src->height; y++) {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

/*===========================================================================
 * 棋盘格效果
 *===========================================================================*/

void checkerboardEffect(Matrix *src, Matrix *dst, int blockSize, float progress) {
    int blocksX = (src->width + blockSize - 1) / blockSize;
    int blocksY = (src->height + blockSize - 1) / blockSize;
    int totalBlocks = blocksX * blocksY;
    int blocksToShow = (int)(totalBlocks * progress);
    
    clearMatrix(dst, COLOR_BLACK);
    
    int count = 0;
    // 先显示偶数格
    for (int by = 0; by < blocksY && count < blocksToShow; by++) {
        for (int bx = 0; bx < blocksX && count < blocksToShow; bx++) {
            if ((bx + by) % 2 == 0) {
                for (int y = by * blockSize; y < (by + 1) * blockSize && y < src->height; y++) {
                    for (int x = bx * blockSize; x < (bx + 1) * blockSize && x < src->width; x++) {
                        setPixel(dst, x, y, getPixel(src, x, y));
                    }
                }
                count++;
            }
        }
    }
    // 再显示奇数格
    for (int by = 0; by < blocksY && count < blocksToShow; by++) {
        for (int bx = 0; bx < blocksX && count < blocksToShow; bx++) {
            if ((bx + by) % 2 == 1) {
                for (int y = by * blockSize; y < (by + 1) * blockSize && y < src->height; y++) {
                    for (int x = bx * blockSize; x < (bx + 1) * blockSize && x < src->width; x++) {
                        setPixel(dst, x, y, getPixel(src, x, y));
                    }
                }
                count++;
            }
        }
    }
}

/*===========================================================================
 * 溶解效果
 *===========================================================================*/

// 有序抖动矩阵
static const uint8_t bayerMatrix8x8[64] = {
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
};

void dissolveOrdered(Matrix *src, Matrix *dst, float progress) {
    int threshold = (int)(progress * 64);
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int ditherValue = bayerMatrix8x8[(y & 7) * 8 + (x & 7)];
            if (ditherValue < threshold) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, COLOR_BLACK);
            }
        }
    }
}

// 随机溶解
void dissolveRandom(Matrix *src, Matrix *dst, float progress, uint32_t seed) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            // 简单哈希生成伪随机
            uint32_t hash = seed ^ (x * 374761393) ^ (y * 668265263);
            hash = (hash ^ (hash >> 13)) * 1274126177;
            float rand = (hash & 0xFFFF) / 65535.0f;
            
            if (rand < progress) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, COLOR_BLACK);
            }
        }
    }
}

/*===========================================================================
 * 螺旋展开效果
 *===========================================================================*/

void spiralReveal(Matrix *src, Matrix *dst, float progress) {
    int totalPixels = src->width * src->height;
    int pixelsToShow = (int)(totalPixels * progress);
    
    clearMatrix(dst, COLOR_BLACK);
    
    int x = 0, y = 0;
    int dx = 1, dy = 0;
    int left = 0, right = src->width - 1;
    int top = 0, bottom = src->height - 1;
    
    for (int i = 0; i < pixelsToShow && i < totalPixels; i++) {
        setPixel(dst, x, y, getPixel(src, x, y));
        
        // 计算下一个位置
        int nextX = x + dx;
        int nextY = y + dy;
        
        if (dx == 1 && nextX > right) {
            dx = 0; dy = 1; top++;
        } else if (dy == 1 && nextY > bottom) {
            dx = -1; dy = 0; right--;
        } else if (dx == -1 && nextX < left) {
            dx = 0; dy = -1; bottom--;
        } else if (dy == -1 && nextY < top) {
            dx = 1; dy = 0; left++;
        }
        
        x += dx;
        y += dy;
    }
}
```

------

### 4. 变形特效

```c
/*===========================================================================
 * 波浪效果
 *===========================================================================*/

void waveHorizontal(Matrix *src, Matrix *dst, float amplitude, float frequency, float phase) {
    for (int y = 0; y < src->height; y++) {
        int offset = (int)(amplitude * sinf(frequency * y + phase));
        for (int x = 0; x < src->width; x++) {
            int srcX = ((x - offset) % src->width + src->width) % src->width;
            setPixel(dst, x, y, getPixel(src, srcX, y));
        }
    }
}

void waveVertical(Matrix *src, Matrix *dst, float amplitude, float frequency, float phase) {
    for (int x = 0; x < src->width; x++) {
        int offset = (int)(amplitude * sinf(frequency * x + phase));
        for (int y = 0; y < src->height; y++) {
            int srcY = ((y - offset) % src->height + src->height) % src->height;
            setPixel(dst, x, y, getPixel(src, x, srcY));
        }
    }
}

// 复合波浪
void waveComplex(Matrix *src, Matrix *dst, float ampX, float ampY, 
                 float freqX, float freqY, float phase) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int offsetX = (int)(ampX * sinf(freqY * y + phase));
            int offsetY = (int)(ampY * sinf(freqX * x + phase));
            
            int srcX = ((x - offsetX) % src->width + src->width) % src->width;
            int srcY = ((y - offsetY) % src->height + src->height) % src->height;
            
            setPixel(dst, x, y, getPixel(src, srcX, srcY));
        }
    }
}

/*===========================================================================
 * 水波纹效果
 *===========================================================================*/

void rippleEffect(Matrix *src, Matrix *dst, float amplitude, float wavelength, 
                  float phase, int centerX, int centerY) {
    if (centerX < 0) centerX = src->width / 2;
    if (centerY < 0) centerY = src->height / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist > 0.001f) {
                float offset = amplitude * sinf(dist / wavelength * 2 * M_PI - phase);
                float srcX = x + offset * dx / dist;
                float srcY = y + offset * dy / dist;
                
                srcX = clampf(srcX, 0, src->width - 1);
                srcY = clampf(srcY, 0, src->height - 1);
                
                setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
            } else {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

/*===========================================================================
 * 漩涡效果
 *===========================================================================*/

void swirlEffect(Matrix *src, Matrix *dst, float strength, float radius, 
                 int centerX, int centerY) {
    if (centerX < 0) centerX = src->width / 2;
    if (centerY < 0) centerY = src->height / 2;
    if (radius <= 0) radius = fminf(src->width, src->height) / 2.0f;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < radius && dist > 0) {
                float factor = 1.0f - dist / radius;
                float angle = strength * factor * factor;
                
                float cosA = cosf(angle);
                float sinA = sinf(angle);
                
                float srcX = cosA * dx - sinA * dy + centerX;
                float srcY = sinA * dx + cosA * dy + centerY;
                
                if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                    setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
                } else {
                    setPixel(dst, x, y, COLOR_BLACK);
                }
            } else {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

/*===========================================================================
 * 鱼眼/球面效果
 *===========================================================================*/

void fisheyeEffect(Matrix *src, Matrix *dst, float strength, int centerX, int centerY) {
    if (centerX < 0) centerX = src->width / 2;
    if (centerY < 0) centerY = src->height / 2;
    
    float radius = fminf(src->width, src->height) / 2.0f;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = sqrtf(dx * dx + dy * dy);
            float normDist = dist / radius;
            
            if (normDist < 1.0f) {
                float newDist = powf(normDist, strength) * radius;
                float srcX = centerX + dx * newDist / dist;
                float srcY = centerY + dy * newDist / dist;
                
                if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                    setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
                } else {
                    setPixel(dst, x, y, COLOR_BLACK);
                }
            } else {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

/*===========================================================================
 * 马赛克效果
 *===========================================================================*/

void mosaicEffect(Matrix *src, Matrix *dst, int blockSize) {
    if (blockSize < 1) blockSize = 1;
    
    for (int by = 0; by < src->height; by += blockSize) {
        for (int bx = 0; bx < src->width; bx += blockSize) {
            // 计算块内平均颜色
            int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int count = 0;
            
            for (int y = by; y < by + blockSize && y < src->height; y++) {
                for (int x = bx; x < bx + blockSize && x < src->width; x++) {
                    Color c = getPixel(src, x, y);
                    sumR += c.r; sumG += c.g; sumB += c.b; sumA += c.a;
                    count++;
                }
            }
            
            Color avg = {sumA / count, sumR / count, sumG / count, sumB / count};
            
            // 填充块
            for (int y = by; y < by + blockSize && y < src->height; y++) {
                for (int x = bx; x < bx + blockSize && x < src->width; x++) {
                    setPixel(dst, x, y, avg);
                }
            }
        }
    }
}

// 动态马赛克（从模糊到清晰）
void mosaicTransition(Matrix *src, Matrix *dst, float progress) {
    int maxBlock = (src->width > src->height ? src->width : src->height) / 4;
    int blockSize = maxBlock - (int)((maxBlock - 1) * progress);
    if (blockSize < 1) blockSize = 1;
    mosaicEffect(src, dst, blockSize);
}

/*===========================================================================
 * 镜像效果
 *===========================================================================*/

void mirrorQuadrant(Matrix *src, Matrix *dst) {
    int hw = src->width / 2;
    int hh = src->height / 2;
    
    for (int y = 0; y < hh; y++) {
        for (int x = 0; x < hw; x++) {
            Color c = getPixel(src, x, y);
            setPixel(dst, x, y, c);
            setPixel(dst, src->width - 1 - x, y, c);
            setPixel(dst, x, src->height - 1 - y, c);
            setPixel(dst, src->width - 1 - x, src->height - 1 - y, c);
        }
    }
}

// 万花筒效果
void kaleidoscopeEffect(Matrix *src, Matrix *dst, int segments) {
    if (segments < 2) segments = 2;
    
    int cx = src->width / 2;
    int cy = src->height / 2;
    float segmentAngle = 2 * M_PI / segments;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrtf(dx * dx + dy * dy);
            float angle = atan2f(dy, dx);
            
            if (angle < 0) angle += 2 * M_PI;
            
            int segment = (int)(angle / segmentAngle);
            float newAngle = fmodf(angle, segmentAngle);
            
            if (segment % 2 == 1) {
                newAngle = segmentAngle - newAngle;
            }
            
            int srcX = (int)(cx + dist * cosf(newAngle));
            int srcY = (int)(cy + dist * sinf(newAngle));
            
            setPixel(dst, x, y, getPixelSafe(src, srcX, srcY, COLOR_BLACK));
        }
    }
}
```

------

### 5. 3D效果

```c
/*===========================================================================
 * 3D翻转效果
 *===========================================================================*/

// 绕Y轴翻转
void flip3D_Y(Matrix *src, Matrix *dst, float angle) {
    float cosA = cosf(angle);
    int cx = src->width / 2;
    
    clearMatrix(dst, COLOR_BLACK);
    
    // 计算透视缩放
    float scale = fabsf(cosA);
    if (scale < 0.01f) return;
    
    int newWidth = (int)(src->width * scale);
    int startX = (dst->width - newWidth) / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int dstX;
            int srcX = x;
            
            if (cosA < 0) {
                srcX = src->width - 1 - x;  // 背面镜像
            }
            
            dstX = startX + (int)(x * scale);
            
            if (dstX >= 0 && dstX < dst->width) {
                setPixel(dst, dstX, y, getPixel(src, srcX, y));
            }
        }
    }
}

// 绕X轴翻转
void flip3D_X(Matrix *src, Matrix *dst, float angle) {
    float cosA = cosf(angle);
    int cy = src->height / 2;
    
    clearMatrix(dst, COLOR_BLACK);
    
    float scale = fabsf(cosA);
    if (scale < 0.01f) return;
    
    int newHeight = (int)(src->height * scale);
    int startY = (dst->height - newHeight) / 2;
    
    for (int y = 0; y < src->height; y++) {
        int dstY;
        int srcY = y;
        
        if (cosA < 0) {
            srcY = src->height - 1 - y;
        }
        
        dstY = startY + (int)(y * scale);
        
        if (dstY >= 0 && dstY < dst->height) {
            for (int x = 0; x < src->width; x++) {
                setPixel(dst, x, dstY, getPixel(src, x, srcY));
            }
        }
    }
}

// 透视变换
void perspectiveEffect(Matrix *src, Matrix *dst, float tiltX, float tiltY) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    clearMatrix(dst, COLOR_BLACK);
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            float dx = x - cx;
            float dy = y - cy;
            
            // 简单透视计算
            float factorX = 1.0f + tiltY * dy / src->height;
            float factorY = 1.0f + tiltX * dx / src->width;
            
            if (factorX > 0 && factorY > 0) {
                float srcX = dx / factorX + cx;
                float srcY = dy / factorY + cy;
                
                if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                    setPixel(dst, x, y, getPixelBilinear(src, srcX, srcY));
                }
            }
        }
    }
}

/*===========================================================================
 * 立方体旋转效果
 *===========================================================================*/

void cubeRotateY(Matrix *front, Matrix *side, Matrix *dst, float angle) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    
    clearMatrix(dst, COLOR_BLACK);
    
    int cx = dst->width / 2;
    int halfW = front->width / 2;
    
    // 根据角度决定显示哪些面
    Matrix *leftFace = (sinA > 0) ? side : front;
    Matrix *rightFace = (sinA > 0) ? front : side;
    
    float leftScale = fabsf(sinA);
    float rightScale = fabsf(cosA);
    
    // 绘制左侧面
    if (leftScale > 0.01f) {
        int leftWidth = (int)(halfW * leftScale);
        for (int y = 0; y < dst->height; y++) {
            for (int i = 0; i < leftWidth; i++) {
                int srcX = (int)(i / leftScale);
                int dstX = cx - leftWidth + i;
                if (dstX >= 0 && srcX < leftFace->width) {
                    setPixel(dst, dstX, y, getPixel(leftFace, leftFace->width - 1 - srcX, y));
                }
            }
        }
    }
    
    // 绘制右侧面
    if (rightScale > 0.01f) {
        int rightWidth = (int)(halfW * rightScale);
        for (int y = 0; y < dst->height; y++) {
            for (int i = 0; i < rightWidth; i++) {
                int srcX = (int)(i / rightScale);
                int dstX = cx + i;
                if (dstX < dst->width && srcX < rightFace->width) {
                    setPixel(dst, dstX, y, getPixel(rightFace, srcX, y));
                }
            }
        }
    }
}
```

------

### 6. 粒子与动态效果

```c
/*===========================================================================
 * 粒子系统
 *===========================================================================*/

#define MAX_PARTICLES 256

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    Color color;
    float size;
    int active;
} Particle;

typedef struct {
    Particle particles[MAX_PARTICLES];
    int count;
    float gravity;
    float friction;
} ParticleSystem;

void initParticleSystem(ParticleSystem *ps) {
    memset(ps, 0, sizeof(ParticleSystem));
    ps->gravity = 0.1f;
    ps->friction = 0.99f;
}

void emitParticle(ParticleSystem *ps, float x, float y, float vx, float vy, 
                  Color color, float life, float size) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ps->particles[i].active) {
            Particle *p = &ps->particles[i];
            p->x = x;
            p->y = y;
            p->vx = vx;
            p->vy = vy;
            p->color = color;
            p->life = life;
            p->maxLife = life;
            p->size = size;
            p->active = 1;
            ps->count++;
            return;
        }
    }
}

void updateParticleSystem(ParticleSystem *ps, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->vy += ps->gravity * dt;
        p->vx *= ps->friction;
        p->vy *= ps->friction;
        p->life -= dt;
        
        if (p->life <= 0) {
            p->active = 0;
            ps->count--;
        }
    }
}

void renderParticleSystem(ParticleSystem *ps, Matrix *dst) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        float alpha = p->life / p->maxLife;
        Color c = p->color;
        c.r = (uint8_t)(c.r * alpha);
        c.g = (uint8_t)(c.g * alpha);
        c.b = (uint8_t)(c.b * alpha);
        
        int size = (int)p->size;
        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                int px = (int)p->x + dx;
                int py = (int)p->y + dy;
                if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
                    Color existing = getPixel(dst, px, py);
                    setPixel(dst, px, py, blendColorAlpha((Color){(uint8_t)(alpha * 255), c.r, c.g, c.b}, existing));
                }
            }
        }
    }
}

/*===========================================================================
 * 雨滴效果
 *===========================================================================*/

typedef struct {
    float x, y;
    float speed;
    int length;
    Color color;
} RainDrop;

#define MAX_RAINDROPS 100

typedef struct {
    RainDrop drops[MAX_RAINDROPS];
    int width, height;
    float wind;
} RainEffect;

void initRainEffect(RainEffect *rain, int width, int height, Color color) {
    rain->width = width;
    rain->height = height;
    rain->wind = 0;
    
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        rain->drops[i].x = rand() % width;
        rain->drops[i].y = rand() % height;
        rain->drops[i].speed = 2.0f + (rand() % 100) / 50.0f;
        rain->drops[i].length = 3 + rand() % 8;
        rain->drops[i].color = color;
    }
}

void updateRainEffect(RainEffect *rain) {
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        RainDrop *d = &rain->drops[i];
        d->y += d->speed;
        d->x += rain->wind;
        
        // 边界处理
        if (d->y >= rain->height) {
            d->y = -d->length;
            d->x = rand() % rain->width;
        }
        if (d->x < 0) d->x += rain->width;
        if (d->x >= rain->width) d->x -= rain->width;
    }
}

void renderRainEffect(RainEffect *rain, Matrix *dst, int clearBg) {
    if (clearBg) {
        clearMatrix(dst, COLOR_BLACK);
    }
    
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        RainDrop *d = &rain->drops[i];
        
        for (int j = 0; j < d->length; j++) {
            int px = (int)d->x;
            int py = (int)d->y - j;
            
            if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
                // 渐变效果：头部亮，尾部暗
                float brightness = 1.0f - (float)j / d->length;
                Color c = adjustBrightness(d->color, brightness);
                setPixel(dst, px, py, c);
            }
        }
    }
}

/*===========================================================================
 * 雪花效果
 *===========================================================================*/

typedef struct {
    float x, y;
    float speed;
    float size;
    float wobble;      // 左右摆动幅度
    float wobblePhase; // 摆动相位
    Color color;
} Snowflake;

#define MAX_SNOWFLAKES 150

typedef struct {
    Snowflake flakes[MAX_SNOWFLAKES];
    int width, height;
    float windPhase;
} SnowEffect;

void initSnowEffect(SnowEffect *snow, int width, int height) {
    snow->width = width;
    snow->height = height;
    snow->windPhase = 0;
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        Snowflake *f = &snow->flakes[i];
        f->x = rand() % width;
        f->y = rand() % height;
        f->speed = 0.5f + (rand() % 100) / 100.0f;
        f->size = 1.0f + (rand() % 100) / 100.0f;
        f->wobble = 1.0f + (rand() % 100) / 50.0f;
        f->wobblePhase = (rand() % 628) / 100.0f;
        f->color = COLOR_WHITE;
    }
}

void updateSnowEffect(SnowEffect *snow, float dt) {
    snow->windPhase += dt * 0.5f;
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        Snowflake *f = &snow->flakes[i];
        
        f->y += f->speed;
        f->x += sinf(snow->windPhase + f->wobblePhase) * f->wobble * 0.1f;
        
        // 边界处理
        if (f->y >= snow->height) {
            f->y = -f->size;
            f->x = rand() % snow->width;
        }
        if (f->x < 0) f->x += snow->width;
        if (f->x >= snow->width) f->x -= snow->width;
    }
}

void renderSnowEffect(SnowEffect *snow, Matrix *dst, int clearBg) {
    if (clearBg) {
        clearMatrix(dst, COLOR_BLACK);
    }
    
    for (int i = 0; i < MAX_SNOWFLAKES; i++) {
        Snowflake *f = &snow->flakes[i];
        int size = (int)f->size;
        
        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                if (dx * dx + dy * dy <= size * size) {
                    int px = (int)f->x + dx;
                    int py = (int)f->y + dy;
                    
                    if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
                        setPixel(dst, px, py, f->color);
                    }
                }
            }
        }
    }
}

/*===========================================================================
 * 火焰效果
 *===========================================================================*/

typedef struct {
    uint8_t *buffer[2];
    int currentBuffer;
    int width, height;
    uint32_t palette[256];
} FireEffect;

void initFirePalette(FireEffect *fire) {
    for (int i = 0; i < 256; i++) {
        uint8_t r, g, b;
        
        if (i < 64) {
            r = i * 4;
            g = 0;
            b = 0;
        } else if (i < 128) {
            r = 255;
            g = (i - 64) * 4;
            b = 0;
        } else if (i < 192) {
            r = 255;
            g = 255;
            b = (i - 128) * 4;
        } else {
            r = 255;
            g = 255;
            b = 255;
        }
        
        fire->palette[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}

FireEffect* createFireEffect(int width, int height) {
    FireEffect *fire = (FireEffect*)malloc(sizeof(FireEffect));
    if (!fire) return NULL;
    
    fire->width = width;
    fire->height = height;
    fire->currentBuffer = 0;
    fire->buffer[0] = (uint8_t*)calloc(width * height, 1);
    fire->buffer[1] = (uint8_t*)calloc(width * height, 1);
    
    if (!fire->buffer[0] || !fire->buffer[1]) {
        free(fire->buffer[0]);
        free(fire->buffer[1]);
        free(fire);
        return NULL;
    }
    
    initFirePalette(fire);
    return fire;
}

void freeFireEffect(FireEffect *fire) {
    if (fire) {
        free(fire->buffer[0]);
        free(fire->buffer[1]);
        free(fire);
    }
}

void updateFireEffect(FireEffect *fire) {
    uint8_t *src = fire->buffer[fire->currentBuffer];
    uint8_t *dst = fire->buffer[1 - fire->currentBuffer];
    int w = fire->width;
    int h = fire->height;
    
    // 底部随机点火
    for (int x = 0; x < w; x++) {
        src[(h - 1) * w + x] = rand() % 256;
    }
    
    // 火焰向上蔓延
    for (int y = 0; y < h - 1; y++) {
        for (int x = 0; x < w; x++) {
            int sum = 0;
            int count = 0;
            
            // 采样周围像素
            for (int dy = 0; dy <= 2; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = (x + dx + w) % w;
                    int ny = y + dy;
                    if (ny < h) {
                        sum += src[ny * w + nx];
                        count++;
                    }
                }
            }
            
            // 计算新值并衰减
            int newVal = (sum / count) - 2;
            if (newVal < 0) newVal = 0;
            dst[y * w + x] = newVal;
        }
    }
    
    fire->currentBuffer = 1 - fire->currentBuffer;
}

void renderFireEffect(FireEffect *fire, Matrix *dst) {
    uint8_t *buf = fire->buffer[fire->currentBuffer];
    
    for (int y = 0; y < fire->height && y < dst->height; y++) {
        for (int x = 0; x < fire->width && x < dst->width; x++) {
            uint8_t val = buf[y * fire->width + x];
            uint32_t color = fire->palette[val];
            
            Color c = {
                (color >> 24) & 0xFF,
                (color >> 16) & 0xFF,
                (color >> 8) & 0xFF,
                color & 0xFF
            };
            setPixel(dst, x, y, c);
        }
    }
}

/*===========================================================================
 * 矩阵雨效果（黑客帝国风格）
 *===========================================================================*/

typedef struct {
    int x;
    float y;
    float speed;
    int length;
    int charOffset;
} MatrixDrop;

#define MAX_MATRIX_DROPS 64

typedef struct {
    MatrixDrop drops[MAX_MATRIX_DROPS];
    int width, height;
    uint8_t *trailBuffer;  // 拖尾缓冲
} MatrixRainEffect;

MatrixRainEffect* createMatrixRainEffect(int width, int height) {
    MatrixRainEffect *effect = (MatrixRainEffect*)malloc(sizeof(MatrixRainEffect));
    if (!effect) return NULL;
    
    effect->width = width;
    effect->height = height;
    effect->trailBuffer = (uint8_t*)calloc(width * height, 1);
    
    if (!effect->trailBuffer) {
        free(effect);
        return NULL;
    }
    
    // 初始化雨滴
    for (int i = 0; i < MAX_MATRIX_DROPS; i++) {
        MatrixDrop *d = &effect->drops[i];
        d->x = rand() % width;
        d->y = -(rand() % height);
        d->speed = 0.3f + (rand() % 100) / 100.0f;
        d->length = 5 + rand() % 15;
        d->charOffset = rand() % 256;
    }
    
    return effect;
}

void freeMatrixRainEffect(MatrixRainEffect *effect) {
    if (effect) {
        free(effect->trailBuffer);
        free(effect);
    }
}

void updateMatrixRainEffect(MatrixRainEffect *effect) {
    int w = effect->width;
    int h = effect->height;
    
    // 淡化拖尾
    for (int i = 0; i < w * h; i++) {
        if (effect->trailBuffer[i] > 5) {
            effect->trailBuffer[i] -= 5;
        } else {
            effect->trailBuffer[i] = 0;
        }
    }
    
    // 更新雨滴
    for (int i = 0; i < MAX_MATRIX_DROPS; i++) {
        MatrixDrop *d = &effect->drops[i];
        d->y += d->speed;
        d->charOffset++;
        
        // 绘制到拖尾缓冲
        for (int j = 0; j < d->length; j++) {
            int py = (int)d->y - j;
            if (py >= 0 && py < h && d->x >= 0 && d->x < w) {
                int brightness;
                if (j == 0) {
                    brightness = 255;  // 头部最亮
                } else {
                    brightness = 200 - j * (150 / d->length);
                }
                
                int idx = py * w + d->x;
                if (brightness > effect->trailBuffer[idx]) {
                    effect->trailBuffer[idx] = brightness;
                }
            }
        }
        
        // 重置
        if (d->y - d->length > h) {
            d->y = -(rand() % 20);
            d->x = rand() % w;
            d->length = 5 + rand() % 15;
            d->speed = 0.3f + (rand() % 100) / 100.0f;
        }
    }
}

void renderMatrixRainEffect(MatrixRainEffect *effect, Matrix *dst) {
    int w = effect->width;
    int h = effect->height;
    
    for (int y = 0; y < h && y < dst->height; y++) {
        for (int x = 0; x < w && x < dst->width; x++) {
            uint8_t val = effect->trailBuffer[y * w + x];
            Color c;
            
            if (val > 200) {
                // 头部白色
                c = (Color){255, val, 255, val};
            } else {
                // 绿色拖尾
                c = (Color){255, 0, val, 0};
            }
            
            setPixel(dst, x, y, c);
        }
    }
}

/*===========================================================================
 * 星空效果
 *===========================================================================*/

typedef struct {
    float x, y, z;
    Color color;
} Star;

#define MAX_STARS 200

typedef struct {
    Star stars[MAX_STARS];
    int width, height;
    float speed;
    float centerX, centerY;
} StarfieldEffect;

void initStarfieldEffect(StarfieldEffect *sf, int width, int height) {
    sf->width = width;
    sf->height = height;
    sf->speed = 2.0f;
    sf->centerX = width / 2.0f;
    sf->centerY = height / 2.0f;
    
    for (int i = 0; i < MAX_STARS; i++) {
        Star *s = &sf->stars[i];
        s->x = (rand() % width) - sf->centerX;
        s->y = (rand() % height) - sf->centerY;
        s->z = (rand() % 100) / 100.0f * 100 + 1;
        s->color = COLOR_WHITE;
    }
}

void updateStarfieldEffect(StarfieldEffect *sf, float dt) {
    for (int i = 0; i < MAX_STARS; i++) {
        Star *s = &sf->stars[i];
        s->z -= sf->speed * dt * 10;
        
        if (s->z <= 0) {
            s->x = (rand() % sf->width) - sf->centerX;
            s->y = (rand() % sf->height) - sf->centerY;
            s->z = 100;
        }
    }
}

void renderStarfieldEffect(StarfieldEffect *sf, Matrix *dst) {
    clearMatrix(dst, COLOR_BLACK);
    
    for (int i = 0; i < MAX_STARS; i++) {
        Star *s = &sf->stars[i];
        
        // 透视投影
        float factor = 50.0f / s->z;
        int screenX = (int)(s->x * factor + sf->centerX);
        int screenY = (int)(s->y * factor + sf->centerY);
        
        if (screenX >= 0 && screenX < dst->width && 
            screenY >= 0 && screenY < dst->height) {
            
            // 距离越近越亮
            float brightness = 1.0f - s->z / 100.0f;
            Color c = adjustBrightness(s->color, brightness);
            
            // 距离越近点越大
            int size = (int)(3 * (1.0f - s->z / 100.0f));
            if (size < 1) size = 1;
            
            for (int dy = -size; dy <= size; dy++) {
                for (int dx = -size; dx <= size; dx++) {
                    int px = screenX + dx;
                    int py = screenY + dy;
                    if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
                        setPixel(dst, px, py, c);
                    }
                }
            }
        }
    }
}
```

### 7. 滤镜效果

```c
/*===========================================================================
 * 图像滤镜
 *===========================================================================*/

// 灰度滤镜
void filterGrayscale(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, x, y, toGrayscale(getPixel(src, x, y)));
        }
    }
}

// 反色滤镜
void filterInvert(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, x, y, invertColor(getPixel(src, x, y)));
        }
    }
}

// 亮度/对比度调整
void filterBrightnessContrast(Matrix *src, Matrix *dst, float brightness, float contrast) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            
            // 对比度调整
            float r = ((c.r / 255.0f - 0.5f) * contrast + 0.5f) * 255;
            float g = ((c.g / 255.0f - 0.5f) * contrast + 0.5f) * 255;
            float b = ((c.b / 255.0f - 0.5f) * contrast + 0.5f) * 255;
            
            // 亮度调整
            r += brightness * 255;
            g += brightness * 255;
            b += brightness * 255;
            
            c.r = clamp((int)r, 0, 255);
            c.g = clamp((int)g, 0, 255);
            c.b = clamp((int)b, 0, 255);
            
            setPixel(dst, x, y, c);
        }
    }
}

// 色调分离（Posterize）
void filterPosterize(Matrix *src, Matrix *dst, int levels) {
    if (levels < 2) levels = 2;
    float step = 255.0f / (levels - 1);
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            
            c.r = (uint8_t)(roundf(c.r / step) * step);
            c.g = (uint8_t)(roundf(c.g / step) * step);
            c.b = (uint8_t)(roundf(c.b / step) * step);
            
            setPixel(dst, x, y, c);
        }
    }
}

// 阈值化（二值化）
void filterThreshold(Matrix *src, Matrix *dst, uint8_t threshold) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            uint8_t gray = (c.r * 299 + c.g * 587 + c.b * 114) / 1000;
            
            if (gray > threshold) {
                setPixel(dst, x, y, COLOR_WHITE);
            } else {
                setPixel(dst, x, y, COLOR_BLACK);
            }
        }
    }
}

// 色相偏移
void filterHueShift(Matrix *src, Matrix *dst, float shift) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            
            // RGB转HSV
            float r = c.r / 255.0f;
            float g = c.g / 255.0f;
            float b = c.b / 255.0f;
            
            float maxC = fmaxf(fmaxf(r, g), b);
            float minC = fminf(fminf(r, g), b);
            float delta = maxC - minC;
            
            float h = 0, s = 0, v = maxC;
            
            if (delta > 0) {
                s = delta / maxC;
                
                if (maxC == r) {
                    h = 60 * fmodf((g - b) / delta, 6);
                } else if (maxC == g) {
                    h = 60 * ((b - r) / delta + 2);
                } else {
                    h = 60 * ((r - g) / delta + 4);
                }
            }
            
            if (h < 0) h += 360;
            
            // 偏移色相
            h = fmodf(h + shift, 360);
            if (h < 0) h += 360;
            
            // HSV转RGB
            float c2 = v * s;
            float x2 = c2 * (1 - fabsf(fmodf(h / 60, 2) - 1));
            float m = v - c2;
            
            float r2, g2, b2;
            if (h < 60) { r2 = c2; g2 = x2; b2 = 0; }
            else if (h < 120) { r2 = x2; g2 = c2; b2 = 0; }
            else if (h < 180) { r2 = 0; g2 = c2; b2 = x2; }
            else if (h < 240) { r2 = 0; g2 = x2; b2 = c2; }
            else if (h < 300) { r2 = x2; g2 = 0; b2 = c2; }
            else { r2 = c2; g2 = 0; b2 = x2; }
            
            c.r = (uint8_t)((r2 + m) * 255);
            c.g = (uint8_t)((g2 + m) * 255);
            c.b = (uint8_t)((b2 + m) * 255);
            
            setPixel(dst, x, y, c);
        }
    }
}

// 饱和度调整
void filterSaturation(Matrix *src, Matrix *dst, float saturation) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            Color gray = toGrayscale(c);
            
            c.r = clamp((int)(gray.r + (c.r - gray.r) * saturation), 0, 255);
            c.g = clamp((int)(gray.g + (c.g - gray.g) * saturation), 0, 255);
            c.b = clamp((int)(gray.b + (c.b - gray.b) * saturation), 0, 255);
            
            setPixel(dst, x, y, c);
        }
    }
}

// 怀旧/复古滤镜
void filterSepia(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            
            int r = (int)(c.r * 0.393f + c.g * 0.769f + c.b * 0.189f);
            int g = (int)(c.r * 0.349f + c.g * 0.686f + c.b * 0.168f);
            int b = (int)(c.r * 0.272f + c.g * 0.534f + c.b * 0.131f);
            
            c.r = clamp(r, 0, 255);
            c.g = clamp(g, 0, 255);
            c.b = clamp(b, 0, 255);
            
            setPixel(dst, x, y, c);
        }
    }
}

/*===========================================================================
 * 卷积滤镜
 *===========================================================================*/

void applyConvolution(Matrix *src, Matrix *dst, const float *kernel, 
                      int kernelSize, float divisor, float offset) {
    int half = kernelSize / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float sumR = 0, sumG = 0, sumB = 0;
            
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int px = x + kx - half;
                    int py = y + ky - half;
                    
                    Color c = getPixelSafe(src, px, py, COLOR_BLACK);
                    float k = kernel[ky * kernelSize + kx];
                    
                    sumR += c.r * k;
                    sumG += c.g * k;
                    sumB += c.b * k;
                }
            }
            
            Color result;
            result.a = 255;
            result.r = clamp((int)(sumR / divisor + offset), 0, 255);
            result.g = clamp((int)(sumG / divisor + offset), 0, 255);
            result.b = clamp((int)(sumB / divisor + offset), 0, 255);
            
            setPixel(dst, x, y, result);
        }
    }
}

// 模糊滤镜
void filterBlur(Matrix *src, Matrix *dst) {
    static const float kernel[9] = {
        1, 1, 1,
        1, 1, 1,
        1, 1, 1
    };
    applyConvolution(src, dst, kernel, 3, 9, 0);
}

// 高斯模糊
void filterGaussianBlur(Matrix *src, Matrix *dst) {
    static const float kernel[25] = {
        1,  4,  6,  4, 1,
        4, 16, 24, 16, 4,
        6, 24, 36, 24, 6,
        4, 16, 24, 16, 4,
        1,  4,  6,  4, 1
    };
    applyConvolution(src, dst, kernel, 5, 256, 0);
}

// 锐化滤镜
void filterSharpen(Matrix *src, Matrix *dst) {
    static const float kernel[9] = {
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0
    };
    applyConvolution(src, dst, kernel, 3, 1, 0);
}

// 边缘检测
void filterEdgeDetect(Matrix *src, Matrix *dst) {
    static const float kernel[9] = {
        -1, -1, -1,
        -1,  8, -1,
        -1, -1, -1
    };
    applyConvolution(src, dst, kernel, 3, 1, 0);
}

// 浮雕效果
void filterEmboss(Matrix *src, Matrix *dst) {
    static const float kernel[9] = {
        -2, -1, 0,
        -1,  1, 1,
         0,  1, 2
    };
    applyConvolution(src, dst, kernel, 3, 1, 128);
}

// 扫描线效果
void filterScanlines(Matrix *src, Matrix *dst, int lineSpacing, float darkness) {
    for (int y = 0; y < src->height; y++) {
        float factor = (y % lineSpacing == 0) ? darkness : 1.0f;
        
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            setPixel(dst, x, y, adjustBrightness(c, factor));
        }
    }
}

// CRT效果（扫描线+轻微弯曲）
void filterCRT(Matrix *src, Matrix *dst, float curvature, int scanlineSpacing) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            // 桶形畸变
            float dx = (x - cx) / (float)cx;
            float dy = (y - cy) / (float)cy;
            float dist = dx * dx + dy * dy;
            float factor = 1.0f + curvature * dist;
            
            float srcX = dx / factor * cx + cx;
            float srcY = dy / factor * cy + cy;
            
            Color c;
            if (srcX >= 0 && srcX < src->width && srcY >= 0 && srcY < src->height) {
                c = getPixelBilinear(src, srcX, srcY);
                
                // 扫描线
                if (y % scanlineSpacing == 0) {
                    c = adjustBrightness(c, 0.7f);
                }
                
                // 边缘变暗
                float vignette = 1.0f - dist * 0.3f;
                c = adjustBrightness(c, vignette);
            } else {
                c = COLOR_BLACK;
            }
            
            setPixel(dst, x, y, c);
        }
    }
}
```

------

### 8. 动画与缓动函数

```c
/*===========================================================================
 * 缓动函数
 *===========================================================================*/

typedef float (*EasingFunc)(float t);

// 线性
float easeLinear(float t) {
    return t;
}

// 二次缓动
float easeInQuad(float t) {
    return t * t;
}

float easeOutQuad(float t) {
    return t * (2 - t);
}

float easeInOutQuad(float t) {
    return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

// 三次缓动
float easeInCubic(float t) {
    return t * t * t;
}

float easeOutCubic(float t) {
    float t1 = t - 1;
    return t1 * t1 * t1 + 1;
}

float easeInOutCubic(float t) {
    return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}

// 弹性缓动
float easeOutElastic(float t) {
    if (t == 0 || t == 1) return t;
    return powf(2, -10 * t) * sinf((t - 0.075f) * (2 * M_PI) / 0.3f) + 1;
}

float easeInElastic(float t) {
    if (t == 0 || t == 1) return t;
    return -powf(2, 10 * (t - 1)) * sinf((t - 1.075f) * (2 * M_PI) / 0.3f);
}

// 弹跳缓动
float easeOutBounce(float t) {
    if (t < 1 / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2 / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

float easeInBounce(float t) {
    return 1 - easeOutBounce(1 - t);
}

// 回退缓动
float easeInBack(float t) {
    float s = 1.70158f;
    return t * t * ((s + 1) * t - s);
}

float easeOutBack(float t) {
    float s = 1.70158f;
    t -= 1;
    return t * t * ((s + 1) * t + s) + 1;
}

// 正弦缓动
float easeInSine(float t) {
    return 1 - cosf(t * M_PI / 2);
}

float easeOutSine(float t) {
    return sinf(t * M_PI / 2);
}

float easeInOutSine(float t) {
    return -(cosf(M_PI * t) - 1) / 2;
}

/*===========================================================================
 * 动画系统
 *===========================================================================*/

typedef enum {
    ANIM_SCROLL_LEFT,
    ANIM_SCROLL_RIGHT,
    ANIM_SCROLL_UP,
    ANIM_SCROLL_DOWN,
    ANIM_FADE_IN,
    ANIM_FADE_OUT,
    ANIM_ZOOM_IN,
    ANIM_ZOOM_OUT,
    ANIM_ROTATE,
    ANIM_WAVE_H,
    ANIM_WAVE_V,
    ANIM_RIPPLE,
    ANIM_SWIRL,
    ANIM_WIPE_LEFT,
    ANIM_WIPE_RIGHT,
    ANIM_WIPE_UP,
    ANIM_WIPE_DOWN,
    ANIM_WIPE_CIRCLE,
    ANIM_BLINDS_H,
    ANIM_BLINDS_V,
    ANIM_DISSOLVE,
    ANIM_MOSAIC,
    ANIM_FLIP_H,
    ANIM_FLIP_V,
    ANIM_BOUNCE,
    ANIM_SPIRAL
} AnimationType;

typedef struct {
    AnimationType type;
    float duration;       // 动画时长（秒）
    float elapsed;        // 已过时间
    EasingFunc easing;    // 缓动函数
    int loop;             // 是否循环
    int pingPong;         // 是否往返
    int reverse;          // 当前是否反向
    float param1, param2; // 额外参数
} Animation;

void initAnimation(Animation *anim, AnimationType type, float duration, EasingFunc easing) {
    anim->type = type;
    anim->duration = duration;
    anim->elapsed = 0;
    anim->easing = easing ? easing : easeLinear;
    anim->loop = 0;
    anim->pingPong = 0;
    anim->reverse = 0;
    anim->param1 = 0;
    anim->param2 = 0;
}

int updateAnimation(Animation *anim, float dt) {
    anim->elapsed += dt;
    
    if (anim->elapsed >= anim->duration) {
        if (anim->pingPong) {
            anim->reverse = !anim->reverse;
            anim->elapsed = 0;
            return 1;  // 继续
        } else if (anim->loop) {
            anim->elapsed = 0;
            return 1;  // 继续
        } else {
            anim->elapsed = anim->duration;
            return 0;  // 结束
        }
    }
    return 1;  // 继续
}

float getAnimationProgress(Animation *anim) {
    float t = anim->elapsed / anim->duration;
    if (t > 1.0f) t = 1.0f;
    
    float progress = anim->easing(t);
    
    if (anim->reverse) {
        progress = 1.0f - progress;
    }
    
    return progress;
}

void applyAnimation(Animation *anim, Matrix *src, Matrix *dst) {
    float progress = getAnimationProgress(anim);
    
    switch (anim->type) {
        case ANIM_SCROLL_LEFT:
            scrollEffect(src, dst, SCROLL_LEFT, (int)(src->width * progress), 0);
            break;
        case ANIM_SCROLL_RIGHT:
            scrollEffect(src, dst, SCROLL_RIGHT, (int)(src->width * progress), 0);
            break;
        case ANIM_SCROLL_UP:
            scrollEffect(src, dst, SCROLL_UP, (int)(src->height * progress), 0);
            break;
        case ANIM_SCROLL_DOWN:
            scrollEffect(src, dst, SCROLL_DOWN, (int)(src->height * progress), 0);
            break;
        case ANIM_FADE_IN:
            fadeIn(src, dst, progress);
            break;
        case ANIM_FADE_OUT:
            fadeOut(src, dst, progress);
            break;
        case ANIM_ZOOM_IN:
            zoomIn(src, dst, progress, 1);
            break;
        case ANIM_ZOOM_OUT:
            zoomOut(src, dst, progress, 1);
            break;
        case ANIM_ROTATE:
            rotateAngle(src, dst, progress * 360 * (anim->param1 > 0 ? anim->param1 : 1), 1);
            break;
        case ANIM_WAVE_H: {
            float amp = anim->param1 > 0 ? anim->param1 : 5;
            waveHorizontal(src, dst, amp, 0.3f, progress * 2 * M_PI);
            break;
        }
        case ANIM_WAVE_V: {
            float amp = anim->param1 > 0 ? anim->param1 : 5;
            waveVertical(src, dst, amp, 0.3f, progress * 2 * M_PI);
            break;
        }
        case ANIM_RIPPLE: {
            float amp = anim->param1 > 0 ? anim->param1 : 3;
            rippleEffect(src, dst, amp, 10, progress * 10, -1, -1);
            break;
        }
        case ANIM_SWIRL: {
            float strength = anim->param1 != 0 ? anim->param1 : 3;
            swirlEffect(src, dst, strength * sinf(progress * M_PI), -1, -1, -1);
            break;
        }
        case ANIM_WIPE_LEFT:
            wipeEffect(src, dst, WIPE_LEFT, progress);
            break;
        case ANIM_WIPE_RIGHT:
            wipeEffect(src, dst, WIPE_RIGHT, progress);
            break;
        case ANIM_WIPE_UP:
            wipeEffect(src, dst, WIPE_UP, progress);
            break;
        case ANIM_WIPE_DOWN:
            wipeEffect(src, dst, WIPE_DOWN, progress);
            break;
        case ANIM_WIPE_CIRCLE:
            wipeEffect(src, dst, WIPE_CIRCLE_OUT, progress);
            break;
        case ANIM_BLINDS_H: {
            int count = anim->param1 > 0 ? (int)anim->param1 : 8;
            blindsHorizontal(src, dst, count, progress);
            break;
        }
        case ANIM_BLINDS_V: {
            int count = anim->param1 > 0 ? (int)anim->param1 : 8;
            blindsVertical(src, dst, count, progress);
            break;
        }
        case ANIM_DISSOLVE:
            dissolveOrdered(src, dst, progress);
            break;
        case ANIM_MOSAIC:
            mosaicTransition(src, dst, progress);
            break;
        case ANIM_FLIP_H:
            flip3D_Y(src, dst, progress * M_PI);
            break;
        case ANIM_FLIP_V:
            flip3D_X(src, dst, progress * M_PI);
            break;
        case ANIM_BOUNCE: {
            float scale = 1.0f + 0.3f * sinf(progress * M_PI * 3) * (1 - progress);
            zoomEffect(src, dst, scale, -1, -1, 1);
            break;
        }
        case ANIM_SPIRAL:
            spiralReveal(src, dst, progress);
            break;
    }
}
```

------

### 9. 文字滚动与显示

```c
/*===========================================================================
 * 文字滚动效果
 *===========================================================================*/

typedef struct {
    Matrix *textBuffer;   // 文字渲染缓冲
    int textWidth;        // 文字总宽度
    int scrollPos;        // 当前滚动位置
    float scrollSpeed;    // 滚动速度（像素/秒）
    float accumulator;    // 时间累加器
    int loop;             // 是否循环
    int direction;        // 方向：0=左，1=右，2=上，3=下
} TextScroller;

void initTextScroller(TextScroller *scroller, Matrix *textBuffer, 
                      float speed, int direction, int loop) {
    scroller->textBuffer = textBuffer;
    scroller->textWidth = textBuffer->width;
    scroller->scrollPos = 0;
    scroller->scrollSpeed = speed;
    scroller->accumulator = 0;
    scroller->loop = loop;
    scroller->direction = direction;
}

void updateTextScroller(TextScroller *scroller, float dt) {
    scroller->accumulator += dt * scroller->scrollSpeed;
    
    while (scroller->accumulator >= 1.0f) {
        scroller->scrollPos++;
        scroller->accumulator -= 1.0f;
    }
    
    int maxScroll;
    if (scroller->direction <= 1) {
        maxScroll = scroller->textBuffer->width;
    } else {
        maxScroll = scroller->textBuffer->height;
    }
    
    if (scroller->loop && scroller->scrollPos >= maxScroll) {
        scroller->scrollPos = 0;
    }
}

void renderTextScroller(TextScroller *scroller, Matrix *dst) {
    int pos = scroller->scrollPos;
    
    switch (scroller->direction) {
        case 0:  // 左
            scrollEffect(scroller->textBuffer, dst, SCROLL_LEFT, pos, scroller->loop);
            break;
        case 1:  // 右
            scrollEffect(scroller->textBuffer, dst, SCROLL_RIGHT, pos, scroller->loop);
            break;
        case 2:  // 上
            scrollEffect(scroller->textBuffer, dst, SCROLL_UP, pos, scroller->loop);
            break;
        case 3:  // 下
            scrollEffect(scroller->textBuffer, dst, SCROLL_DOWN, pos, scroller->loop);
            break;
    }
}

/*===========================================================================
 * 打字机效果
 *===========================================================================*/

typedef struct {
    Matrix *fullText;     // 完整文字
    int charsToShow;      // 当前显示字符数
    int totalChars;       // 总字符数
    float charDelay;      // 每字符延迟
    float accumulator;
    int charWidth;        // 每字符宽度
} TypewriterEffect;

void initTypewriter(TypewriterEffect *tw, Matrix *fullText, 
                    int charWidth, float charDelay) {
    tw->fullText = fullText;
    tw->charWidth = charWidth;
    tw->totalChars = fullText->width / charWidth;
    tw->charsToShow = 0;
    tw->charDelay = charDelay;
    tw->accumulator = 0;
}

int updateTypewriter(TypewriterEffect *tw, float dt) {
    tw->accumulator += dt;
    
    while (tw->accumulator >= tw->charDelay && tw->charsToShow < tw->totalChars) {
        tw->charsToShow++;
        tw->accumulator -= tw->charDelay;
    }
    
    return tw->charsToShow < tw->totalChars;
}

void renderTypewriter(TypewriterEffect *tw, Matrix *dst) {
    clearMatrix(dst, COLOR_BLACK);
    
    int showWidth = tw->charsToShow * tw->charWidth;
    
    for (int y = 0; y < tw->fullText->height && y < dst->height; y++) {
        for (int x = 0; x < showWidth && x < dst->width; x++) {
            setPixel(dst, x, y, getPixel(tw->fullText, x, y));
        }
    }
}
```

------

### 10. 调色板工具

```c
/*===========================================================================
 * 调色板创建工具
 *===========================================================================*/

void createGrayscalePalette(Matrix *m, int levels) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(levels * sizeof(uint32_t));
    m->paletteSize = levels;
    
    for (int i = 0; i < levels; i++) {
        uint8_t gray = (levels > 1) ? (i * 255 / (levels - 1)) : 0;
        m->palette[i] = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
    }
}

void createRGB332Palette(Matrix *m) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(256 * sizeof(uint32_t));
    m->paletteSize = 256;
    
    for (int i = 0; i < 256; i++) {
        uint8_t r = ((i >> 5) & 0x07) * 255 / 7;
        uint8_t g = ((i >> 2) & 0x07) * 255 / 7;
        uint8_t b = (i & 0x03) * 255 / 3;
        m->palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

void createWebSafePalette(Matrix *m) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(216 * sizeof(uint32_t));
    m->paletteSize = 216;
    
    int idx = 0;
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                m->palette[idx++] = 0xFF000000 | 
                    ((r * 51) << 16) | ((g * 51) << 8) | (b * 51);
            }
        }
    }
}

void createHeatmapPalette(Matrix *m) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(256 * sizeof(uint32_t));
    m->paletteSize = 256;
    
    for (int i = 0; i < 256; i++) {
        uint8_t r, g, b;
        
        if (i < 64) {
            r = 0; g = 0; b = i * 4;
        } else if (i < 128) {
            r = 0; g = (i - 64) * 4; b = 255;
        } else if (i < 192) {
            r = (i - 128) * 4; g = 255; b = 255 - (i - 128) * 4;
        } else {
            r = 255; g = 255 - (i - 192) * 4; b = 0;
        }
        
        m->palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

void createRainbowPalette(Matrix *m, int size) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(size * sizeof(uint32_t));
    m->paletteSize = size;
    
    for (int i = 0; i < size; i++) {
        float hue = (float)i / size * 360;
        
        float c = 1.0f;
        float x = 1.0f - fabsf(fmodf(hue / 60, 2) - 1);
        float r, g, b;
        
        if (hue < 60) { r = c; g = x; b = 0; }
        else if (hue < 120) { r = x; g = c; b = 0; }
        else if (hue < 180) { r = 0; g = c; b = x; }
        else if (hue < 240) { r = 0; g = x; b = c; }
        else if (hue < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        
        m->palette[i] = 0xFF000000 | 
            ((uint8_t)(r * 255) << 16) | 
            ((uint8_t)(g * 255) << 8) | 
            (uint8_t)(b * 255);
    }
}

// CGA 16色调色板
void createCGAPalette(Matrix *m) {
    static const uint32_t cgaColors[16] = {
        0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA,
        0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
        0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF,
        0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
    };
    
    free(m->palette);
    m->palette = (uint32_t*)malloc(16 * sizeof(uint32_t));
    m->paletteSize = 16;
    memcpy(m->palette, cgaColors, 16 * sizeof(uint32_t));
}

// EGA 64色调色板
void createEGAPalette(Matrix *m) {
    free(m->palette);
    m->palette = (uint32_t*)malloc(64 * sizeof(uint32_t));
    m->paletteSize = 64;
    
    for (int i = 0; i < 64; i++) {
        uint8_t r = ((i >> 2) & 0x01) * 0xAA + ((i >> 5) & 0x01) * 0x55;
        uint8_t g = ((i >> 1) & 0x01) * 0xAA + ((i >> 4) & 0x01) * 0x55;
        uint8_t b = (i & 0x01) * 0xAA + ((i >> 3) & 0x01) * 0x55;
        m->palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}
```

------

### 11. 格式转换与抖动

```c
/*===========================================================================
 * 格式转换
 *===========================================================================*/

Matrix* convertFormat(Matrix *src, ColorFormat dstFormat) {
    Matrix *dst = createMatrix(src->width, src->height, dstFormat, 4);
    if (!dst) return NULL;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, x, y, getPixel(src, x, y));
        }
    }
    return dst;
}

// Floyd-Steinberg 抖动转换
Matrix* convertFormatDithered(Matrix *src, ColorFormat dstFormat) {
    Matrix *dst = createMatrix(src->width, src->height, dstFormat, 4);
    if (!dst) return NULL;
    
    int w = src->width;
    int h = src->height;
    
    // 误差缓冲
    int16_t *errR = (int16_t*)calloc((w + 2) * 2, sizeof(int16_t));
    int16_t *errG = (int16_t*)calloc((w + 2) * 2, sizeof(int16_t));
    int16_t *errB = (int16_t*)calloc((w + 2) * 2, sizeof(int16_t));
    
    int16_t *curErrR = errR + 1;
    int16_t *curErrG = errG + 1;
    int16_t *curErrB = errB + 1;
    int16_t *nxtErrR = errR + w + 3;
    int16_t *nxtErrG = errG + w + 3;
    int16_t *nxtErrB = errB + w + 3;
    
    for (int y = 0; y < h; y++) {
        memset(nxtErrR - 1, 0, (w + 2) * sizeof(int16_t));
        memset(nxtErrG - 1, 0, (w + 2) * sizeof(int16_t));
        memset(nxtErrB - 1, 0, (w + 2) * sizeof(int16_t));
        
        for (int x = 0; x < w; x++) {
            Color c = getPixel(src, x, y);
            
            int r = clamp(c.r + curErrR[x], 0, 255);
            int g = clamp(c.g + curErrG[x], 0, 255);
            int b = clamp(c.b + curErrB[x], 0, 255);
            
            Color adjusted = {c.a, r, g, b};
            setPixel(dst, x, y, adjusted);
            
            Color quantized = getPixel(dst, x, y);
            
            int er = r - quantized.r;
            int eg = g - quantized.g;
            int eb = b - quantized.b;
            
            // 分散误差
            curErrR[x + 1] += er * 7 / 16;
            curErrG[x + 1] += eg * 7 / 16;
            curErrB[x + 1] += eb * 7 / 16;
            
            nxtErrR[x - 1] += er * 3 / 16;
            nxtErrG[x - 1] += eg * 3 / 16;
            nxtErrB[x - 1] += eb * 3 / 16;
            
            nxtErrR[x] += er * 5 / 16;
            nxtErrG[x] += eg * 5 / 16;
            nxtErrB[x] += eb * 5 / 16;
            
            nxtErrR[x + 1] += er * 1 / 16;
            nxtErrG[x + 1] += eg * 1 / 16;
            nxtErrB[x + 1] += eb * 1 / 16;
        }
        
        // 交换缓冲
        int16_t *tmp;
        tmp = curErrR; curErrR = nxtErrR; nxtErrR = tmp;
        tmp = curErrG; curErrG = nxtErrG; nxtErrG = tmp;
        tmp = curErrB; curErrB = nxtErrB; nxtErrB = tmp;
    }
    
    free(errR);
    free(errG);
    free(errB);
    
    return dst;
}

// 有序抖动转换
Matrix* convertFormatOrderedDither(Matrix *src, ColorFormat dstFormat) {
    Matrix *dst = createMatrix(src->width, src->height, dstFormat, 4);
    if (!dst) return NULL;
    
    static const int bayer8x8[64] = {
         0, 32,  8, 40,  2, 34, 10, 42,
        48, 16, 56, 24, 50, 18, 58, 26,
        12, 44,  4, 36, 14, 46,  6, 38,
        60, 28, 52, 20, 62, 30, 54, 22,
         3, 35, 11, 43,  1, 33,  9, 41,
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47,  7, 39, 13, 45,  5, 37,
        63, 31, 55, 23, 61, 29, 53, 21
    };
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = getPixel(src, x, y);
            int threshold = bayer8x8[(y & 7) * 8 + (x & 7)] * 4;
            
            c.r = clamp(c.r + threshold - 128, 0, 255);
            c.g = clamp(c.g + threshold - 128, 0, 255);
            c.b = clamp(c.b + threshold - 128, 0, 255);
            
            setPixel(dst, x, y, c);
        }
    }
    
    return dst;
}
```

------

### 12. 使用示例

```c
/*===========================================================================
 * 使用示例
 *===========================================================================*/

// 示例1：基本特效使用
void example_basic_effects() {
    // 创建源图像（RGB565格式，常见TFT屏）
    Matrix *src = createMatrix(128, 64, COLOR_FORMAT_RGB565, 2);
    Matrix *dst = createMatrix(128, 64, COLOR_FORMAT_RGB565, 2);
    
    // 填充测试图案
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            Color c = makeColor(x * 2, y * 4, (x + y) * 2);
            setPixel(src, x, y, c);
        }
    }
    
    // 应用各种特效
    fadeIn(src, dst, 0.5f);
    rotateAngle(src, dst, 45.0f, 1);
    waveHorizontal(src, dst, 5.0f, 0.2f, 0.0f);
    mosaicEffect(src, dst, 8);
    filterGrayscale(src, dst);
    
    freeMatrix(src);
    freeMatrix(dst);
}

// 示例2：动画播放
void example_animation() {
    Matrix *src = createMatrix(64, 32, COLOR_FORMAT_ARGB8888, 4);
    Matrix *dst = createMatrix(64, 32, COLOR_FORMAT_ARGB8888, 4);
    
    // 初始化动画
    Animation anim;
    initAnimation(&anim, ANIM_ZOOM_IN, 2.0f, easeOutElastic);
    anim.loop = 0;
    anim.pingPong = 1;
    
    // 动画循环
    float dt = 0.016f;  // 约60fps
    while (updateAnimation(&anim, dt)) {
        applyAnimation(&anim, src, dst);
        // 显示dst到屏幕...
    }
    
    freeMatrix(src);
    freeMatrix(dst);
}

// 示例3：粒子效果
void example_particles() {
    Matrix *screen = createMatrix(128, 64, COLOR_FORMAT_RGB888, 4);
    ParticleSystem ps;
    initParticleSystem(&ps);
    ps.gravity = 0.2f;
    
    // 发射一些粒子
    for (int i = 0; i < 50; i++) {
        float vx = (rand() % 100 - 50) / 10.0f;
        float vy = -(rand() % 50) / 10.0f - 2;
        emitParticle(&ps, 64, 60, vx, vy, COLOR_YELLOW, 3.0f, 2);
    }
    
    // 更新循环
    float dt = 0.016f;
    for (int frame = 0; frame < 200; frame++) {
        clearMatrix(screen, COLOR_BLACK);
        updateParticleSystem(&ps, dt);
        renderParticleSystem(&ps, screen);
        // 显示screen...
    }
    
    freeMatrix(screen);
}

// 示例4：火焰效果

void example_fire() {
    Matrix *screen = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    FireEffect *fire = createFireEffect(64, 32);
    
    // 动画循环
    for (int frame = 0; frame < 500; frame++) {
        updateFireEffect(fire);
        renderFireEffect(fire, screen);
        // 显示screen到点阵屏...
        // delay(30);
    }
    
    freeFireEffect(fire);
    freeMatrix(screen);
}

// 示例5：矩阵雨效果
void example_matrix_rain() {
    Matrix *screen = createMatrix(64, 32, COLOR_FORMAT_RGB888, 4);
    MatrixRainEffect *rain = createMatrixRainEffect(64, 32);
    
    for (int frame = 0; frame < 1000; frame++) {
        updateMatrixRainEffect(rain);
        renderMatrixRainEffect(rain, screen);
        // 显示screen...
    }
    
    freeMatrixRainEffect(rain);
    freeMatrix(screen);
}

// 示例6：多种格式转换
void example_format_conversion() {
    // 创建32位ARGB源图像
    Matrix *src = createMatrix(128, 64, COLOR_FORMAT_ARGB8888, 4);
    
    // 填充渐变
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(src, x, y, makeColor(x * 2, y * 4, 128));
        }
    }
    
    // 转换为各种格式
    Matrix *rgb565 = convertFormat(src, COLOR_FORMAT_RGB565);
    Matrix *gray8 = convertFormat(src, COLOR_FORMAT_8BPP);
    Matrix *mono = convertFormat(src, COLOR_FORMAT_1BPP);
    
    // 带抖动转换（效果更好）
    Matrix *mono_dithered = convertFormatDithered(src, COLOR_FORMAT_1BPP);
    Matrix *gray4_dithered = convertFormatDithered(src, COLOR_FORMAT_4BPP);
    
    freeMatrix(src);
    freeMatrix(rgb565);
    freeMatrix(gray8);
    freeMatrix(mono);
    freeMatrix(mono_dithered);
    freeMatrix(gray4_dithered);
}

// 示例7：组合特效
void example_combined_effects() {
    Matrix *src = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    Matrix *temp = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    Matrix *dst = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    
    // 填充源图像...
    
    // 组合多个特效
    filterSepia(src, temp);           // 先应用复古滤镜
    waveHorizontal(temp, dst, 3, 0.2f, 0);  // 再应用波浪
    filterScanlines(dst, temp, 2, 0.7f);    // 最后添加扫描线
    
    // temp 现在包含组合效果
    
    freeMatrix(src);
    freeMatrix(temp);
    freeMatrix(dst);
}

// 示例8：星空效果
void example_starfield() {
    Matrix *screen = createMatrix(128, 64, COLOR_FORMAT_RGB565, 2);
    StarfieldEffect sf;
    initStarfieldEffect(&sf, 128, 64);
    sf.speed = 3.0f;
    
    float dt = 0.016f;
    for (int frame = 0; frame < 500; frame++) {
        updateStarfieldEffect(&sf, dt);
        renderStarfieldEffect(&sf, screen);
        // 显示screen...
    }
    
    freeMatrix(screen);
}

// 示例9：雨雪效果
void example_weather() {
    Matrix *screen = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    
    // 雨效果
    RainEffect rain;
    initRainEffect(&rain, 64, 32, makeColor(100, 100, 255));
    rain.wind = 0.5f;
    
    for (int frame = 0; frame < 200; frame++) {
        updateRainEffect(&rain);
        renderRainEffect(&rain, screen, 1);
        // 显示...
    }
    
    // 雪效果
    SnowEffect snow;
    initSnowEffect(&snow, 64, 32);
    
    float dt = 0.05f;
    for (int frame = 0; frame < 200; frame++) {
        updateSnowEffect(&snow, dt);
        renderSnowEffect(&snow, screen, 1);
        // 显示...
    }
    
    freeMatrix(screen);
}

// 示例10：过渡动画（两张图片之间）
void example_transition() {
    Matrix *img1 = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    Matrix *img2 = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    Matrix *screen = createMatrix(64, 32, COLOR_FORMAT_RGB565, 2);
    
    // 填充两张不同的图片...
    fillRect(img1, 0, 0, 64, 32, COLOR_RED);
    fillRect(img2, 0, 0, 64, 32, COLOR_BLUE);
    
    // 交叉淡入淡出过渡
    for (int frame = 0; frame <= 60; frame++) {
        float progress = frame / 60.0f;
        crossFade(img1, img2, screen, progress);
        // 显示screen...
    }
    
    // 擦除过渡
    for (int frame = 0; frame <= 60; frame++) {
        float progress = frame / 60.0f;
        // 先显示img1，然后用img2擦除
        copyMatrix(img1, screen);
        
        // 创建临时遮罩
        for (int y = 0; y < screen->height; y++) {
            for (int x = 0; x < (int)(screen->width * progress); x++) {
                setPixel(screen, x, y, getPixel(img2, x, y));
            }
        }
        // 显示screen...
    }
    
    freeMatrix(img1);
    freeMatrix(img2);
    freeMatrix(screen);
}
```

------

### 13. 高级特效补充

```c
/*===========================================================================
 * 等离子效果
 *===========================================================================*/

void plasmaEffect(Matrix *dst, float time) {
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            float value = 0;
            
            // 多个正弦波叠加
            value += sinf(x * 0.1f + time);
            value += sinf(y * 0.1f + time * 0.5f);
            value += sinf((x + y) * 0.1f + time * 0.3f);
            value += sinf(sqrtf(x * x + y * y) * 0.1f + time * 0.7f);
            
            // 归一化到0-1
            value = (value + 4) / 8.0f;
            
            // 转换为彩虹色
            float hue = value * 360;
            float c = 1.0f;
            float x2 = 1.0f - fabsf(fmodf(hue / 60, 2) - 1);
            float r, g, b;
            
            if (hue < 60) { r = c; g = x2; b = 0; }
            else if (hue < 120) { r = x2; g = c; b = 0; }
            else if (hue < 180) { r = 0; g = c; b = x2; }
            else if (hue < 240) { r = 0; g = x2; b = c; }
            else if (hue < 300) { r = x2; g = 0; b = c; }
            else { r = c; g = 0; b = x2; }
            
            setPixel(dst, x, y, makeColor(r * 255, g * 255, b * 255));
        }
    }
}

/*===========================================================================
 * 生命游戏效果
 *===========================================================================*/

typedef struct {
    uint8_t *cells[2];
    int currentBuffer;
    int width, height;
} GameOfLife;

GameOfLife* createGameOfLife(int width, int height) {
    GameOfLife *gol = (GameOfLife*)malloc(sizeof(GameOfLife));
    if (!gol) return NULL;
    
    gol->width = width;
    gol->height = height;
    gol->currentBuffer = 0;
    gol->cells[0] = (uint8_t*)calloc(width * height, 1);
    gol->cells[1] = (uint8_t*)calloc(width * height, 1);
    
    if (!gol->cells[0] || !gol->cells[1]) {
        free(gol->cells[0]);
        free(gol->cells[1]);
        free(gol);
        return NULL;
    }
    
    // 随机初始化
    for (int i = 0; i < width * height; i++) {
        gol->cells[0][i] = (rand() % 100) < 30 ? 1 : 0;
    }
    
    return gol;
}

void freeGameOfLife(GameOfLife *gol) {
    if (gol) {
        free(gol->cells[0]);
        free(gol->cells[1]);
        free(gol);
    }
}

void updateGameOfLife(GameOfLife *gol) {
    uint8_t *src = gol->cells[gol->currentBuffer];
    uint8_t *dst = gol->cells[1 - gol->currentBuffer];
    int w = gol->width;
    int h = gol->height;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int neighbors = 0;
            
            // 计算邻居数量
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = (x + dx + w) % w;
                    int ny = (y + dy + h) % h;
                    neighbors += src[ny * w + nx];
                }
            }
            
            int idx = y * w + x;
            if (src[idx]) {
                // 存活细胞
                dst[idx] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                // 死亡细胞
                dst[idx] = (neighbors == 3) ? 1 : 0;
            }
        }
    }
    
    gol->currentBuffer = 1 - gol->currentBuffer;
}

void renderGameOfLife(GameOfLife *gol, Matrix *dst, Color aliveColor, Color deadColor) {
    uint8_t *cells = gol->cells[gol->currentBuffer];
    
    for (int y = 0; y < gol->height && y < dst->height; y++) {
        for (int x = 0; x < gol->width && x < dst->width; x++) {
            Color c = cells[y * gol->width + x] ? aliveColor : deadColor;
            setPixel(dst, x, y, c);
        }
    }
}

/*===========================================================================
 * 烟花效果
 *===========================================================================*/

typedef struct {
    float x, y;
    float vx, vy;
    Color color;
    float life;
    int exploded;
} Firework;

typedef struct {
    Firework rockets[20];
    Particle sparks[500];
    int width, height;
    float gravity;
} FireworkSystem;

void initFireworkSystem(FireworkSystem *fs, int width, int height) {
    memset(fs, 0, sizeof(FireworkSystem));
    fs->width = width;
    fs->height = height;
    fs->gravity = 0.05f;
}

void launchFirework(FireworkSystem *fs, int x) {
    for (int i = 0; i < 20; i++) {
        if (fs->rockets[i].life <= 0) {
            Firework *r = &fs->rockets[i];
            r->x = x;
            r->y = fs->height;
            r->vx = (rand() % 20 - 10) / 20.0f;
            r->vy = -3.0f - (rand() % 100) / 100.0f;
            r->life = 2.0f;
            r->exploded = 0;
            
            // 随机颜色
            int hue = rand() % 360;
            float c = 1.0f;
            float x2 = 1.0f - fabsf(fmodf(hue / 60.0f, 2) - 1);
            if (hue < 60) r->color = makeColor(c * 255, x2 * 255, 0);
            else if (hue < 120) r->color = makeColor(x2 * 255, c * 255, 0);
            else if (hue < 180) r->color = makeColor(0, c * 255, x2 * 255);
            else if (hue < 240) r->color = makeColor(0, x2 * 255, c * 255);
            else if (hue < 300) r->color = makeColor(x2 * 255, 0, c * 255);
            else r->color = makeColor(c * 255, 0, x2 * 255);
            
            return;
        }
    }
}

void explodeFirework(FireworkSystem *fs, Firework *r) {
    int sparkCount = 30 + rand() % 30;
    
    for (int i = 0; i < sparkCount; i++) {
        for (int j = 0; j < 500; j++) {
            if (fs->sparks[j].life <= 0) {
                Particle *s = &fs->sparks[j];
                float angle = (rand() % 360) * M_PI / 180.0f;
                float speed = 0.5f + (rand() % 100) / 50.0f;
                
                s->x = r->x;
                s->y = r->y;
                s->vx = cosf(angle) * speed;
                s->vy = sinf(angle) * speed;
                s->color = r->color;
                s->life = 1.0f + (rand() % 100) / 100.0f;
                s->maxLife = s->life;
                s->size = 1;
                s->active = 1;
                break;
            }
        }
    }
}

void updateFireworkSystem(FireworkSystem *fs, float dt) {
    // 更新火箭
    for (int i = 0; i < 20; i++) {
        Firework *r = &fs->rockets[i];
        if (r->life <= 0) continue;
        
        r->x += r->vx;
        r->y += r->vy;
        r->vy += fs->gravity;
        r->life -= dt;
        
        // 到达顶点时爆炸
        if (!r->exploded && r->vy >= 0) {
            explodeFirework(fs, r);
            r->exploded = 1;
            r->life = 0;
        }
    }
    
    // 更新火花
    for (int i = 0; i < 500; i++) {
        Particle *s = &fs->sparks[i];
        if (s->life <= 0) continue;
        
        s->x += s->vx;
        s->y += s->vy;
        s->vy += fs->gravity;
        s->vx *= 0.98f;
        s->vy *= 0.98f;
        s->life -= dt;
    }
}

void renderFireworkSystem(FireworkSystem *fs, Matrix *dst) {
    // 淡化背景（产生拖尾效果）
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            Color c = getPixel(dst, x, y);
            c = adjustBrightness(c, 0.85f);
            setPixel(dst, x, y, c);
        }
    }
    
    // 绘制火箭
    for (int i = 0; i < 20; i++) {
        Firework *r = &fs->rockets[i];
        if (r->life <= 0 || r->exploded) continue;
        
        int px = (int)r->x;
        int py = (int)r->y;
        if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
            setPixel(dst, px, py, COLOR_WHITE);
        }
    }
    
    // 绘制火花
    for (int i = 0; i < 500; i++) {
        Particle *s = &fs->sparks[i];
        if (s->life <= 0) continue;
        
        int px = (int)s->x;
        int py = (int)s->y;
        if (px >= 0 && px < dst->width && py >= 0 && py < dst->height) {
            float alpha = s->life / s->maxLife;
            Color c = adjustBrightness(s->color, alpha);
            setPixel(dst, px, py, c);
        }
    }
}

/*===========================================================================
 * 音频可视化效果（频谱）
 *===========================================================================*/

typedef struct {
    float *spectrum;      // 频谱数据
    float *peakValues;    // 峰值
    float *fallSpeed;     // 下落速度
    int barCount;
    int width, height;
    Color barColor;
    Color peakColor;
} SpectrumVisualizer;

SpectrumVisualizer* createSpectrumVisualizer(int width, int height, int barCount) {
    SpectrumVisualizer *sv = (SpectrumVisualizer*)malloc(sizeof(SpectrumVisualizer));
    if (!sv) return NULL;
    
    sv->width = width;
    sv->height = height;
    sv->barCount = barCount;
    sv->spectrum = (float*)calloc(barCount, sizeof(float));
    sv->peakValues = (float*)calloc(barCount, sizeof(float));
    sv->fallSpeed = (float*)calloc(barCount, sizeof(float));
    sv->barColor = makeColor(0, 255, 0);
    sv->peakColor = COLOR_WHITE;
    
    return sv;
}

void freeSpectrumVisualizer(SpectrumVisualizer *sv) {
    if (sv) {
        free(sv->spectrum);
        free(sv->peakValues);
        free(sv->fallSpeed);
        free(sv);
    }
}

void updateSpectrumVisualizer(SpectrumVisualizer *sv, float *newSpectrum, float dt) {
    for (int i = 0; i < sv->barCount; i++) {
        // 平滑过渡
        sv->spectrum[i] += (newSpectrum[i] - sv->spectrum[i]) * 0.3f;
        
        // 更新峰值
        if (sv->spectrum[i] > sv->peakValues[i]) {
            sv->peakValues[i] = sv->spectrum[i];
            sv->fallSpeed[i] = 0;
        } else {
            sv->fallSpeed[i] += 0.001f;
            sv->peakValues[i] -= sv->fallSpeed[i];
            if (sv->peakValues[i] < 0) sv->peakValues[i] = 0;
        }
    }
}

void renderSpectrumVisualizer(SpectrumVisualizer *sv, Matrix *dst) {
    clearMatrix(dst, COLOR_BLACK);
    
    int barWidth = sv->width / sv->barCount;
    int gap = 1;
    
    for (int i = 0; i < sv->barCount; i++) {
        int x = i * barWidth;
        int barHeight = (int)(sv->spectrum[i] * sv->height);
        int peakY = sv->height - (int)(sv->peakValues[i] * sv->height);
        
        // 绘制频谱条
        for (int y = sv->height - barHeight; y < sv->height; y++) {
            // 渐变色
            float ratio = (float)(sv->height - y) / sv->height;
            Color c = lerpColor(makeColor(0, 255, 0), makeColor(255, 0, 0), ratio);
            
            for (int bx = x; bx < x + barWidth - gap && bx < sv->width; bx++) {
                setPixel(dst, bx, y, c);
            }
        }
        
        // 绘制峰值
        if (peakY >= 0 && peakY < sv->height) {
            for (int bx = x; bx < x + barWidth - gap && bx < sv->width; bx++) {
                setPixel(dst, bx, peakY, sv->peakColor);
            }
        }
    }
}

/*===========================================================================
 * VU表效果
 *===========================================================================*/

void renderVUMeter(Matrix *dst, float leftLevel, float rightLevel, 
                   int vertical, Color lowColor, Color midColor, Color highColor) {
    clearMatrix(dst, COLOR_BLACK);
    
    int segments = vertical ? dst->height : dst->width;
    int thickness = vertical ? dst->width / 2 : dst->height / 2;
    
    float levels[2] = {leftLevel, rightLevel};
    
    for (int ch = 0; ch < 2; ch++) {
        int litSegments = (int)(levels[ch] * segments);
        
        for (int i = 0; i < litSegments; i++) {
            float ratio = (float)i / segments;
            Color c;
            
            if (ratio < 0.6f) {
                c = lowColor;
            } else if (ratio < 0.85f) {
                c = midColor;
            } else {
                c = highColor;
            }
            
            if (vertical) {
                int y = dst->height - 1 - i;
                int x = ch * thickness;
                for (int dx = 0; dx < thickness - 1; dx++) {
                    setPixel(dst, x + dx, y, c);
                }
            } else {
                int x = i;
                int y = ch * thickness;
                for (int dy = 0; dy < thickness - 1; dy++) {
                    setPixel(dst, x, y + dy, c);
                }
            }
        }
    }
}

/*===========================================================================
 * 时钟效果
 *===========================================================================*/

void renderAnalogClock(Matrix *dst, int hours, int minutes, int seconds,
                       Color faceColor, Color hourColor, Color minColor, Color secColor) {
    int cx = dst->width / 2;
    int cy = dst->height / 2;
    int radius = (cx < cy ? cx : cy) - 2;
    
    clearMatrix(dst, COLOR_BLACK);
    
    // 绘制表盘刻度
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * M_PI / 180 - M_PI / 2;
        int x1 = cx + (int)(cosf(angle) * (radius - 2));
        int y1 = cy + (int)(sinf(angle) * (radius - 2));
        int x2 = cx + (int)(cosf(angle) * radius);
        int y2 = cy + (int)(sinf(angle) * radius);
        
        // 简单画点
        setPixel(dst, x2, y2, faceColor);
    }
    
    // 绘制时针
    float hourAngle = ((hours % 12) + minutes / 60.0f) * 30 * M_PI / 180 - M_PI / 2;
    int hourLen = radius * 0.5f;
    for (int i = 0; i < hourLen; i++) {
        int x = cx + (int)(cosf(hourAngle) * i);
        int y = cy + (int)(sinf(hourAngle) * i);
        setPixel(dst, x, y, hourColor);
    }
    
    // 绘制分针
    float minAngle = (minutes + seconds / 60.0f) * 6 * M_PI / 180 - M_PI / 2;
    int minLen = radius * 0.7f;
    for (int i = 0; i < minLen; i++) {
        int x = cx + (int)(cosf(minAngle) * i);
        int y = cy + (int)(sinf(minAngle) * i);
        setPixel(dst, x, y, minColor);
    }
    
    // 绘制秒针
    float secAngle = seconds * 6 * M_PI / 180 - M_PI / 2;
    int secLen = radius * 0.85f;
    for (int i = 0; i < secLen; i++) {
        int x = cx + (int)(cosf(secAngle) * i);
        int y = cy + (int)(sinf(secAngle) * i);
        setPixel(dst, x, y, secColor);
    }
    
    // 中心点
    setPixel(dst, cx, cy, COLOR_WHITE);
}

/*===========================================================================
 * 进度条效果
 *===========================================================================*/

typedef enum {
    PROGRESS_STYLE_SOLID,
    PROGRESS_STYLE_GRADIENT,
    PROGRESS_STYLE_STRIPED,
    PROGRESS_STYLE_BLOCKS,
    PROGRESS_STYLE_DOTS
} ProgressStyle;

void renderProgressBar(Matrix *dst, float progress, int x, int y, int width, int height,
                       Color bgColor, Color fgColor, ProgressStyle style, float animPhase) {
    progress = clampf(progress, 0.0f, 1.0f);
    int fillWidth = (int)(width * progress);
    
    // 背景
    fillRect(dst, x, y, width, height, bgColor);
    
    switch (style) {
        case PROGRESS_STYLE_SOLID:
            fillRect(dst, x, y, fillWidth, height, fgColor);
            break;
            
        case PROGRESS_STYLE_GRADIENT:
            for (int i = 0; i < fillWidth; i++) {
                float ratio = (float)i / width;
                Color c = lerpColor(fgColor, adjustBrightness(fgColor, 0.5f), ratio);
                for (int j = 0; j < height; j++) {
                    setPixel(dst, x + i, y + j, c);
                }
            }
            break;
            
        case PROGRESS_STYLE_STRIPED:
            for (int i = 0; i < fillWidth; i++) {
                int stripePos = (int)(i + animPhase * 10) % 10;
                float brightness = (stripePos < 5) ? 1.0f : 0.7f;
                Color c = adjustBrightness(fgColor, brightness);
                for (int j = 0; j < height; j++) {
                    setPixel(dst, x + i, y + j, c);
                }
            }
            break;
            
        case PROGRESS_STYLE_BLOCKS: {
            int blockWidth = 4;
            int gap = 1;
            int blocks = fillWidth / (blockWidth + gap);
            for (int b = 0; b < blocks; b++) {
                int bx = x + b * (blockWidth + gap);
                fillRect(dst, bx, y, blockWidth, height, fgColor);
            }
            break;
        }
            
        case PROGRESS_STYLE_DOTS: {
            int dotSpacing = 3;
            int dots = fillWidth / dotSpacing;
            for (int d = 0; d < dots; d++) {
                int dx = x + d * dotSpacing + 1;
                int dy = y + height / 2;
                setPixel(dst, dx, dy, fgColor);
                if (height > 2) {
                    setPixel(dst, dx, dy - 1, fgColor);
                    setPixel(dst, dx, dy + 1, fgColor);
                }
            }
            break;
        }
    }
}

/*===========================================================================
 * 加载动画效果
 *===========================================================================*/

void renderSpinner(Matrix *dst, int cx, int cy, int radius, float angle, 
                   int segments, Color color) {
    for (int i = 0; i < segments; i++) {
        float segAngle = angle + i * (2 * M_PI / segments);
        float brightness = 1.0f - (float)i / segments;
        Color c = adjustBrightness(color, brightness);
        
        int x = cx + (int)(cosf(segAngle) * radius);
        int y = cy + (int)(sinf(segAngle) * radius);
        
        if (x >= 0 && x < dst->width && y >= 0 && y < dst->height) {
            setPixel(dst, x, y, c);
        }
    }
}

void renderLoadingDots(Matrix *dst, int cx, int cy, float phase, int dotCount, 
                       int spacing, Color color) {
    int totalWidth = (dotCount - 1) * spacing;
    int startX = cx - totalWidth / 2;
    
    for (int i = 0; i < dotCount; i++) {
        float dotPhase = phase - i * 0.3f;
        float scale = (sinf(dotPhase) + 1) / 2;  // 0-1
        
        int x = startX + i * spacing;
        int y = cy - (int)(scale * 3);
        
        Color c = adjustBrightness(color, 0.5f + scale * 0.5f);
        
        if (x >= 0 && x < dst->width && y >= 0 && y < dst->height) {
            setPixel(dst, x, y, c);
        }
    }
}

void renderPulsingCircle(Matrix *dst, int cx, int cy, float phase, 
                         int maxRadius, Color color) {
    float radius = (sinf(phase) + 1) / 2 * maxRadius;
    float brightness = 1.0f - radius / maxRadius;
    Color c = adjustBrightness(color, brightness);
    
    // 简单圆形绘制
    for (int angle = 0; angle < 360; angle += 10) {
        float rad = angle * M_PI / 180;
        int x = cx + (int)(cosf(rad) * radius);
        int y = cy + (int)(sinf(rad) * radius);
        
        if (x >= 0 && x < dst->width && y >= 0 && y < dst->height) {
            setPixel(dst, x, y, c);
        }
    }
}
```

------

### 14. 效果索引表

```c
/*===========================================================================
 * 效果索引表 - 快速参考
 *===========================================================================*/

/*
┌─────────────────────────────────────────────────────────────────────────────┐
│                           点阵屏特效库 - 效果索引                            │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【基础变换】                                                                 │
│   scrollEffect()        - 上下左右滚动                                       │
│   scrollDiagonal()      - 对角线滚动                                         │
│   flipHorizontal()      - 水平翻转                                           │
│   flipVertical()        - 垂直翻转                                           │
│   rotate90CW/CCW()      - 90度旋转                                           │
│   rotate180()           - 180度旋转                                          │
│   rotateAngle()         - 任意角度旋转                                       │
│   zoomEffect()          - 缩放                                               │
│   zoomIn/Out()          - 跳入/跳出                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【过渡效果】                                                                 │
│   fadeIn/Out()          - 淡入/淡出                                          │
│   crossFade()           - 交叉淡入淡出                                       │
│   wipeEffect()          - 擦除（多方向）                                     │
│   blindsHorizontal()    - 水平百叶窗                                         │
│   blindsVertical()      - 垂直百叶窗                                         │
│   checkerboardEffect()  - 棋盘格过渡                                         │
│   dissolveOrdered()     - 有序溶解                                           │
│   dissolveRandom()      - 随机溶解                                           │
│   spiralReveal()        - 螺旋展开                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【变形效果】                                                                 │
│   waveHorizontal()      - 水平波浪                                           │
│   waveVertical()        - 垂直波浪                                           │
│   waveComplex()         - 复合波浪                                           │
│   rippleEffect()        - 水波纹                                             │
│   swirlEffect()         - 漩涡                                               │
│   fisheyeEffect()       - 鱼眼/球面                                          │
│   mosaicEffect()        - 马赛克                                             │
│   mosaicTransition()    - 动态马赛克                                         │
│   mirrorQuadrant()      - 四象限镜像                                         │
│   kaleidoscopeEffect()  - 万花筒                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【3D效果】                                                                   │
│   flip3D_Y()            - 绕Y轴3D翻转                                        │
│   flip3D_X()            - 绕X轴3D翻转                                        │
│   perspectiveEffect()   - 透视变换                                           │
│   cubeRotateY()         - 立方体旋转                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【滤镜效果】                                                                 │
│   filterGrayscale()     - 灰度                                               │
│   filterInvert()        - 反色                                               │
│   filterBrightnessContrast() - 亮度/对比度                                   │
│   filterPosterize()     - 色调分离                                           │
│   filterThreshold()     - 二值化                                             │
│   filterHueShift()      - 色相偏移                                           │
│   filterSaturation()    - 饱和度调整                                         │
│   filterSepia()         - 复古/怀旧                                          │
│   filterBlur()          - 模糊                                               │
│   filterGaussianBlur()  - 高斯模糊                                           │
│   filterSharpen()       - 锐化                                               │
│   filterEdgeDetect()    - 边缘检测                                           │
│   filterEmboss()        - 浮雕                                               │
│   filterScanlines()     - 扫描线                                             │
│   filterCRT()           - CRT显示器效果                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【粒子/动态效果】                                                            │
│   ParticleSystem        - 通用粒子系统                                       │
│   RainEffect            - 雨滴效果                                           │
│   SnowEffect            - 雪花效果                                           │
│   FireEffect            - 火焰效果                                           │
│   MatrixRainEffect      - 矩阵雨（黑客帝国）                                 │
│   StarfieldEffect       - 星空效果                                           │
│   FireworkSystem        - 烟花效果                                           │
│   plasmaEffect()        - 等离子效果                                         │
│   GameOfLife            - 生命游戏                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【UI组件】                                                                   │
│   renderProgressBar()   - 进度条（多种样式）                                 │
│   renderSpinner()       - 旋转加载                                           │
│   renderLoadingDots()   - 加载点动画                                         │
│   renderPulsingCircle() - 脉冲圆圈                                           │
│   renderAnalogClock()   - 模拟时钟                                           │
│   renderVUMeter()       - VU表                                               │
│   SpectrumVisualizer    - 频谱可视化                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【文字效果】                                                                 │
│   TextScroller          - 文字滚动                                           │
│   TypewriterEffect      - 打字机效果                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【动画系统】                                                                 │
│   Animation             - 动画控制器                                         │
│   EasingFunc            - 缓动函数（20+种）                                  │
│   applyAnimation()      - 应用动画                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【格式转换】                                                                 │
│   convertFormat()       - 基本格式转换                                       │
│   convertFormatDithered() - Floyd-Steinberg抖动转换                          │
│   convertFormatOrderedDither() - 有序抖动转换                                │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【调色板】                                                                   │
│   createGrayscalePalette()  - 灰度调色板                                     │
│   createRGB332Palette()     - RGB332调色板                                   │
│   createWebSafePalette()    - Web安全色                                      │
│   createHeatmapPalette()    - 热力图调色板                                   │
│   createRainbowPalette()    - 彩虹调色板                                     │
│   createCGAPalette()        - CGA 16色                                       │
│   createEGAPalette()        - EGA 64色                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【支持的色彩格式】                                                           │
│   COLOR_FORMAT_1BPP     - 1位单色                                            │
│   COLOR_FORMAT_2BPP     - 2位灰度（4级）                                     │
│   COLOR_FORMAT_4BPP     - 4位灰度/索引色（16色）                             │
│   COLOR_FORMAT_8BPP     - 8位灰度/索引色（256色）                            │
│   COLOR_FORMAT_RGB565   - 16位RGB（5-6-5）                                   │
│   COLOR_FORMAT_RGB888   - 24位RGB                                            │
│   COLOR_FORMAT_ARGB8888 - 32位ARGB                                           │
│   COLOR_FORMAT_RGBA8888 - 32位RGBA                                           │
└─────────────────────────────────────────────────────────────────────────────┘
*/
```

------

## 色彩格式内存占用对照表

|   格式   | 位深 |   颜色数   | 64x32屏幕 | 128x64屏幕 | 256x128屏幕 |
| :------: | :--: | :--------: | :-------: | :--------: | :---------: |
|   1BPP   | 1位  |    2色     |   256B    |    1KB     |     4KB     |
|   2BPP   | 2位  |    4色     |   512B    |    2KB     |     8KB     |
|   4BPP   | 4位  |    16色    |    1KB    |    4KB     |    16KB     |
|   8BPP   | 8位  |   256色    |    2KB    |    8KB     |    32KB     |
|  RGB565  | 16位 |   65K色    |    4KB    |    16KB    |    64KB     |
|  RGB888  | 24位 |  16.7M色   |    6KB    |    24KB    |    96KB     |
| ARGB8888 | 32位 | 16.7M+透明 |    8KB    |    32KB    |    128KB    |

------

这个完整的特效库涵盖了：

1. **多色彩深度支持** - 从1位单色到32位真彩色
2. **50+种特效** - 基础变换、过渡、变形、3D、滤镜、粒子等
3. **动画系统** - 20+种缓动函数，完整的动画控制
4. **粒子系统** - 雨、雪、火焰、烟花、星空等
5. **UI组件** - 进度条、加载动画、时钟、频谱等
6. **格式转换** - 支持抖动算法的高质量转换

所有效果都通过统一的`Color`结构和`Matrix`结构实现格式无关的操作！