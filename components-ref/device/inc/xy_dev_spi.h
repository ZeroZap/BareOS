/**
 * @file xy_dev_spi.h
 * @brief XinYi SPI Device Driver API
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEV_SPI_H
#define XY_DEV_SPI_H

#include "xy_device.h"
#include "xy_hal_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI 模式
 */
typedef enum {
    XY_SPI_MODE_0 = 0,              /**< CPOL=0, CPHA=0 */
    XY_SPI_MODE_1,                  /**< CPOL=0, CPHA=1 */
    XY_SPI_MODE_2,                  /**< CPOL=1, CPHA=0 */
    XY_SPI_MODE_3,                  /**< CPOL=1, CPHA=1 */
} xy_spi_mode_t;

/**
 * @brief SPI 方向
 */
typedef enum {
    XY_SPI_DIR_2LINES = 0,          /**< 全双工 */
    XY_SPI_DIR_2LINES_RXONLY,       /**< 双线只接收 */
    XY_SPI_DIR_1LINE,               /**< 单线 */
} xy_spi_direction_t;

/**
 * @brief SPI 数据大小
 */
typedef enum {
    XY_SPI_DATASIZE_4B = 3,         /**< 4 位数据 */
    XY_SPI_DATASIZE_5B = 4,         /**< 5 位数据 */
    XY_SPI_DATASIZE_6B = 5,         /**< 6 位数据 */
    XY_SPI_DATASIZE_7B = 6,         /**< 7 位数据 */
    XY_SPI_DATASIZE_8B = 7,         /**< 8 位数据 */
    XY_SPI_DATASIZE_9B = 8,         /**< 9 位数据 */
    XY_SPI_DATASIZE_10B = 9,        /**< 10 位数据 */
    XY_SPI_DATASIZE_11B = 10,       /**< 11 位数据 */
    XY_SPI_DATASIZE_12B = 11,       /**< 12 位数据 */
    XY_SPI_DATASIZE_13B = 12,       /**< 13 位数据 */
    XY_SPI_DATASIZE_14B = 13,       /**< 14 位数据 */
    XY_SPI_DATASIZE_15B = 14,       /**< 15 位数据 */
    XY_SPI_DATASIZE_16B = 15,       /**< 16 位数据 */
} xy_spi_datasize_t;

/**
 * @brief SPI 位顺序
 */
typedef enum {
    XY_SPI_BITORDER_MSB = 0,        /**< MSB 先传 */
    XY_SPI_BITORDER_LSB,            /**< LSB 先传 */
} xy_spi_bitorder_t;

/**
 * @brief SPI NSS 模式
 */
typedef enum {
    XY_SPI_NSS_SOFT = 0,            /**< 软件 NSS */
    XY_SPI_NSS_HARD_INPUT,          /**< 硬件 NSS 输入 */
    XY_SPI_NSS_HARD_OUTPUT,         /**< 硬件 NSS 输出 */
} xy_spi_nss_mode_t;

/**
 * @brief SPI 配置结构
 */
typedef struct {
    xy_spi_mode_t mode;              /**< SPI 模式 */
    xy_spi_direction_t direction;    /**< 传输方向 */
    xy_spi_datasize_t datasize;      /**< 数据大小 */
    xy_spi_bitorder_t bitorder;      /**< 位顺序 */
    xy_spi_nss_mode_t nss_mode;      /**< NSS 模式 */
    uint32_t baudrate;              /**< 波特率 */
    uint8_t is_master;              /**< 是否主机 */
    uint8_t crc_enable;             /**< CRC 使能 */
    uint8_t fifo_threshold;         /**< FIFO 阈值 */
} xy_spi_config_t;

/**
 * @brief SPI 事件类型
 */
typedef enum {
    XY_SPI_EVT_TX_DONE = 0,          /**< 发送完成 */
    XY_SPI_EVT_RX_DONE,              /**< 接收完成 */
    XY_SPI_EVT_TX_RX_DONE,           /**< 发送接收完成 */
    XY_SPI_EVT_ERROR,                /**< 错误事件 */
    XY_SPI_EVT_CS_CHANGE,            /**< 片选变化 */
    XY_SPI_EVT_OVERRUN,              /**< 溢出错误 */
    XY_SPI_EVT_MODE_FAULT,           /**< 模式错误 */
    XY_SPI_EVT_CRC_ERROR,            /**< CRC 错误 */
} xy_spi_evt_t;

/**
 * @brief SPI 回调类型
 */
typedef void (*xy_spi_callback_t)(void *dev, xy_spi_evt_t event, void *arg);

/**
 * @brief SPI 驱动 API 结构
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_spi_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*transfer)(struct xy_device *dev, const uint8_t *tx_data,
                       uint8_t *rx_data, size_t size, uint32_t timeout);
    xy_error_t (*set_speed)(struct xy_device *dev, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *dev);
    xy_error_t (*set_mode)(struct xy_device *dev, xy_spi_mode_t mode);
    xy_error_t (*set_datasize)(struct xy_device *dev, xy_spi_datasize_t size);
    xy_error_t (*async_transfer)(struct xy_device *dev, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t size,
                                xy_spi_callback_t cb, void *arg);
    xy_error_t (*select_device)(struct xy_device *dev, uint8_t device_id);
    xy_error_t (*deselect_device)(struct xy_device *dev, uint8_t device_id);
    xy_error_t (*set_power_mode)(struct xy_device *dev, uint8_t power_mode);
    xy_error_t (*get_power_mode)(struct xy_device *dev, uint8_t *power_mode);
    xy_error_t (*register_callback)(struct xy_device *dev, 
                                   xy_spi_callback_t callback, void *arg);
} xy_spi_api_t;

/**
 * @brief 初始化 SPI 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_init(void *dev, const xy_spi_config_t *config);

/**
 * @brief 反初始化 SPI 设备
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_deinit(void *dev);

/**
 * @brief SPI 数据传输
 * @param dev 设备指针
 * @param tx_data 发送数据
 * @param rx_data 接收数据缓冲区
 * @param size 数据大小
 * @param timeout 超时时间 (ms)
 * @return 实际传输字节数，负值表示错误
 */
int32_t xy_spi_dev_transfer(void *dev, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t size, uint32_t timeout);

/**
 * @brief 异步 SPI 数据传输
 * @param dev 设备指针
 * @param tx_data 发送数据
 * @param rx_data 接收数据缓冲区
 * @param size 数据大小
 * @param cb 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_async_transfer(void *dev, const uint8_t *tx_data,
                                    uint8_t *rx_data, size_t size,
                                    xy_spi_callback_t cb, void *arg);

/**
 * @brief 设置 SPI 速度
 * @param dev 设备指针
 * @param speed 速度 (Hz)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_set_speed(void *dev, uint32_t speed);

/**
 * @brief 获取 SPI 速度
 * @param dev 设备指针
 * @return 速度 (Hz)，负值表示错误
 */
int32_t xy_spi_dev_get_speed(void *dev);

/**
 * @brief 设置 SPI 模式
 * @param dev 设备指针
 * @param mode SPI 模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_set_mode(void *dev, xy_spi_mode_t mode);

/**
 * @brief 获取 SPI 模式
 * @param dev 设备指针
 * @return SPI 模式，负值表示错误
 */
int32_t xy_spi_dev_get_mode(void *dev);

/**
 * @brief 设置数据大小
 * @param dev 设备指针
 * @param size 数据大小
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_set_datasize(void *dev, xy_spi_datasize_t size);

/**
 * @brief 获取数据大小
 * @param dev 设备指针
 * @return 数据大小，负值表示错误
 */
int32_t xy_spi_dev_get_datasize(void *dev);

/**
 * @brief 选择 SPI 设备
 * @param dev 设备指针
 * @param device_id 设备 ID
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_select_device(void *dev, uint8_t device_id);

/**
 * @brief 取消选择 SPI 设备
 * @param dev 设备指针
 * @param device_id 设备 ID
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_deselect_device(void *dev, uint8_t device_id);

/**
 * @brief 注册 SPI 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_register_callback(void *dev, 
                                       xy_spi_callback_t callback, 
                                       void *arg);

/**
 * @brief SPI 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_spi_dev_control(void *dev, uint32_t cmd, void *args);

/* SPI 控制命令 */
#define XY_SPI_CMD_SET_CONFIG        0x01
#define XY_SPI_CMD_GET_CONFIG        0x02
#define XY_SPI_CMD_SET_SPEED         0x03
#define XY_SPI_CMD_GET_SPEED         0x04
#define XY_SPI_CMD_SET_MODE          0x05
#define XY_SPI_CMD_GET_MODE          0x06
#define XY_SPI_CMD_SET_DATASIZE      0x07
#define XY_SPI_CMD_GET_DATASIZE      0x08
#define XY_SPI_CMD_SELECT_DEVICE     0x09
#define XY_SPI_CMD_DESELECT_DEVICE   0x0A
#define XY_SPI_CMD_FLUSH             0x0B
#define XY_SPI_CMD_SET_CALLBACK      0x0C
#define XY_SPI_CMD_GET_STATE         0x0D
#define XY_SPI_CMD_RESET             0x0E
#define XY_SPI_CMD_ENABLE_CRC        0x0F
#define XY_SPI_CMD_DISABLE_CRC       0x10
#define XY_SPI_CMD_SET_FIFO_THRESHOLD 0x11
#define XY_SPI_CMD_GET_FIFO_THRESHOLD 0x12
#define XY_SPI_CMD_SET_POWER_MODE     0x13
#define XY_SPI_CMD_GET_POWER_MODE     0x14

#ifdef __cplusplus
}
#endif

#endif /* XY_DEV_SPI_H */
