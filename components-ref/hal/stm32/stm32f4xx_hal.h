/**
 * @file stm32f4xx_hal.h
 * @brief STM32F4 HAL Placeholder (for PC build)
 */

#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* Placeholder definitions for PC build */
typedef struct {
    uint32_t Instance;
} GPIO_TypeDef;

typedef struct {
    uint32_t Instance;
} USART_TypeDef;

typedef struct {
    uint32_t Instance;
} SPI_TypeDef;

typedef struct {
    uint32_t Instance;
} I2C_TypeDef;

/* HAL Status */
typedef enum {
    HAL_OK = 0x00,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

#endif /* STM32F4XX_HAL_H */
