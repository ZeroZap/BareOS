/**
 * @file xy_bmp280.c
 * @brief BMP280 Barometric Pressure Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_bmp280.h"
#include <string.h>

/* BMP280 Registers */
#define BMP280_REG_ID           0xD0
#define BMP280_ID_VALUE         0x58
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_TEMP_DATA    0xFA
#define BMP280_REG_PRESS_DATA   0xF7
#define BMP280_REG_CALIB        0x88

int xy_bmp280_init(xy_bmp280_t *bmp, void *i2c_handle)
{
    if (!bmp || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(bmp, 0, sizeof(*bmp));
    xy_i2c_device_init(&bmp->i2c_dev, i2c_handle, 0x76, 1000);
    
    /* Check ID */
    uint8_t id;
    xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_ID, &id, 1);
    
    if (id != BMP280_ID_VALUE) {
        return XY_DEVICE_NOT_FOUND;
    }
    
    /* Read calibration data */
    uint8_t calib_data[24];
    xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_CALIB, calib_data, 24);
    
    /* Parse calibration (simplified) */
    for (int i = 0; i < 8; i++) {
        bmp->calibration[i] = (calib_data[i*2+1] << 8) | calib_data[i*2];
    }
    
    /* Configure sensor */
    uint8_t ctrl = 0x3F; /* oversampling x16 */
    uint8_t config = 0x00;
    
    xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_CTRL_MEAS, &ctrl, 1);
    xy_i2c_device_write_reg(&bmp->i2c_dev, BMP280_REG_CONFIG, &config, 1);
    
    return XY_DEVICE_OK;
}

int xy_bmp280_read(xy_bmp280_t *bmp)
{
    if (!bmp) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t buffer[6];
    int result = xy_i2c_device_read_reg(&bmp->i2c_dev, BMP280_REG_TEMP_DATA, buffer, 6);
    
    if (result < 0) {
        return result;
    }
    
    /* Parse raw data */
    uint32_t temp_raw = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
    uint32_t press_raw = ((buffer[3] << 12) | (buffer[4] << 4) | (buffer[5] >> 4));
    
    /* Simple conversion (real implementation needs compensation) */
    bmp->temperature = (int32_t)((temp_raw / 100) - 4000); /* Simplified */
    bmp->pressure = press_raw / 256; /* Simplified */
    
    return XY_DEVICE_OK;
}

int32_t xy_bmp280_read_temperature(xy_bmp280_t *bmp)
{
    xy_bmp280_read(bmp);
    return bmp->temperature;
}

uint32_t xy_bmp280_read_pressure(xy_bmp280_t *bmp)
{
    xy_bmp280_read(bmp);
    return bmp->pressure;
}
