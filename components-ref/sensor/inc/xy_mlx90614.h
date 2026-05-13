/**
 * @file xy_mlx90614.h
 * @brief MLX90614 IR Temperature Sensor Driver
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#ifndef XY_MLX90614_H
#define XY_MLX90614_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief MLX90614 I2C 地址
 */
#define MLX90614_ADDR_DEFAULT   0x5A

/**
 * @brief MLX90614 RAM 寄存器
 */
#define MLX90614_RAM_TA         0x06  /* 环境温度 */
#define MLX90614_RAM_TOBJ1      0x07  /* 物体温度 1 */
#define MLX90614_RAM_TOBJ2      0x08  /* 物体温度 2 */

/**
 * @brief MLX90614 EEPROM 寄存器
 */
#define MLX90614_EMISSIVITY     0x24

/**
 * @brief 错误码
 */
#define XY_MLX90614_OK          0
#define XY_MLX90614_ERROR       (-1)
#define XY_MLX90614_INVALID_PARAM (-2)
#define XY_MLX90614_NOT_FOUND   (-3)

/**
 * @brief MLX90614 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    uint8_t addr;
    int16_t ta;         /* 环境温度 0.01°C */
    int16_t tobj1;      /* 物体温度 1 0.01°C */
    int16_t tobj2;      /* 物体温度 2 0.01°C */
    uint8_t initialized;
} xy_mlx90614_t;

/**
 * @brief 初始化 MLX90614
 */
int xy_mlx90614_init(xy_mlx90614_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化
 */
int xy_mlx90614_deinit(xy_mlx90614_t *dev);

/**
 * @brief 读取所有温度
 */
int xy_mlx90614_read_all(xy_mlx90614_t *dev);

/**
 * @brief 读取环境温度
 */
int xy_mlx90614_read_ambient(xy_mlx90614_t *dev, int16_t *ta);

/**
 * @brief 读取物体温度 1
 */
int xy_mlx90614_read_object1(xy_mlx90614_t *dev, int16_t *tobj);

/**
 * @brief 读取发射率 (0.001 单位)
 */
int xy_mlx90614_get_emissivity(xy_mlx90614_t *dev, uint16_t *emissivity);

/**
 * @brief 设置发射率 (0.001 单位)
 */
int xy_mlx90614_set_emissivity(xy_mlx90614_t *dev, uint16_t emissivity);

#ifdef __cplusplus
}
#endif

#endif
