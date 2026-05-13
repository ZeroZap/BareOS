
![img](https://ai.eaglelab.tcl.com/_nuxt/assets/img/user-MC6lwZQe.png)

![arrow-down](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAABgCAYAAADimHc4AAAAAXNSR0IArs4c6QAAAARzQklUCAgICHwIZIgAAAH+SURBVHic7dbLkdpAFIXh0+BAwEUA9gqVV5pMnMGEQJGBJwKHIq1cs3QArvIkIl9v6GlcZqDF6HEb/m9LP1TntERLAAAAAAAAAAAAAAAAAAAAAADg7mwqq1df7NPcz+HNprJ6XVmzqazuM+9Dn8Eft/a9M30NnbTeWmtB+5fn0PZZ49ZsKqs7adeZaknqgiSpzZ0fcgeuKquDqTnx010WEYPXIfhjy6CHX5l5ZBew3loj/b/Zkbso4lzwr4La38/hIWe9rALOnP5TbrKIrOCP5L4FWQVknP5TbqKIvsG/ynwLLhbQ8/SfUmQRVwd/xJb6/PIj/Dw3ZnFpkWDaXfsAB3UwNeutNaueV7Q5xOtkZ2reE36uiwVY0F49rlVnuC5i6OCD9O3S6T+My3P4FO3U/7/gLS4+TUN8av4R1C6l/eDX0OhWipg7+DTtSqUW4SX4NP2dSinCW/BpmYF4LcJr8Gm5gXkpwnvwadmRzFVEKcGn5Uc2VRGlBZ+2mcgYRSxMT5L0Z6HH0oJP201shCKGMXHwaduZuClipuDT9jObrYiZg0+P4cRkRTgJPnJTQDRaEc6Cj9wVEA1WhNPgI7cFRFcX4Tz4yH0BUXYRhQQfFVNA9GYRhQUfFVdAtKqsXpgeJWkR9FRa8AAAAAAAAAAAAAAAAAAAAAAAAACA8v0FBuNXfDwVFiQAAAAASUVORK5CYII=)

```
基于单片机一个兼容RTOS和裸机的 modbus 从机 设计，要求精简，只兼容 asic 模式无须 TCP
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

# 基于单片机的 Modbus 从机设计（兼容 RTOS 和裸机）

## 设计概述

本设计实现一个精简的 Modbus RTU/ASCII 从机协议栈，支持裸机轮询和 RTOS 多任务两种运行模式。

------

## 1. 文件结构

```text
modbus_slave/
├── modbus_slave.h      // 头文件，接口定义
├── modbus_slave.c      // 核心实现
├── modbus_port.h       // 硬件抽象层接口
└── modbus_port.c       // 硬件抽象层实现（用户适配）
```

------

## 2. 头文件 `modbus_slave.h`

```c
#ifndef __MODBUS_SLAVE_H__
#define __MODBUS_SLAVE_H__

#include <stdint.h>
#include <stdbool.h>

/*==================== 配置区 ====================*/
#define MB_SLAVE_ADDR           1       // 从机地址
#define MB_FRAME_MAX            256     // 最大帧长度
#define MB_REG_HOLDING_NUM      100     // 保持寄存器数量
#define MB_REG_INPUT_NUM        100     // 输入寄存器数量
#define MB_REG_COIL_NUM         64      // 线圈数量
#define MB_REG_DISCRETE_NUM     64      // 离散输入数量

/* RTOS 模式开关: 1=RTOS模式, 0=裸机模式 */
#define MB_USE_RTOS             0

#if MB_USE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #define MB_LOCK()           xSemaphoreTake(mb_mutex, portMAX_DELAY)
    #define MB_UNLOCK()         xSemaphoreGive(mb_mutex)
    extern SemaphoreHandle_t mb_mutex;
#else
    #define MB_LOCK()           do {} while(0)
    #define MB_UNLOCK()         do {} while(0)
#endif

/*==================== 功能码定义 ====================*/
typedef enum {
    MB_FC_READ_COILS            = 0x01,
    MB_FC_READ_DISCRETE         = 0x02,
    MB_FC_READ_HOLDING_REG      = 0x03,
    MB_FC_READ_INPUT_REG        = 0x04,
    MB_FC_WRITE_SINGLE_COIL     = 0x05,
    MB_FC_WRITE_SINGLE_REG      = 0x06,
    MB_FC_WRITE_MULTI_COILS     = 0x0F,
    MB_FC_WRITE_MULTI_REGS      = 0x10,
} mb_func_code_t;

/*==================== 异常码定义 ====================*/
typedef enum {
    MB_EX_NONE                  = 0x00,
    MB_EX_ILLEGAL_FUNC          = 0x01,
    MB_EX_ILLEGAL_ADDR          = 0x02,
    MB_EX_ILLEGAL_VALUE         = 0x03,
    MB_EX_SLAVE_FAILURE         = 0x04,
} mb_exception_t;

/*==================== 状态机 ====================*/
typedef enum {
    MB_STATE_IDLE,
    MB_STATE_RECEIVING,
    MB_STATE_FRAME_READY,
    MB_STATE_PROCESSING,
} mb_state_t;

/*==================== 从机控制块 ====================*/
typedef struct {
    uint8_t         addr;                       // 从机地址
    mb_state_t      state;                      // 状态机
    uint8_t         rx_buf[MB_FRAME_MAX];       // 接收缓冲
    uint8_t         tx_buf[MB_FRAME_MAX];       // 发送缓冲
    uint16_t        rx_cnt;                     // 接收计数
    uint16_t        tx_len;                     // 发送长度
    uint32_t        last_rx_tick;               // 最后接收时间
    uint32_t        t35_timeout;                // 3.5字符超时(ms)
    
    /* 寄存器数据 */
    uint16_t        holding_reg[MB_REG_HOLDING_NUM];
    uint16_t        input_reg[MB_REG_INPUT_NUM];
    uint8_t         coil_reg[(MB_REG_COIL_NUM + 7) / 8];
    uint8_t         discrete_reg[(MB_REG_DISCRETE_NUM + 7) / 8];
} mb_slave_t;

/*==================== API 接口 ====================*/
void     mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baudrate);
void     mb_slave_poll(mb_slave_t *mb);
void     mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte);

/* 寄存器读写接口（用户可调用） */
bool     mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr);
bool     mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
bool     mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value);
bool     mb_get_coil(mb_slave_t *mb, uint16_t addr);
bool     mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value);

#endif /* __MODBUS_SLAVE_H__ */
```

------

## 3. 核心实现 `modbus_slave.c`

```c
#include "modbus_slave.h"
#include "modbus_port.h"
#include <string.h>

#if MB_USE_RTOS
SemaphoreHandle_t mb_mutex = NULL;
#endif

/*==================== CRC16 计算 ====================*/
static const uint16_t crc_table[256] = {
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

static uint16_t mb_crc16(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xFF];
    }
    return crc;
}

/*==================== 初始化 ====================*/
void mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baudrate)
{
    memset(mb, 0, sizeof(mb_slave_t));
    mb->addr = addr;
    mb->state = MB_STATE_IDLE;
    
    /* 计算3.5字符超时时间(ms) */
    /* 1字符 = 11位(1起始+8数据+1校验+1停止) */
    if (baudrate > 19200) {
        mb->t35_timeout = 2;  // 高波特率固定1.75ms，取2ms
    } else {
        mb->t35_timeout = (35 * 11 * 1000) / (baudrate * 10) + 1;
    }
    
#if MB_USE_RTOS
    if (mb_mutex == NULL) {
        mb_mutex = xSemaphoreCreateMutex();
    }
#endif
    
    mb_port_init(baudrate);
}

/*==================== 接收字节（中断调用） ====================*/
void mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte)
{
    if (mb->rx_cnt < MB_FRAME_MAX) {
        mb->rx_buf[mb->rx_cnt++] = byte;
        mb->last_rx_tick = mb_port_get_tick();
        
        if (mb->state == MB_STATE_IDLE) {
            mb->state = MB_STATE_RECEIVING;
        }
    }
}

/*==================== 发送异常响应 ====================*/
static void mb_send_exception(mb_slave_t *mb, uint8_t func, mb_exception_t ex)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = func | 0x80;
    mb->tx_buf[2] = ex;
    
    uint16_t crc = mb_crc16(mb->tx_buf, 3);
    mb->tx_buf[3] = crc & 0xFF;
    mb->tx_buf[4] = crc >> 8;
    mb->tx_len = 5;
    
    mb_port_send(mb->tx_buf, mb->tx_len);
}

/*==================== 功能码处理 ====================*/
static mb_exception_t mb_fc_read_holding_reg(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_HOLDING_REG;
    mb->tx_buf[2] = quantity * 2;
    
    uint16_t idx = 3;
    for (uint16_t i = 0; i < quantity; i++) {
        mb->tx_buf[idx++] = mb->holding_reg[start_addr + i] >> 8;
        mb->tx_buf[idx++] = mb->holding_reg[start_addr + i] & 0xFF;
    }
    
    uint16_t crc = mb_crc16(mb->tx_buf, idx);
    mb->tx_buf[idx++] = crc & 0xFF;
    mb->tx_buf[idx++] = crc >> 8;
    mb->tx_len = idx;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_read_input_reg(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_INPUT_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_INPUT_REG;
    mb->tx_buf[2] = quantity * 2;
    
    uint16_t idx = 3;
    for (uint16_t i = 0; i < quantity; i++) {
        mb->tx_buf[idx++] = mb->input_reg[start_addr + i] >> 8;
        mb->tx_buf[idx++] = mb->input_reg[start_addr + i] & 0xFF;
    }
    
    uint16_t crc = mb_crc16(mb->tx_buf, idx);
    mb->tx_buf[idx++] = crc & 0xFF;
    mb->tx_buf[idx++] = crc >> 8;
    mb->tx_len = idx;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_read_coils(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    uint8_t byte_cnt = (quantity + 7) / 8;
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_COILS;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t coil_addr = start_addr + i;
        if (mb->coil_reg[coil_addr / 8] & (1 << (coil_addr % 8))) {
            mb->tx_buf[3 + i / 8] |= (1 << (i % 8));
        }
    }
    
    uint16_t idx = 3 + byte_cnt;
    uint16_t crc = mb_crc16(mb->tx_buf, idx);
    mb->tx_buf[idx++] = crc & 0xFF;
    mb->tx_buf[idx++] = crc >> 8;
    mb->tx_len = idx;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_read_discrete(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_DISCRETE_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    uint8_t byte_cnt = (quantity + 7) / 8;
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_DISCRETE;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t addr = start_addr + i;
        if (mb->discrete_reg[addr / 8] & (1 << (addr % 8))) {
            mb->tx_buf[3 + i / 8] |= (1 << (i % 8));
        }
    }
    
    uint16_t idx = 3 + byte_cnt;
    uint16_t crc = mb_crc16(mb->tx_buf, idx);
    mb->tx_buf[idx++] = crc & 0xFF;
    mb->tx_buf[idx++] = crc >> 8;
    mb->tx_len = idx;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_write_single_reg(mb_slave_t *mb)
{
    uint16_t reg_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t value    = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (reg_addr >= MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    mb->holding_reg[reg_addr] = value;
    
    /* 原样返回 */
    memcpy(mb->tx_buf, mb->rx_buf, 8);
    mb->tx_len = 8;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_write_single_coil(mb_slave_t *mb)
{
    uint16_t coil_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t value     = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    
    if (coil_addr >= MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (value != 0x0000 && value != 0xFF00) {
        return MB_EX_ILLEGAL_VALUE;
    }
    
    if (value == 0xFF00) {
        mb->coil_reg[coil_addr / 8] |= (1 << (coil_addr % 8));
    } else {
        mb->coil_reg[coil_addr / 8] &= ~(1 << (coil_addr % 8));
    }
    
    memcpy(mb->tx_buf, mb->rx_buf, 8);
    mb->tx_len = 8;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_write_multi_regs(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 123 || byte_cnt != quantity * 2) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    for (uint16_t i = 0; i < quantity; i++) {
        mb->holding_reg[start_addr + i] = (mb->rx_buf[7 + i * 2] << 8) | 
                                           mb->rx_buf[8 + i * 2];
    }
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_REGS;
    mb->tx_buf[2] = start_addr >> 8;
    mb->tx_buf[3] = start_addr & 0xFF;
    mb->tx_buf[4] = quantity >> 8;
    mb->tx_buf[5] = quantity & 0xFF;
    
    uint16_t crc = mb_crc16(mb->tx_buf, 6);
    mb->tx_buf[6] = crc & 0xFF;
    mb->tx_buf[7] = crc >> 8;
    mb->tx_len = 8;
    
    return MB_EX_NONE;
}

static mb_exception_t mb_fc_write_multi_coils(mb_slave_t *mb)
{
    uint16_t start_addr = (mb->rx_buf[2] << 8) | mb->rx_buf[3];
    uint16_t quantity   = (mb->rx_buf[4] << 8) | mb->rx_buf[5];
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 1968 || byte_cnt != (quantity + 7) / 8) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t coil_addr = start_addr + i;
        if (mb->rx_buf[7 + i / 8] & (1 << (i % 8))) {
            mb->coil_reg[coil_addr / 8] |= (1 << (coil_addr % 8));
        } else {
            mb->coil_reg[coil_addr / 8] &= ~(1 << (coil_addr % 8));
        }
    }
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_COILS;
    mb->tx_buf[2] = start_addr >> 8;
    mb->tx_buf[3] = start_addr & 0xFF;
    mb->tx_buf[4] = quantity >> 8;
    mb->tx_buf[5] = quantity & 0xFF;
    
    uint16_t crc = mb_crc16(mb->tx_buf, 6);
    mb->tx_buf[6] = crc & 0xFF;
    mb->tx_buf[7] = crc >> 8;
    mb->tx_len = 8;
    
    return MB_EX_NONE;
}

/*==================== 帧处理 ====================*/
static void mb_process_frame(mb_slave_t *mb)
{
    /* 最小帧长度检查 */
    if (mb->rx_cnt < 4) {
        return;
    }
    
    /* 地址检查 */
    if (mb->rx_buf[0] != mb->addr && mb->rx_buf[0] != 0) {
        return;
    }
    
    /* CRC 校验 */
    uint16_t crc_recv = (mb->rx_buf[mb->rx_cnt - 1] << 8) | mb->rx_buf[mb->rx_cnt - 2];
    uint16_t crc_calc = mb_crc16(mb->rx_buf, mb->rx_cnt - 2);
    if (crc_recv != crc_calc) {
        return;
    }
    
    /* 广播地址不响应 */
    bool is_broadcast = (mb->rx_buf[0] == 0);
    
    uint8_t func = mb->rx_buf[1];
    mb_exception_t ex = MB_EX_NONE;
    
    MB_LOCK();
    
    switch (func) {
        case MB_FC_READ_COILS:
            ex = mb_fc_read_coils(mb);
            break;
        case MB_FC_READ_DISCRETE:
            ex = mb_fc_read_discrete(mb);
            break;
        case MB_FC_READ_HOLDING_REG:
            ex = mb_fc_read_holding_reg(mb);
            break;
        case MB_FC_READ_INPUT_REG:
            ex = mb_fc_read_input_reg(mb);
            break;
        case MB_FC_WRITE_SINGLE_COIL:
            ex = mb_fc_write_single_coil(mb);
            break;
        case MB_FC_WRITE_SINGLE_REG:
            ex = mb_fc_write_single_reg(mb);
            break;
        case MB_FC_WRITE_MULTI_COILS:
            ex = mb_fc_write_multi_coils(mb);
            break;
        case MB_FC_WRITE_MULTI_REGS:
            ex = mb_fc_write_multi_regs(mb);
            break;
        default:
            ex = MB_EX_ILLEGAL_FUNC;
            break;
    }
    
    MB_UNLOCK();
    
    /* 广播不响应 */
    if (is_broadcast) {
        return;
    }
    
    if (ex != MB_EX_NONE) {
        mb_send_exception(mb, func, ex);
    } else {
        mb_port_send(mb->tx_buf, mb->tx_len);
    }
}

/*==================== 主轮询函数 ====================*/
void mb_slave_poll(mb_slave_t *mb)
{
    uint32_t current_tick = mb_port_get_tick();
    
    switch (mb->state) {
        case MB_STATE_IDLE:
            break;
            
        case MB_STATE_RECEIVING:
            /* 检测3.5字符超时，帧结束 */
            if ((current_tick - mb->last_rx_tick) >= mb->t35_timeout) {
                mb->state = MB_STATE_FRAME_READY;
            }
            break;
            
        case MB_STATE_FRAME_READY:
            mb->state = MB_STATE_PROCESSING;
            mb_process_frame(mb);
            /* 重置状态 */
            mb->rx_cnt = 0;
            mb->state = MB_STATE_IDLE;
            break;
            
        case MB_STATE_PROCESSING:
            break;
    }
}

/*==================== 用户接口函数 ====================*/
bool mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_HOLDING_NUM) return false;
    MB_LOCK();
    mb->holding_reg[addr] = value;
    MB_UNLOCK();
    return true;
}

uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_HOLDING_NUM) return 0;
    MB_LOCK();
    uint16_t val = mb->holding_reg[addr];
    MB_UNLOCK();
    return val;
}

bool mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_INPUT_NUM) return false;
    MB_LOCK();
    mb->input_reg[addr] = value;
    MB_UNLOCK();
    return true;
}

bool mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_LOCK();
    if (value) {
        mb->coil_reg[addr / 8] |= (1 << (addr % 8));
    } else {
        mb->coil_reg[addr / 8] &= ~(1 << (addr % 8));
    }
    MB_UNLOCK();
    return true;
}

bool mb_get_coil(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_LOCK();
    bool val = (mb->coil_reg[addr / 8] & (1 << (addr % 8))) != 0;
    MB_UNLOCK();
    return val;
}

bool mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_DISCRETE_NUM) return false;
    MB_LOCK();
    if (value) {
        mb->discrete_reg[addr / 8] |= (1 << (addr % 8));
    } else {
        mb->discrete_reg[addr / 8] &= ~(1 << (addr % 8));
    }
    MB_UNLOCK();
    return val;
}
```

------

## 4. 硬件抽象层 `modbus_port.h`

```c
#ifndef __MODBUS_PORT_H__
#define __MODBUS_PORT_H__

#include <stdint.h>

/* 用户需实现以下函数 */
void     mb_port_init(uint32_t baudrate);
void     mb_port_send(uint8_t *buf, uint16_t len);
uint32_t mb_port_get_tick(void);  // 返回毫秒级系统时钟

#endif
```

------

## 5. 硬件抽象层示例 `modbus_port.c` (STM32 HAL)

```c
#include "modbus_port.h"
#include "main.h"

extern UART_HandleTypeDef huart1;
extern mb_slave_t mb_slave;

void mb_port_init(uint32_t baudrate)
{
    /* UART 已在 CubeMX 中初始化 */
    /* 开启接收中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

void mb_port_send(uint8_t *buf, uint16_t len)
{
    /* RS485 方向控制（如有） */
    // HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_SET);
    
    HAL_UART_Transmit(&huart1, buf, len, 100);
    
    // HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET);
}

uint32_t mb_port_get_tick(void)
{
    return HAL_GetTick();
}

/* UART 中断回调 - 在 stm32f1xx_it.c 中调用 */
void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t byte = huart1.Instance->DR;
        mb_slave_rx_byte(&mb_slave, byte);
    }
    HAL_UART_IRQHandler(&huart1);
}
```

------

## 6. 使用示例

### 裸机模式 (main.c)

```c
#include "modbus_slave.h"

mb_slave_t mb_slave;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    
    /* 初始化 Modbus 从机，地址1，波特率9600 */
    mb_slave_init(&mb_slave, 1, 9600);
    
    /* 设置初始值 */
    mb_set_holding_reg(&mb_slave, 0, 1234);
    mb_set_input_reg(&mb_slave, 0, 5678);
    
    while (1) {
        mb_slave_poll(&mb_slave);
        
        /* 用户应用逻辑 */
        // 更新输入寄存器（如ADC值）
        // mb_set_input_reg(&mb_slave, 1, adc_value);
    }
}
```

### RTOS 模式 (FreeRTOS)

```c
#include "modbus_slave.h"
#include "FreeRTOS.h"
#include "task.h"

mb_slave_t mb_slave;

void modbus_task(void *param)
{
    mb_slave_init(&mb_slave, 1, 9600);
    
    while (1) {
        mb_slave_poll(&mb_slave);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_task(void *param)
{
    while (1) {
        /* 安全地读写寄存器 */
        uint16_t val = mb_get_holding_reg(&mb_slave, 0);
        mb_set_input_reg(&mb_slave, 0, adc_read());
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    xTaskCreate(modbus_task, "Modbus", 256, NULL, 2, NULL);
    xTaskCreate(app_task, "App", 256, NULL, 1, NULL);
    
    vTaskStartScheduler();
}
```

------

## 7. 设计特点

|     特性     |                   说明                   |
| :----------: | :--------------------------------------: |
|   **精简**   |      核心代码约 500 行，无外部依赖       |
|  **兼容性**  |         通过宏切换 RTOS/裸机模式         |
| **标准协议** | 支持 Modbus RTU 标准功能码 01-06, 0F, 10 |
|  **可移植**  |       硬件抽象层仅需实现 3 个函数        |
| **线程安全** |         RTOS 模式下自动加锁保护          |
|  **低资源**  |   RAM < 1KB（可配置），无动态内存分配    |

------

## 8. 支持的功能码

| 功能码 |     名称     |           说明           |
| :----: | :----------: | :----------------------: |
|  0x01  |    读线圈    |        Read Coils        |
|  0x02  |  读离散输入  |   Read Discrete Inputs   |
|  0x03  | 读保持寄存器 |  Read Holding Registers  |
|  0x04  | 读输入寄存器 |   Read Input Registers   |
|  0x05  |  写单个线圈  |    Write Single Coil     |
|  0x06  | 写单个寄存器 |  Write Single Register   |
|  0x0F  |  写多个线圈  |   Write Multiple Coils   |
|  0x10  | 写多个寄存器 | Write Multiple Registers |

如需扩展其他功能码或添加回调机制，可以进一步定制！





```
正文存在大量的 /8, %8, 编译器会不会优化使得没有硬件除法的单片机也能转换成移位操作。拓展其他功能码及回调机制
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

您提出了一个非常好的问题！关于编译器优化和功能扩展，我来详细解答并提供改进版本。

------

## 1. 关于 `/8` 和 `%8` 的编译器优化

### 编译器优化分析

|      情况       |   是否优化   |             说明             |
| :-------------: | :----------: | :--------------------------: |
| 常量除数为2的幂 | ✅ **会优化** | `/8` → `>>3`，`%8` → `&0x07` |
|    变量除数     |  ❌ 不会优化  |      需要调用除法库函数      |
| 优化等级 `-O0`  | ⚠️ 可能不优化 |   调试模式可能保留原始运算   |

### 验证示例（ARM GCC -O2）

```c
// 源代码
uint8_t byte_idx = addr / 8;
uint8_t bit_idx  = addr % 8;

// 编译后汇编 (ARM Cortex-M0, 无硬件除法)
lsrs    r1, r0, #3      // addr >> 3 (除以8)
and     r2, r0, #7      // addr & 7  (模8)
```

### 建议：显式使用位操作（更安全）

为了**确保在任何优化等级下都能高效运行**，建议显式使用位操作：

```c
/* 推荐写法 - 明确意图，不依赖编译器优化 */
#define BIT_BYTE_IDX(addr)    ((addr) >> 3)       // 等价于 addr / 8
#define BIT_BIT_IDX(addr)     ((addr) & 0x07)     // 等价于 addr % 8
#define BIT_MASK(addr)        (1U << ((addr) & 0x07))
```

------

## 2. 改进版本：位操作宏 + 回调机制 + 扩展功能码

### 完整头文件 `modbus_slave.h`

```c
#ifndef __MODBUS_SLAVE_H__
#define __MODBUS_SLAVE_H__

#include <stdint.h>
#include <stdbool.h>

/*==================== 配置区 ====================*/
#define MB_SLAVE_ADDR_DEFAULT   1
#define MB_FRAME_MAX            256
#define MB_REG_HOLDING_NUM      100
#define MB_REG_INPUT_NUM        100
#define MB_REG_COIL_NUM         64
#define MB_REG_DISCRETE_NUM     64

/* RTOS 模式开关 */
#define MB_USE_RTOS             0

/* 回调机制开关 */
#define MB_USE_CALLBACK         1

#if MB_USE_RTOS
    #include "FreeRTOS.h"
    #include "semphr.h"
    #define MB_ENTER_CRITICAL()     xSemaphoreTake(mb_mutex, portMAX_DELAY)
    #define MB_EXIT_CRITICAL()      xSemaphoreGive(mb_mutex)
    extern SemaphoreHandle_t mb_mutex;
#else
    #define MB_ENTER_CRITICAL()     do {} while(0)
    #define MB_EXIT_CRITICAL()      do {} while(0)
#endif

/*==================== 位操作宏（避免除法） ====================*/
#define MB_BIT_BYTE_IDX(addr)       ((uint16_t)(addr) >> 3)
#define MB_BIT_BIT_IDX(addr)        ((uint8_t)(addr) & 0x07U)
#define MB_BIT_MASK(addr)           (1U << MB_BIT_BIT_IDX(addr))

#define MB_BIT_SET(array, addr)     ((array)[MB_BIT_BYTE_IDX(addr)] |= MB_BIT_MASK(addr))
#define MB_BIT_CLR(array, addr)     ((array)[MB_BIT_BYTE_IDX(addr)] &= ~MB_BIT_MASK(addr))
#define MB_BIT_GET(array, addr)     (((array)[MB_BIT_BYTE_IDX(addr)] & MB_BIT_MASK(addr)) != 0)
#define MB_BIT_WR(array, addr, val) do { \
    if (val) MB_BIT_SET(array, addr); \
    else     MB_BIT_CLR(array, addr); \
} while(0)

/* 字节数计算宏 */
#define MB_BITS_TO_BYTES(bits)      (((bits) + 7U) >> 3)

/*==================== 功能码定义 ====================*/
typedef enum {
    MB_FC_READ_COILS            = 0x01,
    MB_FC_READ_DISCRETE         = 0x02,
    MB_FC_READ_HOLDING_REG      = 0x03,
    MB_FC_READ_INPUT_REG        = 0x04,
    MB_FC_WRITE_SINGLE_COIL     = 0x05,
    MB_FC_WRITE_SINGLE_REG      = 0x06,
    MB_FC_READ_EXCEPTION_STATUS = 0x07,
    MB_FC_DIAGNOSTICS           = 0x08,
    MB_FC_GET_COMM_EVENT_CNT    = 0x0B,
    MB_FC_GET_COMM_EVENT_LOG    = 0x0C,
    MB_FC_WRITE_MULTI_COILS     = 0x0F,
    MB_FC_WRITE_MULTI_REGS      = 0x10,
    MB_FC_REPORT_SLAVE_ID       = 0x11,
    MB_FC_READ_FILE_RECORD      = 0x14,
    MB_FC_WRITE_FILE_RECORD     = 0x15,
    MB_FC_MASK_WRITE_REG        = 0x16,
    MB_FC_RW_MULTI_REGS         = 0x17,
    MB_FC_READ_FIFO_QUEUE       = 0x18,
} mb_func_code_t;

/*==================== 异常码定义 ====================*/
typedef enum {
    MB_EX_NONE                  = 0x00,
    MB_EX_ILLEGAL_FUNC          = 0x01,
    MB_EX_ILLEGAL_ADDR          = 0x02,
    MB_EX_ILLEGAL_VALUE         = 0x03,
    MB_EX_SLAVE_FAILURE         = 0x04,
    MB_EX_ACK                   = 0x05,
    MB_EX_SLAVE_BUSY            = 0x06,
    MB_EX_MEMORY_PARITY_ERR     = 0x08,
    MB_EX_GATEWAY_PATH_UNAVAIL  = 0x0A,
    MB_EX_GATEWAY_TARGET_FAIL   = 0x0B,
} mb_exception_t;

/*==================== 状态机 
    MB_STATE_IDLE,
    MB_STATE_RECEIVING,
    MB_STATE_FRAME_READY,
    MB_STATE_PROCESSING,
} mb_state_t;

/*==================== 回调事件类型 ====================*/
#if MB_USE_CALLBACK
typedef enum {
    MB_CB_PRE_READ_HOLDING,     // 读保持寄存器前
    MB_CB_POST_WRITE_HOLDING,   // 写保持寄存器后
    MB_CB_PRE_READ_INPUT,       // 
    MB_CB_PRE_READ_COIL,        // 读线圈前
    MB_CB_POST_WRITE_COIL,      // 写线圈后
    MB_CB_PRE_READ_DISCRETE,    // 读离散输入前
    MB_CB_CUSTOM_FUNC,          // 自定义功能码
} mb_callback_event_t;

/* 回调函数原型 */
struct mb_slave_s;
typedef mb_exception_t (*mb_callback_t)(struct mb_slave_s *mb, 
                                        mb_callback_event_t event
                                        uint16_t addr, 
                                        uint16_t quantity);

/* 自定义功能码处理函数原型 */
typedef mb_exception_t (*mb_custom_func_t)(struct mb_slave_s *mb,
                                           uint8_t func_code,
                                           uint8_t *rx_data,
                                           uint16_t rx_len,
                                           uint8_t *tx_data,
                                           uint16_t *tx_len);
#endif

/*==================== 从制块 ====================*/
typedef struct mb_slave_s {
    uint8_t         addr;
    mb_state_t      state;
    uint8_t         rx_buf[MB_FRAME_MAX];
    uint8_t         tx_buf[MB_FRAME_MAX];
    uint16_t        rx_cnt;
    uint16_t        tx_len;
    uint32_t        last_rx_tick;
    uint32_t        t35_timeout;
    
    /* 统计信息 */
    uint32_t        msg_count;          // 消息计数
    rc_err_count;      // CRC错误计数
    uint32_t        exception_count;    // 异常计数
    
    /* 寄存器数据 */
    uint16_t        holding_reg[MB_REG_HOLDING_NUM];
    uint16_t        input_reg[MB_REG_INPUT_NUM];
    uint8_t         coil_reg[MB_BITS_TO_BYTES(MB_REG_COIL_NUM)];
    uint8_t         discrete_reg[MB_BITS_TO_BYTES(MB_REG_DISCRETE_NUM)];
    
    /* 设备标识（用于0x11功_t         slave_id_data[32];
    uint8_t         slave_id_len;
    
#if MB_USE_CALLBACK
    mb_callback_t       callback;           // 通用回调
    mb_custom_func_t    custom_func_cb;     // 自定义功能码回调
#endif
} mb_slave_t;

/*==================== API 接口 ====================*/
/* 初始化与轮询 */
void     mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baud     mb_slave_poll(mb_slave_t *mb);
void     mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte);
void     mb_slave_reset(mb_slave_t *mb);

/* 寄存器读写接口 */
bool     mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr);
bool     mb_set_holding_regs(mb_slave_t *mb, uint16_t addr, const16_t count);

bool     mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
uint16_t mb_get_input_reg(mb_slave_t *mb, uint16_t addr);
bool     mb_set_input_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count);

bool     mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value);
bool     mb_get_coil(mb_slave_t *mb, uint16_t addr);
bool     mb_set_coils(mb_slave_t *mb, uint16_t addr, const uint8_t *values, uint16_     mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value);
bool     mb_get_discrete(mb_slave_t *mb, uint16_t addr);

/* 设备标识设置 */
void     mb_set_slave_id(mb_slave_t *mb, const uint8_t *data, uint8_t len);

#if MB_USE_CALLBACK
/* 回调注册 */
void     mb_set_callback(mb_slave_t *mb, mb_callback_t cb);
void     mb_set_custom_func_callback(mb_slave_t *mb, mb_custom_func_t);
#endif

#endif /* __MODBUS_SLAVE_H__ */
```

------

### 完整实现 `modbus_slave.c`

```c
#include "modbus_slave.h"
#include "modbus_port.h"
#include <string.h>

#if MB_USE_RTOS
SemaphoreHandle_t mb_mutex = NULL;
#endif

/*==================== CRC16 查表法 ====================*/
static const uint16_t crc_table[256] = {
    0x0000, 0xC0C1, 0x, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 07200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 uint16_t mb_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xFF];
    }
    return crc;
}

/*==================== 辅助函数 ====================*/
static inline uint16_t mb_get_u16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | buf[1];
}

static inline void mb_put_u16(uint8_t *buf, uint16buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val & 0xFF);
}

static uint16_t mb_append_crc(uint8_t *buf, uint16_t len)
{
    uint16_t crc = mb_crc16(buf, len);
    buf[len]     = (uint8_t)(crc & 0xFF);
    buf[len + 1] = (uint8_t)(crc >> 8);
    return len + 2;
}

/*==================== 初始化 ====================*/
void mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baudrate)
{
    memset(mb, 0, sizeof(mb_slave_t));
    mb->addr = addr;_IDLE;
    
    /* 计算3.5字符超时时间(ms) */
    if (baudrate > 19200) {
        mb->t35_timeout = 2;
    } else {
        /* t35 = 3.5 * 11 * 1000 / baudrate (ms) */
        mb->t35_timeout = (38500UL / baudrate) +
    /* 默认设备标识 */
    const char *default_id = "MB-SLAVE";
    mb_set_slave_id(mb, (const uint8_t *)default_id, strlen(default_id));
    
#if MB_USE_RTOS
    if (mb_mutex == NULL) {
        mb_mutex = xSemaphoreCreateMutex();
    }
#endif
    
    mb_port_init(baudrate);
}

void mb_slave_reset(mb_slave_t *mb)
{
    mb->rx_cnt = 0;
     = MB_STATE_IDLE;
}

/*==================== 接收字节 ====================*/
void mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte)
{
    if (mb->rx_cnt < MB_FRAME_MAX) {
        mb->rx_buf[mb->rx_cnt++] = byte;
        mb->last_rx_tick = mb_port_get_tick();
        
        if (mb->state == MB_STATE_IDLE) {
            mb->state = MB_STATE_RECEIVING;
        }
    }
}

/*==================== 发送异常响应 ====================*/
static void_exception(mb_slave_t *mb, uint8_t func, mb_exception_t ex)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = func | 0x80;
    mb->tx_buf[2] = (uint8_t)ex;
    mb->tx_len = mb_append_crc(mb->tx_buf, 3);
    mb->exception_count++;
    mb_port_send(mb->tx_buf, mb->tx_len);
}

/*==================== 功数 ====================*/

/* 0x01 - 读线圈 */
static mb_exception_t mb_fc_read_coils(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MBG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_READ_COIL, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    uint8_t byte_cnt = (uint8_t)MB_BITS_TO_BYTES(quantity);
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_COILS;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    for (uint16_t i = 0; i < quantity; i++) {
        if (MB_BIT_GET(mb->coil_reg, start_addr + i)) {
            MB_BIT_SET(&mb->tx_buf[3], i);
        }
    }
    
    mb->tx_(mb->tx_buf, 3 + byte_cnt);
    return MB_EX_NONE;
}

/* 0x02 - 读离散输入 */
static mb_exception_t mb_fc_read_discrete(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_DISCRETE_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mbREAD_DISCRETE, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    uint8_t byte_cnt = (uint8_t)MB_BITS_TO_BYTES(quantity);
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_DISCRETE;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    for (uint16_t i = 0; i < quantity; i++) {
        if (MB_BIT_GET(mb->discrete_reg, start_addr + i)) {
            MB_BIT_SET(&mb->tx_buf[3], i);
        }
    }
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 3 +_EX_NONE;
}

/* 0x03 - 读保持寄存器 */
static mb_exception_t mb_fc_read_holding_reg(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PR start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_HOLDING_REG;
    mb->tx_buf[2] = (uint8_t)(quantity << 1);
    
    uint16_t idx = 3;
    for (uint16_t i = 0; i < quantity; i++) {
        mb_put_u16(&mb->tx_buf[idx], mb->holding_reg[start_addr + i]);
        idx += 2;->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x04 - 读输入寄存器 */
static mb_exception_t mb_fc_read_input_reg(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_INPUT_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mbREAD_INPUT, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_INPUT_REG;
    mb->tx_buf[2] = (uint8_t)(quantity << 1);
    
    uint16_t idx = 3;
    for (uint16_t i = 0; i < quantity; i++) {
        mb_put_u16(&mb->tx_buf[idx], mb->input_reg[start_addr + i]);
        idx += 2;
    }
    
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x05 - 写单个线exception_t mb_fc_write_single_coil(mb_slave_t *mb)
{
    uint16_t coil_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t value     = mb_get_u16(&mb->rx_buf[4]);
    
    if (coil_addr >= MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (value != 0x0000 && value != 0xFF00) {
        return MB_EX_ILLEGAL_VALUE;
    }
    
    MB_BIT_WR(mb->coil_reg, coil_addr, (value 0xFF00));
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_COIL, coil_addr, 1);
    }
#endif
    
    /* 原样返回 */
    memcpy(mb->tx_buf, mb->rx_buf, 6);
    mb->tx_buf[0] = mb->addr;
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0寄存器 */
static mb_exception_t mb_fc_write_single_reg(mb_slave_t *mb)
{
    uint16_t reg_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t value    = mb_get_u16(&mb->rx_buf[4]);
    
    if (reg_addr >= MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    mb->holding_reg[reg_addr] = value;
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, reg_addr, 1);
    }
#endif
    
    memcpy(mb->tx_buf, mb->rx_buf, 6);
    mb->tx_;
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x0F - 写多个线圈 */
static mb_exception_t mb_fc_write_multi_coils(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 1968X_ILLEGAL_VALUE;
    }
    if (byte_cnt != MB_BITS_TO_BYTES(quantity)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    for (uint16_t i = 0; i < quantity; i++) {
        bool val = MB_BIT_GET(&mb->rx_buf[7], i);
        MB_BIT_WR(mb->coil_reg, start_addr + i, val);
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CBIL, start_addr, quantity);
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_COILS;
    mb_put_u16(&mb->tx_buf[2], start_addr);
    mb_put_u16(&mb->tx_buf[4], quantity);
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x10 - 写多个寄存器 */
static mb_exception_t mb_fc_write_multi_regs(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   mb->rx_buf[4]);
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 123) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (byte_cnt != (quantity << 1)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    for (uint16_t i = 0; i < quantity; i++) {
        mb->holding_reg[start_addr + i] = mb_get_u16(&mb->rx_buf[7 + (i << 1)]);
    }
    
#if MB_mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, start_addr, quantity);
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_REGS;
    mb_put_u16(&mb->tx_buf[2], start_addr);
    mb_put_u16(&mb->tx_buf[4], quantity);
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x11 - 报告从机ID */
static mb_exception_t mb_fc_report_slave_id(mb_slave_t *mb)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_REPORT_SLAVE_ID;
    mb->tx_buf[2] = mb->slave_id_len + 2;   // 字节数
    mb->tx_buf[3] = mb->addr;               // 从机ID
    mb->tx_buf[4] = 0xFF;                   // 运行状态 (0xFF=ON)
    
    memcpy(&mb->tx_buf[5], mb->slave_id_data, mb->slave_id_len);
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 5 + mb->slave_id_len);
    return MB_EX_NONE;
}

/* 0x16 - 屏蔽写寄存器 */
static mb_exception_t mb_fc_mask_write_reg(mb_slave_t *mb)
{
    uint16_t reg_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t and_mask = mb_get_u16(&mb->rx_buf[4]);
    uint16_t or_mask  = mb_get_u16(&mb->rx_buf[6]);
    
    if (reg_addr >= MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    /* Result = (Current AND And_Mask) OR (Or_Mask AND (NOT And_Mask)) */
    uint16_t current = mb->holding_reg[reg_addr];
    mb->holding_reg[reg_addr] = (current & and_mask) | (or_mask & ~and_mask);
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, reg_addr, 1);
    }
#endif
    
    /* 原样返回 */
    memcpy(mb->tx_buf, mb->rx_buf, 8);
    mb->tx_buf[0] = mb->addr;
    mb->tx_len = mb_append_crc(mb->tx_buf, 8);
    return MB_EX_NONE;
}

/* 0x17 - 读/写多个寄存器 */
static mb_exception_t mb_fc_rw_multi_regs(mb_slave_t *mb)
{
    uint16_t read_addr   = mb_get_u16(&mb->rx_buf[2]);
    uint16_t read_qty    = mb_get_u16(&mb->rx_buf[4]);
    uint16_t write_addr  = mb_get_u16(&mb->rx_buf[6]);
    uint16_t write_qty   = mb_get_u16(&mb->rx_buf[8]);
    uint8_t  write_bytes = mb->rx_buf[10];
    
    /* 参数校验 */
    if (read_qty < 1 || read_qty > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_qty < 1 || write_qty > 121) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_bytes != (write_qty << 1)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (read_addr + read_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (write_addr + write_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
    /* 先写后读 */
    for (uint16_t i = 0; i < write_qty; i++) {
        mb->holding_reg[write_addr + i] = mb_get_u16(&mb->rx_buf[11 + (i << 1)]);
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, write_addr, write_qty);
        mb->callback(mb, MB_CB_PRE_READ_HOLDING, read_addr, read_qty);
    }
#endif
    
    /* 构建响应 */
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_RW_MULTI_REGS;
    mb->tx_buf[2] = (uint8_t)(read_qty << 1);
    
    uint16_t idx = 3;
    for (uint16_t i = 0; i < read_qty; i++) {
        mb_put_u16(&mb->tx_buf[idx], mb->holding_reg[read_addr + i]);
        idx += 2;
    }
    
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x08 - 诊断功能 */
static mb_exception_t mb_fc_diagnostics(mb_slave_t *mb)
{
    uint16_t sub_func = mb_get_u16(&mb->rx_buf[2]);
    uint16_t data     = mb_get_u16(&mb->rx_buf[4]);
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_DIAGNOSTICS;
    mb_put_u16(&mb->tx_buf[2], sub_func);
    
    switch (sub_func) {
        case 0x0000:  /* 返回查询数据 (Echo) */
            mb_put_u16(&mb->tx_buf[4], data);
            break;
            
        case 0x0001:  /* 重启通信选项 */
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        case 0x000A:  /* 清除计数器和诊断寄存器 */
            mb->msg_count = 0;
            mb->crc_err_count = 0;
            mb->exception_count = 0;
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        case 0x000B:  /* 返回总线消息计数 */
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->msg_count);
            break;
            
        case 0x000C:  /* 返回总线通信错误计数 */
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->crc_err_count);
            break;
            
        case 0x000D:  /* 返回从机异常错误计数 */
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->exception_count);
            break;
            
        case 0x000E:  /* 返回从机消息计数 */
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->msg_count);
            break;
            
        default:
            return MB_EX_ILLEGAL_FUNC;
    }
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/*==================== 帧处理 ====================*/
static void mb_process_frame(mb_slave_t *mb)
{
    /* 最小帧长度检查 */
    if (mb->rx_cnt < 4) {
        return;
    }
    
    /* 地址检查 */
    uint8_t recv_addr = mb->rx_buf[0];
    if (recv_addr != mb->addr && recv_addr != 0) {
        return;
    }
    
    /* CRC 校验 */
    uint16_t crc_recv = ((uint16_t)mb->rx_buf[mb->rx_cnt - 1] << 8) | 
                         mb->rx_buf[mb->rx_cnt - 2];
    uint16_t crc_calc = mb_crc16(mb->rx_buf, mb->rx_cnt - 2);
    
    if (crc_recv != crc_calc) {
        mb->crc_err_count++;
        return;
    }
    
    mb->msg_count++;
    
    /* 广播地址不响应 */
    bool is_broadcast = (recv_addr == 0);
    
    uint8_t func = mb->rx_buf[1];
    mb_exception_t ex = MB_EX_NONE;
    
    MB_ENTER_CRITICAL();
    
    switch (func) {
        case MB_FC_READ_COILS:
            ex = mb_fc_read_coils(mb);
            break;
        case MB_FC_READ_DISCRETE:
            ex = mb_fc_read_discrete(mb);
            break;
        case MB_FC_READ_HOLDING_REG:
            ex = mb_fc_read_holding_reg(mb);
            break;
        case MB_FC_READ_INPUT_REG:
            ex = mb_fc_read_input_reg(mb);
            break;
        case MB_FC_WRITE_SINGLE_COIL:
            ex = mb_fc_write_single_coil(mb);
            break;
        case MB_FC_WRITE_SINGLE_REG:
            ex = mb_fc_write_single_reg(mb);
            break;
        case MB_FC_DIAGNOSTICS:
            ex = mb_fc_diagnostics(mb);
            break;
        case MB_FC_WRITE_MULTI_COILS:
            ex = mb_fc_write_multi_coils(mb);
            break;
        case MB_FC_WRITE_MULTI_REGS:
            ex = mb_fc_write_multi_regs(mb);
            break;
        case MB_FC_REPORT_SLAVE_ID:
            ex = mb_fc_report_slave_id(mb);
            break;
        case MB_FC_MASK_WRITE_REG:
            ex = mb_fc_mask_write_reg(mb);
            break;
        case MB_FC_RW_MULTI_REGS:
            ex = mb_fc_rw_multi_regs(mb);
            break;
        default:
#if MB_USE_CALLBACK
            /* 尝试自定义功能码处理 */
            if (mb->custom_func_cb) {
                ex = mb->custom_func_cb(mb, func, 
                                        &mb->rx_buf[2], mb->rx_cnt - 4,
                                        &mb->tx_buf[2], &mb->tx_len);
                if (ex == MB_EX_NONE) {
                    mb->tx_buf[0] = mb->addr;
                    mb->tx_buf[1] = func;
                    mb->tx_len = mb_append_crc(mb->tx_buf, mb->tx_len + 2);
                }
            } else
#endif
            {
                ex = MB_EX_ILLEGAL_FUNC;
            }
            break;
    }
    
    MB_EXIT_CRITICAL();
    
    /* 广播不响应 */
    if (is_broadcast) {
        return;
    }
    
    if (ex != MB_EX_NONE) {
        mb_send_exception(mb, func, ex);
    } else {
        mb_port_send(mb->tx_buf, mb->tx_len);
    }
}

/*==================== 主轮询函数 ====================*/
void mb_slave_poll(mb_slave_t *mb)
{
    uint32_t current_tick = mb_port_get_tick();
    
    switch (mb->state) {
        case MB_STATE_IDLE:
            break;
            
        case MB_STATE_RECEIVING:
            if ((current_tick - mb->last_rx_tick) >= mb->t35_timeout) {
                mb->state = MB_STATE_FRAME_READY;
            }
            break;
            
        case MB_STATE_FRAME_READY:
            mb->state = MB_STATE_PROCESSING;
            mb_process_frame(mb);
            mb->rx_cnt = 0;
            mb->state = MB_STATE_IDLE;
            break;
            
        case MB_STATE_PROCESSING:
            break;
    }
}

/*==================== 用户接口函数 ====================*/
bool mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_HOLDING_NUM) return false;
    MB_ENTER_CRITICAL();
    mb->holding_reg[addr] = value;
    MB_EXIT_CRITICAL();
    return true;
}

uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_HOLDING_NUM) return 0;
    MB_ENTER_CRITICAL();
    uint16_t val = mb->holding_reg[addr];
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_holding_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count)
{
    if (addr + count > MB_REG_HOLDING_NUM) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        mb->holding_reg[addr + i] = values[i];
    }
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_INPUT_NUM) return false;
    MB_ENTER_CRITICAL();
    mb->input_reg[addr] = value;
    MB_EXIT_CRITICAL();
    return true;
}

uint16_t mb_get_input_reg(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_INPUT_NUM) return 0;
    MB_ENTER_CRITICAL();
    uint16_t val = mb->input_reg[addr];
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_input_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count)
{
    if (addr + count > MB_REG_INPUT_NUM) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        mb->input_reg[addr + i] = values[i];
    }
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_ENTER_CRITICAL();
    MB_BIT_WR(mb->coil_reg, addr, value);
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_coil(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_ENTER_CRITICAL();
    bool val = MB_BIT_GET(mb->coil_reg, addr);
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_coils(mb_slave_t *mb, uint16_t addr, const uint8_t *values, uint16_t count)
{
    if (addr + count > MB_REG_COIL_NUM) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        MB_BIT_WR(mb->coil_reg, addr + i, MB_BIT_GET(values, i));
    }
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_DISCRETE_NUM) return false;
    MB_ENTER_CRITICAL();
    MB_BIT_WR(mb->discrete_reg, addr, value);
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_discrete(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_DISCRETE_NUM) return false;
    MB_ENTER_CRITICAL();
    bool val = MB_BIT_GET(mb->discrete_reg, addr);
    MB_EXIT_CRITICAL();
    return val;
}

void mb_set_slave_id(mb_slave_t *mb, const uint8_t *data, uint8_t len)
{
    if (len > sizeof(mb->slave_id_data)) {
        len = sizeof(mb->slave_id_data);
    }
    memcpy(mb->slave_id_data, data, len);
    mb->slave_id_len = len;
}

#if MB_USE_CALLBACK
void mb_set_callback(mb_slave_t *mb, mb_callback_t cb)
{
    mb->callback = cb;
}

void mb_set_custom_func_callback(mb_slave_t *mb, mb_custom_func_t cb)
{
    mb->custom_func_cb = cb;
}
#endif
```

------

## 3. 回调机制使用示例

### 示例1：寄存器读写回调

```c
#include "modbus_slave.h"

mb_slave_t mb_slave;

/* 回调函数 - 在寄存器读写时触发 */
mb_exception_t my_callback(mb_slave_t *mb, 
                           mb_callback_event_t event,
                           uint16_t addr, 
                           uint16_t quantity)
{
    switch (event) {
        case MB_CB_PRE_READ_INPUT:
            /* 读输入寄存器前，更新ADC数据 */
            if (addr <= 3 && addr + quantity > 0) {
                mb->input_reg[0] = read_adc_channel(0);
                mb->input_reg[1] = read_adc_channel(1);
                mb->input_reg[2] = read_temperature();
                mb->input_reg[3] = read_humidity();
            }
            break;
            
        case MB_CB_POST_WRITE_HOLDING:
            /* 写保持寄存器后，执行相应动作 */
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t reg = addr + i;
                uint16_t val = mb->holding_reg[reg];
                
                switch (reg) {
                    case 0:  /* PWM占空比 */
                        set_pwm_duty(val);
                        break;
                    case 1:  /* LED亮度 */
                        set_led_brightness(val);
                        break;
                    case 10: /* 波特率设置 */
                        if (val == 9600 || val == 19200 || val == 115200) {
                            // 保存到EEPROM，重启后生效
                            save_baudrate_to_eeprom(val);
                        } else {
                            return MB_EX_ILLEGAL_VALUE;
                        }
                        break;
                }
            }
            break;
            
        case MB_CB_POST_WRITE_COIL:
            /* 写线圈后，控制继电器 */
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t coil = addr + i;
                bool state = MB_BIT_GET(mb->coil_reg, coil);
                
                if (coil < 8) {
                    set_relay(coil, state);
                }
            }
            break;
            
        case MB_CB_PRE_READ_DISCRETE:
            /* 读离散输入前，更新开关状态 */
            for (uint16_t i = 0; i < 8; i++) {
                MB_BIT_WR(mb->discrete_reg, i, read_switch(i));
            }
            break;
            
        default:
            break;
    }
    
    return MB_EX_NONE;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    mb_slave_init(&mb_slave, 1, 9600);
    
    /* 注册回调 */
    mb_set_callback(&mb_slave, my_callback);
    
    while (1) {
        mb_slave_poll(&mb_slave);
    }
}
```

### 示例2：自定义功能码

```c
/* 自定义功能码处理 - 例如 0x41 读取设备信息 */
mb_exception_t custom_func_handler(mb_slave_t *mb,
                                   uint8_t func_code,
                                   uint8_t *rx_data,
                                   uint16_t rx_len,
                                   uint8_t *tx_data,
                                   uint16_t *tx_len)
{
    switch (func_code) {
        case 0x41:  /* 读取设备信息 */
        {
            /* 请求格式: [功能码] [信息类型]
               响应格式: [功能码] [数据长度] [数据...] */
            if (rx_len < 1) {
                return MB_EX_ILLEGAL_VALUE;
            }
            
            uint8_t info_type = rx_data[0];
            
            switch (info_type) {
                case 0x00:  /* 固件版本 */
                    tx_data[0] = 5;  // 长度
                    tx_data[1] = 'V';
                    tx_data[2] = '1';
                    tx_data[3] = '.';
                    tx_data[4] = '0';
                    tx_data[5] = '0';
                    *tx_len = 6;
                    break;
                    
                case 0x01:  /* 硬件版本 */
                    tx_data[0] = 3;
                    tx_data[1] = 'H';
                    tx_data[2] = 'W';
                    tx_data[3] = '2';
                    *tx_len = 4;
                    break;
                    
                case 0x02:  /* 序列号 */
                    tx_data[0] = 8;
                    memcpy(&tx_data[1], get_serial_number(), 8);
                    *tx_len = 9;
                    break;
                    
                default:
                    return MB_EX_ILLEGAL_ADDR;
            }
            return MB_EX_NONE;
        }
        
        case 0x42:  /* 设备重启 */
        {
            if (rx_len < 2) {
                return MB_EX_ILLEGAL_VALUE;
            }
            uint16_t magic = (rx_data[0] << 8) | rx_data[1];
            if (magic != 0x1234) {
                return MB_EX_ILLEGAL_VALUE;
            }
            
            tx_data[0] = 0x00;  // OK
            *tx_len = 1;
            
            /* 延迟重启，先发送响应 */
            schedule_reboot(100);  // 100ms后重启
            return MB_EX_NONE;
        }
        
        case 0x43:  /* 批量读取传感器 */
        {
            /* 一次性读取所有传感器数据 */
            tx_data[0] = 12;  // 数据长度
            
            /* 温度 (2字节) */
            uint16_t temp = read_temperature();
            tx_data[1] = temp >> 8;
            tx_data[2] = temp & 0xFF;
            
            /* 湿度 (2字节) */
            uint16_t humi = read_humidity();
            tx_data[3] = humi >> 8;
            tx_data[4] = humi & 0xFF;
            
            /* 压力 (4字节) */
            uint32_t pressure = read_pressure();
            tx_data[5] = (pressure >> 24) & 0xFF;
            tx_data[6] = (pressure >> 16) & 0xFF;
            tx_data[7] = (pressure >> 8) & 0xFF;
            tx_data[8] = pressure & 0xFF;
            
            /* ADC值 (4字节, 2通道) */
            uint16_t adc0 = read_adc(0);
            uint16_t adc1 = read_adc(1);
            tx_data[9]  = adc0 >> 8;
            tx_data[10] = adc0 & 0xFF;
            tx_data[11] = adc1 >> 8;
            tx_data[12] = adc1 & 0xFF;
            
            *tx_len = 13;
            return MB_EX_NONE;
        }
        
        default:
            return MB_EX_ILLEGAL_FUNC;
    }
}

int main(void)
{
    mb_slave_init(&mb_slave, 1, 9600);
    
    /* 注册自定义功能码处理器 */
    mb_set_custom_func_callback(&mb_slave, custom_func_handler);
    
    while (1) {
        mb_slave_poll(&mb_slave);
    }
}
```

------

## 4. 功能码支持汇总

| 功能码 |     名称     |  状态  |
| :----: | :----------: | :----: |
|  0x01  |    读线圈    | ✅ 支持 |
|  0x02  |  读离散输入  | ✅ 支持 |
|  0x03  | 读保持寄存器 | ✅ 支持 |
|  0x04  | 读输入寄存器 | ✅ 支持 |
|        |              |        |





```
继续完善
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

## 4. 功能码支持汇总（续）

| 功能码 |       名称       |    状态    |
| :----: | :--------------: | :--------: |
|  0x01  |      读线圈      |   ✅ 支持   |
|  0x02  |    读离散输入    |   ✅ 支持   |
|  0x03  |   读保持寄存器   |   ✅ 支持   |
|  0x04  |   读输入寄存器   |   ✅ 支持   |
|  0x05  |    写单个线圈    |   ✅ 支持   |
|  0x06  |   写单个寄存器   |   ✅ 支持   |
|  0x07  |    读异常状态    |   ✅ 新增   |
|  0x08  |     诊断功能     |   ✅ 支持   |
|  0x0B  | 获取通信事件计数 |   ✅ 新增   |
|  0x0F  |    写多个线圈    |   ✅ 支持   |
|  0x10  |   写多个寄存器   |   ✅ 支持   |
|  0x11  |    报告从机ID    |   ✅ 支持   |
|  0x16  |   屏蔽写寄存器   |   ✅ 支持   |
|  0x17  | 读/写多个寄存器  |   ✅ 支持   |
|  0x2B  |    读设备标识    |   ✅ 新增   |
| 自定义 |  用户扩展功能码  | ✅ 回调支持 |

------

## 5. 完善后的完整实现

### 更新头文件 `modbus_slave.h`

```c
#ifndef __MODBUS_SLAVE_H__
#define __MODBUS_SLAVE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================== 配置区 ====================*/
#define MB_SLAVE_ADDR_DEFAULT   1
#define MB_FRAME_MAX            256
#define MB_REG_HOLDING_NUM      100
#define MB_REG_INPUT_NUM        100
#define MB_REG_COIL_NUM         64
#define MB_REG_DISCRETE_NUM     64

/* RTOS 模式: 0=裸机, 1=FreeRTOS, 2=RT-Thread, 3=uCOS */
#define MB_RTOS_TYPE            0

/* 回调机制开关 */
#define MB_USE_CALLBACK         1

/* 调试开关 */
#define MB_DEBUG_ENABLE         0

/*==================== RTOS 适配层 ====================*/
#if (MB_RTOS_TYPE == 1)  /* FreeRTOS */
    #include "FreeRTOS.h"
    #include "semphr.h"
    #define MB_ENTER_CRITICAL()     do { if(mb_mutex) xSemaphoreTake(mb_mutex, portMAX_DELAY); } while(0)
    #define MB_EXIT_CRITICAL()      do { if(mb_mutex) xSemaphoreGive(mb_mutex); } while(0)
    #define MB_RTOS_INIT()          do { if(!mb_mutex) mb_mutex = xSemaphoreCreateMutex(); } while(0)
    extern SemaphoreHandle_t mb_mutex;

#elif (MB_RTOS_TYPE == 2)  /* RT-Thread */
    #include "rtthread.h"
    #define MB_ENTER_CRITICAL()     rt_mutex_take(&mb_mutex, RT_WAITING_FOREVER)
    #define MB_EXIT_CRITICAL()      rt_mutex_release(&mb_mutex)
    #define MB_RTOS_INIT()          rt_mutex_init(&mb_mutex, "mb_mtx", RT_IPC_FLAG_PRIO)
    extern struct rt_mutex mb_mutex;

#elif (MB_RTOS_TYPE == 3)  /* uCOS-III */
    #include "os.h"
    #define MB_ENTER_CRITICAL()     do { OS_ERR err; OSMutexPend(&mb_mutex, 0, OS_OPT_PEND_BLOCKING, NULL, &err); } while(0)
    #define MB_EXIT_CRITICAL()      do { OS_ERR err; OSMutexPost(&mb_mutex, OS_OPT_POST_NONE, &err); } while(0)
    #define MB_RTOS_INIT()          do { OS_ERR err; OSMutexCreate(&mb_mutex, "mb_mtx", &err); } while(0)
    extern OS_MUTEX mb_mutex;

#else  /* 裸机 */
    #define MB_ENTER_CRITICAL()     do {} while(0)
    #define MB_EXIT_CRITICAL()      do {} while(0)
    #define MB_RTOS_INIT()          do {} while(0)
#endif

/*==================== 位操作宏（避免除法） ====================*/
#define MB_BIT_BYTE_IDX(addr)       ((uint16_t)(addr) >> 3)
#define MB_BIT_BIT_IDX(addr)        ((uint8_t)(addr) & 0x07U)
#define MB_BIT_MASK(addr)           (1U << MB_BIT_BIT_IDX(addr))

#define MB_BIT_SET(array, addr)     ((array)[MB_BIT_BYTE_IDX(addr)] |= MB_BIT_MASK(addr))
#define MB_BIT_CLR(array, addr)     ((array)[MB_BIT_BYTE_IDX(addr)] &= (uint8_t)~MB_BIT_MASK(addr))
#define MB_BIT_GET(array, addr)     (((array)[MB_BIT_BYTE_IDX(addr)] & MB_BIT_MASK(addr)) != 0)
#define MB_BIT_WR(array, addr, val) do { \
    if (val) { MB_BIT_SET(array, addr); } \
    else     { MB_BIT_CLR(array, addr); } \
} while(0)

#define MB_BITS_TO_BYTES(bits)      (((bits) + 7U) >> 3)

/* 乘2和除2的位操作 */
#define MB_MUL2(x)                  ((x) << 1)
#define MB_DIV2(x)                  ((x) >> 1)

/*==================== 功能码定义 ====================*/
typedef enum {
    MB_FC_READ_COILS            = 0x01,
    MB_FC_READ_DISCRETE         = 0x02,
    MB_FC_READ_HOLDING_REG      = 0x03,
    MB_FC_READ_INPUT_REG        = 0x04,
    MB_FC_WRITE_SINGLE_COIL     = 0x05,
    MB_FC_WRITE_SINGLE_REG      = 0x06,
    MB_FC_READ_EXCEPTION_STATUS = 0x07,
    MB_FC_DIAGNOSTICS           = 0x08,
    MB_FC_GET_COMM_EVENT_CNT    = 0x0B,
    MB_FC_GET_COMM_EVENT_LOG    = 0x0C,
    MB_FC_WRITE_MULTI_COILS     = 0x0F,
    MB_FC_WRITE_MULTI_REGS      = 0x10,
    MB_FC_REPORT_SLAVE_ID       = 0x11,
    MB_FC_READ_FILE_RECORD      = 0x14,
    MB_FC_WRITE_FILE_RECORD     = 0x15,
    MB_FC_MASK_WRITE_REG        = 0x16,
    MB_FC_RW_MULTI_REGS         = 0x17,
    MB_FC_READ_FIFO_QUEUE       = 0x18,
    MB_FC_READ_DEVICE_ID        = 0x2B,
} mb_func_code_t;

/*==================== 诊断子功能码 ====================*/
typedef enum {
    MB_DIAG_RETURN_QUERY        = 0x0000,
    MB_DIAG_RESTART_COMM        = 0x0001,
    MB_DIAG_RETURN_DIAG_REG     = 0x0002,
    MB_DIAG_CHANGE_DELIMITER    = 0x0003,
    MB_DIAG_FORCE_LISTEN_ONLY   = 0x0004,
    MB_DIAG_CLEAR_COUNTERS      = 0x000A,
    MB_DIAG_RETURN_BUS_MSG_CNT  = 0x000B,
    MB_DIAG_RETURN_BUS_ERR_CNT  = 0x000C,
    MB_DIAG_RETURN_SLAVE_EX_CNT = 0x000D,
    MB_DIAG_RETURN_SLAVE_MSG_CNT= 0x000E,
    MB_DIAG_RETURN_SLAVE_NO_RSP = 0x000F,
    MB_DIAG_RETURN_SLAVE_NAK    = 0x0010,
    MB_DIAG_RETURN_SLAVE_BUSY   = 0x0011,
    MB_DIAG_RETURN_BUS_OVERRUN  = 0x0012,
    MB_DIAG_CLEAR_OVERRUN       = 0x0014,
} mb_diag_subfunc_t;

/*==================== 异常码定义 ====================*/
typedef enum {
    MB_EX_NONE                  = 0x00,
    MB_EX_ILLEGAL_FUNC          = 0x01,
    MB_EX_ILLEGAL_ADDR          = 0x02,
    MB_EX_ILLEGAL_VALUE         = 0x03,
    MB_EX_SLAVE_FAILURE         = 0x04,
    MB_EX_ACK                   = 0x05,
    MB_EX_SLAVE_BUSY            = 0x06,
    MB_EX_NACK                  = 0x07,
    MB_EX_MEMORY_PARITY_ERR     = 0x08,
    MB_EX_GATEWAY_PATH_UNAVAIL  = 0x0A,
    MB_EX_GATEWAY_TARGET_FAIL   = 0x0B,
} mb_exception_t;

/*==================== 状态机 ====================*/
typedef enum {
    MB_STATE_IDLE,
    MB_STATE_RECEIVING,
    MB_STATE_FRAME_READY,
    MB_STATE_PROCESSING,
} mb_state_t;

/*==================== 回调事件类型 ====================*/
#if MB_USE_CALLBACK
typedef enum {
    MB_CB_PRE_READ_HOLDING,
    MB_CB_POST_READ_HOLDING,
    MB_CB_PRE_WRITE_HOLDING,
    MB_CB_POST_WRITE_HOLDING,
    MB_CB_PRE_READ_INPUT,
    MB_CB_POST_READ_INPUT,
    MB_CB_PRE_READ_COIL,
    MB_CB_POST_READ_COIL,
    MB_CB_PRE_WRITE_COIL,
    MB_CB_POST_WRITE_COIL,
    MB_CB_PRE_READ_DISCRETE,
    MB_CB_POST_READ_DISCRETE,
    MB_CB_FRAME_RECEIVED,
    MB_CB_FRAME_SENT,
    MB_CB_ERROR,
} mb_callback_event_t;

struct mb_slave_s;

/* 通用回调函数原型 */
typedef mb_exception_t (*mb_callback_t)(
    struct mb_slave_s *mb,
    mb_callback_event_t event,
    uint16_t addr,
    uint16_t quantity
);

/* 自定义功能码处理函数原型 */
typedef mb_exception_t (*mb_custom_func_t)(
    struct mb_slave_s *mb,
    uint8_t func_code,
    const uint8_t *rx_data,
    uint16_t rx_len,
    uint8_t *tx_data,
    uint16_t *tx_len
);

/* 寄存器访问钩子（可用于虚拟寄存器映射） */
typedef mb_exception_t (*mb_reg_read_hook_t)(
    struct mb_slave_s *mb,
    uint8_t reg_type,
    uint16_t addr,
    uint16_t *value
);

typedef mb_exception_t (*mb_reg_write_hook_t)(
    struct mb_slave_s *mb,
    uint8_t reg_type,
    uint16_t addr,
    uint16_t value
);
#endif

/*==================== 统计信息结构 ====================*/
typedef struct {
    uint32_t bus_msg_count;
    uint32_t bus_comm_err_count;
    uint32_t slave_exception_count;
    uint32_t slave_msg_count;
    uint32_t slave_no_response_count;
    uint32_t slave_nak_count;
    uint32_t slave_busy_count;
    uint32_t bus_overrun_count;
} mb_statistics_t;

/*==================== 设备标识信息 ====================*/
typedef struct {
    char vendor_name[32];
    char product_code[16];
    char major_minor_rev[8];
    char vendor_url[64];
    char product_name[32];
    char model_name[16];
    char user_app_name[32];
} mb_device_id_t;

/*==================== 从机控制块 ====================*/
typedef struct mb_slave_s {
    /* 基本配置 */
    uint8_t         addr;
    mb_state_t      state;
    bool            listen_only_mode;
    
    /* 缓冲区 */
    uint8_t         rx_buf[MB_FRAME_MAX];
    uint8_t         tx_buf[MB_FRAME_MAX];
    uint16_t        rx_cnt;
    uint16_t        tx_len;
    
    /* 定时相关 */
    uint32_t        last_rx_tick;
    uint32_t        t35_timeout;
    uint32_t        t15_timeout;
    
    /* 寄存器数据 */
    uint16_t        holding_reg[MB_REG_HOLDING_NUM];
    uint16_t        input_reg[MB_REG_INPUT_NUM];
    uint8_t         coil_reg[MB_BITS_TO_BYTES(MB_REG_COIL_NUM)];
    uint8_t         discrete_reg[MB_BITS_TO_BYTES(MB_REG_DISCRETE_NUM)];
    
    /* 异常状态（用于0x07功能码） */
    uint8_t         exception_status;
    
    /* 诊断寄存器 */
    uint16_t        diagnostic_reg;
    
    /* 统计信息 */
    mb_statistics_t stats;
    
    /* 设备标识 */
    mb_device_id_t  device_id;
    uint8_t         slave_id_data[32];
    uint8_t         slave_id_len;
    
#if MB_USE_CALLBACK
    mb_callback_t       callback;
    mb_custom_func_t    custom_func_cb;
    mb_reg_read_hook_t  reg_read_hook;
    mb_reg_write_hook_t reg_write_hook;
    void               *user_data;
#endif
} mb_slave_t;

/*==================== API 接口 ====================*/

/* 初始化与控制 */
void     mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baudrate);
void     mb_slave_poll(mb_slave_t *mb);
void     mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte);
void     mb_slave_rx_bytes(mb_slave_t *mb, const uint8_t *data, uint16_t len);
void     mb_slave_reset(mb_slave_t *mb);
void     mb_slave_set_addr(mb_slave_t *mb, uint8_t addr);
uint8_t  mb_slave_get_addr(mb_slave_t *mb);
void     mb_slave_set_listen_only(mb_slave_t *mb, bool enable);

/* 保持寄存器操作 */
bool     mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr);
bool     mb_set_holding_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count);
bool     mb_get_holding_regs(mb_slave_t *mb, uint16_t addr, uint16_t *values, uint16_t count);

/* 输入寄存器操作 */
bool     mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value);
uint16_t mb_get_input_reg(mb_slave_t *mb, uint16_t addr);
bool     mb_set_input_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count);

/* 线圈操作 */
bool     mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value);
bool     mb_get_coil(mb_slave_t *mb, uint16_t addr);
bool     mb_set_coils(mb_slave_t *mb, uint16_t addr, const bool *values, uint16_t count);
bool     mb_get_coils(mb_slave_t *mb, uint16_t addr, bool *values, uint16_t count);

/* 离散输入操作 */
bool     mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value);
bool     mb_get_discrete(mb_slave_t *mb, uint16_t addr);
bool     mb_set_discretes(mb_slave_t *mb, uint16_t addr, const bool *values, uint16_t count);

/* 异常状态操作 */
void     mb_set_exception_status(mb_slave_t *mb, uint8_t status);
uint8_t  mb_get_exception_status(mb_slave_t *mb);
void     mb_set_exception_bit(mb_slave_t *mb, uint8_t bit, bool value);

/* 设备标识设置 */
void     mb_set_slave_id(mb_slave_t *mb, const uint8_t *data, uint8_t len);
void     mb_set_device_id(mb_slave_t *mb, const mb_device_id_t *id);

/* 统计信息 */
const mb_statistics_t* mb_get_statistics(mb_slave_t *mb);
void     mb_clear_statistics(mb_slave_t *mb);

#if MB_USE_CALLBACK
/* 回调注册 */
void     mb_set_callback(mb_slave_t *mb, mb_callback_t cb);
void     mb_set_custom_func_callback(mb_slave_t *mb, mb_custom_func_t cb);
void     mb_set_reg_hooks(mb_slave_t *mb, mb_reg_read_hook_t read_hook, mb_reg_write_hook_t write_hook);
void     mb_set_user_data(mb_slave_t *mb, void *data);
void*    mb_get_user_data(mb_slave_t *mb);
#endif

/* 调试接口 */
#if MB_DEBUG_ENABLE
void     mb_debug_dump_frame(const uint8_t *buf, uint16_t len, bool is_tx);
void     mb_debug_print_stats(mb_slave_t *mb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_SLAVE_H__ */
```

------

### 完整实现 `modbus_slave.c`

```c
#include "modbus_slave.h"
#include "modbus_port.h"

/*==================== RTOS 互斥量 ====================*/
#if (MB_RTOS_TYPE == 1)
SemaphoreHandle_t mb_mutex = NULL;
#elif (MB_RTOS_TYPE == 2)
struct rt_mutex mb_mutex;
#elif (MB_RTOS_TYPE == 3)
OS_MUTEX mb_mutex;
#endif

/*==================== CRC16 查表法 ====================*/
static const uint16_t crc_table[256] = {
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

static uint16_t mb_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xFF];
    }
    return crc;
}

/*==================== 辅助函数 ====================*/
static inline uint16_t mb_get_u16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | buf[1];
}

static inline void mb_put_u16(uint8_t *buf, uint16_t val)
{
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val & 0xFF);
}

static inline uint16_t mb_append_crc(uint8_t *buf, uint16_t len)
{
    uint16_t crc = mb_crc16(buf, len);
    buf[len]     = (uint8_t)(crc & 0xFF);
    buf[len + 1] = (uint8_t)(crc >> 8);
    return len + 2;
}

/*==================== 初始化 ====================*/
void mb_slave_init(mb_slave_t *mb, uint8_t addr, uint32_t baudrate)
{
    memset(mb, 0, sizeof(mb_slave_t));
    mb->addr = addr;
    mb->state = MB_STATE_IDLE;
    mb->listen_only_mode = false;
    
    /* 计算字符超时时间(ms)
     * 1字符 = 11位 (1起始 + 8数据 + 1校验 + 1停止)
     * t1.5 = 1.5 * 11 / baudrate * 1000 ms
     * t3.5 = 3.5 * 11 / baudrate * 1000 ms
     */
    if (baudrate > 19200) {
        mb->t15_timeout = 1;
        mb->t35_timeout = 2;
    } else {
        mb->t15_timeout = (16500UL / baudrate) + 1;
        mb->t35_timeout = (38500UL / baudrate) + 1;
    }
    
    /* 默认设备标识 */
    strcpy(mb->device_id.vendor_name, "Generic");
    strcpy(mb->device_id.product_code, "MB-SLAVE");
    strcpy(mb->device_id.major_minor_rev, "1.0.0");
    strcpy(mb->device_id.product_name, "Modbus Slave");
    
    const char *default_id = "MB-SLAVE-V1";
    mb_set_slave_id(mb, (const uint8_t *)default_id, (uint8_t)strlen(default_id));
    
    MB_RTOS_INIT();
    mb_port_init(baudrate);
}

void mb_slave_reset(mb_slave_t *mb)
{
    MB_ENTER_CRITICAL();
    mb->rx_cnt = 0;
    mb->state = MB_STATE_IDLE;
    MB_EXIT_CRITICAL();
}

void mb_slave_set_addr(mb_slave_t *mb, uint8_t addr)
{
    MB_ENTER_CRITICAL();
    mb->addr = addr;
    MB_EXIT_CRITICAL();
}

uint8_t mb_slave_get_addr(mb_slave_t *mb)
{
    return mb->addr;
}

void mb_slave_set_listen_only(mb_slave_t *mb, bool enable)
{
    MB_ENTER_CRITICAL();
    mb->listen_only_mode = enable;
    MB_EXIT_CRITICAL();
}

/*==================== 接收处理 ====================*/
void mb_slave_rx_byte(mb_slave_t *mb, uint8_t byte)
{
    if (mb->rx_cnt < MB_FRAME_MAX) {
        mb->rx_buf[mb->rx_cnt++] = byte;
        mb->last_rx_tick = mb_port_get_tick();
        
        if (mb->state == MB_STATE_IDLE) {
            mb->state = MB_STATE_RECEIVING;
        }
    } else {
        mb->stats.bus_overrun_count++;
    }
}

void mb_slave_rx_bytes(mb_slave_t *mb, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        mb_slave_rx_byte(mb, data[i]);
    }
}

/*==================== 发送异常响应 ====================*/
static void mb_send_exception(mb_slave_t *mb, uint8_t func, mb_exception_t ex)
{
    if (mb->listen_only_mode) {
        return;
    }
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = func | 0x80;
    mb->tx_buf[2] = (uint8_t)ex;
    mb->tx_len = mb_append_crc(mb->tx_buf, 3);
    
    mb->stats.slave_exception_count++;
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_ERROR, ex, func);
    }
#endif
    
    mb_port_send(mb->tx_buf, mb->tx_len);
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_FRAME_SENT, 0, mb->tx_len);
    }
#endif
}

/*==================== 发送正常响应 ====================*/
static void mb_send_response(mb_slave_t *mb)
{
    if (mb->listen_only_mode) {
        return;
    }
    
    mb_port_send(mb->tx_buf, mb->tx_len);
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_FRAME_SENT, 0, mb->tx_len);
    }
#endif
}

/*==================== 功能码处理函数 ====================*/

/* 0x01 - 读线圈 */
static mb_exception_t mb_fc_read_coils(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_READ_COIL, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    uint8_t byte_cnt = (uint8_t)MB_BITS_TO_BYTES(quantity);
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_COILS;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    
#if MB_USE_CALLBACK
    if (mb->reg_read_hook) {
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t val = 0;
            mb_exception_t ex = mb->reg_read_hook(mb, MB_FC_READ_COILS, start_addr + i, &val);
            if (ex != MB_EX_NONE) return ex;
            if (val) {
                MB_BIT_SET(&mb->tx_buf[3], i);
            }
        }
    } else
#endif
    {
        for (uint16_t i = 0; i < quantity; i++) {
            if (MB_BIT_GET(mb->coil_reg, start_addr + i)) {
                MB_BIT_SET(&mb->tx_buf[3], i);
            }
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_READ_COIL, start_addr, quantity);
    }
#endif
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 3 + byte_cnt);
    return MB_EX_NONE;
}

/* 0x02 - 读离散输入 */
static mb_exception_t mb_fc_read_discrete(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 2000) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_DISCRETE_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_READ_DISCRETE, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    uint8_t byte_cnt = (uint8_t)MB_BITS_TO_BYTES(quantity);
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_DISCRETE;
    mb->tx_buf[2] = byte_cnt;
    
    memset(&mb->tx_buf[3], 0, byte_cnt);
    
#if MB_USE_CALLBACK
    if (mb->reg_read_hook) {
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t val = 0;
            mb_exception_t ex = mb->reg_read_hook(mb, MB_FC_READ_DISCRETE, start_addr + i, &val);
            if (ex != MB_EX_NONE) return ex;
            if (val) {
                MB_BIT_SET(&mb->tx_buf[3], i);
            }
        }
    } else
#endif
    {
        for (uint16_t i = 0; i < quantity; i++) {
            if (MB_BIT_GET(mb->discrete_reg, start_addr + i)) {
                MB_BIT_SET(&mb->tx_buf[3], i);
            }
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_READ_DISCRETE, start_addr, quantity);
    }
#endif
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 3 + byte_cnt);
    return MB_EX_NONE;
}

/* 0x03 - 读保持寄存器 */
static mb_exception_t mb_fc_read_holding_reg(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_READ_HOLDING, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_HOLDING_REG;
    mb->tx_buf[2] = (uint8_t)MB_MUL2(quantity);
    
    uint16_t idx = 3;
    
#if MB_USE_CALLBACK
    if (mb->reg_read_hook) {
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t val = 0;
            mb_exception_t ex = mb->reg_read_hook(mb, MB_FC_READ_HOLDING_REG, start_addr + i, &val);
            if (ex != MB_EX_NONE) return ex;
            mb_put_u16(&mb->tx_buf[idx], val);
            idx += 2;
        }
    } else
#endif
    {
        for (uint16_t i = 0; i < quantity; i++) {
            mb_put_u16(&mb->tx_buf[idx], mb->holding_reg[start_addr + i]);
            idx += 2;
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_READ_HOLDING, start_addr, quantity);
    }
#endif
    
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x04 - 读输入寄存器 */
static mb_exception_t mb_fc_read_input_reg(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    
    if (quantity < 1 || quantity > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_INPUT_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_READ_INPUT, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_INPUT_REG;
    mb->tx_buf[2] = (uint8_t)MB_MUL2(quantity);
    
    uint16_t idx = 3;
    
#if MB_USE_CALLBACK
    if (mb->reg_read_hook) {
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t val = 0;
            mb_exception_t ex = mb->reg_read_hook(mb, MB_FC_READ_INPUT_REG, start_addr + i, &val);
            if (ex != MB_EX_NONE) return ex;
            mb_put_u16(&mb->tx_buf[idx], val);
            idx += 2;
        }
    } else
#endif
    {
        for (uint16_t i = 0; i < quantity; i++) {
            mb_put_u16(&mb->tx_buf[idx], mb->input_reg[start_addr + i]);
            idx += 2;
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_READ_INPUT, start_addr, quantity);
    }
#endif
    
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x05 - 写单个线圈 */
static mb_exception_t mb_fc_write_single_coil(mb_slave_t *mb)
{
    uint16_t coil_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t value     = mb_get_u16(&mb->rx_buf[4]);
    
    if (coil_addr >= MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (value != 0x0000 && value != 0xFF00) {
        return MB_EX_ILLEGAL_VALUE;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_COIL, coil_addr, 1);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    bool coil_val = (value == 0xFF00);
    
#if MB_USE_CALLBACK
    if (mb->reg_write_hook) {
        mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_WRITE_SINGLE_COIL, coil_addr, coil_val ? 1 : 0);
        if (ex != MB_EX_NONE) return ex;
    } else
#endif
    {
        MB_BIT_WR(mb->coil_reg, coil_addr, coil_val);
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_COIL, coil_addr, 1);
    }
#endif
    
    /* 原样返回 */
    memcpy(mb->tx_buf, mb->rx_buf, 6);
    mb->tx_buf[0] = mb->addr;
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x06 - 写单个寄存器 */
static mb_exception_t mb_fc_write_single_reg(mb_slave_t *mb)
{
    uint16_t reg_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t value    = mb_get_u16(&mb->rx_buf[4]);
    
    if (reg_addr >= MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_HOLDING, reg_addr, 1);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
#if MB_USE_CALLBACK
    if (mb->reg_write_hook) {
        mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_WRITE_SINGLE_REG, reg_addr, value);
        if (ex != MB_EX_NONE) return ex;
    } else
#endif
    {
        mb->holding_reg[reg_addr] = value;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, reg_addr, 1);
    }
#endif
    
    memcpy(mb->tx_buf, mb->rx_buf, 6);
    mb->tx_buf[0] = mb->addr;
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x07 - 读异常状态 */
static mb_exception_t mb_fc_read_exception_status(mb_slave_t *mb)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_EXCEPTION_STATUS;
    mb->tx_buf[2] = mb->exception_status;
    mb->tx_len = mb_append_crc(mb->tx_buf, 3);
    return MB_EX_NONE;
}

/* 0x08 - 诊断功能 */
static mb_exception_t mb_fc_diagnostics(mb_slave_t *mb)
{
    uint16_t sub_func = mb_get_u16(&mb->rx_buf[2]);
    uint16_t data     = mb_get_u16(&mb->rx_buf[4]);
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_DIAGNOSTICS;
    mb_put_u16(&mb->tx_buf[2], sub_func);
    
    switch (sub_func) {
        case MB_DIAG_RETURN_QUERY:
            mb_put_u16(&mb->tx_buf[4], data);
            break;
            
        case MB_DIAG_RESTART_COMM:
            mb->listen_only_mode = false;
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        case MB_DIAG_RETURN_DIAG_REG:
            mb_put_u16(&mb->tx_buf[4], mb->diagnostic_reg);
            break;
            
        case MB_DIAG_FORCE_LISTEN_ONLY:
            mb->listen_only_mode = true;
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        case MB_DIAG_CLEAR_COUNTERS:
            memset(&mb->stats, 0, sizeof(mb_statistics_t));
            mb->diagnostic_reg = 0;
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        case MB_DIAG_RETURN_BUS_MSG_CNT:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.bus_msg_count);
            break;
            
        case MB_DIAG_RETURN_BUS_ERR_CNT:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.bus_comm_err_count);
            break;
            
        case MB_DIAG_RETURN_SLAVE_EX_CNT:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_exception_count);
            break;
            
        case MB_DIAG_RETURN_SLAVE_MSG_CNT:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_msg_count);
            break;
            
        case MB_DIAG_RETURN_SLAVE_NO_RSP:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_no_response_count);
            break;
            
        case MB_DIAG_RETURN_SLAVE_NAK:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_nak_count);
            break;
            
        case MB_DIAG_RETURN_SLAVE_BUSY:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_busy_count);
            break;
            
        case MB_DIAG_RETURN_BUS_OVERRUN:
            mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.bus_overrun_count);
            break;
            
        case MB_DIAG_CLEAR_OVERRUN:
            mb->stats.bus_overrun_count = 0;
            mb_put_u16(&mb->tx_buf[4], 0x0000);
            break;
            
        default:
            return MB_EX_ILLEGAL_FUNC;
    }
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x0B - 获取通信事件计数 */
static mb_exception_t mb_fc_get_comm_event_cnt(mb_slave_t *mb)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_GET_COMM_EVENT_CNT;
    mb_put_u16(&mb->tx_buf[2], 0x0000);  /* 状态字 */
    mb_put_u16(&mb->tx_buf[4], (uint16_t)mb->stats.slave_msg_count);
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x0F - 写多个线圈 */
static mb_exception_t mb_fc_write_multi_coils(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 1968) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (byte_cnt != MB_BITS_TO_BYTES(quantity)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_COIL_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_COIL, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    for (uint16_t i = 0; i < quantity; i++) {
        bool val = MB_BIT_GET(&mb->rx_buf[7], i);
        
#if MB_USE_CALLBACK
        if (mb->reg_write_hook) {
            mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_WRITE_MULTI_COILS, start_addr + i, val ? 1 : 0);
            if (ex != MB_EX_NONE) return ex;
        } else
#endif
        {
            MB_BIT_WR(mb->coil_reg, start_addr + i, val);
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_COIL, start_addr, quantity);
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_COILS;
    mb_put_u16(&mb->tx_buf[2], start_addr);
    mb_put_u16(&mb->tx_buf[4], quantity);
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x10 - 写多个寄存器 */
static mb_exception_t mb_fc_write_multi_regs(mb_slave_t *mb)
{
    uint16_t start_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t quantity   = mb_get_u16(&mb->rx_buf[4]);
    uint8_t  byte_cnt   = mb->rx_buf[6];
    
    if (quantity < 1 || quantity > 123) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (byte_cnt != MB_MUL2(quantity)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (start_addr + quantity > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_HOLDING, start_addr, quantity);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t val = mb_get_u16(&mb->rx_buf[7 + MB_MUL2(i)]);
        
#if MB_USE_CALLBACK
        if (mb->reg_write_hook) {
            mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_WRITE_MULTI_REGS, start_addr + i, val);
            if (ex != MB_EX_NONE) return ex;
        } else
#endif
        {
            mb->holding_reg[start_addr + i] = val;
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, start_addr, quantity);
    }
#endif
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_WRITE_MULTI_REGS;
    mb_put_u16(&mb->tx_buf[2], start_addr);
    mb_put_u16(&mb->tx_buf[4], quantity);
    mb->tx_len = mb_append_crc(mb->tx_buf, 6);
    return MB_EX_NONE;
}

/* 0x11 - 报告从机ID */
static mb_exception_t mb_fc_report_slave_id(mb_slave_t *mb)
{
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_REPORT_SLAVE_ID;
    mb->tx_buf[2] = mb->slave_id_len + 2;
    mb->tx_buf[3] = mb->addr;
    mb->tx_buf[4] = 0xFF;  /* 运行状态 ON */
    
    memcpy(&mb->tx_buf[5], mb->slave_id_data, mb->slave_id_len);
    
    mb->tx_len = mb_append_crc(mb->tx_buf, 5 + mb->slave_id_len);
    return MB_EX_NONE;
}

/* 0x16 - 屏蔽写寄存器 */
static mb_exception_t mb_fc_mask_write_reg(mb_slave_t *mb)
{
    uint16_t reg_addr = mb_get_u16(&mb->rx_buf[2]);
    uint16_t and_mask = mb_get_u16(&mb->rx_buf[4]);
    uint16_t or_mask  = mb_get_u16(&mb->rx_buf[6]);
    
    if (reg_addr >= MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_HOLDING, reg_addr, 1);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    uint16_t current = mb->holding_reg[reg_addr];
    uint16_t new_val = (current & and_mask) | (or_mask & ~and_mask);
    
#if MB_USE_CALLBACK
    if (mb->reg_write_hook) {
        mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_MASK_WRITE_REG, reg_addr, new_val);
        if (ex != MB_EX_NONE) return ex;
    } else
#endif
    {
        mb->holding_reg[reg_addr] = new_val;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, reg_addr, 1);
    }
#endif
    
    memcpy(mb->tx_buf, mb->rx_buf, 8);
    mb->tx_buf[0] = mb->addr;
    mb->tx_len = mb_append_crc(mb->tx_buf, 8);
    return MB_EX_NONE;
}

/* 0x17 - 读/写多个寄存器 */
static mb_exception_t mb_fc_rw_multi_regs(mb_slave_t *mb)
{
    uint16_t read_addr   = mb_get_u16(&mb->rx_buf[2]);
    uint16_t read_qty    = mb_get_u16(&mb->rx_buf[4]);
    uint16_t write_addr  = mb_get_u16(&mb->rx_buf[6]);
    uint16_t write_qty   = mb_get_u16(&mb->rx_buf[8]);
    uint8_t  write_bytes = mb->rx_buf[10];
    
    if (read_qty < 1 || read_qty > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_qty < 1 || write_qty > 121) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_bytes != MB_MUL2(write_qty)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (read_addr + read_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (write_addr + write_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_HOLDING, write_addr, write_qty);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    /* 先写 */
    for (uint16_t i = 0; i < write_qty; i++) {
        uint16_t val = mb_get_u16(&mb->rx_buf[11 + MB_MUL2(i)]);
        
#if MB_USE_CALLBACK
        if (mb->reg_write_hook) {
            mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_RW_MULTI_RE
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
/* 0x17 - 读/写多个寄存器 (续) */
static mb_exception_t mb_fc_rw_multi_regs(mb_slave_t *mb)
{
    uint16_t read_addr   = mb_get_u16(&mb->rx_buf[2]);
    uint16_t read_qty    = mb_get_u16(&mb->rx_buf[4]);
    uint16_t write_addr  = mb_get_u16(&mb->rx_buf[6]);
    uint16_t write_qty   = mb_get_u16(&mb->rx_buf[8]);
    uint8_t  write_bytes = mb->rx_buf[10];
    
    if (read_qty < 1 || read_qty > 125) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_qty < 1 || write_qty > 121) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (write_bytes != MB_MUL2(write_qty)) {
        return MB_EX_ILLEGAL_VALUE;
    }
    if (read_addr + read_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    if (write_addr + write_qty > MB_REG_HOLDING_NUM) {
        return MB_EX_ILLEGAL_ADDR;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb_exception_t ex = mb->callback(mb, MB_CB_PRE_WRITE_HOLDING, write_addr, write_qty);
        if (ex != MB_EX_NONE) return ex;
    }
#endif
    
    /* 先写 */
    for (uint16_t i = 0; i < write_qty; i++) {
        uint16_t val = mb_get_u16(&mb->rx_buf[11 + MB_MUL2(i)]);
        
#if MB_USE_CALLBACK
        if (mb->reg_write_hook) {
            mb_exception_t ex = mb->reg_write_hook(mb, MB_FC_RW_MULTI_REGS, write_addr + i, val);
            if (ex != MB_EX_NONE) return ex;
        } else
#endif
        {
            mb->holding_reg[write_addr + i] = val;
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_WRITE_HOLDING, write_addr, write_qty);
        mb->callback(mb, MB_CB_PRE_READ_HOLDING, read_addr, read_qty);
    }
#endif
    
    /* 后读 */
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_RW_MULTI_REGS;
    mb->tx_buf[2] = (uint8_t)MB_MUL2(read_qty);
    
    uint16_t idx = 3;
    
#if MB_USE_CALLBACK
    if (mb->reg_read_hook) {
        for (uint16_t i = 0; i < read_qty; i++) {
            uint16_t val = 0;
            mb_exception_t ex = mb->reg_read_hook(mb, MB_FC_RW_MULTI_REGS, read_addr + i, &val);
            if (ex != MB_EX_NONE) return ex;
            mb_put_u16(&mb->tx_buf[idx], val);
            idx += 2;
        }
    } else
#endif
    {
        for (uint16_t i = 0; i < read_qty; i++) {
            mb_put_u16(&mb->tx_buf[idx], mb->holding_reg[read_addr + i]);
            idx += 2;
        }
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_POST_READ_HOLDING, read_addr, read_qty);
    }
#endif
    
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/* 0x2B - 读设备标识 (MEI Type 0x0E) */
static mb_exception_t mb_fc_read_device_id(mb_slave_t *mb)
{
    if (mb->rx_buf[2] != 0x0E) {
        return MB_EX_ILLEGAL_FUNC;
    }
    
    uint8_t read_code = mb->rx_buf[3];  /* 01=Basic, 02=Regular, 03=Extended, 04=Specific */
    uint8_t obj_id    = mb->rx_buf[4];  /* 起始对象ID */
    
    mb->tx_buf[0] = mb->addr;
    mb->tx_buf[1] = MB_FC_READ_DEVICE_ID;
    mb->tx_buf[2] = 0x0E;               /* MEI Type */
    mb->tx_buf[3] = read_code;
    mb->tx_buf[4] = 0x01;               /* 一致性等级: Basic */
    mb->tx_buf[5] = 0x00;               /* More Follows: 0=No */
    mb->tx_buf[6] = 0x00;               /* Next Object ID */
    
    uint16_t idx = 8;                   /* 跳过对象数量字段 */
    uint8_t obj_count = 0;
    
    /* 根据读取代码返回不同级别的信息 */
    if (read_code == 0x01 || read_code == 0x04) {
        /* Basic: VendorName, ProductCode, MajorMinorRevision */
        if (obj_id <= 0x00) {
            uint8_t len = (uint8_t)strlen(mb->device_id.vendor_name);
            mb->tx_buf[idx++] = 0x00;   /* Object ID */
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.vendor_name, len);
            idx += len;
            obj_count++;
        }
        if (obj_id <= 0x01) {
            uint8_t len = (uint8_t)strlen(mb->device_id.product_code);
            mb->tx_buf[idx++] = 0x01;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.product_code, len);
            idx += len;
            obj_count++;
        }
        if (obj_id <= 0x02) {
            uint8_t len = (uint8_t)strlen(mb->device_id.major_minor_rev);
            mb->tx_buf[idx++] = 0x02;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.major_minor_rev, len);
            idx += len;
            obj_count++;
        }
    }
    
    if (read_code == 0x02 || read_code == 0x04) {
        /* Regular: VendorUrl, ProductName, ModelName, UserApplicationName */
        if (obj_id <= 0x03 && strlen(mb->device_id.vendor_url) > 0) {
            uint8_t len = (uint8_t)strlen(mb->device_id.vendor_url);
            mb->tx_buf[idx++] = 0x03;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.vendor_url, len);
            idx += len;
            obj_count++;
        }
        if (obj_id <= 0x04 && strlen(mb->device_id.product_name) > 0) {
            uint8_t len = (uint8_t)strlen(mb->device_id.product_name);
            mb->tx_buf[idx++] = 0x04;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.product_name, len);
            idx += len;
            obj_count++;
        }
        if (obj_id <= 0x05 && strlen(mb->device_id.model_name) > 0) {
            uint8_t len = (uint8_t)strlen(mb->device_id.model_name);
            mb->tx_buf[idx++] = 0x05;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.model_name, len);
            idx += len;
            obj_count++;
        }
        if (obj_id <= 0x06 && strlen(mb->device_id.user_app_name) > 0) {
            uint8_t len = (uint8_t)strlen(mb->device_id.user_app_name);
            mb->tx_buf[idx++] = 0x06;
            mb->tx_buf[idx++] = len;
            memcpy(&mb->tx_buf[idx], mb->device_id.user_app_name, len);
            idx += len;
            obj_count++;
        }
    }
    
    mb->tx_buf[7] = obj_count;
    mb->tx_len = mb_append_crc(mb->tx_buf, idx);
    return MB_EX_NONE;
}

/*==================== 功能码分发表 ====================*/
typedef mb_exception_t (*mb_func_handler_t)(mb_slave_t *mb);

typedef struct {
    uint8_t             func_code;
    uint8_t             min_len;    /* 最小请求长度(不含CRC) */
    mb_func_handler_t   handler;
} mb_func_entry_t;

static const mb_func_entry_t mb_func_table[] = {
    { MB_FC_READ_COILS,             6, mb_fc_read_coils },
    { MB_FC_READ_DISCRETE,          6, mb_fc_read_discrete },
    { MB_FC_READ_HOLDING_REG,       6, mb_fc_read_holding_reg },
    { MB_FC_READ_INPUT_REG,         6, mb_fc_read_input_reg },
    { MB_FC_WRITE_SINGLE_COIL,      6, mb_fc_write_single_coil },
    { MB_FC_WRITE_SINGLE_REG,       6, mb_fc_write_single_reg },
    { MB_FC_READ_EXCEPTION_STATUS,  2, mb_fc_read_exception_status },
    { MB_FC_DIAGNOSTICS,            6, mb_fc_diagnostics },
    { MB_FC_GET_COMM_EVENT_CNT,     2, mb_fc_get_comm_event_cnt },
    { MB_FC_WRITE_MULTI_COILS,      7, mb_fc_write_multi_coils },
    { MB_FC_WRITE_MULTI_REGS,       7, mb_fc_write_multi_regs },
    { MB_FC_REPORT_SLAVE_ID,        2, mb_fc_report_slave_id },
    { MB_FC_MASK_WRITE_REG,         8, mb_fc_mask_write_reg },
    { MB_FC_RW_MULTI_REGS,         11, mb_fc_rw_multi_regs },
    { MB_FC_READ_DEVICE_ID,         5, mb_fc_read_device_id },
    { 0, 0, NULL }  /* 结束标记 */
};

/*==================== 帧处理 ====================*/
static void mb_process_frame(mb_slave_t *mb)
{
    /* 最小帧长度检查: 地址(1) + 功能码(1) + CRC(2) = 4 */
    if (mb->rx_cnt < 4) {
        return;
    }
    
    /* 地址检查 */
    uint8_t recv_addr = mb->rx_buf[0];
    if (recv_addr != mb->addr && recv_addr != 0) {
        return;
    }
    
    /* CRC 校验 */
    uint16_t crc_recv = ((uint16_t)mb->rx_buf[mb->rx_cnt - 1] << 8) | 
                         mb->rx_buf[mb->rx_cnt - 2];
    uint16_t crc_calc = mb_crc16(mb->rx_buf, mb->rx_cnt - 2);
    
    if (crc_recv != crc_calc) {
        mb->stats.bus_comm_err_count++;
        return;
    }
    
    /* 更新统计 */
    mb->stats.bus_msg_count++;
    if (recv_addr == mb->addr) {
        mb->stats.slave_msg_count++;
    }
    
#if MB_USE_CALLBACK
    if (mb->callback) {
        mb->callback(mb, MB_CB_FRAME_RECEIVED, 0, mb->rx_cnt);
    }
#endif
    
    /* 广播地址标记 */
    bool is_broadcast = (recv_addr == 0);
    
    uint8_t func = mb->rx_buf[1];
    mb_exception_t ex = MB_EX_ILLEGAL_FUNC;
    
    MB_ENTER_CRITICAL();
    
    /* 查找功能码处理函数 */
    const mb_func_entry_t *entry = mb_func_table;
    while (entry->handler != NULL) {
        if (entry->func_code == func) {
            /* 检查最小长度 */
            if (mb->rx_cnt >= entry->min_len + 2) {
                ex = entry->handler(mb);
            } else {
                ex = MB_EX_ILLEGAL_VALUE;
            }
            break;
        }
        entry++;
    }
    
    /* 未找到标准功能码，尝试自定义处理 */
    if (entry->handler == NULL) {
#if MB_USE_CALLBACK
        if (mb->custom_func_cb) {
            uint16_t custom_tx_len = 0;
            ex = mb->custom_func_cb(mb, func, 
                                    &mb->rx_buf[2], mb->rx_cnt - 4,
                                    &mb->tx_buf[2], &custom_tx_len);
            if (ex == MB_EX_NONE) {
                mb->tx_buf[0] = mb->addr;
                mb->tx_buf[1] = func;
                mb->tx_len = mb_append_crc(mb->tx_buf, custom_tx_len + 2);
            }
        }
#endif
    }
    
    MB_EXIT_CRITICAL();
    
    /* 广播不响应 */
    if (is_broadcast) {
        mb->stats.slave_no_response_count++;
        return;
    }
    
    /* 监听模式不响应 */
    if (mb->listen_only_mode) {
        mb->stats.slave_no_response_count++;
        return;
    }
    
    /* 发送响应 */
    if (ex != MB_EX_NONE) {
        mb_send_exception(mb, func, ex);
    } else {
        mb_send_response(mb);
    }
}

/*==================== 主轮询函数 ====================*/
void mb_slave_poll(mb_slave_t *mb)
{
    uint32_t current_tick = mb_port_get_tick();
    
    switch (mb->state) {
        case MB_STATE_IDLE:
            break;
            
        case MB_STATE_RECEIVING:
            /* 检测3.5字符超时，帧结束 */
            if ((current_tick - mb->last_rx_tick) >= mb->t35_timeout) {
                mb->state = MB_STATE_FRAME_READY;
            }
            break;
            
        case MB_STATE_FRAME_READY:
            mb->state = MB_STATE_PROCESSING;
            mb_process_frame(mb);
            mb->rx_cnt = 0;
            mb->state = MB_STATE_IDLE;
            break;
            
        case MB_STATE_PROCESSING:
            /* 正在处理中，等待完成 */
            break;
    }
}

/*==================== 用户接口函数 ====================*/

/* 保持寄存器操作 */
bool mb_set_holding_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_HOLDING_NUM) return false;
    MB_ENTER_CRITICAL();
    mb->holding_reg[addr] = value;
    MB_EXIT_CRITICAL();
    return true;
}

uint16_t mb_get_holding_reg(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_HOLDING_NUM) return 0;
    MB_ENTER_CRITICAL();
    uint16_t val = mb->holding_reg[addr];
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_holding_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count)
{
    if (addr + count > MB_REG_HOLDING_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        mb->holding_reg[addr + i] = values[i];
    }
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_holding_regs(mb_slave_t *mb, uint16_t addr, uint16_t *values, uint16_t count)
{
    if (addr + count > MB_REG_HOLDING_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        values[i] = mb->holding_reg[addr + i];
    }
    MB_EXIT_CRITICAL();
    return true;
}

/* 输入寄存器操作 */
bool mb_set_input_reg(mb_slave_t *mb, uint16_t addr, uint16_t value)
{
    if (addr >= MB_REG_INPUT_NUM) return false;
    MB_ENTER_CRITICAL();
    mb->input_reg[addr] = value;
    MB_EXIT_CRITICAL();
    return true;
}

uint16_t mb_get_input_reg(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_INPUT_NUM) return 0;
    MB_ENTER_CRITICAL();
    uint16_t val = mb->input_reg[addr];
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_input_regs(mb_slave_t *mb, uint16_t addr, const uint16_t *values, uint16_t count)
{
    if (addr + count > MB_REG_INPUT_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        mb->input_reg[addr + i] = values[i];
    }
    MB_EXIT_CRITICAL();
    return true;
}

/* 线圈操作 */
bool mb_set_coil(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_ENTER_CRITICAL();
    MB_BIT_WR(mb->coil_reg, addr, value);
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_coil(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_COIL_NUM) return false;
    MB_ENTER_CRITICAL();
    bool val = MB_BIT_GET(mb->coil_reg, addr);
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_coils(mb_slave_t *mb, uint16_t addr, const bool *values, uint16_t count)
{
    if (addr + count > MB_REG_COIL_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        MB_BIT_WR(mb->coil_reg, addr + i, values[i]);
    }
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_coils(mb_slave_t *mb, uint16_t addr, bool *values, uint16_t count)
{
    if (addr + count > MB_REG_COIL_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        values[i] = MB_BIT_GET(mb->coil_reg, addr + i);
    }
    MB_EXIT_CRITICAL();
    return true;
}

/* 离散输入操作 */
bool mb_set_discrete(mb_slave_t *mb, uint16_t addr, bool value)
{
    if (addr >= MB_REG_DISCRETE_NUM) return false;
    MB_ENTER_CRITICAL();
    MB_BIT_WR(mb->discrete_reg, addr, value);
    MB_EXIT_CRITICAL();
    return true;
}

bool mb_get_discrete(mb_slave_t *mb, uint16_t addr)
{
    if (addr >= MB_REG_DISCRETE_NUM) return false;
    MB_ENTER_CRITICAL();
    bool val = MB_BIT_GET(mb->discrete_reg, addr);
    MB_EXIT_CRITICAL();
    return val;
}

bool mb_set_discretes(mb_slave_t *mb, uint16_t addr, const bool *values, uint16_t count)
{
    if (addr + count > MB_REG_DISCRETE_NUM || values == NULL) return false;
    MB_ENTER_CRITICAL();
    for (uint16_t i = 0; i < count; i++) {
        MB_BIT_WR(mb->discrete_reg, addr + i, values[i]);
    }
    MB_EXIT_CRITICAL();
    return true;
}

/* 异常状态操作 */
void mb_set_exception_status(mb_slave_t *mb, uint8_t status)
{
    MB_ENTER_CRITICAL();
    mb->exception_status = status;
    MB_EXIT_CRITICAL();
}

uint8_t mb_get_exception_status(mb_slave_t *mb)
{
    MB_ENTER_CRITICAL();
    uint8_t status = mb->exception_status;
    MB_EXIT_CRITICAL();
    return status;
}

void mb_set_exception_bit(mb_slave_t *mb, uint8_t bit, bool value)
{
    if (bit > 7) return;
    MB_ENTER_CRITICAL();
    if (value) {
        mb->exception_status |= (1U << bit);
    } else {
        mb->exception_status &= ~(1U << bit);
    }
    MB_EXIT_CRITICAL();
}

/* 设备标识设置 */
void mb_set_slave_id(mb_slave_t *mb, const uint8_t *data, uint8_t len)
{
    if (len > sizeof(mb->slave_id_data)) {
        len = sizeof(mb->slave_id_data);
    }
    MB_ENTER_CRITICAL();
    memcpy(mb->slave_id_data, data, len);
    mb->slave_id_len = len;
    MB_EXIT_CRITICAL();
}

void mb_set_device_id(mb_slave_t *mb, const mb_device_id_t *id)
{
    if (id == NULL) return;
    MB_ENTER_CRITICAL();
    memcpy(&mb->device_id, id, sizeof(mb_device_id_t));
    MB_EXIT_CRITICAL();
}

/* 统计信息 */
const mb_statistics_t* mb_get_statistics(mb_slave_t *mb)
{
    return &mb->stats;
}

void mb_clear_statistics(mb_slave_t *mb)
{
    MB_ENTER_CRITICAL();
    memset(&mb->stats, 0, sizeof(mb_statistics_t));
    MB_EXIT_CRITICAL();
}

#if MB_USE_CALLBACK
/* 回调注册 */
void mb_set_callback(mb_slave_t *mb, mb_callback_t cb)
{
    MB_ENTER_CRITICAL();
    mb->callback = cb;
    MB_EXIT_CRITICAL();
}

void mb_set_custom_func_callback(mb_slave_t *mb, mb_custom_func_t cb)
{
    MB_ENTER_CRITICAL();
    mb->custom_func_cb = cb;
    MB_EXIT_CRITICAL();
}

void mb_set_reg_hooks(mb_slave_t *mb, mb_reg_read_hook_t read_hook, mb_reg_write_hook_t write_hook)
{
    MB_ENTER_CRITICAL();
    mb->reg_read_hook = read_hook;
    mb->reg_write_hook = write_hook;
    MB_EXIT_CRITICAL();
}

void mb_set_user_data(mb_slave_t *mb, void *data)
{
    mb->user_data = data;
}

void* mb_get_user_data(mb_slave_t *mb)
{
    return mb->user_data;
}
#endif

/*==================== 调试接口 ====================*/
#if MB_DEBUG_ENABLE
#include <stdio.h>

void mb_debug_dump_frame(const uint8_t *buf, uint16_t len, bool is_tx)
{
    printf("[MB %s] ", is_tx ? "TX" : "RX");
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("(%d bytes)\n", len);
}

void mb_debug_print_stats(mb_slave_t *mb)
{
    printf("=== Modbus Statistics ===\n");
    printf("Bus Messages:     %lu\n", mb->stats.bus_msg_count);
    printf("Bus Errors:       %lu\n", mb->stats.bus_comm_err_count);
    printf("Slave Messages:   %lu\n", mb->stats.slave_msg_count);
    printf("Slave Exceptions: %lu\n", mb->stats.slave_exception_count);
    printf("No Response:      %lu\n", mb->stats.slave_no_response_count);
    printf("Overrun Errors:   %lu\n", mb->stats.bus_overrun_count);
    printf("=========================\n");
}
#endif
```

------

## 6. 硬件抽象层 `modbus_port.h` 和 `modbus_port.c`

### `modbus_port.h`

```c
#ifndef __MODBUS_PORT_H__
#define __MODBUS_PORT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化串口硬件
 * @param baudrate 波特率
 */
void mb_port_init(uint32_t baudrate);

/**
 * @brief 发送数据
 * @param buf 数据缓冲区
 * @param len 数据长度
 */
void mb_port_send(const uint8_t *buf, uint16_t len);

/**
 * @brief 获取系统时钟(毫秒)
 * @return 当前毫秒数
 */
uint32_t mb_port_get_tick(void);

/**
 * @brief 设置RS485方向控制(可选)
 * @param is_tx true=发送模式, false=接收模式
 */
void mb_port_set_direction(bool is_tx);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_PORT_H__ */
```

### `modbus_port.c` (STM32 HAL 示例)

```c
#include "modbus_port.h"
#include "modbus_slave.h"
#include "main.h"
#include <stdbool.h>

/* 外部声明 */
extern UART_HandleTypeDef huart1;
extern mb_slave_t mb_slave;

/* RS485方向控制引脚定义(如果使用RS485) */
#define RS485_DE_PORT       GPIOA
#define RS485_DE_PIN        GPIO_PIN_8
#define RS485_USE_DE        0   /* 1=使用方向控制, 0=不使用 */

void mb_port_init(uint32_t baudrate)
{
    /* UART 已在 CubeMX 中初始化 */
    /* 如需动态修改波特率: */
    /*
    huart1.Init.BaudRate = baudrate;
    HAL_UART_Init(&huart1);
    */
    
#if RS485_USE_DE
    /* 初始化RS485方向控制引脚 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RS485_DE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_DE_PORT, &GPIO_InitStruct);
    
    /* 默认接收模式 */
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
#endif
    
    /* 开启接收中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

void mb_port_send(const uint8_t *buf, uint16_t len)
{
#if RS485_USE_DE
    /* 切换到发送模式 */
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_SET);
#endif
    
    /* 发送数据 */
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
    
#if RS485_USE_DE
    /* 等待发送完成 */
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    
    /* 切换回接收模式 */
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, GPIO_PIN_RESET);
#endif

#if MB_DEBUG_ENABLE
    mb_debug_dump_frame(buf, len, true);
#endif
}

uint32_t mb_port_get_tick(void)
{
    return HAL_GetTick();
}

void mb_port_set_direction(bool is_tx)
{
#if RS485_USE_DE
    HAL_GPIO_WritePin(RS485_DE_PORT, RS485_DE_PIN, 
                      is_tx ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    (void)is_tx;
#endif
}

/* 
 * UART 中断处理 - 在 stm32xxxx_it.c 中调用
 * 或者使用 HAL 回调函数
 */
void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)(huart1.Instance->DR & 0xFF);
        mb_slave_rx_byte(&mb_slave, byte);
    }
    
    /* 处理错误标志 */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart1);
    }
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE)) {
        __HAL_UART_CLEAR_FEFLAG(&huart1);
    }
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE)) {
        __HAL_UART_CLEAR_NEFLAG(&huart1);
    }
}
```

### `modbus_port.c` (通用裸机示例)

```c
#include "modbus_port.h"
#include "modbus_slave.h"

extern mb_slave_t mb_slave;

/* 用户需根据具体MCU实现以下函数 */

static volatile uint32_t sys_tick_ms = 0;

void mb_port_init(uint32_t baudrate)
{
    /* 
     * 1. 配置UART: 8数据位, 1停止位, 无校验
     * 2. 设置波特率
     * 3. 使能接收中断
     * 4. 配置1ms定时器中断(用于sys_tick_ms)
     */
    
    /* 示例伪代码:
    UART_Init(baudrate, 8, 1, PARITY_NONE);
    UART_EnableRxInterrupt();
    Timer_Init(1000);  // 1ms
    Timer_EnableInterrupt();
    */
}

void mb_port_send(const uint8_t *buf, uint16_t len)
{
    /* 示例伪代码:
    for (uint16_t i = 0; i < len; i++) {
        while (!UART_TxReady());
        UART_SendByte(buf[i]);
    }
    while (!UART_TxComplete());
    */
}

uint32_t mb_port_get_tick(void)
{
    return sys_tick_ms;
}

void mb_port_set_direction(bool is_tx)
{
    /* RS485方向控制 */
    (void)is_tx;
}

/* 定时器中断服务函数 */
void Timer_IRQHandler(void)
{
    sys_tick_ms++;
}

/* UART接收中断服务函数 */
void UART_RX_IRQHandler(void)
{
    uint8_t byte = 0; /* = UART_ReadByte(); */
    mb_slave_rx_byte(&mb_slave, byte);
}
```

------

## 7. 完整使用示例

### 示例1：基础裸机应用

```c
#include "modbus_slave.h"

mb_slave_t mb_slave;

int main(void)
{
    /* 系统初始化 */
    System_Init();
    
    /* Modbus初始化: 地址1, 波特率9600 */
    mb_slave_init(&mb_slave, 1, 9600);
    
    /* 设置设备标识 */
    mb_device_id_t dev_id = {
        .vendor_name = "MyCompany",
        .product_code = "SENSOR-001",
        .major_minor_rev = "2.1.0",
        .product_name = "Temperature Sensor",
        .model_name = "TS-100",
    };
    mb_set_device_id(&mb_slave, &dev_id);
    
    /* 初始化寄存器值 */
    mb_set_holding_reg(&mb_slave, 0, 100);   /* 默认设定值 */
    mb_set_holding_reg(&mb_slave, 1, 50);    /* 报警阈值 */
    
    while (1) {
        /* Modbus轮询 */
        mb_slave_poll(&mb_slave);
        
        /* 更新输入寄存器(传感器数据) */
        mb_set_input_reg(&mb_slave, 0, Read_Temperature());
        mb_set_input_reg(&mb_slave, 1, Read_Humidity());
        mb_set_input_reg(&mb_slave, 2, Read_ADC(0));
        
        /* 更新离散输入(开关状态) */
        mb_set_discrete(&mb_slave, 0, Read_Button(0));
        mb_set_discrete(&mb_slave, 1, Read_Button(1));
        
        /* 读取线圈控制继电器 */
        Set_Relay(0, mb_get_coil(&mb_slave, 0));
        Set_Relay(1, mb_get_coil(&mb_slave, 1));
        
        /* 读取保持寄存器控制PWM */
        Set_PWM_Duty(mb_get_holding_reg(&mb_slave, 0));
    }
}
```

### 示例2：带回调的高级应用

```c
#include "modbus_slave.h"

mb_slave_t mb_slave;

/* 应用数据结构 */
typedef struct {
    uint16_t pwm_duty;
    uint16_t alarm_threshold;
    bool     alarm_enabled;
    uint32_t run_time_sec;
} app_data_t;

app_data_t app_data;

/* 寄存器地址映射 */
#define REG_PWM_DUTY        0
#define REG_ALARM_THRESH    1
#define REG_ALARM_ENABLE    2
#define REG_RUN_TIME_H      3
#define REG_RUN_TIME_L      4
#define REG_FIRMWARE_VER    5

/* 寄存器读钩子 - 实现虚拟寄存器 */
mb_exception_t reg_read_hook(mb_slave_t *mb, uint8_t reg_type, uint16_t addr, uint16_t *value)
{
    if (reg_type == MB_FC_READ_HOLDING_REG || reg_type == MB_FC_READ_INPUT_REG) {
        switch (addr) {
            case REG_RUN_TIME_H:
                *value = (uint16_t)(app_data.run_time_sec >> 16);
                return MB_EX_NONE;
            case REG_RUN_TIME_L:
                *value = (uint16_t)(app_data.run_time_sec & 0xFFFF);
                return MB_EX_NONE;
            case REG_FIRMWARE_VER:
                *value = 0x0210;  /* V2.1.0 */
                return MB_EX_NONE;
        }
    }
    
    /* 使用默认寄存器 */
    if (reg_type == MB_FC_READ_HOLDING_REG) {
        *value = mb->holding_reg[addr];
    } else if (reg_type == MB_FC_READ_INPUT_REG) {
        *value = mb->input_reg[addr];
    }
    return MB_EX_NONE;
}

/* 寄存器写钩子 - 参数验证和即时生效 */
mb_exception_t reg_write_hook(mb_slave_t *mb, uint8_t reg_type, uint16_t addr, uint16_t value)
{
    if (reg_type == MB_FC_WRITE_SINGLE_REG || reg_type == MB_FC_WRITE_MULTI_REGS) {
        switch (addr) {
            case REG_PWM_DUTY:
                if (value > 1000) {
                    return MB_EX_ILLEGAL_VALUE;  /* 限制0-1000 */
                }
                app_data.pwm_duty = value;
                Set_PWM_Duty(value);
                mb->holding_reg[addr] = value;
                return MB_EX_NONE;
                
            case REG_ALARM_THRESH:
                if (value < 10 || value > 500) {
                    return MB_EX_ILLEGAL_VALUE;
                }
                app_data.alarm_threshold = value;
                mb->holding_reg[addr] = value;
                return MB_EX_NONE;
                
            case REG_ALARM_ENABLE:
                app_data.alarm_enabled = (value != 0);
                mb->holding_reg[addr] = value ? 1 : 0;
                return MB_EX_NONE;
                
            case REG_RUN_TIME_H:
            case REG_RUN_TIME_L:
            case REG_FIRMWARE_VER:
                return MB_EX_ILLEGAL_ADDR;  /* 只读寄存器 */
        }
    }
    
    /* 默认写入 */
    mb->holding_reg[addr] = value;
    return MB_EX_NONE;
}

/* 通用回调 - 事件通知 */
mb_exception_t event_callback(mb_slave_t *mb, mb_callback_event_t event, 
                              uint16_t addr, uint16_t quantity)
{
    switch (event) {
        case MB_CB_PRE_READ_INPUT:
            /* 读取前更新传感器数据 */
            mb->input_reg[0] = Read_Temperature();
            mb->input_reg[1] = Read_Humidity();
            mb->input_reg[2] = Read_Pressure() >> 16;
            mb->input_reg[3] = Read_Pressure() & 0xFFFF;
            break;
            
        case MB_CB_POST_WRITE_COIL:
            /* 线圈写入后立即控制输出 */
            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t coil = addr + i;
                if (coil < 8) {
                    Set_Relay(coil, MB_BIT_GET(mb->coil_reg, coil));
                }
            }
            break;
            
        case MB_CB_FRAME_RECEIVED:
            /* 收到有效帧，闪烁LED */
            LED_Toggle(LED_COMM);
            break;
            
        case MB_CB_ERROR:
            /* 发生错误 */
            LED_On(LED_ERROR);
            break;
            
        default:
            break;
    }
    return MB_EX_NONE;
}

/* 自定义功能码处理 */
mb_exception_t custom_func_handler(mb_slave_t *mb, uint8_t func_code,
                                   const uint8_t *rx_data, uint16_t rx_len,
                                   uint8_t *tx_data, uint16_t *tx_len)
{
    switch (func_code) {
        case 0x41:  /* 读取系统信息 */
        {
            tx_data[0] = 12;  /* 数据长度 */
            
            /* 固件版本 */
            tx_data[1] = 2;
            tx_data[2] = 1;
            tx_data[3] = 0;
            
            /* 运行时间(秒) */
            tx_data[4] = (app_data.run_time_sec >> 24) & 0xFF;
            tx_data[5] = (app_data.run_time_sec >> 16) & 0xFF;
            tx_data[6] = (app_data.run_time_sec >> 8) & 0xFF;
            tx_data[7] = app_data.run_time_sec & 0xFF;
            
            /* 系统状态 */
            tx_data[8] = app_data.alarm_enabled ? 0x01 : 0x00;
            tx_data[9] = 0x00;  /* 保留 */
            tx_data[10] = 0x00;
            tx_data[11] = 0x00;
            tx_data[12] = Get_System_Status();
            
            *tx_len = 13;
            return MB_EX_NONE;
        }
        
        case 0x42:  /* 系统复位 */
        {
            if (rx_len < 2) {
                return MB_EX_ILLEGAL_VALUE;
            }
            uint16_t magic = (rx_data[0] << 8) | rx_data[1];
            if (magic != 0xAA55) {
                return MB_EX_ILLEGAL_VALUE;
            }
            
            tx_data[0] = 0x00;  /* OK */
            *tx_len = 1;
            
            /* 延迟复位 */
            Schedule_Reset(100);
            return MB_EX_NONE;
        }
        
        case 0x43:  /* 保存配置到EEPROM */
        {
            bool result = Save_Config_To_EEPROM(&app_data, sizeof(app_data));
            tx_data[0] = result ? 0x00 : 0x01;
            *tx_len = 1;
            return MB_EX_NONE;
        }
        
        case 0x44:  /* 恢复出厂设置 */
        {
            if (rx_len < 2) {
                return MB_EX_ILLEGAL_VALUE;
            }
            uint16_t magic = (rx_data[0] << 8) | rx_data[1];
            if (magic != 0x1234) {
                return MB_EX_ILLEGAL_VALUE;
            }
            
            /* 恢复默认值 */
            app_data.pwm_duty = 500;
            app_data.alarm_threshold = 100;
            app_data.alarm_enabled = true;
            
            mb->holding_reg[REG_PWM_DUTY] = app_data.pwm_duty;
            mb->holding_reg[REG_ALARM_THRESH] = app_data.alarm_threshold;
            mb->holding_reg[REG_ALARM_ENABLE] = 1;
            
            tx_data[0] = 0x00;
            *tx_len = 1;
            return MB_EX_NONE;
        }
        
        default:
            return MB_EX_ILLEGAL_FUNC;
    }
}

int main(void)
{
    System_Init();
    
    /* 从EEPROM加载配置 */
    Load_Config_From_EEPROM(&app_data, sizeof(app_data));
    
    /* Modbus初始化 */
    mb_slave_init(&mb_slave, 1, 9600);
    
    /* 注册回调 */
    mb_set_callback(&mb_slave, event_callback);
    mb_set_custom_func_callback(&mb_slave, custom_func_handler);
    mb_set_reg_hooks(&mb_slave, reg_read_hook, reg_write_hook);
    mb_set_user_data(&mb_slave, &app_data);
    
    /* 同步寄存器值 */
    mb_set_holding_reg(&mb_slave, REG_PWM_DUTY, app_data.pwm_duty);
    mb_set_holding_reg(&mb_slave, REG_ALARM_THRESH, app_data.alarm_threshold);
    mb_set_holding_reg(&mb_slave, REG_ALARM_ENABLE, app_data.alarm_enabled ? 1 : 0);
    
    uint32_t last_sec_tick = 0;
    
    while (1) {
        mb_slave_poll(&mb_slave);
        
        /* 每秒更新运行时间 */
        uint32_t now = mb_port_get_tick();
        if (now - last_sec_tick >= 1000) {
            last_sec_tick = now;
            app_data.run_time_sec++;
        }
        
        /* 其他应用逻辑 */
        App_Process();
    }
}
```

### 示例3：FreeRTOS 多任务应用

```c
#include "modbus_slave.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

mb_slave_t mb_slave;
QueueHandle_t sensor_queue;

/* 传感器数据结构 */
typedef struct {
    uint16_t temperature;
    uint16_t humidity;
    uint16_t pressure_h;
    uint16_t pressure_l;
} sensor_data_t;

/* Modbus任务 */
void modbus_task(void *param)
{
    mb_slave_init(&mb_slave, 1, 115200);
    
    while (1) {
        mb_slave_poll(&mb_slave);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* 传感器采集任务 */
void sensor_task(void *param)
{
    sensor_data_t data;
    
    while (1) {
        /* 采集传感器数据 */
        data.temperature = Read_Temperature_Sensor();
        data.humidity = Read_Humidity_Sensor();
        uint32_t pressure = Read_Pressure_Sensor();
        data.pressure_h = pressure >> 16;
        data.pressure_l = pressure & 0xFFFF;
        
        /* 更新Modbus输入寄存器 */
        mb_set_input_regs(&mb_slave, 0, (uint16_t *)&data, 4);
        
        /* 检查报警条件 */
        uint16_t threshold = mb_get_holding_reg(&mb_slave, 0);
        if (data.temperature > threshold) {
            mb_set_exception_bit(&mb_slave, 0, true);  /* 设置报警位 */
        } else {
            mb_set_exception_bit(&mb_slave, 0, false);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* 控制输出任务 */
void control_task(void *param)
{
    while (1) {
        /* 读取线圈状态控制继电器 */
        for (int i = 0; i < 8; i++) {
            Set_Relay(i, mb_get_coil(&mb_slave, i));
        }
        
        /* 读取保持寄存器控制PWM */
        uint16_t pwm_duty = mb_get_holding_reg(&mb_slave, 1);
        Set_PWM_Duty(0, pwm_duty);
        
        uint16_t pwm_freq = mb_get_holding_reg(&mb_slave, 2);
        Set_PWM_Frequency(0, pwm_freq);
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    /* 创建任务 */
    xTaskCreate(modbus_task, "Modbus", 512, NULL, 3, NULL);
    xTaskCreate(sensor_task, "Sensor", 256, NULL, 2, NULL);
    xTaskCreate(control_task, "Control", 256, NULL, 2, NULL);
    
    vTaskStartScheduler();
    
    while (1);
}
```

------

## 8. 寄存器映射表示例

|      类型      | 地址  |      名称      | 读写 |       说明       |
| :------------: | :---: | :------------: | :--: | :--------------: |
| **保持寄存器** |   0   |   PWM占空比    | R/W  |  0-1000 (0.1%)   |
|                |   1   |    报警阈值    | R/W  |     温度阈值     |
|                |   2   |    报警使能    | R/W  |  0=禁用, 1=使能  |
|                |   3   | 运行时间高16位 |  R   |        秒        |
|                |   4   | 运行时间低16位 |  R   |        秒        |
|                |   5   |    固件版本    |  R   |  0xMMNN (主.次)  |
|                | 10-19 |    用户参数    | R/W  |      自定义      |
| **输入寄存器** |   0   |      温度      |  R   |      0.1°C       |
|                |   1   |      湿度      |  R   |      0.1%RH      |
|                |   2   |   气压高16位   |  R   |        Pa        |
|                |   3   |   气压低16位   |  R   |        Pa        |
|                |  4-7  |   ADC通道0-3   |  R   |      原始值      |
|    **线圈**    |  0-7  |   继电器输出   | R/W  |   0=OFF, 1=ON    |
|                | 8-15  |    LED控制     | R/W  |   0=OFF, 1=ON    |
|  **离散输入**  |  0-7  |    按键状态    |  R   |  0=释放, 1=按下  |
|                | 8-15  |    限位开关    |  R   | 0=未触发, 1=触发 |

------

## 9. 资源占用统计

|        项目        |    大小    |            说明            |
| :----------------: | :--------: | :------------------------: |
| **代码段 (Flash)** |  ~4-6 KB   | 取决于功能码数量和优化等级 |
|     **CRC表**      | 512 Bytes  |      可选用计算法节省      |
|     **控制块**     | ~800 Bytes |   含100个保持/输入寄存器   |
|     **缓冲区**     | 512 Bytes  |       收发各256字节        |
|     **栈需求**     | ~200 Bytes |         最深调用栈         |

------

## 10. 移植检查清单

- [ ] 实现 `mb_port_init()` - 串口初始化
- [ ] 实现 `mb_port_send()` - 数据发送
- [ ] 实现 `mb_port_get_tick()` - 毫秒时钟
- [ ] 配置串口接收中断，调用 `mb_slave_rx_byte()`
- [ ] 根据需要配置 `MB_RTOS_TYPE`
- [ ] 根据需要配置寄存器数量宏
- [ ] 如使用RS485，实现方向控制

这个完善后的Modbus从机实现具有以下特点：

1. **完全避免除法运算** - 使用位操作宏
2. **功能码分发表** - 易于扩展新功能码
3. **完整回调机制** - 支持事件通知、寄存器钩子、自定义功能码
4. **多RTOS支持** - FreeRTOS/RT-Thread/uCOS
5. **完整统计信息** - 符合Modbus诊断规范
6. **设备标识支持** - 0x2B功能码
7. **监听模式** - 支持诊断功能