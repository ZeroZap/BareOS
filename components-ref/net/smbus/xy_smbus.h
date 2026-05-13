/**
 * SPDX-License-Identifier: MIT
 * @file    xy_smbus.h
 * @brief   SMBus Protocol Implementation - XinYi Framework
 * @version 1.0.0
 *
 * SMBus (System Management Bus) 协议栈
 * 基于 I2C，提供系统管理通信接口
 */

#ifndef __XY_SMBUS_H__
#define __XY_SMBUS_H__

#include <stdint.h>
#include <stdbool.h>

/* ==================== 常量定义 ==================== */
#define SMBUS_MAX_PAYLOAD     32
#define SMBUS_TIMEOUT_MS     100
#define SMBUS_ADDR_MIN       0x08
#define SMBUS_ADDR_MAX       0x77

/* ==================== SMBus 地址类型 ==================== */
typedef enum {
    SMBUS_ADDR_7BIT  = 0,   ///< 7位地址
    SMBUS_ADDR_8BIT  = 1,   ///< 8位地址 (带 R/W 位)
} smbus_addr_type_t;

/* ==================== SMBus 协议命令 ==================== */
typedef enum {
    SMBUS_CMD_QUICK          = 0x00,   ///< Quick command (no data)
    SMBUS_CMD_SLAVE_ADDR     = 0x01,   ///< Send/Receive byte with slave address
    SMBUS_CMD_BYTE           = 0x02,   ///< Byte command
    SMBUS_CMD_BYTE_DATA      = 0x03,   ///< Byte data (1 byte write/read)
    SMBUS_CMD_WORD_DATA      = 0x04,   ///< Word data (2 byte write/read)
    SMBUS_CMD_PROCESS_CALL   = 0x05,   ///< Process call (write 2 bytes, read 2 bytes)
    SMBUS_CMD_BLOCK          = 0x06,   ///< Block commands
    SMBUS_CMD_I2C_BLOCK      = 0x07,   ///< I2C block (no PEC)
    SMBUS_CMD_BLOCK_PROC_CALL= 0x08,   ///< Block process call
    SMBUS_CMD_BLOCK_WRITE    = 0x09,   ///< Block write
    SMBUS_CMD_BLOCK_READ     = 0x0A,   ///< Block read
    SMBUS_CMD_PAGE           = 0x0B,   ///< Page/Block
    SMBUS_CMD_BLOCK_WRITE32  = 0x0C,   ///< 32-byte block write
    SMBUS_CMD_BLOCK_READ32   = 0x0D,   ///< 32-byte block read
} smbus_cmd_type_t;

/* ==================== SMBus 错误码 ==================== */
typedef enum {
    SMBUS_EOK            =  0,
    SMBUS_ERROR          = -1,
    SMBUS_EINVAL         = -2,
    SMBUS_ETIMEOUT       = -3,
    SMBUS_EACK           = -4,    ///< No acknowledge from slave
    SMBUS_ECOLLISION      = -5,    ///< Bus collision
    SMBUS_EBUSY          = -6,
    SMBUS_ELENGTH        = -7,    ///< Data length error
    SMBUS_EPEC           = -8,    ///< PEC error
    SMBUS_ENODEV         = -9,
} smbus_err_t;

/* ==================== SMBus 设备配置 ==================== */
typedef struct {
    uint8_t addr;                ///< 7位 I2C 地址
    uint32_t timeout_ms;        ///< 超时时间 (ms)
    bool pec_enable;             ///< 是否启用 PEC (Packet Error Code)
    uint16_t speed_khz;         ///< 总线速度 (kHz), 默认 100
} smbus_config_t;

/* ==================== SMBus 数据包 ==================== */
typedef struct {
    uint8_t addr;                ///< 7位 I2C 地址
    uint8_t command;            ///< SMBus 命令码
    smbus_cmd_type_t cmd_type;  ///< 命令类型
    uint8_t len;                ///< 数据长度
    uint8_t data[SMBUS_MAX_PAYLOAD];  ///< 数据缓冲区
    uint8_t pec;                ///< PEC (可选)
} smbus_packet_t;

/* ==================== SMBus 设备 ==================== */
typedef struct smbus_device {
    char name[32];
    smbus_config_t config;
    void *i2c_bus;              ///< I2C 总线句柄
    void *priv_data;

    /* 操作接口 */
    const struct smbus_ops *ops;

    struct smbus_device *next;
} smbus_device_t;

/* ==================== SMBus 操作接口 ==================== */
typedef struct smbus_ops {
    smbus_err_t (*init)(smbus_device_t *dev);
    smbus_err_t (*deinit)(smbus_device_t *dev);

    /* 基础读写 */
    smbus_err_t (*write_byte)(smbus_device_t *dev, uint8_t addr, uint8_t command, uint8_t data);
    smbus_err_t (*read_byte)(smbus_device_t *dev, uint8_t addr, uint8_t command, uint8_t *data);
    smbus_err_t (*write_word)(smbus_device_t *dev, uint8_t addr, uint8_t command, uint16_t data);
    smbus_err_t (*read_word)(smbus_device_t *dev, uint8_t addr, uint8_t command, uint16_t *data);

    /* 块操作 */
    smbus_err_t (*write_block)(smbus_device_t *dev, uint8_t addr, uint8_t command,
                                const uint8_t *data, uint8_t len);
    smbus_err_t (*read_block)(smbus_device_t *dev, uint8_t addr, uint8_t command,
                               uint8_t *data, uint8_t *len);

    /* 进程调用 */
    smbus_err_t (*process_call)(smbus_device_t *dev, uint8_t addr, uint8_t command,
                                 uint16_t write_data, uint16_t *read_data);

    /* PEC */
    smbus_err_t (*calculate_pec)(const uint8_t *data, uint8_t len, uint8_t *pec);
    bool (*verify_pec)(const uint8_t *data, uint8_t len, uint8_t expected_pec);
} smbus_ops_t;

/* ==================== SMBus 辅助命令 ==================== */
typedef enum {
    SMBUS_FUNC_QUICK            = (1 << 0),
    SMBUS_FUNC_BYTE            = (1 << 1),
    SMBUS_FUNC_BYTE_DATA       = (1 << 2),
    SMBUS_FUNC_WORD_DATA       = (1 << 3),
    SMBUS_FUNC_PROCESS_CALL    = (1 << 4),
    SMBUS_FUNC_BLOCK           = (1 << 5),
    SMBUS_FUNC_BLOCK_PROC_CALL = (1 << 6),
    SMBUS_FUNC_I2C_BLOCK       = (1 << 7),
} smbus_functionality_t;

/* ==================== SMBus 报警级别 ==================== */
typedef enum {
    SMBUS_ALERT_NONE           = 0,
    SMBUS_ALERT_LOW            = 1,    ///< SMBALERT# 低电平
    SMBUS_ALERT_RESPONSE_ADDR  = 2,    ///< 报警响应地址模式
} smbus_alert_level_t;

/* ==================== SMBus 回调 ==================== */
typedef void (*smbus_alert_callback_t)(smbus_device_t *dev, uint8_t alert_mask);
typedef void (*smbus_callback_t)(smbus_device_t *dev, smbus_err_t result, void *user_data);

/* ==================== SMBus 默认配置 ==================== */
#define SMBUS_CONFIG_DEFAULT(_addr) { \
    .addr = _addr, \
    .timeout_ms = SMBUS_TIMEOUT_MS, \
    .pec_enable = false, \
    .speed_khz = 100, \
}

/* ==================== PEC 计算 (CRC-8) ==================== */
/**
 * @brief   计算 SMBus PEC (Packet Error Code)
 * @param   data: 数据指针
 * @param   len: 数据长度
 * @return  PEC 值 (0-255)
 */
static inline uint8_t smbus_pec_calculate(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;  // Polynomial: x^8 + x^2 + x + 1
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief   验证 PEC
 */
static inline bool smbus_pec_verify(const uint8_t *data, uint8_t len, uint8_t expected)
{
    uint8_t calculated = smbus_pec_calculate(data, len);
    return (calculated == expected);
}

/* ==================== 地址工具函数 ==================== */
/**
 * @brief   7位地址转换为8位地址
 */
static inline uint8_t smbus_addr_7to8(uint8_t addr_7bit, bool read)
{
    return (addr_7bit << 1) | (read ? 0x01 : 0x00);
}

/**
 * @brief   8位地址转换为7位地址
 */
static inline uint8_t smbus_addr_8to7(uint8_t addr_8bit)
{
    return (addr_8bit >> 1);
}

/**
 * @brief   检查地址是否有效
 */
static inline bool smbus_addr_valid(uint8_t addr_7bit)
{
    return (addr_7bit >= SMBUS_ADDR_MIN && addr_7bit <= SMBUS_ADDR_MAX);
}

/* ==================== 全局 API ==================== */
smbus_err_t smbus_init(smbus_device_t *dev);
smbus_err_t smbus_deinit(smbus_device_t *dev);
smbus_err_t smbus_scan(smbus_device_t *dev, uint8_t *addrs, uint8_t *count);
smbus_err_t smbus_quick_command(smbus_device_t *dev, uint8_t addr, bool read);

/* ==================== 字节操作 ==================== */
smbus_err_t smbus_send_byte(smbus_device_t *dev, uint8_t addr, uint8_t data);
smbus_err_t smbus_receive_byte(smbus_device_t *dev, uint8_t addr, uint8_t *data);
smbus_err_t smbus_write_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t data);
smbus_err_t smbus_read_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t *data);

/* ==================== 字操作 ==================== */
smbus_err_t smbus_write_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t data);
smbus_err_t smbus_read_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t *data);

/* ==================== 块操作 ==================== */
smbus_err_t smbus_write_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                               const uint8_t *data, uint8_t len);
smbus_err_t smbus_read_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                              uint8_t *data, uint8_t *len);

/* ==================== 进程调用 ==================== */
smbus_err_t smbus_process_call(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                uint16_t write_data, uint16_t *read_data);
smbus_err_t smbus_block_process_call(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                      const uint8_t *write_data, uint8_t write_len,
                                      uint8_t *read_data, uint8_t *read_len);

/* ==================== 设备注册 ==================== */
smbus_err_t smbus_register(smbus_device_t *dev);
smbus_err_t smbus_unregister(smbus_device_t *dev);
smbus_device_t *smbus_find(const char *name);
smbus_device_t *smbus_find_by_addr(uint8_t addr);

/* ==================== 报警处理 ==================== */
smbus_err_t smbus_alert_register_callback(smbus_alert_callback_t callback, void *user_data);
smbus_err_t smbus_alert_response(uint8_t addr);

/* ==================== 工具函数 ==================== */
const char *smbus_err_str(smbus_err_t err);
void smbus_dump_packet(const smbus_packet_t *pkt);

/**
 * @example SMBus 使用示例
 * @code
 * // 1. 初始化 SMBus 设备
 * smbus_device_t smbus = {
 *     .name = "smbus1",
 *     .config = SMBUS_CONFIG_DEFAULT(0x40),
 *     .ops = &smbus_default_ops,
 *     .i2c_bus = &i2c1,
 * };
 * smbus_register(&smbus);
 * smbus_init(&smbus);
 *
 * // 2. 写字节数据
 * smbus_write_byte(&smbus, 0x40, 0x01, 0xAA);
 *
 * // 3. 读字数据
 * uint16_t value;
 * smbus_read_word(&smbus, 0x40, 0x02, &value);
 *
 * // 4. 块读
 * uint8_t buf[32];
 * uint8_t len;
 * smbus_read_block(&smbus, 0x40, 0x12, buf, &len);
 *
 * // 5. 扫描总线上的设备
 * uint8_t addrs[16];
 * uint8_t count;
 * smbus_scan(&smbus, addrs, &count);
 * for (int i = 0; i < count; i++) {
 *     printf("Found device at 0x%02X\r\n", addrs[i]);
 * }
 * @endcode
 */

#endif /* __XY_SMBUS_H__ */