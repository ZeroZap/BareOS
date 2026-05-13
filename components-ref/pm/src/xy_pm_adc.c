/**
 * @file xy_pm_adc.c
 * @brief PM ADC Implementation - STM32/WCH Platform
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_pm.h"
#include "xy_log.h"
#include <stdint.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* ADC 配置 */
#ifndef PM_ADC_VREF_MV
#define PM_ADC_VREF_MV  3300  /* 参考电压 3.3V */
#endif

#ifndef PM_ADC_RESOLUTION
#define PM_ADC_RESOLUTION  4095  /* 12-bit ADC */
#endif

#ifndef PM_BATTERY_DIVIDER_RATIO
#define PM_BATTERY_DIVIDER_RATIO  2  /* 分压比 2:1 */
#endif

/* 平台特定 ADC 读取 */
#if defined(STM32U5) || defined(STM32F4) || defined(STM32F1)
/* STM32 平台 */
#include "stm32_hal.h"

static ADC_HandleTypeDef *s_adc_handle = NULL;

int xy_pm_adc_init(void)
{
    xy_log_i("PM ADC: Initializing STM32 ADC...\n");
    
    /* STM32 ADC 初始化由 HAL 完成 */
    /* 这里只保存句柄 */
    extern ADC_HandleTypeDef hadc1;
    s_adc_handle = &hadc1;
    
    xy_log_i("PM ADC: STM32 ADC initialized\n");
    return XY_PM_OK;
}

uint32_t xy_pm_adc_read_battery_voltage(void)
{
    if (!s_adc_handle) {
        /* 返回默认值 */
        return 3700;
    }
    
    uint32_t adc_value = 0;
    
    /* 启动 ADC 转换 */
    HAL_ADC_Start(s_adc_handle);
    HAL_ADC_PollForConversion(s_adc_handle, 10);
    
    /* 读取 ADC 值 */
    adc_value = HAL_ADC_GetValue(s_adc_handle);
    HAL_ADC_Stop(s_adc_handle);
    
    /* 转换为电压 (mV) */
    uint32_t voltage = (adc_value * PM_ADC_VREF_MV) / PM_ADC_RESOLUTION;
    
    /* 应用分压比 */
    voltage *= PM_BATTERY_DIVIDER_RATIO;
    
    xy_log_d("PM ADC: Raw=%lu Voltage=%lumV\n", adc_value, voltage);
    return voltage;
}

#elif defined(MCU_CH32)
/* WCH CH32 平台 */
#include "ch32_hal.h"

static int s_adc_initialized = 0;

int xy_pm_adc_init(void)
{
    xy_log_i("PM ADC: Initializing CH32 ADC...\n");
    
    /* CH32 ADC 初始化 */
    /* 配置 ADC1 为电池电压检测 */
    /* 具体实现依赖 CH32 HAL */
    
    s_adc_initialized = 1;
    xy_log_i("PM ADC: CH32 ADC initialized\n");
    return XY_PM_OK;
}

uint32_t xy_pm_adc_read_battery_voltage(void)
{
    if (!s_adc_initialized) {
        return 3700;
    }
    
    uint32_t adc_value = 0;
    
    /* CH32 ADC 读取 */
    /* 具体实现依赖 CH32 HAL */
    // adc_value = ADC_RegularChannelConversion(ADC1, ADC_Channel_X);
    
    /* 转换为电压 */
    uint32_t voltage = (adc_value * PM_ADC_VREF_MV) / PM_ADC_RESOLUTION;
    voltage *= PM_BATTERY_DIVIDER_RATIO;
    
    xy_log_d("PM ADC: CH32 Voltage=%lumV\n", voltage);
    return voltage;
}

#else
/* 通用平台 (PC/其他) */

static int s_adc_initialized = 0;

int xy_pm_adc_init(void)
{
    xy_log_i("PM ADC: Generic platform - using simulated values\n");
    s_adc_initialized = 1;
    return XY_PM_OK;
}

uint32_t xy_pm_adc_read_battery_voltage(void)
{
    /* 返回模拟值 3.7V */
    return 3700;
}

#endif

/* 通用电池电量估算 */
uint8_t xy_pm_estimate_soc_from_voltage(uint32_t voltage_mV)
{
    /* 锂电池典型放电曲线 */
    if (voltage_mV >= 4200) return 100;
    if (voltage_mV >= 4100) return 90;
    if (voltage_mV >= 4000) return 80;
    if (voltage_mV >= 3900) return 70;
    if (voltage_mV >= 3800) return 60;
    if (voltage_mV >= 3750) return 50;
    if (voltage_mV >= 3700) return 40;
    if (voltage_mV >= 3650) return 30;
    if (voltage_mV >= 3600) return 20;
    if (voltage_mV >= 3500) return 10;
    return 0;
}

/* 平台无关的电池电压读取接口 */
uint32_t xy_pm_get_battery_voltage_mV(void)
{
    return xy_pm_adc_read_battery_voltage();
}

/* 平台无关的电量百分比接口 */
uint8_t xy_pm_get_battery_percent(void)
{
    uint32_t voltage = xy_pm_get_battery_voltage_mV();
    return xy_pm_estimate_soc_from_voltage(voltage);
}
