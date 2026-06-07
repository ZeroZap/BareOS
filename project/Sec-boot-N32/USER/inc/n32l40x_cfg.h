/**
 * @file n32l40x_cfg.h
 * @author N32cube
 */
#ifndef __N32L40X_CFG_H__
#define __N32L40X_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif
/* NTFx CODE START INCLUDE*/
#include "n32l40x.h"
#include "n32l40x_conf.h"
#include "usb_lib.h"
#include <stddef.h>
/* NTFx CODE END INCLUDE*/

/* NTFx CODE START ENUM*/
typedef enum {
    RTC_CLK_SRC_TYPE_HSE128=0x01,
    RTC_CLK_SRC_TYPE_LSE=0x02,
    RTC_CLK_SRC_TYPE_LSI=0x03,
}RTC_CLK_SRC_TYPE;
/* NTFx CODE END ENUM*/

/* NTFx CODE START EXTERN*/
extern void SysTick_Delayms(uint32_t Delayms);
extern void DMA_SetPerMemAddr(DMA_ChannelType* DMAChx, uint32_t periphAddr,uint32_t memAddr,uint32_t bufSize );
extern void n32_debug_log_char(char ch);
extern void n32_debug_log_write(const char *str);
extern volatile uint32_t g_n32_debug_log_tx_count;
extern volatile uint8_t g_n32_debug_log_last_char;
extern bool RCC_Configuration(void);
extern bool NVIC_Configuration(void);
extern bool DMA_Configuration(void);
extern bool GPIO_Configuration(void);
extern bool USART_Configuration(void);
extern bool LPTIM_Configuration(void);
extern bool RTC_Configuration(void);
extern bool IWDG_Configuration(void);
extern bool ADC_Configuration(void);
extern bool USBFS_Configuration(void);
extern bool SPI_Configuration(void);
extern bool I2C_Configuration(void);
/* NTFx CODE END EXTERN*/
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
/**
 * @}
 */
