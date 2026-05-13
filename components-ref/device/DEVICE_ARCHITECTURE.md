# XinYi 设备组件架构设计规范

## 1. 总体架构设计

### 1.1 架构层次

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Applications)                    │
│  使用统一的 xy_device_* 接口访问硬件                        │
├─────────────────────────────────────────────────────────────┤
│                设备组件 (xy_device)                         │
│  统一设备框架、设备管理、注册/查找机制                      │
├─────────────────────────────────────────────────────────────┤
│              驱动 API 层 (xy_dev_api)                       │
│  分类驱动 API 结构 (uart_api, spi_api, i2c_api, etc.)        │
├─────────────────────────────────────────────────────────────┤
│            设备驱动实现层 (xy_dev_*.c)                      │
│  具体硬件驱动实现 (stm32u5_uart, stm32u5_spi, etc.)          │
├─────────────────────────────────────────────────────────────┤
│              硬件抽象层 (xy_hal)                           │
│  与 MCU 无关的统一硬件接口 (xy_hal_uart, xy_hal_spi, etc.)  │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 设计原则

1. **统一接口**: 所有设备使用统一的访问接口
2. **模块化**: 按功能分类，松耦合设计
3. **可扩展**: 支持新设备类型和驱动的动态添加
4. **可配置**: 通过 Kconfig 进行编译时裁剪
5. **易用性**: 简单的 API，清晰的文档
6. **兼容性**: 与现有 HAL 框架无缝集成

## 2. 核心数据结构设计

### 2.1 设备结构 (xy_device.h)

```c
/**
 * @brief 设备类型枚举
 */
typedef enum {
    XY_DEV_TYPE_ADC = 0,        /**< ADC 设备 */
    XY_DEV_TYPE_DAC,            /**< DAC 设备 */
    XY_DEV_TYPE_UART,           /**< UART 设备 */
    XY_DEV_TYPE_SPI,            /**< SPI 设备 */
    XY_DEV_TYPE_I2C,            /**< I2C 设备 */
    XY_DEV_TYPE_GPIO,           /**< GPIO 设备 */
    XY_DEV_TYPE_PWM,            /**< PWM 设备 */
    XY_DEV_TYPE_TIMER,          /**< 定时器设备 */
    XY_DEV_TYPE_RTC,            /**< RTC 设备 */
    XY_DEV_TYPE_WDG,            /**< 看门狗设备 */
    XY_DEV_TYPE_FLASH,          /**< Flash 设备 */
    XY_DEV_TYPE_SENSOR,         /**< 传感器设备 */
    XY_DEV_TYPE_STORAGE,        /**< 存储设备 */
    XY_DEV_TYPE_BUS,            /**< 总线设备 */
    XY_DEV_TYPE_MISC,           /**< 杂项设备 */
    XY_DEV_TYPE_MAX
} xy_dev_type_t;

/**
 * @brief 设备状态枚举
 */
typedef enum {
    XY_DEV_STATE_INIT = 0,      /**< 初始化状态 */
    XY_DEV_STATE_READY,         /**< 准备就绪 */
    XY_DEV_STATE_OPENED,        /**< 已打开 */
    XY_DEV_STATE_BUSY,          /**< 忙碌状态 */
    XY_DEV_STATE_ERROR,         /**< 错误状态 */
    XY_DEV_STATE_SUSPENDED,     /**< 挂起状态 */
} xy_dev_state_t;

/**
 * @brief 设备标志位
 */
typedef enum {
    XY_DEV_FLAG_RDWR      = 0x0001, /**< 可读写 */
    XY_DEV_FLAG_RDONLY    = 0x0002, /**< 只读 */
    XY_DEV_FLAG_WRONLY    = 0x0004, /**< 只写 */
    XY_DEV_FLAG_STREAM    = 0x0008, /**< 流设备 */
    XY_DEV_FLAG_BLOCK     = 0x0010, /**< 块设备 */
    XY_DEV_FLAG_INT       = 0x0020, /**< 支持中断 */
    XY_DEV_FLAG_DMA       = 0x0040, /**< 支持 DMA */
    XY_DEV_FLAG_ASYNC     = 0x0080, /**< 支持异步操作 */
    XY_DEV_FLAG_POLL      = 0x0100, /**< 支持轮询 */
    XY_DEV_FLAG_EVENT     = 0x0200, /**< 支持事件 */
    XY_DEV_FLAG_POWER_MGMT = 0x0400, /**< 支持电源管理 */
} xy_dev_flag_t;

/**
 * @brief 通用设备操作集
 */
typedef struct xy_dev_api {
    xy_error_t (*init)(struct xy_device *dev, const void *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*open)(struct xy_device *dev, uint32_t flags);
    xy_error_t (*close)(struct xy_device *dev);
    int32_t (*read)(struct xy_device *dev, uint32_t pos, void *buf, size_t size);
    int32_t (*write)(struct xy_device *dev, uint32_t pos, const void *buf, size_t size);
    xy_error_t (*control)(struct xy_device *dev, uint32_t cmd, void *args);
    xy_error_t (*async_read)(struct xy_device *dev, uint32_t pos, void *buf, 
                            size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*async_write)(struct xy_device *dev, uint32_t pos, const void *buf,
                             size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*ioctl)(struct xy_device *dev, uint32_t cmd, void *args);
} xy_dev_api_t;

/**
 * @brief 设备结构
 */
typedef struct xy_device {
    const char *name;                 /**< 设备名称 */
    xy_dev_type_t type;               /**< 设备类型 */
    uint32_t flags;                   /**< 设备标志 */
    xy_dev_state_t state;             /**< 设备状态 */
    const xy_dev_api_t *api;          /**< 驱动 API 结构 */
    const void *config;               /**< 设备配置 (编译时) */
    void *data;                       /**< 设备私有数据 (运行时) */
    uint8_t ref_count;                /**< 引用计数 */
    uint8_t power_mode;               /**< 电源模式 */
    struct xy_device *next;           /**< 链表指针 */
} xy_device_t;
```

### 2.2 分类驱动 API 结构

```c
/* UART 驱动 API */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_uart_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*send)(struct xy_device *dev, const uint8_t *data, size_t len, uint32_t timeout);
    int32_t (*recv)(struct xy_device *dev, uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*flush)(struct xy_device *dev);
    xy_error_t (*set_baudrate)(struct xy_device *dev, uint32_t baudrate);
    uint32_t (*get_baudrate)(struct xy_device *dev);
    xy_error_t (*enable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*disable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*async_send)(struct xy_device *dev, const uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
    xy_error_t (*async_recv)(struct xy_device *dev, uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
} xy_uart_api_t;

/* SPI 驱动 API */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_spi_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*transfer)(struct xy_device *dev, const uint8_t *tx_data, 
                       uint8_t *rx_data, size_t size, uint32_t timeout);
    xy_error_t (*set_speed)(struct xy_device *dev, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *dev);
    xy_error_t (*set_mode)(struct xy_device *dev, xy_spi_mode_t mode);
    xy_error_t (*async_transfer)(struct xy_device *dev, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_spi_api_t;

/* I2C 驱动 API */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_i2c_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*master_send)(struct xy_device *dev, uint16_t dev_addr,
                             const uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*master_recv)(struct xy_device *dev, uint16_t dev_addr,
                             uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*mem_write)(struct xy_device *dev, uint16_t dev_addr,
                           uint16_t mem_addr, const uint8_t *data, size_t len,
                           uint32_t timeout);
    xy_error_t (*mem_read)(struct xy_device *dev, uint16_t dev_addr,
                          uint16_t mem_addr, uint8_t *data, size_t len,
                          uint32_t timeout);
    xy_error_t (*is_device_ready)(struct xy_device *dev, uint16_t dev_addr,
                                  uint32_t trials, uint32_t timeout);
    xy_error_t (*async_transfer)(struct xy_device *dev, uint16_t dev_addr,
                                const uint8_t *tx_data, uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_i2c_api_t;
```

## 3. 设备注册机制

### 3.1 静态注册 (推荐)

```c
/* 静态设备注册宏 */
#define XY_DEVICE_DEFINE(name, type, init_func, api_ptr, config_ptr) \
    static xy_device_t name##_device = { \
        .name = #name, \
        .type = type, \
        .flags = XY_DEV_FLAG_RDWR, \
        .state = XY_DEV_STATE_INIT, \
        .api = api_ptr, \
        .config = config_ptr, \
        .data = NULL, \
        .ref_count = 0, \
        .power_mode = 0, \
        .next = NULL, \
    }; \
    XY_INITIALIZER(xy_register_##name##_device, \
                   XY_INIT_LEVEL_DRIVER, \
                   xy_device_register, &name##_device)

/* 使用示例 */
static const xy_uart_config_t uart1_config = {
    .baudrate = 115200,
    .wordlen = XY_UART_WORDLEN_8B,
    .stopbits = XY_UART_STOPBITS_1,
    .parity = XY_UART_PARITY_NONE,
    .flowctrl = XY_UART_FLOWCTRL_NONE,
    .mode = XY_UART_MODE_TX_RX,
};

static const xy_uart_api_t uart1_api = {
    .init = xy_uart_stm32_init,
    .deinit = xy_uart_stm32_deinit,
    .send = xy_uart_stm32_send,
    .recv = xy_uart_stm32_recv,
    .flush = xy_uart_stm32_flush,
    .set_baudrate = xy_uart_stm32_set_baudrate,
    .get_baudrate = xy_uart_stm32_get_baudrate,
};

XY_DEVICE_DEFINE(uart1, XY_DEV_TYPE_UART, xy_uart_stm32_init, &uart1_api, &uart1_config);
```

### 3.2 动态注册 (兼容性)

```c
/* 动态设备注册 */
xy_error_t xy_device_register(xy_device_t *dev);
xy_error_t xy_device_unregister(xy_device_t *dev);

/* 设备查找 */
xy_device_t *xy_device_find(const char *name);
xy_device_t *xy_device_open(const char *name, uint32_t flags);
xy_error_t xy_device_close(xy_device_t *dev);
```

## 4. 目录结构设计

```
components/device/
├── inc/                        # 公共头文件
│   ├── xy_device.h             # 设备框架接口
│   ├── xy_dev_api.h            # 通用驱动 API
│   ├── xy_dev_uart.h           # UART 驱动接口
│   ├── xy_dev_spi.h            # SPI 驱动接口
│   ├── xy_dev_i2c.h            # I2C 驱动接口
│   ├── xy_dev_adc.h            # ADC 驱动接口
│   ├── xy_dev_gpio.h           # GPIO 驱动接口
│   ├── xy_dev_pwm.h            # PWM 驱动接口
│   ├── xy_dev_timer.h          # Timer 驱动接口
│   ├── xy_dev_rtc.h            # RTC 驱动接口
│   ├── xy_dev_wdg.h            # Watchdog 驱动接口
│   ├── xy_dev_sensor.h         # 传感器驱动接口
│   └── xy_dev_bus.h            # 总线驱动接口
│
├── src/                        # 源文件
│   ├── xy_device.c             # 设备框架实现
│   ├── xy_dev_uart.c           # UART 通用实现
│   ├── xy_dev_spi.c            # SPI 通用实现
│   ├── xy_dev_i2c.c            # I2C 通用实现
│   └── ...
│
├── bus/                        # 总线驱动
│   ├── xy_bus_spi.c            # SPI 总线
│   ├── xy_bus_i2c.c            # I2C 总线
│   ├── xy_bus_can.c            # CAN 总线
│   └── ...
│
├── sensor/                     # 传感器驱动
│   ├── xy_sensor_temp.c        # 温度传感器
│   ├── xy_sensor_acc.c         # 加速度传感器
│   ├── xy_sensor_gyro.c        # 陀螺仪
│   └── ...
│
├── mcu/                        # MCU 相关驱动
│   ├── stm32/                  # STM32 系列
│   │   ├── stm32u5/           # STM32U5 系列
│   │   │   ├── xy_dev_uart_stm32u5.c
│   │   │   ├── xy_dev_spi_stm32u5.c
│   │   │   ├── xy_dev_i2c_stm32u5.c
│   │   │   ├── xy_dev_adc_stm32u5.c
│   │   │   ├── xy_dev_gpio_stm32u5.c
│   │   │   └── ...
│   │   └── ...
│   └── ...
│
├── tests/                      # 测试
│   ├── test_device.c           # 设备框架测试
│   ├── test_uart.c             # UART 测试
│   ├── test_spi.c              # SPI 测试
│   └── ...
│
├── examples/                   # 示例
│   ├── device_usage.c          # 设备使用示例
│   ├── uart_example.c          # UART 示例
│   └── ...
│
├── docs/                       # 文档
│   ├── device_model.md         # 设备模型文档
│   ├── driver_dev.md           # 驱动开发指南
│   └── ...
│
├── CMakeLists.txt              # CMake 构建配置
├── Kconfig                     # Kconfig 配置
├── Makefile                    # Make 构建配置
└── README.md                   # 说明文档
```

## 5. 构建系统设计

### 5.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)
project(xy_device C)

# Configuration options
option(XY_DEVICE_UART_ENABLED "Enable UART device support" ON)
option(XY_DEVICE_SPI_ENABLED "Enable SPI device support" ON)
option(XY_DEVICE_I2C_ENABLED "Enable I2C device support" ON)
option(XY_DEVICE_GPIO_ENABLED "Enable GPIO device support" ON)
option(XY_DEVICE_ADC_ENABLED "Enable ADC device support" ON)
option(XY_DEVICE_SENSOR_ENABLED "Enable sensor device support" OFF)

# Core sources
set(DEVICE_CORE_SOURCES
    src/xy_device.c
)

# Device-specific sources
if(XY_DEVICE_UART_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_uart.c mcu/stm32/stm32u5/xy_dev_uart_stm32u5.c)
endif()

if(XY_DEVICE_SPI_ENABLED)
    list(APPEND DEVICE_CORE_SOURCES src/xy_dev_spi.c mcu/stm32/stm32u5/xy_dev_spi_stm32u5.c)
endif()

# ... 其他设备

# Create library
add_library(xy_device ${DEVICE_CORE_SOURCES})

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
)

target_link_libraries(xy_device PRIVATE
    xy_hal
)

# Compile definitions
target_compile_definitions(xy_device PUBLIC
    XY_DEVICE_ENABLED
    $<$<BOOL:${XY_DEVICE_UART_ENABLED}>:XY_DEVICE_UART_ENABLED>
    $<$<BOOL:${XY_DEVICE_SPI_ENABLED}>:XY_DEVICE_SPI_ENABLED>
    $<$<BOOL:${XY_DEVICE_I2C_ENABLED}>:XY_DEVICE_I2C_ENABLED>
    $<$<BOOL:${XY_DEVICE_GPIO_ENABLED}>:XY_DEVICE_GPIO_ENABLED>
    $<$<BOOL:${XY_DEVICE_ADC_ENABLED}>:XY_DEVICE_ADC_ENABLED>
    $<$<BOOL:${XY_DEVICE_SENSOR_ENABLED}>:XY_DEVICE_SENSOR_ENABLED>
)
```

### 5.2 Kconfig

```
menu "XY Device Configuration"

config XY_DEVICE_ENABLED
    bool "Enable XY Device Framework"
    default y
    help
      Enable the XY Device framework for unified device management.

if XY_DEVICE_ENABLED

config XY_DEVICE_MAX_COUNT
    int "Maximum number of devices"
    default 32
    range 8 256
    help
      Maximum number of devices that can be registered.

config XY_DEVICE_UART_ENABLED
    bool "Enable UART Device Support"
    default y
    help
      Enable UART device driver support.

config XY_DEVICE_SPI_ENABLED
    bool "Enable SPI Device Support"
    default y
    help
      Enable SPI device driver support.

config XY_DEVICE_I2C_ENABLED
    bool "Enable I2C Device Support"
    default y
    help
      Enable I2C device driver support.

config XY_DEVICE_GPIO_ENABLED
    bool "Enable GPIO Device Support"
    default y
    help
      Enable GPIO device driver support.

config XY_DEVICE_ADC_ENABLED
    bool "Enable ADC Device Support"
    default y
    help
      Enable ADC device driver support.

config XY_DEVICE_SENSOR_ENABLED
    bool "Enable Sensor Device Support"
    default n
    depends on XY_DEVICE_ADC_ENABLED
    help
      Enable sensor device support.

config XY_DEVICE_BUS_ENABLED
    bool "Enable Bus Device Support"
    default y
    help
      Enable bus device support (SPI/I2C/CAN buses).

endif # XY_DEVICE_ENABLED

endmenu
```

## 6. 使用示例

### 6.1 设备使用示例

```c
/**
 * @brief XinYi 设备组件使用示例
 */
#include "xy_device.h"

void device_usage_example(void)
{
    // 1. 查找设备
    xy_device_t *uart1 = xy_device_find("uart1");
    if (!uart1) {
        xy_log_e("UART1 device not found\n");
        return;
    }

    // 2. 打开设备
    xy_device_t *dev = xy_device_open("uart1", XY_DEV_FLAG_RDWR);
    if (!dev) {
        xy_log_e("Failed to open UART1\n");
        return;
    }

    // 3. 发送数据
    const char *msg = "Hello from XY Device Framework!\r\n";
    int32_t ret = xy_device_write(dev, 0, msg, strlen(msg));
    if (ret < 0) {
        xy_log_e("UART write failed: %d\n", ret);
    }

    // 4. 控制设备
    xy_uart_config_t config = {
        .baudrate = 9600,
        .wordlen = XY_UART_WORDLEN_8B,
        .stopbits = XY_UART_STOPBITS_1,
        .parity = XY_UART_PARITY_NONE,
        .flowctrl = XY_UART_FLOWCTRL_NONE,
        .mode = XY_UART_MODE_TX_RX,
    };
    xy_device_control(dev, XY_UART_CMD_SET_CONFIG, &config);

    // 5. 关闭设备
    xy_device_close(dev);
}

/* UART 驱动实现示例 */
static xy_error_t uart_stm32_init(struct xy_device *dev, const xy_uart_config_t *config)
{
    if (!dev || !config) {
        return XY_ERROR_INVALID_PARAM;
    }

    // 获取设备私有数据
    xy_uart_dev_data_t *data = (xy_uart_dev_data_t *)dev->data;
    if (!data) {
        return XY_ERROR_INVALID_STATE;
    }

    // 使用 HAL 初始化
    xy_hal_uart_config_t hal_config = {
        .baudrate = config->baudrate,
        .wordlen = config->wordlen,
        .stopbits = config->stopbits,
        .parity = config->parity,
        .flowctrl = config->flowctrl,
        .mode = config->mode,
    };

    return xy_hal_uart_init(data->hal_handle, &hal_config);
}

static int32_t uart_stm32_send(struct xy_device *dev, const uint8_t *data, 
                              size_t len, uint32_t timeout)
{
    xy_uart_dev_data_t *dev_data = (xy_uart_dev_data_t *)dev->data;
    return xy_hal_uart_send(dev_data->hal_handle, data, len, timeout);
}

static const xy_uart_api_t uart_stm32_api = {
    .init = uart_stm32_init,
    .deinit = uart_stm32_deinit,
    .send = uart_stm32_send,
    .recv = uart_stm32_recv,
    .flush = uart_stm32_flush,
    .set_baudrate = uart_stm32_set_baudrate,
    .get_baudrate = uart_stm32_get_baudrate,
};
```

### 6.2 总线设备示例

```c
/**
 * @brief 总线设备使用示例
 */
void bus_device_example(void)
{
    // 1. 获取 SPI 总线
    xy_device_t *spi_bus = xy_device_find("spi1");
    if (!spi_bus) return;

    // 2. 获取 SPI 设备 (节点)
    xy_device_t *spi_device = xy_device_find("spi_flash");
    if (!spi_device) return;

    // 3. 配置 SPI 设备
    xy_spi_config_t spi_config = {
        .mode = XY_SPI_MODE_0,
        .direction = XY_SPI_DIR_2LINES,
        .datasize = XY_SPI_DATASIZE_8B,
        .bitorder = XY_SPI_MSB_FIRST,
        .baudrate = 1000000,  // 1MHz
        .is_master = 1,
    };
    xy_device_control(spi_device, XY_SPI_CMD_SET_CONFIG, &spi_config);

    // 4. 传输数据
    const uint8_t tx_data[] = {0x9F};  // Read JEDEC ID command
    uint8_t rx_data[3];
    
    int32_t ret = xy_device_write(spi_device, 0, tx_data, sizeof(tx_data));
    if (ret >= 0) {
        ret = xy_device_read(spi_device, 0, rx_data, sizeof(rx_data));
    }
    
    if (ret >= 0) {
        xy_log_i("JEDEC ID: 0x%02X%02X%02X\n", 
                 rx_data[0], rx_data[1], rx_data[2]);
    }
}
```

## 7. 错误处理设计

### 7.1 统一错误码

```c
/* 继承自 xy_hal.h 的错误码 */
typedef enum {
    XY_OK = 0,                    /**< 成功 */
    XY_ERROR = -1,                /**< 通用错误 */
    XY_ERROR_INVALID_PARAM = -2,  /**< 无效参数 */
    XY_ERROR_NOT_SUPPORT = -3,    /**< 不支持 */
    XY_ERROR_TIMEOUT = -4,        /**< 超时 */
    XY_ERROR_BUSY = -5,           /**< 忙碌 */
    XY_ERROR_NO_MEMORY = -6,      /**< 内存不足 */
    XY_ERROR_IO = -7,             /**< I/O 错误 */
    XY_ERROR_NOT_INIT = -8,       /**< 未初始化 */
    XY_ERROR_ALREADY_INIT = -9,   /**< 已初始化 */
    XY_ERROR_NO_RESOURCE = -10,   /**< 无资源 */
    XY_ERROR_FAIL = -11,          /**< 失败 */
    // ... 更多错误码
} xy_error_t;
```

### 7.2 错误处理宏

```c
/* 错误处理宏 */
#define XY_RETURN_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (_err != XY_OK) { \
            return _err; \
        } \
    } while(0)

#define XY_RETURN_VAL_ON_ERROR(expr, val) \
    do { \
        xy_error_t _err = (expr); \
        if (_err != XY_OK) { \
            return (val); \
        } \
    } while(0)
```

## 8. 电源管理集成

```c
/* 电源管理模式 */
typedef enum {
    XY_DEV_POWER_NORMAL = 0,      /**< 正常模式 */
    XY_DEV_POWER_LOW,             /**< 低功耗模式 */
    XY_DEV_POWER_SLEEP,           /**< 睡眠模式 */
    XY_DEV_POWER_DEEP_SLEEP,      /**< 深度睡眠模式 */
    XY_DEV_POWER_OFF,             /**< 关闭模式 */
} xy_dev_power_mode_t;

/* 设备电源管理 API */
typedef struct {
    xy_error_t (*set_power_mode)(struct xy_device *dev, xy_dev_power_mode_t mode);
    xy_error_t (*get_power_mode)(struct xy_device *dev, xy_dev_power_mode_t *mode);
    xy_error_t (*suspend)(struct xy_device *dev);
    xy_error_t (*resume)(struct xy_device *dev);
    xy_error_t (*enter_low_power)(struct xy_device *dev);
    xy_error_t (*exit_low_power)(struct xy_device *dev);
} xy_dev_pm_api_t;
```

## 9. 与现有系统集成

### 9.1 与 HAL 集成

```
设备层 (xy_device) ←→ 硬件抽象层 (xy_hal) ←→ MCU HAL
    ↑                    ↑                     ↑
  统一接口              统一接口                具体实现
```

### 9.2 与 OSAL 集成

- 设备操作可以在不同 RTOS 环境下运行
- 异步操作与 OSAL 事件系统集成
- 电源管理与 OSAL 低功耗模式集成

## 10. 实施计划

### 10.1 阶段一 (1-2 周): 核心框架

- [ ] 设备框架核心实现 (xy_device.c)
- [ ] 通用设备 API 定义 (xy_dev_api.h)
- [ ] UART 驱动实现 (xy_dev_uart.c)
- [ ] STM32U5 UART 驱动 (mcu/stm32/stm32u5/xy_dev_uart_stm32u5.c)
- [ ] 基础测试

### 10.2 阶段二 (2-4 周): 基础外设

- [ ] SPI 驱动实现
- [ ] I2C 驱动实现
- [ ] GPIO 驱动实现
- [ ] ADC 驱动实现
- [ ] 完善测试

### 10.3 阶段三 (1 个月): 高级功能

- [ ] 总线模型实现
- [ ] 传感器框架
- [ ] 电源管理集成
- [ ] 异步操作支持
- [ ] 文档完善

## 11. 性能考虑

- **内存效率**: 静态注册减少运行时内存使用
- **执行效率**: 直接函数指针调用，无额外开销
- **配置验证**: 编译时配置验证，减少运行时错误
- **代码大小**: 按需编译，支持功能裁剪

## 12. 可扩展性

- **新设备类型**: 通过添加新 API 结构支持
- **新 MCU 系列**: 在 mcu/ 目录下添加新子目录
- **新总线类型**: 在 bus/ 目录下添加实现
- **新传感器类型**: 在 sensor/ 目录下添加驱动

---

**维护者**: XinYi Team  
**版本**: 2.0  
**日期**: 2026-02-28
