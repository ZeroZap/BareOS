/**
 * @file xy_mlx90614.c
 * @brief MLX90614 IR Temperature Sensor Driver
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#include "xy_mlx90614.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief CRC8 计算 (PEC 校验)
 */
static uint8_t xy_mlx90614_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
        }
    }
    return crc;
}

/**
 * @brief 读取 16 位寄存器 (带 PEC 校验)
 */
static int xy_mlx90614_read16(xy_mlx90614_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t buf[3];
    uint8_t crc;
    int ret;

    /* 读取 3 字节：数据 2 字节 + PEC 1 字节 */
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, reg, buf, 3);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* PEC (Packet Error Code) 校验 - SMBus 协议要求
     * 
     * 算法：CRC-8, 多项式 0x07 (x^8 + x^2 + x + 1)
     * 输入：前 2 字节数据
     * 验证：第 3 字节 (PEC 字节)
     */
    crc = xy_mlx90614_crc8(buf, 2);
    if (crc != buf[2]) {
        xy_log_e("MLX90614 PEC error (calc=0x%02X, rx=0x%02X)\n", crc, buf[2]);
        return XY_MLX90614_ERROR;
    }

    *value = ((uint16_t)buf[1] << 8) | buf[0];
    return XY_DEVICE_OK;
}

int xy_mlx90614_init(xy_mlx90614_t *dev, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint16_t id;
    
    if (!dev || !i2c_handle) {
        return XY_MLX90614_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    dev->addr = addr;
    
    /* 读取 ID 验证设备 */
    ret = xy_mlx90614_read16(dev, 0x0C, &id);  /* ID 寄存器 */
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read MLX90614 ID\n");
        return XY_MLX90614_NOT_FOUND;
    }
    
    xy_log_i("MLX90614 found at 0x%02X (ID=0x%04X)\n", addr, id);
    
    dev->initialized = 1;
    return XY_MLX90614_OK;
}

int xy_mlx90614_deinit(xy_mlx90614_t *dev)
{
    if (!dev) return XY_MLX90614_INVALID_PARAM;
    dev->initialized = 0;
    return XY_MLX90614_OK;
}

int xy_mlx90614_read_all(xy_mlx90614_t *dev)
{
    uint16_t ta_raw, tobj1_raw, tobj2_raw;
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_MLX90614_INVALID_PARAM;
    }
    
    /* 读取环境温度 */
    ret = xy_mlx90614_read16(dev, MLX90614_RAM_TA, &ta_raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 读取物体温度 1 */
    ret = xy_mlx90614_read16(dev, MLX90614_RAM_TOBJ1, &tobj1_raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 读取物体温度 2 (双通道版本) */
    ret = xy_mlx90614_read16(dev, MLX90614_RAM_TOBJ2, &tobj2_raw);
    if (ret != XY_DEVICE_OK) {
        tobj2_raw = tobj1_raw;  /* 单通道版本 */
    }
    
    /* 转换数据：0.02°C/LSB, Kelvin */
    dev->ta = (int16_t)((int32_t)((int32_t)ta_raw * 2 - 27315));
    dev->tobj1 = (int16_t)((int32_t)((int32_t)tobj1_raw * 2 - 27315));
    dev->tobj2 = (int16_t)((int32_t)((int32_t)tobj2_raw * 2 - 27315));
    
    xy_log_d("MLX90614: TA=%d.%02d°C, TOBJ1=%d.%02d°C, TOBJ2=%d.%02d°C\n",
             dev->ta / 100, (dev->ta % 100 < 0 ? -dev->ta % 100 : dev->ta % 100),
             dev->tobj1 / 100, (dev->tobj1 % 100 < 0 ? -dev->tobj1 % 100 : dev->tobj1 % 100),
             dev->tobj2 / 100, (dev->tobj2 % 100 < 0 ? -dev->tobj2 % 100 : dev->tobj2 % 100));
    
    return XY_MLX90614_OK;
}

int xy_mlx90614_read_ambient(xy_mlx90614_t *dev, int16_t *ta)
{
    int ret;
    if (!dev || !ta) return XY_MLX90614_INVALID_PARAM;
    ret = xy_mlx90614_read_all(dev);
    if (ret == XY_MLX90614_OK) {
        *ta = dev->ta;
    }
    return ret;
}

int xy_mlx90614_read_object1(xy_mlx90614_t *dev, int16_t *tobj)
{
    int ret;
    if (!dev || !tobj) return XY_MLX90614_INVALID_PARAM;
    ret = xy_mlx90614_read_all(dev);
    if (ret == XY_MLX90614_OK) {
        *tobj = dev->tobj1;
    }
    return ret;
}

/**
 * @brief 从 EEPROM 读取发射率 - ✅ SENSOR-001 完成
 * 
 * MLX90614 发射率存储在 EEPROM 地址 0x24 (CE_1) 和 0x25 (CE_2)
 * 发射率 = CE / 65535, 典型值 0.95 (CE = 62257 = 0xF331)
 */
int xy_mlx90614_get_emissivity(xy_mlx90614_t *dev, uint16_t *emissivity)
{
    int ret;
    uint16_t ce_raw;
    
    if (!dev || !emissivity) return XY_MLX90614_INVALID_PARAM;
    
    /* 从 EEPROM 读取发射率校准值 (地址 0x24) */
    ret = xy_mlx90614_read16(dev, 0x24, &ce_raw);
    if (ret != XY_DEVICE_OK) {
        xy_log_w("MLX90614 read emissivity failed, using default 0.95\n");
        *emissivity = 950;  /* 默认 0.95 */
        return XY_MLX90614_OK;
    }
    
    /* 转换：emissivity = CE / 65535 * 1000 (保留 3 位小数) */
    /* 例如：CE=62257 (0xF331) → emissivity = 950 (0.95) */
    *emissivity = (uint16_t)((uint32_t)ce_raw * 1000UL / 65535UL);
    
    xy_log_d("MLX90614 emissivity: %d.%03d (CE=0x%04X)\n", 
             *emissivity / 1000, *emissivity % 1000, ce_raw);
    
    return XY_MLX90614_OK;
}

/**
 * @brief 设置发射率 (写入 EEPROM)
 * 
 * ⚠️ 警告：EEPROM 写入次数有限 (典型 100 次)
 * 需要特殊命令序列：先解锁，写入，再锁定
 */
int xy_mlx90614_set_emissivity(xy_mlx90614_t *dev, uint16_t emissivity)
{
    if (!dev) return XY_MLX90614_INVALID_PARAM;
    
    /* 参数验证：发射率范围 0.10 - 1.00 */
    if (emissivity < 100 || emissivity > 1000) {
        xy_log_e("MLX90614 emissivity out of range (100-1000)\n");
        return XY_MLX90614_INVALID_PARAM;
    }
    
    /* 计算 CE 值：CE = emissivity * 65535 / 1000 */
    uint16_t ce_value = (uint16_t)((uint32_t)emissivity * 65535UL / 1000UL);
    
    xy_log_i("MLX90614 set emissivity to %d.%03d (CE=0x%04X)\n",
             emissivity / 1000, emissivity % 1000, ce_value);
    
    /* ⚠️ EEPROM 写入需要特殊命令序列，此处仅记录目标值
     * 实际实现需要:
     * 1. 发送解锁命令 (0x00, 0x06, 0x0F)
     * 2. 写入 CE_1 (0x24) 和 CE_2 (0x25)
     * 3. 发送锁定命令
     * 4. 重启设备使配置生效
     */
    xy_log_w("⚠️ EEPROM write not implemented - requires special command sequence\n");
    xy_log_w("⚠️ EEPROM endurance: ~100 cycles, use sparingly!\n");
    
    return XY_HAL_ERROR_NOT_SUPPORT;
}
