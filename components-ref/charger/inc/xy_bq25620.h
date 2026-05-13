/**
 * @file xy_bq25620.h
 * @brief TI BQ25620 Standalone I2C 1-Cell Li-Ion Battery Charger Driver
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note BQ25620 是一款独立的 I2C 控制的 1 节锂离子电池充电器
 * 
 * 主要特性:
 * - 输入电压范围：3.5V - 13.5V
 * - 充电电流：最高 2A
 * - 充电电压：4.2V (默认)
 * - I2C 接口配置
 * - 热调节和过温保护
 * - 输入过压保护
 * - 电池温度监测 (TS 引脚)
 */

#ifndef XY_BQ25620_H
#define XY_BQ25620_H

#include "xy_charger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== BQ25620 Register Definitions ==================== */

/**
 * @brief BQ25620 寄存器地址
 */
typedef enum {
    BQ25620_REG_CHG_STAT_0    = 0x00,  /**< 充电状态寄存器 0 */
    BQ25620_REG_CHG_STAT_1    = 0x01,  /**< 充电状态寄存器 1 */
    BQ25620_REG_CHG_CTRL_0    = 0x02,  /**< 充电控制寄存器 0 */
    BQ25620_REG_CHG_CTRL_1    = 0x03,  /**< 充电控制寄存器 1 */
    BQ25620_REG_CHG_CTRL_2    = 0x04,  /**< 充电控制寄存器 2 */
    BQ25620_REG_CHG_CTRL_3    = 0x05,  /**< 充电控制寄存器 3 */
    BQ25620_REG_CHG_CTRL_4    = 0x06,  /**< 充电控制寄存器 4 */
    BQ25620_REG_CHG_CTRL_5    = 0x07,  /**< 充电控制寄存器 5 */
    BQ25620_REG_CHG_CTRL_6    = 0x08,  /**< 充电控制寄存器 6 */
    BQ25620_REG_CHG_CTRL_7    = 0x09,  /**< 充电控制寄存器 7 */
    BQ25620_REG_ADC_CTRL      = 0x0A,  /**< ADC 控制寄存器 */
    BQ25620_REG_ADC_STAT_0    = 0x0B,  /**< ADC 状态寄存器 0 */
    BQ25620_REG_ADC_STAT_1    = 0x0C,  /**< ADC 状态寄存器 1 */
    BQ25620_REG_ADC_STAT_2    = 0x0D,  /**< ADC 状态寄存器 2 */
    BQ25620_REG_ADC_STAT_3    = 0x0E,  /**< ADC 状态寄存器 3 */
    BQ25620_REG_ADC_STAT_4    = 0x0F,  /**< ADC 状态寄存器 4 */
    BQ25620_REG_MANUAL_MODE_0 = 0x10,  /**< 手动模式寄存器 0 */
    BQ25620_REG_MANUAL_MODE_1 = 0x11,  /**< 手动模式寄存器 1 */
    BQ25620_REG_PULSE_CHG_0   = 0x12,  /**< 脉冲充电寄存器 0 */
    BQ25620_REG_PULSE_CHG_1   = 0x13,  /**< 脉冲充电寄存器 1 */
    BQ25620_REG_SHIPMENT_MODE = 0x14,  /**< 运输模式寄存器 */
    BQ25620_REG_DEVICE_ID     = 0x15,  /**< 设备 ID 寄存器 */
} bq25620_reg_t;

/* ==================== Register Bit Definitions ==================== */

/* CHG_STAT_0 (0x00) */
#define BQ25620_STAT_CHG_MASK       (0x07 << 4)  /**< 充电状态掩码 */
#define BQ25620_STAT_CHG_IDLE       (0x00 << 4)  /**< 空闲 */
#define BQ25620_STAT_CHG_PRECHG     (0x01 << 4)  /**< 预充电 */
#define BQ25620_STAT_CHG_FAST       (0x02 << 4)  /**< 快充 */
#define BQ25620_STAT_CHG_DONE       (0x03 << 4)  /**< 充电完成 */
#define BQ25620_STAT_PG_MASK        (0x01 << 2)  /**< Power Good 状态 */
#define BQ25620_STAT_PG             (0x01 << 2)  /**< Power Good */
#define BQ25620_STAT_THERM_MASK     (0x03 << 0)  /**< 温度状态掩码 */

/* CHG_STAT_1 (0x01) */
#define BQ25620_FAULT_MASK          (0x07 << 4)  /**< 故障掩码 */
#define BQ25620_FAULT_NORMAL        (0x00 << 4)  /**< 正常 */
#define BQ25620_FAULT_INPUT_OVP     (0x01 << 4)  /**< 输入过压 */
#define BQ25620_FAULT_THERMAL       (0x02 << 4)  /**< 过热 */
#define BQ25620_FAULT_CHG_TIMEOUT   (0x03 << 4)  /**< 充电超时 */
#define BQ25620_FAULT_BAT_OVP       (0x04 << 4)  /**< 电池过压 */

/* CHG_CTRL_0 (0x02) */
#define BQ25620_EN_CHG              (0x01 << 7)  /**< 充电使能 */
#define BQ25620_EN_TERM             (0x01 << 6)  /**< 终止检测使能 */
#define BQ25620_AUTO_RECHG          (0x01 << 5)  /**< 自动再充电使能 */
#define BQ25620_WD_RST_MASK         (0x03 << 3)  /**< 看门狗复位掩码 */
#define BQ25620_ICHG_SCALE          (0x01 << 2)  /**< 充电电流比例 */

/* CHG_CTRL_1 (0x03) - 充电电流设置 */
#define BQ25620_ICHG_MASK           (0x7F << 0)  /**< 充电电流掩码 */
#define BQ25620_ICHG_STEP_mA        64           /**< 充电电流步长 (mA) */
#define BQ25620_ICHG_MIN_mA         64           /**< 最小充电电流 (mA) */
#define BQ25620_ICHG_MAX_mA         5056         /**< 最大充电电流 (mA) */

/* CHG_CTRL_2 (0x04) - 预充电和终止电流 */
#define BQ25620_ITERM_MASK          (0x0F << 4)  /**< 终止电流掩码 */
#define BQ25620_IPRECHG_MASK        (0x0F << 0)  /**< 预充电电流掩码 */

/* CHG_CTRL_3 (0x05) - 充电电压设置 */
#define BQ25620_VREG_MASK           (0x7F << 0)  /**< 充电电压掩码 */
#define BQ25620_VREG_STEP_mV        10           /**< 充电电压步长 (mV) */
#define BQ25620_VREG_MIN_mV         3500         /**< 最小充电电压 (mV) */
#define BQ25620_VREG_MAX_mV         4470         /**< 最大充电电压 (mV) */

/* CHG_CTRL_4 (0x06) - 输入限制 */
#define BQ25620_EN_ILIM             (0x01 << 7)  /**< 输入电流限制使能 */
#define BQ25620_ILIM_MASK           (0x3F << 0)  /**< 输入电流限制掩码 */
#define BQ25620_ILIM_STEP_mA        100          /**< 输入电流步长 (mA) */
#define BQ25620_ILIM_MIN_mA         100          /**< 最小输入电流 (mA) */
#define BQ25620_ILIM_MAX_mA         6300         /**< 最大输入电流 (mA) */

/* CHG_CTRL_5 (0x07) - 充电终止和再充电 */
#define BQ25620_VRECHG_MASK         (0x03 << 6)  /**< 再充电阈值掩码 */
#define BQ25620_TMR_MASK            (0x03 << 4)  /**< 充电超时掩码 */
#define BQ25620_EN_HOT              (0x01 << 2)  /**< 热充电使能 */
#define BQ25620_EN_COLD             (0x01 << 1)  /**< 冷充电使能 */

/* CHG_CTRL_7 (0x09) -  Miscellaneous */
#define BQ25620_FORCE_DPDM          (0x01 << 7)  /**< 强制 DPDM 检测 */
#define BQ25620_EN_OC               (0x01 << 4)  /**< 过流保护使能 */

/* DEVICE_ID (0x15) */
#define BQ25620_PART_NUMBER_MASK    (0x3F << 2)  /**< 型号掩码 */
#define BQ25620_PART_NUMBER         (0x0A << 2)  /**< BQ25620 型号值 */

/* ==================== BQ25620 Device Structure ==================== */

/**
 * @brief BQ25620 设备结构
 */
typedef struct {
    xy_charger_t base;              /**< 充电器基类 */
    void *i2c_handle;               /**< I2C 句柄 */
    uint8_t i2c_addr;               /**< I2C 地址 (默认 0x6A) */
    bool initialized;               /**< 初始化标志 */
} xy_bq25620_t;

/* ==================== BQ25620 API ==================== */

/**
 * @brief 初始化 BQ25620
 * @param dev BQ25620 设备句柄
 * @param i2c_handle I2C 句柄
 * @param i2c_addr I2C 地址 (0x6A)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr);

/**
 * @brief 反初始化 BQ25620
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_deinit(xy_bq25620_t *dev);

/**
 * @brief 读取 BQ25620 寄存器
 * @param dev BQ25620 设备句柄
 * @param reg 寄存器地址
 * @param value 寄存器值输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value);

/**
 * @brief 写入 BQ25620 寄存器
 * @param dev BQ25620 设备句柄
 * @param reg 寄存器地址
 * @param value 寄存器值
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_write_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t value);

/**
 * @brief 读取 BQ25620 设备 ID
 * @param dev BQ25620 设备句柄
 * @param id 设备 ID 输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id);

/**
 * @brief 获取充电状态
 * @param dev BQ25620 设备句柄
 * @param status 状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_status_t *status);

/**
 * @brief 设置充电电流
 * @param dev BQ25620 设备句柄
 * @param current_mA 充电电流 (mA)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA);

/**
 * @brief 设置充电电压
 * @param dev BQ25620 设备句柄
 * @param voltage_mV 充电电压 (mV)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV);

/**
 * @brief 设置输入电流限制
 * @param dev BQ25620 设备句柄
 * @param current_mA 输入电流限制 (mA)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA);

/**
 * @brief 启动充电
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_start_charge(xy_bq25620_t *dev);

/**
 * @brief 停止充电
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_stop_charge(xy_bq25620_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_BQ25620_H */
