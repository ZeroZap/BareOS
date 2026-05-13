/**
 * SPDX-License-Identifier: MIT
 * @file    xy_pmbus.h
 * @brief   PMBus Protocol Implementation - XinYi Framework
 * @version 1.0.0
 *
 * PMBus (Power Management Bus) 协议栈
 * 基于 SMBus，用于电源管理设备通信
 */

#ifndef __XY_PMBUS_H__
#define __XY_PMBUS_H__

#include "xy_smbus.h"
#include <stdint.h>
#include <stdbool.h>

/* ==================== PMBus 版本 ==================== */
#define PMBUS_SPEC_VERSION_1_1   0x11
#define PMBUS_SPEC_VERSION_1_2   0x12
#define PMBUS_SPEC_VERSION_1_3   0x13

/* ==================== PMBus 命令码 ==================== */
/* Page commands */
#define PMBUS_CMD_PAGE                 0x00
#define PMBUS_CMD_PAGE_PLUS_WRITE      0x01
#define PMBUS_CMD_PAGE_PLUS_READ       0x02

/* Operation commands */
#define PMBUS_CMD_OPERATION            0x01
#define PMBUS_CMD_ON_OFF_CONFIG        0x02
#define PMBUS_CMD_CLEAR_FAULTS         0x03
#define PMBUS_CMD_STORE_DEFAULT_ALL    0x10
#define PMBUS_CMD_RESTORE_DEFAULT_ALL  0x11
#define PMBUS_CMD_STORE_DEFAULT_CODE    0x12
#define PMBUS_CMD_RESTORE_DEFAULT_CODE 0x13
#define PMBUS_CMD_STORE_USER_ALL       0x14
#define PMBUS_CMD_RESTORE_USER_ALL      0x15

/* Version commands */
#define PMBUS_CMD_CAPABILITY           0x19
#define PMBUS_CMD_SMBUS_ALERT_MASK     0x1B

/* Query commands */
#define PMBUS_CMD_VOUT_MODE            0x20
#define PMBUS_CMD_VOUT_COMMAND         0x21
#define PMBUS_CMD_VOUT_TRIM            0x22
#define PMBUS_CMD_VOUT_CAL_OFFSET      0x23
#define PMBUS_CMD_VOUT_MAX             0x24
#define PMBUS_CMD_VOUT_MARGIN_HIGH     0x25
#define PMBUS_CMD_VOUT_MARGIN_LOW      0x26
#define PMBUS_CMD_VOUT_TRANSITION_RATE 0x27
#define PMBUS_CMD_VOUT_DROOP           0x28
#define PMBUS_CMD_VOUT_READ            0x2B
#define PMBUS_CMD_VOUT_OV_FAULT_LIMIT  0x2F
#define PMBUS_CMD_VOUT_OV_WARN_LIMIT   0x30
#define PMBUS_CMD_VOUT_UV_WARN_LIMIT   0x31
#define PMBUS_CMD_VOUT_UV_FAULT_LIMIT  0x32

/* Input/Output commands */
#define PMBUS_CMD_IOUT_IC_ALERT_LIMIT  0x34
#define PMBUS_CMD_IOUT_OC_FAULT_LIMIT  0x38
#define PMBUS_CMD_IOUT_OC_WARN_LIMIT   0x39
#define PMBUS_CMD_IOUT_READ            0x8C
#define PMBUS_CMD_IOUT_PEAK            0x8D

/* Temperature commands */
#define PMBUS_CMD_TEMP_1_INPUT         0x8D
#define PMBUS_CMD_TEMP_1_FAULT_LIMIT  0x8F
#define PMBUS_CMD_TEMP_1_WARN_LIMIT   0x90
#define PMBUS_CMD_TEMP_1_OVER_TEMP_FAULT_LIMIT 0x91
#define PMBUS_CMD_TEMP_1_OT_WARN_LIMIT 0x92
#define PMBUS_CMD_TEMP_2_INPUT         0x93

/* Input commands */
#define PMBUS_CMD_READ_VIN             0x88
#define PMBUS_CMD_IN_UV_FAULT_LIMIT   0x46
#define PMBUS_CMD_IN_UV_WARN_LIMIT    0x47
#define PMBUS_CMD_IN_OV_FAULT_LIMIT   0x35
#define PMBUS_CMD_IN_OV_WARN_LIMIT    0x36

/* Power commands */
#define PMBUS_CMD_READ_PIN             0x97
#define PMBUS_CMD_READ_POUT            0x96
#define PMBUS_CMD_PMBUS_REVISION       0x98
#define PMBUS_CMD_ID_WORD              0x9A
#define PMBUS_CMD_MFR_ID              0x99
#define PMBUS_CMD_MFR_MODEL           0x9A
#define PMBUS_CMD_MFR_REVISION        0x9B
#define PMBUS_CMD_MFR_LOCATION        0x9C
#define PMBUS_CMD_MFR_DATE            0x9D
#define PMBUS_CMD_MFR_SERIAL          0x9E

/* Status commands */
#define PMBUS_CMD_STATUS_WORD         0x79
#define PMBUS_CMD_STATUS_VOUT         0x7A
#define PMBUS_CMD_STATUS_IOUT         0x7B
#define PMBUS_CMD_STATUS_INPUT        0x7C
#define PMBUS_CMD_STATUS_TEMPERATURE   0x7D
#define PMBUS_CMD_STATUS_CML           0x7E
#define PMBUS_CMD_STATUS_OTHER        0x7F
#define PMBUS_CMD_STATUS_MFR_SPECIFIC 0x80

/* ==================== PMBus 操作模式 ==================== */
typedef enum {
    PMBUS_OP_OFF        = 0x00,  ///< Device is OFF
    PMBUS_OP_ON         = 0x01,  ///< Device is ON
    PMBUS_OP_MARGIN_OFF = 0x02,  ///< Margin high, output disabled
    PMBUS_OP_MARGIN_ON  = 0x03,  ///< Margin high, output enabled
} pmbus_operation_t;

/* ==================== PMBus VOUT 模式 ==================== */
typedef enum {
    PMBUS_VOUT_MODE_LINEAR     = 0x00,  ///< Linear mode (V = Y * 2^N)
    PMBUS_VOUT_MODE_VID        = 0x01,  ///< VID mode
    PMBUS_VOUT_MODE_VID_VR11    = 0x02,  ///< VR11 VID
    PMBUS_VOUT_MODE_DIRECT     = 0x03,  ///< Direct mode
    PMBUS_VOUT_MODE_SVID       = 0x04,  ///< SVID mode
    PMBUS_VOUT_MODE_EXT_VID    = 0x05,  ///< Extended VID
} pmbus_vout_mode_t;

/* ==================== PMBus 数据格式 ==================== */
typedef enum {
    PMBUS_FORMAT_LINEAR    = 0,   ///< Linear format (default)
    PMBUS_FORMAT_VID       = 1,   ///< VID format
    PMBUS_FORMAT_DIRECT    = 2,   ///< Direct format
    PMBUS_FORMAT_SIXTEEN   = 3,   ///< 16-bit unsigned
    PMBUS_FORMAT_TWO_COMP  = 4,   ///< Two's complement
} pmbus_format_t;

/* ==================== PMBus 状态字 ==================== */
typedef struct {
    /* VOUT status */
    bool vout_ov_fault;
    bool vout_ov_warning;
    bool vout_uv_warning;
    bool vout_uv_fault;
    bool vout_margin_high;
    bool vout_margin_low;

    /* IOUT status */
    bool iout_oc_fault;
    bool iout_oc_warning;
    bool iout_uc_fault;

    /* INPUT status */
    bool vin_ov_fault;
    bool vin_ov_warning;
    bool vin_uv_warning;
    bool vin_uv_fault;

    /* Temperature status */
    bool temp_ot_fault;
    bool temp_ot_warning;
    bool temp_ut_warning;

    /* General status */
    bool cml_fault;
    bool memory_fault;
    bool processor_fault;
    bool reserved;
    bool off;
    bool busy;
    bool fault_override;
    bool pmbus_revision_fault;
} pmbus_status_t;

/* ==================== PMBus 设备配置 ==================== */
typedef struct {
    smbus_config_t smbus;         ///< SMBus 配置
    pmbus_vout_mode_t vout_mode;  ///< VOUT 格式模式
    uint8_t spec_version;          ///< PMBus 规范版本
} pmbus_config_t;

/* ==================== PMBus 设备 ==================== */
typedef struct {
    char name[32];
    pmbus_config_t config;
    smbus_device_t *smbus;         ///< SMBus 设备
    void *priv_data;

    /* 缓存的状态 */
    uint16_t status_word;
    pmbus_status_t status;

    /* 操作接口 */
    const struct pmbus_ops *ops;

    struct pmbus_device *next;
} pmbus_device_t;

/* ==================== PMBus 操作接口 ==================== */
typedef struct pmbus_ops {
    /* 基础操作 */
    smbus_err_t (*init)(pmbus_device_t *dev);
    smbus_err_t (*deinit)(pmbus_device_t *dev);

    /* 读命令 */
    smbus_err_t (*read_vout)(pmbus_device_t *dev, float *voltage);
    smbus_err_t (*read_iout)(pmbus_device_t *dev, float *current);
    smbus_err_t (*read_vin)(pmbus_device_t *dev, float *voltage);
    smbus_err_t (*read_pin)(pmbus_device_t *dev, float *power);
    smbus_err_t (*read_pout)(pmbus_device_t *dev, float *power);
    smbus_err_t (*read_temp)(pmbus_device_t *dev, uint8_t sensor, float *temp);

    /* 写命令 */
    smbus_err_t (*write_vout_command)(pmbus_device_t *dev, float voltage);
    smbus_err_t (*set_operation)(pmbus_device_t *dev, pmbus_operation_t op);

    /* 状态 */
    smbus_err_t (*read_status_word)(pmbus_device_t *dev, uint16_t *status);
    smbus_err_t (*clear_faults)(pmbus_device_t *dev);
    smbus_err_t (*get_status)(pmbus_device_t *dev, pmbus_status_t *status);
} pmbus_ops_t;

/* ==================== PMBus 线性格式转换 ==================== */
/**
 * @brief   PMBus Linear 格式转换为浮点数
 *
 * PMBus Linear Format: V = Y * 2^N
 * - N: 5位指数 (2's complement, -16 到 +15)
 * - Y: 11位尾数 (2's complement)
 */
static inline float pmbus_linear_to_float(uint16_t raw)
{
    int8_t n = (int8_t)((raw >> 11) & 0x1F);
    if (n >= 0x10) n -= 0x20;  // 扩展符号位

    int16_t y = (int16_t)(raw & 0x07FF);
    if (y >= 0x0400) y -= 0x0800;  // 扩展符号位

    return (float)y * (float)(1 << n);
}

/**
 * @brief   浮点数转换为 PMBus Linear 格式
 */
static inline uint16_t pmbus_float_to_linear(float value)
{
    if (value == 0) return 0;

    int8_t n = 0;
    float y = value;

    /* 归一化 Y 到 11 位范围 */
    while (y > 2047.0f || y < -2048.0f) {
        y /= 2.0f;
        n++;
        if (n > 15) break;  // 防止溢出
    }
    while (y < -1024.0f || y >= 1024.0f) {
        y *= 2.0f;
        n--;
        if (n < -16) break;  // 防止下溢
    }

    if (n < -16) n = -16;
    if (n > 15) n = 15;

    uint8_t n_raw = (uint8_t)(n & 0x1F);
    int16_t y_raw = (int16_t)y;
    uint16_t raw = ((uint16_t)n_raw << 11) | ((uint16_t)(y_raw & 0x07FF));

    return raw;
}

/* ==================== PMBus VID 转换 ==================== */
/**
 * @brief   VID 代码转换为电压 (VR11 格式)
 */
static inline float pmbus_vid_to_voltage(uint8_t vid_code)
{
    /* VR11 格式: V = 0.0625 * (VID_CODE - 1) */
    return 0.0625f * (float)(vid_code - 1);
}

/**
 * @brief   电压转换为 VID 代码
 */
static inline uint8_t pmbus_voltage_to_vid(float voltage)
{
    return (uint8_t)(voltage / 0.0625f + 1.0f);
}

/* ==================== PMBus 直接模式转换 ==================== */
/**
 * @brief   PMBus Direct 格式转换为物理值
 *
 * 公式: Y = (R * X + B) * 10^S
 */
typedef struct {
    float slope;      // R
    float offset;     // B
    int8_t exponent;  // S
} pmbus_direct_coeff_t;

static inline float pmbus_direct_to_value(uint16_t raw, const pmbus_direct_coeff_t *coeff)
{
    float y = (coeff->slope * (float)raw + coeff->offset);
    for (int i = 0; i < abs(coeff->exponent); i++) {
        if (coeff->exponent > 0) {
            y *= 10.0f;
        } else {
            y /= 10.0f;
        }
    }
    return y;
}

/* ==================== 全局 API ==================== */
smbus_err_t pmbus_init(pmbus_device_t *dev);
smbus_err_t pmbus_deinit(pmbus_device_t *dev);
smbus_err_t pmbus_register(pmbus_device_t *dev);
smbus_err_t pmbus_unregister(pmbus_device_t *dev);
pmbus_device_t *pmbus_find(const char *name);

/* ==================== 读取命令 ==================== */
smbus_err_t pmbus_read_vout(pmbus_device_t *dev, float *voltage);
smbus_err_t pmbus_read_iout(pmbus_device_t *dev, float *current);
smbus_err_t pmbus_read_vin(pmbus_device_t *dev, float *voltage);
smbus_err_t pmbus_read_pin(pmbus_device_t *dev, float *power);
smbus_err_t pmbus_read_pout(pmbus_device_t *dev, float *power);
smbus_err_t pmbus_read_temp(pmbus_device_t *dev, uint8_t sensor, float *temp);

/* ==================== 写入命令 ==================== */
smbus_err_t pmbus_write_vout_command(pmbus_device_t *dev, float voltage);
smbus_err_t pmbus_set_operation(pmbus_device_t *dev, pmbus_operation_t op);
smbus_err_t pmbus_clear_faults(pmbus_device_t *dev);

/* ==================== 状态命令 ==================== */
smbus_err_t pmbus_read_status_word(pmbus_device_t *dev, uint16_t *status);
smbus_err_t pmbus_get_status(pmbus_device_t *dev, pmbus_status_t *status);
smbus_err_t pmbus_parse_status_word(uint16_t word, pmbus_status_t *status);

/* ==================== 配置命令 ==================== */
smbus_err_t pmbus_store_default(pmbus_device_t *dev);
smbus_err_t pmbus_restore_default(pmbus_device_t *dev);
smbus_err_t pmbus_set_page(pmbus_device_t *dev, uint8_t page);

/* ==================== 设备 ID ==================== */
smbus_err_t pmbus_read_id(pmbus_device_t *dev, char *model, uint8_t max_len);
smbus_err_t pmbus_read_mfr_id(pmbus_device_t *dev, char *mfr_id, uint8_t max_len);

/* ==================== 工具函数 ==================== */
const char *pmbus_err_str(smbus_err_t err);
void pmbus_dump_status(const pmbus_status_t *status);
void pmbus_dump_all_readings(pmbus_device_t *dev);

/* ==================== 常用 PMBus 设备定义 ==================== */
/* TI UCD 系列 */
#define PMBUS_DEV_TI_UCD90XXX   "TI_UCD90XXX"
/* Infineon (Cypress) TLD 系列 */
#define PMBUS_DEV_IFX_TLD55XX   "IFX_TLD55XX"
/* Analog Devices LTM 系列 */
#define PMBUS_DEV_ADI_LTM       "ADI_LTM"

/**
 * @example PMBus 使用示例
 * @code
 * // 1. 初始化 PMBus 设备
 * pmbus_device_t pmbus = {
 *     .name = "pmbus_vr",
 *     .config = {
 *         .smbus = { .addr = 0x40, .timeout_ms = 100, .pec_enable = true },
 *         .vout_mode = PMBUS_VOUT_MODE_LINEAR,
 *         .spec_version = PMBUS_SPEC_VERSION_1_2,
 *     },
 *     .smbus = &smbus_dev,
 *     .ops = &pmbus_default_ops,
 * };
 * pmbus_register(&pmbus);
 * pmbus_init(&pmbus);
 *
 * // 2. 读取输出电压
 * float vout;
 * pmbus_read_vout(&pmbus, &vout);
 * printf("VOUT: %.3f V\r\n", vout);
 *
 * // 3. 读取输出电流
 * float iout;
 * pmbus_read_iout(&pmbus, &iout);
 * printf("IOUT: %.3f A\r\n", iout);
 *
 * // 4. 设置输出电压
 * pmbus_write_vout_command(&pmbus, 1.200f);  // 设置为 1.2V
 *
 * // 5. 清除故障
 * pmbus_clear_faults(&pmbus);
 *
 * // 6. 读取状态
 * pmbus_status_t status;
 * pmbus_get_status(&pmbus, &status);
 * if (status.vout_ov_fault) {
 *     printf("VOUT Over Voltage Fault!\r\n");
 * }
 * @endcode
 */

#endif /* __XY_PMBUS_H__ */