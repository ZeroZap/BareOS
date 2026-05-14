/**
 * @file xy_nor_port.c
 * @brief NOR Flash HAL port — weak stubs for BSP override.
 *
 * Override these functions in the BSP to drive the MCU SPI/QSPI peripheral.
 * Typical BSP pattern:
 *
 *   void *xy_nor_hw_init(const xy_nor_config_t *config) {
 *       bsp_spi_init(config->clock_freq, config->quad_enable);
 *       return (void *)&g_spi_handle;   // or NULL for no handle
 *   }
 *
 *   xy_nor_status_t xy_nor_hw_command(void *hw, uint8_t cmd,
 *                                     uint32_t addr, uint8_t addr_len,
 *                                     uint8_t *data, uint32_t len, bool wr) {
 *       bsp_spi_cs_low();
 *       bsp_spi_tx(&cmd, 1);
 *       if (addr_len) { uint8_t ab[4]; pack_be(ab, addr, addr_len);
 *                       bsp_spi_tx(ab, addr_len); }
 *       if (len) { wr ? bsp_spi_tx(data, len) : bsp_spi_rx(data, len); }
 *       bsp_spi_cs_high();
 *       return XY_NOR_OK;
 *   }
 */

#include "xy_nor.h"

__attribute__((weak))
void *xy_nor_hw_init(const xy_nor_config_t *config)
{
    (void)config;
    return NULL;
}

__attribute__((weak))
void xy_nor_hw_deinit(void *hw_handle)
{
    (void)hw_handle;
}

__attribute__((weak))
xy_nor_status_t xy_nor_hw_command(void *hw_handle, uint8_t cmd,
                                  uint32_t addr, uint8_t addr_len,
                                  uint8_t *data, uint32_t data_len,
                                  bool is_write)
{
    (void)hw_handle; (void)cmd; (void)addr; (void)addr_len;
    (void)data; (void)data_len; (void)is_write;
    return XY_NOR_ERROR;
}

__attribute__((weak))
void xy_nor_hw_delay_ms(uint32_t ms)
{
    /* BSP override: call bsp_delay_ms() or spin on g_sys_tick_ms */
    (void)ms;
}
