/**
 * @file xy_bh1750.h
 * @brief BH1750 Ambient Light Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_BH1750_H
#define XY_BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief BH1750 I2C 地址
 */
#define BH1750_ADDR_LOW         0x23  /* ADDR = LOW */
#define BH1750_ADDR_HIGH        0x5C  /* ADDR = HIGH */

/**
 * @brief BH1750 命令
 */
#define BH1750_CMD_POWER_DOWN   0x00
#define BH1750_CMD_POWER_ON     0x01
#define BH1750_CMD_RESET        0x07
#define BH1750_CMD_CONT_H       0x10  /* 连续高分辨率模式 1 */
#define BH1750_CMD_CONT_H2      0x11  /* 连续高分辨率模式 2 */
#define BH1750_CMD_CONT_L       0x13  /* 连续低分辨率模式 */
#define BH1750_CMD_ONCE_H       0x20  /* 单次高分辨率模式 1 */
#define BH1750_CMD_ONCE_H2      0x21  /* 单次高分辨率模式 2 */
#define BH1750_CMD_ONCE_L       0x23  /* 单次低分辨率模式 */

/**
 * @brief 错误码
 */
#define XY_BH1750_OK            0
#define XY_BH1750_ERROR         (-1)
#define XY_BH1750_INVALID_PARAM (-2)
#define XY_BH1750_NOT_FOUND     (-3)

/**
 * @brief 分辨率模式
 */
typedef enum {
    XY_BH1750_HIGH_RES = 0,     /* 高分辨率 (1 lux) */
    XY_BH1750_HIGH_RES2,        /* 高分辨率 2 (0.5 lux) */
    XY_BH1750_LOW_RES,          /* 低分辨率 (4 lux) */
} xy_bh1750_res_t;

/**
 * @brief 测量模式
 */
typedef enum {
    XY_BH1750_CONTINUOUS = 0,   /* 连续模式 */
    XY_BH1750_ONE_TIME,         /* 单次模式 */
} xy_bh1750_mode_t;

/**
 * @brief 传感器数据
 */
typedef struct {
    float illuminance;          /* 照度 (lux) */
    uint32_t timestamp;         /* 时间戳 (ms) */
} xy_bh1750_data_t;

/**
 * @brief BH1750 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_bh1750_res_t resolution; /* 分辨率 */
    xy_bh1750_mode_t mode;      /* 测量模式 */
    xy_bh1750_data_t data;      /* 最新数据 */
    bool initialized;           /* 初始化标志 */
} xy_bh1750_t;

/**
 * @brief 初始化 BH1750
 * @param bh1750 BH1750 句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_init(xy_bh1750_t *bh1750, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化
 * @param bh1750 BH1750 句柄
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_deinit(xy_bh1750_t *bh1750);

/**
 * @brief 读取照度
 * @param bh1750 BH1750 句柄
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_read(xy_bh1750_t *bh1750);

/**
 * @brief 获取照度值
 * @param bh1750 BH1750 句柄
 * @param illuminance 照度指针 (lux)
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_get_illuminance(xy_bh1750_t *bh1750, float *illuminance);

/**
 * @brief 设置分辨率
 * @param bh1750 BH1750 句柄
 * @param resolution 分辨率
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_set_resolution(xy_bh1750_t *bh1750, xy_bh1750_res_t resolution);

/**
 * @brief 设置测量模式
 * @param bh1750 BH1750 句柄
 * @param mode 测量模式
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_set_mode(xy_bh1750_t *bh1750, xy_bh1750_mode_t mode);

/**
 * @brief 关机
 * @param bh1750 BH1750 句柄
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_power_down(xy_bh1750_t *bh1750);

/**
 * @brief 开机
 * @param bh1750 BH1750 句柄
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_power_on(xy_bh1750_t *bh1750);

/**
 * @brief 软件复位
 * @param bh1750 BH1750 句柄
 * @return XY_BH1750_OK 成功，其他值失败
 */
int xy_bh1750_reset(xy_bh1750_t *bh1750);

#ifdef __cplusplus
}
#endif

#endif
