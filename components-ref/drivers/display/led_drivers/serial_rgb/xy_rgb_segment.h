/**
 * @file xy_rgb_segment.h
 * @brief RGB LED Segment Management
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_SEGMENT_H
#define XY_RGB_SEGMENT_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED 分段结构
 */
typedef struct {
    uint16_t start;         /* 起始 LED 索引 */
    uint16_t stop;          /* 结束 LED 索引 */
    uint16_t speed;         /* 速度 (0-65535) */
    uint16_t intensity;     /* 强度 (0-65535) */
    rgb_color_t color1;     /* 主颜色 */
    rgb_color_t color2;     /* 次颜色 */
    rgb_color_t color3;     /* 第三颜色 */
    xy_rgb_effect_t effect; /* 效果 ID */
    bool reverse;           /* 反向 */
    bool enabled;           /* 使能 */
} xy_rgb_segment_t;

/**
 * @brief 创建分段
 * @param start 起始 LED 索引
 * @param stop 结束 LED 索引
 * @return 分段 ID，负值表示失败
 */
int32_t xy_rgb_create_segment(uint16_t start, uint16_t stop);

/**
 * @brief 删除分段
 * @param seg_id 分段 ID
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_delete_segment(uint8_t seg_id);

/**
 * @brief 设置分段效果
 * @param seg_id 分段 ID
 * @param effect 效果 ID
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_set_segment_effect(uint8_t seg_id, xy_rgb_effect_t effect);

/**
 * @brief 设置分段颜色
 * @param seg_id 分段 ID
 * @param color1 主颜色
 * @param color2 次颜色 (可选)
 * @param color3 第三颜色 (可选)
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_set_segment_colors(uint8_t seg_id, rgb_color_t color1, 
                                   rgb_color_t color2, rgb_color_t color3);

/**
 * @brief 设置分段参数
 * @param seg_id 分段 ID
 * @param speed 速度
 * @param intensity 强度
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_set_segment_params(uint8_t seg_id, uint16_t speed, uint16_t intensity);

/**
 * @brief 设置分段方向
 * @param seg_id 分段 ID
 * @param reverse true=反向，false=正向
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_set_segment_reverse(uint8_t seg_id, bool reverse);

/**
 * @brief 使能/禁用分段
 * @param seg_id 分段 ID
 * @param enabled true=使能，false=禁用
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_set_segment_enabled(uint8_t seg_id, bool enabled);

/**
 * @brief 获取分段数量
 * @return 分段数量
 */
uint8_t xy_rgb_get_segment_count(void);

/**
 * @brief 获取分段
 * @param seg_id 分段 ID
 * @return 分段指针，NULL 表示无效
 */
xy_rgb_segment_t* xy_rgb_get_segment(uint8_t seg_id);

/**
 * @brief 清除所有分段
 */
void xy_rgb_clear_segments(void);

/**
 * @brief 更新所有分段
 */
void xy_rgb_update_segments(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_SEGMENT_H */
