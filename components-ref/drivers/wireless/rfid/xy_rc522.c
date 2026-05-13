/**
 * @file xy_rc522.c
 * @brief MFRC522 RFID Reader Driver Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 MFRC522/RC522 RFID 读卡器 (SPI 接口)
 */

#include "xy_rc522.h"
#include "xy_hal_spi.h"
#include "xy_hal_gpio.h"
#include "xy_os.h"
#include <string.h>

/* ==================== Private Definitions ==================== */

/* 寄存器地址 */
#define RC522_REG_PAGE0         0x00
#define RC522_REG_COMMAND       0x01
#define RC522_REG_COM_I_EN      0x02
#define RC522_REG_DIV_I_EN      0x03
#define RC522_REG_COM_IRQ       0x04
#define RC522_REG_DIV_IRQ       0x05
#define RC522_REG_ERROR         0x06
#define RC522_REG_STATUS2       0x08
#define RC522_REG_FIFO_DATA     0x09
#define RC522_REG_FIFO_LEVEL    0x0A
#define RC522_REG_WATER_LEVEL   0x0B
#define RC522_REG_CONTROL       0x0C
#define RC522_REG_BIT_FRAMING   0x0D
#define RC522_REG_COLL          0x0E

#define RC522_REG_PAGE1         0x10
#define RC522_REG_MODE          0x11
#define RC522_REG_TX_MODE       0x12
#define RC522_REG_RX_MODE       0x13
#define RC522_REG_TX_CONTROL    0x14
#define RC522_REG_TX_AUTO       0x15
#define RC522_REG_TX_SEL        0x16
#define RC522_REG_RX_SEL        0x17
#define RC522_REG_RX_THRESHOLD  0x18
#define RC522_REG_DEMOD         0x19

#define RC522_REG_PAGE2         0x20
#define RC522_REG_CRC_RESULT_1  0x21
#define RC522_REG_CRC_RESULT_2  0x22
#define RC522_REG_MOD_WIDTH     0x24
#define RC522_REG_GSN           0x28
#define RC522_REG_CFG           0x2A
#define RC522_REG_T_MODE        0x2B
#define RC522_REG_T_PRESCALER   0x2C
#define RC522_REG_T_RELOAD_1    0x2D
#define RC522_REG_T_RELOAD_2    0x2E
#define RC522_REG_T_COUNTER_1   0x2F
#define RC522_REG_T_COUNTER_2   0x30

#define RC522_REG_TEST_ADC      0x37

/* 命令 */
#define RC522_CMD_IDLE          0x00
#define RC522_CMD_MEM           0x01
#define RC522_CMD_GENERATE_RANDOM_ID 0x02
#define RC522_CMD_CRC           0x03
#define RC522_CMD_TRANSMIT      0x04
#define RC522_CMD_NO_CMD_CHANGE 0x07
#define RC522_CMD_RECEIVE       0x08
#define RC522_CMD_TRANSCEIVE    0x0C
#define RC522_CMD_AUTHENT       0x0E
#define RC522_CMD_SOFT_RESET    0x0F

/* 状态位 */
#define RC522_STATUS_RX_WAIT    0x08
#define RC522_STATUS_TX_LAST_BITS 0x07

/* MIFARE 命令 */
#define MIFARE_CMD_AUTH_A       0x60
#define MIFARE_CMD_AUTH_B       0x61
#define MIFARE_CMD_READ         0x30
#define MIFARE_CMD_WRITE        0xA0

/* ==================== Private Types ==================== */

/**
 * @brief RC522 私有数据
 */
typedef struct {
    xy_rc522_t base;
    uint8_t irq_pin;
    bool card_present;
} rc522_private_t;

/* ==================== Private Functions ==================== */

/**
 * @brief 写入寄存器
 * @param dev RC522 设备指针
 * @param addr 寄存器地址
 * @param value 写入值
 * @return XY_DEVICE_OK 成功，其他值失败
 */
static int write_reg(xy_rc522_t *dev, uint8_t addr, uint8_t value)
{
    if (!dev || !dev->spi_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_hal_spi_t spi = (xy_hal_spi_t)dev->spi_handle;
    
    /* RC522 SPI 写：最高位为 0，地址在低 7 位 */
    uint8_t tx_buf[2] = {(addr << 1) & 0x7E, value};
    int32_t ret = xy_hal_spi_transfer(spi, tx_buf, NULL, 2, 100);
    
    return (ret > 0) ? XY_DEVICE_OK : XY_DEVICE_ERROR;
}

/**
 * @brief 读取寄存器
 * @param dev RC522 设备指针
 * @param addr 寄存器地址
 * @param value 读取值 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
static int read_reg(xy_rc522_t *dev, uint8_t addr, uint8_t *value)
{
    if (!dev || !dev->spi_handle || !value) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_hal_spi_t spi = (xy_hal_spi_t)dev->spi_handle;
    
    /* RC522 SPI 读：最高位为 1，地址在低 7 位 */
    uint8_t tx_buf[2] = {((addr << 1) & 0x7E) | 0x80, 0x00};
    uint8_t rx_buf[2] = {0};
    
    int32_t ret = xy_hal_spi_transfer(spi, tx_buf, rx_buf, 2, 100);
    if (ret > 0) {
        *value = rx_buf[1];
        return XY_DEVICE_OK;
    }
    
    return XY_DEVICE_ERROR;
}

/**
 * @brief 设置位掩码
 */
static int set_bit_mask(xy_rc522_t *dev, uint8_t reg, uint8_t mask)
{
    uint8_t value;
    int ret = read_reg(dev, reg, &value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    return write_reg(dev, reg, value | mask);
}

/**
 * @brief 清除位掩码
 */
static int clear_bit_mask(xy_rc522_t *dev, uint8_t reg, uint8_t mask)
{
    uint8_t value;
    int ret = read_reg(dev, reg, &value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    return write_reg(dev, reg, value & ~mask);
}

/**
 * @brief 执行命令
 */
static int execute_command(xy_rc522_t *dev, uint8_t cmd)
{
    return write_reg(dev, RC522_REG_COMMAND, cmd);
}

/**
 * @brief 延迟毫秒
 * @param ms 延迟毫秒数
 */
static void delay_ms(uint32_t ms)
{
    xy_os_time_delay_ms(ms);
}

/**
 * @brief 延迟微秒
 * @param us 延迟微秒数
 */
static void delay_us(uint32_t us)
{
    xy_os_time_delay_us(us);
}

/**
 * @brief 拉低复位 GPIO
 * @param dev RC522 设备指针
 */
static void rst_gpio_low(xy_rc522_t *dev)
{
    if (dev && dev->rst_gpio) {
        xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->rst_gpio;
        xy_hal_gpio_write(gpio, XY_HAL_GPIO_PIN_ALL, 0);
    }
}

/**
 * @brief 拉高复位 GPIO
 * @param dev RC522 设备指针
 */
static void rst_gpio_high(xy_rc522_t *dev)
{
    if (dev && dev->rst_gpio) {
        xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->rst_gpio;
        xy_hal_gpio_write(gpio, XY_HAL_GPIO_PIN_ALL, 1);
    }
}

/* ==================== Public Implementation ==================== */

int xy_rc522_init(xy_rc522_t *dev, void *spi_handle, void *rst_gpio)
{
    if (!dev || !spi_handle || !rst_gpio) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->spi_handle = spi_handle;
    dev->rst_gpio = rst_gpio;
    dev->initialized = false;
    
    /* 硬件复位 */
    rst_gpio_low(dev);
    delay_ms(10);
    rst_gpio_high(dev);
    delay_ms(10);
    
    /* 软复位 */
    int ret = execute_command(dev, RC522_CMD_SOFT_RESET);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    delay_ms(10);
    
    /* 配置定时器 */
    write_reg(dev, RC522_REG_T_MODE, 0x8D);
    write_reg(dev, RC522_REG_T_PRESCALER, 0x3E);
    write_reg(dev, RC522_REG_T_RELOAD_1, 0x00);
    write_reg(dev, RC522_REG_T_RELOAD_2, 0x1E);
    
    /* 配置 TX 自动发送 CRC */
    write_reg(dev, RC522_REG_TX_AUTO, 0x40);
    write_reg(dev, RC522_REG_MODE, 0x3D);
    
    /* 配置接收器 */
    clear_bit_mask(dev, RC522_REG_RX_MODE, 0x80);
    set_bit_mask(dev, RC522_REG_TX_CONTROL, 0x03);
    
    /* 读取固件版本 */
    ret = xy_rc522_get_version(dev, &dev->firmware_version);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 验证版本 (应该是 0x91 或 0x92) */
    if (dev->firmware_version != 0x91 && dev->firmware_version != 0x92) {
        return XY_DEVICE_IO_ERROR;
    }
    
    dev->initialized = true;
    
    return XY_DEVICE_OK;
}

int xy_rc522_deinit(xy_rc522_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 进入掉电模式 */
    xy_rc522_powerdown(dev);
    
    dev->initialized = false;
    
    return XY_DEVICE_OK;
}

int xy_rc522_reset(xy_rc522_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 软复位 */
    int ret = execute_command(dev, RC522_CMD_SOFT_RESET);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    delay_ms(10);
    
    /* 重新初始化 */
    return xy_rc522_init(dev, dev->spi_handle, dev->rst_gpio);
}

int xy_rc522_get_version(xy_rc522_t *dev, uint8_t *version)
{
    if (!dev || !version) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return read_reg(dev, RC522_REG_GSN, version);
}

int xy_rc522_detect_card(xy_rc522_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 清除 IRQ 标志 */
    write_reg(dev, RC522_REG_COM_IRQ, 0x80);
    
    /* 发送请求命令 */
    uint8_t cmd = 0x26; /* REQA */
    write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    write_reg(dev, RC522_REG_FIFO_DATA, cmd);
    execute_command(dev, RC522_CMD_TRANSMIT);
    
    /* 等待完成 */
    uint8_t irq;
    int timeout = 1000;
    do {
        read_reg(dev, RC522_REG_COM_IRQ, &irq);
        timeout--;
    } while (timeout > 0 && !(irq & 0x10));
    
    if (timeout <= 0) {
        return XY_DEVICE_NOT_FOUND;
    }
    
    /* 检查错误 */
    uint8_t error;
    read_reg(dev, RC522_REG_ERROR, &error);
    if (error & 0x13) {
        return XY_DEVICE_NOT_FOUND;
    }
    
    /* 检查是否有卡 */
    uint8_t fifo_level;
    read_reg(dev, RC522_REG_FIFO_LEVEL, &fifo_level);
    if (fifo_level != 2) {
        return XY_DEVICE_NOT_FOUND;
    }
    
    /* 读取卡类型 */
    uint8_t card_type[2];
    read_reg(dev, RC522_REG_FIFO_DATA, &card_type[0]);
    read_reg(dev, RC522_REG_FIFO_DATA, &card_type[1]);
    
    /* 识别卡类型 */
    if (card_type[0] == 0x04 && card_type[1] == 0x00) {
        /* MIFARE 卡 */
        return XY_DEVICE_OK;
    }
    
    return XY_DEVICE_NOT_FOUND;
}

int xy_rc522_select_card(xy_rc522_t *dev, xy_rc522_uid_t *uid)
{
    if (!dev || !uid) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 防冲突命令 */
    uint8_t cmd[2] = {0x93, 0x20};
    
    write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    write_reg(dev, RC522_REG_FIFO_DATA, cmd[0]);
    write_reg(dev, RC522_REG_FIFO_DATA, cmd[1]);
    execute_command(dev, RC522_CMD_TRANSMIT);
    
    /* 等待完成 */
    uint8_t irq;
    int timeout = 1000;
    do {
        read_reg(dev, RC522_REG_COM_IRQ, &irq);
        timeout--;
    } while (timeout > 0 && !(irq & 0x10));
    
    if (timeout <= 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    /* 读取 UID */
    uint8_t fifo_level;
    read_reg(dev, RC522_REG_FIFO_LEVEL, &fifo_level);
    
    if (fifo_level < 4) {
        return XY_DEVICE_IO_ERROR;
    }
    
    uid->uid_len = fifo_level - 2; /* 减去 CRC 字节 */
    for (int i = 0; i < uid->uid_len && i < 10; i++) {
        read_reg(dev, RC522_REG_FIFO_DATA, &uid->uid[i]);
    }
    
    /* 读取 CRC */
    uint8_t crc[2];
    read_reg(dev, RC522_REG_FIFO_DATA, &crc[0]);
    read_reg(dev, RC522_REG_FIFO_DATA, &crc[1]);
    
    /* 识别卡类型 */
    switch (uid->uid_len) {
        case 4:
            uid->card_type = XY_RC522_CARD_MIFARE_1K;
            break;
        case 7:
            uid->card_type = XY_RC522_CARD_MIFARE_MINI;
            break;
        case 10:
            uid->card_type = XY_RC522_CARD_MIFARE_4K;
            break;
        default:
            uid->card_type = XY_RC522_CARD_UNKNOWN;
    }
    
    return XY_DEVICE_OK;
}

int xy_rc522_mifare_read(xy_rc522_t *dev, uint8_t block_addr, 
                         uint8_t *data, uint8_t length)
{
    if (!dev || !data || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* MIFARE 读命令 */
    uint8_t cmd = MIFARE_CMD_READ;
    
    write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    write_reg(dev, RC522_REG_FIFO_DATA, cmd);
    write_reg(dev, RC522_REG_FIFO_DATA, block_addr);
    
    /* 计算 CRC (使用硬件 CRC) */
    write_reg(dev, RC522_REG_COMMAND, RC522_CMD_CRC);
    delay_ms(1);  /* 等待 CRC 计算完成 */
    
    execute_command(dev, RC522_CMD_TRANSMIT);
    
    /* 等待完成 */
    uint8_t irq;
    int timeout = 1000;
    do {
        read_reg(dev, RC522_REG_COM_IRQ, &irq);
        timeout--;
    } while (timeout > 0 && !(irq & 0x10));
    
    if (timeout <= 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    /* 读取数据 */
    uint8_t fifo_level;
    read_reg(dev, RC522_REG_FIFO_LEVEL, &fifo_level);
    
    for (int i = 0; i < length && i < fifo_level; i++) {
        read_reg(dev, RC522_REG_FIFO_DATA, &data[i]);
    }
    
    return XY_DEVICE_OK;
}

int xy_rc522_mifare_write(xy_rc522_t *dev, uint8_t block_addr, 
                          const uint8_t *data, uint8_t length)
{
    if (!dev || !data || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* MIFARE 写命令 */
    uint8_t cmd = MIFARE_CMD_WRITE;
    
    write_reg(dev, RC522_REG_FIFO_LEVEL, 0x80);
    write_reg(dev, RC522_REG_FIFO_DATA, cmd);
    write_reg(dev, RC522_REG_FIFO_DATA, block_addr);
    
    /* 计算 CRC (使用硬件 CRC) */
    write_reg(dev, RC522_REG_COMMAND, RC522_CMD_CRC);
    delay_ms(1);  /* 等待 CRC 计算完成 */
    
    execute_command(dev, RC522_CMD_TRANSMIT);
    
    /* 等待响应 */
    uint8_t irq;
    int timeout = 1000;
    do {
        read_reg(dev, RC522_REG_COM_IRQ, &irq);
        timeout--;
    } while (timeout > 0 && !(irq & 0x10));
    
    if (timeout <= 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    /* 发送数据 */
    for (int i = 0; i < length; i++) {
        write_reg(dev, RC522_REG_FIFO_DATA, data[i]);
    }
    
    execute_command(dev, RC522_CMD_TRANSMIT);
    
    /* 等待完成 */
    timeout = 1000;
    do {
        read_reg(dev, RC522_REG_COM_IRQ, &irq);
        timeout--;
    } while (timeout > 0 && !(irq & 0x10));
    
    if (timeout <= 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    return XY_DEVICE_OK;
}

int xy_rc522_idle(xy_rc522_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return execute_command(dev, RC522_CMD_IDLE);
}

int xy_rc522_powerdown(xy_rc522_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 进入掉电模式 */
    clear_bit_mask(dev, RC522_REG_CONTROL, 0x01);
    
    return XY_DEVICE_OK;
}

/* ==================== End of File ==================== */
