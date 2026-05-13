/**
 * @file xy_w25qxx.h
 * @brief W25Qxx SPI Flash Driver (W25Q16/W25Q32/W25Q64)
 * @version 1.0.0
 * @date 2026-03-01 凌晨 2:30
 */

#ifndef XY_W25QXX_H
#define XY_W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_spi.h"
#include <stdint.h>

/**
 * @brief W25Qxx 命令
 */
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS_REG1   0x05
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_SECTOR_ERASE       0x20  /* 4KB */
#define W25Q_CMD_BLOCK_ERASE_32K    0x52  /* 32KB */
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8  /* 64KB */
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_FAST_READ          0x0B
#define W25Q_CMD_POWER_DOWN         0xB9
#define W25Q_CMD_RELEASE_POWER_DOWN 0xAB
#define W25Q_CMD_MANUFACTURER_ID    0x90
#define W25Q_CMD_JEDEC_ID           0x9F

/**
 * @brief W25Qxx 型号 ID
 */
#define W25Q16_ID                   0x15
#define W25Q32_ID                   0x16
#define W25Q64_ID                   0x17

/**
 * @brief 错误码
 */
#define XY_W25Q_OK                  0
#define XY_W25Q_ERROR               (-1)
#define XY_W25Q_INVALID_PARAM       (-2)
#define XY_W25Q_NOT_FOUND           (-3)
#define XY_W25Q_BUSY                (-4)
#define XY_W25Q_WRITE_PROTECTED     (-5)

/**
 * @brief Flash 容量类型
 */
typedef enum {
    W25Q_UNKNOWN = 0,
    W25Q16 = 16,      /* 2MB */
    W25Q32 = 32,      /* 4MB */
    W25Q64 = 64,      /* 8MB */
} xy_w25q_model_t;

/**
 * @brief Flash 设备结构
 */
typedef struct {
    xy_spi_device_t spi_dev;    /**< SPI 设备 */
    uint8_t cs_pin;             /**< 片选引脚 */
    xy_w25q_model_t model;      /**< 型号 */
    uint32_t capacity_bytes;    /**< 容量 (字节) */
    uint32_t sector_count;      /**< 扇区数 (4KB) */
    uint8_t initialized;        /**< 初始化标志 */
} xy_w25qxx_t;

/**
 * @brief 初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @param spi_handle SPI 句柄
 * @param cs_pin 片选引脚
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, uint8_t cs_pin);

/**
 * @brief 反初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_deinit(xy_w25qxx_t *dev);

/**
 * @brief 读取 Flash ID
 * @param dev W25Qxx 设备句柄
 * @param manufacturer_id 制造商 ID 指针
 * @param device_id 设备 ID 指针
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_read_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id, uint8_t *device_id);

/**
 * @brief 读取状态寄存器
 * @param dev W25Qxx 设备句柄
 * @param status 状态寄存器值指针
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_read_status(xy_w25qxx_t *dev, uint8_t *status);

/**
 * @brief 等待空闲
 * @param dev W25Qxx 设备句柄
 * @param timeout 超时时间 (ms)
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_wait_idle(xy_w25qxx_t *dev, uint32_t timeout);

/**
 * @brief 写入使能
 * @param dev W25Qxx 设备句柄
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_write_enable(xy_w25qxx_t *dev);

/**
 * @brief 扇区擦除 (4KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址 (必须是扇区对齐)
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_sector_erase(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 块擦除 (64KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址 (必须是块对齐)
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_block_erase(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 全片擦除
 * @param dev W25Qxx 设备句柄
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_chip_erase(xy_w25qxx_t *dev);

/**
 * @brief 页编程 (最多 256 字节)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据指针
 * @param len 数据长度 (<=256)
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_page_program(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief 读取数据
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_read_data(xy_w25qxx_t *dev, uint32_t addr, uint8_t *data, uint32_t len);

/**
 * @brief 写入数据 (自动处理擦除和编程)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_write_data(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * @brief 进入掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_power_down(xy_w25qxx_t *dev);

/**
 * @brief 退出掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_W25Q_OK 成功，其他值失败
 */
int xy_w25qxx_release_power_down(xy_w25qxx_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_W25QXX_H */
