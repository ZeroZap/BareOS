/**
 * @file  ddl.h
 * @brief Minimal DDL header for HC32L021 - Optimized for XinYi HAL
 * @note  简化版本 - 移除了不必要的依赖
 */

#ifndef __DDL_H__
#define __DDL_H__

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Include files - Minimal set
 ******************************************************************************/
#include "base_types.h"
#include "hc32l021.h"

/*******************************************************************************
 * Global type definitions
 ******************************************************************************/

/**
 * @brief 布尔类型
 */
#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

/**
 * @brief 寄存器访问宏
 */
#define REG8(addr)      (*(volatile uint8_t *)(addr))
#define REG16(addr)     (*(volatile uint16_t *)(addr))
#define REG32(addr)     (*(volatile uint32_t *)(addr))

/**
 * @brief 位操作宏
 */
#define SET_BIT(REG, BIT)       ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)     ((REG) &= ~(BIT))
#define TOGGLE_BIT(REG, BIT)    ((REG) ^= (BIT))
#define READ_BIT(REG, BIT)      ((REG) & (BIT))

/**
 * @brief 结构体清零宏
 */
#define DDL_ZERO_STRUCT(x)      memset(&(x), 0, sizeof(x))

/*******************************************************************************
 * Global preprocessor symbols/macros ('define')
 ******************************************************************************/

/**
 * @brief 断言宏 (简化版本)
 */
#ifdef USE_FULL_ASSERT
#define DDL_ASSERT(expr)    ((expr) ? (void)0 : (void)0)
#else
#define DDL_ASSERT(expr)    ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DDL_H__ */
