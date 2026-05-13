/**
 * @file xy_hal_i2c_pc.c
 * @brief PC/Linux simulation layer for I2C HAL
 */

#include "xy_hal_i2c.h"
#include <stdio.h>
#include <string.h>

xy_hal_error_t xy_hal_i2c_master_transmit(void *hi2c, uint16_t dev_addr, 
                                           const uint8_t *data, size_t len, 
                                           uint32_t timeout)
{
    (void)hi2c; (void)dev_addr; (void)data; (void)len; (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *hi2c, uint16_t dev_addr, 
                                          uint8_t *data, size_t len, 
                                          uint32_t timeout)
{
    (void)hi2c; (void)dev_addr; (void)timeout;
    if (data && len > 0) memset(data, 0, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *hi2c, uint16_t dev_addr, 
                                     uint16_t mem_addr, const uint8_t *data, 
                                     size_t len, uint32_t timeout)
{
    (void)hi2c; (void)dev_addr; (void)mem_addr; (void)data; (void)len; (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *hi2c, uint16_t dev_addr, 
                                    uint16_t mem_addr, uint8_t *data, 
                                    size_t len, uint32_t timeout)
{
    (void)hi2c; (void)dev_addr; (void)mem_addr; (void)timeout;
    if (data && len > 0) memset(data, 0, len);
    return XY_HAL_OK;
}
