/**
 * @file xy_charger.h
 * @brief XinYi Charger Framework - Unified Charger Management
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note 统一的充电器管理框架
 */

#ifndef XY_CHARGER_H
#define XY_CHARGER_H

#include "xy_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Charger Types ==================== */

/**
 * @brief 充电器状态
 */
typedef enum {
    XY_CHARGER_STATE_IDLE = 0,      /**< 空闲 */
    XY_CHARGER_STATE_PRE_CHARGE,    /**< 预充电 */
    XY_CHARGER_STATE_FAST_CHARGE,   /**< 快充 */
    XY_CHARGER_STATE_CONSTANT_VOLT, /**< 恒压充电 */
    XY_CHARGER_STATE_CHARGE_DONE,   /**< 充电完成 */
    XY_CHARGER_STATE_FAULT,         /**< 故障 */
} xy_charger_state_t;

/**
 * @brief 充电器故障类型
 */
typedef enum {
    XY_CHARGER_FAULT_NONE = 0,      /**< 无故障 */
    XY_CHARGER_FAULT_INPUT_OVP,     /**< 输入过压 */
    XY_CHARGER_FAULT_THERMAL,       /**< 过热 */
    XY_CHARGER_FAULT_CHARGE_TIMEOUT,/**< 充电超时 */
    XY_CHARGER_FAULT_BAT_OVP,       /**< 电池过压 */
    XY_CHARGER_FAULT_COLD,          /**< 电池过冷 */
    XY_CHARGER_FAULT_HOT,           /**< 电池过热 */
} xy_charger_fault_t;

/**
 * @brief 充电器配置
 */
typedef struct {
    uint32_t input_current_limit;   /**< 输入电流限制 (mA) */
    uint32_t charge_current;        /**< 充电电流 (mA) */
    uint32_t charge_voltage;        /**< 充电电压 (mV) */
    uint32_t precharge_current;     /**< 预充电电流 (mA) */
    uint32_t termination_current;   /**< 终止电流 (mA) */
    uint8_t recharge_threshold;     /**< 再充电阈值 (mV 低于 VREG) */
    bool auto_recharge;             /**< 自动再充电使能 */
} xy_charger_config_t;

/**
 * @brief 充电器状态信息
 */
typedef struct {
    xy_charger_state_t state;       /**< 充电状态 */
    xy_charger_fault_t fault;       /**< 故障类型 */
    uint32_t input_voltage;         /**< 输入电压 (mV) */
    uint32_t bat_voltage;           /**< 电池电压 (mV) */
    uint32_t charge_current;        /**< 充电电流 (mA) */
    uint32_t input_current;         /**< 输入电流 (mA) */
    uint8_t temperature;            /**< 温度 (%) */
    bool power_good;                /**< 电源良好 */
    bool charging;                  /**< 充电中 */
    bool done;                      /**< 充电完成 */
} xy_charger_status_t;

/**
 * @brief 充电器设备结构
 */
typedef struct {
    xy_device_t base;               /**< 设备基类 */
    const xy_charger_config_t *config; /**< 配置 */
    xy_charger_status_t status;     /**< 状态 */
    
    /* 硬件操作接口 */
    int (*hw_init)(void *hw_data);
    int (*hw_read_status)(void *hw_data, xy_charger_status_t *status);
    int (*hw_set_config)(void *hw_data, const xy_charger_config_t *config);
    int (*hw_enable)(void *hw_data, bool enable);
    int (*hw_read_reg)(void *hw_data, uint8_t reg, uint8_t *value);
    int (*hw_write_reg)(void *hw_data, uint8_t reg, uint8_t value);
    
    void *hw_data;                  /**< 硬件数据 */
} xy_charger_t;

/* ==================== Charger API ==================== */

/**
 * @brief 初始化充电器
 * @param charger 充电器设备句柄
 * @param config 配置参数
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_init(xy_charger_t *charger, const xy_charger_config_t *config);

/**
 * @brief 反初始化充电器
 * @param charger 充电器设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_deinit(xy_charger_t *charger);

/**
 * @brief 启动充电
 * @param charger 充电器设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_start(xy_charger_t *charger);

/**
 * @brief 停止充电
 * @param charger 充电器设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_stop(xy_charger_t *charger);

/**
 * @brief 获取充电状态
 * @param charger 充电器设备句柄
 * @param status 状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_get_status(xy_charger_t *charger, xy_charger_status_t *status);

/**
 * @brief 设置充电电流
 * @param charger 充电器设备句柄
 * @param current_mA 充电电流 (mA)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_set_charge_current(xy_charger_t *charger, uint32_t current_mA);

/**
 * @brief 设置充电电压
 * @param charger 充电器设备句柄
 * @param voltage_mV 充电电压 (mV)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_set_charge_voltage(xy_charger_t *charger, uint32_t voltage_mV);

/**
 * @brief 设置输入电流限制
 * @param charger 充电器设备句柄
 * @param current_mA 输入电流限制 (mA)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_set_input_limit(xy_charger_t *charger, uint32_t current_mA);

/**
 * @brief 读取充电器寄存器
 * @param charger 充电器设备句柄
 * @param reg 寄存器地址
 * @param value 寄存器值输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_read_reg(xy_charger_t *charger, uint8_t reg, uint8_t *value);

/**
 * @brief 写入充电器寄存器
 * @param charger 充电器设备句柄
 * @param reg 寄存器地址
 * @param value 寄存器值
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_charger_write_reg(xy_charger_t *charger, uint8_t reg, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* XY_CHARGER_H */
