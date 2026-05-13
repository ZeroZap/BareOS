/**
 * @file xy_mux_log.h
 * @brief MUX Log Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MUX_LOG_H
#define XY_MUX_LOG_H

#include "xy_mux.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 日志级别
 */
typedef enum {
    XY_MUX_LOG_DEBUG = 0,
    XY_MUX_LOG_INFO,
    XY_MUX_LOG_WARN,
    XY_MUX_LOG_ERROR,
} xy_mux_log_level_t;

/**
 * @brief 日志包
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;         /**< 时间戳 */
    xy_mux_log_level_t level;   /**< 日志级别 */
    uint8_t channel;            /**< 通道号 */
    char message[];             /**< 日志消息 */
} xy_mux_log_t;

/**
 * @brief 注册 MUX 日志
 */
int32_t xy_mux_log_register(xy_mux_manager_t *mgr,
                            const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 输出日志
 */
int32_t xy_mux_log(xy_mux_manager_t *mgr, xy_mux_log_level_t level,
                   uint8_t channel, const char *format, ...);

/**
 * @brief 调试日志
 */
#define XY_MUX_LOG_DEBUG(mgr, ch, ...) \
    xy_mux_log(mgr, XY_MUX_LOG_DEBUG, ch, __VA_ARGS__)

/**
 * @brief 信息日志
 */
#define XY_MUX_LOG_INFO(mgr, ch, ...) \
    xy_mux_log(mgr, XY_MUX_LOG_INFO, ch, __VA_ARGS__)

/**
 * @brief 警告日志
 */
#define XY_MUX_LOG_WARN(mgr, ch, ...) \
    xy_mux_log(mgr, XY_MUX_LOG_WARN, ch, __VA_ARGS__)

/**
 * @brief 错误日志
 */
#define XY_MUX_LOG_ERROR(mgr, ch, ...) \
    xy_mux_log(mgr, XY_MUX_LOG_ERROR, ch, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
