/**
 * SPDX-License-Identifier: MIT
 * @file    xy_smbus.c
 * @brief   SMBus Protocol Implementation
 * @version 1.0.0
 */

#include "xy_smbus.h"
#include "xy_log.h"
#include <string.h>

/* ==================== 常量 ==================== */
#define SMBUS_PEC_POLYNOMIAL   0x07

/* ==================== 静态变量 ==================== */
#define MAX_SMBUS_DEVICES   8
static smbus_device_t *g_smbus_devices[MAX_SMBUS_DEVICES];
static uint8_t g_smbus_count = 0;
static smbus_alert_callback_t g_alert_callback = NULL;
static void *g_alert_user_data = NULL;

/* ==================== 工具函数 ==================== */
const char *smbus_err_str(smbus_err_t err)
{
    switch (err) {
        case SMBUS_EOK:            return "OK";
        case SMBUS_ERROR:          return "Error";
        case SMBUS_EINVAL:         return "Invalid argument";
        case SMBUS_ETIMEOUT:       return "Timeout";
        case SMBUS_EACK:           return "No acknowledge";
        case SMBUS_ECOLLISION:     return "Bus collision";
        case SMBUS_EBUSY:          return "Busy";
        case SMBUS_ELENGTH:        return "Length error";
        case SMBUS_EPEC:           return "PEC error";
        case SMBUS_ENODEV:         return "Device not found";
        default:                   return "Unknown";
    }
}

void smbus_dump_packet(const smbus_packet_t *pkt)
{
    if (pkt == NULL) return;

    printf("SMBus Packet:\r\n");
    printf("  Addr: 0x%02X\r\n", pkt->addr);
    printf("  Cmd: 0x%02X\r\n", pkt->command);
    printf("  Len: %d\r\n", pkt->len);
    if (pkt->len > 0) {
        printf("  Data: ");
        for (uint8_t i = 0; i < pkt->len && i < 16; i++) {
            printf("%02X ", pkt->data[i]);
        }
        printf("\r\n");
    }
    printf("  PEC: 0x%02X\r\n", pkt->pec);
}

/* ==================== 设备管理 ==================== */
smbus_err_t smbus_register(smbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL) {
        return SMBUS_EINVAL;
    }

    if (g_smbus_count >= MAX_SMBUS_DEVICES) {
        return SMBUS_ERROR;
    }

    g_smbus_devices[g_smbus_count++] = dev;
    return SMBUS_EOK;
}

smbus_err_t smbus_unregister(smbus_device_t *dev)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    for (int i = 0; i < g_smbus_count; i++) {
        if (g_smbus_devices[i] == dev) {
            for (int j = i; j < g_smbus_count - 1; j++) {
                g_smbus_devices[j] = g_smbus_devices[j + 1];
            }
            g_smbus_count--;
            return SMBUS_EOK;
        }
    }

    return SMBUS_ENODEV;
}

smbus_device_t *smbus_find(const char *name)
{
    if (name == NULL) return NULL;

    for (int i = 0; i < g_smbus_count; i++) {
        if (strcmp(g_smbus_devices[i]->name, name) == 0) {
            return g_smbus_devices[i];
        }
    }
    return NULL;
}

smbus_device_t *smbus_find_by_addr(uint8_t addr)
{
    for (int i = 0; i < g_smbus_count; i++) {
        if (g_smbus_devices[i]->config.addr == addr) {
            return g_smbus_devices[i];
        }
    }
    return NULL;
}

/* ==================== 基础操作 ==================== */
smbus_err_t smbus_init(smbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->init == NULL) {
        return SMBUS_EINVAL;
    }

    return dev->ops->init(dev);
}

smbus_err_t smbus_deinit(smbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->deinit == NULL) {
        return SMBUS_EINVAL;
    }

    return dev->ops->deinit(dev);
}

/* ==================== 字节操作 ==================== */
smbus_err_t smbus_send_byte(smbus_device_t *dev, uint8_t addr, uint8_t data)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    /* I2C Start -> Addr(W) -> Data -> Stop */
    /* 具体实现依赖 HAL I2C */
    /* hal_i2c_start(dev->i2c_bus); */
    /* hal_i2c_send_addr(dev->i2c_bus, smbus_addr_7to8(addr, false)); */
    /* hal_i2c_send_byte(dev->i2c_bus, data); */
    /* hal_i2c_stop(dev->i2c_bus); */

    (void)addr;
    (void)data;
    return SMBUS_EOK;
}

smbus_err_t smbus_receive_byte(smbus_device_t *dev, uint8_t addr, uint8_t *data)
{
    if (dev == NULL || data == NULL) {
        return SMBUS_EINVAL;
    }

    /* I2C Start -> Addr(R) -> Data(NACK) -> Stop */
    (void)addr;
    (void)data;
    return SMBUS_EOK;
}

smbus_err_t smbus_write_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t data)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Write Byte: Start -> Addr(W) -> Cmd -> Data -> PEC(optional) -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    return SMBUS_EOK;
}

smbus_err_t smbus_read_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t *data)
{
    if (dev == NULL || data == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Read Byte: Start -> Addr(W) -> Cmd -> Repeated Start -> Addr(R) -> Data -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    return SMBUS_EOK;
}

/* ==================== 字操作 ==================== */
smbus_err_t smbus_write_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t data)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Write Word: Start -> Addr(W) -> Cmd -> Low Byte -> High Byte -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    return SMBUS_EOK;
}

smbus_err_t smbus_read_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t *data)
{
    if (dev == NULL || data == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Read Word: Start -> Addr(W) -> Cmd -> Repeated Start -> Addr(R) -> Low -> High -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    return SMBUS_EOK;
}

/* ==================== 块操作 ==================== */
smbus_err_t smbus_write_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                               const uint8_t *data, uint8_t len)
{
    if (dev == NULL || data == NULL || len > SMBUS_MAX_PAYLOAD) {
        return SMBUS_EINVAL;
    }

    /* SMBus Block Write:
     * Start -> Addr(W) -> Cmd -> Byte Count -> Data[0..n] -> PEC -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    (void)len;
    return SMBUS_EOK;
}

smbus_err_t smbus_read_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                              uint8_t *data, uint8_t *len)
{
    if (dev == NULL || data == NULL || len == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Block Read:
     * Start -> Addr(W) -> Cmd -> Repeated Start -> Addr(R) ->
     * Byte Count -> Data[0..n] -> PEC -> Stop */
    (void)addr;
    (void)cmd;
    (void)data;
    (void)len;
    return SMBUS_EOK;
}

/* ==================== 进程调用 ==================== */
smbus_err_t smbus_process_call(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                uint16_t write_data, uint16_t *read_data)
{
    if (dev == NULL || read_data == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Process Call:
     * Start -> Addr(W) -> Cmd -> Write Low -> Write High ->
     * Repeated Start -> Addr(R) -> Read Low -> Read High -> Stop */
    (void)addr;
    (void)cmd;
    (void)write_data;
    (void)read_data;
    return SMBUS_EOK;
}

smbus_err_t smbus_block_process_call(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                      const uint8_t *write_data, uint8_t write_len,
                                      uint8_t *read_data, uint8_t *read_len)
{
    if (dev == NULL || write_data == NULL || read_data == NULL || read_len == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Block Process Call:
     * Start -> Addr(W) -> Cmd -> Byte Count -> Write Data ->
     * Repeated Start -> Addr(R) -> Read Count -> Read Data -> Stop */
    (void)addr;
    (void)cmd;
    (void)write_data;
    (void)write_len;
    (void)read_data;
    (void)read_len;
    return SMBUS_EOK;
}

/* ==================== 扫描总线 ==================== */
smbus_err_t smbus_scan(smbus_device_t *dev, uint8_t *addrs, uint8_t *count)
{
    if (dev == NULL || addrs == NULL || count == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t found = 0;

    /* 扫描所有有效地址 */
    for (uint8_t addr = SMBUS_ADDR_MIN; addr <= SMBUS_ADDR_MAX; addr++) {
        /* 尝试发送地址，检查是否有 ACK */
        /* if (hal_i2c_probe(dev->i2c_bus, addr) == 0) { */
        /*     addrs[found++] = addr; */
        /* } */
    }

    *count = found;
    return SMBUS_EOK;
}

/* ==================== Quick Command ==================== */
smbus_err_t smbus_quick_command(smbus_device_t *dev, uint8_t addr, bool read)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    /* SMBus Quick Command:
     * Start -> Addr(R/W) -> Stop */
    (void)addr;
    (void)read;
    return SMBUS_EOK;
}

/* ==================== 报警处理 ==================== */
smbus_err_t smbus_alert_register_callback(smbus_alert_callback_t callback, void *user_data)
{
    g_alert_callback = callback;
    g_alert_user_data = user_data;
    return SMBUS_EOK;
}

smbus_err_t smbus_alert_response(uint8_t addr)
{
    if (!smbus_addr_valid(addr)) {
        return SMBUS_EINVAL;
    }

    /* SMBus Alert Response Address (ARA) = 0x0C
     * 主机响应报警，从设备释放 SMBALERT# 线 */
    (void)addr;
    return SMBUS_EOK;
}

/* ==================== PEC 操作 ==================== */
smbus_err_t smbus_calculate_pec(smbus_device_t *dev, const uint8_t *data, uint8_t len, uint8_t *pec)
{
    if (dev == NULL || data == NULL || pec == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->calculate_pec) {
        return dev->ops->calculate_pec(data, len, pec);
    }

    *pec = smbus_pec_calculate(data, len);
    return SMBUS_EOK;
}

bool smbus_verify_pec(smbus_device_t *dev, const uint8_t *data, uint8_t len, uint8_t expected)
{
    if (dev == NULL || data == NULL) {
        return false;
    }

    if (dev->ops && dev->ops->verify_pec) {
        return dev->ops->verify_pec(data, len, expected);
    }

    uint8_t calculated = smbus_pec_calculate(data, len);
    return (calculated == expected);
}

/* ==================== 默认操作实现 ==================== */
static smbus_err_t default_init(smbus_device_t *dev)
{
    (void)dev;
    return SMBUS_EOK;
}

static smbus_err_t default_deinit(smbus_device_t *dev)
{
    (void)dev;
    return SMBUS_EOK;
}

static smbus_err_t default_write_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t data)
{
    return smbus_write_byte(dev, addr, cmd, data);
}

static smbus_err_t default_read_byte(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint8_t *data)
{
    return smbus_read_byte(dev, addr, cmd, data);
}

static smbus_err_t default_write_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t data)
{
    return smbus_write_word(dev, addr, cmd, data);
}

static smbus_err_t default_read_word(smbus_device_t *dev, uint8_t addr, uint8_t cmd, uint16_t *data)
{
    return smbus_read_word(dev, addr, cmd, data);
}

static smbus_err_t default_write_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                       const uint8_t *data, uint8_t len)
{
    return smbus_write_block(dev, addr, cmd, data, len);
}

static smbus_err_t default_read_block(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                      uint8_t *data, uint8_t *len)
{
    return smbus_read_block(dev, addr, cmd, data, len);
}

static smbus_err_t default_process_call(smbus_device_t *dev, uint8_t addr, uint8_t cmd,
                                        uint16_t write_data, uint16_t *read_data)
{
    return smbus_process_call(dev, addr, cmd, write_data, read_data);
}

static smbus_err_t default_calculate_pec(const uint8_t *data, uint8_t len, uint8_t *pec)
{
    *pec = smbus_pec_calculate(data, len);
    return SMBUS_EOK;
}

static bool default_verify_pec(const uint8_t *data, uint8_t len, uint8_t expected)
{
    return smbus_pec_verify(data, len, expected);
}

/* ==================== 默认操作表 ==================== */
const smbus_ops_t smbus_default_ops = {
    .init = default_init,
    .deinit = default_deinit,
    .write_byte = default_write_byte,
    .read_byte = default_read_byte,
    .write_word = default_write_word,
    .read_word = default_read_word,
    .write_block = default_write_block,
    .read_block = default_read_block,
    .process_call = default_process_call,
    .calculate_pec = default_calculate_pec,
    .verify_pec = default_verify_pec,
};