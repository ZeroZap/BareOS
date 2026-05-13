/**
 * @file xy_hal_i2c_stm32.c
 * @brief I2C HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_i2c.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

static uint32_t xy_to_stm32_addr_mode(xy_hal_i2c_addr_mode_t mode)
{
    return (mode == XY_HAL_I2C_ADDR_7BIT) ? XY_HAL_I2C_ADDRESSINGMODE_7BIT
                                          : XY_HAL_I2C_ADDRESSINGMODE_10BIT;
}

static uint32_t xy_to_stm32_duty(xy_hal_i2c_duty_t duty)
{
#ifdef I2C_DUTYCYCLE_2
    return (duty == XY_HAL_I2C_DUTY_2) ? I2C_DUTYCYCLE_2 : I2C_DUTYCYCLE_16_9;
#else
    return 0;
#endif
}

int xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return -1;
    }

    xy_hal_i2c_handle_t *hi2c = (xy_hal_i2c_handle_t *)i2c;

    hi2c->Init.ClockSpeed     = config->clock_speed;
    hi2c->Init.AddressingMode = xy_to_stm32_addr_mode(config->addr_mode);
#ifdef I2C_DUTYCYCLE_2
    hi2c->Init.DutyCycle = xy_to_stm32_duty(config->duty_cycle);
#endif
    hi2c->Init.OwnAddress1     = config->own_address;
    hi2c->Init.GeneralCallMode = config->general_call_mode
                                     ? I2C_GENERALCALL_ENABLE
                                     : I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(hi2c) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_deinit(void *i2c)
{
    if (!i2c) {
        return -1;
    }

    if (HAL_I2C_DeInit((xy_hal_i2c_handle_t *)i2c) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                               const uint8_t *data, size_t len,
                               uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Master_Transmit((xy_hal_i2c_handle_t *)i2c, dev_addr << 1,
                                (uint8_t *)data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                              size_t len, uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Master_Receive(
            (xy_hal_i2c_handle_t *)i2c, dev_addr << 1, data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                         const uint8_t *data, size_t len, uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Mem_Write((xy_hal_i2c_handle_t *)i2c, dev_addr << 1, reg_addr,
                          I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                        uint8_t *data, size_t len, uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Mem_Read((xy_hal_i2c_handle_t *)i2c, dev_addr << 1, reg_addr,
                         I2C_MEMADD_SIZE_8BIT, data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_master_transmit_dma(void *i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t len)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Master_Transmit_DMA(
            (xy_hal_i2c_handle_t *)i2c, dev_addr << 1, (uint8_t *)data, len)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_master_receive_dma(void *i2c, uint16_t dev_addr, uint8_t *data,
                                  size_t len)
{
    if (!i2c || !data || len == 0) {
        return -1;
    }

    if (HAL_I2C_Master_Receive_DMA(
            (xy_hal_i2c_handle_t *)i2c, dev_addr << 1, data, len)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_i2c_register_callback(void *i2c, xy_hal_i2c_callback_t callback,
                                 void *arg)
{
    /* Store callback in context */
    return 0;
}

int xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr, uint32_t trials,
                               uint32_t timeout)
{
    if (!i2c) {
        return -1;
    }

    if (HAL_I2C_IsDeviceReady(
            (xy_hal_i2c_handle_t *)i2c, dev_addr << 1, trials, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

#endif /* STM32_HAL_ENABLED */
