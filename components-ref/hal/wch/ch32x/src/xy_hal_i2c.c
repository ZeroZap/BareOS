/**
 * @file xy_hal_i2c.c
 * @brief WCH CH32V30x I2C HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_hal_i2c.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief I2C 时钟使能
 */
static void xy_hal_i2c_enable_clock(void *instance)
{
    if (instance == I2C1) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (instance == I2C2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
}

xy_hal_error_t xy_hal_i2c_init(void *instance, const xy_hal_i2c_config_t *config)
{
    I2C_InitTypeDef I2C_InitStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_i2c_enable_clock(instance);
    
    /* 配置 I2C 参数 */
    I2C_InitStructure.I2C_ClockSpeed = config->speed;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = config->own_address;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    
    I2C_Init(instance, &I2C_InitStructure);
    I2C_Cmd(instance, ENABLE);
    
    xy_log_d("WCH I2C init: instance=%p, speed=%d\n", instance, config->speed);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void *instance)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    I2C_Cmd(instance, DISABLE);
    I2C_DeInit(instance);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *instance, uint16_t addr, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    uint16_t i;
    uint32_t start;
    
    if (!instance || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 发送 START 条件 */
    I2C_GenerateSTART(instance, ENABLE);
    
    start = xy_os_tick_get();
    while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 发送地址 */
    I2C_Send7bitAddress(instance, addr, I2C_Direction_Transmitter);
    
    start = xy_os_tick_get();
    while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 发送数据 */
    for (i = 0; i < len; i++) {
        I2C_SendData(instance, data[i]);
        
        start = xy_os_tick_get();
        while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
    }
    
    /* 发送 STOP 条件 */
    I2C_GenerateSTOP(instance, ENABLE);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *instance, uint16_t addr, uint8_t *data, uint16_t len, uint32_t timeout)
{
    uint16_t i;
    uint32_t start;
    
    if (!instance || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 发送 START 条件 */
    I2C_GenerateSTART(instance, ENABLE);
    
    start = xy_os_tick_get();
    while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 发送地址 (读) */
    I2C_Send7bitAddress(instance, addr, I2C_Direction_Receiver);
    
    start = xy_os_tick_get();
    while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 接收数据 */
    for (i = 0; i < len; i++) {
        if (i == len - 1) {
            /* 最后一个字节 */
            I2C_AcknowledgeConfig(instance, DISABLE);
            I2C_GenerateSTOP(instance, ENABLE);
        }
        
        start = xy_os_tick_get();
        while (!I2C_CheckEvent(instance, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
        
        data[i] = I2C_ReceiveData(instance);
    }
    
    /* 恢复 ACK */
    I2C_AcknowledgeConfig(instance, ENABLE);
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_i2c_init(void *instance, const xy_hal_i2c_config_t *config)
{
    (void)instance;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
