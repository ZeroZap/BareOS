/**
 * @file xy_hal_adc.c
 * @brief WCH CH32V30x ADC HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_hal_adc.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief ADC 时钟使能
 */
static void xy_hal_adc_enable_clock(void *instance)
{
    if (instance == ADC1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    } else if (instance == ADC2) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
    } else if (instance == ADC3) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
    }
}

xy_hal_error_t xy_hal_adc_init(void *instance, const xy_hal_adc_config_t *config)
{
    ADC_InitTypeDef ADC_InitStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_adc_enable_clock(instance);
    
    /* 配置 ADC 参数 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = config->continuous ? ENABLE : DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    
    ADC_Init(instance, &ADC_InitStructure);
    
    /* 配置 ADC 时钟 (PCLK2/8) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    
    /* 使能 ADC */
    ADC_Cmd(instance, ENABLE);
    
    /* ADC 校准 */
    ADC_ResetCalibration(instance);
    while (ADC_GetResetCalibrationStatus(instance));
    ADC_StartCalibration(instance);
    while (ADC_GetCalibrationStatus(instance));
    
    xy_log_d("WCH ADC init: instance=%p, continuous=%d\n",
             instance, config->continuous);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_deinit(void *instance)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ADC_Cmd(instance, DISABLE);
    ADC_DeInit(instance);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_read(void *instance, uint8_t channel, 
                               uint16_t *value, uint32_t timeout)
{
    uint32_t start;
    
    if (!instance || !value || channel > 17) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 配置通道 */
    ADC_RegularChannelConfig(instance, channel, 1, ADC_SampleTime_55Cycles5);
    
    /* 启动转换 */
    ADC_SoftwareStartConvCmd(instance, ENABLE);
    
    /* 等待转换完成 */
    start = xy_os_tick_get();
    while (ADC_GetFlagStatus(instance, ADC_FLAG_EOC) == RESET) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 读取结果 */
    *value = ADC_GetConversionValue(instance);
    
    /* 清除标志 */
    ADC_ClearFlag(instance, ADC_FLAG_EOC);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_read_dma(void *instance, uint8_t channel,
                                   uint16_t *buffer, uint16_t count,
                                   uint32_t timeout)
{
    /* DMA 读取实现 */
    /* 需要配置 DMA 通道和中断 */
    (void)instance;
    (void)channel;
    (void)buffer;
    (void)count;
    (void)timeout;
    
    /* 简化实现：使用普通读取 */
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#else

xy_hal_error_t xy_hal_adc_init(void *instance, const xy_hal_adc_config_t *config)
{
    (void)instance;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
