/**
 * @file xy_mlxeeprom.h
 * @brief MLX90614 EEPROM Access Functions
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_MLXEERPROM_H
#define XY_MLXEERPROM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MLX90614 EEPROM 地址
 */
#define MLX90614_EE_ADDR emissivity   0x20
#define MLX90614_EE_ADDR_Ia           0x22
#define MLX90614_EE_ADDR_Ib           0x23
#define MLX90614_EE_ADDR_Ka           0x24
#define MLX90614_EE_ADDR_Kb           0x25
#define MLX90614_EE_ADDR_Ta           0x26
#define MLX90614_EE_ADDR_Tb           0x27
#define MLX90614_EE_ADDR_Kc           0x28
#define MLX90614_EE_ADDR_Kd           0x29
#define MLX90614_EE_ADDR_Config       0x2A
#define MLX90614_EE_ADDR_Tadj         0x2B
#define MLX90614_EE_ADDR_Resolution   0x2C

/**
 * @brief 读取 EEPROM 数据
 * @param i2c_handle I2C 句柄
 * @param addr EEPROM 地址
 * @param value 读取值指针
 * @return 0 成功，-1 失败
 */
int xy_mlx90614_eeprom_read(void *i2c_handle, uint8_t addr, uint16_t *value);

/**
 * @brief 写入 EEPROM 数据
 * @param i2c_handle I2C 句柄
 * @param addr EEPROM 地址
 * @param value 写入值
 * @return 0 成功，-1 失败
 */
int xy_mlx90614_eeprom_write(void *i2c_handle, uint8_t addr, uint16_t value);

/**
 * @brief 读取发射率
 * @param i2c_handle I2C 句柄
 * @param emissivity 发射率指针 (0.001 单位)
 * @return 0 成功，-1 失败
 */
int xy_mlx90614_get_emissivity(void *i2c_handle, uint16_t *emissivity);

/**
 * @brief 设置发射率
 * @param i2c_handle I2C 句柄
 * @param emissivity 发射率 (0.001 单位)
 * @return 0 成功，-1 失败
 */
int xy_mlx90614_set_emissivity(void *i2c_handle, uint16_t emissivity);

#ifdef __cplusplus
}
#endif

#endif
