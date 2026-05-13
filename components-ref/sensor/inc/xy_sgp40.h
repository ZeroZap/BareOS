/**
 * @file xy_sgp40.h
 * @brief SGP40 VOC (Volatile Organic Compounds) Gas Sensor Driver
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * SGP40 是 Sensirion 出品的 VOC 气体传感器
 * 特点：
 * - VOC 指数输出 (0-50000)
 * - 内置温湿度补偿
 * - 长期稳定性好
 * - I2C 接口
 * - 低功耗
 * 
 * @note SGP40 是 SGP30 的升级版，具有更好的性能和稳定性
 * @note 工作电压 1.8V-3.6V
 */

#ifndef XY_SGP40_H
#define XY_SGP40_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 硬件抽象层接口
 *===========================================================================*/

/**
 * I2C 设备结构体
 */
typedef struct {
    void *handle;
    uint8_t address;
} xy_i2c_dev_t;

/**
 * @brief I2C 写命令 (需要用户实现)
 */
xy_ret_t xy_i2c_write_command(xy_i2c_dev_t *dev, uint16_t command);

/**
 * @brief I2C 写数据 (需要用户实现)
 */
xy_ret_t xy_i2c_write_data(xy_i2c_dev_t *dev, const uint8_t *data, uint16_t len);

/**
 * @brief I2C 读数据 (需要用户实现)
 */
xy_ret_t xy_i2c_read_data(xy_i2c_dev_t *dev, uint8_t *data, uint16_t len);

/**
 * @brief 延时函数 (需要用户实现)
 */
void xy_delay_ms(uint32_t ms);

/*============================================================================
 * SGP40 命令定义
 *===========================================================================*/

#define SGP40_I2C_ADDR          0x59  /* 默认 I2C 地址 */

/* 测量命令 */
#define SGP40_CMD_MEASURE_RAW   0x260F  /* 测量原始信号 */
#define SGP40_CMD_MEASURE_VOC   0x261E  /* 测量 VOC 指数 */

/* 特征化命令 */
#define SGP40_CMD_FEATURE_SET   0x201E  /* 获取特征集 */
#define SGP40_CMD_SERIAL_ID     0x3682  /* 读取序列号 */

/* 自测试命令 */
#define SGP40_CMD_SELF_TEST     0x280E  /* 执行自测试 */

/* 烧写命令 */
#define SGP40_CMD_BURN_IN       0x260D  /* 老化/烧写模式 */

/* 温湿度补偿命令 */
#define SGP40_CMD_CONDITIONING  0x2618  /* 温湿度补偿 */

/* CRC 多项式 */
#define SGP40_CRC8_POLYNOMIAL   0x31
#define SGP40_CRC8_INIT_VALUE   0xFF

/* 测量时间 */
#define SGP40_MEASURE_TIME_MS   30      /* 单次测量时间 */
#define SGP40_WARMUP_TIME_MS    10000   /* 预热时间 (10 秒) */
#define SGP40_BURN_IN_TIME_MS   168     /* 老化时间 (小时) */

/*============================================================================
 * 数据类型定义
 *===========================================================================*/

/**
 * VOC 指数范围
 */
typedef enum {
    XY_SGP40_VOC_EXCELLENT = 0,    /*!< 0-100: 优秀 */
    XY_SGP40_VOC_GOOD      = 1,    /*!< 100-200: 良好 */
    XY_SGP40_VOC_MODERATE  = 2,    /*!< 200-300: 中等 */
    XY_SGP40_VOC_POOR      = 3,    /*!< 300-400: 较差 */
    XY_SGP40_VOC_BAD       = 4     /*!< 400+: 差 */
} xy_sgp40_voc_level_t;

/**
 * 测量结果结构体
 */
typedef struct {
    uint16_t voc_index;            /*!< VOC 指数 (0-50000) */
    uint16_t raw_signal;           /*!< 原始信号值 */
    float temperature;             /*!< 补偿温度 (°C) */
    float humidity;                /*!< 补偿湿度 (%RH) */
    uint32_t timestamp;            /*!< 时间戳 (ms) */
    uint8_t status;                /*!< 状态 */
} xy_sgp40_data_t;

/**
 * 设备配置结构体
 */
typedef struct {
    bool enable_compensation;      /*!< 使能温湿度补偿 */
    float default_temperature;     /*!< 默认补偿温度 (°C) */
    float default_humidity;        /*!< 默认补偿湿度 (%RH) */
    uint8_t i2c_address;           /*!< I2C 地址 */
} xy_sgp40_config_t;

/**
 * SGP40 设备结构体
 */
typedef struct {
    xy_i2c_dev_t *i2c;             /*!< I2C 设备 */
    xy_sgp40_config_t config;      /*!< 配置 */
    bool is_initialized;           /*!< 是否已初始化 */
    
    /* 设备信息 */
    uint16_t feature_set;          /*!< 特征集版本 */
    uint32_t serial_id[3];         /*!< 序列号 */
    
    /* 状态 */
    bool is_warmed_up;             /*!< 是否已完成预热 */
    bool is_burned_in;             /*!< 是否已完成老化 */
    uint32_t uptime_ms;            /*!< 上电时间 (ms) */
    
    /* 最后测量结果 */
    xy_sgp40_data_t last_data;     /*!< 最后测量数据 */
    uint32_t measurement_count;    /*!< 测量次数 */
    
    /* 校准参数 */
    int16_t offset;                /*!< VOC 偏移 */
} xy_sgp40_dev_t;

/*============================================================================
 * API 函数声明
 *===========================================================================*/

/**
 * @brief 初始化 SGP40 设备
 * @param dev SGP40 设备结构体
 * @param i2c I2C 设备指针
 * @param config 配置结构体 (可为 NULL 使用默认配置)
 * @return XY_OK 成功，其他失败
 * 
 * @note 初始化流程:
 * 1. 读取特征集
 * 2. 读取序列号
 * 3. 执行自测试
 * 4. 开始预热
 */
xy_ret_t xy_sgp40_init(xy_sgp40_dev_t *dev, xy_i2c_dev_t *i2c, xy_sgp40_config_t *config);

/**
 * @brief 去初始化 SGP40 设备
 * @param dev SGP40 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_deinit(xy_sgp40_dev_t *dev);

/**
 * @brief 读取特征集
 * @param dev SGP40 设备结构体
 * @param feature_set 特征集指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_read_feature_set(xy_sgp40_dev_t *dev, uint16_t *feature_set);

/**
 * @brief 读取序列号
 * @param dev SGP40 设备结构体
 * @param serial_id 序列号数组 (3 个 uint32_t)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_read_serial_id(xy_sgp40_dev_t *dev, uint32_t serial_id[3]);

/**
 * @brief 执行自测试
 * @param dev SGP40 设备结构体
 * @param passed 自测试结果指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_self_test(xy_sgp40_dev_t *dev, bool *passed);

/**
 * @brief 开始测量 VOC
 * @param dev SGP40 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_start_measurement(xy_sgp40_dev_t *dev);

/**
 * @brief 读取 VOC 测量结果
 * @param dev SGP40 设备结构体
 * @param data 数据指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_read_voc(xy_sgp40_dev_t *dev, xy_sgp40_data_t *data);

/**
 * @brief 执行单次 VOC 测量 (阻塞)
 * @param dev SGP40 设备结构体
 * @param data 数据指针
 * @param timeout_ms 超时时间 (ms)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_measure_voc(xy_sgp40_dev_t *dev, xy_sgp40_data_t *data, uint32_t timeout_ms);

/**
 * @brief 启动连续测量
 * @param dev SGP40 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_start_continuous(xy_sgp40_dev_t *dev);

/**
 * @brief 停止测量
 * @param dev SGP40 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_sgp40_stop(xy_sgp40_dev_t *dev);

/**
 * @brief 设置温湿度补偿
 * @param dev SGP40 设备结构体
 * @param temperature 温度 (°C)
 * @param humidity 湿度 (%RH)
 * @return XY_OK 成功，其他失败
 * 
 * @note 温湿度补偿可以提高 VOC 测量精度
 */
xy_ret_t xy_sgp40_set_compensation(xy_sgp40_dev_t *dev, float temperature, float humidity);

/**
 * @brief 启用老化模式
 * @param dev SGP40 设备结构体
 * @return XY_OK 成功，其他失败
 * 
 * @note 老化模式用于提高传感器长期稳定性
 * @note 建议首次使用时运行 168 小时 (7 天)
 */
xy_ret_t xy_sgp40_enable_burn_in(xy_sgp40_dev_t *dev);

/**
 * @brief 检查预热是否完成
 * @param dev SGP40 设备结构体
 * @return true 预热完成，false 预热中
 */
bool xy_sgp40_is_warmed_up(xy_sgp40_dev_t *dev);

/**
 * @brief 获取 VOC 等级
 * @param voc_index VOC 指数
 * @return VOC 等级
 */
xy_sgp40_voc_level_t xy_sgp40_get_voc_level(uint16_t voc_index);

/**
 * @brief 设置 VOC 偏移
 * @param dev SGP40 设备结构体
 * @param offset 偏移值
 */
void xy_sgp40_set_offset(xy_sgp40_dev_t *dev, int16_t offset);

/**
 * @brief 获取设备状态
 * @param dev SGP40 设备结构体
 * @return true 已初始化，false 未初始化
 */
bool xy_sgp40_is_ready(xy_sgp40_dev_t *dev);

/**
 * @brief 获取最后测量结果
 * @param dev SGP40 设备结构体
 * @return 数据指针
 */
xy_sgp40_data_t *xy_sgp40_get_last_data(xy_sgp40_dev_t *dev);

/**
 * @brief 获取上电时间
 * @param dev SGP40 设备结构体
 * @return 上电时间 (ms)
 */
uint32_t xy_sgp40_get_uptime(xy_sgp40_dev_t *dev);

/*============================================================================
 * CRC8 计算 (内部使用)
 *===========================================================================*/

/**
 * @brief 计算 CRC8 校验
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC8 值
 */
uint8_t xy_sgp40_crc8(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_SGP40_H */
