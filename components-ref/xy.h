/**
 * @file xy.h
 * @brief XinYi Framework Master Header
 * @version 2.0.0
 * @date 2026-03-05
 * 
 * @mainpage XinYi 嵌入式框架
 * 
 * @section intro 简介
 * XinYi 是一个模块化、生产级的嵌入式 C 框架
 * 
 * @section features 核心特性
 * - 模块化架构
 * - 跨平台支持 (STM32/WCH/HC32)
 * - 多 RTOS 后端
 * - 完善的驱动库
 * - 丰富的中间件
 * 
 * @section quickstart 快速开始
 * @code
 * #include "xy.h"
 * 
 * int main(void) {
 *     xy_os_kernel_init();
 *     xy_log_i("Hello XinYi!\n");
 *     xy_os_kernel_start();
 *     return 0;
 * }
 * @endcode
 */

#ifndef XINYI_H
#define XINYI_H

/* ==================== 版本和配置 ==================== */

#include "xy_version.h"      /* 版本管理 */
#include "xy_config.h"       /* 统一配置 */
#include "xy_features.h"     /* 特性开关 */

/* ==================== 基础类型 ==================== */

#include "xy_typedef.h"      /* 类型定义 */
#include "xy_def.h"          /* 基础定义 */

/* ==================== OSAL (OS 抽象层) ==================== */

#if XY_FEATURE_OSAL
#include "kernel/osal/inc/xy_os_sys.h"
#endif

/* ==================== HAL (硬件抽象层) ==================== */

#if XY_FEATURE_HAL
#include "hal/inc/xy_hal.h"
#endif

/* ==================== 基础库 ==================== */

#if XY_FEATURE_CLIB
#include "clib/xy_clib/xy_common.h"
#include "clib/xy_clib/xy_string.h"
#include "clib/xy_clib/xy_math.h"
#endif

#if XY_FEATURE_CRYPTO
#include "crypto/inc/xy_aes.h"
#include "crypto/inc/xy_crc.h"
#include "crypto/inc/xy_hmac.h"
#endif

/* ==================== 设备驱动 ==================== */

#if XY_FEATURE_SENSOR
#include "sensor/inc/xy_sensor.h"
#endif

#if XY_FEATURE_FUEL_GAUGE
#include "fuel_gauge/inc/xy_fuel_gauge.h"
#endif

/* ==================== 中间件 ==================== */

#if XY_FEATURE_PID
#include "pid/inc/xy_pid.h"
#endif

#if XY_FEATURE_FOTA
#include "fota/inc/xy_fota.h"
#include "fota/inc/xy_fota_secure.h"
#endif

#if XY_FEATURE_FS
#include "dm/inc/xy_fs.h"
#endif

#if XY_FEATURE_JSON
#include "dm/inc/xy_json.h"
#endif

/* ==================== 系统服务 ==================== */

#if XY_FEATURE_LOG
#include "trace/xy_log/inc/xy_log.h"
#endif

#if XY_FEATURE_ASSERT
#include "clib/xy_clib/xy_assert.h"
#endif

#endif /* XINYI_H */
