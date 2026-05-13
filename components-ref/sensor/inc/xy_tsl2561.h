/**
 * @file xy_tsl2561.h
 * @brief TSL2561 Digital Light Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_TSL2561_H
#define XY_TSL2561_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief TSL2561 I2C 地址
 */
#define TSL2561_ADDR_FLOAT      0x29  /* ADDR = FLOAT */
#define TSL2561_ADDR_LOW        0x39  /* ADDR = GND */
#define TSL2561_ADDR_HIGH       0x49  /* ADDR = VCC */

/**
 * @brief TSL2561 寄存器地址
 */
#define TSL2561_REG_CONTROL     0x00
#define TSL2561_REG_TIMING      0x01
#define TSL2561_REG_THRESH_L    0x02
#define TSL2561_REG_THRESH_H    0x03
#define TSL2561_REG_INTERRUPT   0x04
#define TSL2561_REG_BYTE_DATA   0x05
#define TSL2561_REG_WORD_DATA   0x06
#define TSL2561_REG_BLOCK_DATA  0x07
#define TSL2561_REG_ID          0x0A
#define TSL2561_REG_DATA0_L     0x0C
#define TSL2561_REG_DATA0_H     0x0D
#define TSL2561_REG_DATA1_L     0x0E
#define TSL2561_REG_DATA1_H     0x0F

/**
 * @brief 命令寄存器位
 */
#define TSL2561_CMD_BIT         0x80
#define TSL2561_WORD_BIT        0x10
#define TSL2561_BLOCK_BIT       0x20
#define TSL2561_SPECIAL_BIT     0x60

/**
 * @brief 错误码
 */
#define XY_TSL2561_OK           0
#define XY_TSL2561_ERROR        (-1)
#define XY_TSL2561_INVALID_PARAM (-2)
#define XY_TSL2561_NOT_FOUND    (-3)

/**
 * @brief 增益设置
 */
typedef enum {
    XY_TSL2561_GAIN_1X = 0,     /* 1x 增益 (无放大) */
    XY_TSL2561_GAIN_16X = 1,    /* 16x 增益 */
} xy_tsl2561_gain_t;

/**
 * @brief 积分时间
 */
typedef enum {
    XY_TSL2561_INTEGRATION_13MS = 0,    /* 13.7ms */
    XY_TSL2561_INTEGRATION_101MS = 1,   /* 101ms */
    XY_TSL2561_INTEGRATION_402MS = 2,   /* 402ms */
} xy_tsl2561_integration_t;

/**
 * @brief 传感器数据
 */
typedef struct {
    uint16_t broadband;         /* 宽带通道数据 */
    uint16_t ir;                /* 红外通道数据 */
    float lux;                  /* 照度 (lux) */
    uint32_t timestamp;         /* 时间戳 (ms) */
} xy_tsl2561_data_t;

/**
 * @brief TSL2561 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_tsl2561_gain_t gain;     /* 增益设置 */
    xy_tsl2561_integration_t integration;  /* 积分时间 */
    xy_tsl2561_data_t data;     /* 最新数据 */
    bool initialized;           /* 初始化标志 */
} xy_tsl2561_t;

/**
 * @brief 初始化 TSL2561
 * @param tsl2561 TSL2561 句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_init(xy_tsl2561_t *tsl2561, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化
 * @param tsl2561 TSL2561 句柄
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_deinit(xy_tsl2561_t *tsl2561);

/**
 * @brief 读取照度数据
 * @param tsl2561 TSL2561 句柄
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_read(xy_tsl2561_t *tsl2561);

/**
 * @brief 获取宽带通道数据
 * @param tsl2561 TSL2561 句柄
 * @param broadband 宽带数据指针
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_get_broadband(xy_tsl2561_t *tsl2561, uint16_t *broadband);

/**
 * @brief 获取红外通道数据
 * @param tsl2561 TSL2561 句柄
 * @param ir 红外数据指针
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_get_ir(xy_tsl2561_t *tsl2561, uint16_t *ir);

/**
 * @brief 获取照度值
 * @param tsl2561 TSL2561 句柄
 * @param lux 照度指针 (lux)
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_get_lux(xy_tsl2561_t *tsl2561, float *lux);

/**
 * @brief 设置增益
 * @param tsl2561 TSL2561 句柄
 * @param gain 增益
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_set_gain(xy_tsl2561_t *tsl2561, xy_tsl2561_gain_t gain);

/**
 * @brief 设置积分时间
 * @param tsl2561 TSL2561 句柄
 * @param integration 积分时间
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_set_integration(xy_tsl2561_t *tsl2561, 
                               xy_tsl2561_integration_t integration);

/**
 * @brief 使能传感器
 * @param tsl2561 TSL2561 句柄
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_enable(xy_tsl2561_t *tsl2561);

/**
 * @brief 禁用传感器
 * @param tsl2561 TSL2561 句柄
 * @return XY_TSL2561_OK 成功，其他值失败
 */
int xy_tsl2561_disable(xy_tsl2561_t *tsl2561);

#ifdef __cplusplus
}
#endif

#endif
