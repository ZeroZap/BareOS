/**
 * @file xy_bno055.h
 * @brief Bosch BNO055 9-Axis Smart IMU Sensor Driver
 * @version 1.0.0
 * @date 2026-03-18
 * 
 * @note BNO055 是内置传感器融合算法的 9 轴智能 IMU
 * 
 * 主要特性:
 * - 3 轴加速度计 (±2g/±4g/±8g/±16g)
 * - 3 轴陀螺仪 (±125/±250/±500/±1000/±2000°/s)
 * - 3 轴磁力计
 * - 内置 ARM Cortex-M0 处理器
 * - 传感器融合算法 (姿态/四元数/欧拉角)
 * - 工作模式：NDOF/IMU/COMPASS/M4G 等
 * - I2C/UART 接口
 * 
 * 使用示例:
 * @code
 * xy_bno055_t bno055;
 * 
 * // 初始化
 * xy_bno055_init(&bno055, i2c_handle, 0x28);
 * 
 * // 设置 NDOF 模式
 * xy_bno055_set_mode(&bno055, BNO055_MODE_NDOF);
 * 
 * // 读取四元数
 * bno055_quaternion_t quat;
 * xy_bno055_get_quaternion(&bno055, &quat);
 * 
 * // 读取欧拉角
 * bno055_euler_t euler;
 * xy_bno055_get_euler(&bno055, &euler);
 * @endcode
 */

#ifndef XY_BNO055_H
#define XY_BNO055_H

#include "xy_sensor.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== BNO055 Register Definitions ==================== */

/**
 * @brief BNO055 寄存器地址 (Page 0)
 */
typedef enum {
    /* 页面 ID */
    BNO055_REG_PAGE_ID       = 0x07,
    
    /* 信息寄存器 */
    BNO055_REG_CHIP_ID       = 0x00,  /**< 芯片 ID (0xA0) */
    BNO055_REG_ACC_ID        = 0x01,  /**< 加速度计 ID */
    BNO055_REG_MAG_ID        = 0x02,  /**< 磁力计 ID */
    BNO055_REG_GYR_ID        = 0x03,  /**< 陀螺仪 ID */
    BNO055_REG_SW_REV_ID_LSB = 0x04,  /**< 固件版本 LSB */
    BNO055_REG_SW_REV_ID_MSB = 0x05,  /**< 固件版本 MSB */
    BNO055_REG_BL_REV_ID     = 0x06,  /**< Bootloader 版本 */
    
    /* 加速度计数据 */
    BNO055_REG_ACC_DATA_X_LSB = 0x08,
    BNO055_REG_ACC_DATA_X_MSB = 0x09,
    BNO055_REG_ACC_DATA_Y_LSB = 0x0A,
    BNO055_REG_ACC_DATA_Y_MSB = 0x0B,
    BNO055_REG_ACC_DATA_Z_LSB = 0x0C,
    BNO055_REG_ACC_DATA_Z_MSB = 0x0D,
    
    /* 磁力计数据 */
    BNO055_REG_MAG_DATA_X_LSB = 0x0E,
    BNO055_REG_MAG_DATA_X_MSB = 0x0F,
    BNO055_REG_MAG_DATA_Y_LSB = 0x10,
    BNO055_REG_MAG_DATA_Y_MSB = 0x11,
    BNO055_REG_MAG_DATA_Z_LSB = 0x12,
    BNO055_REG_MAG_DATA_Z_MSB = 0x13,
    
    /* 陀螺仪数据 */
    BNO055_REG_GYR_DATA_X_LSB = 0x14,
    BNO055_REG_GYR_DATA_X_MSB = 0x15,
    BNO055_REG_GYR_DATA_Y_LSB = 0x16,
    BNO055_REG_GYR_DATA_Y_MSB = 0x17,
    BNO055_REG_GYR_DATA_Z_LSB = 0x18,
    BNO055_REG_GYR_DATA_Z_MSB = 0x19,
    
    /* 欧拉角数据 */
    BNO055_REG_EUL_HEADING_LSB = 0x1A,
    BNO055_REG_EUL_HEADING_MSB = 0x1B,
    BNO055_REG_EUL_ROLL_LSB    = 0x1C,
    BNO055_REG_EUL_ROLL_MSB    = 0x1D,
    BNO055_REG_EUL_PITCH_LSB   = 0x1E,
    BNO055_REG_EUL_PITCH_MSB   = 0x1F,
    
    /* 四元数数据 */
    BNO055_REG_QUA_DATA_W_LSB  = 0x20,
    BNO055_REG_QUA_DATA_W_MSB  = 0x21,
    BNO055_REG_QUA_DATA_X_LSB  = 0x22,
    BNO055_REG_QUA_DATA_X_MSB  = 0x23,
    BNO055_REG_QUA_DATA_Y_LSB  = 0x24,
    BNO055_REG_QUA_DATA_Y_MSB  = 0x25,
    BNO055_REG_QUA_DATA_Z_LSB  = 0x26,
    BNO055_REG_QUA_DATA_Z_MSB  = 0x27,
    
    /* 线性加速度数据 */
    BNO055_REG_LIA_DATA_X_LSB  = 0x28,
    BNO055_REG_LIA_DATA_X_MSB  = 0x29,
    BNO055_REG_LIA_DATA_Y_LSB  = 0x2A,
    BNO055_REG_LIA_DATA_Y_MSB  = 0x2B,
    BNO055_REG_LIA_DATA_Z_LSB  = 0x2C,
    BNO055_REG_LIA_DATA_Z_MSB  = 0x2D,
    
    /* 重力向量数据 */
    BNO055_REG_GRV_DATA_X_LSB  = 0x2E,
    BNO055_REG_GRV_DATA_X_MSB  = 0x2F,
    BNO055_REG_GRV_DATA_Y_LSB  = 0x30,
    BNO055_REG_GRV_DATA_Y_MSB  = 0x31,
    BNO055_REG_GRV_DATA_Z_LSB  = 0x32,
    BNO055_REG_GRV_DATA_Z_MSB  = 0x33,
    
    /* 温度数据 */
    BNO055_REG_TEMP            = 0x34,
    
    /* 校准状态 */
    BNO055_REG_CALIB_STAT      = 0x35,
    BNO055_REG_ST_RESULT       = 0x36,
    
    /* 中断状态 */
    BNO055_REG_INT_STA         = 0x37,
    BNO055_REG_SYS_CLK_STATUS  = 0x38,
    BNO055_REG_SYS_STATUS      = 0x39,
    BNO055_REG_SYS_ERR         = 0x3A,
    BNO055_REG_UNIT_SEL        = 0x3B,
    BNO055_REG_OPR_MODE        = 0x3D,
    BNO055_REG_PWR_MODE        = 0x3E,
    
    /* 系统配置 */
    BNO055_REG_SYS_TRIGGER     = 0x3F,
    BNO055_REG_TEMP_SOURCE     = 0x40,
    
    /* 轴向重映射 */
    BNO055_REG_AXIS_MAP_CONFIG = 0x41,
    BNO055_REG_AXIS_MAP_SIGN   = 0x42,
    
    /* 校准数据 */
    BNO055_REG_ACC_OFFSET_X_LSB = 0x55,
    BNO055_REG_ACC_OFFSET_Y_LSB = 0x57,
    BNO055_REG_ACC_OFFSET_Z_LSB = 0x59,
    BNO055_REG_MAG_OFFSET_X_LSB = 0x5B,
    BNO055_REG_MAG_OFFSET_Y_LSB = 0x5D,
    BNO055_REG_MAG_OFFSET_Z_LSB = 0x5F,
    BNO055_REG_GYR_OFFSET_X_LSB = 0x61,
    BNO055_REG_GYR_OFFSET_Y_LSB = 0x63,
    BNO055_REG_GYR_OFFSET_Z_LSB = 0x65,
    BNO055_REG_ACC_RADIUS_LSB   = 0x67,
    BNO055_REG_MAG_RADIUS_LSB   = 0x69,
} bno055_reg_t;

/* ==================== 常量定义 ==================== */

/** BNO055 芯片 ID */
#define BNO055_CHIP_ID             (0xA0)

/** 固件版本 (最低要求) */
#define BNO055_SW_REV_MIN          (0x0319)

/* ==================== 操作模式 ==================== */

/**
 * @brief BNO055 操作模式
 */
typedef enum {
    BNO055_MODE_CONFIG      = 0x00,  /**< 配置模式 */
    BNO055_MODE_ACCONLY     = 0x01,  /**< 仅加速度计 */
    BNO055_MODE_MAGONLY     = 0x02,  /**< 仅磁力计 */
    BNO055_MODE_GYRONLY     = 0x03,  /**< 仅陀螺仪 */
    BNO055_MODE_ACCMAG      = 0x04,  /**< 加速度计 + 磁力计 */
    BNO055_MODE_ACCGYRO     = 0x05,  /**< 加速度计 + 陀螺仪 */
    BNO055_MODE_MAGGYRO     = 0x06,  /**< 磁力计 + 陀螺仪 */
    BNO055_MODE_AMG         = 0x07,  /**< 加速度计 + 磁力计 + 陀螺仪 */
    BNO055_MODE_IMU         = 0x08,  /**< IMU (相对方向) */
    BNO055_MODE_COMPASS     = 0x09,  /**< 罗盘 */
    BNO055_MODE_M4G         = 0x0A,  /**< M4G (无磁力计 4 轴地磁) */
    BNO055_MODE_NDOF_FMC_OFF = 0x0B, /**< NDOF 无磁力计校准 */
    BNO055_MODE_NDOF        = 0x0C,  /**< NDOF (完全 9 轴融合) */
} bno055_mode_t;

/* ==================== 电源模式 ==================== */

/**
 * @brief BNO055 电源模式
 */
typedef enum {
    BNO055_PWR_NORMAL   = 0x00,  /**< 正常模式 */
    BNO055_PWR_LOWPOWER = 0x01,  /**< 低功耗模式 */
    BNO055_PWR_SUSPEND  = 0x02,  /**< 挂起模式 */
} bno055_pwr_t;

/* ==================== 单位选择 ==================== */

/** 角度单位：度 */
#define BNO055_UNIT_DEG      (0x00 << 0)
/** 角度单位：弧度 */
#define BNO055_UNIT_RAD      (0x01 << 0)
/** 温度单位：摄氏度 */
#define BNO055_UNIT_CELSIUS  (0x00 << 1)
/** 温度单位：华氏度 */
#define BNO055_UNIT_FAHRENHEIT (0x01 << 1)
/** 旋转数据：欧拉角 */
#define BNO055_UNIT_EULER    (0x00 << 2)
/** 旋转数据：四元数 */
#define BNO055_UNIT_QUAT     (0x01 << 2)
/** 加速度单位：m/s² */
#define BNO055_UNIT_MS2      (0x00 << 3)
/** 加速度单位：g */
#define BNO055_UNIT_G        (0x01 << 3)
/** 磁场单位：μT */
#define BNO055_UNIT_UT       (0x00 << 4)
/** 磁场单位：mG */
#define BNO055_UNIT_MG       (0x01 << 4)

/* ==================== 数据结构 ==================== */

/**
 * @brief BNO055 校准状态
 */
typedef struct {
    uint8_t sys;      /**< 系统校准 (0-3) */
    uint8_t gyro;     /**< 陀螺仪校准 (0-3) */
    uint8_t acc;      /**< 加速度计校准 (0-3) */
    uint8_t mag;      /**< 磁力计校准 (0-3) */
} bno055_calib_t;

/**
 * @brief BNO055 四元数
 */
typedef struct {
    float w;          /**< W 分量 */
    float x;          /**< X 分量 */
    float y;          /**< Y 分量 */
    float z;          /**< Z 分量 */
} bno055_quaternion_t;

/**
 * @brief BNO055 欧拉角
 */
typedef struct {
    float heading;    /**< 航向角 (0-360°) */
    float roll;       /**< 横滚角 (-180°~+180°) */
    float pitch;      /**< 俯仰角 (-90°~+90°) */
} bno055_euler_t;

/**
 * @brief BNO055 原始传感器数据
 */
typedef struct {
    int16_t acc_x;    /**< 加速度 X */
    int16_t acc_y;    /**< 加速度 Y */
    int16_t acc_z;    /**< 加速度 Z */
    int16_t mag_x;    /**< 磁场 X */
    int16_t mag_y;    /**< 磁场 Y */
    int16_t mag_z;    /**< 磁场 Z */
    int16_t gyr_x;    /**< 陀螺仪 X */
    int16_t gyr_y;    /**< 陀螺仪 Y */
    int16_t gyr_z;    /**< 陀螺仪 Z */
    int8_t temp;      /**< 温度 */
} bno055_raw_data_t;

/**
 * @brief BNO055 融合数据
 */
typedef struct {
    bno055_euler_t euler;         /**< 欧拉角 */
    bno055_quaternion_t quat;     /**< 四元数 */
    float acc_x;                  /**< 加速度 X (m/s²) */
    float acc_y;                  /**< 加速度 Y (m/s²) */
    float acc_z;                  /**< 加速度 Z (m/s²) */
    float linear_acc_x;           /**< 线性加速度 X */
    float linear_acc_y;           /**< 线性加速度 Y */
    float linear_acc_z;           /**< 线性加速度 Z */
    float gravity_x;              /**< 重力向量 X */
    float gravity_y;              /**< 重力向量 Y */
    float gravity_z;              /**< 重力向量 Z */
    float mag_x;                  /**< 磁场 X (μT) */
    float mag_y;                  /**< 磁场 Y (μT) */
    float mag_z;                  /**< 磁场 Z (μT) */
    float gyr_x;                  /**< 陀螺仪 X (rad/s) */
    float gyr_y;                  /**< 陀螺仪 Y (rad/s) */
    float gyr_z;                  /**< 陀螺仪 Z (rad/s) */
    float temperature;            /**< 温度 (°C) */
    bno055_calib_t calib;         /**< 校准状态 */
} bno055_data_t;

/**
 * @brief BNO055 设备结构
 */
typedef struct {
    xy_sensor_t base;         /**< 传感器基类 */
    void *bus_handle;         /**< 总线句柄 (I2C/UART) */
    uint8_t bus_addr;         /**< 总线地址 (I2C: 0x28 或 0x29) */
    bool is_uart;             /**< UART 模式标志 */
    bno055_mode_t mode;       /**< 当前模式 */
    uint16_t sw_version;      /**< 固件版本 */
    uint8_t unit_flags;       /**< 单位选择标志 */
    bool initialized;         /**< 初始化标志 */
} xy_bno055_t;

/* ==================== BNO055 API ==================== */

/**
 * @brief 初始化 BNO055
 * @param dev BNO055 设备句柄
 * @param bus_handle 总线句柄 (I2C 或 UART)
 * @param bus_addr I2C 地址 (0x28 或 0x29)
 * @param is_uart UART 模式标志
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_init(xy_bno055_t *dev, void *bus_handle, uint8_t bus_addr, bool is_uart);

/**
 * @brief 反初始化 BNO055
 * @param dev BNO055 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_deinit(xy_bno055_t *dev);

/**
 * @brief 读取 BNO055 寄存器
 * @param dev BNO055 设备句柄
 * @param reg 寄存器地址
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_read_regs(xy_bno055_t *dev, uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @brief 写入 BNO055 寄存器
 * @param dev BNO055 设备句柄
 * @param reg 寄存器地址
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_write_regs(xy_bno055_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len);

/**
 * @brief 读取芯片 ID
 * @param dev BNO055 设备句柄
 * @param chip_id 芯片 ID 输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_chip_id(xy_bno055_t *dev, uint8_t *chip_id);

/**
 * @brief 读取固件版本
 * @param dev BNO055 设备句柄
 * @param version 固件版本输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_sw_version(xy_bno055_t *dev, uint16_t *version);

/**
 * @brief 设置操作模式
 * @param dev BNO055 设备句柄
 * @param mode 操作模式
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_set_mode(xy_bno055_t *dev, bno055_mode_t mode);

/**
 * @brief 获取当前模式
 * @param dev BNO055 设备句柄
 * @param mode 模式输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_mode(xy_bno055_t *dev, bno055_mode_t *mode);

/**
 * @brief 设置电源模式
 * @param dev BNO055 设备句柄
 * @param pwr 电源模式
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_set_power_mode(xy_bno055_t *dev, bno055_pwr_t pwr);

/**
 * @brief 设置数据单位
 * @param dev BNO055 设备句柄
 * @param unit_flags 单位标志
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_set_units(xy_bno055_t *dev, uint8_t unit_flags);

/**
 * @brief 复位 BNO055
 * @param dev BNO055 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_reset(xy_bno055_t *dev);

/**
 * @brief 获取校准状态
 * @param dev BNO055 设备句柄
 * @param calib 校准状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_calib_status(xy_bno055_t *dev, bno055_calib_t *calib);

/**
 * @brief 读取四元数
 * @param dev BNO055 设备句柄
 * @param quat 四元数输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_quaternion(xy_bno055_t *dev, bno055_quaternion_t *quat);

/**
 * @brief 读取欧拉角
 * @param dev BNO055 设备句柄
 * @param euler 欧拉角输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_euler(xy_bno055_t *dev, bno055_euler_t *euler);

/**
 * @brief 读取所有融合数据
 * @param dev BNO055 设备句柄
 * @param data 数据输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_data(xy_bno055_t *dev, bno055_data_t *data);

/**
 * @brief 读取原始传感器数据
 * @param dev BNO055 设备句柄
 * @param raw 原始数据输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_raw_data(xy_bno055_t *dev, bno055_raw_data_t *raw);

/**
 * @brief 获取系统状态
 * @param dev BNO055 设备句柄
 * @param status 状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_sys_status(xy_bno055_t *dev, uint8_t *status);

/**
 * @brief 获取自测试结果
 * @param dev BNO055 设备句柄
 * @param result 结果输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_get_self_test(xy_bno055_t *dev, uint8_t *result);

/**
 * @brief 配置轴向重映射
 * @param dev BNO055 设备句柄
 * @param config 重映射配置
 * @param sign 符号配置
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_set_axis_remap(xy_bno055_t *dev, uint8_t config, uint8_t sign);

/**
 * @brief 进入睡眠模式
 * @param dev BNO055 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_sleep(xy_bno055_t *dev);

/**
 * @brief 退出睡眠模式
 * @param dev BNO055 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bno055_wakeup(xy_bno055_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_BNO055_H */
