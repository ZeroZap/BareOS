# XY HAL 集成指南 / Integration Guide

## 快速开始 / Quick Start

### 重要说明 / Important Notes

**函数名约定**: 所有 HAL 实现函数名必须与 `inc/` 中的接口声明完全一致，不包含平台后缀。

例如：
- 接口声明: `inc/xy_hal_pin.h` 中的 `int xy_hal_pin_init(...)`
- STM32F4 实现: `stm32/stm32f4/xy_hal_pin.c` 中的 `int xy_hal_pin_init(...)`
- STM32U0 实现: `stm32/stm32u0/xy_hal_pin.c` 中的 `int xy_hal_pin_init(...)`

### 1. 添加到现有 STM32 项目 / Add to Existing STM32 Project

#### 1.1 使用 STM32CubeMX + Makefile

在你的 `Makefile` 中添加：

```makefile
# XY HAL 相关配置
XY_HAL_DIR = bsp/xy_hal
XY_HAL_INC = $(XY_HAL_DIR)/inc
XY_HAL_STM32 = $(XY_HAL_DIR)/stm32
STM32_SERIES = stm32f4  # 或 stm32u0, stm32l4, stm32h7

# 添加头文件路径
C_INCLUDES += -I$(XY_HAL_INC)
C_INCLUDES += -I$(XY_HAL_STM32)

# 添加源文件
C_SOURCES += $(wildcard $(XY_HAL_STM32)/$(STM32_SERIES)/*.c)

# 添加宏定义
C_DEFS += -DSTM32_HAL_ENABLED
```

#### 1.2 使用 STM32CubeIDE (CMake)

在你的 `CMakeLists.txt` 中添加：

```cmake
# XY HAL 配置
set(XY_HAL_DIR ${CMAKE_SOURCE_DIR}/bsp/xy_hal)
set(STM32_SERIES "stm32f4" CACHE STRING "STM32 Series")  # 或 stm32u0, stm32l4, stm32h7

# 包含头文件
include_directories(
    ${XY_HAL_DIR}/inc
    ${XY_HAL_DIR}/stm32
)

# 添加源文件
file(GLOB XY_HAL_SOURCES
    "${XY_HAL_DIR}/stm32/${STM32_SERIES}/*.c"
)

# 添加到可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${XY_HAL_SOURCES}
)

# 添加宏定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32_HAL_ENABLED
)
```

#### 1.3 使用 Keil MDK

1. 添加 Group "XY_HAL"
2. 根据 STM32 系列选择源文件目录：
   - STM32F4: 添加所有 `bsp/xy_hal/stm32/stm32f4/*.c` 文件
   - STM32U0: 添加所有 `bsp/xy_hal/stm32/stm32u0/*.c` 文件
   - STM32L4: 添加所有 `bsp/xy_hal/stm32/stm32l4/*.c` 文件
   - STM32H7: 添加所有 `bsp/xy_hal/stm32/stm32h7/*.c` 文件
3. 在 C/C++ 选项中添加 Include Paths:
   - `bsp/xy_hal/inc`
   - `bsp/xy_hal/stm32`
4. 在 C/C++ 选项中添加 Define: `STM32_HAL_ENABLED`

#### 1.4 使用 IAR EWARM

1. 根据 STM32 系列选择源文件目录，添加到项目：
   - STM32F4: `bsp/xy_hal/stm32/stm32f4/*.c`
   - STM32U0: `bsp/xy_hal/stm32/stm32u0/*.c`
   - STM32L4: `bsp/xy_hal/stm32/stm32l4/*.c`
   - STM32H7: `bsp/xy_hal/stm32/stm32h7/*.c`
2. 在 Project Options → C/C++ Compiler → Preprocessor → Additional include directories 中添加:
   - `$PROJ_DIR$/bsp/xy_hal/inc`
   - `$PROJ_DIR$/bsp/xy_hal/stm32`
3. 在 Defined symbols 中添加: `STM32_HAL_ENABLED`

### 2. 初始化 / Initialization

#### 2.1 在 main.c 中初始化外设

```c
#include "xy_hal.h"

/* HAL 句柄定义（由 CubeMX 生成或手动定义）*/
UART_HandleTypeDef huart1;
SPI_HandleTypeDef hspi1;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;

int main(void)
{
    /* STM32 HAL 初始化 */
    HAL_Init();
    SystemClock_Config();

    /* 使能外设时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* 配置 GPIO */
    xy_hal_pin_config_t led_config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };
    xy_hal_pin_init(GPIOA, 5, &led_config);

    /* 配置 UART */
    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_uart_init(&huart1, &uart_config);

    while (1)
    {
        xy_hal_pin_toggle(GPIOA, 5);
        HAL_Delay(500);
    }
}
```

### 3. 常见使用场景 / Common Use Cases

#### 3.1 LED 闪烁

```c
void blink_led(void)
{
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };
    xy_hal_pin_init(GPIOA, 5, &config);

    while (1) {
        xy_hal_pin_toggle(GPIOA, 5);
        HAL_Delay(500);
    }
}
```

#### 3.2 串口打印

```c
void uart_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    xy_hal_uart_send(&huart1, (uint8_t *)buffer, len, 1000);
}

// 使用
uart_printf("Temperature: %d°C\r\n", temperature);
```

#### 3.3 I2C 传感器读取

```c
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
} mpu6050_data_t;

int read_mpu6050(mpu6050_data_t *data)
{
    uint8_t raw_data[6];
    uint16_t device_addr = 0x68;
    uint16_t reg_addr = 0x3B;  // ACCEL_XOUT_H

    int ret = xy_hal_i2c_mem_read(&hi2c1, device_addr, reg_addr,
                                  raw_data, 6, 1000);
    if (ret == 0) {
        data->accel_x = (raw_data[0] << 8) | raw_data[1];
        data->accel_y = (raw_data[2] << 8) | raw_data[3];
        data->accel_z = (raw_data[4] << 8) | raw_data[5];
    }

    return ret;
}
```

#### 3.4 PWM 调光

```c
void led_fade(void)
{
    xy_hal_pwm_config_t config = {
        .frequency = 1000,  // 1kHz
        .duty_cycle = 0,
        .polarity = XY_HAL_PWM_POLARITY_HIGH,
    };

    xy_hal_pwm_init(&htim2, XY_HAL_PWM_CHANNEL_1, &config);
    xy_hal_pwm_start(&htim2, XY_HAL_PWM_CHANNEL_1);

    // 渐亮
    for (uint32_t duty = 0; duty <= 10000; duty += 50) {
        xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
        HAL_Delay(10);
    }

    // 渐暗
    for (uint32_t duty = 10000; duty > 0; duty -= 50) {
        xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
        HAL_Delay(10);
    }
}
```

#### 3.5 按键中断

```c
void button_pressed(void *arg)
{
    static uint32_t count = 0;
    count++;
    uart_printf("Button pressed %lu times\r\n", count);
}

void setup_button(void)
{
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_INPUT,
        .pull = XY_HAL_PIN_PULL_UP,
    };

    xy_hal_pin_init(GPIOC, 13, &config);
    xy_hal_pin_attach_irq(GPIOC, 13, XY_HAL_PIN_IRQ_FALLING,
                          button_pressed, NULL);
}
```

### 4. 调试技巧 / Debugging Tips

#### 4.1 检查返回值

```c
int ret = xy_hal_uart_init(&huart1, &config);
if (ret != 0) {
    // 初始化失败，检查配置
    uart_printf("UART init failed: %d\r\n", ret);
}
```

#### 4.2 添加调试输出

```c
#define HAL_DEBUG 1

#if HAL_DEBUG
    #define HAL_DBG(fmt, ...) uart_printf("[HAL] " fmt, ##__VA_ARGS__)
#else
    #define HAL_DBG(fmt, ...)
#endif

// 使用
HAL_DBG("GPIO init: port=%p, pin=%d\r\n", port, pin);
```

#### 4.3 检查时钟使能

确保在使用外设前使能了相应的时钟：

```c
/* GPIO */
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();

/* UART */
__HAL_RCC_USART1_CLK_ENABLE();

/* SPI */
__HAL_RCC_SPI1_CLK_ENABLE();

/* I2C */
__HAL_RCC_I2C1_CLK_ENABLE();

/* TIM */
__HAL_RCC_TIM2_CLK_ENABLE();
```

### 5. 性能优化 / Performance Optimization

#### 5.1 使用 DMA 传输

```c
/* UART DMA 发送 */
uint8_t tx_buffer[1024];
// 填充数据...
xy_hal_uart_send_dma(&huart1, tx_buffer, sizeof(tx_buffer));

/* SPI DMA 传输 */
uint8_t spi_tx[256], spi_rx[256];
xy_hal_spi_transmit_receive_dma(&hspi1, spi_tx, spi_rx, 256);
```

#### 5.2 中断优先级配置

```c
/* 在初始化中设置合适的优先级 */
HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6, 0);
HAL_NVIC_SetPriority(TIM2_IRQn, 7, 0);
```

### 6. 常见问题 / FAQ

#### Q1: 编译时找不到头文件？
**A**: 检查 Include Path 是否正确添加了 `bsp/xy_hal/inc` 和 `bsp/xy_hal/stm32`

#### Q2: 链接时出现未定义的引用？
**A**: 确保添加了所有 `xy_hal_*_stm32.c` 文件到项目中

#### Q3: GPIO 操作无效？
**A**: 检查是否使能了 GPIO 时钟：`__HAL_RCC_GPIOx_CLK_ENABLE()`

#### Q4: UART 无法发送数据？
**A**:
1. 检查 UART 时钟是否使能
2. 检查 GPIO 复用功能配置
3. 检查波特率配置是否正确

#### Q5: I2C 通信失败？
**A**:
1. 检查设备地址是否正确（7位地址）
2. 检查上拉电阻是否存在
3. 使用示波器检查 SCL/SDA 信号

#### Q6: 如何移植到其他 STM32 系列？
**A**: 修改宏定义即可，如 `STM32F1`、`STM32F4`、`STM32L4` 等

### 7. 迁移到其他平台 / Porting to Other Platforms

如需支持其他平台（如 ESP32、Nordic、HC32 等）：

1. 创建新目录：`bsp/xy_hal/esp32/`
2. 实现所有接口：`xy_hal_*_esp32.c`
3. 创建平台头文件：`esp32_hal.h`
4. 更新构建脚本

参考 STM32 实现作为模板。

### 8. 技术支持 / Support

- 查看详细文档：`bsp/xy_hal/README.md`
- 参考示例代码：`bsp/xy_hal/stm32/example_usage.c`
- 实现总结：`bsp/xy_hal/IMPLEMENTATION_SUMMARY.md`

---

**更新时间**: 2025-10-26
**版本**: 1.0
