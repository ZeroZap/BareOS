/**
 * @file xy_mux.h
 * @brief Universal MUX Interface Library - Software Defined Hardware
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * @brief 通用 MUX 接口库 - 软件定义硬件
 * 
 * 核心思想:
 * 1. 通过单一物理接口 (USB/UART/SPI) 复用多种外设
 * 2. 统一的协议封装 (TLV - Type-Length-Value)
 * 3. 插件式外设驱动管理
 * 4. 跨平台支持 (MCU + PC SDK)
 * 
 * 支持的外设类型:
 * - GPIO (输入/输出/中断)
 * - UART (多路串口)
 * - I2C (多路总线)
 * - SPI (多路总线)
 * - PWM (多通道)
 * - ADC (多通道采集)
 * - Sensor (传感器数据)
 * - Custom (自定义外设)
 */

#ifndef XY_MUX_H
#define XY_MUX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 版本信息 ==================== */

#define XY_MUX_VERSION_MAJOR    1
#define XY_MUX_VERSION_MINOR    0
#define XY_MUX_VERSION_PATCH    0
#define XY_MUX_VERSION_STRING   "1.0.0"

/* ==================== MUX 外设类型定义 ==================== */

/**
 * @brief MUX 外设类型
 */
typedef enum {
    XY_MUX_TYPE_NONE = 0,       /**< 无类型 */
    XY_MUX_TYPE_GPIO,           /**< GPIO */
    XY_MUX_TYPE_UART,           /**< UART */
    XY_MUX_TYPE_I2C,            /**< I2C */
    XY_MUX_TYPE_SPI,            /**< SPI */
    XY_MUX_TYPE_PWM,            /**< PWM */
    XY_MUX_TYPE_ADC,            /**< ADC */
    XY_MUX_TYPE_DAC,            /**< DAC */
    XY_MUX_TYPE_SENSOR,         /**< 传感器 */
    XY_MUX_TYPE_LOG,            /**< 日志输出 */
    XY_MUX_TYPE_CONFIG,         /**< 配置管理 */
    XY_MUX_TYPE_CUSTOM = 0x80,  /**< 自定义类型 */
} xy_mux_type_t;

/**
 * @brief MUX 错误码
 */
typedef enum {
    XY_MUX_OK = 0,              /**< 成功 */
    XY_MUX_ERROR,               /**< 通用错误 */
    XY_MUX_ERROR_BUSY,          /**< 忙 */
    XY_MUX_ERROR_NOT_SUPPORTED, /**< 不支持 */
    XY_MUX_ERROR_INVALID_PARAM, /**< 无效参数 */
    XY_MUX_ERROR_NO_MEMORY,     /**< 内存不足 */
    XY_MUX_ERROR_TIMEOUT,       /**< 超时 */
    XY_MUX_ERROR_NO_DEVICE,     /**< 无设备 */
} xy_mux_error_t;

/* ==================== TLV 协议定义 ==================== */

/**
 * @brief TLV 包头 (4 字节)
 */
typedef struct __attribute__((packed)) {
    uint8_t type;       /**< 外设类型 */
    uint8_t channel;    /**< 通道号 */
    uint16_t length;    /**< 数据长度 */
} xy_mux_header_t;

/**
 * @brief TLV 数据包
 */
typedef struct __attribute__((packed)) {
    xy_mux_header_t header; /**< 包头 */
    uint8_t data[];         /**< 数据 */
} xy_mux_packet_t;

/* ==================== MUX 管理器 ==================== */

/**
 * @brief MUX 外设操作接口
 */
typedef struct {
    int32_t (*init)(uint8_t channel, const void *config);
    int32_t (*deinit)(uint8_t channel);
    int32_t (*read)(uint8_t channel, void *data, size_t len);
    int32_t (*write)(uint8_t channel, const void *data, size_t len);
    int32_t (*ioctl)(uint8_t channel, int cmd, void *arg);
} xy_mux_ops_t;

/**
 * @brief MUX 外设节点
 */
typedef struct xy_mux_device {
    struct xy_mux_device *next;   /**< 下一节点 */
    xy_mux_type_t type;           /**< 外设类型 */
    uint8_t channel;              /**< 通道号 */
    const xy_mux_ops_t *ops;      /**< 操作接口 */
    void *user_data;              /**< 用户数据 */
    bool enabled;                 /**< 使能状态 */
} xy_mux_device_t;

/**
 * @brief MUX 管理器
 */
typedef struct {
    xy_mux_device_t *devices;     /**< 设备链表 */
    uint16_t device_count;        /**< 设备数量 */
    uint16_t max_packet_size;     /**< 最大包大小 */
    uint8_t *tx_buffer;           /**< 发送缓冲区 */
    uint8_t *rx_buffer;           /**< 接收缓冲区 */
    size_t buffer_size;           /**< 缓冲区大小 */
} xy_mux_manager_t;

/* ==================== 核心 API ==================== */

/**
 * @brief 初始化 MUX 管理器
 * @param mgr MUX 管理器指针
 * @param tx_buffer 发送缓冲区
 * @param rx_buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_init(xy_mux_manager_t *mgr, 
                    uint8_t *tx_buffer, uint8_t *rx_buffer, 
                    size_t buffer_size);

/**
 * @brief 反初始化 MUX 管理器
 * @param mgr MUX 管理器指针
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_deinit(xy_mux_manager_t *mgr);

/**
 * @brief 注册 MUX 外设
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @param ops 操作接口
 * @param user_data 用户数据
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_register(xy_mux_manager_t *mgr,
                        xy_mux_type_t type, uint8_t channel,
                        const xy_mux_ops_t *ops, void *user_data);

/**
 * @brief 注销 MUX 外设
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_unregister(xy_mux_manager_t *mgr,
                          xy_mux_type_t type, uint8_t channel);

/**
 * @brief 查找 MUX 外设
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @return 设备指针，NULL 表示未找到
 */
xy_mux_device_t* xy_mux_find(xy_mux_manager_t *mgr,
                             xy_mux_type_t type, uint8_t channel);

/**
 * @brief 处理接收到的数据包
 * @param mgr MUX 管理器指针
 * @param packet 数据包
 * @param len 数据长度
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_process_packet(xy_mux_manager_t *mgr,
                              const uint8_t *packet, size_t len);

/**
 * @brief 构建发送数据包
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @param data 数据
 * @param len 数据长度
 * @param out_packet 输出数据包
 * @param out_len 输出长度
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_build_packet(xy_mux_manager_t *mgr,
                            xy_mux_type_t type, uint8_t channel,
                            const void *data, size_t len,
                            uint8_t *out_packet, size_t *out_len);

/**
 * @brief 读取外设数据
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return 读取字节数，负值表示错误
 */
int32_t xy_mux_read(xy_mux_manager_t *mgr,
                    xy_mux_type_t type, uint8_t channel,
                    void *data, size_t len);

/**
 * @brief 写入外设数据
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @param data 数据
 * @param len 数据长度
 * @return 写入字节数，负值表示错误
 */
int32_t xy_mux_write(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     const void *data, size_t len);

/**
 * @brief 控制外设
 * @param mgr MUX 管理器指针
 * @param type 外设类型
 * @param channel 通道号
 * @param cmd 命令
 * @param arg 参数
 * @return XY_MUX_OK 成功，其他值失败
 */
int32_t xy_mux_ioctl(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     int cmd, void *arg);

/* ==================== 辅助函数 ==================== */

/**
 * @brief 获取外设类型字符串
 * @param type 外设类型
 * @return 类型字符串
 */
const char* xy_mux_type_to_string(xy_mux_type_t type);

/**
 * @brief 解析外设类型
 * @param str 类型字符串
 * @return 外设类型
 */
xy_mux_type_t xy_mux_string_to_type(const char *str);

/**
 * @brief 获取设备数量
 * @param mgr MUX 管理器指针
 * @return 设备数量
 */
uint16_t xy_mux_get_device_count(xy_mux_manager_t *mgr);

/**
 * @brief 获取设备列表
 * @param mgr MUX 管理器指针
 * @param devices 设备数组
 * @param max_count 最大数量
 * @return 实际数量
 */
uint16_t xy_mux_get_device_list(xy_mux_manager_t *mgr,
                                xy_mux_device_t **devices,
                                uint16_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* XY_MUX_H */
