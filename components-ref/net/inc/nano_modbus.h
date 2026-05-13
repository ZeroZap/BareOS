/**
 * @file nano_modbus.h
 * @brief Nano Modbus - Unified API (Tiny + Full)
 * 
 * This is a compatibility layer. For new projects, use:
 *   - mb_tiny.h  : For resource-constrained devices (RTU only)
 *   - mb_full.h  : For full-featured Modbus (RTU + TCP + ASCII)
 * 
 * @version 2.0.0
 * @date 2026-03-31
 */

#ifndef NANO_MODBUS_H
#define NANO_MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置开关 ==================== */

/* 选择使用哪个版本:
 *   1 = 使用 Tiny 版本 (RTU only, minimal footprint)
 *   0 = 使用 Full 版本 (RTU + TCP + ASCII)
 */
#ifndef NANO_MB_USE_TINY
#define NANO_MB_USE_TINY 1
#endif

/* ==================== 统一 API ==================== */

#if NANO_MB_USE_TINY

/* 使用 Tiny 版本 */
#include "modbus_tiny/mb_tiny.h"

#else

/* 使用 Full 版本 */
#include "modbus_full/mb_full.h"

/* 兼容类型别名 */
typedef mb_full_slave_t  mb_slave_t;
typedef mb_full_master_t mb_master_t;
typedef mb_full_send_cb_t mb_send_cb_t;
typedef mb_full_recv_cb_t mb_recv_cb_t;
typedef mb_mode_t mb_mode_t;

/* 兼容宏 */
#define NANO_MB_OK MB_FULL_OK
#define NANO_MB_ERROR MB_FULL_ERROR
#define NANO_MB_INVALID_PARAM MB_FULL_INVALID_PARAM
#define NANO_MB_TIMEOUT MB_FULL_TIMEOUT
#define NANO_MB_CRC_ERROR MB_FULL_CRC_ERROR

#define MB_FUNC_READ_COILS MB_FUNC_READ_COILS
#define MB_FUNC_READ_HOLDING MB_FUNC_READ_HOLDING_REGISTERS
#define MB_FUNC_WRITE_SINGLE_REG MB_FUNC_WRITE_SINGLE_REGISTER
#define MB_FUNC_WRITE_MULTIPLE_REGS MB_FUNC_WRITE_MULTIPLE_REGISTERS

#define nano_mb_crc16 mb_full_crc16
#define nano_mb_error_string mb_full_error_string

/* 兼容函数 */
static inline int nano_mb_slave_init(mb_slave_t *slave, void *config)
{
    return mb_full_slave_init(slave, ((mb_slave_config_t*)config)->slave_id, MB_MODE_RTU);
}

static inline int nano_mb_master_init(mb_master_t *master)
{
    return mb_full_master_init(master, MB_MODE_RTU);
}

#endif

/* ==================== 兼容层 ==================== */

/**
 * @brief 从站配置 (兼容旧 API)
 */
typedef struct {
    uint8_t slave_id;
    uint16_t coil_start;
    uint16_t coil_count;
    uint16_t discrete_start;
    uint16_t discrete_count;
    uint16_t holding_start;
    uint16_t holding_count;
    uint16_t input_start;
    uint16_t input_count;
} mb_slave_config_t;

/* ==================== 兼容 API ==================== */

#if NANO_MB_USE_TINY

/* 兼容类型别名 */
typedef mb_tiny_slave_t  mb_slave_t;
typedef mb_tiny_master_t mb_master_t;

/* 兼容宏 */
#define NANO_MB_OK MB_TINY_OK
#define NANO_MB_ERROR MB_TINY_ERROR
#define NANO_MB_INVALID_PARAM MB_TINY_INVALID_PARAM
#define NANO_MB_TIMEOUT MB_TINY_TIMEOUT
#define NANO_MB_CRC_ERROR MB_TINY_CRC_ERROR

#define MB_FUNC_READ_COILS MB_FUNC_READ_COILS
#define MB_FUNC_READ_HOLDING MB_FUNC_READ_HOLDING
#define MB_FUNC_WRITE_SINGLE_REG MB_FUNC_WRITE_SINGLE_REG
#define MB_FUNC_WRITE_MULTIPLE_REGS MB_FUNC_WRITE_MULTIPLE_REGS

#define nano_mb_crc16 mb_tiny_crc16

static inline const char* nano_mb_error_string(int error)
{
    switch (error) {
        case MB_TINY_OK: return "OK";
        case MB_TINY_ERROR: return "Error";
        case MB_TINY_INVALID_PARAM: return "Invalid Parameter";
        case MB_TINY_TIMEOUT: return "Timeout";
        case MB_TINY_CRC_ERROR: return "CRC Error";
        default: return "Unknown";
    }
}

/* 兼容函数 - 将旧 API 映射到 Tiny API */
static inline int nano_mb_slave_init(mb_slave_t *slave, const mb_slave_config_t *config)
{
    return mb_tiny_slave_init(slave, config ? config->slave_id : 1);
}

static inline int nano_mb_slave_deinit(mb_slave_t *slave)
{
    if (!slave) return MB_TINY_INVALID_PARAM;
    slave->initialized = false;
    return MB_TINY_OK;
}

static inline int nano_mb_master_init(mb_master_t *master)
{
    return mb_tiny_master_init(master);
}

static inline int nano_mb_master_deinit(mb_master_t *master)
{
    if (!master) return MB_TINY_INVALID_PARAM;
    master->initialized = false;
    return MB_TINY_OK;
}

static inline int nano_mb_slave_poll(mb_slave_t *slave, const uint8_t *data, uint16_t len)
{
    return mb_tiny_slave_handle(slave, data, len);
}

static inline int nano_mb_master_read_holding(mb_master_t *master, uint8_t slave_id,
                                               uint16_t addr, uint16_t count, 
                                               uint16_t *data, uint32_t timeout)
{
    (void)timeout;
    return mb_tiny_master_read_holding(master, slave_id, addr, count, data);
}

static inline int nano_mb_master_write_reg(mb_master_t *master, uint8_t slave_id,
                                            uint16_t addr, uint16_t value, uint32_t timeout)
{
    (void)timeout;
    return mb_tiny_master_write_reg(master, slave_id, addr, value);
}

static inline int nano_mb_master_write_multi_regs(mb_master_t *master, uint8_t slave_id,
                                                   uint16_t addr, uint16_t count, 
                                                   const uint16_t *data, uint32_t timeout)
{
    (void)timeout;
    return mb_tiny_master_write_regs(master, slave_id, addr, count, data);
}

static inline int nano_mb_master_read_coils(mb_master_t *master, uint8_t slave_id,
                                             uint16_t addr, uint16_t count, 
                                             uint8_t *data, uint32_t timeout)
{
    (void)timeout;
    return mb_tiny_master_read_coils(master, slave_id, addr, count, data);
}

#endif /* NANO_MB_USE_TINY */

#ifdef __cplusplus
}
#endif

#endif /* NANO_MODBUS_H */
