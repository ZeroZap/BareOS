/**
 * @file xy_ads1115.h
 * @brief ADS1115 4-Channel 16-Bit ADC Driver
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_ADS1115_H
#define XY_ADS1115_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief ADS1115 默认 I2C 地址
 */
#define ADS1115_ADDR_GND        0x48  /* ADDR -> GND */
#define ADS1115_ADDR_VDD        0x49  /* ADDR -> VDD */
#define ADS1115_ADDR_SDA        0x4A  /* ADDR -> SDA */
#define ADS1115_ADDR_SCL        0x4B  /* ADDR -> SCL */

/**
 * @brief ADS1115 寄存器
 */
#define ADS1115_REG_CONVERT     0x00
#define ADS1115_REG_CONFIG      0x01
#define ADS1115_REG_LO_THRESH   0x02
#define ADS1115_REG_HI_THRESH   0x03

/**
 * @brief 配置寄存器位定义
 */
#define ADS1115_CONFIG_OS_SINGLE    (1 << 15)   /* 开始单次转换 */
#define ADS1115_CONFIG_MUX_DIFF_0_1 (0 << 12)   /* 差分 AIN0-AIN1 */
#define ADS1115_CONFIG_MUX_DIFF_0_3 (1 << 12)   /* 差分 AIN0-AIN3 */
#define ADS1115_CONFIG_MUX_DIFF_1_3 (2 << 12)   /* 差分 AIN1-AIN3 */
#define ADS1115_CONFIG_MUX_DIFF_2_3 (3 << 12)   /* 差分 AIN2-AIN3 */
#define ADS1115_CONFIG_MUX_SINGLE_0 (4 << 12)   /* 单端 AIN0 */
#define ADS1115_CONFIG_MUX_SINGLE_1 (5 << 12)   /* 单端 AIN1 */
#define ADS1115_CONFIG_MUX_SINGLE_2 (6 << 12)   /* 单端 AIN2 */
#define ADS1115_CONFIG_MUX_SINGLE_3 (7 << 12)   /* 单端 AIN3 */
#define ADS1115_CONFIG_PGA_6_144V   (0 << 9)    /* ±6.144V */
#define ADS1115_CONFIG_PGA_4_096V   (1 << 9)    /* ±4.096V */
#define ADS1115_CONFIG_PGA_2_048V   (2 << 9)    /* ±2.048V */
#define ADS1115_CONFIG_PGA_1_024V   (3 << 9)    /* ±1.024V */
#define ADS1115_CONFIG_PGA_0_512V   (4 << 9)    /* ±0.512V */
#define ADS1115_CONFIG_PGA_0_256V   (5 << 9)    /* ±0.256V */
#define ADS1115_CONFIG_MODE_SINGLE  (0 << 8)    /* 单次模式 */
#define ADS1115_CONFIG_MODE_CONT    (1 << 8)    /* 连续模式 */
#define ADS1115_CONFIG_DR_8SPS      (0 << 5)    /* 8 SPS */
#define ADS1115_CONFIG_DR_16SPS     (1 << 5)    /* 16 SPS */
#define ADS1115_CONFIG_DR_32SPS     (2 << 5)    /* 32 SPS */
#define ADS1115_CONFIG_DR_64SPS     (3 << 5)    /* 64 SPS */
#define ADS1115_CONFIG_DR_128SPS    (4 << 5)    /* 128 SPS (默认) */
#define ADS1115_CONFIG_DR_250SPS    (5 << 5)    /* 250 SPS */
#define ADS1115_CONFIG_DR_475SPS    (6 << 5)    /* 475 SPS */
#define ADS1115_CONFIG_DR_860SPS    (7 << 5)    /* 860 SPS */
#define ADS1115_CONFIG_COMP_MODE_TRAD (0 << 4)  /* 传统比较器 */
#define ADS1115_CONFIG_COMP_MODE_WINDOW (1 << 4)/* 窗口比较器 */
#define ADS1115_CONFIG_COMP_POL_LOW   (0 << 3)  /* 低电平有效 */
#define ADS1115_CONFIG_COMP_POL_HIGH  (1 << 3)  /* 高电平有效 */
#define ADS1115_CONFIG_COMP_LAT_NON   (0 << 2)  /* 非锁存 */
#define ADS1115_CONFIG_COMP_LAT_LATCH (1 << 2)  /* 锁存 */
#define ADS1115_CONFIG_COMP_QUIET_1   (0 << 0)  /* 1 次转换后关断 */
#define ADS1115_CONFIG_COMP_QUIET_2   (1 << 0)  /* 2 次转换后关断 */
#define ADS1115_CONFIG_COMP_QUIET_4   (2 << 0)  /* 4 次转换后关断 */
#define ADS1115_CONFIG_COMP_DISABLE   (3 << 0)  /* 禁用比较器 */

/**
 * @brief 增益范围
 */
typedef enum {
    ADS1115_PGA_6_144V = 0,   /**< ±6.144V */
    ADS1115_PGA_4_096V = 1,   /**< ±4.096V */
    ADS1115_PGA_2_048V = 2,   /**< ±2.048V */
    ADS1115_PGA_1_024V = 3,   /**< ±1.024V */
    ADS1115_PGA_0_512V = 4,   /**< ±0.512V */
    ADS1115_PGA_0_256V = 5,   /**< ±0.256V */
} xy_ads1115_pga_t;

/**
 * @brief 数据速率
 */
typedef enum {
    ADS1115_DR_8SPS = 0,      /**< 8 SPS */
    ADS1115_DR_16SPS = 1,     /**< 16 SPS */
    ADS1115_DR_32SPS = 2,     /**< 32 SPS */
    ADS1115_DR_64SPS = 3,     /**< 64 SPS */
    ADS1115_DR_128SPS = 4,    /**< 128 SPS (默认) */
    ADS1115_DR_250SPS = 5,    /**< 250 SPS */
    ADS1115_DR_475SPS = 6,    /**< 475 SPS */
    ADS1115_DR_860SPS = 7,    /**< 860 SPS */
} xy_ads1115_dr_t;

/**
 * @brief 错误码
 */
#define XY_ADS1115_OK           0
#define XY_ADS1115_ERROR        (-1)
#define XY_ADS1115_INVALID_PARAM (-2)
#define XY_ADS1115_NOT_FOUND    (-3)
#define XY_ADS1115_TIMEOUT      (-4)

/**
 * @brief ADS1115 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /**< I2C 设备 */
    uint8_t addr;               /**< I2C 地址 */
    xy_ads1115_pga_t pga;       /**< 增益设置 */
    xy_ads1115_dr_t dr;         /**< 数据速率 */
    int16_t last_value;         /**< 上次转换值 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_ads1115_t;

/**
 * @brief 初始化 ADS1115
 * @param dev ADS1115 设备句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_init(xy_ads1115_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化 ADS1115
 * @param dev ADS1115 设备句柄
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_deinit(xy_ads1115_t *dev);

/**
 * @brief 读取单端 ADC 值
 * @param dev ADS1115 设备句柄
 * @param channel 通道 (0-3)
 * @param value ADC 值指针
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_read_single(xy_ads1115_t *dev, uint8_t channel, int16_t *value);

/**
 * @brief 读取差分 ADC 值
 * @param dev ADS1115 设备句柄
 * @param channel_p 正通道 (0-2)
 * @param channel_n 负通道 (1-3)
 * @param value ADC 值指针
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_read_diff(xy_ads1115_t *dev, uint8_t channel_p, uint8_t channel_n, int16_t *value);

/**
 * @brief 读取电压 (mV)
 * @param dev ADS1115 设备句柄
 * @param channel 通道 (0-3)
 * @param voltage_mv 电压指针 (mV)
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_read_voltage(xy_ads1115_t *dev, uint8_t channel, int32_t *voltage_mv);

/**
 * @brief 设置增益
 * @param dev ADS1115 设备句柄
 * @param pga 增益
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_set_pga(xy_ads1115_t *dev, xy_ads1115_pga_t pga);

/**
 * @brief 设置数据速率
 * @param dev ADS1115 设备句柄
 * @param dr 数据速率
 * @return XY_ADS1115_OK 成功，其他值失败
 */
int xy_ads1115_set_dr(xy_ads1115_t *dev, xy_ads1115_dr_t dr);

#ifdef __cplusplus
}
#endif

#endif /* XY_ADS1115_H */
