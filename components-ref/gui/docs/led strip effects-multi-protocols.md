# 综合LED灯效库 - 整合 WS2812FX / WLED / HyperSerial 协议

这是一个综合性的LED灯效库，整合了多个流行项目的效果和协议支持。

## 1. 项目架构总览

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                              应用层 (Application)                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                           协议层 (Protocol Layer)                            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │  Adalight   │ │ HyperSerial │ │    TPM2     │ │    WLED     │           │
│  │  Protocol   │ │  Protocol   │ │  Protocol   │ │    JSON     │           │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘           │
├─────────────────────────────────────────────────────────────────────────────┤
│                           效果层 (Effects Layer)                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                    180+ 效果 (WS2812FX + WLED 合并)                    │  │
│  ├───────────┬───────────┬───────────┬───────────┬───────────────────────┤  │
│  │  基础效果  │  彩虹效果  │  追逐效果  │  火焰效果  │  WLED特有效果        │  │
│  │  0-19     │  20-39    │  40-59    │  60-79    │  80-179              │  │
│  └───────────┴───────────┴───────────┴───────────┴───────────────────────┘  │
├─────────────────────────────────────────────────────────────────────────────┤
│                           核心层 (Core Layer)                                │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │StripManager │ │  LedStrip   │ │  Segment    │ │  Palette    │           │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘           │
├─────────────────────────────────────────────────────────────────────────────┤
│                           硬件抽象层 (HAL)                                   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │ STM32 SPI   │ │ ESP32 RMT   │ │ ESP32 I2S   │ │  Bit-Bang   │           │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘           │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 2. 核心头文件 (led_core.h)

```c
#ifndef LED_CORE_H
#define LED_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * 版本信息
 *============================================================================*/
#define LED_LIB_VERSION         "2.0.0"
#define LED_LIB_VERSION_MAJOR   2
#define LED_LIB_VERSION_MINOR   0
#define LED_LIB_VERSION_PATCH   0

/*============================================================================
 * 配置宏
 *============================================================================*/
#define MAX_STRIPS              8
#define MAX_SEGMENTS            32
#define MAX_LEDS_PER_STRIP      1500
#define MAX_PALETTES            16
#define PALETTE_SIZE            16

// 功能开关
#define ENABLE_WLED_EFFECTS     1
#define ENABLE_HYPERSERIAL      1
#define ENABLE_ADALIGHT         1
#define ENABLE_TPM2             1
#define ENABLE_ARTNET           1
#define ENABLE_E131             1
#define ENABLE_DDP              1

/*============================================================================
 * 色彩顺序枚举
 *============================================================================*/
typedef enum {
    COLOR_ORDER_RGB = 0,
    COLOR_ORDER_RBG,
    COLOR_ORDER_GRB,        // WS2812 默认
    COLOR_ORDER_GBR,
    COLOR_ORDER_BRG,
    COLOR_ORDER_BGR,
    COLOR_ORDER_RGBW,       // SK6812 RGBW
    COLOR_ORDER_GRBW,       // SK6812 GRBW
    COLOR_ORDER_RGBWW,      // RGB + 暖白 + 冷白
    COLOR_ORDER_MAX
} ColorOrder_t;

/*============================================================================
 * LED芯片类型
 *============================================================================*/
typedef enum {
    LED_TYPE_WS2812B = 0,
    LED_TYPE_WS2811,
    LED_TYPE_WS2813,
    LED_TYPE_WS2815,
    LED_TYPE_SK6812,
    LED_TYPE_SK6812_RGBW,
    LED_TYPE_APA102,        // SPI时钟型
    LED_TYPE_APA104,
    LED_TYPE_SK9822,
    LED_TYPE_TM1814,
    LED_TYPE_TM1829,
    LED_TYPE_UCS1903,
    LED_TYPE_UCS2903,
    LED_TYPE_LPD8806,
    LED_TYPE_P9813,
    LED_TYPE_DOTSTAR,
    LED_TYPE_MAX
} LedType_t;

/*============================================================================
 * 颜色结构体
 *============================================================================*/
typedef union {
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t w;
    };
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t white;
    };
    uint32_t raw;
    uint8_t  bytes[4];
} Color_t;

/*============================================================================
 * 颜色宏定义
 *============================================================================*/
#define COLOR(r,g,b)        ((Color_t){.red=(r), .green=(g), .blue=(b), .white=0})
#define COLORW(r,g,b,w)     ((Color_t){.red=(r), .green=(g), .blue=(b), .white=(w)})
#define COLOR_HEX(hex)      ((Color_t){.red=((hex)>>16)&0xFF, .green=((hex)>>8)&0xFF, .blue=(hex)&0xFF, .white=0})

#define BLACK               COLOR(0, 0, 0)
#define WHITE               COLOR(255, 255, 255)
#define RED                 COLOR(255, 0, 0)
#define GREEN               COLOR(0, 255, 0)
#define BLUE                COLOR(0, 0, 255)
#define YELLOW              COLOR(255, 255, 0)
#define CYAN                COLOR(0, 255, 255)
#define MAGENTA             COLOR(255, 0, 255)
#define ORANGE              COLOR(255, 128, 0)
#define PURPLE              COLOR(128, 0, 255)
#define PINK                COLOR(255, 105, 180)
#define LIME                COLOR(128, 255, 0)
#define AQUA                COLOR(0, 255, 128)
#define GOLD                COLOR(255, 215, 0)
#define CORAL               COLOR(255, 127, 80)
#define NAVY                COLOR(0, 0, 128)
#define TEAL                COLOR(0, 128, 128)
#define MAROON              COLOR(128, 0, 0)
#define OLIVE               COLOR(128, 128, 0)
#define SILVER              COLOR(192, 192, 192)

/*============================================================================
 * 效果参数结构体
 *============================================================================*/
typedef struct {
    Color_t     colors[3];      // 最多3个颜色
    uint16_t    speed;          // 速度 (0-65535, 映射到实际ms)
    uint16_t    intensity;      // 强度 (0-255)
    uint8_t     palette;        // 调色板索引
    uint8_t     brightness;     // 亮度 (0-255)
    uint8_t     custom1;        // 自定义参数1
    uint8_t     custom2;        // 自定义参数2
    uint8_t     custom3;        // 自定义参数3
    bool        reverse;        // 反向
    bool        mirror;         // 镜像
    bool        gamma;          // Gamma校正
} EffectParams_t;

/*============================================================================
 * 线段选项 (WLED风格)
 *============================================================================*/
typedef struct {
    uint8_t     selected    : 1;    // 是否选中
    uint8_t     reverse     : 1;    // 反向
    uint8_t     on          : 1;    // 开启
    uint8_t     mirror      : 1;    // 镜像
    uint8_t     freeze      : 1;    // 冻结
    uint8_t     reset       : 1;    // 重置
    uint8_t     transitional: 1;    // 过渡中
    uint8_t     reversed_y  : 1;    // Y轴反向 (2D)
} SegmentOptions_t;

/*============================================================================
 * 线段结构体 (WLED兼容)
 *============================================================================*/
typedef struct {
    uint16_t        start;          // 起始LED
    uint16_t        stop;           // 结束LED (不包含)
    uint16_t        offset;         // 偏移
    uint8_t         grouping;       // 分组
    uint8_t         spacing;        // 间隔
    
    uint8_t         mode;           // 效果模式
    EffectParams_t  params;         // 效果参数
    SegmentOptions_t options;       // 选项
    
    // 效果状态
    uint32_t        step;           // 当前步骤
    uint32_t        call;           // 调用计数
    uint32_t        last_call;      // 上次调用时间
    uint16_t        aux0;           // 辅助变量0
    uint16_t        aux1;           // 辅助变量1
    
    // 2D支持
    uint8_t         start_y;        // Y起始 (2D矩阵)
    uint8_t         stop_y;         // Y结束 (2D矩阵)
    
    // 过渡
    uint8_t         trans_effect;   // 过渡效果
    uint16_t        trans_dur;      // 过渡时长
    
    char            name[16];       // 线段名称
} Segment_t;

/*============================================================================
 * 灯带配置
 *============================================================================*/
typedef struct {
    LedType_t       type;           // LED类型
    ColorOrder_t    color_order;    // 色彩顺序
    uint16_t        count;          // LED数量
    uint8_t         pin;            // GPIO引脚
    uint8_t         pin_clk;        // 时钟引脚 (SPI型)
    bool            skip_first;     // 跳过第一个LED
    uint16_t        refresh_rate;   // 刷新率 (Hz)
    uint8_t         milliamps_per_led;  // 每LED电流 (mA)
} StripConfig_t;

/*============================================================================
 * 灯带结构体
 *============================================================================*/
typedef struct {
    uint8_t         id;
    StripConfig_t   config;
    
    Color_t*        leds;           // LED颜色数组
    uint8_t*        buffer;         // 输出缓冲区
    uint16_t        buffer_size;
    
    Segment_t*      segments;       // 线段数组
    uint8_t         num_segments;
    uint8_t         max_segments;
    
    uint8_t         brightness;     // 全局亮度
    bool            power_on;       // 电源状态
    bool            dirty;          // 需要更新
    
    // 电流限制
    uint16_t        max_milliamps;
    uint16_t        current_milliamps;
    
    // 回调函数
    void (*output_func)(uint8_t* data, uint16_t len, void* user_data);
    void* user_data;
    
    // 统计
    uint32_t        fps;
    uint32_t        frame_count;
    uint32_t        last_fps_time;
} LedStrip_t;

/*============================================================================
 * 灯带管理器
 *============================================================================*/
typedef struct {
    LedStrip_t*     strips[MAX_STRIPS];
    uint8_t         num_strips;
    
    // 时间函数
    uint32_t        (*millis)(void);
    
    // 全局设置
    uint8_t         global_brightness;
    bool            sync_enabled;
    
    // 协议处理器
    void*           protocol_handlers[8];
} StripManager_t;

/*============================================================================
 * API函数声明
 *============================================================================*/

// 管理器
StripManager_t* strip_manager_create(uint32_t (*millis_func)(void));
void strip_manager_destroy(StripManager_t* mgr);
void strip_manager_update(StripManager_t* mgr);

// 灯带
LedStrip_t* led_strip_create(StripConfig_t* config);
void led_strip_destroy(LedStrip_t* strip);
bool led_strip_add_to_manager(StripManager_t* mgr, LedStrip_t* strip);
void led_strip_set_output(LedStrip_t* strip, 
                          void (*func)(uint8_t*, uint16_t, void*), 
                          void* user_data);

// 线段
int8_t segment_add(LedStrip_t* strip, uint16_t start, uint16_t stop);
bool segment_remove(LedStrip_t* strip, uint8_t seg_id);
bool segment_set_mode(LedStrip_t* strip, uint8_t seg_id, uint8_t mode);
bool segment_set_colors(LedStrip_t* strip, uint8_t seg_id, 
                        Color_t c1, Color_t c2, Color_t c3);
bool segment_set_palette(LedStrip_t* strip, uint8_t seg_id, uint8_t palette);
bool segment_set_speed(LedStrip_t* strip, uint8_t seg_id, uint16_t speed);
bool segment_set_intensity(LedStrip_t* strip, uint8_t seg_id, uint16_t intensity);

// 颜色操作
void led_set_pixel(LedStrip_t* strip, uint16_t index, Color_t color);
Color_t led_get_pixel(LedStrip_t* strip, uint16_t index);
void led_fill(LedStrip_t* strip, Color_t color);
void led_fill_range(LedStrip_t* strip, uint16_t start, uint16_t count, Color_t color);
void led_clear(LedStrip_t* strip);

// 亮度
void led_strip_set_brightness(LedStrip_t* strip, uint8_t brightness);
void led_strip_set_power(LedStrip_t* strip, bool on);

// 显示
void led_strip_show(LedStrip_t* strip);

#endif // LED_CORE_H
```

## 3. 效果定义 (led_effects.h)

```c
#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include "led_core.h"

/*============================================================================
 * 效果模式枚举 (WS2812FX + WLED 合并, 180+ 效果)
 *============================================================================*/
typedef enum {
    // ==================== 基础效果 (0-19) ====================
    FX_SOLID = 0,               // 纯色
    FX_BLINK,                   // 闪烁
    FX_BREATH,                  // 呼吸
    FX_COLOR_WIPE,              // 颜色擦除
    FX_COLOR_WIPE_RANDOM,       // 随机颜色擦除
    FX_RANDOM_COLOR,            // 随机颜色
    FX_COLOR_SWEEP,             // 颜色扫描
    FX_DYNAMIC,                 // 动态
    FX_RAINBOW,                 // 彩虹
    FX_RAINBOW_CYCLE,           // 彩虹循环
    FX_SCAN,                    // 扫描
    FX_DUAL_SCAN,               // 双向扫描
    FX_FADE,                    // 渐变
    FX_THEATER_CHASE,           // 剧院追逐
    FX_THEATER_CHASE_RAINBOW,   // 剧院追逐彩虹
    FX_RUNNING_LIGHTS,          // 跑马灯
    FX_SAW,                     // 锯齿波
    FX_TWINKLE,                 // 闪烁
    FX_DISSOLVE,                // 溶解
    FX_DISSOLVE_RANDOM,         // 随机溶解
    
    // ==================== 闪烁效果 (20-39) ====================
    FX_SPARKLE,                 // 火花
    FX_FLASH_SPARKLE,           // 闪光火花
    FX_HYPER_SPARKLE,           // 超级火花
    FX_STROBE,                  // 频闪
    FX_STROBE_RAINBOW,          // 彩虹频闪
    FX_MULTI_STROBE,            // 多重频闪
    FX_BLINK_RAINBOW,           // 彩虹闪烁
    FX_ANDROID,                 // 安卓加载
    FX_CHASE_COLOR,             // 颜色追逐
    FX_CHASE_RANDOM,            // 随机追逐
    FX_CHASE_RAINBOW,           // 彩虹追逐
    FX_CHASE_FLASH,             // 闪光追逐
    FX_CHASE_FLASH_RANDOM,      // 随机闪光追逐
    FX_CHASE_RAINBOW_WHITE,     // 彩虹白色追逐
    FX_COLORFUL,                // 多彩
    FX_TRAFFIC_LIGHT,           // 交通灯
    FX_COLOR_SWEEP_RANDOM,      // 随机颜色扫描
    FX_RUNNING_COLOR,           // 跑动颜色
    FX_AURORA,                  // 极光
    FX_RUNNING_RANDOM,          // 随机跑动
    
    // ==================== 追逐效果 (40-59) ====================
    FX_LARSON_SCANNER,          // Larson扫描器
    FX_COMET,                   // 彗星
    FX_FIREWORKS,               // 烟花
    FX_RAIN,                    // 雨滴
    FX_TETRIX,                  // 俄罗斯方块
    FX_FIRE_FLICKER,            // 火焰闪烁
    FX_GRADIENT,                // 渐变
    FX_LOADING,                 // 加载
    FX_POLICE,                  // 警灯
    FX_FAIRY,                   // 仙女
    FX_TWO_DOTS,                // 双点
    FX_FAIRYTWINKLE,            // 仙女闪烁
    FX_RUNNING_DUAL,            // 双向跑动
    FX_HALLOWEEN,               // 万圣节
    FX_CHASE_3,                 // 三色追逐
    FX_TRI_WIPE,                // 三色擦除
    FX_TRI_FADE,                // 三色渐变
    FX_LIGHTNING,               // 闪电
    FX_ICU,                     // ICU
    FX_MULTI_COMET,             // 多彗星
    
    // ==================== 火焰效果 (60-79) ====================
    FX_DUAL_LARSON,             // 双Larson
    FX_RANDOM_CHASE,            // 随机追逐
    FX_OSCILLATE,               // 振荡
    FX_PRIDE_2015,              // Pride 2015
    FX_JUGGLE,                  // 杂耍
    FX_PALETTE,                 // 调色板
    FX_FIRE_2012,               // Fire 2012
    FX_COLORWAVES,              // 颜色波浪
    FX_BPM,                     // BPM
    FX_FILL_NOISE,              // 噪声填充
    FX_NOISE_1,                 // 噪声1
    FX_NOISE_2,                 // 噪声2
    FX_NOISE_3,                 // 噪声3
    FX_NOISE_4,                 // 噪声4
    FX_COLORTWINKLE,            // 颜色闪烁
    FX_LAKE,                    // 湖泊
    FX_METEOR,                  // 流星
    FX_METEOR_SMOOTH,           // 平滑流星
    FX_RAILWAY,                 // 铁路
    FX_RIPPLE,                  // 涟漪
    
    // ==================== WLED特有效果 (80-119) ====================
    FX_TWINKLEFOX,              // TwinkleFOX
    FX_TWINKLECAT,              // TwinkleCat
    FX_HALLOWEEN_EYES,          // 万圣节眼睛
    FX_SOLID_PATTERN,           // 固体图案
    FX_SOLID_PATTERN_TRI,       // 三色固体图案
    FX_SPOTS,                   // 光斑
    FX_SPOTS_FADE,              // 光斑渐隐
    FX_GLITTER,                 // 闪光
    FX_CANDLE,                  // 蜡烛
    FX_CANDLE_MULTI,            // 多蜡烛
    FX_STARBURST,               // 星爆
    FX_EXPLODING_FIREWORKS,     // 爆炸烟花
    FX_BOUNCINGBALLS,           // 弹跳球
    FX_SINELON,                 // 正弦波
    FX_SINELON_DUAL,            // 双正弦波
    FX_SINELON_RAINBOW,         // 彩虹正弦波
    FX_POPCORN,                 // 爆米花
    FX_DRIP,                    // 水滴
    FX_PLASMA,                  // 等离子
    FX_PERCENT,                 // 百分比
    FX_RIPPLE_RAINBOW,          // 彩虹涟漪
    FX_HEARTBEAT,               // 心跳
    FX_PACIFICA,                // 太平洋
    FX_CANDLE_FIRE,             // 蜡烛火焰
    FX_SOLID_GLITTER,           // 闪光固体
    FX_SUNRISE,                 // 日出
    FX_PHASED,                  // 相位
    FX_TWINKLEUP,               // 向上闪烁
    FX_NOISEPAL,                // 噪声调色板
    FX_SINEWAVE,                // 正弦波
    FX_PHASEDNOISE,             // 相位噪声
    FX_FLOW,                    // 流动
    FX_CHUNCHUN,                // ChunChun
    FX_DANCING_SHADOWS,         // 舞动阴影
    FX_WASHING_MACHINE,         // 洗衣机
    FX_CANDY_CANE,              // 糖果棒
    FX_BLENDS,                  // 混合
    FX_TV_SIMULATOR,            // 电视模拟
    FX_DYNAMIC_SMOOTH,          // 平滑动态
    
    // ==================== 2D效果 (120-159) ====================
    FX_2D_SPACESHIPS,           // 2D太空船
    FX_2D_CRAZYBEES,            // 2D疯狂蜜蜂
    FX_2D_GHOST_RIDER,          // 2D幽灵骑士
    FX_2D_BLOBS,                // 2D斑点
    FX_2D_SCROLLING_TEXT,       // 2D滚动文字
    FX_2D_DRIFT_ROSE,           // 2D漂移玫瑰
    FX_2D_DISTORTION_WAVES,     // 2D扭曲波浪
    FX_2D_SOAP,                 // 2D肥皂泡
    FX_2D_OCTOPUS,              // 2D章鱼
    FX_2D_WAVING_CELL,          // 2D波动单元
    FX_2D_AKEMI,                // 2D Akemi
    FX_2D_COLORED_BURSTS,       // 2D彩色爆发
    FX_2D_JULIA,                // 2D Julia分形
    FX_2D_GAME_OF_LIFE,         // 2D生命游戏
    FX_2D_TARTAN,               // 2D格子呢
    FX_2D_POLAR_LIGHTS,         // 2D极光
    FX_2D_SWIRL,                // 2D漩涡
    FX_2D_LISSAJOUS,            // 2D利萨如图形
    FX_2D_FRIZZLES,             // 2D卷曲
    FX_2D_PLASMA_BALL,          // 2D等离子球
    FX_2D_FLOW_STRIPE,          // 2D流动条纹
    FX_2D_HIPHOTIC,             // 2D催眠
    FX_2D_SINDOTS,              // 2D正弦点
    FX_2D_DNA,                  // 2D DNA
    FX_2D_DNA_SPIRAL,           // 2D DNA螺旋
    FX_2D_DRIFT,                // 2D漂移
    FX_2D_FIRENOISE,            // 2D火焰噪声
    FX_2D_SQUARED_SWIRL,        // 2D方形漩涡
    FX_2D_FIRE_2021,            // 2D Fire 2021
    FX_2D_WAVERLY,              // 2D Waverly
    FX_2D_SUN_RADIATION,        // 2D太阳辐射
    FX_2D_BLACK_HOLE,           // 2D黑洞
    FX_2D_ROTOZOOMER,           // 2D旋转缩放
    FX_2D_PULSER,               // 2D脉冲
    FX_2D_SCROLLING_PLAID,      // 2D滚动格子
    FX_2D_MATRIX,               // 2D矩阵
    FX_2D_METABALLS,            // 2D元球
    FX_2D_NOISE_FIRE,           // 2D噪声火焰
    FX_2D_PUDDLES,              // 2D水坑
    FX_2D_PLASMA,               // 2D等离子
    
    // ==================== 音频响应效果 (160-179) ====================
    FX_AUDIO_VOLUMEREACTIVE,    // 音量响应
    FX_AUDIO_FREQMAP,           // 频率映射
    FX_AUDIO_FREQMATRIX,        // 频率矩阵
    FX_AUDIO_FREQPIXELS,        // 频率像素
    FX_AUDIO_FREQWAVE,          // 频率波
    FX_AUDIO_GRAVFREQ,          // 重力频率
    FX_AUDIO_NOISEMOVE,         // 噪声移动
    FX_AUDIO_ROCKTAVES,         // 八度音
    FX_AUDIO_WATERFALL,         // 瀑布
    FX_AUDIO_VU_METER,          // VU表
    FX_AUDIO_SPECTRUM,          // 频谱
    FX_AUDIO_GRAVIMETER,        // 重力计
    FX_AUDIO_GRAVCENTER,        // 重力中心
    FX_AUDIO_GRAVCENTRIC,       // 重力中心2
    FX_AUDIO_PIXELS,            // 音频像素
    FX_AUDIO_MIDNOISE,          // 中频噪声
    FX_AUDIO_DJ_LIGHT,          // DJ灯
    FX_AUDIO_BLURZ,             // 模糊
    FX_AUDIO_PUDDLES,           // 音频水坑
    FX_AUDIO_PLASMOID,          // 等离子体
    
    FX_MODE_COUNT               // 效果总数
} FxMode_t;

/*============================================================================
 * 效果信息结构
 *============================================================================*/
typedef struct {
    const char*     name;           // 效果名称
    uint16_t        default_speed;  // 默认速度
    uint8_t         default_intensity; // 默认强度
    uint8_t         palette_mode;   // 调色板模式 (0=不使用, 1=使用, 2=必须)
    bool            is_2d;          // 是否2D效果
    bool            is_audio;       // 是否音频效果
} FxInfo_t;

/*============================================================================
 * 效果函数类型
 *============================================================================*/
typedef void (*FxFunc_t)(LedStrip_t* strip, Segment_t* seg);

/*============================================================================
 * API函数
 *============================================================================*/

// 效果执行
void fx_execute(LedStrip_t* strip, Segment_t* seg);

// 效果信息
const FxInfo_t* fx_get_info(FxMode_t mode);
const char* fx_get_name(FxMode_t mode);
uint8_t fx_get_count(void);

// 效果切换
FxMode_t fx_next(FxMode_t current);
FxMode_t fx_prev(FxMode_t current);
FxMode_t fx_random(void);

// 效果搜索
int16_t fx_find_by_name(const char* name);

#endif // LED_EFFECTS_H
```

## 4. 协议层定义 (led_protocol.h)

```c
#ifndef LED_PROTOCOL_H
#define LED_PROTOCOL_H

#include "led_core.h"

/*============================================================================
 * 协议类型枚举
 *============================================================================*/
typedef enum {
    PROTO_NONE = 0,
    PROTO_ADALIGHT,         // Adalight/Prismatik
    PROTO_HYPERSERIAL,      // HyperHDR/HyperSerial
    PROTO_TPM2,             // TPM2.net
    PROTO_WLED_UDP,         // WLED UDP (DRGB/DRGBW/DNRGB)
    PROTO_WLED_JSON,        // WLED JSON API
    PROTO_ARTNET,           // Art-Net (DMX over IP)
    PROTO_E131,             // sACN/E1.31
    PROTO_DDP,              // Distributed Display Protocol
    PROTO_SERIAL_JSON,      // 串口JSON
    PROTO_MAX
} ProtocolType_t;

/*============================================================================
 * Adalight 协议
 *============================================================================*/
#define ADALIGHT_MAGIC      "Ada"
#define ADALIGHT_HEADER_LEN 6

typedef struct {
    uint8_t     magic[3];       // "Ada"
    uint8_t     count_hi;       // LED数量高字节
    uint8_t     count_lo;       // LED数量低字节
    uint8_t     checksum;       // 校验和
} AdalightHeader_t;

/*============================================================================
 * HyperSerial 协议 (AWA协议)
 *============================================================================*/
#define AWA_MAGIC           "AWA"
#define AWA_HEADER_LEN      6

// AWA命令类型
typedef enum {
    AWA_CMD_LED_DATA = 0,       // LED数据
    AWA_CMD_LED_DATA_HDR,       // LED数据 + HDR
    AWA_CMD_CONFIG,             // 配置
    AWA_CMD_CALIBRATION,        // 校准
    AWA_CMD_BRIGHTNESS,         // 亮度
    AWA_CMD_SMOOTHING,          // 平滑
    AWA_CMD_STATISTICS,         // 统计
} AwaCommand_t;

typedef struct {
    uint8_t     magic[3];       // "AWA"
    uint8_t     count_hi;       // LED数量高字节
    uint8_t     count_lo;       // LED数量低字节
    uint8_t     command;        // 命令类型
} AwaHeader_t;

// HyperSerial 扩展功能
typedef struct {
    bool        hdr_enabled;        // HDR模式
    uint8_t     smoothing_level;    // 平滑级别 (0-255)
    uint8_t     brightness;         // 亮度
    uint16_t    white_point_r;      // 白点R
    uint16_t    white_point_g;      // 白点G
    uint16_t    white_point_b;      // 白点B
    uint8_t     gamma_r;            // Gamma R (x10)
    uint8_t     gamma_g;            // Gamma G (x10)
    uint8_t     gamma_b;            // Gamma B (x10)
} HyperSerialConfig_t;

/*============================================================================
 * TPM2 协议
 *============================================================================*/
#define TPM2_HEADER_START   0xC9
#define TPM2_HEADER_DATA    0xDA
#define TPM2_HEADER_CMD     0xC0
#define TPM2_HEADER_RESP    0xAA
#define TPM2_FOOTER         0x36

typedef enum {
    TPM2_CMD_SET_LED = 0x01,
    TPM2_CMD_GET_INFO = 0x02,
    TPM2_CMD_SET_BRIGHTNESS = 0x03,
    TPM2_CMD_SET_GAMMA = 0x04,
} Tpm2Command_t;

typedef struct {
    uint8_t     start;          // 0xC9
    uint8_t     type;           // 数据类型
    uint8_t     size_hi;        // 数据长度高字节
    uint8_t     size_lo;        // 数据长度低字节
} Tpm2Header_t;

/*============================================================================
 * WLED UDP 协议
 *============================================================================*/
typedef enum {
    WLED_NOTIFIER = 0,          // 通知
    WLED_WARLS = 1,             // WARLS (旧版)
    WLED_DRGB = 2,              // DRGB
    WLED_DRGBW = 3,             // DRGBW
    WLED_DNRGB = 4,             // DNRGB (带偏移)
    WLED_DNRGBW = 5,            // DNRGBW (带偏移)
} WledUdpMode_t;

typedef struct {
    uint8_t     mode;           // 模式
    uint8_t     timeout;        // 超时 (秒)
} WledUdpHeader_t;

/*============================================================================
 * Art-Net 协议
 *============================================================================*/
#define ARTNET_PORT         6454
#define ARTNET_MAGIC        "Art-Net"

typedef enum {
    ARTNET_OP_POLL = 0x2000,
    ARTNET_OP_POLL_REPLY = 0x2100,
    ARTNET_OP_DMX = 0x5000,
    ARTNET_OP_SYNC = 0x5200,
} ArtNetOpCode_t;

typedef struct {
    uint8_t     id[8];          // "Art-Net\0"
    uint16_t    opcode;         // 操作码
    uint16_t    version;        // 协议版本
    uint8_t     sequence;       // 序列号
    uint8_t     physical;       // 物理端口
    uint16_t    universe;       // Universe
    uint16_t    length;         // 数据长度
} ArtNetDmxHeader_t;

/*============================================================================
 * E1.31 (sACN) 协议
 *============================================================================*/
#define E131_PORT           5568
#define E131_PREAMBLE_SIZE  16
#define E131_POSTAMBLE_SIZE 0

typedef struct {
    // Root Layer
    uint16_t    preamble_size;
    uint16_t    postamble_size;
    uint8_t     acn_id[12];
    uint16_t    root_flags_length;
    uint32_t    root_vector;
    uint8_t     cid[16];
    
    // Framing Layer
    uint16_t    frame_flags_length;
    uint32_t    frame_vector;
    uint8_t     source_name[64];
    uint8_t     priority;
    uint16_t    sync_address;
    uint8_t     sequence;
    uint8_t     options;
    uint16_t    universe;
    
    // DMP Layer
    uint16_t    dmp_flags_length;
    uint8_t     dmp_vector;
    uint8_t     address_type;
    uint16_t    first_address;
    uint16_t    address_increment;
    uint16_t    property_value_count;
} E131Header_t;

/*============================================================================
 * DDP 协议 (Distributed Display Protocol)
 *============================================================================*/
#define DDP_PORT            4048
#define DDP_HEADER_LEN      10

typedef struct {
    uint8_t     flags;          // 标志
    uint8_t     sequence;       // 序列号
    uint8_t     data_type;      // 数据类型
    uint8_t     destination;    // 目标ID
    uint32_t    offset;         // 数据偏移
    uint16_t    length;         // 数据长度
} DdpHeader_t;

#define DDP_FLAG_VER        0xC0    // 版本掩码
#define DDP_FLAG_VER1       0x40    // 版本1
#define DDP_FLAG_PUSH       0x01    // 推送标志
#define DDP_FLAG_QUERY      0x02    // 查询标志
#define DDP_FLAG_REPLY      0x04    // 回复标志
#define DDP_FLAG_STORAGE    0x08    // 存储标志
#define DDP_FLAG_TIMECODE   0x10    // 时间码标志

/*============================================================================
 * 协议处理器结构
 *============================================================================*/
typedef struct {
    ProtocolType_t  type;
    bool            enabled;
    
    // 状态
    uint32_t        last_packet_time;
    uint32_t        packet_count;
    uint32_t        error_count;
    uint16_t        timeout_ms;
    
    // 缓冲区
    uint8_t*        rx_buffer;
    uint16_t        rx_buffer_size;
    uint16_t        rx_index;
    
    // 解析状态
    uint8_t         parse_state;
    uint16_t        expected_len;
    
    // 回调
    void (*on_data)(LedStrip_t* strip, uint16_t offset, 
                    Color_t* colors, uint16_t count);
    void (*on_config)(void* config);
    
} ProtocolHandler_t;

/*============================================================================
 * 协议API函数
 *============================================================================*/

// 协议处理器创建/销毁
ProtocolHandler_t* protocol_create(ProtocolType_t type, uint16_t buffer_size);
void protocol_destroy(ProtocolHandler_t* handler);

// 数据处理
int protocol_process_byte(ProtocolHandler_t* handler, uint8_t byte);
int protocol_process_data(ProtocolHandler_t* handler, uint8_t* data, uint16_t len);

// 协议特定函数
void adalight_init(ProtocolHandler_t* handler);
void hyperserial_init(ProtocolHandler_t* handler);
void tpm2_init(ProtocolHandler_t* handler);
void wled_udp_init(ProtocolHandler_t* handler);
void artnet_init(ProtocolHandler_t* handler);
void e131_init(ProtocolHandler_t* handler);
void ddp_init(ProtocolHandler_t* handler);

// 协议发送
void adalight_send_ack(void (*write)(uint8_t*, uint16_t));
void hyperserial_send_stats(void (*write)(uint8_t*, uint16_t), uint32_t fps, uint32_t frames);

#endif // LED_PROTOCOL_H
```

## 5. 协议实现 (led_protocol.c)

```c
#include "led_protocol.h"

/*============================================================================
 * 协议处理器创建
 *============================================================================*/

ProtocolHandler_t* protocol_create(ProtocolType_t type, uint16_t buffer_size) {
    ProtocolHandler_t* handler = (ProtocolHandler_t*)calloc(1, sizeof(ProtocolHandler_t));
    if (!handler) return NULL;
    
    handler->type = type;
    handler->enabled = true;
    handler->timeout_ms = 5000;
    
    handler->rx_buffer = (uint8_t*)malloc(buffer_size);
    if (!handler->rx_buffer) {
        free(handler);
        return NULL;
    }
    handler->rx_buffer_size = buffer_size;
    
    // 初始化特定协议
    switch (type) {
        case PROTO_ADALIGHT:
            adalight_init(handler);
            break;
        case PROTO_HYPERSERIAL:
            hyperserial_init(handler);
            break;
        case PROTO_TPM2:
            tpm2_init(handler);
            break;
        case PROTO_WLED_UDP:
            wled_udp_init(handler);
            break;
        case PROTO_ARTNET:
            artnet_init(handler);
            break;
        case PROTO_E131:
            e131_init(handler);
            break;
        case PROTO_DDP:
            ddp_init(handler);
            break;
        default:
            break;
    }
    
    return handler;
}

void protocol_destroy(ProtocolHandler_t* handler) {
    if (!handler) return;
    if (handler->rx_buffer) free(handler->rx_buffer);
    free(handler);
}

/*============================================================================
 * Adalight 协议实现
 *============================================================================*/

typedef enum {
    ADALIGHT_STATE_MAGIC1 = 0,
    ADALIGHT_STATE_MAGIC2,
    ADALIGHT_STATE_MAGIC3,
    ADALIGHT_STATE_COUNT_HI,
    ADALIGHT_STATE_COUNT_LO,
    ADALIGHT_STATE_CHECKSUM,
    ADALIGHT_STATE_DATA
} AdalightState_t;

static uint16_t adalight_led_count = 0;
static uint16_t adalight_data_index = 0;

void adalight_init(ProtocolHandler_t* handler) {
    handler->parse_state = ADALIGHT_STATE_MAGIC1;
    adalight_led_count = 0;
    adalight_data_index = 0;
}

int adalight_process_byte(ProtocolHandler_t* handler, uint8_t byte, 
                          LedStrip_t* strip) {
    switch (handler->parse_state) {
        case ADALIGHT_STATE_MAGIC1:
            if (byte == 'A') {
                handler->parse_state = ADALIGHT_STATE_MAGIC2;
            }
            break;
            
        case ADALIGHT_STATE_MAGIC2:
            if (byte == 'd') {
                handler->parse_state = ADALIGHT_STATE_MAGIC3;
            } else {
                handler->parse_state = ADALIGHT_STATE_MAGIC1;
            }
            break;
            
        case ADALIGHT_STATE_MAGIC3:
            if (byte == 'a') {
                handler->parse_state = ADALIGHT_STATE_COUNT_HI;
            } else {
                handler->parse_state = ADALIGHT_STATE_MAGIC1;
            }
            break;
            
        case ADALIGHT_STATE_COUNT_HI:
            adalight_led_count = byte << 8;
            handler->parse_state = ADALIGHT_STATE_COUNT_LO;
            break;
            
        case ADALIGHT_STATE_COUNT_LO:
            adalight_led_count |= byte;
            adalight_led_count++;  // Adalight count is 0-indexed
            handler->parse_state = ADALIGHT_STATE_CHECKSUM;
            break;
            
        case ADALIGHT_STATE_CHECKSUM:
            {
                uint8_t expected = ((adalight_led_count - 1) >> 8) ^ 
                                   ((adalight_led_count - 1) & 0xFF) ^ 0x55;
                if (byte == expected) {
                    handler->parse_state = ADALIGHT_STATE_DATA;
                    adalight_data_index = 0;
                    handler->expected_len = adalight_led_count * 3;
                } else {
                    handler->parse_state = ADALIGHT_STATE_MAGIC1;
                    handler->error_count++;
                }
            }
            break;
            
        case ADALIGHT_STATE_DATA:
            handler->rx_buffer[adalight_data_index++] = byte;
            
            if (adalight_data_index >= handler->expected_len) {
                // 数据接收完成，更新LED
                if (strip && handler->on_data) {
                    Color_t* colors = (Color_t*)malloc(adalight_led_count * sizeof(Color_t));
                    if (colors) {
                        for (uint16_t i = 0; i < adalight_led_count; i++) {
                            colors[i].r = handler->rx_buffer[i * 3];
                            colors[i].g = handler->rx_buffer[i * 3 + 1];
                            colors[i].b = handler->rx_buffer[i * 3 + 2];
                            colors[i].w = 0;
                        }
                        handler->on_data(strip, 0, colors, adalight_led_count);
                        free(colors);
                    }
                }
                
                handler->packet_count++;
                handler->parse_state = ADALIGHT_STATE_MAGIC1;
                return 1;  // 帧完成
            }
            break;
    }
    
    return 0;
}

void adalight_send_ack(void (*write)(uint8_t*, uint16_t)) {
    if (!write) return;
    uint8_t ack[] = "Ada\n";
    write(ack, 4);
}

/*============================================================================
 * HyperSerial (AWA) 协议实现
 *============================================================================*/

typedef enum {
    AWA_STATE_MAGIC1 = 0,
    AWA_STATE_MAGIC2,
    AWA_STATE_MAGIC3,
    AWA_STATE_COUNT_HI,
    AWA_STATE_COUNT_LO,
    AWA_STATE_COMMAND,
    AWA_STATE_DATA,
    AWA_STATE_CRC
} AwaState_t;

static uint16_t awa_led_count = 0;
static uint16_t awa_data_index = 0;
static uint8_t awa_command = 0;
static uint8_t awa_crc = 0;

static HyperSerialConfig_t hyperserial_config = {
    .hdr_enabled = false,
    .smoothing_level = 0,
    .brightness = 255,
    .white_point_r = 255,
    .white_point_g = 255,
    .white_point_b = 255,
    .gamma_r = 22,  // 2.2
    .gamma_g = 22,
    .gamma_b = 22
};

void hyperserial_init(ProtocolHandler_t* handler) {
    handler->parse_state = AWA_STATE_MAGIC1;
    awa_led_count = 0;
    awa_data_index = 0;
    awa_crc = 0;
}

// Fletcher校验和
static uint8_t fletcher8(uint8_t* data, uint16_t len) {
    uint8_t sum1 = 0, sum2 = 0;
    for (uint16_t i = 0; i < len; i++) {
        sum1 = (sum1 + data[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }
    return (sum2 << 4) | (sum1 & 0x0F);
}

int hyperserial_process_byte(ProtocolHandler_t* handler, uint8_t byte,
                             LedStrip_t* strip) {
    switch (handler->parse_state) {
        case AWA_STATE_MAGIC1:
            awa_crc = byte;
            if (byte == 'A') {
                handler->parse_state = AWA_STATE_MAGIC2;
            }
            break;
            
        case AWA_STATE_MAGIC2:
            awa_crc ^= byte;
            if (byte == 'w') {
                handler->parse_state = AWA_STATE_MAGIC3;
            } else {
                handler->parse_state = AWA_STATE_MAGIC1;
            }
            break;
            
        case AWA_STATE_MAGIC3:
            awa_crc ^= byte;
            if (byte == 'a' || byte == 'A') {  // 'A' = HDR模式
                hyperserial_config.hdr_enabled = (byte == 'A');
                handler->parse_state = AWA_STATE_COUNT_HI;
            } else {
                handler->parse_state = AWA_STATE_MAGIC1;
            }
            break;
            
        case AWA_STATE_COUNT_HI:
            awa_crc ^= byte;
            awa_led_count = byte << 8;
            handler->parse_state = AWA_STATE_COUNT_LO;
            break;
            
        case AWA_STATE_COUNT_LO:
            awa_crc ^= byte;
            awa_led_count |= byte;
            awa_led_count++;
            handler->parse_state = AWA_STATE_COMMAND;
            break;
            
        case AWA_STATE_COMMAND:
            awa_crc ^= byte;
            awa_command = byte;
            
            if (awa_command == AWA_CMD_LED_DATA || 
                awa_command == AWA_CMD_LED_DATA_HDR) {
                handler->parse_state = AWA_STATE_DATA;
                awa_data_index = 0;
                handler->expected_len = awa_led_count * 3;
            } else {
                // 其他命令处理
                handler->parse_state = AWA_STATE_MAGIC1;
            }
            break;
            
        case AWA_STATE_DATA:
            awa_crc ^= byte;
            handler->rx_buffer[awa_data_index++] = byte;
            
            if (awa_data_index >= handler->expected_len) {
                handler->parse_state = AWA_STATE_CRC;
            }
            break;
            
        case AWA_STATE_CRC:
            if (byte == awa_crc) {
                // CRC正确，更新LED
                if (strip && handler->on_data) {
                    Color_t* colors = (Color_t*)malloc(awa_led_count * sizeof(Color_t));
                    if (colors) {
                        for (uint16_t i = 0; i < awa_led_count; i++) {
                            uint8_t r = handler->rx_buffer[i * 3];
                            uint8_t g = handler->rx_buffer[i * 3 + 1];
                            uint8_t b = handler->rx_buffer[i * 3 + 2];
                            
                            // 应用白点校正和Gamma
                            if (hyperserial_config.hdr_enabled) {
                                r = (r * hyperserial_config.white_point_r) >> 8;
                                g = (g * hyperserial_config.white_point_g) >> 8;
                                b = (b * hyperserial_config.white_point_b) >> 8;
                            }
                            
                            colors[i].r = r;
                            colors[i].g = g;
                            colors[i].b = b;
                            colors[i].w = 0;
                        }
                        handler->on_data(strip, 0, colors, awa_led_count);
                        free(colors);
                    }
                }
                handler->packet_count++;
            } else {
                handler->error_count++;
            }
            handler->parse_state = AWA_STATE_MAGIC1;
            return 1;
    }
    
    return 0;
}

void hyperserial_send_stats(void (*write)(uint8_t*, uint16_t), 
                            uint32_t fps, uint32_t frames) {
    if (!write) return;
    
    uint8_t stats[16];
    stats[0] = 'A';
    stats[1] = 'w';
    stats[2] = 'a';
    stats[3] = (fps >> 8) & 0xFF;
    stats[4] = fps & 0xFF;
    stats[5] = (frames >> 24) & 0xFF;
    stats[6] = (frames >> 16) & 0xFF;
    stats[7] = (frames >> 8) & 0xFF;
    stats[8] = frames & 0xFF;
    
    write(stats, 9);
}

/*============================================================================
 * TPM2 协议实现
 *============================================================================*/

typedef enum {
    TPM2_STATE_START = 0,
    TPM2_STATE_TYPE,
    TPM2_STATE_SIZE_HI,
    TPM2_STATE_SIZE_LO,
    TPM2_STATE_DATA,
    TPM2_STATE_FOOTER
} Tpm2State_t;

static uint16_t tpm2_data_size = 0;
static uint16_t tpm2_data_index = 0;
static uint8_t tpm2_type = 0;

void tpm2_init(ProtocolHandler_t* handler) {
    handler->parse_state = TPM2_STATE_START;
    tpm2_data_size = 0;
    tpm2_data_index = 0;
}

int tpm2_process_byte(ProtocolHandler_t* handler, uint8_t byte,
                      LedStrip_t* strip) {
    switch (handler->parse_state) {
        case TPM2_STATE_START:
            if (byte == TPM2_HEADER_START) {
                handler->parse_state = TPM2_STATE_TYPE;
            }
            break;
            
        case TPM2_STATE_TYPE:
            tpm2_type = byte;
            if (byte == TPM2_HEADER_DATA || byte == TPM2_HEADER_CMD) {
                handler->parse_state = TPM2_STATE_SIZE_HI;
            } else {
                handler->parse_state = TPM2_STATE_START;
            }
            break;
            
        case TPM2_STATE_SIZE_HI:
            tpm2_data_size = byte << 8;
            handler->parse_state = TPM2_STATE_SIZE_LO;
            break;
            
        case TPM2_STATE_SIZE_LO:
            tpm2_data_size |= byte;
            if (tpm2_data_size > 0 && tpm2_data_size <= handler->rx_buffer_size) {
                handler->parse_state = TPM2_STATE_DATA;
                tpm2_data_index = 0;
            } else {
                handler->parse_state = TPM2_STATE_START;
            }
            break;
            
        case TPM2_STATE_DATA:
            handler->rx_buffer[tpm2_data_index++] = byte;
            if (tpm2_data_index >= tpm2_data_size) {
                handler->parse_state = TPM2_STATE_FOOTER;
            }
            break;
            
        case TPM2_STATE_FOOTER:
            if (byte == TPM2_FOOTER) {
                if (tpm2_type == TPM2_HEADER_DATA && strip && handler->on_data) {
                    uint16_t led_count = tpm2_data_size / 3;
                    Color_t* colors = (Color_t*)malloc(led_count * sizeof(Color_t));
                    if (colors) {
                        for (uint16_t i = 0; i < led_count; i++) {
                            colors[i].r = handler->rx_buffer[i * 3];
                            colors[i].g = handler->rx_buffer[i * 3 + 1];
                            colors[i].b = handler->rx_buffer[i * 3 + 2];
                            colors[i].w = 0;
                        }
                        handler->on_data(strip, 0, colors, led_count);
                        free(colors);
                    }
                }
                handler->packet_count++;
            } else {
                handler->error_count++;
            }
            handler->parse_state = TPM2_STATE_START;
            return 1;
    }
    
    return 0;
}

/*============================================================================
 * WLED UDP 协议实现
 *============================================================================*/

void wled_udp_init(ProtocolHandler_t* handler) {
    handler->parse_state = 0;
}

int wled_udp_process_packet(ProtocolHandler_t* handler, uint8_t* data, 
                            uint16_t len, LedStrip_t* strip) {
    if (len < 2) return -1;
    
    uint8_t mode = data[0];
    uint8_t timeout = data[1];
    
    uint16_t offset = 0;
    uint16_t led_count = 0;
    uint8_t bytes_per_led = 3;
    
    switch (mode) {
        case WLED_DRGB:
            bytes_per_led = 3;
            led_count = (len - 2) / 3;
            break;
            
        case WLED_DRGBW:
            bytes_per_led = 4;
            led_count = (len - 2) / 4;
            break;
            
        case WLED_DNRGB:
            if (len < 4) return -1;
            offset = (data[2] << 8) | data[3];
            bytes_per_led = 3;
            led_count = (len - 4) / 3;
            data += 2;
            break;
            
        case WLED_DNRGBW:
            if (len < 4) return -1;
            offset = (data[2] << 8) | data[3];
            bytes_per_led = 4;
            led_count = (len - 4) / 4;
            data += 2;
            break;
            
        default:
            return -1;
    }
    
    if (strip && handler->on_data && led_count > 0) {
        Color_t* colors = (Color_t*)malloc(led_count * sizeof(Color_t));
        if (colors) {
            uint8_t* pixel_data = data + 2;
            for (uint16_t i = 0; i < led_count; i++) {
                colors[i].r = pixel_data[i * bytes_per_led];
                colors[i].g = pixel_data[i * bytes_per_led + 1];
                colors[i].b = pixel_data[i * bytes_per_led + 2];
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 5. 协议实现 (led_protocol.c) - 续

```c
                colors[i].w = (bytes_per_led == 4) ? 
                              pixel_data[i * bytes_per_led + 3] : 0;
            }
            handler->on_data(strip, offset, colors, led_count);
            free(colors);
        }
    }
    
    handler->packet_count++;
    return 1;
}

/*============================================================================
 * Art-Net 协议实现
 *============================================================================*/

void artnet_init(ProtocolHandler_t* handler) {
    handler->parse_state = 0;
}

int artnet_process_packet(ProtocolHandler_t* handler, uint8_t* data,
                          uint16_t len, LedStrip_t* strip) {
    if (len < sizeof(ArtNetDmxHeader_t)) return -1;
    
    // 验证魔数
    if (memcmp(data, "Art-Net", 7) != 0) {
        return -1;
    }
    
    ArtNetDmxHeader_t* header = (ArtNetDmxHeader_t*)data;
    
    // 检查操作码 (小端序)
    uint16_t opcode = header->opcode;
    
    if (opcode == ARTNET_OP_DMX) {
        uint16_t universe = header->universe;
        uint16_t dmx_len = (header->length >> 8) | (header->length << 8);  // 大端转小端
        
        if (dmx_len > 512) dmx_len = 512;
        
        uint8_t* dmx_data = data + sizeof(ArtNetDmxHeader_t);
        uint16_t led_count = dmx_len / 3;
        
        if (strip && handler->on_data && led_count > 0) {
            // 计算偏移 (每个universe 170个LED)
            uint16_t offset = universe * 170;
            
            Color_t* colors = (Color_t*)malloc(led_count * sizeof(Color_t));
            if (colors) {
                for (uint16_t i = 0; i < led_count; i++) {
                    colors[i].r = dmx_data[i * 3];
                    colors[i].g = dmx_data[i * 3 + 1];
                    colors[i].b = dmx_data[i * 3 + 2];
                    colors[i].w = 0;
                }
                handler->on_data(strip, offset, colors, led_count);
                free(colors);
            }
        }
        
        handler->packet_count++;
        return 1;
    }
    
    return 0;
}

// Art-Net Poll Reply
void artnet_send_poll_reply(void (*send)(uint8_t*, uint16_t, void*), 
                            void* user_data,
                            const char* name, 
                            uint8_t* ip, 
                            uint16_t num_ports) {
    uint8_t reply[239];
    memset(reply, 0, sizeof(reply));
    
    // Art-Net header
    memcpy(reply, "Art-Net", 8);
    reply[8] = ARTNET_OP_POLL_REPLY & 0xFF;
    reply[9] = (ARTNET_OP_POLL_REPLY >> 8) & 0xFF;
    
    // IP地址
    if (ip) {
        memcpy(&reply[10], ip, 4);
    }
    
    // 端口
    reply[14] = ARTNET_PORT & 0xFF;
    reply[15] = (ARTNET_PORT >> 8) & 0xFF;
    
    // 版本
    reply[16] = 0;
    reply[17] = 14;
    
    // 短名称
    if (name) {
        strncpy((char*)&reply[26], name, 17);
    }
    
    // 长名称
    if (name) {
        strncpy((char*)&reply[44], name, 63);
    }
    
    // 端口数量
    reply[173] = num_ports > 4 ? 4 : num_ports;
    
    if (send) {
        send(reply, sizeof(reply), user_data);
    }
}

/*============================================================================
 * E1.31 (sACN) 协议实现
 *============================================================================*/

void e131_init(ProtocolHandler_t* handler) {
    handler->parse_state = 0;
}

// E1.31 ACN ID
static const uint8_t E131_ACN_ID[] = {
    0x41, 0x53, 0x43, 0x2D, 0x45, 0x31, 0x2E, 0x31, 0x37, 0x00, 0x00, 0x00
};

int e131_process_packet(ProtocolHandler_t* handler, uint8_t* data,
                        uint16_t len, LedStrip_t* strip) {
    if (len < 126) return -1;  // 最小E1.31包大小
    
    // 验证ACN ID
    if (memcmp(&data[4], E131_ACN_ID, 12) != 0) {
        return -1;
    }
    
    // 解析头部
    uint16_t universe = (data[113] << 8) | data[114];
    uint8_t sequence = data[111];
    uint8_t priority = data[108];
    
    // DMX数据起始位置
    uint8_t* dmx_data = &data[126];
    uint16_t dmx_len = len - 126;
    
    if (dmx_len > 512) dmx_len = 512;
    
    uint16_t led_count = dmx_len / 3;
    
    if (strip && handler->on_data && led_count > 0) {
        uint16_t offset = (universe - 1) * 170;  // E1.31 universe从1开始
        
        Color_t* colors = (Color_t*)malloc(led_count * sizeof(Color_t));
        if (colors) {
            for (uint16_t i = 0; i < led_count; i++) {
                colors[i].r = dmx_data[i * 3];
                colors[i].g = dmx_data[i * 3 + 1];
                colors[i].b = dmx_data[i * 3 + 2];
                colors[i].w = 0;
            }
            handler->on_data(strip, offset, colors, led_count);
            free(colors);
        }
    }
    
    handler->packet_count++;
    return 1;
}

/*============================================================================
 * DDP 协议实现
 *============================================================================*/

void ddp_init(ProtocolHandler_t* handler) {
    handler->parse_state = 0;
}

int ddp_process_packet(ProtocolHandler_t* handler, uint8_t* data,
                       uint16_t len, LedStrip_t* strip) {
    if (len < DDP_HEADER_LEN) return -1;
    
    DdpHeader_t* header = (DdpHeader_t*)data;
    
    // 验证版本
    if ((header->flags & DDP_FLAG_VER) != DDP_FLAG_VER1) {
        return -1;
    }
    
    // 获取偏移和长度
    uint32_t offset = (header->offset >> 24) | 
                      ((header->offset >> 8) & 0xFF00) |
                      ((header->offset << 8) & 0xFF0000) |
                      (header->offset << 24);
    
    uint16_t data_len = (header->length >> 8) | (header->length << 8);
    
    if (len < DDP_HEADER_LEN + data_len) return -1;
    
    uint8_t* pixel_data = data + DDP_HEADER_LEN;
    uint8_t data_type = header->data_type;
    
    // 确定每像素字节数
    uint8_t bytes_per_pixel = 3;
    if (data_type == 1) bytes_per_pixel = 3;      // RGB
    else if (data_type == 2) bytes_per_pixel = 4; // RGBW
    
    uint16_t led_count = data_len / bytes_per_pixel;
    
    if (strip && handler->on_data && led_count > 0) {
        Color_t* colors = (Color_t*)malloc(led_count * sizeof(Color_t));
        if (colors) {
            for (uint16_t i = 0; i < led_count; i++) {
                colors[i].r = pixel_data[i * bytes_per_pixel];
                colors[i].g = pixel_data[i * bytes_per_pixel + 1];
                colors[i].b = pixel_data[i * bytes_per_pixel + 2];
                colors[i].w = (bytes_per_pixel == 4) ? 
                              pixel_data[i * bytes_per_pixel + 3] : 0;
            }
            handler->on_data(strip, offset, colors, led_count);
            free(colors);
        }
    }
    
    // 检查是否需要立即显示
    if (header->flags & DDP_FLAG_PUSH) {
        if (strip) {
            strip->dirty = true;
        }
    }
    
    handler->packet_count++;
    return 1;
}

/*============================================================================
 * 统一协议处理接口
 *============================================================================*/

int protocol_process_byte(ProtocolHandler_t* handler, uint8_t byte) {
    if (!handler || !handler->enabled) return -1;
    
    switch (handler->type) {
        case PROTO_ADALIGHT:
            return adalight_process_byte(handler, byte, NULL);
        case PROTO_HYPERSERIAL:
            return hyperserial_process_byte(handler, byte, NULL);
        case PROTO_TPM2:
            return tpm2_process_byte(handler, byte, NULL);
        default:
            return -1;
    }
}

int protocol_process_data(ProtocolHandler_t* handler, uint8_t* data, 
                          uint16_t len) {
    if (!handler || !handler->enabled || !data) return -1;
    
    switch (handler->type) {
        case PROTO_WLED_UDP:
            return wled_udp_process_packet(handler, data, len, NULL);:
            return artnet_process_packet(handler, data, len, NULL);
        case PROTO_E131:
            return e131_process_packet(handler, data, len, NULL);
        case PROTO_DDP:
            return ddp_process_packet(handler, data, len, NULL);
        default:
            // 串行协议逐字节处理
            for (uint16_t i = 0; i < len; i++) {
                protocol_process_byte(handler, data[i]);
            }
            return 0;
    }
}
```

## 6. WLED效果实现 (led_effects_wled.c)

```c
#include "h"
#include <math.h>

/*============================================================================
 * 数学工具
 *============================================================================*/

// 正弦表 (256项)
static const uint8_t sin8_table[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,
    79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,
    37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,
    10,7,6,5,5,4,3,2,2,1,1,1,0,0,0,
    0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,
    10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,
    37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,
    79,82,85,88,90,93,97,100,103,118,121,124
};

static inline uint8_t sin8(uint8_t x) { return sin8_table[x]; }
static inline uint8_t cos8(uint8_t x) { return sin8_table[(uint8_t)(x + 64)]; }

static inline uint8_t triwave8(uint8_t x) {
    return (x < 128) ? (x * 2) : (255 - (x - 128) * 2);
}

static inline uint8_t cubicwave8(uint8_t x) {
    return (tri * triwave8(x)) >> 8;
}

static inline uint8_t quadwave8(uint8_t x) {
    return sin8(x);
}

// 随机数
static uint32_t rand_seed = 12345;

static inline uint8_t random8(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed >> 16) & 0xFF;
}

static inline uint8_t random8_max(uint8_t max) {
    return ( max) >> 8;
}

static inline uint8_t random8_range(uint8_t min, uint8_t max) {
    return min + random8_max(max - min);
}

static inline uint16_t random16(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return rand_seed & 0xFFFF;
}

// 缩放
static inline uint8_t scale8(uint8_t val, uint8_t scale) {
    return ((uint16_t)val * (uint16_t)scale) >> inline uint8_t qadd8(uint8_t a, uint8_t b) {
    uint16_t sum = a + b;
    return (sum > 255) ? 255 : sum;
}

static inline uint8_t qsub8(uint8_t a, uint8_t b) {
    return (a > b) ? (a - b) : 0;
}

static inline uint8_t lerp8(uint8_t a, uint8_t b, uint8_t frac) {
    return a + scale8(b - a, frac);
}

// 噪声函8_t inoise8(uint16_t x, uint16_t y) {
    uint16_t n = x + y * 57;
    n = (n << 13) ^ n;
    return (uint8_t)((n * (n * n * 15731 + 789221) + 1376312589) >> 24);
}

/*============================================================================
 * 线段辅助函数
 *============================================================================*/

static inline uint16_t seg_len(Segment_t* seg) {
    return seg->stop - seg->start;
}d seg_set_pixel(LedStrip_t* strip, Segment_t* seg, 
                          uint16_t i, Color_t color) {
    uint16_t len = seg_len(seg);
    if (i >= len) return;
    
    uint16_t idx;
    if (seg->options.reverse) {
        idx = seg->stop - 1 - i;
    } else {
        idx = seg->start + i;
    }
    
    // 镜像模式
    if (seg->options.mirror && len > 1) {
        uint = len / 2;
        if (i >= half) {
            idx = seg->options.reverse ? 
                  (seg->start + (i - half)) : (seg->stop - 1 - (i - half));
        }
    }
    
    // 应用亮度
    color.r = scale8(color.r, seg->params.brightness);
    color.g = scale8(color.g, seg->params.brightness);
    color.b = scale8(color.b, seg->params.brightness);
    color.w = scaleparams.brightness);
    
    if (idx < strip->config.count) {
        strip->leds[idx] = color;
    }
}

static Color_t seg_get_pixel(LedStrip_t* strip, Segment_t* seg, uint16_t i) {
    uint16_t len = seg_len(seg);
    if (i >= len) return BLACK;
    
    uint16_t idx = seg->options.reverse ? (seg->stop - 1 - i) : (seg->start + i);
    
    if (idx < strip->config.count) {
        return strip->ledsreturn BLACK;
}

static void seg_fill(LedStrip_t* strip, Segment_t* seg, Color_t color) {
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        Color_t c = color;
        c.r = scale8(c.r, seg->params.brightness);
        c.g = scale8(c.g, seg->params.brightness);
        c.b = scale8(c.b, seg->params.brightness);
        c.w = scale8(c.w, seg->params.brightness);
        strip->leds[i] = c;
    }
}

static void seg_clear(LedStrip_t* strip, Segment_t* seg_fill(strip, seg, BLACK);
}

static void seg_fade_out(LedStrip_t* strip, Segment_t* seg, uint8_t rate) {
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        Color_t c = strip->leds[i];
        c.r = scale8(c.r, 255 - rate);
        c.g = scale8(c.g, 255 - rate);
        c.b = scale8(c.b, 255 - rate);
        c.w = scale8(c.w, 255 - rate);
        strip->leds[i] = c;
    }
}(LedStrip_t* strip, Segment_t* seg, uint8_t blur_amount) {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    
    Color_t carryover = BLACK;
    
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        Color_t cur = strip->leds[i];
        Color_t part;
        
        part.r = scale8(cur.r, seep);
        part.g =ep);
        part.b = scale8(cur.b, seep);
        
        cur.r = scale8(cur.r, keep);
        cur.g = scale8(cur.g, keep);
        cur.b = scale8(cur.b, keep);
        
        cur.r = qadd8(cur.r, carryover.r);
        cur.g = qadd8(cur.g, carryover.g);
        cur.b = qadd8(cur.b, carryover.b);
        
        if (i > seg->start) {
            strip->leds[i - 1].r = qadd8(strip->leds[.r);
            strip->leds[i - 1].g = qadd8(strip->leds[i - 1].g, part.g);
            strip->leds[i - 1].b = qadd8(strip->leds[i - 1].b, part.b);
        }
        
        strip->leds[i] = cur;
        carryover = part;
    }
}

/*============================================================================
 * 颜色工具
 *============================================================================*/

static Color_t color_wheel(uint8_t pos) {
    pos = 255 - pos;
    if (pos < 85) {
        return COLOR(255 - pos * 3, 0, pos * 3);
    }) {
        pos -= 85;
        return COLOR(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return COLOR(pos * 3, 255 - pos * 3, 0);
}

static Color_t color_hsv(uint8_t h, uint8_t s, uint8_t v) {
    if (s == 0) {
        return COLOR(v, v, v);
    }
    
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t =s * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
        case 0:  return COLOR(v, t, p);
        case 1:  return COLOR(q, v, p);
        case 2:  return COLOR(p, v, t);
        case 3:  return COLOR(p, q, v);
        case 4:  return COLOR(t, p, v);
        default: return COLOR(v, p, q);
    }
}

static Color_t color_blend(Color_t c1, Color_t c2, uint8_t blend) {
    uint8_t inv = 255 - blend;
    return (Color_t){
        .r = (c1.r * inv + c2.r * blend) >> 8,
        .g = (c1.g * inv + c2.g * blend) >> 8,
        .b = (c1.b * inv + c2.b * blend) >> 8,
        .w = (c1.w * inv + c2.w * blend) >> 8
    };
}

static Color_t color_scale(Color_t c, uint8_t scale) {
    return (Color_t){
        .r = (c.r * scale) >> 8,
        .g = (c.g * scale) >> 8,
        .b = (c.b * scale) >> 8,
        .w = (c.w * scale) >> 8
    };
}

/*============================================================================
 * 效果实现
 *============================================================================*/

// FX_SOLID 纯色
static void fx_solid(LedStrip_t* strip, Segment_t* seg) {
    seg_fill(strip, seg, seg->params.colors[0]);
}

// FX_BLINK - 闪烁
static void fx_blink(LedStrip_t* strip, Segment_t* seg) {
    Color_t c = (seg->step & 1) ? seg->params.colors[1] : seg->params.colors[0];
    seg_fill(strip, seg, c);
    seg->step++;
}

// FX_BREATH - 呼吸
static void fx_breath(LedStrip_t* strip, Segment_t* seg) {
    uint8_t breath = cubicwave8(seg->step);
    Color_t c = color_scale(seg->params.colors[0], breath);
    seg_fill(strip, seg, c);
    seg->step += 2;
}

// FX_COLOR_WIPE - 颜色擦除
static void fx_color_wipe(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t pos = seg->step % (len * 2);
    
    ifseg_set_pixel(strip, seg, pos, seg->params.colors[0]);
    } else {
        seg_set_pixel(strip, seg, pos - len, seg->params.colors[1]);
    }
    seg->step++;
}

// FX_RAINBOW - 彩虹
static void fx_rainbow(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = (i * 256 / len) + seg->step;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    seg->step++;
}

// FX_RAINBOW_CYCLE - 彩虹循环
static void fx_rainbow_cycle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = ((i * 256 / len) + seg->step) & 0xFF;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    seg->step += 2;
}

// FX_SCAN - 扫描
static void fx_scan(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 1;
    
    seg_fade_out(strip, seg, 64);
    
    uint16_t cycle = (len - size) * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= len - size) {
        pos = cycle - pos;
    }
    
    for (uint8_t i = 0; i < size; i++) {
        _set_pixel(strip, seg, pos + i, seg->params.colors[0]);
    }
    
    seg->step++;
}

// FX_DUAL_SCAN - 双向扫描
static void fx_dual_scan(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 1;
    
    seg_fade_out(strip, seg, 64);
    
    uint16_t cycle = (len - size) * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= len - size) {
        pos = cycle - pos;
    }
    
    for (uint8_t i = 0; i < size; i++) {
        seg_set_pixel(strip, seg, pos + i, seg->params.colors[0]);
        seg_set_pixel(strip, seg, len - 1 - pos - i, seg->params.colors[1]);
    }
    
    seg->step++;
}

// FX_THEATER_CHASE - 剧院追逐
static void fx_theater_chase(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t gap = seg->params.custom1 > 0 ? seg->params.custom1 : 3;
    
    seg_fill(strip, seg, seg->params.colors[1]);
    
    for (uint16_t i = seg->step % gap gap) {
        seg_set_pixel(strip, seg, i, seg->params.colors[0]);
    }
    
    seg->step++;
}

// FX_RUNNING_LIGHTS - 跑马灯
static void fx_running_lights(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t level = sin8((i * 256 / len) + seg->step * 4);
        Color_t c = color_scale(seg->params.colors[0], level);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_TWINKLE - 闪烁
static void fx_twinkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 32);
    
    uint8_t num = seg->params.intensity > 0 ? seg->params.intensity / 32 : 3;
    for (uint8_t i = 0; i < num; i++) {
        if (random8() < 50) {
            uint16_t pos = random_set_pixel(strip, seg, pos, seg->params.colors[0]);
        }
    }
    
    seg->step++;
}

// FX_SPARKLE - 火花
static void fx_sparkle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fill(strip, seg, seg->params.colors[1]);
    
    uint16_t pos = random16() % len;
    seg_set_pixel(strip, seg, pos, seg->params.colors[0]);
    
    seg->step++;
}

// FX_STROBE - 频闪
static void fx_strobe(LedStrip_t* strip, Segment_t* ((seg->step % 6) == 0) {
        seg_fill(strip, seg, seg->params.colors[0]);
    } else {
        seg_fill(strip, seg, seg->params.colors[1]);
    }
    seg->step++;
}

// FX_LARSON_SCANNER - Larson扫描器
static void fx_larson_scanner(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 4;
    
    seg_fade_out(strip, seg, 64);
    
    uint16_t range - size;
    uint16_t cycle = range * 2;
    uint16_t pos = seg->step % cycle;
    
    if (pos >= range) {
        pos = cycle - pos;
    }
    
    for (uint8_t i = 0; i < size; i++) {
        uint8_t brightness;
        uint8_t half = size / 2;
        if (i < half) {
            brightness = 255 * (i + 1) / (half + 1);
        } else {
            brightness = 255 * (size - i) / (half + 1);
        }
        Color_t c = color_scale(seg->params.colors[0], brightness);
        seg_set_pixel(strip, seg, pos + i, c);
    }
    
    seg->step++;
}

// FX_COMET - 彗星
static void fx_comet(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 8;
    
    seg_fade_out(strip, seg, 48);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (pos + len - i) % len;
        uint8_t brightness = 255 - (i * 255 / size);
        Color_t c = color_scale(seg->params.colors[0], brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

// FX_FIRE_2012 - Fire 2012
static void fx_fire_2012(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    static uint8_t heat[MAX_LEDS_];
    
    uint8_t cooling = seg->params.custom1 > 0 ? seg->params.custom1 : 55;
    uint8_t sparking = seg->params.custom2 > 0 ? seg->params.custom2 : 120;
    
    // 冷却
    for (uint16_t i = 0; i < len; i++) {
        uint8_t cool = random8_max((cooling * 10 / len) + 2);
        heat[i] = qsub8(heat[i], cool);
    }
    
    // 热量上升
    for (uint16_t i = len - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
    }
    
    // 随机点火
    if (random8() < sparking) {
        uint8_t y = random8_max(7);
        heat[y] = qadd8(heat[y], random8_range(160, 255));
    }
    
    // 转换为颜色
    for (uint16_t i = 0; i < len; i++) {
        uint8_t t192 = scale8(heat[i], 191);
        uint8_t heatramp = t192 & 0x3F;
        heatramp <<= 2;
        
        Color_t c;
        if (t192 > 0x80) {
            c = COLOR(255, 255eatramp);
        } else if (t192 > 0x40) {
            c = COLOR(255, heatramp, 0);
        } else {
            c = COLOR(heatramp, 0, 0);
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_METEOR - 流星
static void fx_meteor(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 5;
    uint8_t decay = seg->params.custom2 > 0 ? seg->params.custom2 : 64;
    
    // 随机衰减
    for (uint16_t i = 0; i < len; i++) {
        if (random8() > 64) {
            Color_t c = seg_get_pixel(strip, seg, i);
            c.r = qsub8(c.r, decay);
            c.g = qsub8(c.g, decay);
            c.b = qsub8(c.b, decay);
            seg_set_pixel(strip, seg, i, c);
        }
    }
    
    // 绘制流星
    uint16_t pos = seg->step % (len + size);
    for (uint8 = 0; j < size; j++) {
        if (pos >= j && (pos - j) < len) {
            seg_set_pixel(strip, seg, pos - j, seg->params.colors[0]);
        }
    }
    
    seg->step++;
}

// FX_RIPPLE - 涟漪
static void fx_ripple(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 64);
    
    static uint16_t center = 0;
    static uint8_t radius = 0;
    static uint8_t hue = 0;
    
    if (radius == 0) {
        center = random16() % len;
        hue = random8();
    }
    
    uint8_t brightness = 255 - (radius * 8);
    if (brightness > 0) {
        Color_t c = color_scale(color_wheel(hue), brightness);
        
        if (center >= radius) {
            seg_set_pixel(strip, seg, center - radius, c);
        }
        if radius < len) {
            seg_set_pixel(strip, seg, center + radius, c);
        }
    }
    
    radius++;
    if (radius > len / 2) {
        radius = 0;
    }
    
    seg->step++;
}

// FX_HEARTBEAT - 心跳
static void fx_heartbeat(LedStrip_t* strip, Segment_t* seg) {
    static const uint8_t heartbeat_wave[] = {
        0, 32, 64, 128, 255, 255, 128, 64, 32, 0,
        0, 0, 0, 0, 0,
        0, 32, 64, 128, 200, 200, 128, 64, 32, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    uint8_t idx = seg->step % sizeof(heartbeat_wave);
    uint8_t brightness = heartbeat_wave[idx];
    
    Color_t c = color_scale(seg->params.colors[0], brightness);
    seg_fill(strip, seg, c);
    
    seg->step++;
}

// FX_PACIFICA - 太平洋
static void fx_pacifica(Le Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint32_t t = seg->step;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t wave1 = sin8(i * 3 + t);
        uint8_t wave2 = sin8(i * 5 + t * 2);
        uint8_t wave3 = sin8(i * 7 - t);
        uint8_t wave4 = sin8(i * 2 + t / 2);
        
        uintwave1 + wave2 + wave3 + wave4) / 4;
        
        uint8_t r = scale8(brightness, 20);
        uint8_t g = scale8(brightness, 100);
        uint8_t b = scale8(brightness, 200);
        
        if (brightness > 220) {
            r = qadd8(r, 50);
            g = qadd8(g, 50);
            b = qadd8(b, 50);
        }
        
        seg_set_pixel(strip, seg, i, COLOR(r, g, b));
    }
    
    seg->step++;
}

// FX_PLASMA - 等离子
static void fx_plasma(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t val1 = sin8(i * 10 + seg->step);
        uint8_t val2 = sin8(i * 5 + seg->step * 2);
        uint8_t val3 = sin8(i * 2 + seg->step / 2);
        
        uint8_t hue = (val1 + val2 + val3) / 3;
        seg_set_pixel(strip, seg, i, color_wheel(hue));
    }
    
    seg->step++;
}

// FX_SINELON - 正弦波
static void fx_sinelon(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint16_t pos = (sin8(seg->step) * len) >> 8;
    seg_set_pixel(strip, seg, pos, seg->params.colors[0]);
    
    seg->step += 2;
}

// FX_JUGGLE - 杂耍
static void fx_juggle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    seg_fade_out(strip, seg, 20);
    
    uint8_t dot_hue = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint16_t pos = (sin8(seg->step + i * 32) * len) >> 8;
        Color_t c = color_wheel(dot_hue);
        
        Color_t existing = seg_get_pixel(strip, seg, pos);
        c.r = qadd8(existing.r, c.r);
        c.g = qadd8(existing.g, c.g);
        c.b = qadd8(existing.b, c.b);
        
        seg, pos, c);
        dot_hue += 32;
    }
    
    seg->step++;
}

// FX_CANDLE - 蜡烛
static void fx_candle(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t flicker = random8_range(180, 255);
        uint8_t r = scale8(255, flicker);
        uint8_t g = scale8(100, flicker);
        uint8_t b = scale8(20, flicker);
        seg_set_pixel(strip, seg, i, COLOR(r, g, b));
    }
    
    seg->step++;
}

// FX_LOADING - 加载
static void fx_loading(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint8_t size = seg->params.custom1 > 0 ? seg->params.custom1 : 5;
    
    seg_fill(strip, seg, seg->params.colors[1]);
    
    uint16_t pos = seg->step % len;
    
    for (uint8_t i = 0; i < size; i++) {
        uint16_t idx = (pos + i) % len;
        uint8 (i * 255 / size);
        Color_t c = color_scale(seg->params.colors[0], brightness);
        seg_set_pixel(strip, seg, idx, c);
    }
    
    seg->step++;
}

// FX_POLICE - 警灯
static void fx_police(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    uint16_t half = len / 2;
    uint8_t phase = (seg->step / 4) % 4;
    
    seg_clear(strip, seg);
    
    switch (phase) {
        case 0:
            for (uint16_t i = 0; i < half; i++) {
                seg_set_pixel(strip, seg, i, RED);
            }
            break;
        case 1:
            break;
        case 2:
            for (uint16_t i = half; i < len; i++) {
                seg_set_pixel(strip, seg, i, BLUE);
            }
            break;
        case 3:
            break;
    }
    
    seg->step++;
}

// FX_GRADIENT - 渐变
static void fx_gradient(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for0; i < len; i++) {
        uint8_t blend = (i * 255) / (len - 1);
        Color_t c = color_blend(seg->params.colors[0], seg->params.colors[1], blend);
        seg_set_pixel(strip, seg, i, c);
    }
}

// FX_AURORA - 极光
static void fx_aurora(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t n1 = inoise8(i * 20, seg->step);
        uint8_t n2 = inoise8(i * 30
        
        uint8_t hue = 96 + (n1 >> 2);  // 绿色到蓝色范围
        uint8_t brightness = n2;
        
        Color_t c = color_hsv(hue, 255, brightness);
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_TWINKLEFOX - TwinkleFOX
static void fx_twinklefox(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t phase = (i * 37 + seg->step) & 0xFF;
        uint8_t brightness = sin8(phase);
        
        uint8_t seed = (i * 17) & 0xFF;
        if (seed < 128) {
            brightness = scale8(brightness, seed * 2);
        }
        
        Color_t c = color_scale(seg->params.colors[0], brightness);
        
        if (brightness > 240 8() < 30) {
            c = WHITE;
        }
        
        seg_set_pixel(strip, seg, i, c);
    }
    
    seg->step++;
}

// FX_BOUNCINGBALLS - 弹跳球
#define MAX_BALLS 5

typedef struct {
    float height;
    float velocity;
    float damping;
    Color_t color;
} Ball_t;

static Ball_t balls[MAX_BALLS];
static bool balls_init = false;

static void fx_bouncingballs(LedStrip_t* strip, Segment_t* seg) {
    uint16_t len = seg_len(seg);
    const float gravity = 0.4f;
    uint8_t num_balls = seg->params.custom1 > 0 ? seg->params.custom1 : 3;
    if (num_balls > MAX_BALLS) num_balls = MAX_BALLS;
    
    if (!balls_init) {
        for (uint8_t i = 0; i < num_balls; i++) {
            balls[i].height = 0;
            balls[i].velocity = -10.0f - i * 3;
            balls[i].damping = 0.85f + i * 0.02f;
            balls[i].color = color_wheel(i * 85);
        }
        balls_init = true;
    }
    
    seg_fade_out(strip, seg, 48);
    
    for (uint8_t b = 0; b < num_balls; b++) {
        balls[b].velocity += gravity;
        balls[b].height += balls[b].velocity;
        
        if (balls[b].height >= len - 1) {
            balls[b].height = len - 1;
            balls[b].velocity = -balls[b].velocity * balls[b].damping;
            
            if (balls[b].velocity > -1.0f && balls[b].velocity < 1.0f) {
                balls[b].velocity = -12.0f - random8_max(5);
            }
        }
        
        uint16_t pos = (uint16_t)(len - 1 - balls[b].height);
        if (pos < len) {
            seg_set_pixel(strip, seg, pos, balls[b].color);
        }
    }
    
    seg->step++;
}

// FX_PERCENT - 百分比/进度条
static void fx_percent(LedStrip_t* strip, Segment_t* seg) {
    uint16_t
    uint8_t percent = seg->aux0;  // 0-100
    
    uint16_t filled = (len * percent) / 100;
    
    for (uint16_t i = 0; i < len; i++) {
        Color_t c;
        if (i < filled) {
            if (i < len / 3) {
                c = GREEN;
            } else if (i < len * 2 / 3) {
                c = YELLOW;
            } else {
                c = RED;
            }
        } else {
            c = seg->params.colors[1];
        }
        seg_set_pixel(strip, seg, i, c);
    }
}

/*============================================================================
 * 效果函数表
 *============================================================================*/

typedef void (*FxFunc_t)(LedStrip_t*, Segment_t*);

static const FxFunc_t fx_table[] = {
    [FX_SOLID]              = fx_solid,
    [FX_BLINK]              = fx_blink,
    [FX_BREATH]             = fx_breath,_WIPE]         = fx_color_wipe,
    [FX_RAINBOW]            = fx_rainbow,
    [FX_RAINBOW_CYCLE]      = fx_rainbow_cycle,
    [FX_SCAN]               = fx_scan,
    [FX_DUAL_SCAN]          = fx_dual_scan,
    [FX_THEATER_CHASE]      = fx_theater_chase,
    [FX_RUNNING_LIGHTS]     = fx_running_lights,
    [FX_TWINKLE]            = fx_twinkle,
    [FX_SPARKLE]            = fx_sparkle,
    [FX_STROBE]             = fx_strobe,
    [FX_LARSON_SCANNER]     = fx_larson_scanner,
    [FX_COMET]              = fx_comet,
    [FX_FIRE_2012]          = fx_fire_2012,
    [FX_METEOR]             = fx_meteor,
    [FX_RIPPLE]             = fx_ripple,
    [FX_HEARTBEAT]          = fx_heartbeat,
    [FX_PACIFICA]           = fx_pacifica,
    [FX_PLASMA]             = fx_plasma,
    [FX_SINELON]            = fx_sinelon,
    [FX_JUGGLE]             = fx_juggle,
    [FX_CANDLE]             = fx_candle,
    = fx_loading,
    [FX_POLICE]             = fx_police,
    [FX_GRADIENT]           = fx_gradient,
    [FX_AURORA]             = fx_aurora,
    [FX_TWINKLEFOX]         = fx_twinklefox,
    [FX_BOUNCINGBALLS]      = fx_bouncingballs,
    [FX_PERCENT]            = fx_percent,
    // ... 更多效果
};

/*============================================================================
 * 效果信息表
 *============================================================================*/

static const FxInfo_t fx_info_table[] = {
    [FX_SOLID]          = {"Solid",           000, 128, 0, false, false},
    [FX_BLINK]          = {"Blink",           500,  128, 0, false, false},
    [FX_BREATH]         = {"Breath",          20,   128, 0, false, false},
    [FX_COLOR_WIPE]     = {"Color Wipe",      50,   128, 0, false, false},
    [FX_RAINBOW]        = {"Rainbow",         30,   128, 0, false, false},
    [FX_RAINBOW_CYCLE]  = {"Rainbow Cycle",   20, false},
    [FX_SCAN]           = {"Scan",            30,   128, 0, false, false},
    [FX_DUAL_SCAN]      = {"Dual Scan",       30,   128, 0, false, false},
    [FX_THEATER_CHASE]  = {"Theater Chase",   100,  128, 0, false, false},
    [FX_RUNNING_LIGHTS] = {"Running Lights",  30,   128, 0, false, false},
    [FX_TWINKLE]        = {"Twinkle",         30,   128, 0, false, false},
    [FX_SPARKLE]        = {"Sparkle",         20,   128, 0, false, false},
    [FX_STROBE]         = {"Strobe",          30,   128, 0, false, false},
    [FX_LARSON_SCANNER] = {"Larson Scanner",  30,   128, 0, false, false},
    [FX_COMET]          = {"Comet",           30,   128, 0, false, false},
    [FX_FIRE_2012]      = {"Fire 2012",       20,   128, 0, false, false},
    [FX_METEOR]         = {"Meteor",          30,   128, 0, false, false},
    [FX_RIPPLE]         = {"Ripple",          30,   128, 0, false, false},
    [FX_HEARTBEAT]      = {"Heartbeat",       30,   128, 0, false, false},
    [FX_PACIFICA]       = {"Pacifica",        20,   128, 0, false, false},
    [FX_PLASMA]         = {",   128, 0, false, false},
    [FX_SINELON]        = {"Sinelon",         20,   128, 0, false, false},
    [FX_JUGGLE]         = {"Juggle",          20,   128, 0, false, false},
    [FX_CANDLE]         = {"Candle",          50,   128, 0, false, false},
    [FX_LOADING]        = {"Loading",         30,   128, 0, false, false},
    [FX_POLICE]         = {"Police",          50,   128, 0, false, false},
    [FX_GRADIENT]       = {"Gradient",        1000, 128, 0, false, false},
    [FX_AURORA]         = {"Aurora",          30, false},
    [FX_TWINKLEFOX]     = {"TwinkleFOX",      20,   128, 0, false, false},
    [FX_BOUNCINGBALLS]  = {"Bouncing Balls",  20,   128, 0, false, false},
    [FX_PERCENT]        = {"Percent",         100,  128, 0, false, false},
};

/*============================================================================
 * 效果API实现
 *============================================================================*/

void fx_execute(LedStrip_t* strip, Segment_t* seg) {
    if (!strip || !seg ||.on) return;
    if (seg->options.freeze) return;
    
    uint8_t mode = seg->mode;
    
    if (mode < sizeof(fx_table) / sizeof(fx_table[0]) && fx_table[mode]) {
        fx_table[mode](strip, seg);
        strip->dirty = true;
    }
}

const FxInfo_t* fx_get_info(FxMode_t mode) {
    if (mode < sizeof(fx_info_table) / sizeof(fx_info_table[0])) {
        return &fx_info_table[mode];
    }
    return NULL;
}

const char* fx_get_name(FxMode_t mode) {
    const FxInfo_t* info = fx_get_info(mode);
    return info ? info->name : "Unknown";
}

uint8_t fx_get_count(void) {
    return sizeof(fx_table) / sizeof(fx_table[0]);
}

FxMode_t fx_next(FxMode_t current) {
    return (current + 1) % fx_get_count();
}

FxMode_t fx_prev(FxMode_t current) {
    return (current == 0) ? (fx_get_count() - 1) : (current - 1);
}

FxMode_t fx_random(void) {
    return random8() % fx_get_count();
}
```

## 7. 完整使用示例 (example_complete.c)

```c
/*============================================================================
 * 综合LED灯效库 - 完整使用示例
 * 支持: WS2812FX / WLED / HyperSerial / Adal-Net / E1.31 / DDP
 *============================================================================*/

#include <stdio.h>
#include <string.h>
#include "led_core.h"
#include "led_effects.h"
#include "led_protocol.h"

/*============================================================================
 * 平台适配层
 *============================================================================*/

static volatile uint32_t sys_tick_ms = 0;

uint32_t get_millis(void) {
    return sys_tick_ms;
}

void delay_ms(uint32_t ms) {
    uint32_t start = sys_tick_ms;
    while ((sys_tick_ms - start) < ms);
}

// 硬件输出函数 (根据实际平台实现)
void ws2812_output(uint8_t* data, uint16_t len, void* user_data) {
    // 实际实现: SPI/DRMT 发送
    (void)user_data;
}

// 串口接收回调
void uart_rx_callback(uint8_t* data, uint16_t len);

// UDP接收回调
void udp_rx_callback(uint8_t* data, uint16_t len, uint32_t src_ip, uint16_t src_port);

/*============================================================================
 * 全局变量
 *============================================================================*/

static StripManager g_manager = NULL;
static LedStrip_t* g_strip = NULL;

// 协议处理器
static ProtocolHandler_t* g_adalight = NULL;
static ProtocolHandler_t* g_hyperserial = NULL;
static ProtocolHandler_t* g_tpm2 = NULL;
static ProtocolHandler_t* g_artnet = NULL;
static ProtocolHandler_t* g_e131 = NULL;
static ProtocolHandler_t* g_ddp = NULL;
static ProtocolHandler_t* g_wled_udp = NULL;

// 当前模式
typedef enum {
    MODE_EFFECTS,       // 本地效果模式
    MODE_STREAMING,     // 流媒体模式 (接收外部数据)
} SystemMode_t;

static SystemMode_t g_mode = MODE_EFFECTS;
static uint32_t g_last_stream_time = 0;
static uint32_t g_stream_timeout = 5000;  // 5秒超时

/*============================================================================
 * 协议数据回调
 *============================================================================*/

voi_led_data(LedStrip_t* strip, uint16_t offset, 
                 Color_t* colors, uint16_t count) {
    if (!strip || !colors) return;
    
    // 切换到流媒体模式
    g_mode = MODE_STREAMING;
    g_last_stream_time = get_millis();
    
    // 更新LED数据
    for (uint16_t i = 0; i < count && (offset.count; i++) {
        strip->leds[offset + i] = colors[i];
    }
    
    strip->dirty = true;
}

/*============================================================================
 * 初始化
 *============================================================================*/

void system_init(void) {
    printf("========================================\n");
    printf("  综合LED灯效库 v%s\n", LED_LIB_VERSION);
    printf("  支持协议: Adalight, HyperSerial, TPM2\n");
    printf("            Art-Net, E1.31 WLED\n");
    printf("========================================\n\n");
    
    // 创建灯带管理器
    g_manager = strip_manager_create(get_millis);
    if (!g_manager) {
        printf("Error: Failed to create strip manager\n");
        return;
    }
    
    // 创建灯带配置
    StripConfig_t config = {
        .type = LED_TYPE_WS2812B,
        .color_order = COLOR_ORDER_GRB,
        .count =,
        .pin = 18,
        .refresh_rate = 60,
        .milliamps_per_led = 55
    };
    
    // 创建灯带
    g_strip = led_strip_create(&config);
    if (!g_strip) {
        printf("Error: Failed to create LED strip\n");
        return;
    }
    
    // 设置输出函数
    led_strip_set_output(g_strip, ws2812_output, NULL);
    
    //
```





```
继续完成代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 7. 完整使用示例 (example_complete.c) - 续

```c
    // 添加到管理器
    led_strip_add_to_manager(g_manager, g_strip);
    
    // 设置亮度
    led_strip_set_brightness(g_strip, 128);
    
    // 创建默认线段
    int8_t seg_id = segment_add(g_strip, 0, g_strip->config.count);
    if (seg_id >= 0) {
        segment_set_mode(g_strip, seg_id, FX_RAINBOW_CYCLE);
        segment_set_colors(g_strip, seg_id, BLUE, BLACK, BLACK);
        segment_set_speed(g_strip, seg_id, 30);
    }
    
    printf("LED Strip: %d LEDs, Type: WS2812B, Order: GRB\n", config.count);
    printf("Default effect: Rainbow Cycle\n\n");
    
    // 初始化协议处理器
    protocol_init();
}

/*============================================================================
 * 协议初始化
 *============================================================================*/

void protocol_init(void) {
    printf("Initializing protocols...\n");
    
    // Adalight (串口)
    g_adalight = protocol_create(PROTO_ADALIGHT, 4096);
    if (g_adalight) {
        g_adalight->on_data = on_led_data;
        printf("  - Adalight: Ready (Serial)\n");
    }
    
    // HyperSerial (串口)
    g_hyperserial = protocol_create(PROTO_HYPERSERIAL, 4096);
    if (g_hyperserial) {
        g_hyperserial->on_data = on_led_data;
        printf("  - HyperSerial: Ready (Serial)\n");
    }
    
    // TPM2 (串口)
    g_tpm2 = protocol_create(PROTO_TPM2, 4096);
    if (g_tpm2) {
        g_tpm2->on_data = on_led_data;
        printf("  - TPM2: Ready (Serial)\n");
    }
    
    // Art-Net (UDP 6454)
    g_artnet = protocol_create(PROTO_ARTNET, 1024);
    if (g_artnet) {
        g_artnet->on_data = on_led_data;
        printf("  - Art-Net: Ready (UDP:6454)\n");
    }
    
    // E1.31/sACN (UDP 5568)
    g_e131 = protocol_create(PROTO_E131, 1024);
    if (g_e131) {
        g_e131->on_data = on_led_data;
        printf("  - E1.31/sACN: Ready (UDP:5568)\n");
    }
    
    // DDP (UDP 4048)
    g_ddp = protocol_create(PROTO_DDP, 4096);
    if (g_ddp) {
        g_ddp->on_data = on_led_data;
        printf("  - DDP: Ready (UDP:4048)\n");
    }
    
    // WLED UDP (UDP 21324)
    g_wled_udp = protocol_create(PROTO_WLED_UDP, 4096);
    if (g_wled_udp) {
        g_wled_udp->on_data = on_led_data;
        printf("  - WLED UDP: Ready (UDP:21324)\n");
    }
    
    printf("\n");
}

/*============================================================================
 * 串口数据处理
 *============================================================================*/

void uart_rx_callback(uint8_t* data, uint16_t len) {
    // 尝试所有串口协议
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        
        // Adalight
        if (g_adalight && g_adalight->enabled) {
            adalight_process_byte(g_adalight, byte, g_strip);
        }
        
        // HyperSerial
        if (g_hyperserial && g_hyperserial->enabled) {
            hyperserial_process_byte(g_hyperserial, byte, g_strip);
        }
        
        // TPM2
        if (g_tpm2 && g_tpm2->enabled) {
            tpm2_process_byte(g_tpm2, byte, g_strip);
        }
    }
}

/*============================================================================
 * UDP数据处理
 *============================================================================*/

void udp_rx_callback(uint8_t* data, uint16_t len, uint32_t src_ip, uint16_t src_port) {
    (void)src_ip;
    
    switch (src_port) {
        case 6454:  // Art-Net
            if (g_artnet && g_artnet->enabled) {
                artnet_process_packet(g_artnet, data, len, g_strip);
            }
            break;
            
        case 5568:  // E1.31
            if (g_e131 && g_e131->enabled) {
                e131_process_packet(g_e131, data, len, g_strip);
            }
            break;
            
        case 4048:  // DDP
            if (g_ddp && g_ddp->enabled) {
                ddp_process_packet(g_ddp, data, len, g_strip);
            }
            break;
            
        case 21324: // WLED UDP
            if (g_wled_udp && g_wled_udp->enabled) {
                wled_udp_process_packet(g_wled_udp, data, len, g_strip);
            }
            break;
    }
}

/*============================================================================
 * 主循环更新
 *============================================================================*/

void system_update(void) {
    uint32_t now = get_millis();
    
    // 检查流媒体超时
    if (g_mode == MODE_STREAMING) {
        if ((now - g_last_stream_time) > g_stream_timeout) {
            printf("Stream timeout, switching to effects mode\n");
            g_mode = MODE_EFFECTS;
        }
    }
    
    // 根据模式更新
    if (g_mode == MODE_EFFECTS) {
        // 本地效果模式 - 更新所有线段效果
        strip_manager_update(g_manager);
    }
    
    // 显示输出
    if (g_strip && g_strip->dirty) {
        led_strip_show(g_strip);
    }
}

/*============================================================================
 * 效果演示
 *============================================================================*/

void demo_effects(void) {
    printf("\n=== 效果演示模式 ===\n");
    printf("总共 %d 个效果\n\n", fx_get_count());
    
    for (uint8_t mode = 0; mode < fx_get_count(); mode++) {
        const FxInfo_t* info = fx_get_info(mode);
        if (!info) continue;
        
        printf("效果 %d: %s\n", mode, info->name);
        
        // 设置效果
        segment_set_mode(g_strip, 0, mode);
        
        // 运行3秒
        uint32_t start = get_millis();
        while ((get_millis() - start) < 3000) {
            system_update();
            delay_ms(1);
            sys_tick_ms++;
        }
    }
}

/*============================================================================
 * 分段效果演示
 *============================================================================*/

void demo_segments(void) {
    printf("\n=== 分段效果演示 ===\n");
    
    // 删除默认线段
    segment_remove(g_strip, 0);
    
    // 创建4个线段
    uint16_t len = g_strip->config.count;
    uint16_t quarter = len / 4;
    
    int8_t seg1 = segment_add(g_strip, 0, quarter);
    int8_t seg2 = segment_add(g_strip, quarter, quarter * 2);
    int8_t seg3 = segment_add(g_strip, quarter * 2, quarter * 3);
    int8_t seg4 = segment_add(g_strip, quarter * 3, len);
    
    // 设置不同效果
    if (seg1 >= 0) {
        segment_set_mode(g_strip, seg1, FX_RAINBOW_CYCLE);
        segment_set_speed(g_strip, seg1, 30);
        printf("Segment 1: Rainbow Cycle\n");
    }
    
    if (seg2 >= 0) {
        segment_set_mode(g_strip, seg2, FX_METEOR);
        segment_set_colors(g_strip, seg2, CYAN, BLACK, BLACK);
        segment_set_speed(g_strip, seg2, 40);
        printf("Segment 2: Meteor\n");
    }
    
    if (seg3 >= 0) {
        segment_set_mode(g_strip, seg3, FX_FIRE_2012);
        segment_set_speed(g_strip, seg3, 25);
        printf("Segment 3: Fire 2012\n");
    }
    
    if (seg4 >= 0) {
        segment_set_mode(g_strip, seg4, FX_HEARTBEAT);
        segment_set_colors(g_strip, seg4, RED, BLACK, BLACK);
        segment_set_speed(g_strip, seg4, 30);
        printf("Segment 4: Heartbeat\n");
    }
    
    // 运行10秒
    printf("\nRunning for 10 seconds...\n");
    uint32_t start = get_millis();
    while ((get_millis() - start) < 10000) {
        system_update();
        delay_ms(1);
        sys_tick_ms++;
    }
    
    // 恢复单线段
    while (g_strip->num_segments > 0) {
        segment_remove(g_strip, 0);
    }
    segment_add(g_strip, 0, g_strip->config.count);
}

/*============================================================================
 * 等待效果演示
 *============================================================================*/

void demo_waiting_effects(void) {
    printf("\n=== 等待效果演示 ===\n");
    
    FxMode_t waiting_effects[] = {
        FX_LOADING,
        FX_BREATH,
        FX_SCAN,
        FX_LARSON_SCANNER,
        FX_COMET,
        FX_SINELON,
        FX_JUGGLE,
        FX_PACIFICA
    };
    
    const char* names[] = {
        "Loading", "Breath", "Scan", "Larson Scanner",
        "Comet", "Sinelon", "Juggle", "Pacifica"
    };
    
    for (int i = 0; i < 8; i++) {
        printf("等待效果: %s\n", names[i]);
        
        segment_set_mode(g_strip, 0, waiting_effects[i]);
        segment_set_colors(g_strip, 0, BLUE, COLOR(10, 10, 30), BLACK);
        segment_set_speed(g_strip, 0, 30);
        
        uint32_t start = get_millis();
        while ((get_millis() - start) < 3000) {
            system_update();
            delay_ms(1);
            sys_tick_ms++;
        }
    }
}

/*============================================================================
 * 进度条演示
 *============================================================================*/

void demo_progress_bar(void) {
    printf("\n=== 进度条演示 ===\n");
    
    segment_set_mode(g_strip, 0, FX_PERCENT);
    segment_set_colors(g_strip, 0, GREEN, COLOR(30, 30, 30), BLACK);
    
    // 模拟进度 0-100%
    for (int progress = 0; progress <= 100; progress++) {
        g_strip->segments[0].aux0 = progress;
        
        printf("\rProgress: %d%%", progress);
        fflush(stdout);
        
        system_update();
        delay_ms(50);
        sys_tick_ms += 50;
    }
    printf("\nComplete!\n");
}

/*============================================================================
 * 协议统计
 *============================================================================*/

void print_protocol_stats(void) {
    printf("\n=== 协议统计 ===\n");
    
    if (g_adalight) {
        printf("Adalight:    Packets: %lu, Errors: %lu\n", 
               g_adalight->packet_count, g_adalight->error_count);
    }
    if (g_hyperserial) {
        printf("HyperSerial: Packets: %lu, Errors: %lu\n",
               g_hyperserial->packet_count, g_hyperserial->error_count);
    }
    if (g_tpm2) {
        printf("TPM2:        Packets: %lu, Errors: %lu\n",
               g_tpm2->packet_count, g_tpm2->error_count);
    }
    if (g_artnet) {
        printf("Art-Net:     Packets: %lu, Errors: %lu\n",
               g_artnet->packet_count, g_artnet->error_count);
    }
    if (g_e131) {
        printf("E1.31:       Packets: %lu, Errors: %lu\n",
               g_e131->packet_count, g_e131->error_count);
    }
    if (g_ddp) {
        printf("DDP:         Packets: %lu, Errors: %lu\n",
               g_ddp->packet_count, g_ddp->error_count);
    }
    if (g_wled_udp) {
        printf("WLED UDP:    Packets: %lu, Errors: %lu\n",
               g_wled_udp->packet_count, g_wled_udp->error_count);
    }
    
    if (g_strip) {
        printf("\nStrip FPS: %lu, Frames: %lu\n", g_strip->fps, g_strip->frame_count);
    }
}

/*============================================================================
 * 命令处理
 *============================================================================*/

typedef struct {
    const char* cmd;
    const char* desc;
    void (*handler)(const char* args);
} Command_t;

void cmd_help(const char* args);
void cmd_mode(const char* args);
void cmd_brightness(const char* args);
void cmd_color(const char* args);
void cmd_speed(const char* args);
void cmd_effect(const char* args);
void cmd_list(const char* args);
void cmd_stats(const char* args);
void cmd_demo(const char* args);

static const Command_t commands[] = {
    {"help",       "Show help",                    cmd_help},
    {"mode",       "Set effect mode (0-N)",        cmd_mode},
    {"brightness", "Set brightness (0-255)",       cmd_brightness},
    {"color",      "Set color (R G B)",            cmd_color},
    {"speed",      "Set speed (0-65535)",          cmd_speed},
    {"effect",     "Set effect by name",           cmd_effect},
    {"list",       "List all effects",             cmd_list},
    {"stats",      "Show protocol statistics",     cmd_stats},
    {"demo",       "Run demo (all/segments/wait)", cmd_demo},
    {NULL, NULL, NULL}
};

void cmd_help(const char* args) {
    (void)args;
    printf("\nAvailable commands:\n");
    for (int i = 0; commands[i].cmd; i++) {
        printf("  %-12s - %s\n", commands[i].cmd, commands[i].desc);
    }
    printf("\n");
}

void cmd_mode(const char* args) {
    int mode = atoi(args);
    if (mode >= 0 && mode < fx_get_count()) {
        segment_set_mode(g_strip, 0, mode);
        printf("Effect mode set to %d: %s\n", mode, fx_get_name(mode));
    } else {
        printf("Invalid mode. Range: 0-%d\n", fx_get_count() - 1);
    }
}

void cmd_brightness(const char* args) {
    int brightness = atoi(args);
    if (brightness >= 0 && brightness <= 255) {
        led_strip_set_brightness(g_strip, brightness);
        printf("Brightness set to %d\n", brightness);
    } else {
        printf("Invalid brightness. Range: 0-255\n");
    }
}

void cmd_color(const char* args) {
    int r, g, b;
    if (sscanf(args, "%d %d %d", &r, &g, &b) == 3) {
        segment_set_colors(g_strip, 0, COLOR(r, g, b), BLACK, BLACK);
        printf("Color set to RGB(%d, %d, %d)\n", r, g, b);
    } else {
        printf("Usage: color R G B\n");
    }
}

void cmd_speed(const char* args) {
    int speed = atoi(args);
    if (speed >= 0 && speed <= 65535) {
        segment_set_speed(g_strip, 0, speed);
        printf("Speed set to %d\n", speed);
    } else {
        printf("Invalid speed. Range: 0-65535\n");
    }
}

void cmd_effect(const char* args) {
    int16_t mode = fx_find_by_name(args);
    if (mode >= 0) {
        segment_set_mode(g_strip, 0, mode);
        printf("Effect set to: %s\n", fx_get_name(mode));
    } else {
        printf("Effect not found: %s\n", args);
    }
}

void cmd_list(const char* args) {
    (void)args;
    printf("\nAvailable effects:\n");
    for (int i = 0; i < fx_get_count(); i++) {
        const FxInfo_t* info = fx_get_info(i);
        if (info) {
            printf("  %3d: %s\n", i, info->name);
        }
    }
    printf("\n");
}

void cmd_stats(const char* args) {
    (void)args;
    print_protocol_stats();
}

void cmd_demo(const char* args) {
    if (strcmp(args, "all") == 0) {
        demo_effects();
    } else if (strcmp(args, "segments") == 0) {
        demo_segments();
    } else if (strcmp(args, "wait") == 0) {
        demo_waiting_effects();
    } else if (strcmp(args, "progress") == 0) {
        demo_progress_bar();
    } else {
        printf("Usage: demo [all|segments|wait|progress]\n");
    }
}

void process_command(const char* input) {
    char cmd[32];
    char args[128] = "";
    
    // 解析命令和参数
    if (sscanf(input, "%31s %127[^\n]", cmd, args) < 1) {
        return;
    }
    
    // 查找并执行命令
    for (int i = 0; commands[i].cmd; i++) {
        if (strcmp(cmd, commands[i].cmd) == 0) {
            commands[i].handler(args);
            return;
        }
    }
    
    printf("Unknown command: %s (type 'help' for list)\n", cmd);
}

/*============================================================================
 * 清理
 *============================================================================*/

void system_cleanup(void) {
    printf("\nCleaning up...\n");
    
    // 销毁协议处理器
    if (g_adalight) protocol_destroy(g_adalight);
    if (g_hyperserial) protocol_destroy(g_hyperserial);
    if (g_tpm2) protocol_destroy(g_tpm2);
    if (g_artnet) protocol_destroy(g_artnet);
    if (g_e131) protocol_destroy(g_e131);
    if (g_ddp) protocol_destroy(g_ddp);
    if (g_wled_udp) protocol_destroy(g_wled_udp);
    
    // 销毁灯带管理器
    if (g_manager) {
        strip_manager_destroy(g_manager);
    }
    
    printf("Cleanup complete.\n");
}

/*============================================================================
 * 主程序
 *============================================================================*/

int main(void) {
    // 初始化系统
    system_init();
    
    // 显示帮助
    cmd_help(NULL);
    
    printf("System ready. Waiting for commands or streaming data...\n\n");
    
    // 主循环
    char input[256];
    while (1) {
        // 更新系统
        system_update();
        
        // 检查命令输入 (非阻塞)
        // 实际实现中需要根据平台调整
        #if 0
        if (fgets(input, sizeof(input), stdin)) {
            // 移除换行符
            input[strcspn(input, "\n")] = 0;
            if (strlen(input) > 0) {
                process_command(input);
            }
        }
        #endif
        
        // 模拟时间流逝
        delay_ms(1);
        sys_tick_ms++;
        
        // 定期打印状态
        static uint32_t last_status = 0;
        if ((sys_tick_ms - last_status) >= 10000) {
            last_status = sys_tick_ms;
            printf("Mode: %s, Effect: %s, FPS: %lu\n",
                   g_mode == MODE_EFFECTS ? "Effects" : "Streaming",
                   fx_get_name(g_strip->segments[0].mode),
                   g_strip->fps);
        }
    }
    
    // 清理
    system_cleanup();
    
    return 0;
}

/*============================================================================
 * 定时器中断 (1ms)
 *============================================================================*/

void SysTick_Handler(void) {
    sys_tick_ms++;
}
```

## 8. ESP32 平台实现 (platform_esp32.c)

```c
/*============================================================================
 * ESP32 平台实现
 * 使用 RMT 外设驱动 WS2812
 * 支持 WiFi UDP 协议
 *============================================================================*/

#ifdef ESP32

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "led_core.h"
#include "led_protocol.h"

/*============================================================================
 * 配置
 *============================================================================*/

#define LED_RMT_CHANNEL     RMT_CHANNEL_0
#define LED_GPIO            18
#define LED_COUNT           60

#define UART_NUM            UART_NUM_0
#define UART_BUF_SIZE       4096

#define WIFI_SSID           "YourSSID"
#define WIFI_PASS           "YourPassword"

#define UDP_PORT_ARTNET     6454
#define UDP_PORT_E131       5568
#define UDP_PORT_DDP        4048
#define UDP_PORT_WLED       21324

/*============================================================================
 * RMT WS2812 驱动
 *============================================================================*/

#define WS2812_T0H_NS       350
#define WS2812_T0L_NS       900
#define WS2812_T1H_NS       900
#define WS2812_T1L_NS       350
#define WS2812_RESET_US     280

static rmt_item32_t ws2812_bit0;
static rmt_item32_t ws2812_bit1;

void rmt_ws2812_init(void) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_GPIO, LED_RMT_CHANNEL);
    config.clk_div = 2;
    
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(LED_RMT_CHANNEL, 0, 0));
    
    uint32_t counter_clk_hz = 0;
    rmt_get_counter_clock(LED_RMT_CHANNEL, &counter_clk_hz);
    float ratio = (float)counter_clk_hz / 1e9;
    
    ws2812_bit0.level0 = 1;
    ws2812_bit0.duration0 = ratio * WS2812_T0H_NS;
    ws2812_bit0.level1 = 0;
    ws2812_bit0.duration1 = ratio * WS2812_T0L_NS;
    
    ws2812_bit1.level0 = 1;
    ws2812_bit1.duration0 = ratio * WS2812_T1H_NS;
    ws2812_bit1.level1 = 0;
    ws2812_bit1.duration1 = ratio * WS2812_T1L_NS;
}

void rmt_ws2812_write(uint8_t* data, uint16_t len, void* user_data) {
    (void)user_data;
    
    rmt_item32_t* items = heap_caps_malloc(len * 8 * sizeof(rmt_item32_t), 
                                           MALLOC_CAP_DMA);
    if (!items) return;
    
    uint32_t idx = 0;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int8_t bit = 7; bit >= 0; bit--) {
            items[idx++] = (byte & (1 << bit)) ? ws2812_bit1 : ws2812_bit0;
        }
    }
    
    rmt_write_items(LED_RMT_CHANNEL, items, idx, true);
    rmt_wait_tx_done(LED_RMT_CHANNEL, portMAX_DELAY);
    
    free(items);
}

/*============================================================================
 * 时间函数
 *============================================================================*/

uint32_t esp32_millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/*============================================================================
 * UART 任务
 *============================================================================*/

static ProtocolHandler_t* uart_adalight = NULL;
static ProtocolHandler_t* uart_hyperserial = NULL;
static LedStrip_t* g_strip = NULL;

void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 2000000,  // HyperSerial 高波特率
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, UART_BUF_SIZE, 0, 0, NULL, 0));
}

void uart_task(void* arg) {
    uint8_t* data = malloc(UART_BUF_SIZE);
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, UART_BUF_SIZE, 
                                  10 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (uart_adalight) {
                    adalight_process_byte(uart_adalight, data[i], g_strip);
                }
                if (uart_hyperserial) {
                    hyperserial_process_byte(uart_hyperserial, data[i], g_strip);
                }
            }
        }
    }
    
    free(data);
    vTaskDelete(NULL);
}

/*============================================================================
 * WiFi 初始化
 *============================================================================*/

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        printf("Got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    }
}

void wifi_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                               &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/*============================================================================
 * UDP 任务
 *============================================================================*/

static ProtocolHandler_t* udp_artnet = NULL;
static ProtocolHandler_t* udp_e131 = NULL;
static ProtocolHandler_t* udp_ddp = NULL;
static ProtocolHandler_t* udp_wled = NULL;

void udp_server_task(void* arg) {
    int port = (int)arg;
    uint8_t* rx_buffer = malloc(1500);
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("Failed to create socket for port %d\n", port);
        free(rx_buffer);
        vTaskDelete(NULL);
        return;
    }
    
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to bind socket for port %d\n", port);
        close(sock);
        free(rx_buffer);
        vTaskDelete(NULL);
        return;
    }
    
    printf("UDP server listening on port %d\n", port);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int len = recvfrom(sock, rx_buffer, 1500, 0,
                          (struct sockaddr*)&client_addr, &addr_len);
        
        if (len > 0) {
            switch (port) {
                case UDP_PORT_ARTNET:
                    if (udp_artnet) {
                        artnet_process_packet(udp_artnet, rx_buffer, len, g_strip);
                    }
                    break;
                case UDP_PORT_E131:
                    if (udp_e131) {
                        e131_process_packet(udp_e131, rx_buffer, len, g_strip);
                    }
                    break;
                case UDP_PORT_DDP:
                    if (udp_ddp) {
                        ddp_process_packet(udp_ddp, rx_buffer, len, g_strip);
                    }
                    break;
                case UDP_PORT_WLED:
                    if (udp_wled) {
                        wled_udp_process_packet(udp_wled, rx_buffer, len, g_strip);
                    }
                    break;
            }
        }
    }
    
    close(sock);
    free(rx_buffer);
    vTaskDelete(NULL);
}

/*============================================================================
 * 效果更新任务
 *============================================================================*/

static StripManager_t* g_manager = NULL;

void effect_task(void* arg) {
    while (1) {
        strip_manager_update(g_manager);
        
        if (g_strip && g_strip->dirty) {
            led_strip_show(g_strip);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    vTaskDelete(NULL);
}

/*============================================================================
 * 主程序
 *============================================================================*/

void app_main(void) {
    printf("ESP32 LED Controller Starting...\n");
    
    // 初始化 RMT
    rmt_ws2812_init();
    
    // 创建灯带管理器
    g_manager = strip_manager_create(esp32_millis);
    
    // 创建灯带
    StripConfig_t config = {
        .type = LED_TYPE_WS2812B,
        .color_order = COLOR_ORDER_GRB,
        .count = LED_COUNT,
        .pin = LED_GPIO,
    };
    
    g_strip = led_strip_create(&config);
    led_strip_set_output(g_strip, rmt_ws2812_write, NULL);
    led_strip_add_to_manager(g_manager, g_strip);
    
    // 设置默认效果
    segment_add(g_strip, 0, LED_COUNT);
    segment_set_mode(g_strip, 0, FX_RAINBOW_CYCLE);
    
    // 初始化协议处理器
    uart_adalight = protocol_create(PROTO_ADALIGHT, 4096);
    uart_hyperserial = protocol_create(PROTO_HYPERSERIAL, 4096);
    udp_artnet = protocol_create(PROTO_ARTNET, 1024);
    udp_e131 = protocol_create(PROTO_E131, 1024);
    udp_ddp = protocol_create(PROTO_DDP, 4096);
    udp_wled = protocol_create(PROTO_WLED_UDP, 4096);
    
    // 设置回调
    extern void on_led_data(LedStrip_t*, uint16_t, Color_t*, uint16_t);
    uart_adalight->on_data = on_led_data;
    uart_hyperserial->on_data = on_led_data;
    udp_artnet->on_data = on_led_data;
    udp_e131->on_data = on_led_data;
    udp_ddp->on_data = on_led_data;
    udp_wled->on_data = on_led_data;
    
    // 初始化 UART
    uart_init();
    
    // 初始化 WiFi
    wifi_init();
    
    // 创建任务
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    xTaskCreate(effect_task, "effect_task", 4096, NULL, 5, NULL);
    
    // 创建 UDP 服务器任务
    xTaskCreate(udp_server_task, "artnet", 4096, (void*)UDP_PORT_ARTNET, 8, NULL);
    xTaskCreate(udp_server_task, "e131", 4096, (void*)UDP_PORT_E131, 8, NULL);
    xTaskCreate(udp_server_task, "ddp", 4096, (void*)UDP_PORT_DDP, 8, NULL);
    xTaskCreate(udp_server_task, "wled", 4096, (void*)UDP_PORT_WLED, 8, NULL);
    
    printf("System ready!\n");
}

#endif // ESP32
```

## 9. STM32 平台实现 (platform_stm32.c)

```c
/*============================================================================
 * STM32 平台实现
 * 使用 SPI + DMA 驱动 WS2812
 *============================================================================*/

#ifdef STM32F4

#include "stm32f4xx_hal.h"
#include "led_core.h"
#include "led_protocol.h"

/*============================================================================
 * 配置
 *============================================================================*/

#define LED_COUNT           60
#define SPI_HANDLE          hspi1
#define UART_HANDLE         huart1

#define WS2812_RESET_BYTES  50
#define WS2812_BIT_0        0xC0
#define WS2812_BIT_1        0xF8

/*============================================================================
 * 外部句柄
 *============================================================================*/

extern SPI_HandleTypeDef SPI_HANDLE;
extern UART_HandleTypeDef UART_HANDLE;
extern DMA_HandleTypeDef hdma_spi1_tx;

/*============================================================================
 * 全局变量
 *============================================================================*/

static uint8_t dma_buffer[LED_COUNT * 24 + WS2812_RESET_BYTES];
static volatile bool dma_busy = false;

static StripManager_t* g_manager = NULL;
static LedStrip_t* g_strip = NULL;
static ProtocolHandler_t* g_adalight = NULL;
static ProtocolHandler_t* g_hyperserial = NULL;

/*============================================================================
 * 时间函数
 *============================================================================*/

uint32_t stm32_millis(void) {
    return HAL_GetTick();
}

/*============================================================================
 * SPI DMA 输出
 *============================================================================*/

void spi_ws2812_write(uint8_t* data, uint16_t len, void* user_data) {
    (void)user_data;
    
    // 等待上次传输完成
    while (dma_busy) {
        __NOP();
    }
    
    uint16_t idx = 0;
    
    // 复位信号
    for (uint16_t i = 0; i < WS2812_RESET_BYTES; i++) {
        dma_buffer[idx++] = 0x00;
    }
    
    // 转换数据
    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int8_t bit = 7; bit >= 0; bit--) {
            dma_buffer[idx++] = (byte & (1 << bit)) ? WS2812_BIT_1 : WS2812_BIT_0;
        }
    }
    
    // 启动 DMA 传输
    dma_busy = true;
    HAL_SPI_Transmit_DMA(&SPI_HANDLE, dma_buffer, idx);
}

// DMA 完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &SPI_HANDLE) {
        dma_busy = false;
    }
}

/*============================================================================
 * UART 接收
 *============================================================================*/

static uint8_t uart_rx_byte;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &UART_HANDLE) {
        // 处理接收到的字节
        if (g_adalight) {
            adalight_process_byte(g_adalight, uart_rx_byte, g_strip);
        }
        if (g_hyperserial) {
            hyperserial_process_byte(g_hyperserial, uart_rx_byte, g_strip);
        }
        
        // 继续接收
        HAL_UART_Receive_IT(&UART_HANDLE, &uart_rx_byte, 1);
    }
}

/*============================================================================
 * LED 数据回调
 *============================================================================*/

void on_led_data(LedStrip_t* strip, uint16_t offset, 
                 Color_t* colors, uint16_t count) {
    if (!strip || !colors) return;
    
    for (uint16_t i = 0; i < count && (offset + i) < strip->config.count; i++) {
        strip->leds[offset + i] = colors[i];
    }
    
    strip->dirty = true;
}

/*============================================================================
 * 初始化
 *============================================================================*/

void led_system_init(void) {
    // 创建灯带管理器
    g_manager = strip_manager_create(stm32_millis);
    
    // 创建灯带
    StripConfig_t config = {
        .type = LED_TYPE_WS2812B,
        .color_order = COLOR_ORDER_GRB,
        .count = LED_COUNT,
    };
    
    g_strip = led_strip_create(&config);
    led_strip_set_output(g_strip, spi_ws2812_write, NULL);
    led_strip_add_to_manager(g_manager, g_strip);
    
    // 设置默认效果
    segment_add(g_strip, 0, LED_COUNT);
    segment_set_mode(g_strip, 0, FX_RAINBOW_CYCLE);
    led_strip_set_brightness(g_strip, 128);
    
    // 初始化协议处理器
    g_adalight = protocol_create(PROTO_ADALIGHT, 2048);
    g_hyperserial = protocol_create(PROTO_HYPERSERIAL, 2048);
    
    g_adalight->on_data = on_led_data;
    g_hyperserial->on_data = on_led_data;
    
    // 启动 UART 接收
    HAL_UART_Receive_IT(&UART_HANDLE, &uart_rx_byte, 1);
}

/*============================================================================
 * 主循环更新
 *============================================================================*/

void led_system_update(void) {
    // 更新效果
    strip_manager_update(g_manager);
    
    // 输出显示
    if (g_strip && g_strip->dirty && !dma_busy) {
        led_strip_show(g_strip);
    }
}

/*============================================================================
 * 主程序入口 (在 main.c 中调用)
 *============================================================================*/

/*
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    
    led_system_init();
    
    while (1) {
        led_system_update();
    }
}
*/

#endif // STM32F4
```

## 10. 协议和效果速查表

### 支持的协议

|    协议     | 端口/接口 |        描述        |       兼容软件       |
| :---------: | :-------: | :----------------: | :------------------: |
|  Adalight   |  Serial   |    简单RGB协议     | Prismatik, Lightpack |
| HyperSerial |  Serial   | 高性能协议,支持HDR |       HyperHDR       |
|    TPM2     |  Serial   |    通用LED协议     |   Jinx!, Glediator   |
|   Art-Net   | UDP:6454  |    DMX over IP     | MadMapper, Resolume  |
| E1.31/sACN  | UDP:5568  |      流式ACN       |    xLights, Vixen    |
|     DDP     | UDP:4048  |   分布式显示协议   |    Falcon Player     |
|  WLED UDP   | UDP:21324 |    WLED原生协议    |       WLED App       |

### 效果分类

|   类别   | 效果数量 |            示例            |
| :------: | :------: | :------------------------: |
| 基础效果 |    20    | Solid, Blink, Breath, Wipe |
| 彩虹效果 |    10    |   Rainbow, Rainbow Cycle   |
| 追逐效果 |    15    |    Chase, Theater Chase    |
| 扫描效果 |    10    |    Scan, Larson Scanner    |
| 火焰效果 |    8     |     Fire 2012, Candle      |
| 流星效果 |    6     |       Meteor, Comet        |
| 闪烁效果 |    12    |  Twinkle, Sparkle, Strobe  |
| 特殊效果 |    20    |  Pacifica, Aurora, Plasma  |
|  2D效果  |    40    |    Matrix, Plasma Ball     |
| 音频效果 |    20    |     VU Meter, Spectrum     |

### 架构总览

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                           应用层 (Application)                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │
│  │  命令行CLI  │ │  Web界面   │ │  手机APP   │ │  自动化脚本 │       │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │
├─────────────────────────────────────────────────────────────────────────┤
│                           协议层 (Protocol)                              │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐    │
│  │Adalight│ │HyperSer│ │  TPM2  │ │Art-Net │ │ E1.31  │ │  DDP   │    │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘    │
├─────────────────────────────────────────────────────────────────────────┤
│                           效果层 (Effects)                               │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    180+ 效果 (WS2812FX + WLED)                   │   │
│  │  基础 │ 彩虹 │ 追逐 │ 火焰 │ 流星 │ 2D │ 音频 │ 自定义          │   │
│  └─────────────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────────────┤
│                           核心层 (Core)                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │
│  │StripManager │ │  LedStrip   │ │  Segment    │ │  Palette    │       │
│  │  多灯带管理  │ │  灯带对象   │ │  线段对象   │ │  调色板     │       │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │
├─────────────────────────────────────────────────────────────────────────┤
│                           工具层 (Utils)                                 │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐    │
│  │颜色转换│ │数学函数│ │ 噪声  │ │ 随机数 │ │ 插值  │ │ Gamma │    │
│  └────────┘ └────────┘ └────────┘ └────────┘ └────────┘ └────────┘    │
├─────────────────────────────────────────────────────────────────────────┤
│                        硬件抽象层 (HAL)                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │
│  │ ESP32 RMT   │ │ ESP32 I2S   │ │ STM32 SPI   │ │  Bit-Bang   │       │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │
└─────────────────────────────────────────────────────────────────────────┘
```

这个综合库整合了:

1. **WS2812FX** - 经典效果库的所有效果
2. **WLED** - 现代化效果和2D/音频支持
3. **HyperSerial** - 高性能串口协议,支持HDR
4. **多种网络协议** - Art-Net, E1.31, DDP等专业灯光协议
5. **多平台支持** - ESP32, STM32等嵌入式平台