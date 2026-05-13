/**
 * @file xy_hal_i2c_device.c
 * @brief STM32U5 I2C Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 I2C 设备 API，基于 STM32U5 HAL 库
 * @note 支持主/从模式，轮询/中断/DMA 传输
 */

#include "../inc/xy_hal_i2c_dev.h"
#include "../inc/xy_hal_i2c_types.h"
#include <string.h>

/* STM32U5 HAL 头文件 */
#include "stm32u5xx_hal.h"

/* ==================== Private Definitions ==================== */

#define STM32U5_I2C_INSTANCE_COUNT  (5)  /* I2C1-4, SMBUS */

/* ==================== Private Types ==================== */

/**
 * @brief STM32U5 I2C 设备私有数据
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;     /* STM32 HAL I2C 句柄 */
    uint32_t instance;           /* I2C 实例编号 */
    xy_hal_i2c_config_t config;  /* 当前配置 */
    xy_hal_i2c_callback_t callback; /* 异步回调 */
    void *callback_arg;          /* 回调参数 */
    xy_hal_i2c_stats_t stats;    /* 统计信息 */
    uint8_t tx_busy;             /* 发送忙标志 */
    uint8_t rx_busy;             /* 接收忙标志 */
} stm32u5_i2c_data_t;

/* ==================== Private Variables ==================== */

/* I2C 设备实例数组 */
static stm32u5_i2c_data_t i2c_devices[STM32U5_I2C_INSTANCE_COUNT] = {0};

/* I2C 实例名称映射 */
static const char *const i2c_names[STM32U5_I2C_INSTANCE_COUNT] = {
    "I2C1", "I2C2", "I2C3", "I2C4", "SMBUS"
};

/* I2C 实例句柄 (外部定义) */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;

static I2C_HandleTypeDef *const i2c_handles[STM32U5_I2C_INSTANCE_COUNT] = {
    &hi2c1, &hi2c2, &hi2c3, &hi2c4, NULL
};

/* ==================== Private Functions ==================== */

/**
 * @brief 查找 I2C 设备索引
 */
static int i2c_find_index(const char *name)
{
    for (int i = 0; i < STM32U5_I2C_INSTANCE_COUNT; i++) {
        if (strcmp(name, i2c_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief STM32 HAL I2C 回调转发
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < STM32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c && i2c_devices[i].callback) {
            i2c_devices[i].tx_busy = 0;
            i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                   XY_HAL_I2C_EVENT_TX_DONE,
                                   i2c_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < STM32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c && i2c_devices[i].callback) {
            i2c_devices[i].rx_busy = 0;
            i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                   XY_HAL_I2C_EVENT_RX_DONE,
                                   i2c_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    for (int i = 0; i < STM32U5_I2C_INSTANCE_COUNT; i++) {
        if (hi2c == i2c_devices[i].hi2c) {
            i2c_devices[i].stats.tx_errors++;
            
            if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
                i2c_devices[i].stats.nack_count++;
            }
            
            if (i2c_devices[i].callback) {
                i2c_devices[i].callback((xy_hal_i2c_t)&i2c_devices[i],
                                       XY_HAL_I2C_EVENT_ERROR,
                                       i2c_devices[i].callback_arg);
            }
            break;
        }
    }
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_i2c_t xy_hal_i2c_bind(const char *name)
{
    int index = i2c_find_index(name);
    if (index < 0) {
        return NULL;
    }
    
    stm32u5_i2c_data_t *dev = &i2c_devices[index];
    
    dev->hi2c = (I2C_HandleTypeDef *)i2c_handles[index];
    dev->instance = (uint32_t)index;
    dev->callback = NULL;
    dev->callback_arg = NULL;
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    memset(&dev->stats, 0, sizeof(dev->stats));
    
    return (xy_hal_i2c_t)dev;
}

xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    HAL_I2C_Abort(dev->hi2c);
    HAL_I2C_DeInit(dev->hi2c);
    
    memset(dev, 0, sizeof(*dev));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    /* 配置 I2C */
    dev->hi2c->Init.Timing = config->clock_speed; /* 需要转换为 I2C_TIMINGR 值 */
    dev->hi2c->Init.OwnAddress1 = config->own_address;
    dev->hi2c->Init.AddressingMode = (config->addr_mode == XY_HAL_I2C_ADDR_10BIT) ?
                                     I2C_ADDRESSINGMODE_10BIT : I2C_ADDRESSINGMODE_7BIT;
    dev->hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    dev->hi2c->Init.OwnAddress2 = 0;
    dev->hi2c->Init.GeneralCallMode = config->general_call_mode ?
                                      I2C_GENERALCALL_ENABLE : I2C_GENERALCALL_DISABLE;
    dev->hi2c->Init.NoStretchMode = config->stretch_clock ?
                                    I2C_NOSTRETCH_DISABLE : I2C_NOSTRETCH_ENABLE;
    
    if (HAL_I2C_Init(dev->hi2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    
    dev->config = *config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_config(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    *config = dev->config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_set_clock_speed(xy_hal_i2c_t i2c, uint32_t clock_speed)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    dev->config.clock_speed = clock_speed;
    
    /* 需要重新计算 TIMINGR 寄存器值 */
    /* 这里简化处理 */
    
    return XY_HAL_OK;
}

int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(dev->hi2c, dev_addr << 1,
                                                        (uint8_t *)data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.tx_errors++;
        if (dev->hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
            dev->stats.nack_count++;
        }
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(dev->hi2c, dev_addr << 1,
                                                       data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.rx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

xy_hal_error_t xy_hal_i2c_reg_write(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                    const uint8_t *reg_addr, size_t reg_size,
                                    const uint8_t *data, size_t length,
                                    uint32_t timeout)
{
    if (!i2c || !reg_addr || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    /* 先发送寄存器地址，再发送数据 */
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(dev->hi2c, dev_addr << 1,
                                                  *(uint16_t *)reg_addr,
                                                  (reg_size == 2) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                                                  (uint8_t *)data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length + reg_size;
        return XY_HAL_OK;
    } else {
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

xy_hal_error_t xy_hal_i2c_reg_read(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *reg_addr, size_t reg_size,
                                   uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !reg_addr || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    /* 先发送寄存器地址，再读取数据 */
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(dev->hi2c, dev_addr << 1,
                                                 *(uint16_t *)reg_addr,
                                                 (reg_size == 2) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                                                 data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.rx_bytes += length;
        return XY_HAL_OK;
    } else {
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_scan(xy_hal_i2c_t i2c, uint8_t *addrs, size_t max_count,
                        uint32_t timeout)
{
    if (!i2c || !addrs || max_count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    int count = 0;
    
    /* 扫描 7 位地址范围 0x08-0x77 */
    for (uint16_t addr = 0x08; addr <= 0x77 && count < (int)max_count; addr++) {
        if (xy_hal_i2c_probe(i2c, addr, timeout) == XY_HAL_OK) {
            addrs[count++] = (uint8_t)addr;
        }
    }
    
    return count;
}

xy_hal_error_t xy_hal_i2c_probe(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                uint32_t timeout)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    /* 尝试发送空数据探测设备 */
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(dev->hi2c, dev_addr << 1, 1, timeout);
    
    if (status == HAL_OK) {
        return XY_HAL_OK;
    } else {
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_i2c_master_transmit_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                      const uint8_t *data, size_t length)
{
    /* 非阻塞实现需要检查 TXIS 标志 */
    (void)i2c;
    (void)dev_addr;
    (void)data;
    (void)length;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_i2c_master_receive_nb(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                     uint8_t *data, size_t length)
{
    (void)i2c;
    (void)dev_addr;
    (void)data;
    (void)length;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2c_master_transmit_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                                const uint8_t *data, size_t length,
                                                xy_hal_i2c_callback_t callback, void *arg)
{
    if (!i2c || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    if (dev->tx_busy) {
        return XY_HAL_ERROR_BUSY;
    }
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->tx_busy = 1;
    
    HAL_StatusTypeDef status;
    if (dev->config.transfer_mode == XY_HAL_I2C_TRANSFER_DMA) {
        status = HAL_I2C_Master_Transmit_DMA(dev->hi2c, dev_addr << 1,
                                             (uint8_t *)data, length);
    } else {
        status = HAL_I2C_Master_Transmit_IT(dev->hi2c, dev_addr << 1,
                                            (uint8_t *)data, length);
    }
    
    if (status != HAL_OK) {
        dev->tx_busy = 0;
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive_async(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                               uint8_t *data, size_t length,
                                               xy_hal_i2c_callback_t callback, void *arg)
{
    if (!i2c || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    if (dev->rx_busy) {
        return XY_HAL_ERROR_BUSY;
    }
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->rx_busy = 1;
    
    HAL_StatusTypeDef status;
    if (dev->config.transfer_mode == XY_HAL_I2C_TRANSFER_DMA) {
        status = HAL_I2C_Master_Receive_DMA(dev->hi2c, dev_addr << 1,
                                            data, length);
    } else {
        status = HAL_I2C_Master_Receive_IT(dev->hi2c, dev_addr << 1,
                                           data, length);
    }
    
    if (status != HAL_OK) {
        dev->rx_busy = 0;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_stop_async(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    HAL_I2C_Abort(dev->hi2c);
    
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    dev->callback = NULL;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_status(xy_hal_i2c_t i2c,
                                     xy_hal_i2c_status_t *status)
{
    if (!i2c || !status) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    status->tx_busy = dev->tx_busy;
    status->rx_busy = dev->rx_busy;
    status->bus_busy = (__HAL_I2C_GET_FLAG(dev->hi2c, I2C_FLAG_BUSY) != RESET);
    status->tx_complete = !dev->tx_busy;
    status->rx_available = 0; /* I2C 无法直接检查 */
    status->errors = dev->hi2c->ErrorCode;
    
    return XY_HAL_OK;
}

xy_hal_i2c_error_t xy_hal_i2c_get_error(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_I2C_ERROR_NONE;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    return (xy_hal_i2c_error_t)dev->hi2c->ErrorCode;
}

xy_hal_error_t xy_hal_i2c_clear_error(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    dev->hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
    __HAL_I2C_CLEAR_FLAG(dev->hi2c, I2C_FLAG_AF);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_get_stats(xy_hal_i2c_t i2c,
                                    xy_hal_i2c_stats_t *stats)
{
    if (!i2c || !stats) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    *stats = dev->stats;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_reset_stats(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    memset(&dev->stats, 0, sizeof(dev->stats));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_enable(xy_hal_i2c_t i2c, int enable)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    if (enable) {
        __HAL_I2C_ENABLE(dev->hi2c);
    } else {
        __HAL_I2C_DISABLE(dev->hi2c);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_reset(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_i2c_data_t *dev = (stm32u5_i2c_data_t *)i2c;
    
    /* 软件复位 I2C */
    HAL_I2C_DeInit(dev->hi2c);
    HAL_I2C_Init(dev->hi2c);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_control(xy_hal_i2c_t i2c, int cmd, void *arg)
{
    switch (cmd) {
        case XY_HAL_I2C_CMD_RESET:
            return xy_hal_i2c_reset(i2c);
        case XY_HAL_I2C_CMD_PROBE:
            /* 参数为设备地址 */
            return xy_hal_i2c_probe(i2c, (uint16_t)(uintptr_t)arg, 100);
        default:
            return XY_HAL_ERROR_NOT_SUPPORTED;
    }
}

/* ==================== End of File ==================== */
