/**
 * @file xy_w25qxx.h
 * @brief W25Qxx SPI Flash Memory Driver
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 W25Q16/W25Q32/W25Q64/W25Q128 系列 SPI Flash
 */

#ifndef XY_W25QXX_H
#define XY_W25QXX_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== W25Qxx Device Structure ==================== */

/**
 * @brief W25Qxx 设备结构
 */
typedef struct {
    xy_device_t base;              /**< 设备基类 */
    void *spi_handle;              /**< SPI 句柄 */
    void *cs_gpio;                 /**< 片选 GPIO */
    uint32_t capacity_bytes;       /**< 容量 (字节) */
    uint8_t manufacturer_id;       /**< 制造商 ID */
    uint8_t memory_type;           /**< 存储器类型 */
    uint8_t memory_capacity;       /**< 存储器容量 */
    uint8_t sector_size;           /**< 扇区大小 (KB) */
    uint16_t page_size;            /**< 页大小 (字节) */
    uint32_t block_count;          /**< 块数量 */
    uint32_t sector_count;         /**< 扇区数量 */
    bool write_enabled;            /**< 写使能标志 */
} xy_w25qxx_t;

/* ==================== W25Qxx Commands ==================== */

/* 基础命令 */
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS_REG1   0x05
#define W25Q_CMD_READ_STATUS_REG2   0x35
#define W25Q_CMD_WRITE_STATUS_REG   0x01
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_SECTOR_ERASE       0x20
#define W25Q_CMD_BLOCK_ERASE_32K    0x52
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_FAST_READ          0x0B
#define W25Q_CMD_FAST_READ_DUAL     0x3B
#define W25Q_CMD_FAST_READ_QUAD     0x6B
#define W25Q_CMD_RELEASE_POWERDOWN  0xAB
#define W25Q_CMD_DEVICE_ID          0x90
#define W25Q_CMD_JEDEC_ID           0x9F
#define W25Q_CMD_UNIQUE_ID          0x4B

/* 状态寄存器位定义 */
#define W25Q_STATUS_BUSY            (1 << 0)
#define W25Q_STATUS_WEL             (1 << 1)
#define W25Q_STATUS_BP0             (1 << 2)
#define W25Q_STATUS_BP1             (1 << 3)
#define W25Q_STATUS_BP2             (1 << 4)
#define W25Q_STATUS_TB              (1 << 5)
#define W25Q_STATUS_SEC             (1 << 6)
#define W25Q_STATUS_SRWD            (1 << 7)

/* ==================== W25Qxx API ==================== */

/**
 * @brief 初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @param spi_handle SPI 句柄
 * @param cs_gpio 片选 GPIO
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, void *cs_gpio);

/**
 * @brief 反初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_deinit(xy_w25qxx_t *dev);

/**
 * @brief 读取制造商和设备 ID
 * @param dev W25Qxx 设备句柄
 * @param manufacturer_id 制造商 ID (输出)
 * @param device_id 设备 ID (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_device_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id, 
                             uint8_t *device_id);

/**
 * @brief 读取 JEDEC ID
 * @param dev W25Qxx 设备句柄
 * @param jedec_id JEDEC ID 缓冲区 (3 字节)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_jedec_id(xy_w25qxx_t *dev, uint8_t *jedec_id);

/**
 * @brief 读取状态寄存器 1
 * @param dev W25Qxx 设备句柄
 * @param status 状态值 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_status_reg1(xy_w25qxx_t *dev, uint8_t *status);

/**
 * @brief 等待空闲
 * @param dev W25Qxx 设备句柄
 * @param timeout_ms 超时时间
 * @return XY_DEVICE_OK 成功，XY_DEVICE_TIMEOUT 超时
 */
int xy_w25qxx_wait_idle(xy_w25qxx_t *dev, uint32_t timeout_ms);

/**
 * @brief 写使能
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_write_enable(xy_w25qxx_t *dev);

/**
 * @brief 页编程 (256 字节)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据缓冲区
 * @param length 数据长度 (最大 256 字节)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_page_program(xy_w25qxx_t *dev, uint32_t addr, 
                           const uint8_t *data, uint16_t length);

/**
 * @brief 扇区擦除 (4KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 扇区地址
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_sector_erase(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 块擦除 (32KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 块地址
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_block_erase_32k(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 块擦除 (64KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 块地址
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_block_erase_64k(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 整片擦除
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_chip_erase(xy_w25qxx_t *dev);

/**
 * @brief 读取数据
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read(xy_w25qxx_t *dev, uint32_t addr, 
                   uint8_t *data, uint32_t length);

/**
 * @brief 写入数据 (自动处理页边界)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_write(xy_w25qxx_t *dev, uint32_t addr, 
                    const uint8_t *data, uint32_t length);

/**
 * @brief 进入掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_powerdown(xy_w25qxx_t *dev);

/**
 * @brief 退出掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_release_powerdown(xy_w25qxx_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_W25QXX_H */
