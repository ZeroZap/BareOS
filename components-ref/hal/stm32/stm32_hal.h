/**
 * @file stm32_hal.h
 * @brief STM32 HAL Helper Header
 * @version 2.0
 * @date 2026-02-28
 *
 * This file provides a unified include for STM32 HAL libraries.
 * Define STM32_HAL_ENABLED and the appropriate STM32 family macro
 * (e.g., STM32F1, STM32F4, STM32U5, etc.) in your build system.
 */

#ifndef STM32_HAL_H
#define STM32_HAL_H

#ifdef STM32_HAL_ENABLED

/* Auto-detect STM32 family based on common defines */
#if defined(STM32F0)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1)
#include "stm32f1xx_hal.h"
#elif defined(STM32F2)
#include "stm32f2xx_hal.h"
#elif defined(STM32F3)
#include "stm32f3xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7)
#include "stm32f7xx_hal.h"
#elif defined(STM32G0)
#include "stm32g0xx_hal.h"
#elif defined(STM32G4)
#include "stm32g4xx_hal.h"
#elif defined(STM32H7)
#include "stm32h7xx_hal.h"
#elif defined(STM32L0)
#include "stm32l0xx_hal.h"
#elif defined(STM32L1)
#include "stm32l1xx_hal.h"
#elif defined(STM32L4)
#include "stm32l4xx_hal.h"
#elif defined(STM32L5)
#include "stm32l5xx_hal.h"
#elif defined(STM32U5)
#include "stm32u5xx_hal.h"
#elif defined(STM32WB)
#include "stm32wbxx_hal.h"
#elif defined(STM32WL)
#include "stm32wlxx_hal.h"
#else
#warning "STM32 family not defined. Please define STM32Fxx or STM32Lxx macro."
#endif

#endif /* STM32_HAL_ENABLED */

#endif /* STM32_HAL_H */
