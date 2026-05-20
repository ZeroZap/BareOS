/**
 * @file mb_tiny.h
 * @brief Nano Modbus Tiny - RTU Only, Minimal Footprint
 * @version 1.0.0
 * @date 2026-03-31
 */

#ifndef MB_TINY_H
#define MB_TINY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_typedef.h"

/* ==================== 配置 ==================== */

#define MB_TINY_MAX_ADU_SIZE     128     /**< 最大 ADU 大小 */
#define MB_TINY_TIMEOUT_MS       1000    /**< 默认超时 (ms) */

/* ==================== 错误码 ==================== */

#define MB_TINY_OK               0
#define MB_TINY_ERROR            (-1)
#define MB_TINY_INVALID_PARAM   (-2)
#define MB_TINY_TIMEOUT         (-3)
#define MB_TINY_CRC_ERROR       (-4)

/* ==================== 功能码 ==================== */

#define MB_FUNC_READ_COILS          0x01
#define MB_FUNC_READ_DISCRETE       0x02
#define MB_FUNC_READ_HOLDING        0x03
#define MB_FUNC_READ_INPUT          0x04
#define MB_FUNC_WRITE_SINGLE_COIL   0x05
#define MB_FUNC_WRITE_SINGLE_REG    0x06
#define MB_FUNC_WRITE_MULTIPLE_COILS 0x0F
#define MB_FUNC_WRITE_MULTIPLE_REGS  0x10

/* ==================== 错误码 (从站) ==================== */

#define MB_ERR_ILLEGAL_FUNC       0x01
#define MB_ERR_ILLEGAL_DATA_ADDR  0x02
#define MB_ERR_ILLEGAL_DATA_VALUE 0x03

/* ==================== 从站设备 ==================== */

/**
 * @brief 保持寄存器 (用户定义)
 */
typedef struct {
    uint16_t *data;         /**< 寄存器数据 */
    uint16_t start_addr;    /**< 起始地址 */
    uint16_t count;         /**< 寄存器数量 */
} mb_tiny_holding_t;

/**
 * @brief 线圈 (用户定义)
 */
typedef struct {
    uint8_t *data;          /**< 线圈数据 */
    uint16_t start_addr;    /**< 起始地址 */
    uint16_t count;         /**< 线圈数量 */
} mb_tiny_coils_t;

/**
 * @brief Modbus Tiny 从站
 */
typedef struct {
    uint8_t slave_id;                   /**< 从站 ID (1-247) */
    mb_tiny_holding_t holding;         /**< 保持寄存器 */
    mb_tiny_coils_t coils;              /**< 线圈 */
    
    uint8_t rx_buf[MB_TINY_MAX_ADU_SIZE];
    uint16_t rx_len;
    
    uint32_t request_count;
    uint32_t error_count;
    bool initialized;
} mb_tiny_slave_t;

/**
 * @brief Modbus Tiny 主站
 */
typedef struct {
    uint8_t slave_id;                   /**< 当前从站 ID */
    uint32_t timeout_ms;                 /**< 超时时间 */
    
    uint8_t tx_buf[MB_TINY_MAX_ADU_SIZE];
    uint8_t rx_buf[MB_TINY_MAX_ADU_SIZE];
    uint16_t tx_len;
    uint16_t rx_len;
    
    uint32_t request_count;
    uint32_t error_count;
    bool initialized;
} mb_tiny_master_t;

/* ==================== 串口接口 ==================== */

/**
 * @brief 串口发送回调 (用户实现)
 * @param data 数据指针
 * @param len 数据长度
 * @return 实际发送字节数或错误码
 */
typedef int (*mb_tiny_send_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief 串口接收回调 (用户实现)
 * @param data 数据缓冲区
 * @param len 最大接收长度
 * @param timeout_ms 超时时间
 * @return 实际接收字节数或错误码
 */
typedef int (*mb_tiny_recv_cb_t)(uint8_t *data, uint16_t len, uint32_t timeout_ms);

/* ==================== 从站 API ==================== */

/**
 * @brief 初始化 Modbus Tiny 从站
 * @param slave 从站句柄
 * @param slave_id 从站 ID
 * @return MB_TINY_OK 成功
 */
int mb_tiny_slave_init(mb_tiny_slave_t *slave, uint8_t slave_id);

/**
 * @brief 配置保持寄存器
 * @param slave 从站句柄
 * @param data 寄存器数据缓冲区
 * @param start_addr 起始地址
 * @param count 寄存器数量
 * @return MB_TINY_OK 成功
 */
int mb_tiny_slave_config_holding(mb_tiny_slave_t *slave, uint16_t *data,
                                  uint16_t start_addr, uint16_t count);

/**
 * @brief 配置线圈
 * @param slave 从站句柄
 * @param data 线圈数据缓冲区
 * @param start_addr 起始地址
 * @param count 线圈数量
 * @return MB_TINY_OK 成功
 */
int mb_tiny_slave_config_coils(mb_tiny_slave_t *slave, uint8_t *data,
                                uint16_t start_addr, uint16_t count);

/**
 * @brief 设置发送回调
 * @param slave 从站句柄
 * @param send_cb 发送回调
 * @return MB_TINY_OK 成功
 */
void mb_tiny_slave_set_send(mb_tiny_slave_t *slave, mb_tiny_send_cb_t send_cb);

/**
 * @brief 处理接收数据 (在串口中断或轮询中调用)
 * @param slave 从站句柄
 * @param data 接收数据
 * @param len 数据长度
 * @return MB_TINY_OK 成功
 */
int mb_tiny_slave_handle(mb_tiny_slave_t *slave, const uint8_t *data, uint16_t len);

/* ==================== 主站 API ==================== */

/**
 * @brief 初始化 Modbus Tiny 主站
 * @param master 主站句柄
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_init(mb_tiny_master_t *master);

/**
 * @brief 设置串口接口
 * @param master 主站句柄
 * @param send_cb 发送回调
 * @param recv_cb 接收回调
 */
void mb_tiny_master_set_uart(mb_tiny_master_t *master, 
                              mb_tiny_send_cb_t send_cb, 
                              mb_tiny_recv_cb_t recv_cb);

/**
 * @brief 设置超时时间
 * @param master 主站句柄
 * @param timeout_ms 超时时间 (ms)
 */
void mb_tiny_master_set_timeout(mb_tiny_master_t *master, uint32_t timeout_ms);

/**
 * @brief 读保持寄存器 (0x03)
 * @param master 主站句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 寄存器数量
 * @param data 数据缓冲区
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_read_holding(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data);

/**
 * @brief 写单个寄存器 (0x06)
 * @param master 主站句柄
 * @param slave_id 从站 ID
 * @param addr 寄存器地址
 * @param value 寄存器值
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_write_reg(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value);

/**
 * @brief 写多个寄存器 (0x10)
 * @param master 主站句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 寄存器数量
 * @param data 数据
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_write_regs(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, const uint16_t *data);

/**
 * @brief 读线圈 (0x01)
 * @param master 主站句柄
 * @param slave_id 从站 ID
 * @param addr 起始地址
 * @param count 线圈数量
 * @param data 数据缓冲区
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_read_coils(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, uint8_t *data);

/**
 * @brief 写单个线圈 (0x05)
 * @param master 主站句柄
 * @param slave_id 从站 ID
 * @param addr 线圈地址
 * @param value 线圈值 (0x0000 或 0xFF00)
 * @return MB_TINY_OK 成功
 */
int mb_tiny_master_write_coil(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value);

/* ==================== 工具函数 ==================== */

/**
 * @brief 计算 CRC16 (Modbus)
 * @param data 数据
 * @param len 长度
 * @return CRC16 值
 */
uint16_t mb_tiny_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* MB_TINY_H */
