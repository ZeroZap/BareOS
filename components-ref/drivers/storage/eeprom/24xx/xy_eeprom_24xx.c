/**
 * @file xy_eeprom_24xx.c
 * @brief 24xx Series EEPROM Device Driver Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_eeprom_24xx.h"
#include <string.h>

#ifdef HAL_PLATFORM_PC
#include "../hal/PC/xy_hal_pc.h"
#endif

int xy_eeprom_24xx_init(xy_eeprom_24xx_t *eeprom, void *i2c_handle, 
                        uint16_t addr, uint16_t page_size, uint16_t total_size)
{
    if (!eeprom || !i2c_handle || page_size == 0 || total_size == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(eeprom, 0, sizeof(*eeprom));
    
    xy_i2c_device_init(&eeprom->i2c_dev, i2c_handle, addr, 1000);
    eeprom->page_size = page_size;
    eeprom->total_size = total_size;
    eeprom->address_bits = (total_size > 256) ? 16 : 8;

    return XY_DEVICE_OK;
}

int xy_eeprom_24xx_read(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                        uint8_t *data, size_t len)
{
    if (!eeprom || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }

    if (addr + len > eeprom->total_size) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Send address */
    uint8_t addr_buf[2];
    if (eeprom->address_bits == 16) {
        addr_buf[0] = (addr >> 8) & 0xFF;
        addr_buf[1] = addr & 0xFF;
        xy_i2c_device_write(&eeprom->i2c_dev, addr_buf, 2);
    } else {
        addr_buf[0] = addr & 0xFF;
        xy_i2c_device_write(&eeprom->i2c_dev, addr_buf, 1);
    }

    /* Read data */
    return xy_i2c_device_read(&eeprom->i2c_dev, data, len);
}

int xy_eeprom_24xx_write_page(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                              const uint8_t *data, size_t len)
{
    if (!eeprom || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* Calculate page boundary */
    uint16_t page_start = addr & ~(eeprom->page_size - 1);
    uint16_t page_end = page_start + eeprom->page_size;
    
    /* Limit to page boundary */
    if (addr + len > page_end) {
        len = page_end - addr;
    }

    /* Send address */
    uint8_t buffer[128];
    if (eeprom->address_bits == 16) {
        buffer[0] = (addr >> 8) & 0xFF;
        buffer[1] = addr & 0xFF;
        memcpy(&buffer[2], data, len);
        xy_i2c_device_write(&eeprom->i2c_dev, buffer, len + 2);
    } else {
        buffer[0] = addr & 0xFF;
        memcpy(&buffer[1], data, len);
        xy_i2c_device_write(&eeprom->i2c_dev, buffer, len + 1);
    }

    /* Wait for write complete (max 5ms) */
    xy_hal_delay_ms(5);

    return (int)len;
}

int xy_eeprom_24xx_write(xy_eeprom_24xx_t *eeprom, uint16_t addr, 
                         const uint8_t *data, size_t len)
{
    if (!eeprom || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }

    int total_written = 0;

    while (len > 0) {
        int written = xy_eeprom_24xx_write_page(eeprom, addr, data, len);
        if (written < 0) {
            return written;
        }

        total_written += written;
        addr += written;
        data += written;
        len -= written;
    }

    return total_written;
}
