/**
 * @file xy_hx711.c
 * @brief HX711 24-bit ADC for Weight Scale
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_adc_ext.h"
#include "xy_hal_gpio.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 读取 24 位数据
 */
static uint32_t xy_hx711_read_data(xy_hx711_t *hx)
{
    uint32_t data = 0;
    uint8_t i;
    
    for (i = 0; i < 24; i++) {
        xy_hal_gpio_set(hx->pd_sck_pin, 1);
        data = (data << 1) | xy_hal_gpio_read(hx->dout_pin);
        xy_hal_gpio_set(hx->pd_sck_pin, 0);
    }
    
    /* 额外脉冲选择增益 */
    for (i = 0; i < hx->gain; i++) {
        xy_hal_gpio_set(hx->pd_sck_pin, 1);
        xy_hal_gpio_set(hx->pd_sck_pin, 0);
    }
    
    /* 24 位补码转有符号数 */
    data ^= 0x800000;
    
    return data;
}

int xy_hx711_init(xy_hx711_t *hx, uint8_t pd_sck_pin, uint8_t dout_pin)
{
    xy_hal_gpio_config_t gpio_cfg;
    
    if (!hx) {
        return -1;
    }
    
    hx->pd_sck_pin = pd_sck_pin;
    hx->dout_pin = dout_pin;
    hx->gain = 128;  /* 默认 128 增益 */
    hx->offset = 0;
    hx->scale = 1.0f;
    
    /* 初始化 PD_SCK (输出) */
    gpio_cfg.mode = XY_HAL_GPIO_MODE_OUTPUT;
    gpio_cfg.pull = XY_HAL_GPIO_PULL_NONE;
    xy_hal_gpio_init(pd_sck_pin, &gpio_cfg);
    xy_hal_gpio_set(pd_sck_pin, 0);
    
    /* 初始化 DOUT (输入) */
    gpio_cfg.mode = XY_HAL_GPIO_MODE_INPUT;
    gpio_cfg.pull = XY_HAL_GPIO_PULL_UP;
    xy_hal_gpio_init(dout_pin, &gpio_cfg);
    
    xy_os_delay(100);  /* 等待 HX711 稳定 */
    
    /* 读取一次以设置增益 */
    xy_hx711_read(hx, NULL);
    
    xy_log_i("HX711 initialized (PD_SCK=%d, DOUT=%d, Gain=%d)\n",
             pd_sck_pin, dout_pin, hx->gain);
    return 0;
}

int xy_hx711_read(xy_hx711_t *hx, int32_t *value)
{
    uint32_t count = 0;
    uint32_t timeout = 1000;
    
    if (!hx) {
        return -1;
    }
    
    /* 等待数据就绪 */
    while (xy_hal_gpio_read(hx->dout_pin) && count++ < timeout) {
        xy_os_delay(1);
    }
    
    if (count >= timeout) {
        return -1;  /* 超时 */
    }
    
    /* 读取 24 位数据 */
    uint32_t data = xy_hx711_read_data(hx);
    
    if (value) {
        *value = (int32_t)data - hx->offset;
    }
    
    return 0;
}

int xy_hx711_tare(xy_hx711_t *hx)
{
    int32_t sum = 0;
    uint8_t i;
    
    if (!hx) {
        return -1;
    }
    
    /* 读取多次取平均 */
    for (i = 0; i < 32; i++) {
        int32_t value;
        xy_hx711_read(hx, &value);
        sum += value;
        xy_os_delay(10);
    }
    
    hx->offset = sum / 32;
    
    xy_log_i("HX711 tared (offset=%ld)\n", hx->offset);
    return 0;
}

int xy_hx711_calibrate(xy_hx711_t *hx, float known_weight)
{
    int32_t value;
    
    if (!hx || known_weight <= 0) {
        return -1;
    }
    
    /* 读取当前值 */
    xy_hx711_read(hx, &value);
    value -= hx->offset;
    
    /* 计算比例因子 */
    hx->scale = known_weight / value;
    
    xy_log_i("HX711 calibrated (scale=%.6f)\n", hx->scale);
    return 0;
}

float xy_hx711_get_weight(xy_hx711_t *hx)
{
    int32_t value;
    
    if (xy_hx711_read(hx, &value) != 0) {
        return 0.0f;
    }
    
    value -= hx->offset;
    return value * hx->scale;
}
