/**
 * @file xy_fg_bq27z746.h
 * @brief BQ27Z746 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Texas Instruments BQ27Z746
 * - Impedance Track™ 技术
 * - 支持 1 节串联电池
 * - 高精度电量计
 */

#ifndef XY_FG_BQ27Z746_H
#define XY_FG_BQ27Z746_H

#include "xy_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ27Z746 I2C 地址
 */
#define BQ27Z746_ADDR           0x55

/**
 * @brief 注册 BQ27Z746 电量计
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址 (可选，默认 0x55)
 * @return 状态码
 */
int xy_fuel_gauge_bq27z746_register(void *i2c_handle, uint8_t addr);

/**
 * @brief 获取充电状态
 * @param fg 电量计设备
 * @return true=充电中，false=未充电
 */
bool xy_fuel_gauge_bq27z746_is_charging(xy_fuel_gauge_t *fg);

/**
 * @brief 获取充满状态
 * @param fg 电量计设备
 * @return true=充满，false=未充满
 */
bool xy_fuel_gauge_bq27z746_is_full(xy_fuel_gauge_t *fg);

/**
 * @brief 获取告警标志
 * @param fg 电量计设备
 * @return 标志位
 */
uint16_t xy_fuel_gauge_bq27z746_get_flags(xy_fuel_gauge_t *fg);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_BQ27Z746_H */
