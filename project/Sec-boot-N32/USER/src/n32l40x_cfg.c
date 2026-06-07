/**
 * @file n32l40x_cfg.c
 * @brief Minimal SecBoot-N32 board configuration.
 */

#include "n32l40x_cfg.h"
#include "xy_rb.h"
#include <stdio.h>

__IO uint32_t mwTick;
volatile uint32_t g_n32_debug_log_tx_count;
volatile uint8_t g_n32_debug_log_last_char;
volatile uint32_t g_n32_uart5_rx_count;
volatile uint32_t g_n32_uart5_tx_count;
volatile uint32_t g_n32_uart5_rx_drop_count;
volatile uint32_t g_n32_uart5_rb_pending;
volatile uint8_t g_n32_uart5_last_rx;

static xy_rb_t s_uart5_rx_rb;
static uint8_t s_uart5_rx_pool[128];

static void n32_uart5_secboot_irq_enable(void)
{
    NVIC_InitType NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void SysTick_Delayms(uint32_t Delayms)
{
    uint32_t tickstart = mwTick;
    uint32_t wait = Delayms;

    if (wait < 0xFFFFFFFFU) {
        wait += 1U;
    }
    while ((mwTick - tickstart) < wait) {
        IWDG_ReloadKey();
    }
}

void n32_debug_log_char(char ch)
{
    if (ch == '\n') {
        n32_debug_log_char('\r');
    }

    while (USART_GetFlagStatus(UART4, USART_FLAG_TXDE) == RESET) {
    }
    USART_SendData(UART4, (uint16_t)ch);
    g_n32_debug_log_last_char = (uint8_t)ch;
    g_n32_debug_log_tx_count++;
}

void n32_debug_log_write(const char *str)
{
    while (*str != '\0') {
        n32_debug_log_char(*str++);
    }
}

void xy_log_char(char ch)
{
    n32_debug_log_char(ch);
}

void n32_uart5_secboot_send_char(char ch)
{
    while (USART_GetFlagStatus(UART5, USART_FLAG_TXDE) == RESET) {
    }
    USART_SendData(UART5, (uint16_t)ch);
    g_n32_uart5_tx_count++;
}

void n32_uart5_secboot_write_str(const char *str)
{
    while (*str != '\0') {
        n32_uart5_secboot_send_char(*str++);
    }
}

void n32_uart5_secboot_init(void)
{
    xy_rb_init(&s_uart5_rx_rb, s_uart5_rx_pool, (int32_t)sizeof(s_uart5_rx_pool));
    while (USART_GetFlagStatus(UART5, USART_FLAG_RXDNE) == SET) {
        (void)USART_ReceiveData(UART5);
    }
    USART_ConfigInt(UART5, USART_INT_RXDNE, ENABLE);
    USART_ConfigInt(UART5, USART_INT_ERRF, ENABLE);
    n32_uart5_secboot_irq_enable();
}

void n32_uart5_secboot_poll(void)
{
    g_n32_uart5_rb_pending = (uint32_t)xy_rb_data_len(&s_uart5_rx_rb);
}

int n32_uart5_secboot_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
    uint32_t start;
    size_t count = 0U;

    if ((data == NULL) && (len != 0U)) {
        return -1;
    }

    start = mwTick;
    while (count < len) {
        if (xy_rb_getchar(&s_uart5_rx_rb, &data[count]) == 1U) {
            count++;
            continue;
        }

        if ((int32_t)(mwTick - start) >= (int32_t)timeout_ms) {
            break;
        }
        IWDG_ReloadKey();
    }

    g_n32_uart5_rb_pending = (uint32_t)xy_rb_data_len(&s_uart5_rx_rb);
    return (int)count;
}

int n32_uart5_secboot_write(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    uint32_t start;
    size_t count = 0U;

    if ((data == NULL) && (len != 0U)) {
        return -1;
    }

    start = mwTick;
    while (count < len) {
        if (USART_GetFlagStatus(UART5, USART_FLAG_TXDE) == SET) {
            USART_SendData(UART5, (uint16_t)data[count]);
            g_n32_uart5_tx_count++;
            count++;
            start = mwTick;
            continue;
        }

        if ((int32_t)(mwTick - start) >= (int32_t)timeout_ms) {
            break;
        }
        IWDG_ReloadKey();
    }

    return (int)count;
}

void n32_uart5_secboot_isr(void)
{
    if (USART_GetIntStatus(UART5, USART_INT_RXDNE) == SET) {
        uint8_t ch = (uint8_t)USART_ReceiveData(UART5);
        g_n32_uart5_last_rx = ch;
        g_n32_uart5_rx_count++;
        if (xy_rb_putchar(&s_uart5_rx_rb, ch) == 0U) {
            g_n32_uart5_rx_drop_count++;
        }
    }

    if ((USART_GetFlagStatus(UART5, USART_FLAG_OREF) == SET) ||
        (USART_GetFlagStatus(UART5, USART_FLAG_NEF) == SET) ||
        (USART_GetFlagStatus(UART5, USART_FLAG_FEF) == SET) ||
        (USART_GetFlagStatus(UART5, USART_FLAG_PEF) == SET)) {
        (void)UART5->STS;
        (void)UART5->DAT;
    }
}

int fputc(int ch, FILE *f)
{
    (void)f;
    n32_debug_log_char((char)ch);
    return ch;
}

int _write(int file, char *ptr, int len)
{
    int i;

    (void)file;
    for (i = 0; i < len; i++) {
        n32_debug_log_char(ptr[i]);
    }
    return len;
}

bool RCC_Configuration(void)
{
    ErrorStatus ClockStatus;

    RCC_DeInit();
    RCC_ConfigHclk(RCC_SYSCLK_DIV1);
    RCC_ConfigPclk2(RCC_HCLK_DIV4);
    RCC_ConfigPclk1(RCC_HCLK_DIV4);

    RCC_EnableHsi(ENABLE);
    ClockStatus = RCC_WaitHsiStable();
    if (ClockStatus != SUCCESS) {
        return false;
    }

    RCC_ConfigHse(RCC_HSE_DISABLE);
    RCC_ConfigPll(RCC_PLL_HSI_PRE_DIV2, RCC_PLL_MUL_12, RCC_PLLDIVCLK_ENABLE);
    RCC_EnablePll(ENABLE);
    while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) != SET) {
    }

    FLASH_PrefetchBufSet(FLASH_PrefetchBuf_DIS);
    FLASH_iCacheCmd(FLASH_iCache_EN);
    FLASH_SetLatency(FLASH_LATENCY_1);

    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);
    while (RCC_GetSysclkSrc() != 0x0c) {
    }

    SysTick_Config(48000U);
    NVIC_SetPriority(SysTick_IRQn, 0);
    return true;
}

bool GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;

    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pull = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Current = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitStructure.Pin = GPIO_PIN_0;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
    GPIO_InitStructure.Pin = GPIO_PIN_8;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitStructure.Pin = GPIO_PIN_1;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
    GPIO_InitStructure.Pin = GPIO_PIN_9;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
    return true;
}

bool USART_Configuration(void)
{
    USART_InitType USART_InitStructure;

    USART_StructInit(&USART_InitStructure);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4 | RCC_APB2_PERIPH_UART5, ENABLE);

    USART_InitStructure.BaudRate = 115200;
    USART_InitStructure.WordLength = USART_WL_8B;
    USART_InitStructure.StopBits = USART_STPB_1;
    USART_InitStructure.Parity = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;

    USART_Init(UART4, &USART_InitStructure);
    USART_ConfigInt(UART4, USART_INT_IDLEF, DISABLE);
    USART_Enable(UART4, ENABLE);

    USART_Init(UART5, &USART_InitStructure);
    USART_ConfigInt(UART5, USART_INT_IDLEF, DISABLE);
    USART_Enable(UART5, ENABLE);
    return true;
}

bool IWDG_Configuration(void)
{
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV256);
    IWDG_CntReload(0x0fff);
    while (IWDG_GetStatus(IWDG_PVU_FLAG) == SET ||
           IWDG_GetStatus(IWDG_CRVU_FLAG) == SET) {
    }
    IWDG_ReloadKey();
    IWDG_WriteConfig(IWDG_WRITE_DISABLE);
    return true;
}
