/**
 * @file mb_full.c
 * @brief Nano Modbus Full Implementation - RTU + TCP + ASCII
 * @version 1.0.0
 * @date 2026-03-31
 */

#include "mb_full.h"
#include <string.h>

/* ==================== CRC16 表 ==================== */

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

/* ==================== 全局回调 ==================== */

static mb_full_send_cb_t g_slave_send_cb = NULL;
static mb_full_recv_cb_t g_slave_recv_cb = NULL;
static mb_full_tcp_send_cb_t g_slave_tcp_send_cb = NULL;
static mb_full_tcp_recv_cb_t g_slave_tcp_recv_cb = NULL;

static mb_full_connect_cb_t g_master_connect_cb = NULL;
static mb_full_send_cb_t g_master_send_cb = NULL;
static mb_full_recv_cb_t g_master_recv_cb = NULL;
static mb_full_tcp_send_cb_t g_master_tcp_send_cb = NULL;
static mb_full_tcp_recv_cb_t g_master_tcp_recv_cb = NULL;
static mb_full_disconnect_cb_t g_master_disconnect_cb = NULL;

/* ==================== 工具函数 ==================== */

uint16_t mb_full_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }
    return crc;
}

uint8_t mb_full_lrc(const uint8_t *data, uint16_t len)
{
    int8_t lrc = 0;
    for (uint16_t i = 0; i < len; i++) {
        lrc = (int8_t)(lrc + data[i]);
    }
    return (uint8_t)(-(int16_t)lrc);
}

int mb_full_ascii_decode(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t j = 0;
    for (uint16_t i = 0; i + 1 < src_len; i += 2) {
        uint8_t hi = src[i];
        uint8_t lo = src[i + 1];
        
        /* 转换 Hex 字符 */
        if (hi >= '0' && hi <= '9') hi -= '0';
        else if (hi >= 'A' && hi <= 'F') hi -= 'A' - 10;
        else if (hi >= 'a' && hi <= 'f') hi -= 'a' - 10;
        else continue;
        
        if (lo >= '0' && lo <= '9') lo -= '0';
        else if (lo >= 'A' && lo <= 'F') lo -= 'A' - 10;
        else if (lo >= 'a' && lo <= 'f') lo -= 'a' - 10;
        else continue;
        
        dst[j++] = (hi << 4) | lo;
    }
    return j;
}

int mb_full_ascii_encode(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t j = 0;
    dst[j++] = ':';  /* 起始符 */
    
    for (uint16_t i = 0; i < src_len; i++) {
        dst[j++] = hex[(src[i] >> 4) & 0x0F];
        dst[j++] = hex[src[i] & 0x0F];
    }
    
    /* LRC */
    uint8_t lrc = mb_full_lrc(src, src_len);
    dst[j++] = hex[(lrc >> 4) & 0x0F];
    dst[j++] = hex[lrc & 0x0F];
    
    /* 结束符 */
    dst[j++] = '\r';
    dst[j++] = '\n';
    
    return j;
}

const char* mb_full_version(void)
{
    return "1.0.0";
}

const char* mb_full_error_string(int error)
{
    switch (error) {
        case MB_FULL_OK: return "OK";
        case MB_FULL_ERROR: return "Error";
        case MB_FULL_INVALID_PARAM: return "Invalid Parameter";
        case MB_FULL_TIMEOUT: return "Timeout";
        case MB_FULL_CRC_ERROR: return "CRC Error";
        case MB_FULL_LRC_ERROR: return "LRC Error";
        case MB_FULL_NOT_INITIALIZED: return "Not Initialized";
        case MB_FULL_NO_MEMORY: return "No Memory";
        default: return "Unknown";
    }
}

/* ==================== 地址检查 ==================== */

static int mb_check_bit_addr(uint16_t addr, uint16_t start, uint16_t count)
{
    if (addr < start || addr >= start + count) {
        return MB_ERR_ILLEGAL_DATA_ADDRESS;
    }
    return 0;
}

static int mb_check_reg_addr(uint16_t addr, uint16_t start, uint16_t count)
{
    if (addr < start || addr >= start + count) {
        return MB_ERR_ILLEGAL_DATA_ADDRESS;
    }
    return 0;
}

/* ==================== 从站实现 ==================== */

int mb_full_slave_init(mb_full_slave_t *slave, uint8_t slave_id, mb_mode_t mode)
{
    if (!slave) {
        return MB_FULL_INVALID_PARAM;
    }
    
    memset(slave, 0, sizeof(*slave));
    slave->slave_id = slave_id;
    slave->mode = mode;
    slave->initialized = true;
    
    return MB_FULL_OK;
}

int mb_full_slave_config_coils(mb_full_slave_t *slave, uint8_t *data,
                                uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->coils.data = data;
    slave->coils.start_addr = start_addr;
    slave->coils.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_discrete(mb_full_slave_t *slave, uint8_t *data,
                                    uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->discrete.data = data;
    slave->discrete.start_addr = start_addr;
    slave->discrete.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_holding(mb_full_slave_t *slave, uint16_t *data,
                                   uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->holding.data = data;
    slave->holding.start_addr = start_addr;
    slave->holding.count = count;
    return MB_FULL_OK;
}

int mb_full_slave_config_input(mb_full_slave_t *slave, uint16_t *data,
                                uint16_t start_addr, uint16_t count)
{
    if (!slave || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    slave->input.data = data;
    slave->input.start_addr = start_addr;
    slave->input.count = count;
    return MB_FULL_OK;
}

void mb_full_slave_set_uart(mb_full_slave_t *slave, mb_full_send_cb_t send_cb)
{
    (void)slave;
    g_slave_send_cb = send_cb;
}

void mb_full_slave_set_tcp(mb_full_slave_t *slave, mb_full_tcp_send_cb_t send_cb,
                            mb_full_tcp_recv_cb_t recv_cb)
{
    (void)slave;
    g_slave_tcp_send_cb = send_cb;
    g_slave_tcp_recv_cb = recv_cb;
}

static void mb_full_slave_send(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    if (slave->mode == MB_MODE_TCP) {
        if (g_slave_tcp_send_cb) g_slave_tcp_send_cb(data, len);
    } else {
        if (g_slave_send_cb) g_slave_send_cb(data, len);
    }
    (void)slave;
}

static void mb_full_slave_send_error(mb_full_slave_t *slave, uint8_t func, uint8_t err)
{
    uint8_t tx[5];
    tx[0] = slave->slave_id;
    tx[1] = func | 0x80;
    tx[2] = err;
    uint16_t crc = mb_full_crc16(tx, 3);
    tx[3] = crc & 0xFF;
    tx[4] = (crc >> 8) & 0xFF;
    mb_full_slave_send(slave, tx, 5);
}

/**
 * @brief 从站处理帧
 */
static int mb_full_slave_process(mb_full_slave_t *slave, uint8_t func,
                                  const uint8_t *req, uint16_t req_len,
                                  uint8_t *rsp, uint16_t *rsp_len)
{
    int error = 0;
    uint8_t *tx = slave->tx_buf;
    
    switch (func) {
        /* 0x01: 读线圈 */
        case MB_FUNC_READ_COILS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            
            if (count == 0 || count > 2000) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_bit_addr(addr, slave->coils.start_addr, slave->coils.count);
            if (error != 0) break;
            if (addr + count > slave->coils.start_addr + slave->coils.count) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }
            
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = (count + 7) / 8;
            
            uint8_t *coil = &slave->coils.data[addr - slave->coils.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                if (coil[i / 8] & (1 << (i % 8))) {
                    tx[3 + i / 8] |= (1 << (i % 8));
                }
            }
            
            uint16_t len = 3 + tx[2];
            uint16_t crc = mb_full_crc16(tx, len);
            tx[len] = crc & 0xFF;
            tx[len + 1] = (crc >> 8) & 0xFF;
            *rsp_len = len + 2;
            return 0;
        }
        
        /* 0x02: 读离散输入 */
        case MB_FUNC_READ_DISCRETE_INPUTS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            
            if (count == 0 || count > 2000) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_bit_addr(addr, slave->discrete.start_addr, slave->discrete.count);
            if (error != 0) break;
            
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = (count + 7) / 8;
            
            uint8_t *disc = &slave->discrete.data[addr - slave->discrete.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                if (disc[i / 8] & (1 << (i % 8))) {
                    tx[3 + i / 8] |= (1 << (i % 8));
                }
            }
            
            uint16_t len = 3 + tx[2];
            uint16_t crc = mb_full_crc16(tx, len);
            tx[len] = crc & 0xFF;
            tx[len + 1] = (crc >> 8) & 0xFF;
            *rsp_len = len + 2;
            return 0;
        }
        
        /* 0x03: 读保持寄存器 */
        case MB_FUNC_READ_HOLDING_REGISTERS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            
            if (count == 0 || count > 125) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_reg_addr(addr, slave->holding.start_addr, slave->holding.count);
            if (error != 0) break;
            if (addr + count > slave->holding.start_addr + slave->holding.count) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }
            
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = count * 2;
            
            uint16_t *reg = &slave->holding.data[addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                tx[3 + i * 2] = (reg[i] >> 8) & 0xFF;
                tx[4 + i * 2] = reg[i] & 0xFF;
            }
            
            uint16_t len = 3 + count * 2;
            uint16_t crc = mb_full_crc16(tx, len);
            tx[len] = crc & 0xFF;
            tx[len + 1] = (crc >> 8) & 0xFF;
            *rsp_len = len + 2;
            return 0;
        }
        
        /* 0x04: 读输入寄存器 */
        case MB_FUNC_READ_INPUT_REGISTERS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            
            if (count == 0 || count > 125) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_reg_addr(addr, slave->input.start_addr, slave->input.count);
            if (error != 0) break;
            
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = count * 2;
            
            uint16_t *reg = &slave->input.data[addr - slave->input.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                tx[3 + i * 2] = (reg[i] >> 8) & 0xFF;
                tx[4 + i * 2] = reg[i] & 0xFF;
            }
            
            uint16_t len = 3 + count * 2;
            uint16_t crc = mb_full_crc16(tx, len);
            tx[len] = crc & 0xFF;
            tx[len + 1] = (crc >> 8) & 0xFF;
            *rsp_len = len + 2;
            return 0;
        }
        
        /* 0x05: 写单个线圈 */
        case MB_FUNC_WRITE_SINGLE_COIL: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t value = ((uint16_t)req[4] << 8) | req[5];
            
            error = mb_check_bit_addr(addr, slave->coils.start_addr, slave->coils.count);
            if (error != 0) break;
            
            uint8_t *coil = &slave->coils.data[addr - slave->coils.start_addr];
            if (value == 0xFF00) {
                *coil |= (1 << (addr % 8));
            } else if (value == 0x0000) {
                *coil &= ~(1 << (addr % 8));
            } else {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            /* 响应 (回显) */
            memcpy(tx, req, 6);
            uint16_t crc = mb_full_crc16(tx, 6);
            tx[6] = crc & 0xFF;
            tx[7] = (crc >> 8) & 0xFF;
            *rsp_len = 8;
            return 0;
        }
        
        /* 0x06: 写单个寄存器 */
        case MB_FUNC_WRITE_SINGLE_REGISTER: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t value = ((uint16_t)req[4] << 8) | req[5];
            
            error = mb_check_reg_addr(addr, slave->holding.start_addr, slave->holding.count);
            if (error != 0) break;
            
            slave->holding.data[addr - slave->holding.start_addr] = value;
            
            /* 响应 (回显) */
            memcpy(tx, req, 6);
            uint16_t crc = mb_full_crc16(tx, 6);
            tx[6] = crc & 0xFF;
            tx[7] = (crc >> 8) & 0xFF;
            *rsp_len = 8;
            return 0;
        }
        
        /* 0x0F: 写多个线圈 */
        case MB_FUNC_WRITE_MULTIPLE_COILS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            uint8_t byte_count = req[6];
            
            if (byte_count != (count + 7) / 8) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_bit_addr(addr, slave->coils.start_addr, slave->coils.count);
            if (error != 0) break;
            
            uint8_t *coil = &slave->coils.data[addr - slave->coils.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                if (req[7 + i / 8] & (1 << (i % 8))) {
                    coil[i / 8] |= (1 << (i % 8));
                } else {
                    coil[i / 8] &= ~(1 << (i % 8));
                }
            }
            
            /* 响应 */
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = req[2];
            tx[3] = req[3];
            tx[4] = req[4];
            tx[5] = req[5];
            
            uint16_t crc = mb_full_crc16(tx, 6);
            tx[6] = crc & 0xFF;
            tx[7] = (crc >> 8) & 0xFF;
            *rsp_len = 8;
            return 0;
        }
        
        /* 0x10: 写多个寄存器 */
        case MB_FUNC_WRITE_MULTIPLE_REGISTERS: {
            uint16_t addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t count = ((uint16_t)req[4] << 8) | req[5];
            uint8_t byte_count = req[6];
            
            if (byte_count != count * 2) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            error = mb_check_reg_addr(addr, slave->holding.start_addr, slave->holding.count);
            if (error != 0) break;
            if (addr + count > slave->holding.start_addr + slave->holding.count) {
                error = MB_ERR_ILLEGAL_DATA_ADDRESS;
                break;
            }
            
            uint16_t *reg = &slave->holding.data[addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < count; i++) {
                reg[i] = ((uint16_t)req[7 + i * 2] << 8) | req[8 + i * 2];
            }
            
            /* 响应 */
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = req[2];
            tx[3] = req[3];
            tx[4] = req[4];
            tx[5] = req[5];
            
            uint16_t crc = mb_full_crc16(tx, 6);
            tx[6] = crc & 0xFF;
            tx[7] = (crc >> 8) & 0xFF;
            *rsp_len = 8;
            return 0;
        }
        
        /* 0x17: 读写多个寄存器 */
        case MB_FUNC_READ_WRITE_MULTIPLE: {
            uint16_t read_addr = ((uint16_t)req[2] << 8) | req[3];
            uint16_t read_count = ((uint16_t)req[4] << 8) | req[5];
            uint16_t write_addr = ((uint16_t)req[6] << 8) | req[7];
            uint16_t write_count = ((uint16_t)req[8] << 8) | req[9];
            uint8_t write_byte_count = req[10];
            
            if (write_byte_count != write_count * 2) {
                error = MB_ERR_ILLEGAL_DATA_VALUE;
                break;
            }
            
            /* 读 */
            error = mb_check_reg_addr(read_addr, slave->holding.start_addr, slave->holding.count);
            if (error != 0) break;
            
            /* 写 */
            error = mb_check_reg_addr(write_addr, slave->holding.start_addr, slave->holding.count);
            if (error != 0) break;
            
            /* 读响应 */
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = read_count * 2;
            
            uint16_t *read_reg = &slave->holding.data[read_addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < read_count; i++) {
                tx[3 + i * 2] = (read_reg[i] >> 8) & 0xFF;
                tx[4 + i * 2] = read_reg[i] & 0xFF;
            }
            
            /* 写入数据 */
            uint16_t *write_reg = &slave->holding.data[write_addr - slave->holding.start_addr];
            for (uint16_t i = 0; i < write_count; i++) {
                write_reg[i] = ((uint16_t)req[11 + i * 2] << 8) | req[12 + i * 2];
            }
            
            uint16_t len = 3 + read_count * 2;
            uint16_t crc = mb_full_crc16(tx, len);
            tx[len] = crc & 0xFF;
            tx[len + 1] = (crc >> 8) & 0xFF;
            *rsp_len = len + 2;
            return 0;
        }
        
        /* 0x2E: 读设备 ID */
        case MB_FUNC_READ_DEVICE_ID: {
            uint8_t object_id = req[2];
            
            tx[0] = slave->slave_id;
            tx[1] = func;
            tx[2] = 0x01;  /* 符合性级别 */
            tx[3] = 0x00;  /* 后续对象数 */
            tx[4] = object_id;
            
            switch (object_id) {
                case MB_OBJ_BASIC:
                    tx[5] = 0x00;  /* 对象 ID */
                    tx[6] = 0x01;  /* 长度 */
                    tx[7] = slave->device_id.device_id;
                    break;
                default:
                    error = MB_ERR_ILLEGAL_DATA_VALUE;
                    break;
            }
            
            if (error == 0) {
                uint16_t len = 8;
                uint16_t crc = mb_full_crc16(tx, len);
                tx[len] = crc & 0xFF;
                tx[len + 1] = (crc >> 8) & 0xFF;
                *rsp_len = len + 2;
            }
            break;
        }
        
        default:
            error = MB_ERR_ILLEGAL_FUNCTION;
            break;
    }
    
    return error;
}

int mb_full_slave_handle(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    uint8_t slave_id, func;
    uint16_t crc_rx, crc_calc;
    uint8_t rsp[MB_FULL_MAX_ADU_SIZE];
    uint16_t rsp_len = 0;
    int error;
    
    if (!slave || !data || len < 5) {
        return MB_FULL_INVALID_PARAM;
    }
    
    /* RTU: 验证 CRC */
    crc_rx = ((uint16_t)data[len - 1] << 8) | data[len - 2];
    crc_calc = mb_full_crc16(data, len - 2);
    if (crc_rx != crc_calc) {
        slave->crc_error_count++;
        slave->error_count++;
        return MB_FULL_CRC_ERROR;
    }
    
    slave_id = data[0];
    func = data[1];
    
    /* 检查从站 ID */
    if (slave_id != slave->slave_id) {
        return MB_FULL_OK;
    }
    
    /* 处理请求 */
    error = mb_full_slave_process(slave, func, data + 1, len - 3, rsp, &rsp_len);
    
    if (error != 0) {
        /* 发送错误响应 */
        rsp[0] = slave->slave_id;
        rsp[1] = func | 0x80;
        rsp[2] = (uint8_t)error;
        crc_calc = mb_full_crc16(rsp, 3);
        rsp[3] = crc_calc & 0xFF;
        rsp[4] = (crc_calc >> 8) & 0xFF;
        rsp_len = 5;
        slave->error_count++;
    } else {
        slave->request_count++;
    }
    
    mb_full_slave_send(slave, rsp, rsp_len);
    return MB_FULL_OK;
}

int mb_full_slave_handle_tcp(mb_full_slave_t *slave, const uint8_t *data, uint16_t len)
{
    mb_tcp_header_t *hdr = (mb_tcp_header_t *)data;
    uint8_t slave_id, func;
    uint16_t crc_calc;
    uint8_t rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len = 0;
    int error;
    
    if (!slave || !data || len < 9) {
        return MB_FULL_INVALID_PARAM;
    }
    
    /* 检查协议 ID */
    if (hdr->protocol_id != 0) {
        return MB_FULL_INVALID_PARAM;
    }
    
    slave_id = hdr->unit_id;
    func = data[7];
    
    /* 检查从站 ID */
    if (slave_id != slave->slave_id) {
        return MB_FULL_OK;
    }
    
    /* 处理请求 */
    error = mb_full_slave_process(slave, func, data + 7, len - 9, rsp + 7, &rsp_len);
    
    /* 构建 TCP 响应 */
    if (error != 0) {
        rsp[0] = (hdr->transaction_id >> 8) & 0xFF;
        rsp[1] = hdr->transaction_id & 0xFF;
        rsp[2] = 0x00;
        rsp[3] = 0x00;
        rsp[4] = 0x00;
        rsp[5] = 3;  /* 长度 */
        rsp[6] = slave->slave_id;
        rsp[7] = func | 0x80;
        rsp[8] = (uint8_t)error;
        
        rsp_len = 9;
        slave->error_count++;
    } else {
        /* 添加 TCP 头 */
        rsp[0] = (hdr->transaction_id >> 8) & 0xFF;
        rsp[1] = hdr->transaction_id & 0xFF;
        rsp[2] = 0x00;
        rsp[3] = 0x00;
        
        uint16_t payload_len = rsp_len - 2;  /* 去掉 CRC */
        rsp[4] = (payload_len >> 8) & 0xFF;
        rsp[5] = payload_len & 0xFF;
        rsp[6] = slave->slave_id;
        
        rsp_len += 7;  /* 加上 TCP 头 */
        slave->request_count++;
    }
    
    mb_full_slave_send(slave, rsp, rsp_len);
    return MB_FULL_OK;
}

/* ==================== 主站实现 ==================== */

int mb_full_master_init(mb_full_master_t *master, mb_mode_t mode)
{
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    memset(master, 0, sizeof(*master));
    master->mode = mode;
    master->timeout_ms = MB_FULL_TIMEOUT_MS;
    master->initialized = true;
    
    return MB_FULL_OK;
}

void mb_full_master_set_timeout(mb_full_master_t *master, uint32_t timeout_ms)
{
    if (master) {
        master->timeout_ms = timeout_ms;
    }
}

void mb_full_master_set_uart(mb_full_master_t *master, 
                              mb_full_send_cb_t send_cb,
                              mb_full_recv_cb_t recv_cb)
{
    (void)master;
    g_master_send_cb = send_cb;
    g_master_recv_cb = recv_cb;
}

void mb_full_master_set_tcp(mb_full_master_t *master,
                             mb_full_connect_cb_t connect_cb,
                             mb_full_tcp_send_cb_t send_cb,
                             mb_full_tcp_recv_cb_t recv_cb,
                             mb_full_disconnect_cb_t disconnect_cb)
{
    (void)master;
    g_master_connect_cb = connect_cb;
    g_master_tcp_send_cb = send_cb;
    g_master_tcp_recv_cb = recv_cb;
    g_master_disconnect_cb = disconnect_cb;
}

int mb_full_master_connect(mb_full_master_t *master, const char *host, uint16_t port)
{
    (void)master;
    (void)host;
    (void)port;
    
    if (g_master_connect_cb) {
        return g_master_connect_cb(host, port);
    }
    return MB_FULL_ERROR;
}

int mb_full_master_disconnect(mb_full_master_t *master)
{
    (void)master;
    
    if (g_master_disconnect_cb) {
        return g_master_disconnect_cb();
    }
    return MB_FULL_ERROR;
}

static int mb_full_master_tx(mb_full_master_t *master, uint8_t *data, uint16_t len)
{
    (void)master;
    if (master->mode == MB_MODE_TCP) {
        if (g_master_tcp_send_cb) return g_master_tcp_send_cb(data, len);
    } else {
        if (g_master_send_cb) return g_master_send_cb(data, len);
    }
    return MB_FULL_ERROR;
}

static int mb_full_master_rx(mb_full_master_t *master, uint8_t *data, uint16_t len)
{
    (void)master;
    if (master->mode == MB_MODE_TCP) {
        if (g_master_tcp_recv_cb) return g_master_tcp_recv_cb(data, len, master->timeout_ms);
    } else {
        if (g_master_recv_cb) return g_master_recv_cb(data, len, master->timeout_ms);
    }
    return MB_FULL_ERROR;
}

/**
 * @brief 主站发送请求并接收响应
 */
static int mb_full_master_request(mb_full_master_t *master, uint8_t slave_id,
                                   const uint8_t *req, uint16_t req_len,
                                   uint8_t *rsp, uint16_t *rsp_len)
{
    uint8_t tx[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t tx_len = 0;
    int ret;
    
    /* 构建 TCP 头 (如果是 TCP 模式) */
    if (master->mode == MB_MODE_TCP) {
        mb_tcp_header_t *hdr = (mb_tcp_header_t *)tx;
        hdr->transaction_id = master->conn.transaction_id++;
        hdr->protocol_id = 0;
        hdr->length = req_len + 1;  /* + unit_id */
        hdr->unit_id = slave_id;
        
        memcpy(tx + 7, req, req_len);
        tx_len = 7 + req_len;
    } else {
        tx[0] = slave_id;
        memcpy(tx + 1, req, req_len);
        tx_len = 1 + req_len;
        
        /* 添加 CRC */
        uint16_t crc = mb_full_crc16(tx, tx_len);
        tx[tx_len++] = crc & 0xFF;
        tx[tx_len++] = (crc >> 8) & 0xFF;
    }
    
    /* 发送 */
    ret = mb_full_master_tx(master, tx, tx_len);
    if (ret < 0) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 接收 */
    ret = mb_full_master_rx(master, rsp, MB_FULL_MAX_TCP_ADU_SIZE);
    if (ret < 5) {
        master->error_count++;
        return MB_FULL_TIMEOUT;
    }
    
    *rsp_len = ret;
    master->request_count++;
    return MB_FULL_OK;
}

int mb_full_master_read_coils(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint8_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_COILS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    /* 检查错误 */
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 复制数据 */
    uint8_t byte_count = rsp[2];
    memcpy(data, &rsp[3], byte_count);
    
    return MB_FULL_OK;
}

int mb_full_master_read_discrete(mb_full_master_t *master, uint8_t slave_id,
                                  uint16_t addr, uint16_t count, uint8_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_DISCRETE_INPUTS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    memcpy(data, &rsp[3], byte_count);
    
    return MB_FULL_OK;
}

int mb_full_master_read_holding(mb_full_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_HOLDING_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_input(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint16_t *data)
{
    uint8_t req[6], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 125) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_INPUT_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_coil(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value)
{
    uint8_t req[6], rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_SINGLE_COIL;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (value != 0) ? 0xFF : 0x00;
    req[4] = 0x00;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_register(mb_full_master_t *master, uint8_t slave_id,
                                   uint16_t addr, uint16_t value)
{
    uint8_t req[6], rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_SINGLE_REGISTER;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (value >> 8) & 0xFF;
    req[4] = value & 0xFF;
    
    ret = mb_full_master_request(master, slave_id, req, 5, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_coils(mb_full_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, const uint8_t *data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[8];
    uint16_t rsp_len;
    uint16_t byte_count = (count + 7) / 8;
    int ret;
    
    if (!master || !data || count == 0 || count > 2000) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_MULTIPLE_COILS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    req[5] = byte_count;
    memcpy(&req[6], data, byte_count);
    
    ret = mb_full_master_request(master, slave_id, req, 6 + byte_count, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count, const uint16_t *data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[8];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data || count == 0 || count > 123) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    req[1] = (addr >> 8) & 0xFF;
    req[2] = addr & 0xFF;
    req[3] = (count >> 8) & 0xFF;
    req[4] = count & 0xFF;
    req[5] = count * 2;
    
    for (uint16_t i = 0; i < count; i++) {
        req[6 + i * 2] = (data[i] >> 8) & 0xFF;
        req[7 + i * 2] = data[i] & 0xFF;
    }
    
    ret = mb_full_master_request(master, slave_id, req, 6 + count * 2, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                         uint16_t read_addr, uint16_t read_count,
                                         uint16_t *read_data,
                                         uint16_t write_addr, uint16_t write_count,
                                         const uint16_t *write_data)
{
    uint8_t req[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !read_data || !write_data) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_WRITE_MULTIPLE;
    req[1] = (read_addr >> 8) & 0xFF;
    req[2] = read_addr & 0xFF;
    req[3] = (read_count >> 8) & 0xFF;
    req[4] = read_count & 0xFF;
    req[5] = (write_addr >> 8) & 0xFF;
    req[6] = write_addr & 0xFF;
    req[7] = (write_count >> 8) & 0xFF;
    req[8] = write_count & 0xFF;
    req[9] = write_count * 2;
    
    for (uint16_t i = 0; i < write_count; i++) {
        req[10 + i * 2] = (write_data[i] >> 8) & 0xFF;
        req[11 + i * 2] = write_data[i] & 0xFF;
    }
    
    ret = mb_full_master_request(master, slave_id, req, 10 + write_count * 2, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    /* 复制读回的数据 */
    uint8_t byte_count = rsp[2];
    for (uint8_t i = 0; i < byte_count / 2; i++) {
        read_data[i] = ((uint16_t)rsp[3 + i * 2] << 8) | rsp[4 + i * 2];
    }
    
    return MB_FULL_OK;
}

int mb_full_master_read_device_id(mb_full_master_t *master, uint8_t slave_id,
                                    mb_object_id_t object_id, uint8_t *data)
{
    uint8_t req[3], rsp[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t rsp_len;
    int ret;
    
    if (!master || !data) {
        return MB_FULL_INVALID_PARAM;
    }
    
    req[0] = MB_FUNC_READ_DEVICE_ID;
    req[1] = 0x0E;  /* 读设备 ID */
    req[2] = (uint8_t)object_id;
    
    ret = mb_full_master_request(master, slave_id, req, 3, rsp, &rsp_len);
    if (ret != MB_FULL_OK) return ret;
    
    if (rsp[1] & 0x80) {
        master->error_count++;
        return MB_FULL_ERROR;
    }
    
    memcpy(data, &rsp[2], rsp_len - 2);
    
    return MB_FULL_OK;
}
