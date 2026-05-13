## 效果总结表

| 类别 |        效果名称        | 适用场景 |
| :--: | :--------------------: | :------: |
| 基础 |    移动、旋转、翻转    |   通用   |
| 缩放 |    跳入、跳出、弹跳    | 强调显示 |
| 过渡 | 淡入淡出、溶解、百叶窗 | 画面切换 |
| 变形 |   波浪、漩涡、水波纹   | 动态效果 |
|  3D  |    翻转、立方体旋转    | 高级展示 |
| 粒子 |    雨滴、雪花、火焰    | 氛围营造 |

### 1. 基础数据结构

```c
// 点阵数据结构
typedef struct {
    uint32_t *data;      // 像素数据（可以是1bit或32bit）
    int width;           // 宽度
    int height;          // 高度
    int bpp;             // 每像素位数 (1 或 32)
} Matrix;

// 获取像素
uint32_t getPixel(Matrix *m, int x, int y) {
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) return 0;
    return m->data[y * m->width + x];
}

// 设置像素
void setPixel(Matrix *m, int x, int y, uint32_t color) {
    if (x < 0 || x >= m->width || y < 0 || y >= m->height) return;
    m->data[y * m->width + x] = color;
}
```

------

### 2. 移动效果（上下左右）

```c
// 向左移动
void scrollLeft(Matrix *src, Matrix *dst, int offset) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int srcX = x + offset;
            if (srcX >= 0 && srcX < src->width) {
                setPixel(dst, x, y, getPixel(src, srcX, y));
            } else {
                setPixel(dst, x, y, 0); // 空白填充
            }
        }
    }
}

// 向右移动
void scrollRight(Matrix *src, Matrix *dst, int offset) {
    scrollLeft(src, dst, -offset);
}

// 向上移动
void scrollUp(Matrix *src, Matrix *dst, int offset) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int srcY = y + offset;
            if (srcY >= 0 && srcY < src->height) {
                setPixel(dst, x, y, getPixel(src, x, srcY));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}

// 向下移动
void scrollDown(Matrix *src, Matrix *dst, int offset) {
    scrollUp(src, dst, -offset);
}

// 循环滚动（以左移为例）
void scrollLeftLoop(Matrix *src, Matrix *dst, int offset) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int srcX = (x + offset) % src->width;
            if (srcX < 0) srcX += src->width;
            setPixel(dst, x, y, getPixel(src, srcX, y));
        }
    }
}
```

------

### 3. 旋转效果

```c
#include <math.h>

// 90度旋转（顺时针）
void rotate90CW(Matrix *src, Matrix *dst) {
    // dst尺寸应为 width=src->height, height=src->width
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int newX = src->height - 1 - y;
            int newY = x;
            setPixel(dst, newX, newY, getPixel(src, x, y));
        }
    }
}

// 90度旋转（逆时针）
void rotate90CCW(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int newX = y;
            int newY = src->width - 1 - x;
            setPixel(dst, newX, newY, getPixel(src, x, y));
        }
    }
}

// 180度旋转
void rotate180(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, src->width - 1 - x, src->height - 1 - y, 
                     getPixel(src, x, y));
        }
    }
}

// 任意角度旋转（绕中心点）
void rotateAngle(Matrix *src, Matrix *dst, float angleDeg) {
    float angleRad = angleDeg * M_PI / 180.0f;
    float cosA = cos(angleRad);
    float sinA = sin(angleRad);
    
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    // 清空目标
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            // 反向映射：从目标找源
            float dx = x - cx;
            float dy = y - cy;
            
            int srcX = (int)(dx * cosA + dy * sinA + cx + 0.5f);
            int srcY = (int)(-dx * sinA + dy * cosA + cy + 0.5f);
            
            if (srcX >= 0 && srcX < src->width && 
                srcY >= 0 && srcY < src->height) {
                setPixel(dst, x, y, getPixel(src, srcX, srcY));
            }
        }
    }
}
```

------

### 4. 跳入效果（Zoom In）

```c
// 从中心放大跳入
void zoomIn(Matrix *src, Matrix *dst, float scale) {
    // scale: 0.0 -> 1.0 (从无到完整显示)
    if (scale <= 0) return;
    
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            // 反向映射
            int srcX = (int)((x - cx) / scale + cx);
            int srcY = (int)((y - cy) / scale + cy);
            
            if (srcX >= 0 && srcX < src->width && 
                srcY >= 0 && srcY < src->height) {
                setPixel(dst, x, y, getPixel(src, srcX, srcY));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}

// 跳入动画帧生成
void animateZoomIn(Matrix *src, Matrix *dst, int frame, int totalFrames) {
    float scale = (float)(frame + 1) / totalFrames;
    zoomIn(src, dst, scale);
}
```

------

### 5. 跳出效果（Zoom Out）

```c
// 从中心缩小跳出
void zoomOut(Matrix *src, Matrix *dst, float scale) {
    // scale: 1.0 -> 0.0 (从完整到消失)
    if (scale <= 0) {
        // 清空
        for (int i = 0; i < dst->width * dst->height; i++) {
            dst->data[i] = 0;
        }
        return;
    }
    
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    // 先清空
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    // 正向映射
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int dstX = (int)((x - cx) * scale + cx);
            int dstY = (int)((y - cy) * scale + cy);
            
            if (dstX >= 0 && dstX < dst->width && 
                dstY >= 0 && dstY < dst->height) {
                setPixel(dst, dstX, dstY, getPixel(src, x, y));
            }
        }
    }
}
```

------

### 6. 淡入淡出效果（适用于32bit彩色）

```c
// 淡入效果
void fadeIn(Matrix *src, Matrix *dst, float alpha) {
    // alpha: 0.0 -> 1.0
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            uint32_t pixel = getPixel(src, x, y);
            
            uint8_t r = ((pixel >> 16) & 0xFF) * alpha;
            uint8_t g = ((pixel >> 8) & 0xFF) * alpha;
            uint8_t b = (pixel & 0xFF) * alpha;
            uint8_t a = ((pixel >> 24) & 0xFF) * alpha;
            
            setPixel(dst, x, y, (a << 24) | (r << 16) | (g << 8) | b);
        }
    }
}

// 淡出效果
void fadeOut(Matrix *src, Matrix *dst, float alpha) {
    fadeIn(src, dst, 1.0f - alpha);
}
```

------

### 7. 百叶窗效果

```c
// 水平百叶窗
void blindsHorizontal(Matrix *src, Matrix *dst, int step, int blindSize) {
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    for (int y = 0; y < src->height; y++) {
        int blindIndex = y / blindSize;
        int posInBlind = y % blindSize;
        
        if (posInBlind < step) {
            for (int x = 0; x < src->width; x++) {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}

// 垂直百叶窗
void blindsVertical(Matrix *src, Matrix *dst, int step, int blindSize) {
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    for (int x = 0; x < src->width; x++) {
        int blindIndex = x / blindSize;
        int posInBlind = x % blindSize;
        
        if (posInBlind < step) {
            for (int y = 0; y < src->height; y++) {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}
```

------

### 8. 擦除效果（Wipe）

```c
// 从左向右擦除显示
void wipeRight(Matrix *src, Matrix *dst, int position) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            if (x <= position) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}

// 从上向下擦除显示
void wipeDown(Matrix *src, Matrix *dst, int position) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            if (y <= position) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}

// 中心向外扩展
void wipeFromCenter(Matrix *src, Matrix *dst, int radius) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int dx = abs(x - cx);
            int dy = abs(y - cy);
            
            if (dx <= radius && dy <= radius) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}
```

------

### 9. 翻转效果

```c
// 水平翻转
void flipHorizontal(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, src->width - 1 - x, y, getPixel(src, x, y));
        }
    }
}

// 垂直翻转
void flipVertical(Matrix *src, Matrix *dst) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            setPixel(dst, x, src->height - 1 - y, getPixel(src, x, y));
        }
    }
}
```

------

### 10. 动画控制框架

```c
typedef enum {
    EFFECT_SCROLL_LEFT,
    EFFECT_SCROLL_RIGHT,
    EFFECT_SCROLL_UP,
    EFFECT_SCROLL_DOWN,
    EFFECT_ZOOM_IN,
    EFFECT_ZOOM_OUT,
    EFFECT_FADE_IN,
    EFFECT_FADE_OUT,
    EFFECT_ROTATE,
    EFFECT_BLINDS_H,
    EFFECT_BLINDS_V
} EffectType;

void playEffect(Matrix *src, Matrix *dst, EffectType type, 
                int frame, int totalFrames) {
    float progress = (float)frame / totalFrames;
    
    switch (type) {
        case EFFECT_SCROLL_LEFT:
            scrollLeft(src, dst, (int)(src->width * progress));
            break;
        case EFFECT_ZOOM_IN:
            zoomIn(src, dst, progress);
            break;
        case EFFECT_ZOOM_OUT:
            zoomOut(src, dst, 1.0f - progress);
            break;
        case EFFECT_FADE_IN:
            fadeIn(src, dst, progress);
            break;
        case EFFECT_ROTATE:
            rotateAngle(src, dst, 360.0f * progress);
            break;
        // ... 其他效果
    }
}
```

------

这些算法覆盖了点阵屏最常用的显示效果，可以根据实际硬件（单色/彩色）和性能需求进行优化。需要我详细解释某个特定效果或提供其他语言版本吗？





```
还有其他特效效果可添加的麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

当然！点阵屏还有很多炫酷的特效可以实现。我来补充更多高级效果算法：

## 更多点阵屏特效算法

------

### 11. 波浪效果（Wave）

```c
#include <math.h>

// 水平波浪
void waveHorizontal(Matrix *src, Matrix *dst, float amplitude, float frequency, float phase) {
    for (int y = 0; y < src->height; y++) {
        // 计算该行的水平偏移
        int offsetX = (int)(amplitude * sin(frequency * y + phase));
        
        for (int x = 0; x < src->width; x++) {
            int srcX = x - offsetX;
            // 循环处理或边界处理
            if (srcX < 0) srcX += src->width;
            if (srcX >= src->width) srcX -= src->width;
            
            setPixel(dst, x, y, getPixel(src, srcX, y));
        }
    }
}

// 垂直波浪
void waveVertical(Matrix *src, Matrix *dst, float amplitude, float frequency, float phase) {
    for (int x = 0; x < src->width; x++) {
        int offsetY = (int)(amplitude * sin(frequency * x + phase));
        
        for (int y = 0; y < src->height; y++) {
            int srcY = y - offsetY;
            if (srcY < 0) srcY += src->height;
            if (srcY >= src->height) srcY -= src->height;
            
            setPixel(dst, x, y, getPixel(src, x, srcY));
        }
    }
}

// 水波纹效果（从中心扩散）
void rippleEffect(Matrix *src, Matrix *dst, float amplitude, float wavelength, float phase) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            
            // 计算位移
            float offset = amplitude * sin(dist / wavelength * 2 * M_PI - phase);
            
            int srcX = x + (int)(offset * dx / (dist + 0.001f));
            int srcY = y + (int)(offset * dy / (dist + 0.001f));
            
            // 边界检查
            srcX = (srcX < 0) ? 0 : (srcX >= src->width) ? src->width - 1 : srcX;
            srcY = (srcY < 0) ? 0 : (srcY >= src->height) ? src->height - 1 : srcY;
            
            setPixel(dst, x, y, getPixel(src, srcX, srcY));
        }
    }
}
```

------

### 12. 马赛克/像素化效果

```c
// 马赛克效果
void mosaic(Matrix *src, Matrix *dst, int blockSize) {
    for (int by = 0; by < src->height; by += blockSize) {
        for (int bx = 0; bx < src->width; bx += blockSize) {
            // 计算块内平均颜色（或取中心点）
            uint32_t color = getPixel(src, 
                bx + blockSize / 2, 
                by + blockSize / 2);
            
            // 如果是32bit彩色，可以计算平均值
            // uint32_t color = getBlockAverage(src, bx, by, blockSize);
            
            // 填充整个块
            for (int y = by; y < by + blockSize && y < src->height; y++) {
                for (int x = bx; x < bx + blockSize && x < src->width; x++) {
                    setPixel(dst, x, y, color);
                }
            }
        }
    }
}

// 动态马赛克（从模糊到清晰）
void mosaicTransition(Matrix *src, Matrix *dst, int frame, int totalFrames) {
    int maxBlock = (src->width > src->height ? src->width : src->height) / 2;
    int blockSize = maxBlock - (maxBlock - 1) * frame / totalFrames;
    if (blockSize < 1) blockSize = 1;
    mosaic(src, dst, blockSize);
}
```

------

### 13. 随机溶解效果（Dissolve）

```c
// 简单伪随机数生成
uint32_t simpleRandom(uint32_t seed) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// 随机溶解显示
void dissolveIn(Matrix *src, Matrix *dst, int frame, int totalFrames, uint32_t seed) {
    int totalPixels = src->width * src->height;
    int pixelsToShow = totalPixels * frame / totalFrames;
    
    // 先清空
    for (int i = 0; i < totalPixels; i++) {
        dst->data[i] = 0;
    }
    
    // 使用随机序列决定显示顺序
    uint32_t rnd = seed;
    for (int i = 0; i < pixelsToShow; i++) {
        rnd = simpleRandom(rnd);
        int idx = rnd % totalPixels;
        int x = idx % src->width;
        int y = idx / src->width;
        setPixel(dst, x, y, getPixel(src, x, y));
    }
}

// 有序溶解（基于抖动矩阵）
static const int ditherMatrix4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

void orderedDissolve(Matrix *src, Matrix *dst, int threshold) {
    // threshold: 0-16
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int ditherValue = ditherMatrix4x4[(y % 4) * 4 + (x % 4)];
            if (ditherValue < threshold) {
                setPixel(dst, x, y, getPixel(src, x, y));
            } else {
                setPixel(dst, x, y, 0);
            }
        }
    }
}
```

------

### 14. 旋涡/漩涡效果（Swirl）

```c
void swirlEffect(Matrix *src, Matrix *dst, float strength, float radius) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist < radius) {
                // 计算旋转角度（距离中心越近旋转越大）
                float angle = strength * (radius - dist) / radius;
                
                float cosA = cos(angle);
                float sinA = sin(angle);
                
                int srcX = (int)(cosA * dx - sinA * dy + cx);
                int srcY = (int)(sinA * dx + cosA * dy + cy);
                
                if (srcX >= 0 && srcX < src->width && 
                    srcY >= 0 && srcY < src->height) {
                    setPixel(dst, x, y, getPixel(src, srcX, srcY));
                } else {
                    setPixel(dst, x, y, 0);
                }
            } else {
                setPixel(dst, x, y, getPixel(src, x, y));
            }
        }
    }
}
```

------

### 15. 镜像分割效果

```c
// 四象限镜像
void mirrorQuadrant(Matrix *src, Matrix *dst) {
    int hw = src->width / 2;
    int hh = src->height / 2;
    
    for (int y = 0; y < hh; y++) {
        for (int x = 0; x < hw; x++) {
            uint32_t pixel = getPixel(src, x, y);
            
            // 四个象限
            setPixel(dst, x, y, pixel);                           // 左上
            setPixel(dst, src->width - 1 - x, y, pixel);          // 右上
            setPixel(dst, x, src->height - 1 - y, pixel);         // 左下
            setPixel(dst, src->width - 1 - x, src->height - 1 - y, pixel); // 右下
        }
    }
}

// 万花筒效果
void kaleidoscope(Matrix *src, Matrix *dst, int segments) {
    int cx = src->width / 2;
    int cy = src->height / 2;
    float segmentAngle = 2 * M_PI / segments;
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dx = x - cx;
            float dy = y - cy;
            
            float angle = atan2(dy, dx);
            if (angle < 0) angle += 2 * M_PI;
            
            float dist = sqrt(dx * dx + dy * dy);
            
            // 映射到第一个扇区
            float newAngle = fmod(angle, segmentAngle);
            // 镜像处理
            int segment = (int)(angle / segmentAngle);
            if (segment % 2 == 1) {
                newAngle = segmentAngle - newAngle;
            }
            
            int srcX = (int)(cx + dist * cos(newAngle));
            int srcY = (int)(cy + dist * sin(newAngle));
            
            if (srcX >= 0 && srcX < src->width && 
                srcY >= 0 && srcY < src->height) {
                setPixel(dst, x, y, getPixel(src, srcX, srcY));
            }
        }
    }
}
```

------

### 16. 雨滴/雪花下落效果

```c
#define MAX_PARTICLES 100

typedef struct {
    float x, y;
    float speed;
    float size;
    uint32_t color;
    int active;
} Particle;

Particle particles[MAX_PARTICLES];

// 初始化粒子
void initParticles(int width, int height, uint32_t color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].x = rand() % width;
        particles[i].y = rand() % height;
        particles[i].speed = 1 + (rand() % 3);
        particles[i].size = 1;
        particles[i].color = color;
        particles[i].active = 1;
    }
}

// 更新并绘制雨滴
void updateRain(Matrix *dst, int width, int height) {
    // 先清空或保留背景
    for (int i = 0; i < width * height; i++) {
        dst->data[i] = 0;
    }
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // 更新位置
        particles[i].y += particles[i].speed;
        
        // 超出边界重置
        if (particles[i].y >= height) {
            particles[i].y = 0;
            particles[i].x = rand() % width;
        }
        
        // 绘制
        int x = (int)particles[i].x;
        int y = (int)particles[i].y;
        setPixel(dst, x, y, particles[i].color);
        
        // 雨滴拖尾
        if (y > 0) setPixel(dst, x, y - 1, particles[i].color / 2);
    }
}

// 雪花效果（带左右飘动）
void updateSnow(Matrix *dst, int width, int height, float windPhase) {
    for (int i = 0; i < width * height; i++) {
        dst->data[i] = 0;
    }
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        // 更新位置（带风向摆动）
        particles[i].y += particles[i].speed;
        particles[i].x += sin(windPhase + particles[i].y * 0.1f) * 0.5f;
        
        // 边界处理
        if (particles[i].y >= height) {
            particles[i].y = 0;
            particles[i].x = rand() % width;
        }
        if (particles[i].x < 0) particles[i].x = width - 1;
        if (particles[i].x >= width) particles[i].x = 0;
        
        setPixel(dst, (int)particles[i].x, (int)particles[i].y, particles[i].color);
    }
}
```

------

### 17. 扫描线效果

```c
// CRT扫描线效果
void scanlineEffect(Matrix *src, Matrix *dst, int lineSpacing, float darkness) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            uint32_t pixel = getPixel(src, x, y);
            
            if (y % lineSpacing == 0) {
                // 暗线
                if (src->bpp == 32) {
                    uint8_t r = ((pixel >> 16) & 0xFF) * darkness;
                    uint8_t g = ((pixel >> 8) & 0xFF) * darkness;
                    uint8_t b = (pixel & 0xFF) * darkness;
                    pixel = (pixel & 0xFF000000) | (r << 16) | (g << 8) | b;
                } else {
                    pixel = 0;
                }
            }
            setPixel(dst, x, y, pixel);
        }
    }
}

// 移动扫描线
void movingScanline(Matrix *src, Matrix *dst, int scanY, int scanHeight) {
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            uint32_t pixel = getPixel(src, x, y);
            
            // 扫描线区域高亮
            if (y >= scanY && y < scanY + scanHeight) {
                if (src->bpp == 32) {
                    uint8_t r = ((pixel >> 16) & 0xFF);
                    uint8_t g = ((pixel >> 8) & 0xFF);
                    uint8_t b = (pixel & 0xFF);
                    r = (r + 50 > 255) ? 255 : r + 50;
                    g = (g + 50 > 255) ? 255 : g + 50;
                    b = (b + 50 > 255) ? 255 : b + 50;
                    pixel = (pixel & 0xFF000000) | (r << 16) | (g << 8) | b;
                }
            }
            setPixel(dst, x, y, pixel);
        }
    }
}
```

------

### 18. 弹跳效果

```c
// 弹跳缩放
void bounceScale(Matrix *src, Matrix *dst, int frame, int totalFrames) {
    // 使用弹性函数
    float t = (float)frame / totalFrames;
    float scale;
    
    // 弹性缓动
    if (t < 0.5f) {
        scale = 4 * t * t * t;
    } else {
        float f = (2 * t - 2);
        scale = 0.5f * f * f * f + 1;
    }
    
    // 添加弹跳
    scale = 1.0f + 0.2f * sin(t * M_PI * 3) * (1 - t);
    
    zoomIn(src, dst, scale);
}

// 弹跳位移
void bounceMove(Matrix *src, Matrix *dst, int frame, int totalFrames, 
                int startX, int startY, int endX, int endY) {
    float t = (float)frame / totalFrames;
    
    // 弹跳衰减
    float bounce = fabs(sin(t * M_PI * 4)) * (1 - t) * 0.3f;
    
    int currentX = startX + (endX - startX) * t;
    int currentY = startY + (endY - startY) * t - (int)(bounce * src->height);
    
    // 清空
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    // 绘制在新位置
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int nx = x + currentX;
            int ny = y + currentY;
            if (nx >= 0 && nx < dst->width && ny >= 0 && ny < dst->height) {
                setPixel(dst, nx, ny, getPixel(src, x, y));
            }
        }
    }
}
```

------

### 19. 3D翻转效果

```c
// 水平3D翻转（绕Y轴）
void flip3D_Y(Matrix *src, Matrix *dst, float angle) {
    float cosA = cos(angle);
    int cx = src->width / 2;
    
    // 清空
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            // 计算3D投影后的X坐标
            float dx = x - cx;
            int newX = (int)(dx * fabs(cosA) + cx);
            
            // 翻转到背面时镜像
            if (cosA < 0) {
                newX = src->width - 1 - newX;
            }
            
            if (newX >= 0 && newX < dst->width) {
                setPixel(dst, newX, y, getPixel(src, x, y));
            }
        }
    }
}

// 垂直3D翻转（绕X轴）
void flip3D_X(Matrix *src, Matrix *dst, float angle) {
    float cosA = cos(angle);
    int cy = src->height / 2;
    
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            float dy = y - cy;
            int newY = (int)(dy * fabs(cosA) + cy);
            
            if (cosA < 0) {
                newY = src->height - 1 - newY;
            }
            
            if (newY >= 0 && newY < dst->height) {
                setPixel(dst, x, newY, getPixel(src, x, y));
            }
        }
    }
}

// 立方体旋转效果（显示两个面）
void cubeRotate(Matrix *front, Matrix *side, Matrix *dst, float angle) {
    int cx = dst->width / 2;
    float cosA = cos(angle);
    float sinA = sin(angle);
    
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    // 根据角度决定显示哪个面
    Matrix *visible = (cosA > 0) ? front : side;
    float scale = fabs(cosA);
    
    for (int y = 0; y < visible->height; y++) {
        for (int x = 0; x < visible->width; x++) {
            float dx = x - cx;
            int newX = (int)(dx * scale + cx);
            
            if (newX >= 0 && newX < dst->width) {
                setPixel(dst, newX, y, getPixel(visible, x, y));
            }
        }
    }
}
```

------

### 20. 闪烁/呼吸灯效果

```c
// 闪烁效果
void blinkEffect(Matrix *src, Matrix *dst, int frame, int onFrames, int offFrames) {
    int cycle = onFrames + offFrames;
    int pos = frame % cycle;
    
    if (pos < onFrames) {
        // 显示
        memcpy(dst->data, src->data, src->width * src->height * sizeof(uint32_t));
    } else {
        // 隐藏
        memset(dst->data, 0, dst->width * dst->height * sizeof(uint32_t));
    }
}

// 呼吸灯效果（正弦波亮度变化）
void breatheEffect(Matrix *src, Matrix *dst, float phase) {
    float brightness = (sin(phase) + 1.0f) / 2.0f; // 0.0 ~ 1.0
    
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            uint32_t pixel = getPixel(src, x, y);
            
            if (src->bpp == 32) {
                uint8_t a = (pixel >> 24) & 0xFF;
                uint8_t r = ((pixel >> 16) & 0xFF) * brightness;
                uint8_t g = ((pixel >> 8) & 0xFF) * brightness;
                uint8_t b = (pixel & 0xFF) * brightness;
                pixel = (a << 24) | (r << 16) | (g << 8) | b;
            } else {
                pixel = (brightness > 0.5f) ? pixel : 0;
            }
            
            setPixel(dst, x, y, pixel);
        }
    }
}
```

------

### 21. 棋盘格过渡效果

```c
void checkerboardTransition(Matrix *src, Matrix *dst, int blockSize, int frame, int totalFrames) {
    int maxBlocks = (src->width / blockSize + 1) * (src->height / blockSize + 1);
    int blocksToShow = maxBlocks * frame / totalFrames;
    
    // 清空
    for (int i = 0; i < dst->width * dst->height; i++) {
        dst->data[i] = 0;
    }
    
    int count = 0;
    // 先显示偶数格
    for (int by = 0; by < src->height; by += blockSize) {
        for (int bx = 0; bx < src->width; bx += blockSize) {
            int gridX = bx / blockSize;
            int gridY = by / blockSize;
            
            if ((gridX + gridY) % 2 == 0 && count < blocksToShow) {
                for (int y = by; y < by + blockSize && y < src->height; y++) {
                    for (int x = bx; x < bx + blockSize && x < src->width; x++) {
                        setPixel(dst, x, y, getPixel(src, x, y));
                    }
                }
                count++;
            }
        }
    }
    
    // 再显示奇数格
    for (int by = 0; by < src->height; by += blockSize) {
        for (int bx = 0; bx < src->width; bx += blockSize) {
            int gridX = bx / blockSize;
            int gridY = by / blockSize;
            
            if ((gridX + gridY) % 2 == 1 && count < blocksToShow) {
                for (int y = by; y < by + blockSize && y < src->height; y++) {
                    for (int x = bx; x < bx + blockSize && x < src->width; x++) {
                        setPixel(dst, x, y, getPixel(src, x, y));
                    }
                }
                count++;
            }
        }
    }
}
```

------

### 22. 螺旋展开效果

```c
void spiralReveal(Matrix *src, Matrix *dst, int frame, int totalFrames) {
    int totalPixels = src->width * src->height;
    int pixelsToShow = totalPixels * frame / totalFrames;
    
    // 清空
    for (int i = 0; i < totalPixels; i++) {
        dst->data[i] = 0;
    }
    
    int x = 0, y = 0;
    int dx = 1, dy = 0;
    int left = 0, right = src->width - 1;
    int top = 0, bottom = src->height - 1;
    
    for (int i = 0; i < pixelsToShow; i++) {
        setPixel(dst, x, y, getPixel(src, x, y));
        
        // 移动到下一个位置
        if (dx == 1 && x == right) {
            dx = 0; dy = 1; top++;
        } else if (dy == 1 && y == bottom) {
            dx = -1; dy = 0; right--;
        } else if (dx == -1 && x == left) {
            dx = 0; dy = -1; bottom--;
        } else if (dy == -1 && y == top) {
            dx = 1; dy = 0; left++;
        }
        
        x += dx;
        y += dy;
        
        // 边界保护
        if (x < 0 || x >= src->width || y < 0 || y >= src->height) break;
    }
}
```

------

### 23. 火焰效果

```c
uint8_t fireBuffer[2][64][32]; // 双缓冲，假设64x32屏幕

// 火焰调色板
uint32_t firePalette[256];

void initFirePalette() {
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i < 128) ? i * 2 : 255;
        uint8_t g = (i < 128) ? 0 : (i - 128) * 2;
        uint8_t b = 0;
        firePalette[i] = (r << 16) | (g << 8) | b;
    }
}

void updateFire(Matrix *dst, int width, int height) {
    static int bufIdx = 0;
    
    // 底部随机点火
    for (int x = 0; x < width; x++) {
        fireBuffer[bufIdx][x][height - 1] = rand() % 256;
    }
    
    // 火焰向上蔓延
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width; x++) {
            int sum = fireBuffer[bufIdx][(x - 1 + width) % width][y + 1]
                    + fireBuffer[bufIdx][x][y + 1]
                    + fireBuffer[bufIdx][(x + 1) % width][y + 1]
                    + fireBuffer[bufIdx][x][(y + 2) % height];
            
            fireBuffer[1 - bufIdx][x][y] = (sum * 60) / 256; // 衰减
        }
    }
    
    // 渲染到目标
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            setPixel(dst, x, y, firePalette[fireBuffer[1 - bufIdx][x][y]]);
        }
    }
    
    bufIdx = 1 - bufIdx;
}
```

------

### 24. 矩阵雨效果（黑客帝国风格）

```c
typedef struct {
    int x;
    float y;
    float speed;
    int length;
    int active;
} MatrixDrop;

#define MAX_DROPS 50
MatrixDrop drops[MAX_DROPS];

void initMatrixRain(int width) {
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = rand() % width;
        drops[i].y = -(rand() % 20);
        drops[i].speed = 0.5f + (rand() % 100) / 100.0f;
        drops[i].length = 5 + rand() % 15;
        drops[i].active = 1;
    }
}

void updateMatrixRain(Matrix *dst, int width, int height) {
    // 清空（或淡化背景）
    for (int i = 0; i < width * height; i++) {
        uint32_t pixel = dst->data[i];
        if (pixel > 0) {
            // 淡化
            uint8_t r = ((pixel >> 16) & 0xFF) * 0.9f;
            uint8_t g = ((pixel >> 8) & 0xFF) * 0.9f;
            uint8_t b = (pixel & 0xFF) * 0.9f;
            dst->data[i] = (r << 16) | (g << 8) | b;
        }
    }
    
    for (int i = 0; i < MAX_DROPS; i++) {
        if (!drops[i].active) continue;
        
        drops[i].y += drops[i].speed;
        
        // 绘制雨滴
        for (int j = 0; j < drops[i].length; j++) {
            int y = (int)drops[i].y - j;
            if (y >= 0 && y < height) {
                // 头部最亮，尾部渐暗
                uint8_t brightness = 255 - (j * 255 / drops[i].length);
                uint32_t color = (brightness << 8); // 绿色
                if (j == 0) color = 0xFFFFFF; // 头部白色
                setPixel(dst, drops[i].x, y, color);
            }
        }
        
        // 重置
        if (drops[i].y - drops[i].length > height) {
            drops[i].y = -(rand() % 20);
            drops[i].x = rand() % width;
            drops[i].length = 5 + rand() % 15;
        }
    }
}
```

------

### 25. 效果组合器

```c
typedef void (*EffectFunc)(Matrix*, Matrix*, float);

// 效果链
typedef struct {
    EffectFunc effects[10];
    int count;
} EffectChain;

void applyEffectChain(EffectChain *chain, Matrix *src, Matrix *dst, float progress) {
    Matrix temp1, temp2;
    // 分配临时缓冲区...
    
    Matrix *current = src;
    Matrix *next = &temp1;
    
    for (int i = 0; i < chain->count; i++) {
        chain->effects[i](current, next, progress);
        
        // 交换缓冲区
        Matrix *swap = current;
        current = next;
        next = (swap == src) ? &temp2 : swap;
    }
    
    // 复制结果到dst
    memcpy(dst->data, current->data, dst->width * dst->height * sizeof(uint32_t));
}
```
