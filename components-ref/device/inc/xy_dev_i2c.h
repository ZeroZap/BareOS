/**
 * @file xy_dev_i2c.h
 * @brief XinYi I2C Device Driver API
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEV_I2C_H
#define XY_DEV_I2C_H

#include "xy_device.h"
#include "xy_hal_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C 地址模式
 */
typedef enum {
    XY_I2C_ADDR_7BIT = 0,           /**< 7 位地址 */
    XY_I2C_ADDR_10BIT,             /**< 10 位地址 */
} xy_i2c_addr_mode_t;

/**
 * @brief I2C 时钟速度
 */
typedef enum {
    XY_I2C_SPEED_STANDARD = 0,      /**< 标准速度 (100kHz) */
    XY_I2C_SPEED_FAST,             /**< 快速速度 (400kHz) */
    XY_I2C_SPEED_FAST_PLUS,        /**< 快速+速度 (1MHz) */
    XY_I2C_SPEED_HIGH,             /**< 高速 (3.4MHz) */
} xy_i2c_speed_t;

/**
 * @brief I2C 时钟占空比
 */
typedef enum {
    XY_I2C_DUTY_2 = 0,             /**< T_low/T_high = 2 */
    XY_I2C_DUTY_16_9,              /**< T_low/T_high = 16/9 */
} xy_i2c_duty_t;

/**
 * @brief I2C 配置结构
 */
typedef struct {
    uint32_t clock_speed;           /**< 时钟速度 (Hz) */
    xy_i2c_addr_mode_t addr_mode;   /**< 地址模式 */
    xy_i2c_duty_t duty_cycle;       /**< 占空比 */
    uint16_t own_address;           /**< 自己地址 */
    uint8_t general_call_mode;      /**< 通用呼叫模式 */
    uint8_t no_stretch_mode;        /**< 无拉伸模式 */
    uint8_t analog_filter;          /**< 模拟滤波器 */
    uint8_t digital_filter;         /**< 数字滤波器 */
} xy_i2c_config_t;

/**
 * @brief I2C 事件类型
 */
typedef enum {
    XY_I2C_EVT_TX_DONE = 0,         /**< 发送完成 */
    XY_I2C_EVT_RX_DONE,             /**< 接收完成 */
    XY_I2C_EVT_TX_RX_DONE,          /**< 发送接收完成 */
    XY_I2C_EVT_ERROR,               /**< 错误事件 */
    XY_I2C_EVT_NACK,                /**< NACK 事件 */
    XY_I2C_EVT_ARBITRATION_LOST,    /**< 仲裁丢失 */
    XY_I2C_EVT_BUS_ERROR,           /**< 总线错误 */
    XY_I2C_EVT_TIMEOUT,             /**< 超时事件 */
} xy_i2c_evt_t;

/**
 * @brief I2C 回调类型
 */
typedef void (*xy_i2c_callback_t)(void *dev, xy_i2c_evt_t event, void *arg);

/**
 * @brief I2C 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_i2c_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*master_send)(struct xy_device *dev, uint16_t dev_addr,
                             const uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*master_recv)(struct xy_device *dev, uint16_t dev_addr,
                             uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*mem_write)(struct xy_device *dev, uint16_t dev_addr,
                           uint16_t mem_addr, const uint8_t *data, size_t len,
                           uint32_t timeout);
    xy_error_t (*mem_read)(struct xy_device *dev, uint16_t dev_addr,
                          uint16_t mem_addr, uint8_t *data, size_t len,
                          uint32_t timeout);
    xy_error_t (*is_device_ready)(struct xy_device *dev, uint16_t dev_addr,
                                  uint32_t trials, uint32_t timeout);
    xy_error_t (*async_transfer)(struct xy_device *dev, uint16_t dev_addr,
                                const uint8_t *tx_data, uint8_t *rx_data, size_t size,
                                xy_i2c_callback_t cb, void *arg);
    xy_error_t (*set_speed)(struct xy_device *dev, xy_i2c_speed_t speed);
    xy_i2c_speed_t (*get_speed)(struct xy_device *dev);
    xy_error_t (*set_addr_mode)(struct xy_device *dev, xy_i2c_addr_mode_t mode);
    xy_i2c_addr_mode_t (*get_addr_mode)(struct xy_device *dev);
    xy_error_t (*set_power_mode)(struct xy_device *dev, uint8_t power_mode);
    xy_error_t (*get_power_mode)(struct xy_device *dev, uint8_t *power_mode);
    xy_error_t (*register_callback)(struct xy_device *dev, 
                                   xy_i2c_callback_t callback, void *arg);
} xy_i2c_api_t;

/**
 * @brief 初始化 I2C 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_init(void *dev, const xy_i2c_config_t *config);

/**
 * @brief 反初始化 I2C 设备
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_deinit(void *dev);

/**
 * @brief I2C 主机发送
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout 超时时间 (ms)
 * @return 实际发送字节数，负值表示错误
 */
int32_t xy_i2c_dev_master_send(void *dev, uint16_t dev_addr,
                               const uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief I2C 主机接收
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param timeout 超时时间 (ms)
 * @return 实际接收字节数，负值表示错误
 */
int32_t xy_i2c_dev_master_recv(void *dev, uint16_t dev_addr,
                               uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief I2C 内存写入
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout 超时时间 (ms)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_mem_write(void *dev, uint16_t dev_addr,
                                uint16_t mem_addr, const uint8_t *data, 
                                size_t len, uint32_t timeout);

/**
 * @brief I2C 内存读取
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param mem_addr 内存地址
 * @param data 数据缓冲区
 * @param len 缓冲区长度
 * @param timeout 超时时间 (ms)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_mem_read(void *dev, uint16_t dev_addr,
                               uint16_t mem_addr, uint8_t *data, 
                               size_t len, uint32_t timeout);

/**
 * @brief 检查设备就绪
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param trials 重试次数
 * @param timeout 超时时间 (ms)
 * @return XY_OK 设备就绪，其他值失败
 */
xy_error_t xy_i2c_dev_is_device_ready(void *dev, uint16_t dev_addr,
                                     uint32_t trials, uint32_t timeout);

/**
 * @brief 异步 I2C 传输
 * @param dev 设备指针
 * @param dev_addr 设备地址
 * @param tx_data 发送数据
 * @param rx_data 接收数据缓冲区
 * @param size 数据大小
 * @param cb 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_async_transfer(void *dev, uint16_t dev_addr,
                                    const uint8_t *tx_data, uint8_t *rx_data,
                                    size_t size, xy_i2c_callback_t cb, void *arg);

/**
 * @brief 设置 I2C 速度
 * @param dev 设备指针
 * @param speed 速度
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_set_speed(void *dev, xy_i2c_speed_t speed);

/**
 * @brief 获取 I2C 速度
 * @param dev 设备指针
 * @return 速度，负值表示错误
 */
int32_t xy_i2c_dev_get_speed(void *dev);

/**
 * @brief 设置地址模式
 * @param dev 设备指针
 * @param mode 地址模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_set_addr_mode(void *dev, xy_i2c_addr_mode_t mode);

/**
 * @brief 获取地址模式
 * @param dev 设备指针
 * @return 地址模式，负值表示错误
 */
int32_t xy_i2c_dev_get_addr_mode(void *dev);

/**
 * @brief 注册 I2C 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_register_callback(void *dev, 
                                       xy_i2c_callback_t callback, 
                                       void *arg);

/**
 * @brief I2C 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_i2c_dev_control(void *dev, uint32_t cmd, void *args);

/* I2C 控制命令 */
#define XY_I2C_CMD_SET_CONFIG        0x01
#define XY_I2C_CMD_GET_CONFIG        0x02
#define XY_I2C_CMD_SET_SPEED         0x03
#define XY_I2C_CMD_GET_SPEED         0x04
#define XY_I2C_CMD_SET_ADDR_MODE     0x05
#define XY_I2C_CMD_GET_ADDR_MODE     0x06
#define XY_I2C_CMD_SET_CALLBACK      0x07
#define XY_I2C_CMD_GET_STATE         0x08
#define XY_I2C_CMD_RESET             0x09
#define XY_I2C_CMD_FLUSH             0x0A
#define XY_I2C_CMD_SET_TIMEOUT       0x0B
#define XY_I2C_CMD_GET_TIMEOUT       0x0C
#define XY_I2C_CMD_SET_TRIALS        0x0D
#define XY_I2C_CMD_GET_TRIALS        0x0E
#define XY_I2C_CMD_ENABLE_ANALOG_FILTER 0x0F
#define XY_I2C_CMD_DISABLE_ANALOG_FILTER 0x10
#define XY_I2C_CMD_ENABLE_DIGITAL_FILTER 0x11
#define XY_I2C_CMD_DISABLE_DIGITAL_FILTER 0x12
#define XY_I2C_CMD_SET_DIGITAL_FILTER_COEFF 0x13
#define XY_I2C_CMD_GET_DIGITAL_FILTER_COEFF 0x14
#define XY_I2C_CMD_SET_POWER_MODE    0x15
#define XY_I2C_CMD_GET_POWER_MODE    0x16

#ifdef __cplusplus
}
#endif

#endif /* XY_DEV_I2C_H */
