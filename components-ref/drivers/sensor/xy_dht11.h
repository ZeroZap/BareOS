/**
 * @file xy_dht11.h
 * @brief DHT11/DHT22 Temperature and Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 单总线温湿度传感器驱动
 */

#ifndef XY_DHT11_H
#define XY_DHT11_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== DHT11 Device Structure ==================== */

/**
 * @brief DHT11 设备结构
 */
typedef struct {
    xy_device_t base;              /**< 设备基类 */
    void *gpio_handle;             /**< GPIO 句柄 */
    uint32_t gpio_pin;             /**< GPIO 引脚 */
    uint8_t type;                  /**< 传感器类型：1=DHT11, 2=DHT22 */
    uint32_t timeout_ms;           /**< 超时时间 */
} xy_dht11_t;

/* ==================== DHT11 Data Structure ==================== */

/**
 * @brief DHT11 数据
 */
typedef struct {
    float humidity;                /**< 湿度 (%RH) */
    float temperature;             /**< 温度 (°C) */
    uint8_t humidity_int;          /**< 湿度整数部分 */
    uint8_t humidity_dec;          /**< 湿度小数部分 */
    uint8_t temp_int;              /**< 温度整数部分 */
    uint8_t temp_dec;              /**< 温度小数部分 */
    uint8_t checksum;              /**< 校验和 */
} xy_dht11_data_t;

/* ==================== DHT11 API ==================== */

/**
 * @brief 初始化 DHT11
 * @param dev DHT11 设备句柄
 * @param gpio_handle GPIO 句柄
 * @param gpio_pin GPIO 引脚
 * @param type 传感器类型 (1=DHT11, 2=DHT22)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_dht11_init(xy_dht11_t *dev, void *gpio_handle, 
                  uint32_t gpio_pin, uint8_t type);

/**
 * @brief 反初始化 DHT11
 * @param dev DHT11 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_dht11_deinit(xy_dht11_t *dev);

/**
 * @brief 读取温湿度数据
 * @param dev DHT11 设备句柄
 * @param data 数据输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_dht11_read(xy_dht11_t *dev, xy_dht11_data_t *data);

/**
 * @brief 读取温度 (°C)
 * @param dev DHT11 设备句柄
 * @param temp 温度值 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_dht11_read_temperature(xy_dht11_t *dev, float *temp);

/**
 * @brief 读取湿度 (%RH)
 * @param dev DHT11 设备句柄
 * @param humidity 湿度值 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_dht11_read_humidity(xy_dht11_t *dev, float *humidity);

/**
 * @brief 校验数据
 * @param data DHT11 数据
 * @return XY_DEVICE_OK 校验通过，其他值失败
 */
int xy_dht11_verify_checksum(xy_dht11_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* XY_DHT11_H */
