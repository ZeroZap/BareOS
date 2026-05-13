/**
 * @file stm32u5_platform.h
 * @brief STM32U5 Platform Specific Definitions
 * @version 2.0
 * @date 2026-02-28
 *
 * @note This file contains STM32U5 specific macros and helpers
 * @note It is NOT the STM32U5 HAL library - that comes from MCU/ST/STM32U5/
 */

#ifndef STM32U5_PLATFORM_H
#define STM32U5_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief STM32U5 平台检测宏
 */
#if !defined(STM32U5) && !defined(STM32U5xx)
    #error "STM32U5 platform not defined!"
#endif

/**
 * @brief GPIO 端口定义
 */
#define XY_HAL_GPIO_PORT_A      GPIOA
#define XY_HAL_GPIO_PORT_B      GPIOB
#define XY_HAL_GPIO_PORT_C      GPIOC
#define XY_HAL_GPIO_PORT_D      GPIOD
#define XY_HAL_GPIO_PORT_E      GPIOE
#define XY_HAL_GPIO_PORT_F      GPIOF
#define XY_HAL_GPIO_PORT_G      GPIOG
#define XY_HAL_GPIO_PORT_H      GPIOH
#define XY_HAL_GPIO_PORT_I      GPIOI

/**
 * @brief 获取 GPIO 端口索引
 */
#define XY_HAL_GPIO_PORT_IDX(port)    ((uint32_t)((port) >> 4) & 0x07)

/**
 * @brief 系统滴答定时器
 */
#define xy_hal_sys_tick_get()         HAL_GetTick()

/**
 * @brief 延时函数
 */
#define xy_hal_delay_ms(ms)           HAL_Delay(ms)

/**
 * @brief 进入中断临界区
 */
#define XY_HAL_ENTER_CRITICAL()       __disable_irq()

/**
 * @brief 退出中断临界区
 */
#define XY_HAL_EXIT_CRITICAL()        __enable_irq()

/**
 * @brief 软件复位
 */
#define xy_hal_software_reset()       NVIC_SystemReset()

/**
 * @brief 获取系统时钟频率
 */
#define xy_hal_get_sysclock_freq()    HAL_RCC_GetSysClockFreq()
#define xy_hal_get_hclk_freq()        HAL_RCC_GetHCLKFreq()
#define xy_hal_get_pclk1_freq()       HAL_RCC_GetPCLK1Freq()
#define xy_hal_get_pclk2_freq()       HAL_RCC_GetPCLK2Freq()

/**
 * @brief 使能外设时钟 (示例)
 */
#define XY_HAL_ENABLE_GPIOA_CLOCK()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOB_CLOCK()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOC_CLOCK()   __HAL_RCC_GPIOC_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOD_CLOCK()   __HAL_RCC_GPIOD_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOE_CLOCK()   __HAL_RCC_GPIOE_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOF_CLOCK()   __HAL_RCC_GPIOF_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOG_CLOCK()   __HAL_RCC_GPIOG_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOH_CLOCK()   __HAL_RCC_GPIOH_CLK_ENABLE()
#define XY_HAL_ENABLE_GPIOI_CLOCK()   __HAL_RCC_GPIOI_CLK_ENABLE()

#define XY_HAL_ENABLE_USART1_CLOCK()  __HAL_RCC_USART1_CLK_ENABLE()
#define XY_HAL_ENABLE_USART2_CLOCK()  __HAL_RCC_USART2_CLK_ENABLE()
#define XY_HAL_ENABLE_USART3_CLOCK()  __HAL_RCC_USART3_CLK_ENABLE()

#define XY_HAL_ENABLE_SPI1_CLOCK()    __HAL_RCC_SPI1_CLK_ENABLE()
#define XY_HAL_ENABLE_SPI2_CLOCK()    __HAL_RCC_SPI2_CLK_ENABLE()
#define XY_HAL_ENABLE_SPI3_CLOCK()    __HAL_RCC_SPI3_CLK_ENABLE()

#define XY_HAL_ENABLE_I2C1_CLOCK()    __HAL_RCC_I2C1_CLK_ENABLE()
#define XY_HAL_ENABLE_I2C2_CLOCK()    __HAL_RCC_I2C2_CLK_ENABLE()
#define XY_HAL_ENABLE_I2C3_CLOCK()    __HAL_RCC_I2C3_CLK_ENABLE()

#define XY_HAL_ENABLE_TIM1_CLOCK()    __HAL_RCC_TIM1_CLK_ENABLE()
#define XY_HAL_ENABLE_TIM2_CLOCK()    __HAL_RCC_TIM2_CLK_ENABLE()
#define XY_HAL_ENABLE_TIM3_CLOCK()    __HAL_RCC_TIM3_CLK_ENABLE()
#define XY_HAL_ENABLE_TIM4_CLOCK()    __HAL_RCC_TIM4_CLK_ENABLE()
#define XY_HAL_ENABLE_TIM5_CLOCK()    __HAL_RCC_TIM5_CLK_ENABLE()

#define XY_HAL_ENABLE_ADC1_CLOCK()    __HAL_RCC_ADC1_CLK_ENABLE()
#define XY_HAL_ENABLE_ADC2_CLOCK()    __HAL_RCC_ADC2_CLK_ENABLE()

#define XY_HAL_ENABLE_DMA1_CLOCK()    __HAL_RCC_DMA1_CLK_ENABLE()
#define XY_HAL_ENABLE_DMA2_CLOCK()    __HAL_RCC_DMA2_CLK_ENABLE()

#define XY_HAL_ENABLE_RTC_CLOCK()     __HAL_RCC_RTCAPB_CLK_ENABLE()
#define XY_HAL_ENABLE_FLASH_CLOCK()   __HAL_RCC_FLASH_CLK_ENABLE()

/**
 * @brief 禁用外设时钟 (示例)
 */
#define XY_HAL_DISABLE_GPIOA_CLOCK()  __HAL_RCC_GPIOA_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOB_CLOCK()  __HAL_RCC_GPIOB_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOC_CLOCK()  __HAL_RCC_GPIOC_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOD_CLOCK()  __HAL_RCC_GPIOD_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOE_CLOCK()  __HAL_RCC_GPIOE_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOF_CLOCK()  __HAL_RCC_GPIOF_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOG_CLOCK()  __HAL_RCC_GPIOG_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOH_CLOCK()  __HAL_RCC_GPIOH_CLK_DISABLE()
#define XY_HAL_DISABLE_GPIOI_CLOCK()  __HAL_RCC_GPIOI_CLK_DISABLE()

/**
 * @brief 缓存操作 (STM32U5 支持缓存)
 */
#define XY_HAL_DCACHE_CLEAN()         SCB_CleanDCache()
#define XY_HAL_DCACHE_INVALIDATE()    SCB_InvalidateDCache()
#define XY_HAL_DCACHE_CLEAN_INVALIDATE() SCB_CleanInvalidateDCache()

/**
 * @brief 内存屏障
 */
#define XY_HAL_MEMORY_BARRIER()       __DSB()
#define XY_HAL_INSTRUCTION_BARRIER()  __ISB()

/**
 * @brief 获取唯一设备 ID 基地址
 */
#define XY_HAL_GET_UID_BASE()         (0x0BFA1000UL)

/**
 * @brief 获取 Flash 大小 (KB)
 */
#define XY_HAL_GET_FLASH_SIZE()       ((*((uint16_t *)0x0BFA05F0UL)))

/**
 * @brief 看门狗喂狗
 */
#define XY_HAL_IWDG_RELOAD()          __HAL_IWDG_RELOAD_COUNTER()

/**
 * @brief 清除复位标志
 */
#define XY_HAL_CLEAR_RESET_FLAG()     __HAL_RCC_CLEAR_RESET_FLAGS()

/**
 * @brief 获取复位标志
 */
#define XY_HAL_GET_RESET_FLAG()       (__HAL_RCC_GET_RESET_FLAG())

#ifdef __cplusplus
}
#endif

#endif /* STM32U5_PLATFORM_H */
