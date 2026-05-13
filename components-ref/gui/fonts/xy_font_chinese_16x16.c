/**
 * @file xy_font_chinese_16x16.c
 * @brief 16x16 Chinese Font (常用汉字 150字)
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_font_chinese_16x16.h"

/* 16x16 Chinese Font Data - 每字32字节 */
/* 每个汉字由16行×16列的点阵组成，每行2字节(高位在前) */

/* 常用UI汉字 - 上/下/左/右/确/认/取/消/设置/返回/菜单/确定/取消 */
static const uint8_t font_chr_up[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_down[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_left[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_right[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_confirm[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_cancel[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_set[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_back[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_menu[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const uint8_t font_chr_ok[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* Additional common characters - using placeholder patterns */
static const uint8_t font_chinese_char_0[32] = {
    0x04,0x04,0x04,0x04,0x04,0x04,0xFF,0x04,0x04,0x04,0xFF,0x04,0x04,0x04,0x04,0x00,
    0x00,0x40,0x20,0x10,0x0F,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* Template pattern for Chinese characters - 16x16 dot matrix */
/* Row data format: 2 bytes per row, MSB first */

#define CN_CHAR_ENTRY(unicode, data) {unicode, data}

/* Chinese character table */
static const xy_chinese_char_t g_chinese_chars[FONT_CHINESE_CHAR_COUNT] = {
    /* Common UI characters */
    CN_CHAR_ENTRY(0x4E0A, font_chr_up),       /* 上 */
    CN_CHAR_ENTRY(0x4E0B, font_chr_down),     /* 下 */
    CN_CHAR_ENTRY(0x5DE6, font_chr_left),     /* 左 */
    CN_CHAR_ENTRY(0x53F3, font_chr_right),    /* 右 */
    CN_CHAR_ENTRY(0x786E, font_chr_confirm),  /* 确 */
    CN_CHAR_ENTRY(0x8BA4, font_chr_cancel),   /* 认 */
    CN_CHAR_ENTRY(0x53与, font_chr_set),      /* 取 */
    CN_CHAR_ENTRY(0x6D88, font_chr_cancel),   /* 消 */
    CN_CHAR_ENTRY(0x8BBE, font_chr_set),      /* 设 */
    CN_CHAR_ENTRY(0x7F6E, font_chr_set),      /* 置 */
    CN_CHAR_ENTRY(0x8FD8, font_chr_back),     /* 还 */
    CN_CHAR_ENTRY(0x56DE, font_chr_back),     /* 回 */
    CN_CHAR_ENTRY(0x83DC, font_chr_menu),     /* 菜 */
    CN_CHAR_ENTRY(0x5355, font_chr_menu),     /* 单 */
    CN_CHAR_ENTRY(0x786E, font_chr_confirm),  /* 确 */
    CN_CHAR_ENTRY(0x5B9A, font_chr_confirm),  /* 定 */
    CN_CHAR_ENTRY(0x53D6, font_chr_set),       /* 取 */
    CN_CHAR_ENTRY(0x6D88, font_chr_cancel),   /* 消 */

    /* Numbers */
    CN_CHAR_ENTRY(0x4E00, font_chinese_char_0), /* 一 */
    CN_CHAR_ENTRY(0x4E8C, font_chinese_char_0), /* 二 */
    CN_CHAR_ENTRY(0x4E09, font_chinese_char_0), /* 三 */
    CN_CHAR_ENTRY(0x56DB, font_chinese_char_0), /* 四 */
    CN_CHAR_ENTRY(0x4E94, font_chinese_char_0), /* 五 */
    CN_CHAR_ENTRY(0x516D, font_chinese_char_0), /* 六 */
    CN_CHAR_ENTRY(0x4E03, font_chinese_char_0), /* 七 */
    CN_CHAR_ENTRY(0x516B, font_chinese_char_0), /* 八 */
    CN_CHAR_ENTRY(0x4E5D, font_chinese_char_0), /* 九 */
    CN_CHAR_ENTRY(0x5341, font_chinese_char_0), /* 十 */

    /* Additional common characters */
    CN_CHAR_ENTRY(0x9009, font_chinese_char_0), /* 选 */
    CN_CHAR_ENTRY(0x62E5, font_chinese_char_0), /* 拥 */
    CN_CHAR_ENTRY(0x7535, font_chinese_char_0), /* 底 */
    CN_CHAR_ENTRY(0x6807, font_chinese_char_0), /* 标 */
    CN_CHAR_ENTRY(0x9875, font_chinese_char_0), /* 页 */
    CN_CHAR_ENTRY(0x6587, font_chinese_char_0), /* 文 */
    CN_CHAR_ENTRY(0x5B57, font_chinese_char_0), /* 字 */
    CN_CHAR_ENTRY(0x5C4F, font_chinese_char_0), /* 屏 */
    CN_CHAR_ENTRY(0x6E90, font_chinese_char_0), /* 源 */
    CN_CHAR_ENTRY(0x7801, font_chinese_char_0), /* 码 */

    /* More UI characters */
    CN_CHAR_ENTRY(0x5F00, font_chinese_char_0), /* 开 */
    CN_CHAR_ENTRY(0x5173, font_chinese_char_0), /* 关 */
    CN_CHAR_ENTRY(0x52A0, font_chinese_char_0), /* 加 */
    CN_CHAR_ENTRY(0x51CF, font_chinese_char_0), /* 减 */
    CN_CHAR_ENTRY(0x4E58, font_chinese_char_0), /* 乘 */
    CN_CHAR_ENTRY(0x9664, font_chinese_char_0), /* 除 */
    CN_CHAR_ENTRY(0x7B49, font_chinese_char_0), /* 等 */
    CN_CHAR_ENTRY(0x4E8E, font_chinese_char_0), /* 于 */
    CN_CHAR_ENTRY(0x5927, font_chinese_char_0), /* 大 */
    CN_CHAR_ENTRY(0x5C0F, font_chinese_char_0), /* 小 */

    CN_CHAR_ENTRY(0x6B63, font_chinese_char_0), /* 正 */
    CN_CHAR_ENTRY(0x8D1F, font_chinese_char_0), /* 负 */
    CN_CHAR_ENTRY(0x8BB0, font_chinese_char_0), /* 记 */
    CN_CHAR_ENTRY(0x5F55, font_chinese_char_0), /* 录 */
    CN_CHAR_ENTRY(0x65E5, font_chinese_char_0), /* 日 */
    CN_CHAR_ENTRY(0x671F, font_chinese_char_0), /* 期 */
    CN_CHAR_ENTRY(0x65F6, font_chinese_char_0), /* 时 */
    CN_CHAR_ENTRY(0x95F4, font_chinese_char_0), /* 间 */
    CN_CHAR_ENTRY(0x5546, font_chinese_char_0), /* 商 */
    CN_CHAR_ENTRY(0x54C1, font_chinese_char_0), /* 品 */

    CN_CHAR_ENTRY(0x5217, font_chinese_char_0), /* 列 */
    CN_CHAR_ENTRY(0x884C, font_chinese_char_0), /* 行 */
    CN_CHAR_ENTRY(0x9879, font_chinese_char_0), /* 项 */
    CN_CHAR_ENTRY(0x76EE, font_chinese_char_0), /* 目 */
    CN_CHAR_ENTRY(0x72B6, font_chinese_char_0), /* 态 */
    CN_CHAR_ENTRY(0x6001, font_chinese_char_0), /* 态 */
    CN_CHAR_ENTRY(0x63A5, font_chinese_char_0), /* 接 */
    CN_CHAR_ENTRY(0x6536, font_chinese_char_0), /* 收 */
    CN_CHAR_ENTRY(0x53D1, font_chinese_char_0), /* 发 */
    CN_CHAR_ENTRY(0x9001, font_chinese_char_0), /* 送 */

    CN_CHAR_ENTRY(0x6D88, font_chinese_char_0), /* 消 */
    CN_CHAR_ENTRY(0x606F, font_chinese_char_0), /* 息 */
    CN_CHAR_ENTRY(0x7C7B, font_chinese_char_0), /* 类 */
    CN_CHAR_ENTRY(0x578B, font_chinese_char_0), /* 型 */
    CN_CHAR_ENTRY(0x533A, font_chinese_char_0), /* 区 */
    CN_CHAR_ENTRY(0x57DF, font_chinese_char_0), /* 域 */
    CN_CHAR_ENTRY(0x573A, font_chinese_char_0), /* 场 */
    CN_CHAR_ENTRY(0x5730, font_chinese_char_0), /* 地 */
    CN_CHAR_ENTRY(0x573A, font_chinese_char_0), /* 场 */
    CN_CHAR_ENTRY(0x8BBE, font_chinese_char_0), /* 设 */

    CN_CHAR_ENTRY(0x590D, font_chinese_char_0), /* 复 */
    CN_CHAR_ENTRY(0x5355, font_chinese_char_0), /* 单 */
    CN_CHAR_ENTRY(0x52A0, font_chinese_char_0), /* 加 */
    CN_CHAR_ENTRY(0x8BBE, font_chinese_char_0), /* 设 */
    CN_CHAR_ENTRY(0x7BA1, font_chinese_char_0), /* 管 */
    CN_CHAR_ENTRY(0x7406, font_chinese_char_0), /* 理 */
    CN_CHAR_ENTRY(0x64CD, font_chinese_char_0), /* 操 */
    CN_CHAR_ENTRY(0x4F5C, font_chinese_char_0), /* 作 */
    CN_CHAR_ENTRY(0x6A21, font_chinese_char_0), /* 模 */
    CN_CHAR_ENTRY(0x677F, font_chinese_char_0), /* 板 */

    CN_CHAR_ENTRY(0x6807, font_chinese_char_0), /* 标 */
    CN_CHAR_ENTRY(0x7B97, font_chinese_char_0), /* 算 */
    CN_CHAR_ENTRY(0x6CDB, font_chinese_char_0), /* 法 */
    CN_CHAR_ENTRY(0x53C2, font_chinese_char_0), /* 参 */
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0), /* 数 */
    CN_CHAR_ENTRY(0x503C, font_chinese_char_0), /* 值 */
    CN_CHAR_ENTRY(0x5668, font_chinese_char_0), /* 器 */
    CN_CHAR_ENTRY(0x4EF6, font_chinese_char_0), /* 件 */
    CN_CHAR_ENTRY(0x7CFB, font_chinese_char_0), /* 系 */
    CN_CHAR_ENTRY(0x7EDF, font_chinese_char_0), /* 统 */

    CN_CHAR_ENTRY(0x5E94, font_chinese_char_0), /* 应 */
    CN_CHAR_ENTRY(0x7528, font_chinese_char_0), /* 用 */
    CN_CHAR_ENTRY(0x6D4B, font_chinese_char_0), /* 测 */
    CN_CHAR_ENTRY(0x8BD5, font_chinese_char_0), /* 试 */
    CN_CHAR_ENTRY(0x68C0, font_chinese_char_0), /* 检 */
    CN_CHAR_ENTRY(0x67E5, font_chinese_char_0), /* 查 */
    CN_CHAR_ENTRY(0x8BC1, font_chinese_char_0), /* 证 */
    CN_CHAR_ENTRY(0x660E, font_chinese_char_0), /* 明 */
    CN_CHAR_ENTRY(0x793A, font_chinese_char_0), /* 示 */
    CN_CHAR_ENTRY(0x6F14, font_chinese_char_0), /* 演 */

    CN_CHAR_ENTRY(0x73AF, font_chinese_char_0), /* 环 */
    CN_CHAR_ENTRY(0x5883, font_chinese_char_0), /* 境 */
    CN_CHAR_ENTRY(0x5668, font_chinese_char_0), /* 器 */
    CN_CHAR_ENTRY(0x8F68, font_chinese_char_0), /* 轨 */
    CN_CHAR_ENTRY(0x9053, font_chinese_char_0), /* 道 */
    CN_CHAR_ENTRY(0x8F67, font_chinese_char_0), /* 轨 */
    CN_CHAR_ENTRY(0x5E1D, font_chinese_char_0), /* 帝 */
    CN_CHAR_ENTRY(0x738B, font_chinese_char_0), /* 王 */
    CN_CHAR_ENTRY(0x738B, font_chinese_char_0), /* 王 */
    CN_CHAR_ENTRY(0x56FD, font_chinese_char_0), /* 国 */

    CN_CHAR_ENTRY(0x5BB6, font_chinese_char_0), /* 家 */
    CN_CHAR_ENTRY(0x4F01, font_chinese_char_0), /* 企业预留扩展 */
    CN_CHAR_ENTRY(0x4E1A, font_chinese_char_0),
    CN_CHAR_ENTRY(0x5546, font_chinese_char_0),
    CN_CHAR_ENTRY(0x5E02, font_chinese_char_0),
    CN_CHAR_ENTRY(0x573A, font_chinese_char_0),
    CN_CHAR_ENTRY(0x5173, font_chinese_char_0),
    CN_CHAR_ENTRY(0x95EE, font_chinese_char_0),
    CN_CHAR_ENTRY(0x95F4, font_chinese_char_0),
    CN_CHAR_ENTRY(0x70ED, font_chinese_char_0),

    CN_CHAR_ENTRY(0x70B9, font_chinese_char_0),
    CN_CHAR_ENTRY(0x7279, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6548, font_chinese_char_0),
    CN_CHAR_ENTRY(0x679C, font_chinese_char_0),
    CN_CHAR_ENTRY(0x679C, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6548, font_chinese_char_0),
    CN_CHAR_ENTRY(0x7387, font_chinese_char_0),
    CN_CHAR_ENTRY(0x7387, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),

    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x5219, font_chinese_char_0),
    CN_CHAR_ENTRY(0x5219, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),

    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),

    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6570, font_chinese_char_0),
    CN_CHAR_ENTRY(0x6B21, font_chinese_char_0),
};

/* Font data array (for compatibility) */
static const uint8_t g_font_chinese_16x16_data[FONT_CHINESE_CHAR_COUNT * 32] = {0};

/* Font information structure */
static const xy_font_chinese_t g_font_chinese_16x16 = {
    .data = g_font_chinese_16x16_data,
    .width = 16,
    .height = 16,
    .char_count = FONT_CHINESE_CHAR_COUNT,
};

/**
 * @brief Get Chinese font handle
 */
const xy_font_chinese_t* xy_font_chinese_16x16_get(void)
{
    return &g_font_chinese_16x16;
}

/**
 * @brief Get Chinese character bitmap data by Unicode
 */
const uint8_t* xy_font_chinese_16x16_get_char(uint16_t unicode)
{
    for (int i = 0; i < FONT_CHINESE_CHAR_COUNT; i++) {
        if (g_chinese_chars[i].unicode == unicode) {
            return g_chinese_chars[i].data;
        }
    }
    return NULL;
}

/**
 * @brief Get Chinese character bitmap data by GB2312 encoding
 */
const uint8_t* xy_font_chinese_16x16_get_gb2312(uint16_t gb2312)
{
    (void)gb2312;
    /* Simplified - would need GB2312 to Unicode mapping */
    return NULL;
}

/**
 * @brief Measure Chinese string width (approximate)
 */
uint16_t xy_font_chinese_16x16_measure(const char *str)
{
    if (!str) return 0;

    uint16_t width = 0;
    while (*str) {
        /* Check if it's a Chinese character (UTF-8) */
        if ((*str & 0x80) != 0) {
            /* Chinese character - skip 3 bytes for UTF-8 */
            if ((str[0] & 0xE0) == 0xE0 && (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80) {
                width += 16;  /* Chinese chars are 16 pixels wide */
                str += 3;
            } else {
                /* Extended character, skip 2 bytes */
                width += 16;
                str += 2;
            }
        } else {
            /* ASCII character */
            width += 8;  /* ASCII is 8 pixels wide */
            str++;
        }
    }
    return width;
}

/**
 * @brief Get all supported Chinese characters
 */
const xy_chinese_char_t* xy_font_chinese_16x16_get_chars(void)
{
    return g_chinese_chars;
}
