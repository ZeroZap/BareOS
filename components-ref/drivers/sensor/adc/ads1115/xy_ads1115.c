/**
 * @file xy_ads1115.c
 * @brief ADS1115 16-bit ADC Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_ads1115.h"
#include <string.h>

/* ADS1115 Registers */
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

/* Config values */
#define ADS1115_CONFIG_OS       (1 << 15)
#define ADS1115_CONFIG_MUX_SINGLE_0  (4 << 12)
#define ADS1115_CONFIG_PGA_6_144      (0 << 9)
#define ADS1115_CONFIG_MODE_SINGLE    (1 << 8)
#define ADS1115_CONFIG_DR_128         (5 << 5)
#define ADS1115_CONFIG_COMP_MODE      (0 << 4)
#define ADS1115_CONFIG_COMP_POLAR     (0 << 3)
#define ADS1115_CONFIG_COMP_LAT       (0 << 2)
#define ADS1115_CONFIG_COMP_QUE       (3 << 0)

int xy_ads1115_init(xy_ads1115_t *ads, void *i2c_handle, float vref)
{
    if (!ads || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(ads, 0, sizeof(*ads));
    xy_i2c_device_init(&ads->i2c_dev, i2c_handle, 0x48, 1000);
    ads->vref = vref ? vref : 4.096f;
    
    /* Write config register */
    uint8_t config[2];
    config[0] = (ADS1115_CONFIG_OS | ADS1115_CONFIG_MUX_SINGLE_0 | 
                 ADS1115_CONFIG_PGA_6_144 | ADS1115_CONFIG_MODE_SINGLE |
                 ADS1115_CONFIG_DR_128 | ADS1115_CONFIG_COMP_QUE) >> 8;
    config[1] = (ADS1115_CONFIG_OS | ADS1115_CONFIG_MUX_SINGLE_0 | 
                 ADS1115_CONFIG_PGA_6_144 | ADS1115_CONFIG_MODE_SINGLE |
                 ADS1115_CONFIG_DR_128 | ADS1115_CONFIG_COMP_QUE) & 0xFF;
    
    return xy_i2c_device_write(&ads->i2c_dev, config, 2);
}

int xy_ads1115_read(xy_ads1115_t *ads)
{
    if (!ads) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    for (uint8_t ch = 0; ch < 4; ch++) {
        ads->adc_value[ch] = xy_ads1115_read_channel(ads, ch);
        ads->voltage[ch] = xy_ads1115_get_voltage(ads, ch);
    }
    
    return XY_DEVICE_OK;
}

int16_t xy_ads1115_read_channel(xy_ads1115_t *ads, uint8_t channel)
{
    if (!ads || channel > 3) {
        return 0;
    }
    
    /* Update config for channel */
    uint8_t config[2];
    uint16_t mux = (4 + channel) << 12;
    uint16_t cfg = ADS1115_CONFIG_OS | mux | ADS1115_CONFIG_PGA_6_144 |
                   ADS1115_CONFIG_MODE_SINGLE | ADS1115_CONFIG_DR_128 |
                   ADS1115_CONFIG_COMP_QUE;
    
    config[0] = cfg >> 8;
    config[1] = cfg & 0xFF;
    
    xy_i2c_device_write(&ads->i2c_dev, config, 2);
    
    /* Wait for conversion (8ms at 128SPS) */
    xy_hal_delay_ms(8);
    
    /* Read conversion register */
    uint8_t data[2];
    xy_i2c_device_read(&ads->i2c_dev, data, 2);
    
    return (int16_t)((data[0] << 8) | data[1]);
}

float xy_ads1115_get_voltage(xy_ads1115_t *ads, uint8_t channel)
{
    int16_t adc = ads->adc_value[channel];
    return (float)adc * ads->vref / 32768.0f;
}
