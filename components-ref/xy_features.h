/**
 * @file xy_features.h
 * @brief XinYi Framework Feature Flags
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_FEATURES_H
#define XY_FEATURES_H

/**
 * @page features 特性开关
 * 
 * 特性开关用于启用/禁用框架功能
 * 可以在 xy_config.h 中配置
 */

/* ==================== 核心特性 ==================== */

/* OS 抽象层 */
#ifndef XY_FEATURE_OSAL
#define XY_FEATURE_OSAL            1
#endif

/* 硬件抽象层 */
#ifndef XY_FEATURE_HAL
#define XY_FEATURE_HAL             1
#endif

/* 基础库 */
#ifndef XY_FEATURE_CLIB
#define XY_FEATURE_CLIB            1
#endif

/* ==================== 驱动特性 ==================== */

/* Sensor 传感器 */
#ifndef XY_FEATURE_SENSOR
#define XY_FEATURE_SENSOR          1
#endif

/* Fuel Gauge 电量计 */
#ifndef XY_FEATURE_FUEL_GAUGE
#define XY_FEATURE_FUEL_GAUGE      1
#endif

/* Display 显示 */
#ifndef XY_FEATURE_DISPLAY
#define XY_FEATURE_DISPLAY         0
#endif

/* ==================== 中间件特性 ==================== */

/* FOTA 固件升级 */
#ifndef XY_FEATURE_FOTA
#define XY_FEATURE_FOTA            1
#endif

/* PID 控制 */
#ifndef XY_FEATURE_PID
#define XY_FEATURE_PID             1
#endif

/* 文件系统 */
#ifndef XY_FEATURE_FS
#define XY_FEATURE_FS              1
#endif

/* JSON 解析 */
#ifndef XY_FEATURE_JSON
#define XY_FEATURE_JSON            1
#endif

/* ==================== 安全特性 ==================== */

/* 加密 */
#ifndef XY_FEATURE_CRYPTO
#define XY_FEATURE_CRYPTO          1
#endif

/* 安全认证 */
#ifndef XY_FEATURE_SECURITY
#define XY_FEATURE_SECURITY        1
#endif

/* ==================== 调试特性 ==================== */

/* 日志 */
#ifndef XY_FEATURE_LOG
#define XY_FEATURE_LOG             1
#endif

/* 断言 */
#ifndef XY_FEATURE_ASSERT
#define XY_FEATURE_ASSERT          1
#endif

/* 调试输出 */
#ifndef XY_FEATURE_DEBUG
#define XY_FEATURE_DEBUG           0
#endif

/* ==================== 特性检查宏 ==================== */

/**
 * @brief 检查特性是否启用
 */
#define XY_FEATURE_IS_ENABLED(feature) (XY_FEATURE_##feature)

/**
 * @brief 条件编译辅助宏
 */
#define XY_IF_ENABLED(feature, code) \
    XY_FEATURE_##feature ? (code) : (void)0

#endif /* XY_FEATURES_H */
