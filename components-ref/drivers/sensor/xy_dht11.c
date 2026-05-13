/**
 * @file xy_dht11.c
 * @brief DHT11/DHT22 Temperature and Humidity Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 单总线温湿度传感器驱动实现
 * 
 * DHT11 时序:
 * - 主机启动信号：拉低总线 18ms，然后拉高 20-40us
 * - 传感器响应：拉低总线 80us，然后拉高 80us
 * - 数据传送：每个 bit 50us 低电平 + 高电平持续时间表示 0 或 1
 *   - 0: 高电平 26-28us
 *   - 1: 高电平 70us
 * 
 * DHT22 时序类似，但数据格式不同
 */

#include "xy_dht11.h"
#include "xy_hal_gpio.h"
#include "xy_os.h"
#include <string.h>

/* ==================== Private Functions ==================== */

/**
 * @brief 微秒级延迟
 * @param us 延迟微秒数
 */
static void delay_us(uint32_t us)
{
    xy_os_time_delay_us(us);
}

/**
 * @brief 毫秒级延迟
 * @param ms 延迟毫秒数
 */
static void delay_ms(uint32_t ms)
{
    xy_os_time_delay_ms(ms);
}

/**
 * @brief 读取 GPIO 电平
 * @param dev DHT11 设备指针
 * @return int 0=低电平，1=高电平
 */
static int read_bit(xy_dht11_t *dev)
{
    if (!dev || !dev->gpio_handle) {
        return 0;
    }
    
    xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->gpio_handle;
    return xy_hal_gpio_read(gpio, dev->gpio_pin);
}

/**
 * @brief 写入 GPIO 电平
 * @param dev DHT11 设备指针
 * @param value 0=低电平，1=高电平
 */
static void write_bit(xy_dht11_t *dev, int value)
{
    if (!dev || !dev->gpio_handle) {
        return;
    }
    
    xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->gpio_handle;
    xy_hal_gpio_write(gpio, dev->gpio_pin, value ? 1 : 0);
}

/**
 * @brief 配置 GPIO 为输出
 * @param dev DHT11 设备指针
 * @param open_drain 1=开漏，0=推挽
 */
static void gpio_config_output(xy_dht11_t *dev, int open_drain)
{
    if (!dev || !dev->gpio_handle) {
        return;
    }
    
    xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->gpio_handle;
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = open_drain ? XY_HAL_GPIO_PULL_UP : XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
        .open_drain = open_drain ? 1 : 0
    };
    xy_hal_gpio_configure(gpio, dev->gpio_pin, &cfg);
}

/**
 * @brief 配置 GPIO 为输入
 * @param dev DHT11 设备指针
 */
static void gpio_config_input(xy_dht11_t *dev)
{
    if (!dev || !dev->gpio_handle) {
        return;
    }
    
    xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->gpio_handle;
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_INPUT,
        .pull = XY_HAL_GPIO_PULL_UP,
        .speed = XY_HAL_GPIO_SPEED_HIGH
    };
    xy_hal_gpio_configure(gpio, dev->gpio_pin, &cfg);
}

/**
 * @brief 等待电平变化
 * @param dev DHT11 设备指针
 * @param level 目标电平 (0=低，1=高)
 * @param timeout_us 超时时间 (微秒)
 * @return int 等待时间 (微秒)，-1=超时
 */
static int wait_level(xy_dht11_t *dev, uint8_t level, uint32_t timeout_us)
{
    uint32_t count = 0;
    
    while (read_bit(dev) != level) {
        delay_us(1);
        count++;
        if (count > timeout_us) {
            return -1; /* 超时 */
        }
    }
    
    return count;
}

/* ==================== Public Implementation ==================== */

int xy_dht11_init(xy_dht11_t *dev, void *gpio_handle, 
                  uint32_t gpio_pin, uint8_t type)
{
    if (!dev || !gpio_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->gpio_handle = gpio_handle;
    dev->gpio_pin = gpio_pin;
    dev->type = type;
    dev->timeout_ms = 100;
    
    /* 配置 GPIO 为开漏输出 (单总线需要) */
    gpio_config_output(dev, 1);
    
    return XY_DEVICE_OK;
}

int xy_dht11_deinit(xy_dht11_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 释放 GPIO 资源 (配置为浮空输入) */
    if (dev->gpio_handle) {
        xy_hal_gpio_t gpio = (xy_hal_gpio_t)dev->gpio_handle;
        xy_hal_gpio_config_t cfg = {
            .mode = XY_HAL_GPIO_MODE_INPUT,
            .pull = XY_HAL_GPIO_PULL_NONE
        };
        xy_hal_gpio_configure(gpio, dev->gpio_pin, &cfg);
    }
    
    return XY_DEVICE_OK;
}

int xy_dht11_read(xy_dht11_t *dev, xy_dht11_data_t *data)
{
    if (!dev || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(data, 0, sizeof(*data));
    
    /* 1. 主机启动信号 */
    gpio_config_output(dev, 1);  /* 开漏输出 */
    write_bit(dev, 0);  /* 拉低 GPIO 18ms */
    delay_ms(18);
    
    write_bit(dev, 1);  /* 拉高 GPIO 20-40us */
    delay_us(30);
    
    /* 2. 等待传感器响应 */
    gpio_config_input(dev);  /* 配置为输入 */
    
    /* 等待传感器拉低总线 80us */
    if (wait_level(dev, 0, 100) < 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    /* 等待传感器拉高总线 80us */
    if (wait_level(dev, 1, 100) < 0) {
        return XY_DEVICE_TIMEOUT;
    }
    
    /* 3. 读取 40 bit 数据 */
    /* 数据格式：
     * - DHT11: 湿度整数 (8bit) + 湿度小数 (8bit) + 温度整数 (8bit) + 温度小数 (8bit) + 校验和 (8bit)
     * - DHT22: 湿度 (16bit) + 温度 (16bit) + 校验和 (8bit)
     */
    
    uint8_t bits[5] = {0};
    
    for (int i = 0; i < 40; i++) {
        /* 等待低电平结束 */
        if (wait_level(dev, 1, 100) < 0) {
            return XY_DEVICE_TIMEOUT;
        }
        
        /* 读取高电平持续时间 */
        uint32_t high_time = wait_level(dev, 0, 100);
        
        /* 判断是 0 还是 1 */
        /* DHT11: 26-28us 表示 0, 70us 表示 1 */
        if (high_time > 50) {
            bits[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    
    /* 4. 解析数据 */
    if (dev->type == 1) {
        /* DHT11 */
        data->humidity_int = bits[0];
        data->humidity_dec = bits[1];
        data->temp_int = bits[2];
        data->temp_dec = bits[3];
        data->checksum = bits[4];
        
        data->humidity = (float)bits[0] + (float)bits[1] / 10.0f;
        data->temperature = (float)bits[2] + (float)bits[3] / 10.0f;
    } else {
        /* DHT22 */
        int16_t humidity_raw = (bits[0] << 8) | bits[1];
        int16_t temp_raw = (bits[2] << 8) | bits[3];
        data->checksum = bits[4];
        
        data->humidity = (float)humidity_raw / 10.0f;
        data->temperature = (float)temp_raw / 10.0f;
    }
    
    /* 5. 校验和验证 */
    if (xy_dht11_verify_checksum(data) != XY_DEVICE_OK) {
        return XY_DEVICE_IO_ERROR;
    }
    
    return XY_DEVICE_OK;
}

int xy_dht11_read_temperature(xy_dht11_t *dev, float *temp)
{
    if (!dev || !temp) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_dht11_data_t data;
    int ret = xy_dht11_read(dev, &data);
    
    if (ret == XY_DEVICE_OK) {
        *temp = data.temperature;
    }
    
    return ret;
}

int xy_dht11_read_humidity(xy_dht11_t *dev, float *humidity)
{
    if (!dev || !humidity) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_dht11_data_t data;
    int ret = xy_dht11_read(dev, &data);
    
    if (ret == XY_DEVICE_OK) {
        *humidity = data.humidity;
    }
    
    return ret;
}

int xy_dht11_verify_checksum(xy_dht11_data_t *data)
{
    if (!data) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t checksum = data->humidity_int + data->humidity_dec + 
                       data->temp_int + data->temp_dec;
    
    if (checksum == data->checksum) {
        return XY_DEVICE_OK;
    }
    
    return XY_DEVICE_IO_ERROR;
}

/* ==================== End of File ==================== */
