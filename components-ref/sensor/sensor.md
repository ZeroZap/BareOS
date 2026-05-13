# 适配 OSAL/裸机的 Sensor 框架设计

基于 RT-Thread 和 Zephyr 的 Sensor 框架设计思想,我为您设计一个可适配 OSAL 或裸机环境的轻量级 Sensor 框架。

## 1. 框架架构设计

```text
┌─────────────────────────────────────────┐
│         Application Layer               │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│      Sensor Framework API Layer         │
│  (sensor_init/read/config/control)      │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         Sensor Manager Layer            │
│  (Device Registration & Management)     │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         Sensor Driver Layer             │
│  (Specific Sensor Implementation)       │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         Hardware Abstraction            │
│       (I2C/SPI/GPIO/ADC...)             │
└─────────────────────────────────────────┘
```

## 2. 核心头文件

### sensor_type.h - 传感器类型定义

```c
#ifndef __SENSOR_TYPE_H__
#define __SENSOR_TYPE_H__

#include <stdint.h>
#include <stdbool.h>

/* Sensor 类型定义 (参考 Zephyr 和 RT-Thread) */
typedef enum {
    SENSOR_TYPE_NONE                    = 0x00,
    
    /* 运动传感器 */
    SENSOR_TYPE_ACCELEROMETER           = 0x01,  /* 加速度计 */
    SENSOR_TYPE_GYROSCOPE               = 0x02,  /* 陀螺仪 */
    SENSOR_TYPE_MAGNETOMETER            = 0x03,  /* 磁力计 */
    SENSOR_TYPE_ORIENTATION             = 0x04,  /* 方向传感器 */
    SENSOR_TYPE_GRAVITY                 = 0x05,  /* 重力传感器 */
    SENSOR_TYPE_LINEAR_ACCELERATION     = 0x06,  /* 线性加速度 */
    SENSOR_TYPE_ROTATION_VECTOR         = 0x07,  /* 旋转矢量 */
    
    /* 环境传感器 */
    SENSOR_TYPE_TEMPERATURE             = 0x10,  /* 温度 */
    SENSOR_TYPE_HUMIDITY                = 0x11,  /* 湿度 */
    SENSOR_TYPE_PRESSURE                = 0x12,  /* 气压 */
    SENSOR_TYPE_AMBIENT_LIGHT           = 0x13,  /* 环境光 */
    SENSOR_TYPE_AMBIENT_TEMP            = 0x14,  /* 环境温度 */
    SENSOR_TYPE_GAS_RESISTANCE          = 0x15,  /* 气体阻值 */
    SENSOR_TYPE_CO2                     = 0x16,  /* 二氧化碳 */
    SENSOR_TYPE_TVOC                    = 0x17,  /* 挥发性有机物 */
    
    /* 位置传感器 */
    SENSOR_TYPE_PROXIMITY               = 0x20,  /* 接近传感器 */
    SENSOR_TYPE_DISTANCE                = 0x21,  /* 距离传感器 */
    SENSOR_TYPE_GPS                     = 0x22,  /* GPS */
    
    /* 光学传感器 */
    SENSOR_TYPE_LIGHT                   = 0x30,  /* 光照强度 */
    SENSOR_TYPE_IR                      = 0x31,  /* 红外 */
    SENSOR_TYPE_UV                      = 0x32,  /* 紫外线 */
    SENSOR_TYPE_RGB                     = 0x33,  /* RGB颜色 */
    
    /* 生物传感器 */
    SENSOR_TYPE_HEART_RATE              = 0x40,  /* 心率 */
    SENSOR_TYPE_HEART_RATE_VARIABILITY  = 0x41,  /* 心率变异性 */
    SENSOR_TYPE_SPO2                    = 0x42,  /* 血氧 */
    SENSOR_TYPE_BLOOD_PRESSURE          = 0x43,  /* 血压 */
    SENSOR_TYPE_ECG                     = 0x44,  /* 心电图 */
    
    /* 其他传感器 */
    SENSOR_TYPE_VOLTAGE                 = 0x50,  /* 电压 */
    SENSOR_TYPE_CURRENT                 = 0x51,  /* 电流 */
    SENSOR_TYPE_POWER                   = 0x52,  /* 功率 */
    SENSOR_TYPE_PM2_5                   = 0x53,  /* PM2.5 */
    SENSOR_TYPE_PM10                    = 0x54,  /* PM10 */
    SENSOR_TYPE_NOISE                   = 0x55,  /* 噪音 */
    
    SENSOR_TYPE_CUSTOM                  = 0xFF,  /* 自定义 */
} sensor_type_t;

/* Sensor 单位定义 */
typedef enum {
    SENSOR_UNIT_NONE                    = 0,
    
    /* 加速度单位 */
    SENSOR_UNIT_METER_PER_SECOND_SQUARED,        /* m/s² */
    SENSOR_UNIT_MILLI_G,                         /* mg */
    
    /* 角速度单位 */
    SENSOR_UNIT_RADIAN_PER_SECOND,               /* rad/s */
    SENSOR_UNIT_DEGREE_PER_SECOND,               /* °/s */
    
    /* 磁场单位 */
    SENSOR_UNIT_GAUSS,                           /* Gauss */
    SENSOR_UNIT_MICRO_TESLA,                     /* μT */
    
    /* 温度单位 */
    SENSOR_UNIT_CELSIUS,                         /* °C */
    SENSOR_UNIT_FAHRENHEIT,                      /* °F */
    SENSOR_UNIT_KELVIN,                          /* K */
    
    /* 湿度单位 */
    SENSOR_UNIT_PERCENT,                         /* % */
    
    /* 压力单位 */
    SENSOR_UNIT_PASCAL,                          /* Pa */
    SENSOR_UNIT_KILOPASCAL,                      /* kPa */
    SENSOR_UNIT_HECTOPASCAL,                     /* hPa */
    
    /* 光照单位 */
    SENSOR_UNIT_LUX,                             /* lux */
    
    /* 距离单位 */
    SENSOR_UNIT_METER,                           /* m */
    SENSOR_UNIT_CENTIMETER,                      /* cm */
    SENSOR_UNIT_MILLIMETER,                      /* mm */
    
    /* 浓度单位 */
    SENSOR_UNIT_PPM,                             /* ppm */
    SENSOR_UNIT_PPB,                             /* ppb */
    SENSOR_UNIT_UG_PER_CUBIC_METER,              /* μg/m³ */
    
    /* 电气单位 */
    SENSOR_UNIT_VOLT,                            /* V */
    SENSOR_UNIT_MILLIVOLT,                       /* mV */
    SENSOR_UNIT_AMPERE,                          /* A */
    SENSOR_UNIT_MILLIAMPERE,                     /* mA */
    SENSOR_UNIT_WATT,                            /* W */
    
    /* 生物单位 */
    SENSOR_UNIT_BPM,                             /* 次/分钟 */
    SENSOR_UNIT_MMHG,                            /* mmHg */
} sensor_unit_t;

/* Sensor 数据值类型 */
typedef union {
    int32_t     val_int32;
    uint32_t    val_uint32;
    float       val_float;
    double      val_double;
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } val_3axis;
    uint8_t     val_bytes[16];
} sensor_value_t;

/* Sensor 数据结构 */
typedef struct {
    sensor_type_t   type;           /* 传感器类型 */
    sensor_unit_t   unit;           /* 数据单位 */
    sensor_value_t  value;          /* 传感器数据 */
    uint32_t        timestamp;      /* 时间戳 */
} sensor_data_t;

/* Sensor 配置参数 */
typedef enum {
    SENSOR_CFG_ODR,                 /* 输出数据速率 (Hz) */
    SENSOR_CFG_RANGE,               /* 测量范围 */
    SENSOR_CFG_RESOLUTION,          /* 分辨率 */
    SENSOR_CFG_POWER_MODE,          /* 功耗模式 */
    SENSOR_CFG_TRIGGER_MODE,        /* 触发模式 */
    SENSOR_CFG_THRESHOLD,           /* 阈值 */
    SENSOR_CFG_CALIBRATION,         /* 校准 */
    SENSOR_CFG_FILTER,              /* 滤波器 */
} sensor_config_type_t;

/* Sensor 功耗模式 */
typedef enum {
    SENSOR_POWER_MODE_SHUTDOWN,     /* 关闭 */
    SENSOR_POWER_MODE_SLEEP,        /* 睡眠 */
    SENSOR_POWER_MODE_STANDBY,      /* 待机 */
    SENSOR_POWER_MODE_NORMAL,       /* 正常 */
    SENSOR_POWER_MODE_HIGH_PERFORMANCE, /* 高性能 */
} sensor_power_mode_t;

/* Sensor 触发模式 */
typedef enum {
    SENSOR_TRIGGER_MODE_POLLING,    /* 轮询模式 */
    SENSOR_TRIGGER_MODE_INTERRUPT,  /* 中断模式 */
    SENSOR_TRIGGER_MODE_FIFO,       /* FIFO模式 */
} sensor_trigger_mode_t;

/* Sensor 状态 */
typedef enum {
    SENSOR_STATUS_IDLE,             /* 空闲 */
    SENSOR_STATUS_READY,            /* 就绪 */
    SENSOR_STATUS_BUSY,             /* 忙碌 */
    SENSOR_STATUS_ERROR,            /* 错误 */
} sensor_status_t;

/* 错误码定义 */
typedef enum {
    SENSOR_EOK          = 0,        /* 成功 */
    SENSOR_ERROR        = -1,       /* 通用错误 */
    SENSOR_EINVAL       = -2,       /* 无效参数 */
    SENSOR_ENODEV       = -3,       /* 设备不存在 */
    SENSOR_EBUSY        = -4,       /* 设备忙 */
    SENSOR_ETIMEOUT     = -5,       /* 超时 */
    SENSOR_ENOMEM       = -6,       /* 内存不足 */
    SENSOR_ENOSYS       = -7,       /* 功能未实现 */
    SENSOR_EIO          = -8,       /* I/O错误 */
} sensor_err_t;

#endif /* __SENSOR_TYPE_H__ */
```

### sensor_core.h - 框架核心接口

```c
#ifndef __SENSOR_CORE_H__
#define __SENSOR_CORE_H__

#include "sensor_type.h"

/* 前置声明 */
struct sensor_device;

/* Sensor 回调函数类型 */
typedef void (*sensor_callback_t)(struct sensor_device *sensor, sensor_data_t *data, void *user_data);

/* Sensor 设备操作接口 */
typedef struct sensor_ops {
    /* 必须实现的接口 */
    sensor_err_t (*init)(struct sensor_device *sensor);
    sensor_err_t (*deinit)(struct sensor_device *sensor);
    sensor_err_t (*read)(struct sensor_device *sensor, sensor_data_t *data);
    
    /* 可选接口 */
    sensor_err_t (*write)(struct sensor_device *sensor, const sensor_data_t *data);
    sensor_err_t (*control)(struct sensor_device *sensor, int cmd, void *args);
    sensor_err_t (*config)(struct sensor_device *sensor, sensor_config_type_t cfg, void *value);
    sensor_err_t (*enable)(struct sensor_device *sensor, bool enable);
} sensor_ops_t;

/* Sensor 设备信息 */
typedef struct sensor_info {
    const char          *name;          /* 设备名称 */
    const char          *vendor;        /* 厂商名称 */
    const char          *model;         /* 型号 */
    uint32_t            version;        /* 版本 */
    sensor_type_t       type;           /* 传感器类型 */
    sensor_unit_t       unit;           /* 默认单位 */
    int32_t             range_max;      /* 最大量程 */
    int32_t             range_min;      /* 最小量程 */
    uint32_t            resolution;     /* 分辨率 */
    uint32_t            max_odr;        /* 最大输出速率 */
} sensor_info_t;

/* Sensor 设备结构体 */
typedef struct sensor_device {
    /* 设备信息 */
    sensor_info_t       info;           /* 传感器信息 */
    uint8_t             id;             /* 设备ID */
    sensor_status_t     status;         /* 设备状态 */
    
    /* 设备操作 */
    const sensor_ops_t  *ops;           /* 操作接口 */
    void                *bus;           /* 总线句柄 (I2C/SPI) */
    void                *priv_data;     /* 私有数据 */
    
    /* 配置参数 */
    uint32_t            odr;            /* 当前输出速率 */
    sensor_power_mode_t power_mode;     /* 功耗模式 */
    sensor_trigger_mode_t trigger_mode; /* 触发模式 */
    
    /* 回调与用户数据 */
    sensor_callback_t   callback;       /* 数据回调 */
    void                *user_data;     /* 用户数据 */
    
    /* 链表节点 (用于设备管理) */
    struct sensor_device *next;
} sensor_device_t;

/* =============== 核心API接口 =============== */

/**
 * @brief 注册传感器设备
 * @param sensor 传感器设备指针
 * @return 错误码
 */
sensor_err_t sensor_register(sensor_device_t *sensor);

/**
 * @brief 注销传感器设备
 * @param sensor 传感器设备指针
 * @return 错误码
 */
sensor_err_t sensor_unregister(sensor_device_t *sensor);

/**
 * @brief 根据名称查找传感器
 * @param name 设备名称
 * @return 传感器设备指针
 */
sensor_device_t* sensor_find_by_name(const char *name);

/**
 * @brief 根据类型查找传感器
 * @param type 传感器类型
 * @return 传感器设备指针
 */
sensor_device_t* sensor_find_by_type(sensor_type_t type);

/**
 * @brief 初始化传感器
 * @param sensor 传感器设备指针
 * @return 错误码
 */
sensor_err_t sensor_init(sensor_device_t *sensor);

/**
 * @brief 反初始化传感器
 * @param sensor 传感器设备指针
 * @return 错误码
 */
sensor_err_t sensor_deinit(sensor_device_t *sensor);

/**
 * @brief 使能/失能传感器
 * @param sensor 传感器设备指针
 * @param enable true-使能, false-失能
 * @return 错误码
 */
sensor_err_t sensor_enable(sensor_device_t *sensor, bool enable);

/**
 * @brief 读取传感器数据
 * @param sensor 传感器设备指针
 * @param data 数据缓冲区
 * @return 错误码
 */
sensor_err_t sensor_read(sensor_device_t *sensor, sensor_data_t *data);

/**
 * @brief 写入传感器数据
 * @param sensor 传感器设备指针
 * @param data 数据
 * @return 错误码
 */
sensor_err_t sensor_write(sensor_device_t *sensor, const sensor_data_t *data);

/**
 * @brief 配置传感器参数
 * @param sensor 传感器设备指针
 * @param cfg 配置类型
 * @param value 配置值
 * @return 错误码
 */
sensor_err_t sensor_config(sensor_device_t *sensor, sensor_config_type_t cfg, void *value);

/**
 * @brief 控制传感器
 * @param sensor 传感器设备指针
 * @param cmd 控制命令
 * @param args 参数
 * @return 错误码
 */
sensor_err_t sensor_control(sensor_device_t *sensor, int cmd, void *args);

/**
 * @brief 设置数据回调函数
 * @param sensor 传感器设备指针
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 错误码
 */
sensor_err_t sensor_set_callback(sensor_device_t *sensor, sensor_callback_t callback, void *user_data);

/**
 * @brief 获取传感器信息
 * @param sensor 传感器设备指针
 * @return 传感器信息指针
 */
const sensor_info_t* sensor_get_info(sensor_device_t *sensor);

/**
 * @brief 获取传感器状态
 * @param sensor 传感器设备指针
 * @return 传感器状态
 */
sensor_status_t sensor_get_status(sensor_device_t *sensor);

#endif /* __SENSOR_CORE_H__ */
```

### sensor_core.c - 框架实现

```c
#include "sensor_core.h"
#include <string.h>

/* 配置参数 */
#ifndef SENSOR_MAX_DEVICES
#define SENSOR_MAX_DEVICES  16
#endif

/* 传感器设备链表头 */
static sensor_device_t *sensor_list_head = NULL;

/* 临界区保护 (根据OSAL适配) */
#ifdef USE_OSAL
    #include "osal.h"
    #define SENSOR_LOCK()       osal_mutex_lock()
    #define SENSOR_UNLOCK()     osal_mutex_unlock()
#else
    /* 裸机环境 - 关闭中断 */
    static uint32_t irq_state = 0;
    #define SENSOR_LOCK()       do { irq_state = __get_PRIMASK(); __disable_irq(); } while(0)
    #define SENSOR_UNLOCK()     do { __set_PRIMASK(irq_state); } while(0)
#endif

/* 字符串比较 */
static int sensor_strcmp(const char *s1, const char *s2)
{
    if (s1 == NULL || s2 == NULL) {
        return -1;
    }
    return strcmp(s1, s2);
}

/**
 * @brief 注册传感器设备
 */
sensor_err_t sensor_register(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL) {
        return SENSOR_EINVAL;
    }
    
    /* 检查必须的操作接口 */
    if (sensor->ops->init == NULL || sensor->ops->read == NULL) {
        return SENSOR_EINVAL;
    }
    
    SENSOR_LOCK();
    
    /* 检查是否已存在 */
    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (sensor_strcmp(dev->info.name, sensor->info.name) == 0) {
            SENSOR_UNLOCK();
            return SENSOR_ERROR;
        }
        dev = dev->next;
    }
    
    /* 添加到链表头 */
    sensor->next = sensor_list_head;
    sensor_list_head = sensor;
    sensor->status = SENSOR_STATUS_IDLE;
    
    SENSOR_UNLOCK();
    
    return SENSOR_EOK;
}

/**
 * @brief 注销传感器设备
 */
sensor_err_t sensor_unregister(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }
    
    SENSOR_LOCK();
    
    sensor_device_t *prev = NULL;
    sensor_device_t *curr = sensor_list_head;
    
    while (curr != NULL) {
        if (curr == sensor) {
            if (prev == NULL) {
                sensor_list_head = curr->next;
            } else {
                prev->next = curr->next;
            }
            sensor->status = SENSOR_STATUS_IDLE;
            SENSOR_UNLOCK();
            return SENSOR_EOK;
        }
        prev = curr;
        curr = curr->next;
    }
    
    SENSOR_UNLOCK();
    return SENSOR_ENODEV;
}

/**
 * @brief 根据名称查找传感器
 */
sensor_device_t* sensor_find_by_name(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    
    SENSOR_LOCK();
    
    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (sensor_strcmp(dev->info.name, name) == 0) {
            SENSOR_UNLOCK();
            return dev;
        }
        dev = dev->next;
    }
    
    SENSOR_UNLOCK();
    return NULL;
}

/**
 * @brief 根据类型查找传感器
 */
sensor_device_t* sensor_find_by_type(sensor_type_t type)
{
    SENSOR_LOCK();
    
    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (dev->info.type == type) {
            SENSOR_UNLOCK();
            return dev;
        }
        dev = dev->next;
    }
    
    SENSOR_UNLOCK();
    return NULL;
}

/**
 * @brief 初始化传感器
 */
sensor_err_t sensor_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL || sensor->ops->init == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->status != SENSOR_STATUS_IDLE) {
        return SENSOR_EBUSY;
    }
    
    sensor_err_t ret = sensor->ops->init(sensor);
    if (ret == SENSOR_EOK) {
        sensor->status = SENSOR_STATUS_READY;
    }
    
    return ret;
}

/**
 * @brief 反初始化传感器
 */
sensor_err_t sensor_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL || sensor->ops->deinit == NULL) {
        return SENSOR_EINVAL;
    }
    
    sensor_err_t ret = sensor->ops->deinit(sensor);
    if (ret == SENSOR_EOK) {
        sensor->status = SENSOR_STATUS_IDLE;
    }
    
    return ret;
}

/**
 * @brief 使能/失能传感器
 */
sensor_err_t sensor_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->ops->enable != NULL) {
        return sensor->ops->enable(sensor, enable);
    }
    
    return SENSOR_ENOSYS;
}

/**
 * @brief 读取传感器数据
 */
sensor_err_t sensor_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->status != SENSOR_STATUS_READY) {
        return SENSOR_ERROR;
    }
    
    if (sensor->ops->read == NULL) {
        return SENSOR_ENOSYS;
    }
    
    sensor->status = SENSOR_STATUS_BUSY;
    sensor_err_t ret = sensor->ops->read(sensor, data);
    sensor->status = SENSOR_STATUS_READY;
    
    /* 调用回调函数 */
    if (ret == SENSOR_EOK && sensor->callback != NULL) {
        sensor->callback(sensor, data, sensor->user_data);
    }
    
    return ret;
}

/**
 * @brief 写入传感器数据
 */
sensor_err_t sensor_write(sensor_device_t *sensor, const sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->ops->write == NULL) {
        return SENSOR_ENOSYS;
    }
    
    return sensor->ops->write(sensor, data);
}

/**
 * @brief 配置传感器参数
 */
sensor_err_t sensor_config(sensor_device_t *sensor, sensor_config_type_t cfg, void *value)
{
    if (sensor == NULL || value == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->ops->config == NULL) {
        return SENSOR_ENOSYS;
    }
    
    return sensor->ops->config(sensor, cfg, value);
}

/**
 * @brief 控制传感器
 */
sensor_err_t sensor_control(sensor_device_t *sensor, int cmd, void *args)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }
    
    if (sensor->ops->control == NULL) {
        return SENSOR_ENOSYS;
    }
    
    return sensor->ops->control(sensor, cmd, args);
}

/**
 * @brief 设置数据回调函数
 */
sensor_err_t sensor_set_callback(sensor_device_t *sensor, sensor_callback_t callback, void *user_data)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }
    
    sensor->callback = callback;
    sensor->user_data = user_data;
    
    return SENSOR_EOK;
}

/**
 * @brief 获取传感器信息
 */
const sensor_info_t* sensor_get_info(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return NULL;
    }
    
    return &sensor->info;
}

/**
 * @brief 获取传感器状态
 */
sensor_status_t sensor_get_status(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_STATUS_ERROR;
    }
    
    return sensor->status;
}
```

## 3. 示例驱动实现

### sensor_mpu6050.h - MPU6050 驱动示例

```c
#ifndef __SENSOR_MPU6050_H__
#define __SENSOR_MPU6050_H__

#include "sensor_core.h"

/* MPU6050 设备地址 */
#define MPU6050_ADDR_DEFAULT    0x68

/* MPU6050 寄存器定义 */
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C

/* MPU6050 私有数据 */
typedef struct {
    uint8_t     i2c_addr;
    uint8_t     accel_range;    /* 加速度量程 */
    uint8_t     gyro_range;     /* 陀螺仪量程 */
    int16_t     accel_offset[3]; /* 加速度偏移 */
    int16_t     gyro_offset[3];  /* 陀螺仪偏移 */
} mpu6050_priv_t;

/**
 * @brief 创建 MPU6050 加速度计设备
 * @param name 设备名称
 * @param i2c_bus I2C总线句柄
 * @return 传感器设备指针
 */
sensor_device_t* mpu6050_create_accel(const char *name, void *i2c_bus);

/**
 * @brief 创建 MPU6050 陀螺仪设备
 * @param name 设备名称
 * @param i2c_bus I2C总线句柄
 * @return 传感器设备指针
 */
sensor_device_t* mpu6050_create_gyro(const char *name, void *i2c_bus);

#endif /* __SENSOR_MPU6050_H__ */
```

### sensor_mpu6050.c - MPU6050 驱动实现

```c
#include "sensor_mpu6050.h"
#include <stdlib.h>

/* I2C 读写接口 (需要根据实际HAL适配) */
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/* MPU6050 初始化 */
static sensor_err_t mpu6050_init(sensor_device_t *sensor)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t data;
    
    /* 唤醒设备 */
    data = 0x00;
    if (hal_i2c_mem_write(sensor->bus, priv->i2c_addr, MPU6050_REG_PWR_MGMT_1, &data, 1) != 0) {
        return SENSOR_EIO;
    }
    
    /* 配置加速度计量程 ±2g */
    data = 0x00;
    if (hal_i2c_mem_write(sensor->bus, priv->i2c_addr, MPU6050_REG_ACCEL_CONFIG, &data, 1) != 0) {
        return SENSOR_EIO;
    }
    
    /* 配置陀螺仪量程 ±250°/s */
    data = 0x00;
    if (hal_i2c_mem_write(sensor->bus, priv->i2c_addr, MPU6050_REG_GYRO_CONFIG, &data, 1) != 0) {
        return SENSOR_EIO;
    }
    
    return SENSOR_EOK;
}

/* MPU6050 反初始化 */
static sensor_err_t mpu6050_deinit(sensor_device_t *sensor)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t data = 0x40; /* 进入睡眠模式 */
    
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, MPU6050_REG_PWR_MGMT_1, &data, 1);
    
    return SENSOR_EOK;
}

/* 读取加速度数据 */
static sensor_err_t mpu6050_accel_read(sensor_device_t *sensor, sensor_data_t *data)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];
    
    /* 读取6字节加速度数据 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, MPU6050_REG_ACCEL_XOUT_H, buf, 6) != 0) {
        return SENSOR_EIO;
    }
    
    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);
    
    /* 应用偏移校准 */
    raw[0] -= priv->accel_offset[0];
    raw[1] -= priv->accel_offset[1];
    raw[2] -= priv->accel_offset[2];
    
    /* 填充数据结构 */
    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = raw[0];
    data->value.val_3axis.y = raw[1];
    data->value.val_3axis.z = raw[2];
    data->timestamp = 0; /* 需要实现时间戳获取 */
    
    return SENSOR_EOK;
}

/* 读取陀螺仪数据 */
static sensor_err_t mpu6050_gyro_read(sensor_device_t *sensor, sensor_data_t *data)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];
    
    /* 读取6字节陀螺仪数据 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, MPU6050_REG_GYRO_XOUT_H, buf, 6) != 0) {
        return SENSOR_EIO;
    }
    
    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);
    
    /* 应用偏移校准 */
    raw[0] -= priv->gyro_offset[0];
    raw[1] -= priv->gyro_offset[1];
    raw[2] -= priv->gyro_offset[2];
    
    /* 填充数据结构 */
    data->type = SENSOR_TYPE_GYROSCOPE;
    data->unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = raw[0];
    data->value.val_3axis.y = raw[1];
    data->value.val_3axis.z = raw[2];
    data->timestamp = 0;
    
    return SENSOR_EOK;
}

/* 加速度计操作接口 */
static const sensor_ops_t mpu6050_accel_ops = {
    .init = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read = mpu6050_accel_read,
};

/* 陀螺仪操作接口 */
static const sensor_ops_t mpu6050_gyro_ops = {
    .init = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read = mpu6050_gyro_read,
};

/**
 * @brief 创建 MPU6050 加速度计设备
 */
sensor_device_t* mpu6050_create_accel(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)malloc(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }
    
    mpu6050_priv_t *priv = (mpu6050_priv_t *)malloc(sizeof(mpu6050_priv_t));
    if (priv == NULL) {
        free(sensor);
        return NULL;
    }
    
    /* 初始化私有数据 */
    priv->i2c_addr = MPU6050_ADDR_DEFAULT;
    priv->accel_range = 0;
    
    /* 设置设备信息 */
    sensor->info.name = name;
    sensor->info.vendor = "InvenSense";
    sensor->info.model = "MPU6050";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;  /* ±2g = ±2000mg */
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 1000;
    
    sensor->ops = &mpu6050_accel_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    
    return sensor;
}

/**
 * @brief 创建 MPU6050 陀螺仪设备
 */
sensor_device_t* mpu6050_create_gyro(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)malloc(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }
    
    mpu6050_priv_t *priv = (mpu6050_priv_t *)malloc(sizeof(mpu6050_priv_t));
    if (priv == NULL) {
        free(sensor);
        return NULL;
    }
    
    /* 初始化私有数据 */
    priv->i2c_addr = MPU6050_ADDR_DEFAULT;
    priv->gyro_range = 0;
    
    /* 设置设备信息 */
    sensor->info.name = name;
    sensor->info.vendor = "InvenSense";
    sensor->info.model = "MPU6050";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max = 250;
    sensor->info.range_min = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 8000;
    
    sensor->ops = &mpu6050_gyro_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    
    return sensor;
}
```

## 4. 使用示例

### main.c - 应用示例

```c
#include "sensor_core.h"
#include "sensor_mpu6050.h"
#include <stdio.h>

/* 数据回调函数 */
void sensor_data_callback(sensor_device_t *sensor, sensor_data_t *data, void *user_data)
{
    printf("[%s] X=%d, Y=%d, Z=%d\n", 
           sensor->info.name,
           data->value.val_3axis.x,
           data->value.val_3axis.y,
           data->value.val_3axis.z);
}

int main(void)
{
    sensor_device_t *accel, *gyro;
    sensor_data_t data;
    sensor_err_t ret;
    void *i2c_bus; /* I2C总线句柄,需要根据实际情况初始化 */
    
    /* 创建传感器设备 */
    accel = mpu6050_create_accel("accel0", i2c_bus);
    gyro = mpu6050_create_gyro("gyro0", i2c_bus);
    
    if (accel == NULL || gyro == NULL) {
        printf("Failed to create sensor devices\n");
        return -1;
    }
    
    /* 注册设备 */
    sensor_register(accel);
    sensor_register(gyro);
    
    /* 初始化设备 */
    ret = sensor_init(accel);
    if (ret != SENSOR_EOK) {
        printf("Failed to init accelerometer: %d\n", ret);
        return -1;
    }
    
    ret = sensor_init(gyro);
    if (ret != SENSOR_EOK) {
        printf("Failed to init gyroscope: %d\n", ret);
        return -1;
    }
    
    /* 设置回调函数 */
    sensor_set_callback(accel, sensor_data_callback, NULL);
    
    /* 主循环 */
    while (1) {
        /* 读取加速度计数据 */
        ret = sensor_read(accel, &data);
        if (ret == SENSOR_EOK) {
            printf("Accel: X=%d mg, Y=%d mg, Z=%d mg\n",
                   data.value.val_3axis.x,
                   data.value.val_3axis.y,
                   data.value.val_3axis.z);
        }
        
        /* 读取陀螺仪数据 */
        ret = sensor_read(gyro, &data);
        if (ret == SENSOR_EOK) {
            printf("Gyro: X=%d °/s, Y=%d °/s, Z=%d °/s\n",
                   data.value.val_3axis.x,
                   data.value.val_3axis.y,
                   data.value.val_3axis.z);
        }
        
        /* 延时 */
        delay_ms(100);
    }
    
    return 0;
}
```

## 5. 框架特点

### 优势

1. **轻量级设计**：最小内存占用，适合资源受限的嵌入式系统
2. **灵活适配**：可配合OSAL或运行在裸机环境
3. **统一接口**：参考RT-Thread和Zephyr，提供标准化API
4. **扩展性强**：易于添加新的传感器类型和驱动
5. **类型丰富**：支持多种传感器类型和单位定义
6. **回调机制**：支持数据回调，方便事件驱动设计

### 可扩展功能

- FIFO缓冲支持
- 中断触发模式
- DMA数据传输
- 传感器融合算法
- 功耗管理
- 动态校准

这个框架可以根据您的具体需求进一步定制和优化！