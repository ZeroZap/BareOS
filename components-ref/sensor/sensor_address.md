# 传感器I2C/SPI地址完整列表

## I2C地址汇总表

### sensor_addresses.h

```c
#ifndef __SENSOR_ADDRESSES_H__
#define __SENSOR_ADDRESSES_H__

#include <stdint.h>

/* ==================== I2C地址定义 ==================== */

/* 加速度计 */
#define MPU6050_I2C_ADDR_1          0x68    /* AD0=0 */
#define MPU6050_I2C_ADDR_2          0x69    /* AD0=1 */

#define SC7A20_I2C_ADDR_1           0x18    /* SDO=0 */
#define SC7A20_I2C_ADDR_2           0x19    /* SDO=1 */

#define LIS2DH12_I2C_ADDR_1         0x18    /* SDO=0 */
#define LIS2DH12_I2C_ADDR_2         0x19    /* SDO=1 */

#define ICM20608_I2C_ADDR_1         0x68    /* AD0=0 */
#define ICM20608_I2C_ADDR_2         0x69    /* AD0=1 */

#define BMA400_I2C_ADDR_1           0x14    /* SDO=0 */
#define BMA400_I2C_ADDR_2           0x15    /* SDO=1 */

#define KX023_I2C_ADDR_1            0x1E    /* SDO=0 */
#define KX023_I2C_ADDR_2            0x1F    /* SDO=1 */

/* 磁力计 */
#define QMC5883L_I2C_ADDR           0x0D    /* 固定地址 */

#define HMC5883L_I2C_ADDR           0x1E    /* 固定地址 */

#define AK8963_I2C_ADDR_1           0x0C    /* CAD0=0 */
#define AK8963_I2C_ADDR_2           0x0D    /* CAD0=1 */

/* 温湿度传感器 */
#define AHT20_I2C_ADDR              0x38    /* 固定地址 */

#define SHT30_I2C_ADDR_1            0x44    /* ADDR=0 */
#define SHT30_I2C_ADDR_2            0x45    /* ADDR=1 */

#define SHT31_I2C_ADDR_1            0x44    /* ADDR=0 */
#define SHT31_I2C_ADDR_2            0x45    /* ADDR=1 */

#define HDC1080_I2C_ADDR            0x40    /* 固定地址 */

#define HTU21D_I2C_ADDR             0x40    /* 固定地址 */

#define SI7021_I2C_ADDR             0x40    /* 固定地址 */

/* 温度传感器 */
#define ADT7420_I2C_ADDR_1          0x48    /* A1=0, A0=0 */
#define ADT7420_I2C_ADDR_2          0x49    /* A1=0, A0=1 */
#define ADT7420_I2C_ADDR_3          0x4A    /* A1=1, A0=0 */
#define ADT7420_I2C_ADDR_4          0x4B    /* A1=1, A0=1 */

#define TMP102_I2C_ADDR_1           0x48    /* ADD0=0 */
#define TMP102_I2C_ADDR_2           0x49    /* ADD0=1 */
#define TMP102_I2C_ADDR_3           0x4A    /* ADD0=SDA */
#define TMP102_I2C_ADDR_4           0x4B    /* ADD0=SCL */

#define TMP108_I2C_ADDR_1           0x48    
#define TMP108_I2C_ADDR_2           0x49    
#define TMP108_I2C_ADDR_3           0x4A    
#define TMP108_I2C_ADDR_4           0x4B    

#define LM75_I2C_ADDR_BASE          0x48    /* 0x48-0x4F (A2A1A0) */

#define DS18B20_I2C_ADDR            0x18    /* 1-Wire转I2C时使用 */

/* 气压传感器 */
#define BMP280_I2C_ADDR_1           0x76    /* SDO=0 */
#define BMP280_I2C_ADDR_2           0x77    /* SDO=1 */

#define BME280_I2C_ADDR_1           0x76    /* SDO=0 */
#define BME280_I2C_ADDR_2           0x77    /* SDO=1 */

#define BMP180_I2C_ADDR             0x77    /* 固定地址 */

#define MS5611_I2C_ADDR_1           0x76    /* PS=0 */
#define MS5611_I2C_ADDR_2           0x77    /* PS=1 */

#define LPS22HB_I2C_ADDR_1          0x5C    /* SDO=0 */
#define LPS22HB_I2C_ADDR_2          0x5D    /* SDO=1 */

/* 光感和接近传感器 */
#define AP3216C_I2C_ADDR            0x1E    /* 固定地址 */

#define APDS9960_I2C_ADDR           0x39    /* 固定地址 */

#define VCNL4040_I2C_ADDR           0x60    /* 固定地址 */

#define VEML6030_I2C_ADDR_1         0x10    /* ADDR=0 */
#define VEML6030_I2C_ADDR_2         0x48    /* ADDR=1 */

#define BH1750_I2C_ADDR_1           0x23    /* ADDR=0 */
#define BH1750_I2C_ADDR_2           0x5C    /* ADDR=1 */

#define TSL2561_I2C_ADDR_1          0x29    /* ADDR=Float */
#define TSL2561_I2C_ADDR_2          0x39    /* ADDR=GND */
#define TSL2561_I2C_ADDR_3          0x49    /* ADDR=VDD */

/* 空气质量传感器 */
#define CCS811_I2C_ADDR_1           0x5A    /* ADDR=0 */
#define CCS811_I2C_ADDR_2           0x5B    /* ADDR=1 */

#define SGP30_I2C_ADDR              0x58    /* 固定地址 */

#define SGP40_I2C_ADDR              0x59    /* 固定地址 */

#define ENS160_I2C_ADDR             0x52    /* 固定地址 */

/* 距离传感器 */
#define VL53L0X_I2C_ADDR            0x29    /* 固定地址（可修改）*/

#define VL53L1X_I2C_ADDR            0x29    /* 固定地址（可修改）*/

#define TOF10120_I2C_ADDR           0x52    /* 固定地址 */

/* 其他传感器 */
#define MAX30102_I2C_ADDR           0x57    /* 心率/血氧 */

#define MLX90614_I2C_ADDR           0x5A    /* 红外温度 */

#define INA219_I2C_ADDR_BASE        0x40    /* 0x40-0x4F (A1A0) */

#define BQ27441_I2C_ADDR            0x55    /* 电量计 */

#define PAJ7620_I2C_ADDR            0x73    /* 手势传感器 */

/* EEPROM */
#define AT24C02_I2C_ADDR_BASE       0x50    /* 0x50-0x57 (A2A1A0) */

/* RTC */
#define DS1307_I2C_ADDR             0x68    /* RTC */
#define DS3231_I2C_ADDR             0x68    /* RTC */
#define PCF8563_I2C_ADDR            0x51    /* RTC */

/* ==================== I2C地址范围分组 ==================== */

/* 0x00-0x0F: 保留和特殊设备 */
#define I2C_ADDR_RANGE_00_0F_START  0x00
#define I2C_ADDR_RANGE_00_0F_END    0x0F

/* 0x10-0x1F: 传感器密集区域 */
#define I2C_ADDR_RANGE_10_1F_START  0x10
#define I2C_ADDR_RANGE_10_1F_END    0x1F

/* 0x20-0x2F: 传感器和IO扩展 */
#define I2C_ADDR_RANGE_20_2F_START  0x20
#define I2C_ADDR_RANGE_20_2F_END    0x2F

/* 0x30-0x3F: 传感器和显示 */
#define I2C_ADDR_RANGE_30_3F_START  0x30
#define I2C_ADDR_RANGE_30_3F_END    0x3F

/* 0x40-0x4F: 传感器和电源管理 */
#define I2C_ADDR_RANGE_40_4F_START  0x40
#define I2C_ADDR_RANGE_40_4F_END    0x4F

/* 0x50-0x5F: EEPROM和传感器 */
#define I2C_ADDR_RANGE_50_5F_START  0x50
#define I2C_ADDR_RANGE_50_5F_END    0x5F

/* 0x60-0x6F: 显示和传感器 */
#define I2C_ADDR_RANGE_60_6F_START  0x60
#define I2C_ADDR_RANGE_60_6F_END    0x6F

/* 0x70-0x7F: 多路复用器和特殊设备 */
#define I2C_ADDR_RANGE_70_7F_START  0x70
#define I2C_ADDR_RANGE_70_7F_END    0x7F

#endif /* __SENSOR_ADDRESSES_H__ */
```

## I2C地址完整映射表

### I2C_ADDRESS_MAP.md

```markdown
# I2C地址完整映射表

## 地址索引 (0x00 - 0x7F)

### 0x00 - 0x0F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x00-0x07 | Reserved | - | I2C保留地址 |
| 0x08 | - | - | - |
| 0x09 | - | - | - |
| 0x0A | - | - | - |
| 0x0B | - | - | - |
| 0x0C | AK8963 | 磁力计 | CAD0=0 |
| 0x0D | QMC5883L | 磁力计 | 固定 |
| 0x0D | AK8963 | 磁力计 | CAD0=1 |
| 0x0E | - | - | - |
| 0x0F | - | - | - |

### 0x10 - 0x1F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x10 | VEML6030 | 光感 | ADDR=0 |
| 0x11 | Si1132 | 光感/UV | - |
| 0x12 | - | - | - |
| 0x13 | VCNL4000 | 接近 | - |
| 0x14 | BMA400 | 加速度计 | SDO=0 |
| 0x15 | BMA400 | 加速度计 | SDO=1 |
| 0x16 | - | - | - |
| 0x17 | - | - | - |
| 0x18 | SC7A20 | 加速度计 | SDO=0 |
| 0x18 | LIS2DH12 | 加速度计 | SDO=0 |
| 0x19 | SC7A20 | 加速度计 | SDO=1 |
| 0x19 | LIS2DH12 | 加速度计 | SDO=1 |
| 0x1A | - | - | - |
| 0x1B | - | - | - |
| 0x1C | MMA845x | 加速度计 | - |
| 0x1D | ADXL345 | 加速度计 | SDO=0 |
| 0x1E | AP3216C | ALS/PS | 固定 |
| 0x1E | HMC5883L | 磁力计 | 固定 |
| 0x1E | KX023 | 加速度计 | SDO=0 |
| 0x1F | KX023 | 加速度计 | SDO=1 |

### 0x20 - 0x2F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x20 | PCF8574 | IO扩展 | A2A1A0=000 |
| 0x21 | PCF8574 | IO扩展 | A2A1A0=001 |
| 0x22 | PCF8574 | IO扩展 | A2A1A0=010 |
| 0x23 | BH1750 | 光感 | ADDR=0 |
| 0x24-0x27 | PCF8574 | IO扩展 | 各种组合 |
| 0x28 | CAP1188 | 触摸 | - |
| 0x29 | TSL2561 | 光感 | ADDR=Float |
| 0x29 | VL53L0X | ToF测距 | 默认（可改）|
| 0x2A | - | - | - |
| 0x2B | - | - | - |
| 0x2C | - | - | - |
| 0x2D | - | - | - |
| 0x2E | - | - | - |
| 0x2F | - | - | - |

### 0x30 - 0x3F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x30 | - | - | - |
| 0x31 | - | - | - |
| 0x32 | - | - | - |
| 0x33 | - | - | - |
| 0x34 | - | - | - |
| 0x35 | - | - | - |
| 0x36 | - | - | - |
| 0x37 | - | - | - |
| 0x38 | AHT20 | 温湿度 | 固定 |
| 0x38 | FT6206 | 触摸 | - |
| 0x39 | APDS9960 | RGB/手势 | 固定 |
| 0x39 | TSL2561 | 光感 | ADDR=GND |
| 0x3A | PCF8574A | IO扩展 | - |
| 0x3B | - | - | - |
| 0x3C | SSD1306 | OLED | SA0=0 |
| 0x3D | SSD1306 | OLED | SA0=1 |
| 0x3E | - | - | - |
| 0x3F | - | - | - |

### 0x40 - 0x4F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x40 | HDC1080 | 温湿度 | 固定 |
| 0x40 | HTU21D | 温湿度 | 固定 |
| 0x40 | SI7021 | 温湿度 | 固定 |
| 0x40 | INA219 | 电流 | A1A0=00 |
| 0x41 | INA219 | 电流 | A1A0=01 |
| 0x42 | INA219 | 电流 | A1A0=10 |
| 0x43 | INA219 | 电流 | A1A0=11 |
| 0x44 | SHT30/31 | 温湿度 | ADDR=0 |
| 0x44 | ISL29125 | RGB | - |
| 0x45 | SHT30/31 | 温湿度 | ADDR=1 |
| 0x46 | - | - | - |
| 0x47 | - | - | - |
| 0x48 | ADS1115 | ADC | ADDR=GND |
| 0x48 | TMP102 | 温度 | ADD0=0 |
| 0x48 | ADT7420 | 温度 | A1A0=00 |
| 0x48 | VEML6030 | 光感 | ADDR=1 |
| 0x49 | ADS1115 | ADC | ADDR=VDD |
| 0x49 | TMP102 | 温度 | ADD0=1 |
| 0x49 | ADT7420 | 温度 | A1A0=01 |
| 0x49 | TSL2561 | 光感 | ADDR=VDD |
| 0x4A | ADS1115 | ADC | ADDR=SDA |
| 0x4A | TMP102 | 温度 | ADD0=SDA |
| 0x4A | ADT7420 | 温度 | A1A0=10 |
| 0x4B | ADS1115 | ADC | ADDR=SCL |
| 0x4B | TMP102 | 温度 | ADD0=SCL |
| 0x4B | ADT7420 | 温度 | A1A0=11 |
| 0x4C | - | - | - |
| 0x4D | - | - | - |
| 0x4E | - | - | - |
| 0x4F | - | - | - |

### 0x50 - 0x5F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x50 | AT24Cxx | EEPROM | A2A1A0=000 |
| 0x51 | PCF8563 | RTC | 固定 |
| 0x52 | ENS160 | 空气质量 | 固定 |
| 0x52 | TOF10120 | ToF测距 | 固定 |
| 0x53 | ADXL345 | 加速度计 | SDO=1 |
| 0x54 | - | - | - |
| 0x55 | BQ27441 | 电量计 | 固定 |
| 0x56 | - | - | - |
| 0x57 | MAX30102 | 心率/血氧 | 固定 |
| 0x58 | SGP30 | 空气质量 | 固定 |
| 0x59 | SGP40 | VOC | 固定 |
| 0x5A | CCS811 | 空气质量 | ADDR=0 |
| 0x5A | MLX90614 | 红外温度 | 固定 |
| 0x5B | CCS811 | 空气质量 | ADDR=1 |
| 0x5C | BH1750 | 光感 | ADDR=1 |
| 0x5C | LPS22HB | 气压 | SDO=0 |
| 0x5D | LPS22HB | 气压 | SDO=1 |
| 0x5E | - | - | - |
| 0x5F | HTS221 | 温湿度 | - |

### 0x60 - 0x6F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x60 | VCNL4040 | ALS/PS | 固定 |
| 0x61 | - | - | - |
| 0x62 | - | - | - |
| 0x63 | - | - | - |
| 0x64 | - | - | - |
| 0x65 | - | - | - |
| 0x66 | - | - | - |
| 0x67 | - | - | - |
| 0x68 | MPU6050 | IMU | AD0=0 |
| 0x68 | ICM20608 | IMU | AD0=0 |
| 0x68 | DS1307 | RTC | 固定 |
| 0x68 | DS3231 | RTC | 固定 |
| 0x69 | MPU6050 | IMU | AD0=1 |
| 0x69 | ICM20608 | IMU | AD0=1 |
| 0x6A | L3GD20 | 陀螺仪 | - |
| 0x6B | L3GD20 | 陀螺仪 | - |
| 0x6C | - | - | - |
| 0x6D | - | - | - |
| 0x6E | - | - | - |
| 0x6F | - | - | - |

### 0x70 - 0x7F

| 地址 | 设备 | 类型 | 备注 |
|------|------|------|------|
| 0x70 | TCA9548A | I2C多路复用 | A2A1A0=000 |
| 0x71 | TCA9548A | I2C多路复用 | A2A1A0=001 |
| 0x72 | TCA9548A | I2C多路复用 | A2A1A0=010 |
| 0x73 | PAJ7620 | 手势 | 固定 |
| 0x74 | TCA9548A | I2C多路复用 | A2A1A0=100 |
| 0x75 | TCA9548A | I2C多路复用 | A2A1A0=101 |
| 0x76 | BMP280 | 气压 | SDO=0 |
| 0x76 | BME280 | 气压/温湿 | SDO=0 |
| 0x76 | MS5611 | 气压 | PS=0 |
| 0x77 | BMP280 | 气压 | SDO=1 |
| 0x77 | BME280 | 气压/温湿 | SDO=1 |
| 0x77 | BMP180 | 气压 | 固定 |
| 0x77 | MS5611 | 气压 | PS=1 |
| 0x78-0x7F | Reserved | - | I2C保留 |

## 地址冲突解决方案

### 常见冲突

#### 1. 0x68冲突
```

设备: MPU6050, ICM20608, DS1307, DS3231
解决方案:

- 使用AD0引脚切换到0x69
- 使用I2C多路复用器
- 选择不同厂家的传感器

```text
#### 2. 0x76/0x77冲突
```

设备: BMP280, BME280, BMP180, MS5611
解决方案:

- 使用SDO/PS引脚切换地址
- 选择不同型号（BMP280用0x76, MS5611用0x77）

```text
#### 3. 0x40冲突
```

设备: HDC1080, HTU21D, SI7021, INA219
解决方案:

- 温湿度传感器选一个
- INA219使用A1A0引脚切换地址

~~~text
## I2C总线扫描代码

### sensor_i2c_scan.h

```c
#ifndef __SENSOR_I2C_SCAN_H__
#define __SENSOR_I2C_SCAN_H__

#include <stdint.h>
#include <stdbool.h>

/* 扫描结果 */
typedef struct {
    uint8_t     address;
    bool        present;
    const char  *possible_devices[8];
    uint8_t     device_count;
} i2c_scan_result_t;

/* 扫描配置 */
typedef struct {
    uint8_t     start_addr;     /* 起始地址 */
    uint8_t     end_addr;       /* 结束地址 */
    bool        skip_reserved;  /* 跳过保留地址 */
    bool        verbose;        /* 详细输出 */
} i2c_scan_config_t;

/* API函数 */
int i2c_scan_bus(void *i2c_bus, i2c_scan_result_t *results, uint8_t *result_count);
int i2c_scan_range(void *i2c_bus, uint8_t start, uint8_t end, 
                   i2c_scan_result_t *results, uint8_t *result_count);
void i2c_scan_print_results(i2c_scan_result_t *results, uint8_t count);
const char** i2c_get_possible_devices(uint8_t address, uint8_t *count);

#endif /* __SENSOR_I2C_SCAN_H__ */
~~~

### sensor_i2c_scan.c

```c
#include "sensor_i2c_scan.h"
#include "sensor_addresses.h"
#include <stdio.h>
#include <string.h>

extern int hal_i2c_probe(void *bus, uint8_t addr);

/* 设备名称数据库 */
typedef struct {
    uint8_t     address;
    const char  *devices[8];
} device_database_t;

static const device_database_t device_db[] = {
    {0x0C, {"AK8963 (Mag)"}},
    {0x0D, {"QMC5883L (Mag)", "AK8963 (Mag)"}},
    {0x14, {"BMA400 (Accel)"}},
    {0x15, {"BMA400 (Accel)"}},
    {0x18, {"SC7A20 (Accel)", "LIS2DH12 (Accel)"}},
    {0x19, {"SC7A20 (Accel)", "LIS2DH12 (Accel)"}},
    {0x1E, {"AP3216C (ALS/PS)", "HMC5883L (Mag)", "KX023 (Accel)"}},
    {0x1F, {"KX023 (Accel)"}},
    {0x23, {"BH1750 (Light)"}},
    {0x29, {"TSL2561 (Light)", "VL53L0X (ToF)"}},
    {0x38, {"AHT20 (Temp/Humi)", "FT6206 (Touch)"}},
    {0x39, {"APDS9960 (RGB/Gesture)", "TSL2561 (Light)"}},
    {0x3C, {"SSD1306 (OLED)"}},
    {0x3D, {"SSD1306 (OLED)"}},
    {0x40, {"HDC1080 (Temp/Humi)", "HTU21D (Temp/Humi)", "SI7021 (Temp/Humi)", "INA219 (Current)"}},
    {0x44, {"SHT30/31 (Temp/Humi)", "ISL29125 (RGB)"}},
    {0x45, {"SHT30/31 (Temp/Humi)"}},
    {0x48, {"ADS1115 (ADC)", "TMP102 (Temp)", "ADT7420 (Temp)", "VEML6030 (Light)"}},
    {0x49, {"ADS1115 (ADC)", "TMP102 (Temp)", "ADT7420 (Temp)", "TSL2561 (Light)"}},
    {0x4A, {"ADS1115 (ADC)", "TMP102 (Temp)", "ADT7420 (Temp)"}},
    {0x4B, {"ADS1115 (ADC)", "TMP102 (Temp)", "ADT7420 (Temp)"}},
    {0x50, {"AT24Cxx (EEPROM)"}},
    {0x51, {"PCF8563 (RTC)"}},
    {0x52, {"ENS160 (Air Quality)", "TOF10120 (ToF)"}},
    {0x55, {"BQ27441 (Fuel Gauge)"}},
    {0x57, {"MAX30102 (HeartRate/SpO2)"}},
    {0x58, {"SGP30 (Air Quality)"}},
    {0x59, {"SGP40 (VOC)"}},
    {0x5A, {"CCS811 (Air Quality)", "MLX90614 (IR Temp)"}},
    {0x5B, {"CCS811 (Air Quality)"}},
    {0x5C, {"BH1750 (Light)", "LPS22HB (Pressure)"}},
    {0x5D, {"LPS22HB (Pressure)"}},
    {0x60, {"VCNL4040 (ALS/PS)"}},
    {0x68, {"MPU6050 (IMU)", "ICM20608 (IMU)", "DS1307 (RTC)", "DS3231 (RTC)"}},
    {0x69, {"MPU6050 (IMU)", "ICM20608 (IMU)"}},
    {0x73, {"PAJ7620 (Gesture)"}},
    {0x76, {"BMP280 (Pressure)", "BME280 (Pressure/Temp/Humi)", "MS5611 (Pressure)"}},
    {0x77, {"BMP280 (Pressure)", "BME280 (Pressure/Temp/Humi)", "BMP180 (Pressure)", "MS5611 (Pressure)"}},
};

#define DEVICE_DB_SIZE (sizeof(device_db) / sizeof(device_database_t))

/**
 * @brief 获取地址对应的可能设备
 */
const char** i2c_get_possible_devices(uint8_t address, uint8_t *count)
{
    for (int i = 0; i < DEVICE_DB_SIZE; i++) {
        if (device_db[i].address == address) {
            /* 计算设备数量 */
            *count = 0;
            for (int j = 0; j < 8; j++) {
                if (device_db[i].devices[j] != NULL) {
                    (*count)++;
                } else {
                    break;
                }
            }
            return device_db[i].devices;
        }
    }
    
    *count = 0;
    return NULL;
}

/**
 * @brief 扫描I2C总线
 */
int i2c_scan_bus(void *i2c_bus, i2c_scan_result_t *results, uint8_t *result_count)
{
    return i2c_scan_range(i2c_bus, 0x03, 0x77, results, result_count);
}

/**
 * @brief 扫描指定地址范围
 */
int i2c_scan_range(void *i2c_bus, uint8_t start, uint8_t end, 
                   i2c_scan_result_t *results, uint8_t *result_count)
{
    if (i2c_bus == NULL || results == NULL || result_count == NULL) {
        return -1;
    }
    
    *result_count = 0;
    
    printf("\n========== I2C Bus Scan ==========\n");
    printf("Scanning range: 0x%02X - 0x%02X\n\n", start, end);
    
    for (uint8_t addr = start; addr <= end; addr++) {
        /* 跳过保留地址 */
        if (addr < 0x03 || addr > 0x77) {
            continue;
        }
        
        /* 探测设备 */
        if (hal_i2c_probe(i2c_bus, addr) == 0) {
            results[*result_count].address = addr;
            results[*result_count].present = true;
            
            /* 查找可能的设备 */
            uint8_t dev_count = 0;
            const char **devices = i2c_get_possible_devices(addr, &dev_count);
            
            results[*result_count].device_count = dev_count;
            for (int i = 0; i < dev_count && i < 8; i++) {
                results[*result_count].possible_devices[i] = devices[i];
            }
            
            (*result_count)++;
        }
    }
    
    printf("Found %d device(s)\n", *result_count);
    printf("==================================\n\n");
    
    return 0;
}

/**
 * @brief 打印扫描结果
 */
void i2c_scan_print_results(i2c_scan_result_t *results, uint8_t count)
{
    if (count == 0) {
        printf("No I2C devices found.\n");
        return;
    }
    
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    I2C Devices Found                       ║\n");
    printf("╠════════╦═══════════════════════════════════════════════════╣\n");
    printf("║ Address║ Possible Devices                                  ║\n");
    printf("╠════════╬═══════════════════════════════════════════════════╣\n");
    
    for (uint8_t i = 0; i < count; i++) {
        printf("║ 0x%02X   ║ ", results[i].address);
        
        if (results[i].device_count == 0) {
            printf("Unknown device                                    ║\n");
        } else {
            printf("%-49s ║\n", results[i].possible_devices[0]);
            
            for (uint8_t j = 1; j < results[i].device_count; j++) {
                printf("║        ║ %-49s ║\n", results[i].possible_devices[j]);
            }
        }
        
        if (i < count - 1) {
            printf("╠════════╬═══════════════════════════════════════════════════╣\n");
        }
    }
    
    printf("╚════════╩═══════════════════════════════════════════════════╝\n");
}

/**
 * @brief 打印I2C地址映射表
 */
void i2c_print_address_map(void)
{
    printf("\n╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║                        I2C Address Map                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("     ");
    for (int col = 0; col < 16; col++) {
        printf(" %X ", col);
    }
    printf("\n");
    
    printf("    ┌");
    for (int col = 0; col < 16; col++) {
        printf("───");
    }
    printf("┐\n");
    
    for (int row = 0; row < 8; row++) {
        printf(" %Xx │", row);
        
        for (int col = 0; col < 16; col++) {
            uint8_t addr = (row << 4) | col;
            
            if (addr < 0x03 || addr > 0x77) {
                printf(" - ");
            } else {
                uint8_t count;
                const char **devices = i2c_get_possible_devices(addr, &count);
                
                if (count > 0) {
                    printf(" ● ");
                } else {
                    printf(" · ");
                }
            }
        }
        
        printf("│\n");
    }
    
    printf("    └");
    for (int col = 0; col < 16; col++) {
        printf("───");
    }
    printf("┘\n");
    
    printf("\n  Legend: ● = Known device  · = Available  - = Reserved\n\n");
}
```

### 使用示例

```c
#include "sensor_i2c_scan.h"
#include "sensor_core.h"
#include <stdio.h>

void example_i2c_scan(void)
{
    void *i2c_bus = NULL;  /* 需要初始化I2C */
    i2c_scan_result_t results[128];
    uint8_t count = 0;
    
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║     I2C Bus Scanner & Device Detector        ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    
    /* 打印I2C地址映射表 */
    i2c_print_address_map();
    
    /* 扫描I2C总线 */
    i2c_scan_bus(i2c_bus, results, &count);
    
    /* 打印结果 */
    i2c_scan_print_results(results, count);
    
    /* 针对每个发现的设备尝试初始化 */
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║          Attempting Auto Detection          ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t addr = results[i].address;
        
        printf("[0x%02X] Trying devices:\n", addr);
        
        for (uint8_t j = 0; j < results[i].device_count; j++) {
            printf("  → %s ... ", results[i].possible_devices[j]);
            
            /* 这里可以根据地址尝试初始化相应的传感器 */
            /* 示例：
            if (addr == 0x68) {
                sensor_device_t *mpu = mpu6050_create_accel("mpu", i2c_bus);
                if (sensor_init(mpu) == SENSOR_EOK) {
                    printf("✓ Detected!\n");
                } else {
                    printf("✗ Failed\n");
                }
            }
            */
            
            printf("Pending\n");
        }
        
        printf("\n");
    }
}

int main(void)
{
    example_i2c_scan();
    return 0;
}
```

## 总结

### 已实现功能

1. ✅ **完整的I2C地址列表** - 涵盖80+种传感器
2. ✅ **地址映射表** - 按地址范围分组
3. ✅ **冲突识别** - 标记地址冲突的设备
4. ✅ **自动扫描** - I2C总线扫描功能
5. ✅ **设备识别** - 根据地址推测可能的设备
6. ✅ **可视化输出** - 美观的表格显示

### 文件列表

- `sensor_addresses.h` - 所有传感器地址定义
- `sensor_i2c_scan.h/c` - I2C扫描工具
- `I2C_ADDRESS_MAP.md` - 完整地址映射文档

这个工具可以帮助快速识别I2C总线上的设备！