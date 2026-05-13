/**
 * @file xy_w25qxx.c
 * @brief W25Qxx SPI Flash Driver Implementation
 * @version 1.0.0
 * @date 2026-03-01 凌晨 2:30
 */

#include "xy_w25qxx.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define W25Q_SECTOR_SIZE        4096        /* 4KB */
#define W25Q_BLOCK_SIZE         65536       /* 64KB */
#define W25Q_PAGE_SIZE          256         /* 256B */
#define W25Q_WRITE_TIMEOUT      1000        /* 1s */
#define W25Q_ERASE_TIMEOUT      5000        /* 5s */

/**
 * @brief 片选控制
 */
static inline void xy_w25q_cs_low(xy_w25qxx_t *dev)
{
    /* 根据实际平台实现 */
    xy_hal_gpio_write(dev->cs_pin, 0);
}

static inline void xy_w25q_cs_high(xy_w25qxx_t *dev)
{
    xy_hal_gpio_write(dev->cs_pin, 1);
}

/**
 * @brief 发送命令
 */
static int xy_w25q_send_cmd(xy_w25qxx_t *dev, uint8_t cmd)
{
    return xy_spi_device_write(&dev->spi_dev, &cmd, 1);
}

/**
 * @brief 发送命令 + 地址
 */
static int xy_w25q_send_cmd_addr(xy_w25qxx_t *dev, uint8_t cmd, uint32_t addr)
{
    uint8_t buf[4];
    buf[0] = cmd;
    buf[1] = (addr >> 16) & 0xFF;
    buf[2] = (addr >> 8) & 0xFF;
    buf[3] = addr & 0xFF;
    
    return xy_spi_device_write(&dev->spi_dev, buf, 4);
}

int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, uint8_t cs_pin)
{
    int ret;
    uint8_t manufacturer_id, device_id;
    
    if (!dev || !spi_handle) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 SPI */
    xy_spi_device_init(&dev->spi_dev, spi_handle, 1000000);  /* 1MHz */
    dev->cs_pin = cs_pin;
    
    /* 初始化 CS 引脚 */
    /* xy_hal_gpio_init(cs_pin, XY_HAL_GPIO_MODE_OUTPUT); */
    xy_w25q_cs_high(dev);
    
    xy_os_delay(10);
    
    /* 退出掉电模式 */
    xy_w25qxx_release_power_down(dev);
    
    /* 读取 ID */
    ret = xy_w25qxx_read_id(dev, &manufacturer_id, &device_id);
    if (ret != XY_W25Q_OK) {
        xy_log_e("Failed to read Flash ID\n");
        return XY_W25Q_NOT_FOUND;
    }
    
    xy_log_i("W25Qxx found: Manufacturer=0x%02X, Device=0x%02X\n", 
             manufacturer_id, device_id);
    
    /* 识别型号 */
    switch (device_id) {
        case W25Q16_ID:
            dev->model = W25Q16;
            dev->capacity_bytes = 2 * 1024 * 1024;  /* 2MB */
            break;
        case W25Q32_ID:
            dev->model = W25Q32;
            dev->capacity_bytes = 4 * 1024 * 1024;  /* 4MB */
            break;
        case W25Q64_ID:
            dev->model = W25Q64;
            dev->capacity_bytes = 8 * 1024 * 1024;  /* 8MB */
            break;
        default:
            dev->model = W25Q_UNKNOWN;
            dev->capacity_bytes = 0;
            xy_log_w("Unknown W25Q model\n");
            break;
    }
    
    dev->sector_count = dev->capacity_bytes / W25Q_SECTOR_SIZE;
    dev->initialized = 1;
    
    xy_log_i("W25Q%d initialized (%d bytes, %d sectors)\n",
             dev->model, dev->capacity_bytes, dev->sector_count);
    
    return XY_W25Q_OK;
}

int xy_w25qxx_deinit(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_power_down(dev);
    dev->initialized = 0;
    
    return XY_W25Q_OK;
}

int xy_w25qxx_read_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id, uint8_t *device_id)
{
    uint8_t cmd = W25Q_CMD_MANUFACTURER_ID;
    uint8_t addr[3] = {0x00, 0x00, 0x00};
    uint8_t rx[2];
    
    if (!dev || !manufacturer_id || !device_id) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_spi_device_write(&dev->spi_dev, &cmd, 1);
    xy_spi_device_write(&dev->spi_dev, addr, 3);
    xy_spi_device_read(&dev->spi_dev, rx, 2);
    xy_w25q_cs_high(dev);
    
    *manufacturer_id = rx[0];
    *device_id = rx[1];
    
    return XY_W25Q_OK;
}

int xy_w25qxx_read_status(xy_w25qxx_t *dev, uint8_t *status)
{
    uint8_t cmd = W25Q_CMD_READ_STATUS_REG1;
    
    if (!dev || !status) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_spi_device_write(&dev->spi_dev, &cmd, 1);
    xy_spi_device_read(&dev->spi_dev, status, 1);
    xy_w25q_cs_high(dev);
    
    return XY_W25Q_OK;
}

int xy_w25qxx_wait_idle(xy_w25qxx_t *dev, uint32_t timeout)
{
    uint8_t status;
    uint32_t start = xy_os_tick_get();
    
    do {
        xy_w25qxx_read_status(dev, &status);
        if (!(status & 0x01)) {  /* BUSY bit */
            return XY_W25Q_OK;
        }
        xy_os_delay(1);
    } while ((xy_os_tick_get() - start) < timeout);
    
    return XY_W25Q_BUSY;
}

int xy_w25qxx_write_enable(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd(dev, W25Q_CMD_WRITE_ENABLE);
    xy_w25q_cs_high(dev);
    
    return XY_W25Q_OK;
}

int xy_w25qxx_sector_erase(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev || (addr % W25Q_SECTOR_SIZE) != 0) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    /* 写入使能 */
    xy_w25qxx_write_enable(dev);
    
    /* 发送擦除命令 */
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd_addr(dev, W25Q_CMD_SECTOR_ERASE, addr);
    xy_w25q_cs_high(dev);
    
    /* 等待完成 */
    return xy_w25qxx_wait_idle(dev, W25Q_ERASE_TIMEOUT);
}

int xy_w25qxx_block_erase(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev || (addr % W25Q_BLOCK_SIZE) != 0) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd_addr(dev, W25Q_CMD_BLOCK_ERASE_64K, addr);
    xy_w25q_cs_high(dev);
    
    return xy_w25qxx_wait_idle(dev, W25Q_ERASE_TIMEOUT);
}

int xy_w25qxx_chip_erase(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd(dev, W25Q_CMD_CHIP_ERASE);
    xy_w25q_cs_high(dev);
    
    return xy_w25qxx_wait_idle(dev, W25Q_ERASE_TIMEOUT * 10);
}

int xy_w25qxx_page_program(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (!dev || !data || len > W25Q_PAGE_SIZE) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd_addr(dev, W25Q_CMD_PAGE_PROGRAM, addr);
    xy_spi_device_write(&dev->spi_dev, data, len);
    xy_w25q_cs_high(dev);
    
    return xy_w25qxx_wait_idle(dev, W25Q_WRITE_TIMEOUT);
}

int xy_w25qxx_read_data(xy_w25qxx_t *dev, uint32_t addr, uint8_t *data, uint32_t len)
{
    if (!dev || !data) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd_addr(dev, W25Q_CMD_READ_DATA, addr);
    xy_spi_device_read(&dev->spi_dev, data, len);
    xy_w25q_cs_high(dev);
    
    return XY_W25Q_OK;
}

int xy_w25qxx_write_data(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint16_t page_offset;
    uint16_t write_len;
    int ret;
    
    if (!dev || !data) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    for (i = 0; i < len; ) {
        /* 计算页边界 */
        page_offset = addr % W25Q_PAGE_SIZE;
        write_len = (page_offset + len - i > W25Q_PAGE_SIZE) ? 
                    (W25Q_PAGE_SIZE - page_offset) : (len - i);
        
        /* 擦除扇区 (如果必要) */
        if ((addr % W25Q_SECTOR_SIZE) == 0) {
            ret = xy_w25qxx_sector_erase(dev, addr);
            if (ret != XY_W25Q_OK) {
                return ret;
            }
        }
        
        /* 页编程 */
        ret = xy_w25qxx_page_program(dev, addr, &data[i], write_len);
        if (ret != XY_W25Q_OK) {
            return ret;
        }
        
        addr += write_len;
        i += write_len;
    }
    
    return XY_W25Q_OK;
}

int xy_w25qxx_power_down(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd(dev, W25Q_CMD_POWER_DOWN);
    xy_w25q_cs_high(dev);
    
    return XY_W25Q_OK;
}

int xy_w25qxx_release_power_down(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_W25Q_INVALID_PARAM;
    }
    
    xy_w25q_cs_low(dev);
    xy_w25q_send_cmd(dev, W25Q_CMD_RELEASE_POWER_DOWN);
    xy_w25q_cs_high(dev);
    
    xy_os_delay(10);
    
    return XY_W25Q_OK;
}
