/**
 * @file mb_tiny.c
 * @brief Nano Modbus Tiny Implementation - RTU Only, Minimal Footprint
 * @version 1.0.0
 * @date 2026-03-31
 */

#include "mb_tiny.h"
#include <string.h>

/* ==================== CRC16 ==================== */

static const uint16_t crc16_table[256] = {
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

uint16_t mb_tiny_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

/* ==================== 从站实现 ==================== */

/* 发送回调 (NULL by default) */
static mb_tiny_send_cb_t g_slave_send_cb = NULL;

int mb_tiny_slave_init(mb_tiny_slave_t *slave, uint8_t slave_id)
{
    if (!slave) {
        return MB_TINY_INVALID_PARAM;
    }
    
    memset(slave, 0, sizeof(*slave));
    slave->slave_id = slave_id;
    slave->initialized = true;
    
    return MB_TINY_OK;
}

int mb_tiny_slave_config_holding(mb_tiny_slave_t *slave, uint16_t *data,
                                  uint16_t start_addr, uint16_t count)
{
    if (!slave || !data || count == 0) {
        return MB_TINY_INVALID_PARAM;
    }
    
    slave->holding.data = data;
    slave->holding.start_addr = start_addr;
    slave->holding.count = count;
    
    return MB_TINY_OK;
}

int mb_tiny_slave_config_coils(mb_tiny_slave_t *slave, uint8_t *data,
                                uint16_t start_addr, uint16_t count)
{
    if (!slave || !data || count == 0) {
        return MB_TINY_INVALID_PARAM;
    }
    
    slave->coils.data = data;
    slave->coils.start_addr = start_addr;
    slave->coils.count = count;
    
    return MB_TINY_OK;
}

void mb_tiny_slave_set_send(mb_tiny_slave_t *slave, mb_tiny_send_cb_t send_cb)
{
    if (slave) {
        g_slave_send_cb = send_cb;
    }
}

static void mb_tiny_slave_send(mb_tiny_slave_t *slave, const uint8_t *data, uint16_t len)
{
    if (g_slave_send_cb) {
        g_slave_send_cb(data, len);
    }
    (void)slave;
}

static void mb_tiny_slave_send_error(mb_tiny_slave_t *slave, uint8_t func, uint8_t err)
{
    uint8_t tx[5];
    tx[0] = slave->slave_id;
    tx[1] = func | 0x80;
    tx[2] = err;
    uint16_t crc = mb_tiny_crc16(tx, 3);
    tx[3] = crc & 0xFF;
    tx[4] = (crc >> 8) & 0xFF;
    mb_tiny_slave_send(slave, tx, 5);
}

/**
 * @brief 检查地址是否在寄存器范围内
 */
static int mb_tiny_check_addr(uint16_t addr, uint16_t start, uint16_t count)
{
    if (addr < start || addr >= start + count) {
        return MB_ERR_ILLEGAL_DATA_ADDR;
    }
    return 0;
}

int mb_tiny_slave_handle(mb_tiny_slave_t *slave, const uint8_t *data, uint16_t len)
{
    uint8_t slave_id, func;
    uint16_t crc_rx, crc_calc;
    uint8_t tx_buf[MB_TINY_MAX_ADU_SIZE];
    int error = 0;
    
    if (!slave || !data || len < 5) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 验证 CRC */
    crc_rx = ((uint16_t)data[len - 1] << 8) | data[len - 2];
    crc_calc = mb_tiny_crc16(data, len - 2);
    if (crc_rx != crc_calc) {
        slave->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    /* 解析帧头 */
    slave_id = data[0];
    func = data[1];
    
    /* 检查从站 ID */
    if (slave_id != slave->slave_id) {
        return MB_TINY_OK; /* 不是给我的 */
    }
    
    /* 处理功能码 */
    switch (func) {
        /* 0x03: 读保持寄存器 */
        case MB_FUNC_READ_HOLDING: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t count = ((uint16_t)data[4] << 8) | data[5];
            
            /* 检查参数 */
            if (count == 0 || count > 125) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            /* 检查地址范围 */
            error = mb_tiny_check_addr(addr, slave->holding.start_addr, 
                                        slave->holding.count);
            if (error != 0) break;
            if (addr + count > slave->holding.start_addr + slave->holding.count) {
                error = MB_ERR_ILLEGAL_DATA_ADDR;
                break;
            }
            
            /* 构建响应 */
            tx_buf[0] = slave->slave_id;
            tx_buf[1] = func;
            tx_buf[2] = count * 2; /* 字节数 */
            
            /* 复制寄存器数据 (高字节先) */
            uint16_t *reg = &slave->holding.data[addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                tx_buf[3 + i * 2] = (reg[i] >> 8) & 0xFF;
                tx_buf[4 + i * 2] = reg[i] & 0xFF;
            }
            
            /* CRC */
            uint16_t tx_len = 3 + count * 2;
            crc_calc = mb_tiny_crc16(tx_buf, tx_len);
            tx_buf[tx_len] = crc_calc & 0xFF;
            tx_buf[tx_len + 1] = (crc_calc >> 8) & 0xFF;
            
            mb_tiny_slave_send(slave, tx_buf, tx_len + 2);
            break;
        }
        
        /* 0x06: 写单个寄存器 */
        case MB_FUNC_WRITE_SINGLE_REG: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t value = ((uint16_t)data[4] << 8) | data[5];
            
            /* 检查地址范围 */
            error = mb_tiny_check_addr(addr, slave->holding.start_addr,
                                        slave->holding.count);
            if (error != 0) break;
            
            /* 写入寄存器 */
            slave->holding.data[addr - slave->holding.start_addr] = value;
            
            /* 响应 (回显) */
            memcpy(tx_buf, data, 6);
            crc_calc = mb_tiny_crc16(tx_buf, 6);
            tx_buf[6] = crc_calc & 0xFF;
            tx_buf[7] = (crc_calc >> 8) & 0xFF;
            
            mb_tiny_slave_send(slave, tx_buf, 8);
            break;
        }
        
        /* 0x10: 写多个寄存器 */
        case MB_FUNC_WRITE_MULTIPLE_REGS: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t count = ((uint16_t)data[4] << 8) | data[5];
            uint8_t byte_count = data[6];
            
            /* 检查参数 */
            if (byte_count != count * 2) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            /* 检查地址范围 */
            error = mb_tiny_check_addr(addr, slave->holding.start_addr,
                                        slave->holding.count);
            if (error != 0) break;
            if (addr + count > slave->holding.start_addr + slave->holding.count) {
                error = MB_ERR_ILLEGAL_DATA_ADDR;
                break;
            }
            
            /* 写入寄存器 */
            uint16_t *reg = &slave->holding.data[addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                reg[i] = ((uint16_t)data[7 + i * 2] << 8) | data[8 + i * 2];
            }
            
            /* 响应 */
            tx_buf[0] = slave->slave_id;
            tx_buf[1] = func;
            tx_buf[2] = (addr >> 8) & 0xFF;
            tx_buf[3] = addr & 0xFF;
            tx_buf[4] = (count >> 8) & 0xFF;
            tx_buf[5] = count & 0xFF;
            
            crc_calc = mb_tiny_crc16(tx_buf, 6);
            tx_buf[6] = crc_calc & 0xFF;
            tx_buf[7] = (crc_calc >> 8) & 0xFF;
            
            mb_tiny_slave_send(slave, tx_buf, 8);
            break;
        }
        
        /* 0x01: 读线圈 */
        case MB_FUNC_READ_COILS: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t count = ((uint16_t)data[4] << 8) | data[5];
            
            if (count == 0 || count > 2000) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_tiny_check_addr(addr, slave->coils.start_addr,
                                        slave->coils.count);
            if (error != 0) break;
            
            /* 构建响应 */
            tx_buf[0] = slave->slave_id;
            tx_buf[1] = func;
            tx_buf[2] = (count + 7) / 8; /* 字节数 */
            
            /* 复制线圈数据 */
            uint8_t *coil = &slave->coils.data[addr - slave->coils.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                if (coil[i / 8] & (1 << (i % 8))) {
                    tx_buf[3 + i / 8] |= (1 << (i % 8));
                }
            }
            
            uint16_t tx_len = 3 + tx_buf[2];
            crc_calc = mb_tiny_crc16(tx_buf, tx_len);
            tx_buf[tx_len] = crc_calc & 0xFF;
            tx_buf[tx_len + 1] = (crc_calc >> 8) & 0xFF;
            
            mb_tiny_slave_send(slave, tx_buf, tx_len + 2);
            break;
        }
        
        /* 0x05: 写单个线圈 */
        case MB_FUNC_WRITE_SINGLE_COIL: {
            uint16_t addr = ((uint16_t)data[2] << 8) | data[3];
            uint16_t value = ((uint16_t)data[4] << 8) | data[5];
            
            error = mb_tiny_check_addr(addr, slave->coils.start_addr,
                                        slave->coils.count);
            if (error != 0) break;
            
            /* 写入线圈 (0xFF00 = ON, 0x0000 = OFF) */
            uint8_t *coil = &slave->coils.data[addr - slave->coils.start_addr];
            if (value == 0xFF00) {
                *coil |= (1 << (addr % 8));
            } else {
                *coil &= ~(1 << (addr % 8));
            }
            
            /* 响应 (回显) */
            memcpy(tx_buf, data, 6);
            crc_calc = mb_tiny_crc16(tx_buf, 6);
            tx_buf[6] = crc_calc & 0xFF;
            tx_buf[7] = (crc_calc >> 8) & 0xFF;
            
            mb_tiny_slave_send(slave, tx_buf, 8);
            break;
        }
        
        default:
            error = MB_ERR_ILLEGAL_FUNC;
            break;
    }
    
    if (error != 0) {
        mb_tiny_slave_send_error(slave, func, (uint8_t)error);
        slave->error_count++;
    } else {
        slave->request_count++;
    }
    
    return MB_TINY_OK;
}

/* ==================== 主站实现 ==================== */

static mb_tiny_send_cb_t g_master_send_cb = NULL;
static mb_tiny_recv_cb_t g_master_recv_cb = NULL;

int mb_tiny_master_init(mb_tiny_master_t *master)
{
    if (!master) {
        return MB_TINY_INVALID_PARAM;
    }
    
    memset(master, 0, sizeof(*master));
    master->timeout_ms = MB_TINY_TIMEOUT_MS;
    master->initialized = true;
    
    return MB_TINY_OK;
}

void mb_tiny_master_set_uart(mb_tiny_master_t *master, 
                              mb_tiny_send_cb_t send_cb, 
                              mb_tiny_recv_cb_t recv_cb)
{
    if (master) {
        g_master_send_cb = send_cb;
        g_master_recv_cb = recv_cb;
    }
}

void mb_tiny_master_set_timeout(mb_tiny_master_t *master, uint32_t timeout_ms)
{
    if (master) {
        master->timeout_ms = timeout_ms;
    }
}

static int mb_tiny_master_tx(mb_tiny_master_t *master, uint8_t *data, uint16_t len)
{
    if (g_master_send_cb) {
        return g_master_send_cb(data, len);
    }
    (void)master;
    return MB_TINY_ERROR;
}

static int mb_tiny_master_rx(mb_tiny_master_t *master, uint8_t *data, uint16_t len)
{
    if (g_master_recv_cb) {
        return g_master_recv_cb(data, len, master->timeout_ms);
    }
    return MB_TINY_ERROR;
}

int mb_tiny_master_read_holding(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data)
{
    uint8_t tx[8];
    uint8_t rx[MB_TINY_MAX_ADU_SIZE];
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx[0] = slave_id;
    tx[1] = MB_FUNC_READ_HOLDING;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = (count >> 8) & 0xFF;
    tx[5] = count & 0xFF;
    
    uint16_t crc = mb_tiny_crc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = (crc >> 8) & 0xFF;
    
    /* 发送 */
    ret = mb_tiny_master_tx(master, tx, 8);
    if (ret < 0) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 接收 */
    ret = mb_tiny_master_rx(master, rx, MB_TINY_MAX_ADU_SIZE);
    if (ret < 5) {
        master->error_count++;
        return MB_TINY_TIMEOUT;
    }
    
    /* 验证 CRC */
    crc = ((uint16_t)rx[ret - 1] << 8) | rx[ret - 2];
    if (crc != mb_tiny_crc16(rx, ret - 2)) {
        master->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    /* 检查功能码 */
    if (rx[1] & 0x80) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 复制数据 */
    uint8_t byte_count = rx[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        data[i] = ((uint16_t)rx[3 + i * 2] << 8) | rx[4 + i * 2];
    }
    
    master->request_count++;
    return MB_TINY_OK;
}

int mb_tiny_master_write_reg(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value)
{
    uint8_t tx[8];
    uint8_t rx[8];
    int ret;
    
    if (!master) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx[0] = slave_id;
    tx[1] = MB_FUNC_WRITE_SINGLE_REG;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = (value >> 8) & 0xFF;
    tx[5] = value & 0xFF;
    
    uint16_t crc = mb_tiny_crc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = (crc >> 8) & 0xFF;
    
    /* 发送 */
    ret = mb_tiny_master_tx(master, tx, 8);
    if (ret < 0) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 接收 */
    ret = mb_tiny_master_rx(master, rx, 8);
    if (ret < 5) {
        master->error_count++;
        return MB_TINY_TIMEOUT;
    }
    
    /* 验证 CRC */
    crc = ((uint16_t)rx[ret - 1] << 8) | rx[ret - 2];
    if (crc != mb_tiny_crc16(rx, ret - 2)) {
        master->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    if (rx[1] & 0x80) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    master->request_count++;
    return MB_TINY_OK;
}

int mb_tiny_master_write_regs(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, const uint16_t *data)
{
    uint8_t tx[MB_TINY_MAX_ADU_SIZE];
    uint8_t rx[8];
    int ret;
    uint16_t i;
    
    if (!master || !data || count == 0 || count > 123) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx[0] = slave_id;
    tx[1] = MB_FUNC_WRITE_MULTIPLE_REGS;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = (count >> 8) & 0xFF;
    tx[5] = count & 0xFF;
    tx[6] = count * 2; /* 字节数 */
    
    for (i = 0; i < count; i++) {
        tx[7 + i * 2] = (data[i] >> 8) & 0xFF;
        tx[8 + i * 2] = data[i] & 0xFF;
    }
    
    uint16_t tx_len = 7 + count * 2;
    uint16_t crc = mb_tiny_crc16(tx, tx_len);
    tx[tx_len] = crc & 0xFF;
    tx[tx_len + 1] = (crc >> 8) & 0xFF;
    
    /* 发送 */
    ret = mb_tiny_master_tx(master, tx, tx_len + 2);
    if (ret < 0) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 接收 */
    ret = mb_tiny_master_rx(master, rx, 8);
    if (ret < 5) {
        master->error_count++;
        return MB_TINY_TIMEOUT;
    }
    
    /* 验证 CRC */
    crc = ((uint16_t)rx[ret - 1] << 8) | rx[ret - 2];
    if (crc != mb_tiny_crc16(rx, ret - 2)) {
        master->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    if (rx[1] & 0x80) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    master->request_count++;
    return MB_TINY_OK;
}

int mb_tiny_master_read_coils(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, uint8_t *data)
{
    uint8_t tx[8];
    uint8_t rx[MB_TINY_MAX_ADU_SIZE];
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 构建请求帧 */
    tx[0] = slave_id;
    tx[1] = MB_FUNC_READ_COILS;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = (count >> 8) & 0xFF;
    tx[5] = count & 0xFF;
    
    uint16_t crc = mb_tiny_crc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = (crc >> 8) & 0xFF;
    
    /* 发送 */
    ret = mb_tiny_master_tx(master, tx, 8);
    if (ret < 0) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 接收 */
    ret = mb_tiny_master_rx(master, rx, MB_TINY_MAX_ADU_SIZE);
    if (ret < 5) {
        master->error_count++;
        return MB_TINY_TIMEOUT;
    }
    
    /* 验证 CRC */
    crc = ((uint16_t)rx[ret - 1] << 8) | rx[ret - 2];
    if (crc != mb_tiny_crc16(rx, ret - 2)) {
        master->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    if (rx[1] & 0x80) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 复制数据 */
    uint8_t byte_count = rx[2];
    memcpy(data, &rx[3], byte_count);
    
    master->request_count++;
    return MB_TINY_OK;
}

int mb_tiny_master_write_coil(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value)
{
    uint8_t tx[8];
    uint8_t rx[8];
    int ret;
    
    if (!master) {
        return MB_TINY_INVALID_PARAM;
    }
    
    /* 构建请求帧: 0xFF00 = ON, 0x0000 = OFF */
    tx[0] = slave_id;
    tx[1] = MB_FUNC_WRITE_SINGLE_COIL;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    tx[4] = (value != 0) ? 0xFF : 0x00;
    tx[5] = 0x00;
    
    uint16_t crc = mb_tiny_crc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = (crc >> 8) & 0xFF;
    
    /* 发送 */
    ret = mb_tiny_master_tx(master, tx, 8);
    if (ret < 0) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    /* 接收 */
    ret = mb_tiny_master_rx(master, rx, 8);
    if (ret < 5) {
        master->error_count++;
        return MB_TINY_TIMEOUT;
    }
    
    /* 验证 CRC */
    crc = ((uint16_t)rx[ret - 1] << 8) | rx[ret - 2];
    if (crc != mb_tiny_crc16(rx, ret - 2)) {
        master->error_count++;
        return MB_TINY_CRC_ERROR;
    }
    
    if (rx[1] & 0x80) {
        master->error_count++;
        return MB_TINY_ERROR;
    }
    
    master->request_count++;
    return MB_TINY_OK;
}
