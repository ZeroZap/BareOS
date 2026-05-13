/**
 * @file xy_fuel_gauge_security.h
 * @brief Fuel Gauge Security Authentication Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * 电量计安全认证功能:
 * - SHA256 认证
 * - 加密通信
 * - 防篡改保护
 */

#ifndef XY_FUEL_GAUGE_SECURITY_H
#define XY_FUEL_GAUGE_SECURITY_H

#include "xy_fuel_gauge.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 安全认证类型 ==================== */

/**
 * @brief 安全认证类型
 */
typedef enum {
    XY_FG_SECURITY_NONE = 0,      /* 无认证 */
    XY_FG_SECURITY_SHA256,        /* SHA256 认证 */
    XY_FG_SECURITY_AES128,        /* AES128 加密 */
    XY_FG_SECURITY_AES256,        /* AES256 加密 */
} xy_fg_security_type_t;

/**
 * @brief 认证结果
 */
typedef enum {
    XY_FG_AUTH_OK = 0,            /* 认证成功 */
    XY_FG_AUTH_FAIL,              /* 认证失败 */
    XY_FG_AUTH_TIMEOUT,           /* 认证超时 */
    XY_FG_AUTH_NOT_SUPPORTED,     /* 不支持认证 */
} xy_fg_auth_result_t;

/**
 * @brief 安全配置
 */
typedef struct {
    xy_fg_security_type_t type;   /* 安全类型 */
    const uint8_t *key;           /* 密钥 */
    uint16_t key_len;             /* 密钥长度 */
    const uint8_t *challenge;     /* 挑战码 */
    uint16_t challenge_len;       /* 挑战码长度 */
} xy_fg_security_config_t;

/* ==================== 安全认证 API ==================== */

/**
 * @brief 配置安全认证
 * @param fg 电量计设备
 * @param config 安全配置
 * @return 状态码
 */
int xy_fuel_gauge_security_config(xy_fuel_gauge_t *fg, 
                                  const xy_fg_security_config_t *config);

/**
 * @brief 执行安全认证
 * @param fg 电量计设备
 * @return 认证结果
 */
xy_fg_auth_result_t xy_fuel_gauge_authenticate(xy_fuel_gauge_t *fg);

/**
 * @brief 验证设备真伪
 * @param fg 电量计设备
 * @return true=真品，false=仿冒
 */
bool xy_fuel_gauge_verify_device(xy_fuel_gauge_t *fg);

/**
 * @brief 加密数据
 * @param fg 电量计设备
 * @param data 原始数据
 * @param len 数据长度
 * @param encrypted 加密数据
 * @param encrypted_len 加密数据长度
 * @return 状态码
 */
int xy_fuel_gauge_encrypt_data(xy_fuel_gauge_t *fg,
                               const uint8_t *data, uint16_t len,
                               uint8_t *encrypted, uint16_t *encrypted_len);

/**
 * @brief 解密数据
 * @param fg 电量计设备
 * @param encrypted 加密数据
 * @param encrypted_len 加密数据长度
 * @param data 解密数据
 * @param len 解密数据长度
 * @return 状态码
 */
int xy_fuel_gauge_decrypt_data(xy_fuel_gauge_t *fg,
                               const uint8_t *encrypted, uint16_t encrypted_len,
                               uint8_t *data, uint16_t *len);

#ifdef __cplusplus
}
#endif

#endif /* XY_FUEL_GAUGE_SECURITY_H */
