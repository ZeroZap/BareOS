/**
 * @file xy_fg_bq40z50.h
 * @brief BQ40Z50 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Texas Instruments BQ40Z50
 * - 支持 2-4 节串联电池
 * - Impedance Track™ 技术
 * - 集成保护功能
 */

#ifndef XY_FG_BQ40Z50_H
#define XY_FG_BQ40Z50_H

#include "xy_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ40Z50 I2C 地址
 */
#define BQ40Z50_ADDR            0x0B

/**
 * @brief 注册 BQ40Z50 电量计
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址 (可选，默认 0x0B)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_register(void *i2c_handle, uint8_t addr);

/**
 * @brief 获取电池组电压
 * @param fg 电量计设备
 * @param voltage_mv 电压 (mV)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_get_battery_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv);

/**
 * @brief 获取单体电压
 * @param fg 电量计设备
 * @param cell_index 单体索引 (1-4)
 * @param voltage_mv 单体电压 (mV)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_get_cell_voltage(xy_fuel_gauge_t *fg, 
                                           uint8_t cell_index,
                                           uint16_t *voltage_mv);

/**
 * @brief 获取平衡状态
 * @param fg 电量计设备
 * @return 平衡状态位掩码
 */
uint8_t xy_fuel_gauge_bq40z50_get_balance_status(xy_fuel_gauge_t *fg);

/**
 * @brief 获取保护状态
 * @param fg 电量计设备
 * @return 保护状态位掩码
 */
uint32_t xy_fuel_gauge_bq40z50_get_protection_status(xy_fuel_gauge_t *fg);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_BQ40Z50_H */
