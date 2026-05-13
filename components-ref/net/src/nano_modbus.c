/**
 * @file nano_modbus.c
 * @brief Nano Modbus RTU Protocol Stack Implementation (TCP-free)
 * @version 1.0.0
 * @date 2026-03-01 下午
 */

#include "nano_modbus.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief Modbus CRC16 表
 */
static const uint16_t g_crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t nano_mb_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    
    for (i = 0; i < len; i++) {
        crc = (crc >> 8) ^ g_crc16_table[(crc ^ data[i]) & 0xFF];
    }
    
    return crc;
}

const char* nano_mb_error_string(int error)
{
    switch (error) {
        case NANO_MB_OK: return "OK";
        case NANO_MB_ERROR: return "General Error";
        case NANO_MB_INVALID_PARAM: return "Invalid Parameter";
        case NANO_MB_TIMEOUT: return "Timeout";
        case NANO_MB_CRC_ERROR: return "CRC Error";
        case NANO_MB_BUSY: return "Busy";
        default: return "Unknown";
    }
}

/* ==================== 从站实现 ==================== */

int nano_mb_slave_init(mb_slave_t *slave, const mb_slave_config_t *config)
{
    if (!slave || !config) {
        return NANO_MB_INVALID_PARAM;
    }
    
    memset(slave, 0, sizeof(*slave));
    memcpy(&slave->config, config, sizeof(mb_slave_config_t));
    
    slave->initialized = true;
    
    xy_log_i("Nano Modbus Slave initialized (ID=%d)\n", slave->config.slave_id);
    return NANO_MB_OK;
}

int nano_mb_slave_deinit(mb_slave_t *slave)
{
    if (!slave) {
        return NANO_MB_INVALID_PARAM;
    }
    
    slave->initialized = false;
    return NANO_MB_OK;
}

int nano_mb_slave_register_read_coils(mb_slave_t *slave, mb_read_coils_cb_t callback)
{
    if (!slave) {
        return NANO_MB_INVALID_PARAM;
    }
    
    slave->read_coils_cb = callback;
    return NANO_MB_OK;
}

int nano_mb_slave_register_read_holding(mb_slave_t *slave, mb_read_holding_cb_t callback)
{
    if (!slave) {
        return NANO_MB_INVALID_PARAM;
    }
    
    slave->read_holding_cb = callback;
    return NANO_MB_OK;
}

int nano_mb_slave_register_write_reg(mb_slave_t *slave, mb_write_reg_cb_t callback)
{
    if (!slave) {
        return NANO_MB_INVALID_PARAM;
    }
    
    slave->write_reg_cb = callback;
    return NANO_MB_OK;
}

/**
 * @brief 处理读保持寄存器请求
 */
static int mb_slave_handle_read_holding(mb_slave_t *slave, uint16_t addr, uint16_t count)
{
    uint8_t *tx_buf = slave->tx_buf;
    int ret;
    
    if (!slave->read_holding_cb) {
        return MB_ERROR_ILLEGAL_FUNC;
    }
    
    /* 检查地址范围 */
    if (addr < slave->config.holding_start ||
        addr >= slave->config.holding_start + slave->config.holding_count) {
        return MB_ERROR_ILLEGAL_DATA_ADDR;
    }
    
    /* 构建响应帧 */
    tx_buf[0] = slave->config.slave_id;
    tx_buf[1] = MB_FUNC_READ_HOLDING;
    tx_buf[2] = count * 2;  /* 字节数 */
    
    /* 读取寄存器数据 */
    uint16_t *data = (uint16_t *)&tx_buf[3];
    ret = slave->read_holding_cb(addr, count, data);
    if (ret < 0) {
        return MB_ERROR_SLAVE_DEVICE_FAIL;
    }
    
    /* 计算 CRC */
    uint16_t crc = nano_mb_crc16(tx_buf, 3 + count * 2);
    tx_buf[3 + count * 2] = crc & 0xFF;
    tx_buf[3 + count * 2 + 1] = (crc >> 8) & 0xFF;

    /* 发送响应 */
    if (slave->send_cb) {
        slave->send_cb(tx_buf, 4 + count * 2);
    }

    slave->request_count++;
    return NANO_MB_OK;
}

/**
 * @brief 处理写单个寄存器请求
 */
static int mb_slave_handle_write_reg(mb_slave_t *slave, uint16_t addr, uint16_t value)
{
    uint8_t *tx_buf = slave->tx_buf;
    int ret;
    
    if (!slave->write_reg_cb) {
        return MB_ERROR_ILLEGAL_FUNC;
    }
    
    /* 检查地址范围 */
    if (addr < slave->config.holding_start ||
        addr >= slave->config.holding_start + slave->config.holding_count) {
        return MB_ERROR_ILLEGAL_DATA_ADDR;
    }
    
    /* 写入寄存器 */
    ret = slave->write_reg_cb(addr, value);
    if (ret < 0) {
        return MB_ERROR_SLAVE_DEVICE_FAIL;
    }
    
    /* 构建响应帧 (回写) */
    tx_buf[0] = slave->config.slave_id;
    tx_buf[1] = MB_FUNC_WRITE_SINGLE_REG;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    tx_buf[4] = (value >> 8) & 0xFF;
    tx_buf[5] = value & 0xFF;
    
    /* 计算 CRC */
    uint16_t crc = nano_mb_crc16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;
    tx_buf[7] = (crc >> 8) & 0xFF;

    /* 发送响应 */
    if (slave->send_cb) {
        slave->send_cb(tx_buf, 8);
    }

    slave->request_count++;
    return NANO_MB_OK;
}

int nano_mb_slave_poll(mb_slave_t *slave, const uint8_t *data, uint16_t len)
{
    uint8_t slave_id;
    uint8_t function;
    uint16_t crc_rx, crc_calc;
    int error = MB_ERROR_NONE;
    
    if (!slave || !data || len < 8) {
        return NANO_MB_INVALID_PARAM;
    }
    
    /* 检查帧长度 */
    if (len > MB_MAX_ADU_SIZE) {
        return NANO_MB_INVALID_PARAM;
    }
    
    /* 验证 CRC */
    crc_rx = ((uint16_t)data[len - 1] << 8) | data[len - 2];
    crc_calc = nano_mb_crc16(data, len - 2);
    
    if (crc_rx != crc_calc) {
        xy_log_d("Modbus CRC error (rx=0x%04X, calc=0x%04X)\n", crc_rx, crc_calc);
        slave->error_count++;
        return NANO_MB_CRC_ERROR;
    }
    
    /* 解析帧 */
    slave_id = data[0];
    function = data[1];
    
    /* 检查从站 ID */
    if (slave_id != slave->config.slave_id) {
        return NANO_MB_OK;  /* 不是给我的 */
    }
    
    slave->last_rx_time = xy_os_tick_get();
    
    /* 处理功能码 */
    switch (function) {
        case MB_FUNC_READ_HOLDING: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t count = ((uint16_t)data[4] << 8) | data[5];
            error = mb_slave_handle_read_holding(slave, addr, count);
            break;
        }
        
        case MB_FUNC_WRITE_SINGLE_REG: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t value = ((uint16_t)data[4] << 8) | data[5];
            error = mb_slave_handle_write_reg(slave, addr, value);
            break;
        }
        
        default:
            xy_log_d("Modbus unknown function: 0x%02X\n", function);
            error = MB_ERROR_ILLEGAL_FUNC;
            break;
    }
    
    /* 发送错误响应 */
    if (error != NANO_MB_OK && error != MB_ERROR_NONE) {
        slave->tx_buf[0] = slave->config.slave_id;
        slave->tx_buf[1] = function | 0x80;
        slave->tx_buf[2] = error;
        
        uint16_t crc = nano_mb_crc16(slave->tx_buf, 3);
        slave->tx_buf[3] = crc & 0xFF;
        slave->tx_buf[4] = (crc >> 8) & 0xFF;
        
        /* 发送错误响应 */
        tx_buf[1] |= 0x80;
        tx_buf[2] = error;
        xy_modbus_send_response(slave, tx_buf, 3);
        /* mb_uart_send(slave->tx_buf, 5); */
        
        slave->error_count++;
    }
    
    return NANO_MB_OK;
}

/* ==================== 主站实现 ==================== */

int nano_mb_master_init(mb_master_t *master)
{
    if (!master) {
        return NANO_MB_INVALID_PARAM;
    }
    
    memset(master, 0, sizeof(*master));
    master->timeout_ms = MB_DEFAULT_TIMEOUT_MS;
    master->initialized = true;
    
    xy_log_i("Nano Modbus Master initialized\n");
    return NANO_MB_OK;
}

int nano_mb_master_deinit(mb_master_t *master)
{
    if (!master) {
        return NANO_MB_INVALID_PARAM;
    }
    
    master->initialized = false;
    return NANO_MB_OK;
}

int nano_mb_master_set_timeout(mb_master_t *master, uint32_t timeout_ms)
{
    if (!master || timeout_ms == 0) {
        return NANO_MB_INVALID_PARAM;
    }
    
    master->timeout_ms = timeout_ms;
    return NANO_MB_OK;
}

int nano_mb_master_read_holding(mb_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, uint16_t *data,
                                uint32_t timeout)
{
    uint8_t *tx_buf = master->tx_buf;
    uint16_t crc;
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return NANO_MB_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx_buf[0] = slave_id;
    tx_buf[1] = MB_FUNC_READ_HOLDING;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    tx_buf[4] = (count >> 8) & 0xFF;
    tx_buf[5] = count & 0xFF;
    
    crc = nano_mb_crc16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;
    tx_buf[7] = (crc >> 8) & 0xFF;

    /* 发送 Modbus 请求帧 */
    ret = xy_modbus_send(master, tx_buf, 8);
    if (ret != XY_MODBUS_OK) {
        master->error_count++;
        return NANO_MB_TIMEOUT;
    }

    /* 接收 Modbus 响应帧 */
    ret = xy_modbus_receive(master, rx_buf, sizeof(rx_buf), timeout);
    if (ret < 0) {
        master->error_count++;
        return NANO_MB_TIMEOUT;
    }

    /* 验证响应 CRC */
    uint16_t crc_rx = ((uint16_t)rx_buf[3] << 8) | rx_buf[4];
    uint16_t crc_calc = nano_mb_crc16(rx_buf, 5);
    if (crc_rx != crc_calc) {
        xy_log_e("Modbus CRC error (rx=0x%04X, calc=0x%04X)\n", crc_rx, crc_calc);
        return NANO_MB_ERROR;
    }

    master->request_count++;
    return NANO_MB_OK;
}

int nano_mb_master_write_reg(mb_master_t *master, uint8_t slave_id,
                             uint16_t addr, uint16_t value, uint32_t timeout)
{
    uint8_t *tx_buf = master->tx_buf;
    uint16_t crc;
    
    if (!master) {
        return NANO_MB_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx_buf[0] = slave_id;
    tx_buf[1] = MB_FUNC_WRITE_SINGLE_REG;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    tx_buf[4] = (value >> 8) & 0xFF;
    tx_buf[5] = value & 0xFF;
    
    crc = nano_mb_crc16(tx_buf, 6);
    tx_buf[6] = crc & 0xFF;
    tx_buf[7] = (crc >> 8) & 0xFF;
    
    /* 发送请求并等待响应 */
    ret = xy_modbus_send_request(master, tx_buf, 8);
    if (ret != XY_MODBUS_OK) {
        return ret;
    }
    
    ret = xy_modbus_receive_response(master, rx_buf, sizeof(rx_buf), timeout);
    return ret;
    
    master->request_count++;
    return NANO_MB_OK;
}

int nano_mb_master_read_coils(mb_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint8_t *data,
                              uint32_t timeout)
{
    /* 实现读线圈功能 */
    /* 类似读保持寄存器的实现 */
    return XY_MODBUS_OK;
    (void)master;
    (void)slave_id;
    (void)addr;
    (void)count;
    (void)data;
    (void)timeout;
    
    return NANO_MB_OK;
}
