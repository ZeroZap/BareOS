/**
 * @file xy_hal_uart_device.c
 * @brief HC32L021 UART Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 UART 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_uart_dev.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_UART_COUNT  (2)  /* UART0/UART1 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 UART 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* UART 实例号 (0/1) */
    bool initialized;          /* 初始化标志 */
    xy_hal_uart_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
} hc32_uart_dev_t;

/* ==================== Private Variables ==================== */

/* UART 设备实例池 */
static hc32_uart_dev_t uart_devices[HC32L021_UART_COUNT];
static bool uart_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 UART 设备池
 */
static void uart_init_devices(void)
{
    if (!uart_devices_initialized) {
        memset(uart_devices, 0, sizeof(uart_devices));
        uart_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 UART 设备
 * @param instance UART 实例号
 * @return hc32_uart_dev_t* UART 设备指针
 */
static hc32_uart_dev_t *uart_find_or_alloc(uint8_t instance)
{
    uart_init_devices();
    
    if (instance >= HC32L021_UART_COUNT) {
        return NULL;
    }
    
    hc32_uart_dev_t *dev = &uart_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
    }
    
    return dev;
}

/**
 * @brief 解析 UART 名称
 * @param name UART 名称 (如 "UART0", "UART1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_uart_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "UART", 4) == 0) {
        if (name[4] >= '0' && name[4] <= '1') {
            *instance = name[4] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_uart_t xy_hal_uart_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_uart_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_uart_dev_t *dev = uart_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_uart_t)dev;
}

xy_hal_error_t xy_hal_uart_unbind(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_uart_dev_t *dev = (hc32_uart_dev_t *)uart;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart,
                                     const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_uart_dev_t *dev = (hc32_uart_dev_t *)uart;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
/* 配置 HC32L021 UART 硬件寄存器 */
    /* 注意：需要 HC32 SDK 头文件 support_device.h */
    /* 1. 配置波特率 - 使用定时器计算分频 */
    /* 2. 配置数据位 (7/8/9) */
    /* 3. 配置停止位 (1/1.5/2) */
    /* 4. 配置校验位 (无/奇/偶) */
    /* 5. 使能 UART */
    
    /* 伪代码实现 - 实际需根据 HC32 SDK 调整 */
    #if defined(HC32_L021_SUPPORT)
    /* 波特率计算 */
    uint32_t pclk = SystemCoreClock;
    uint16_t baud_div = (pclk / (16 * config->baudrate)) - 1;
    
    /* 配置寄存器 */
    M0P_UART0->SCRL_b.SMODE = 0;  /* UART 模式 */
    M0P_UART0->SCRL_b.PREN = (config->parity != XY_HAL_UART_PARITY_NONE) ? 1 : 0;
    M0P_UART0->SCRL_b.EVEN = (config->parity == XY_HAL_UART_PARITY_EVEN) ? 1 : 0;
    M0P_UART0->SCRL_b.STOP = (config->stop_bits == XY_HAL_UART_STOP_2) ? 1 : 0;
    M0P_UART0->SCRL_b.DOR = 0;  /* 8 数据位 */
    
    /* 波特率分频 */
    M0P_UART0->SCRL_b.BGR = baud_div & 0xFF;
    M0P_UART0->SCRH_b.SGR = (baud_div >> 8) & 0x03;
    
    /* 使能 UART */
    M0P_UART0->SCRL_b.UARTEN = 1;
    #else
    /* 无 SDK 时返回错误 */
    return XY_HAL_ERROR_NOT_SUPPORTED;
    #endif
    
    return XY_HAL_OK;
}

int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data,
                          size_t length, uint32_t timeout)
{
    if (!uart || !data || length == 0) {
        return -1;
    }
    
    hc32_uart_dev_t *dev = (hc32_uart_dev_t *)uart;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
/* 实现 UART 发送 */
    /* 1. 等待 TXE (发送缓冲区空) */
    /* 2. 写入数据到 DR */
    /* 3. 等待传输完成 */
    
    #if defined(HC32_L021_SUPPORT)
    for (size_t i = 0; i < length; i++) {
        /* 等待 TXE */
        while (!M0P_UART0->SCHR_b.TC) {
            if (timeout && (xy_hal_get_tick() - start) > timeout) {
                dev->busy = false;
                return -XY_HAL_ERROR_TIMEOUT;
            }
        }
        /* 写入数据 */
        M0P_UART0->SCHR_b.DATA = data[i];
    }
    /* 等待传输完成 */
    while (!M0P_UART0->SCHR_b.TC);
    #else
    (void)timeout;
    return -XY_HAL_ERROR_NOT_SUPPORTED;
    #endif
    
    dev->busy = false;
    
    return (int32_t)length;
}

int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data,
                         size_t length, uint32_t timeout)
{
    if (!uart || !data || length == 0) {
        return -1;
    }
    
    hc32_uart_dev_t *dev = (hc32_uart_dev_t *)uart;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
/* 实现 UART 接收 */
    /* 1. 等待 RXNE (接收缓冲区非空) */
    /* 2. 读取 DR 数据 */
    
    #if defined(HC32_L021_SUPPORT)
    uint32_t start = xy_hal_get_tick();
    for (size_t i = 0; i < length; i++) {
        /* 等待 RXNE */
        while (!M0P_UART0->SCHR_b.RC) {
            if (timeout && (xy_hal_get_tick() - start) > timeout) {
                dev->busy = false;
                return -XY_HAL_ERROR_TIMEOUT;
            }
        }
        /* 读取数据 */
        data[i] = M0P_UART0->SCHR_b.DATA & 0xFF;
    }
    #else
    (void)timeout;
    return -XY_HAL_ERROR_NOT_SUPPORTED;
    #endif
    
    dev->busy = false;
    
    return (int32_t)length;
}

/* ==================== End of File ==================== */
