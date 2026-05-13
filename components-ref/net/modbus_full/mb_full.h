/**
 * @file mb_full.h
 * @brief Nano Modbus Full - RTU + TCP + ASCII, Full Feature Set
 * @version 1.0.0
 * @date 2026-03-31
 */

#ifndef MB_FULL_H
#define MB_FULL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==================== 版本信息 ==================== */

#define MB_FULL_VERSION_MAJOR   1
#define MB_FULL_VERSION_MINOR   0
#define MB_FULL_VERSION_PATCH   0

/* ==================== 配置 ==================== */

#define MB_FULL_MAX_ADU_SIZE       260     /**< RTU 最大 ADU */
#define MB_FULL_MAX_TCP_ADU_SIZE   260     /**< TCP 最大 ADU */
#define MB_FULL_MAX_ASCII_SIZE     513     /**< ASCII 最大帧 */
#define MB_FULL_TIMEOUT_MS         1000    /**< 默认超时 (ms) */
#define MB_FULL_MAX_SLAVES         8       /**< 最大从站数 */

/* ==================== 传输模式 ==================== */

typedef enum {
    MB_MODE_RTU   = 0,    /**< RTU 模式 */
    MB_MODE_TCP   = 1,    /**< TCP 模式 */
    MB_MODE_ASCII = 2,    /**< ASCII 模式 */
} mb_mode_t;

/* ==================== 错误码 ==================== */

#define MB_FULL_OK                 0
#define MB_FULL_ERROR              (-1)
#define MB_FULL_INVALID_PARAM      (-2)
#define MB_FULL_TIMEOUT            (-3)
#define MB_FULL_CRC_ERROR          (-4)
#define MB_FULL_LRC_ERROR          (-5)
#define MB_FULL_NOT_INITIALIZED    (-6)
#define MB_FULL_NO_MEMORY          (-7)

/* ==================== 功能码 ==================== */

/* 0x00-0x0F: 位操作 */
#define MB_FUNC_READ_COILS                 0x01
#define MB_FUNC_READ_DISCRETE_INPUTS       0x02
#define MB_FUNC_WRITE_SINGLE_COIL          0x05
#define MB_FUNC_WRITE_MULTIPLE_COILS      0x0F

/* 0x10-0x1F: 寄存器操作 */
#define MB_FUNC_READ_HOLDING_REGISTERS    0x03
#define MB_FUNC_READ_INPUT_REGISTERS      0x04
#define MB_FUNC_WRITE_SINGLE_REGISTER     0x06
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS  0x10
#define MB_FUNC_READ_WRITE_MULTIPLE       0x17

/* 0x2B: 诊断 */
#define MB_FUNC_READ_DEVICE_ID             0x2E

/* ==================== 错误码 (从站) ==================== */

#define MB_ERR_ILLEGAL_FUNCTION            0x01
#define MB_ERR_ILLEGAL_DATA_ADDRESS       0x02
#define MB_ERR_ILLEGAL_DATA_VALUE         0x03
#define MB_ERR_SERVER_DEVICE_FAILURE      0x04
#define MB_ERR_ACKNOWLEDGE                0x05
#define MB_ERR_SERVER_DEVICE_BUSY         0x06
#define MB_ERR_MEMORY_PARITY              0x08
#define MB_ERR_GATEWAY_PATH              0x0A
#define MB_ERR_GATEWAY_TARGET            0x0B

/* ==================== 设备 ID 对象 ==================== */

typedef enum {
    MB_OBJ_BASIC               = 0x00,  /**< 基本设备 ID */
    MB_OBJ_REGULAR             = 0x01,  /**< 常规设备 ID */
    MB_OBJ_EXTENDED            = 0x02,  /**< 扩展设备 ID */
    MB_OBJ_SPECIFIC            = 0x03,  /**< 特定设备 ID */
} mb_object_id_t;

/**
 * @brief 设备 ID 信息
 */
typedef struct {
    uint8_t device_id;         /**< 设备 ID */
    uint8_t vendor_id[2];      /**< 供应商 ID */
    uint8_t product_code[2];   /**< 产品代码 */
    uint8_t major_rev;         /**< 主版本号 */
    uint8_t minor_rev;         /**< 次版本号 */
    uint8_t reserved[4];       /**< 保留 */
} mb_device_id_t;

/* ==================== 数据区 ==================== */

/**
 * @brief 线圈/离散输入
 */
typedef struct {
    uint8_t *data;             /**< 数据缓冲区 */
    uint16_t start_addr;       /**< 起始地址 */
    uint16_t count;            /**< 数量 */
} mb_bit_area_t;

/**
 * @brief 寄存器
 */
typedef struct {
    uint16_t *data;            /**< 数据缓冲区 */
    uint16_t start_addr;       /**< 起始地址 */
    uint16_t count;            /**< 数量 */
} mb_reg_area_t;

/* ==================== 从站设备 ==================== */

/**
 * @brief Modbus 从站
 */
typedef struct {
    uint8_t slave_id;                      /**< 从站 ID (1-247) */
    mb_mode_t mode;                         /**< 传输模式 */
    
    /* 数据区 */
    mb_bit_area_t coils;                    /**< 线圈 (0x) */
    mb_bit_area_t discrete;                 /**< 离散输入 (1x) */
    mb_reg_area_t holding;                 /**< 保持寄存器 (3x) */
    mb_reg_area_t input;                   /**< 输入寄存器 (4x) */
    
    /* 设备 ID (可选) */
    mb_device_id_t device_id;
    
    /* 缓冲区 */
    uint8_t rx_buf[MB_FULL_MAX_ADU_SIZE];
    uint8_t tx_buf[MB_FULL_MAX_ADU_SIZE];
    uint16_t rx_len;
    uint16_t tx_len;
    
    /* 统计 */
    uint32_t request_count;
    uint32_t error_count;
    uint32_t crc_error_count;
    
    bool initialized;
} mb_full_slave_t;

/* ==================== 主站设备 ==================== */

/**
 * @brief Modbus TCP 连接
 */
typedef struct {
    uint8_t slave_id;              /**< 从站 ID (TCP 模式下为单元ID) */
    uint8_t connected;             /**< 连接状态 */
    uint8_t transaction_id;       /**< 事务 ID (高字节) */
} mb_full_connection_t;

/**
 * @brief Modbus 主站
 */
typedef struct {
    mb_mode_t mode;                         /**< 传输模式 */
    uint8_t slave_id;                       /**< 当前从站 ID */
    uint32_t timeout_ms;                    /**< 超时时间 */
    
    /* 连接 (TCP) */
    mb_full_connection_t conn;
    
    /* 缓冲区 */
    uint8_t tx_buf[MB_FULL_MAX_TCP_ADU_SIZE];
    uint8_t rx_buf[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t tx_len;
    uint16_t rx_len;
    
    /* 统计 */
    uint32_t request_count;
    uint32_t error_count;
    
    bool initialized;
} mb_full_master_t;

/* ==================== TCP 帧头 ==================== */

/**
 * @brief Modbus TCP 帧头 (MBAP)
 */
typedef struct {
    uint16_t transaction_id;    /**< 事务 ID */
    uint16_t protocol_id;       /**< 协议 ID (0 = Modbus) */
    uint16_t length;            /**< 后续字节数 */
    uint8_t unit_id;            /**< 单元 ID */
} __attribute__((packed)) mb_tcp_header_t;

/* ==================== 串口接口 ==================== */

/**
 * @brief 串口发送回调
 */
typedef int (*mb_full_send_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief 串口接收回调
 */
typedef int (*mb_full_recv_cb_t)(uint8_t *data, uint16_t len, uint32_t timeout_ms);

/* ==================== TCP 接口 ==================== */

/**
 * @brief TCP 连接回调
 */
typedef int (*mb_full_connect_cb_t)(const char *host, uint16_t port);

/**
 * @brief TCP 发送回调
 */
typedef int (*mb_full_tcp_send_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief TCP 接收回调
 */
typedef int (*mb_full_tcp_recv_cb_t)(uint8_t *data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief TCP 关闭回调
 */
typedef int (*mb_full_disconnect_cb_t)(void);

/* ==================== 从站 API ==================== */

/**
 * @brief 初始化 Modbus 从站
 * @param slave 从站句柄
 * @param slave_id 从站 ID
 * @param mode 传输模式
 * @return MB_FULL_OK 成功
 */
int mb_full_slave_init(mb_full_slave_t *slave, uint8_t slave_id, mb_mode_t mode);

/**
 * @brief 配置线圈区
 */
int mb_full_slave_config_coils(mb_full_slave_t *slave, uint8_t *data,
                                uint16_t start_addr, uint16_t count);

/**
 * @brief 配置离散输入区
 */
int mb_full_slave_config_discrete(mb_full_slave_t *slave, uint8_t *data,
                                    uint16_t start_addr, uint16_t count);

/**
 * @brief 配置保持寄存器区
 */
int mb_full_slave_config_holding(mb_full_slave_t *slave, uint16_t *data,
                                   uint16_t start_addr, uint16_t count);

/**
 * @brief 配置输入寄存器区
 */
int mb_full_slave_config_input(mb_full_slave_t *slave, uint16_t *data,
                                uint16_t start_addr, uint16_t count);

/**
 * @brief 设置串口发送回调 (RTU/ASCII)
 */
void mb_full_slave_set_uart(mb_full_slave_t *slave, mb_full_send_cb_t send_cb);

/**
 * @brief 设置 TCP 发送回调 (TCP)
 */
void mb_full_slave_set_tcp(mb_full_slave_t *slave, mb_full_tcp_send_cb_t send_cb,
                            mb_full_tcp_recv_cb_t recv_cb);

/**
 * @brief 处理 RTU/ASCII 帧
 */
int mb_full_slave_handle(mb_full_slave_t *slave, const uint8_t *data, uint16_t len);

/**
 * @brief 处理 TCP 帧
 */
int mb_full_slave_handle_tcp(mb_full_slave_t *slave, const uint8_t *data, uint16_t len);

/* ==================== 主站 API ==================== */

/**
 * @brief 初始化 Modbus 主站
 */
int mb_full_master_init(mb_full_master_t *master, mb_mode_t mode);

/**
 * @brief 设置超时时间
 */
void mb_full_master_set_timeout(mb_full_master_t *master, uint32_t timeout_ms);

/**
 * @brief 设置串口接口 (RTU/ASCII)
 */
void mb_full_master_set_uart(mb_full_master_t *master, 
                              mb_full_send_cb_t send_cb,
                              mb_full_recv_cb_t recv_cb);

/**
 * @brief 设置 TCP 接口
 */
void mb_full_master_set_tcp(mb_full_master_t *master,
                             mb_full_connect_cb_t connect_cb,
                             mb_full_tcp_send_cb_t send_cb,
                             mb_full_tcp_recv_cb_t recv_cb,
                             mb_full_disconnect_cb_t disconnect_cb);

/**
 * @brief 连接到 TCP 服务器
 */
int mb_full_master_connect(mb_full_master_t *master, const char *host, uint16_t port);

/**
 * @brief 断开 TCP 连接
 */
int mb_full_master_disconnect(mb_full_master_t *master);

/**
 * @brief 读线圈 (0x01)
 */
int mb_full_master_read_coils(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint8_t *data);

/**
 * @brief 读离散输入 (0x02)
 */
int mb_full_master_read_discrete(mb_full_master_t *master, uint8_t slave_id,
                                  uint16_t addr, uint16_t count, uint8_t *data);

/**
 * @brief 读保持寄存器 (0x03)
 */
int mb_full_master_read_holding(mb_full_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data);

/**
 * @brief 读输入寄存器 (0x04)
 */
int mb_full_master_read_input(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count, uint16_t *data);

/**
 * @brief 写单个线圈 (0x05)
 */
int mb_full_master_write_coil(mb_full_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t value);

/**
 * @brief 写单个寄存器 (0x06)
 */
int mb_full_master_write_register(mb_full_master_t *master, uint8_t slave_id,
                                   uint16_t addr, uint16_t value);

/**
 * @brief 写多个线圈 (0x0F)
 */
int mb_full_master_write_coils(mb_full_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, const uint8_t *data);

/**
 * @brief 写多个寄存器 (0x10)
 */
int mb_full_master_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count, const uint16_t *data);

/**
 * @brief 读写多个寄存器 (0x17)
 */
int mb_full_master_read_write_registers(mb_full_master_t *master, uint8_t slave_id,
                                         uint16_t read_addr, uint16_t read_count,
                                         uint16_t *read_data,
                                         uint16_t write_addr, uint16_t write_count,
                                         const uint16_t *write_data);

/**
 * @brief 读设备 ID (0x2E)
 */
int mb_full_master_read_device_id(mb_full_master_t *master, uint8_t slave_id,
                                    mb_object_id_t object_id, uint8_t *data);

/* ==================== 工具函数 ==================== */

/**
 * @brief 计算 CRC16 (Modbus RTU)
 */
uint16_t mb_full_crc16(const uint8_t *data, uint16_t len);

/**
 * @brief 计算 LRC (Modbus ASCII)
 */
uint8_t mb_full_lrc(const uint8_t *data, uint16_t len);

/**
 * @brief ASCII 帧解码
 * @return 解码后的字节数
 */
int mb_full_ascii_decode(const uint8_t *src, uint16_t src_len, uint8_t *dst);

/**
 * @brief ASCII 帧编码
 * @return 编码后的字符数
 */
int mb_full_ascii_encode(const uint8_t *src, uint16_t src_len, uint8_t *dst);

/**
 * @brief 获取版本字符串
 */
const char* mb_full_version(void);

/**
 * @brief 获取错误字符串
 */
const char* mb_full_error_string(int error);

#ifdef __cplusplus
}
#endif

#endif /* MB_FULL_H */
