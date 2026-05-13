/**
 * @file xy_eeprom_24xx.h
 * @brief 24xx Series EEPROM Device Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_EEPROM_24XX_H
#define XY_EEPROM_24XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief 24xx EEPROM 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    uint16_t page_size;
    uint16_t total_size;
    uint8_t address_bits;
} xy_eeprom_24xx_t;

/**
 * @brief 初始化 EEPROM
 * @param eeprom EEPROM 设备
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @param page_size 页大小
 * @param total_size 总容量
 * @return XY_DEVICE_OK 成功
 */
int xy_eeprom_24xx_init(xy_eeprom_24xx_t *eeprom, void *i2c_handle, 
                        uint16_t addr, uint16_t page_size, uint16_t total_size);

/**
 * @brief 读取 EEPROM
 * @param eeprom EEPROM 设备
 * @param addr 读取地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return 实际读取字节数
 */
int xy_eeprom_24xx_read(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                        uint8_t *data, size_t len);

/**
 * @brief 写入 EEPROM
 * @param eeprom EEPROM 设备
 * @param addr 写入地址
 * @param data 数据缓冲区
 * @param len 写入长度
 * @return 实际写入字节数
 */
int xy_eeprom_24xx_write(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                         const uint8_t *data, size_t len);

/**
 * @brief 页写入 EEPROM
 * @param eeprom EEPROM 设备
 * @param addr 写入地址
 * @param data 数据缓冲区
 * @param len 写入长度
 * @return 实际写入字节数
 */
int xy_eeprom_24xx_write_page(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                              const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_EEPROM_24XX_H */
