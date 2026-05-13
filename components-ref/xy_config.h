/**
 * @file xy_config.h
 * @brief XinYi Framework Unified Configuration
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_CONFIG_H
#define XY_CONFIG_H

/**
 * @page config 配置指南
 * 
 * 配置分为三层:
 * 1. xy_config_auto.h - 自动生成 (Kconfig)
 * 2. xy_config.h - 框架默认配置
 * 3. xy_config_user.h - 用户自定义配置 (可选)
 */

/* ==================== 自动生成的配置 ==================== */

/* 如果存在 Kconfig 生成的配置，优先使用 */
#ifdef XY_HAVE_CONFIG_AUTO_H
#include "xy_config_auto.h"
#endif

/* ==================== 框架默认配置 ==================== */

/* OSAL 配置 */
#ifndef XY_CONFIG_OSAL_ENABLED
#define XY_CONFIG_OSAL_ENABLED       1
#endif

#ifndef XY_CONFIG_OSAL_BACKEND
#define XY_CONFIG_OSAL_BACKEND       "freertos"
#endif

/* HAL 配置 */
#ifndef XY_CONFIG_HAL_ENABLED
#define XY_CONFIG_HAL_ENABLED        1
#endif

#ifndef XY_CONFIG_HAL_MCU
#define XY_CONFIG_HAL_MCU            "stm32"
#endif

/* 日志配置 */
#ifndef XY_CONFIG_LOG_ENABLED
#define XY_CONFIG_LOG_ENABLED        1
#endif

#ifndef XY_CONFIG_LOG_LEVEL
#define XY_CONFIG_LOG_LEVEL          XY_LOG_LEVEL_INFO
#endif

#ifndef XY_CONFIG_LOG_USE_PRINTF
#define XY_CONFIG_LOG_USE_PRINTF     0
#endif

/* 内存配置 */
#ifndef XY_CONFIG_HEAP_SIZE
#define XY_CONFIG_HEAP_SIZE          (16 * 1024)
#endif

#ifndef XY_CONFIG_STACK_SIZE
#define XY_CONFIG_STACK_SIZE         (2 * 1024)
#endif

/* 调试配置 */
#ifndef XY_CONFIG_DEBUG_ENABLED
#define XY_CONFIG_DEBUG_ENABLED      0
#endif

#ifndef XY_CONFIG_ASSERT_ENABLED
#define XY_CONFIG_ASSERT_ENABLED     1
#endif

/* ==================== 组件配置 ==================== */

/* Sensor 组件 */
#ifndef XY_CONFIG_SENSOR_ENABLED
#define XY_CONFIG_SENSOR_ENABLED     1
#endif

#ifndef XY_CONFIG_SENSOR_MAX_DEVICES
#define XY_CONFIG_SENSOR_MAX_DEVICES 8
#endif

/* Actuator 组件 */
#ifndef XY_CONFIG_ACTUATOR_ENABLED
#define XY_CONFIG_ACTUATOR_ENABLED   1
#endif

#ifndef XY_CONFIG_ACTUATOR_MAX_DEVICES
#define XY_CONFIG_ACTUATOR_MAX_DEVICES 16
#endif

/* SMBus / PMBus 组件 */
#ifndef XY_CONFIG_SMBUS_ENABLED
#define XY_CONFIG_SMBUS_ENABLED      1
#endif

#ifndef XY_CONFIG_SMBUS_MAX_DEVICES
#define XY_CONFIG_SMBUS_MAX_DEVICES   8
#endif

/* Fuel Gauge 组件 */
#ifndef XY_CONFIG_FUEL_GAUGE_ENABLED
#define XY_CONFIG_FUEL_GAUGE_ENABLED 1
#endif

#ifndef XY_CONFIG_FUEL_GAUGE_MAX_DEVICES
#define XY_CONFIG_FUEL_GAUGE_MAX_DEVICES 4
#endif

/* FOTA 组件 */
#ifndef XY_CONFIG_FOTA_ENABLED
#define XY_CONFIG_FOTA_ENABLED       1
#endif

#ifndef XY_CONFIG_FOTA_SLOT_SIZE
#define XY_CONFIG_FOTA_SLOT_SIZE     (256 * 1024)
#endif

#ifndef XY_CONFIG_FOTA_SECURITY_ENABLED
#define XY_CONFIG_FOTA_SECURITY_ENABLED 1
#endif

/* PID 组件 */
#ifndef XY_CONFIG_PID_ENABLED
#define XY_CONFIG_PID_ENABLED        1
#endif

#ifndef XY_CONFIG_PID_MAX_INSTANCES
#define XY_CONFIG_PID_MAX_INSTANCES  4
#endif

/* FlashDB 组件 */
#ifndef XY_CONFIG_FLASHDB_ENABLED
#define XY_CONFIG_FLASHDB_ENABLED    1
#endif

#ifndef XY_CONFIG_FLASHDB_KVDB_ENABLED
#define XY_CONFIG_FLASHDB_KVDB_ENABLED  1
#endif

#ifndef XY_CONFIG_FLASHDB_TSDB_ENABLED
#define XY_CONFIG_FLASHDB_TSDB_ENABLED 1
#endif

/* ==================== 用户自定义配置 ==================== */

/* 如果存在用户配置文件，包含它 */
#ifdef XY_CONFIG_USER_H
#include XY_CONFIG_USER_H
#endif

#endif /* XY_CONFIG_H */
