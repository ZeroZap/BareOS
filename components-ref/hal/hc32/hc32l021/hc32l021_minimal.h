/**
 * @file  hc32l021_minimal.h
 * @brief Minimal HC32L021 Header for XinYi HAL
 * @note  完全剥离 board 依赖 - 只保留核心寄存器定义
 */

#ifndef __HC32L021_MINIMAL_H__
#define __HC32L021_MINIMAL_H__

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Include files - Minimal set, no board dependencies
 ******************************************************************************/
#include "base_types.h"

/*******************************************************************************
 * Global type definitions
 ******************************************************************************/

/**
 * @brief GPIO Port Type Definition
 */
typedef struct {
    volatile uint32_t PDIR;   /* 0x00 Port Data Input Register */
    volatile uint32_t PDOR;   /* 0x04 Port Data Output Register */
    volatile uint32_t DDR;    /* 0x08 Port Data Direction Register */
    volatile uint32_t DR;     /* 0x0C Port Driver Register */
    volatile uint32_t PUR;    /* 0x10 Pull-up Register */
    volatile uint32_t PDR;    /* 0x14 Pull-down Register */
    volatile uint32_t ODTR;   /* 0x18 Open Drain Control Register */
    volatile uint32_t ADE;    /* 0x1C Analog/Digital Enable Register */
} M0P_GPIO_TypeDef;

/**
 * @brief GPIO Port Base Addresses
 */
#define M0P_GPIO_PA_BASE    (0x40020000UL)
#define M0P_GPIO_PB_BASE    (0x40020400UL)
#define M0P_GPIO_PC_BASE    (0x40020800UL)
#define M0P_GPIO_PH_BASE    (0x40021000UL)

/**
 * @brief GPIO Port Pointers
 */
#define M0P_GPIO_PA         ((M0P_GPIO_TypeDef *)M0P_GPIO_PA_BASE)
#define M0P_GPIO_PB         ((M0P_GPIO_TypeDef *)M0P_GPIO_PB_BASE)
#define M0P_GPIO_PC         ((M0P_GPIO_TypeDef *)M0P_GPIO_PC_BASE)
#define M0P_GPIO_PH         ((M0P_GPIO_TypeDef *)M0P_GPIO_PH_BASE)

/*******************************************************************************
 * Global pre-processor symbols/macros
 ******************************************************************************/

/**
 * @brief GPIO Pin Definitions
 */
#define GPIO_PIN_0          (1u << 0)
#define GPIO_PIN_1          (1u << 1)
#define GPIO_PIN_2          (1u << 2)
#define GPIO_PIN_3          (1u << 3)
#define GPIO_PIN_4          (1u << 4)
#define GPIO_PIN_5          (1u << 5)
#define GPIO_PIN_6          (1u << 6)
#define GPIO_PIN_7          (1u << 7)
#define GPIO_PIN_ALL        (0xFFu)

/**
 * @brief GPIO Function Selection
 */
typedef enum {
    GpioFuncPortIn = 0,     /* Input */
    GpioFuncPortOut,        /* Output */
    GpioFuncAnalogIn,       /* Analog Input */
} en_gpio_func_t;

/**
 * @brief GPIO Pull-up/Pull-down Selection
 */
typedef enum {
    GpioPuDisable = 0,      /* Pull-up Disable */
    GpioPuEnable,           /* Pull-up Enable */
} en_gpio_pu_t;

typedef enum {
    GpioPdDisable = 0,      /* Pull-down Disable */
    GpioPdEnable,           /* Pull-down Enable */
} en_gpio_pd_t;

/**
 * @brief GPIO Drive Capability Selection
 */
typedef enum {
    GpioDrvL = 0,           /* Low Drive */
    GpioDrvH,               /* High Drive */
} en_gpio_drv_t;

/**
 * @brief GPIO Configuration Structure
 */
typedef struct {
    en_gpio_func_t enFunc;  /* Function Selection */
    en_gpio_pu_t   enPu;    /* Pull-up Selection */
    en_gpio_pd_t   enPd;    /* Pull-down Selection */
    en_gpio_drv_t  enDrv;   /* Drive Capability */
} stc_gpio_cfg_t;

/*******************************************************************************
 * Inline GPIO Functions (No board dependencies)
 ******************************************************************************/

/**
 * @brief Initialize GPIO Pin
 * @param [in] pstcGpio GPIO Port Pointer
 * @param [in] u8Pin GPIO Pin
 * @param [in] pstcCfg GPIO Configuration
 */
static inline void GPIO_Init(M0P_GPIO_TypeDef *pstcGpio, uint8_t u8Pin, stc_gpio_cfg_t *pstcCfg)
{
    if (pstcCfg->enFunc == GpioFuncPortOut) {
        pstcGpio->DDR |= u8Pin;  /* Output */
    } else {
        pstcGpio->DDR &= ~u8Pin; /* Input */
    }
    
    if (pstcCfg->enPu == GpioPuEnable) {
        pstcGpio->PUR |= u8Pin;
    } else {
        pstcGpio->PUR &= ~u8Pin;
    }
    
    if (pstcCfg->enPd == GpioPdEnable) {
        pstcGpio->PDR |= u8Pin;
    } else {
        pstcGpio->PDR &= ~u8Pin;
    }
    
    pstcGpio->DR |= (pstcCfg->enDrv == GpioDrvH) ? u8Pin : 0;
}

/**
 * @brief Write GPIO Pin
 * @param [in] pstcGpio GPIO Port Pointer
 * @param [in] u8Pin GPIO Pin
 * @param [in] bVal Value (TRUE/FALSE)
 */
static inline void GPIO_WriteBit(M0P_GPIO_TypeDef *pstcGpio, uint8_t u8Pin, uint8_t bVal)
{
    if (bVal) {
        pstcGpio->PDOR |= u8Pin;
    } else {
        pstcGpio->PDOR &= ~u8Pin;
    }
}

/**
 * @brief Read GPIO Pin
 * @param [in] pstcGpio GPIO Port Pointer
 * @param [in] u8Pin GPIO Pin
 * @return Pin Value (TRUE/FALSE)
 */
static inline uint8_t GPIO_ReadInputDataBit(M0P_GPIO_TypeDef *pstcGpio, uint8_t u8Pin)
{
    return (pstcGpio->PDIR & u8Pin) ? 1 : 0;
}

/**
 * @brief Toggle GPIO Pin
 * @param [in] pstcGpio GPIO Port Pointer
 * @param [in] u8Pin GPIO Pin
 */
static inline void GPIO_ToggleBits(M0P_GPIO_TypeDef *pstcGpio, uint8_t u8Pin)
{
    pstcGpio->PDOR ^= u8Pin;
}

#ifdef __cplusplus
}
#endif

#endif /* __HC32L021_MINIMAL_H__ */
