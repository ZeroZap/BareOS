/**
 * @file xy_w25qxx.c
 * @brief W25Qxx SPI Flash Memory Driver Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 W25Q16/W25Q32/W25Q64/W25Q128 系列
 * 
 * 容量规格:
 * - W25Q16: 2MB (256 块，512 扇区)
 * - W25Q32: 4MB (512 块，1024 扇区)
 * - W25Q64: 8MB (1024 块，2048 扇区)
 * - W25Q128: 16MB (2048 块，4096 扇区)
 */

#include "xy_w25qxx.h"
#include <string.h>

/* ==================== Private Functions ==================== */

/**
 * @brief 片选使能
 */
static void cs_enable(xy_w25qxx_t *dev)
{
    XY_HAL_GPIO_WRITE(dev->cs_gpio, 0);  /* CS 拉低 */
    (void)dev;
}

/**
 * @brief 片选禁用
 */
static void cs_disable(xy_w25qxx_t *dev)
{
    XY_HAL_GPIO_WRITE(dev->cs_gpio, 1);  /* CS 拉高 */
    (void)dev;
}

/**
 * @brief SPI 发送/接收
 */
static int spi_transfer(xy_w25qxx_t *dev, const uint8_t *tx, uint8_t *rx, 
                        size_t length)
{
    return xy_hal_spi_transfer(dev->spi, tx_data, rx_data, length, timeout);
    (void)dev;
    (void)tx;
    (void)rx;
    (void)length;
    return 0;
}

/**
 * @brief 发送命令
 */
static int send_command(xy_w25qxx_t *dev, uint8_t cmd)
{
    cs_enable(dev);
    int ret = spi_transfer(dev, &cmd, NULL, 1);
    return ret;
}

/**
 * @brief 发送命令 + 地址
 */
static int send_command_addr(xy_w25qxx_t *dev, uint8_t cmd, uint32_t addr)
{
    uint8_t buffer[4];
    buffer[0] = cmd;
    buffer[1] = (addr >> 16) & 0xFF;
    buffer[2] = (addr >> 8) & 0xFF;
    buffer[3] = addr & 0xFF;
    
    cs_enable(dev);
    int ret = spi_transfer(dev, buffer, NULL, 4);
    return ret;
}

/* ==================== Public Implementation ==================== */

int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, void *cs_gpio)
{
    if (!dev || !spi_handle || !cs_gpio) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->spi_handle = spi_handle;
    dev->cs_gpio = cs_gpio;
    dev->write_enabled = false;
    
    /* 读取 JEDEC ID 识别型号 */
    uint8_t jedec_id[3];
    int ret = xy_w25qxx_read_jedec_id(dev, jedec_id);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    dev->manufacturer_id = jedec_id[0];
    dev->memory_type = jedec_id[1];
    dev->memory_capacity = jedec_id[2];
    
    /* 根据容量配置参数 */
    switch (jedec_id[2]) {
        case 0x15: /* W25Q16 */
            dev->capacity_bytes = 2 * 1024 * 1024;
            dev->sector_size = 4; /* KB */
            dev->page_size = 256;
            dev->block_count = 32;
            dev->sector_count = 512;
            break;
        case 0x16: /* W25Q32 */
            dev->capacity_bytes = 4 * 1024 * 1024;
            dev->sector_size = 4;
            dev->page_size = 256;
            dev->block_count = 64;
            dev->sector_count = 1024;
            break;
        case 0x17: /* W25Q64 */
            dev->capacity_bytes = 8 * 1024 * 1024;
            dev->sector_size = 4;
            dev->page_size = 256;
            dev->block_count = 128;
            dev->sector_count = 2048;
            break;
        case 0x18: /* W25Q128 */
            dev->capacity_bytes = 16 * 1024 * 1024;
            dev->sector_size = 4;
            dev->page_size = 256;
            dev->block_count = 256;
            dev->sector_count = 4096;
            break;
        default:
            return XY_DEVICE_IO_ERROR;
    }
    
    /* 退出掉电模式 */
    xy_w25qxx_release_powerdown(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_deinit(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 进入掉电模式 */
    xy_w25qxx_powerdown(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_read_device_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id, 
                             uint8_t *device_id)
{
    if (!dev || !manufacturer_id || !device_id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q_CMD_DEVICE_ID;
    uint8_t addr[3] = {0, 0, 0};
    uint8_t rx[2];
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    spi_transfer(dev, addr, NULL, 3);
    spi_transfer(dev, NULL, rx, 2);
    cs_disable(dev);
    
    *manufacturer_id = rx[0];
    *device_id = rx[1];
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_read_jedec_id(xy_w25qxx_t *dev, uint8_t *jedec_id)
{
    if (!dev || !jedec_id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q_CMD_JEDEC_ID;
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    spi_transfer(dev, NULL, jedec_id, 3);
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_read_status_reg1(xy_w25qxx_t *dev, uint8_t *status)
{
    if (!dev || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q_CMD_READ_STATUS_REG1;
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    spi_transfer(dev, NULL, status, 1);
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_wait_idle(xy_w25qxx_t *dev, uint32_t timeout_ms)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t status;
    uint32_t start = xy_hal_get_tick();  /* 获取开始时间 */
    
    do {
        xy_w25qxx_read_status_reg1(dev, &status);
        
        if (!(status & W25Q_STATUS_BUSY)) {
            return XY_DEVICE_OK;
        }
        
        /* 简单延迟 */
        for (volatile int i = 0; i < 1000; i++);
        
    } while ((xy_hal_get_tick() - start) < timeout);  /* 检查超时 */
    
    return XY_DEVICE_TIMEOUT;
}

int xy_w25qxx_write_enable(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    cs_disable(dev);
    
    dev->write_enabled = true;
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_page_program(xy_w25qxx_t *dev, uint32_t addr, 
                           const uint8_t *data, uint16_t length)
{
    if (!dev || !data || length == 0 || length > 256) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 写使能 */
    xy_w25qxx_write_enable(dev);
    
    /* 发送命令和地址 */
    send_command_addr(dev, W25Q_CMD_PAGE_PROGRAM, addr);
    
    /* 发送数据 */
    spi_transfer(dev, data, NULL, length);
    
    cs_disable(dev);
    
    /* 等待完成 */
    return xy_w25qxx_wait_idle(dev, 100);
}

int xy_w25qxx_sector_erase(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 写使能 */
    xy_w25qxx_write_enable(dev);
    
    /* 发送命令和地址 */
    send_command_addr(dev, W25Q_CMD_SECTOR_ERASE, addr);
    
    cs_disable(dev);
    
    /* 等待完成 (扇区擦除约 45ms) */
    return xy_w25qxx_wait_idle(dev, 1000);
}

int xy_w25qxx_block_erase_32k(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    send_command_addr(dev, W25Q_CMD_BLOCK_ERASE_32K, addr);
    cs_disable(dev);
    
    return xy_w25qxx_wait_idle(dev, 1000);
}

int xy_w25qxx_block_erase_64k(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    send_command_addr(dev, W25Q_CMD_BLOCK_ERASE_64K, addr);
    cs_disable(dev);
    
    return xy_w25qxx_wait_idle(dev, 1000);
}

int xy_w25qxx_chip_erase(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_w25qxx_write_enable(dev);
    
    uint8_t cmd = W25Q_CMD_CHIP_ERASE;
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    cs_disable(dev);
    
    /* 整片擦除时间较长 (W25Q128 约 30 秒) */
    return xy_w25qxx_wait_idle(dev, 30000);
}

int xy_w25qxx_read(xy_w25qxx_t *dev, uint32_t addr, 
                   uint8_t *data, uint32_t length)
{
    if (!dev || !data || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 发送快速读取命令 */
    send_command_addr(dev, W25Q_CMD_FAST_READ, addr);
    
    /* 发送 1 字节 dummy */
    uint8_t dummy = 0;
    spi_transfer(dev, &dummy, NULL, 1);
    
    /* 读取数据 */
    spi_transfer(dev, NULL, data, length);
    
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_write(xy_w25qxx_t *dev, uint32_t addr, 
                    const uint8_t *data, uint32_t length)
{
    if (!dev || !data || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint32_t remaining = length;
    uint32_t offset = 0;
    
    while (remaining > 0) {
        /* 计算当前页剩余空间 */
        uint32_t page_offset = addr % dev->page_size;
        uint32_t page_space = dev->page_size - page_offset;
        uint32_t write_size = (remaining < page_space) ? remaining : page_space;
        
        /* 页编程 */
        int ret = xy_w25qxx_page_program(dev, addr, data + offset, write_size);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        
        addr += write_size;
        offset += write_size;
        remaining -= write_size;
    }
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_powerdown(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = 0xB9; /* Power-down command */
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_release_powerdown(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd = W25Q_CMD_RELEASE_POWERDOWN;
    
    cs_enable(dev);
    spi_transfer(dev, &cmd, NULL, 1);
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

/* ==================== End of File ==================== */
