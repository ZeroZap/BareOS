# XY HAL STM32U5 实现

## 概述

本目录包含 XinYi 硬件抽象层 (HAL) 的 STM32U5 系列微控制器实现。

## 架构说明

```
┌─────────────────────────────────────────┐
│           应用层 (Projects)              │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│  HAL 实现层 (components/hal/stm32/u5/)   │
│  xy_hal_*.c (平台相关实现)               │
│  stm32u5_platform.h (平台宏定义)         │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│        HAL 接口层 (components/hal/inc/)  │
│  xy_hal_*.h (平台无关接口)               │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│      MCU SDK 层 (MCU/ST/STM32U5/)        │
│  stm32u5xx_hal.h (STM 官方 HAL 库)        │
└─────────────────────────────────────────┘
```

**注意**: 
- `stm32u5_platform.h` 仅包含平台特定宏定义，不包含 STM32 HAL 库
- STM32 HAL 库来自 `MCU/ST/STM32U5/` 目录，需在项目构建配置中包含

## 支持的 peripherals

| 模块 | 文件 | 状态 |
|------|------|------|
| GPIO/Pin | xy_hal_pin.c | ✅ 完成 |
| UART | xy_hal_uart.c | ✅ 完成 |
| SPI | xy_hal_spi.c | ✅ 完成 |
| I2C | xy_hal_i2c.c | ✅ 完成 |
| Timer | xy_hal_timer.c | ✅ 完成 |
| PWM | xy_hal_pwm.c | ✅ 完成 |
| RTC | xy_hal_rtc.c | ✅ 完成 |
| DMA | xy_hal_dma.c | ✅ 完成 |
| ADC | xy_hal_adc.c | ✅ 完成 |
| DAC | xy_hal_dac.c | ✅ 完成 |
| Flash | xy_hal_flash.c | ✅ 完成 |
| Watchdog | xy_hal_wdg.c | ✅ 完成 |
| RNG | xy_hal_rng.c | ✅ 完成 |
| EXTI | xy_hal_exti.c | ✅ 完成 |
| Low Power Timer | xy_hal_lp_timer.c | ✅ 完成 |
| Time-Sensitive GPIO | xy_hal_tgpio.c | ✅ 完成 |
| IR | xy_hal_ir.c | ✅ 完成 |
| I2S | xy_hal_i2s.c | ✅ 完成 |
| CAN | xy_hal_can.c | ✅ 完成 |

## 构建

### 使用 CMake

```bash
mkdir build && cd build
cmake .. -DSTM32_FAMILY=STM32U5 -DSTM32_DEVICE=STM32U5xx
make
```

### 使用 Make

```bash
make clean
make all
```

## 依赖

- STM32U5 HAL 库 (stm32u5xx_hal)
- CMSIS-CORE for STM32U5

## 使用示例

### GPIO 示例

```c
#include "xy_hal_pin.h"

void setup_led(void)
{
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };

    xy_hal_pin_init(GPIOA, 5, &config);  // PA5 作为输出
}

void toggle_led(void)
{
    xy_hal_pin_toggle(GPIOA, 5);
}
```

### UART 示例

```c
#include "xy_hal_uart.h"

UART_HandleTypeDef huart1;

void setup_uart(void)
{
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };

    xy_hal_uart_init(&huart1, &config);
}

void send_message(void)
{
    const char *msg = "Hello World!\r\n";
    xy_hal_uart_send(&huart1, (uint8_t *)msg, strlen(msg), 1000);
}
```

### I2C 传感器读取示例

```c
#include "xy_hal_i2c.h"

I2C_HandleTypeDef hi2c1;

void read_sensor(void)
{
    uint8_t data[2];
    uint16_t sensor_addr = 0x68;  // 示例传感器地址
    uint16_t reg_addr = 0x00;      // 示例寄存器

    xy_hal_i2c_mem_read(&hi2c1, sensor_addr, reg_addr, data, 2, 1000);
}
```

### PWM LED 调光示例

```c
#include "xy_hal_pwm.h"

TIM_HandleTypeDef htim2;

void setup_pwm(void)
{
    xy_hal_pwm_config_t config = {
        .frequency = 1000,        // 1kHz
        .duty_cycle = 5000,       // 50% 占空比
        .polarity = XY_HAL_PWM_POLARITY_HIGH,
    };

    xy_hal_pwm_init(&htim2, XY_HAL_PWM_CHANNEL_1, &config);
    xy_hal_pwm_start(&htim2, XY_HAL_PWM_CHANNEL_1);
}

void set_brightness(uint8_t percent)
{
    uint32_t duty = (percent * 100);  // 转换 0-100 到 0-10000
    xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
}
```

### ADC 读取示例

```c
#include "xy_hal_adc.h"

ADC_HandleTypeDef hadc1;

void setup_adc(void)
{
    xy_hal_adc_config_t config = {
        .resolution = XY_HAL_ADC_RESOLUTION_12B,
        .align = XY_HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = XY_HAL_ADC_SCAN_DISABLE,
        .continuous = XY_HAL_ADC_CONTINUOUS_DISABLE,
        .trigger_src = XY_HAL_ADC_TRIGGER_SOFTWARE,
        .clock_div = ADC_CLOCK_SYNC_PCLK_DIV4,
        .sampling_time = ADC_SAMPLETIME_2CYCLES_5,
    };

    xy_hal_adc_init(&hadc1, &config);
}

void read_adc_channel(uint8_t channel)
{
    int value = xy_hal_adc_read(&hadc1, channel, 1000);
    if (value >= 0) {
        // 处理 ADC 值
    }
}
```

### RTC 实时时钟示例

```c
#include "xy_hal_rtc.h"

RTC_HandleTypeDef hrtc;

void setup_rtc(void)
{
    xy_hal_rtc_init(&hrtc);
}

void set_time(void)
{
    xy_hal_rtc_time_t time = {
        .hours = 12,
        .minutes = 30,
        .seconds = 0,
    };
    xy_hal_rtc_date_t date = {
        .weekday = 1,
        .month = 1,
        .date = 1,
        .year = 24,
    };

    xy_hal_rtc_set_time(&hrtc, &time, XY_HAL_RTC_FORMAT_BIN);
    xy_hal_rtc_set_date(&hrtc, &date, XY_HAL_RTC_FORMAT_BIN);
}

void get_time(void)
{
    xy_hal_rtc_time_t time;
    xy_hal_rtc_date_t date;

    xy_hal_rtc_get_time(&hrtc, &time, XY_HAL_RTC_FORMAT_BIN);
    xy_hal_rtc_get_date(&hrtc, &date, XY_HAL_RTC_FORMAT_BIN);
}
```

## 错误处理

所有 XY HAL 函数返回标准化错误码：

| 错误码 | 值 | 含义 |
|--------|-----|------|
| XY_HAL_OK | 0 | 成功 |
| XY_HAL_ERROR | -1 | 通用错误 |
| XY_HAL_ERROR_INVALID_PARAM | -2 | 无效参数 |
| XY_HAL_ERROR_NOT_SUPPORT | -3 | 不支持的功能 |
| XY_HAL_ERROR_TIMEOUT | -4 | 操作超时 |
| XY_HAL_ERROR_BUSY | -5 | 资源忙 |
| XY_HAL_ERROR_NO_MEM | -6 | 内存不足 |
| XY_HAL_ERROR_IO | -7 | I/O 错误 |
| XY_HAL_ERROR_NOT_INIT | -8 | 未初始化 |
| XY_HAL_ERROR_ALREADY_INIT | -9 | 已初始化 |
| XY_HAL_ERROR_NO_RESOURCE | -10 | 资源不可用 |
| XY_HAL_ERROR_FAIL | -11 | 操作失败 |

## 代码风格

本实现遵循 [xy_code_style.md](../../../../docs/rules/100-code_style/xy_code_style.md) 编码规范。

## 许可证

Apache License 2.0
