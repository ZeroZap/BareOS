/**
 * @file n32l40x_cfg.h
 * @brief Minimal SecBoot-N32 board configuration.
 */
#ifndef __N32L40X_CFG_H__
#define __N32L40X_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32l40x.h"
#include "n32l40x_conf.h"
#include <stddef.h>

extern void SysTick_Delayms(uint32_t Delayms);
extern void n32_debug_log_char(char ch);
extern void n32_debug_log_write(const char *str);
extern volatile uint32_t g_n32_debug_log_tx_count;
extern volatile uint8_t g_n32_debug_log_last_char;

extern bool RCC_Configuration(void);
extern bool GPIO_Configuration(void);
extern bool USART_Configuration(void);
extern bool IWDG_Configuration(void);

extern void n32_uart5_secboot_init(void);
extern void n32_uart5_secboot_poll(void);
extern void n32_uart5_secboot_isr(void);
extern void n32_uart5_secboot_write_str(const char *str);
extern int n32_uart5_secboot_read(uint8_t *data, size_t len, uint32_t timeout_ms);
extern int n32_uart5_secboot_write(const uint8_t *data, size_t len, uint32_t timeout_ms);
extern volatile uint32_t g_n32_uart5_rx_count;
extern volatile uint32_t g_n32_uart5_tx_count;
extern volatile uint32_t g_n32_uart5_rx_drop_count;
extern volatile uint32_t g_n32_uart5_rb_pending;
extern volatile uint8_t g_n32_uart5_last_rx;

#ifdef __cplusplus
}
#endif

#endif /* __N32L40X_CFG_H__ */
