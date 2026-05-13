#include "sensor_core.h"
#include "sensor_mpu6050.h"
#include "sensor_sc7a20.h"
#include "sensor_aht20.h"
#include "sensor_bmp280.h"
#include "sensor_ap3216c.h"
#include "sensor_icm20608.h"
#include "sensor_qmc5883l.h"
#include <stdio.h>

/* 传感器句柄 */
static struct {
    sensor_device_t *accel;
    sensor_device_t *gyro;
    sensor_device_t *mag;
    sensor_device_t *temp;
    sensor_device_t *humi;
    sensor_device_t *pressure;
    sensor_device_t *light;
    sensor_device_t *proximity;
} sensors;

/**
 * @brief 初始化所有传感器
 */
int sensors_init_all(void *i2c_bus, void *spi_bus)
{
    int count = 0;

    printf("=== Initializing All Sensors ===\n");

    /* 创建并初始化MPU6050 */
    sensors.accel = mpu6050_create_accel("mpu6050_accel", i2c_bus);
    sensors.gyro  = mpu6050_create_gyro("mpu6050_gyro", i2c_bus);

    if (sensors.accel && sensor_register(sensors.accel) == SENSOR_EOK
        && sensor_init(sensors.accel) == SENSOR_EOK) {
        printf("✓ MPU6050 Accelerometer initialized\n");
        count++;
    }

    if (sensors.gyro && sensor_register(sensors.gyro) == SENSOR_EOK
        && sensor_init(sensors.gyro) == SENSOR_EOK) {
        printf("✓ MPU6050 Gyroscope initialized\n");
        count++;
    }

    /* 创建并初始化AHT20 */
    sensors.temp = aht20_create_temperature("aht20_temp", i2c_bus);
    sensors.humi = aht20_create_humidity("aht20_humi", i2c_bus);

    if (sensors.temp && sensor_register(sensors.temp) == SENSOR_EOK
        && sensor_init(sensors.temp) == SENSOR_EOK) {
        printf("✓ AHT20 Temperature initialized\n");
        count++;
    }

    if (sensors.humi && sensor_register(sensors.humi) == SENSOR_EOK
        && sensor_init(sensors.humi) == SENSOR_EOK) {
        printf("✓ AHT20 Humidity initialized\n");
        count++;
    }

    /* 创建并初始化BMP280 */
    sensors.pressure = bmp280_create_pressure("bmp280_pressure", i2c_bus);

    if (sensors.pressure && sensor_register(sensors.pressure) == SENSOR_EOK
        && sensor_init(sensors.pressure) == SENSOR_EOK) {
        printf("✓ BMP280 Pressure initialized\n");
        count++;
    }

    /* 创建并初始化AP3216C */
    sensors.light     = ap3216c_create_light("ap3216c_light", i2c_bus);
    sensors.proximity = ap3216c_create_proximity("ap3216c_proximity", i2c_bus);

    if (sensors.light && sensor_register(sensors.light) == SENSOR_EOK
        && sensor_init(sensors.light) == SENSOR_EOK) {
        printf("✓ AP3216C Light sensor initialized\n");
        count++;
    }

    if (sensors.proximity && sensor_register(sensors.proximity) == SENSOR_EOK
        && sensor_init(sensors.proximity) == SENSOR_EOK) {
        printf("✓ AP3216C Proximity sensor initialized\n");
        count++;
    }

    /* 创建并初始化QMC5883L */
    sensors.mag = qmc5883l_create("qmc5883l_mag", i2c_bus);

    if (sensors.mag && sensor_register(sensors.mag) == SENSOR_EOK
        && sensor_init(sensors.mag) == SENSOR_EOK) {
        printf("✓ QMC5883L Magnetometer initialized\n");
        count++;
    }

    printf("=== Total %d sensors initialized ===\n\n", count);

    return count;
}

/**
 * @brief 读取并显示所有传感器数据
 */
void sensors_read_all(void)
{
    sensor_data_t data;

    printf("\n=== Sensor Data ===\n");

    /* 读取加速度计 */
    if (sensors.accel && sensor_read(sensors.accel, &data) == SENSOR_EOK) {
        printf("Accelerometer: X=%d, Y=%d, Z=%d mg\n", data.value.val_3axis.x,
               data.value.val_3axis.y, data.value.val_3axis.z);
    }

    /* 读取陀螺仪 */
    if (sensors.gyro && sensor_read(sensors.gyro, &data) == SENSOR_EOK) {
        printf("Gyroscope:     X=%d, Y=%d, Z=%d °/s\n", data.value.val_3axis.x,
               data.value.val_3axis.y, data.value.val_3axis.z);
    }

    /* 读取磁力计 */
    if (sensors.mag && sensor_read(sensors.mag, &data) == SENSOR_EOK) {
        printf("Magnetometer:  X=%d, Y=%d, Z=%d μT\n", data.value.val_3axis.x,
               data.value.val_3axis.y, data.value.val_3axis.z);
    }

    /* 读取温度 */
    if (sensors.temp && sensor_read(sensors.temp, &data) == SENSOR_EOK) {
#if SENSOR_USE_FLOAT
        printf("Temperature:   %.2f °C\n", data.value.val_float);
#else
        printf("Temperature:   %d.%02d °C\n", data.value.val_int32 / 100,
               abs(data.value.val_int32 % 100));
#endif
    }

    /* 读取湿度 */
    if (sensors.humi && sensor_read(sensors.humi, &data) == SENSOR_EOK) {
#if SENSOR_USE_FLOAT
        printf("Humidity:      %.2f %%\n", data.value.val_float);
#else
        printf("Humidity:      %d.%02d %%\n", data.value.val_int32 / 100,
               data.value.val_int32 % 100);
#endif
    }

    /* 读取气压 */
    if (sensors.pressure
        && sensor_read(sensors.pressure, &data) == SENSOR_EOK) {
        printf("Pressure:      %u Pa (%.2f hPa)\n", data.value.val_uint32,
               data.value.val_uint32 / 100.0f);
    }

    /* 读取光照 */
    if (sensors.light && sensor_read(sensors.light, &data) == SENSOR_EOK) {
        printf("Light:         %u lux\n", data.value.val_uint32);
    }

    /* 读取接近传感器 */
    if (sensors.proximity
        && sensor_read(sensors.proximity, &data) == SENSOR_EOK) {
        printf("Proximity:     %d (object %s)\n", data.value.val_int32,
               data.value.val_uint32 ? "detected" : "not detected");
    }

    printf("==================\n");
}

#if SENSOR_ENABLE_FUSION && SENSOR_USE_FLOAT
/**
 * @brief 传感器融合示例
 */
void sensors_fusion_demo(void)
{
    printf("\n=== 9DOF Sensor Fusion Demo ===\n");

    if (!sensors.accel || !sensors.gyro || !sensors.mag) {
        printf("Required sensors not available\n");
        return;
    }

    /* 配置9轴融合 */
    sensor_fusion_config_t fusion_cfg = { .type         = SENSOR_FUSION_9DOF,
                                          .beta         = 0.1f,
                                          .enable       = true,
                                          .sensor_count = 3 };

    fusion_cfg.sensors[0] = sensors.accel;
    fusion_cfg.sensors[1] = sensors.gyro;
    fusion_cfg.sensors[2] = sensors.mag;

    if (sensor_fusion_init(&fusion_cfg) != SENSOR_EOK) {
        printf("Failed to initialize fusion\n");
        return;
    }

    /* 持续更新姿态 */
    for (int i = 0; i < 100; i++) {
        sensor_data_t orientation;

        if (sensor_fusion_update(&fusion_cfg, &orientation) == SENSOR_EOK) {
            printf("[%d] Roll=%.2f°, Pitch=%.2f°, Yaw=%.2f°\n", i,
                   orientation.value.val_3axis.x / 100.0f,
                   orientation.value.val_3axis.y / 100.0f,
                   orientation.value.val_3axis.z / 100.0f);
        }

        delay_ms(10);
    }

    sensor_fusion_deinit(&fusion_cfg);
}
#endif

/**
 * @brief 列出所有注册的传感器
 */
void sensors_list_all(void)
{
    printf("\n=== Registered Sensors ===\n");
    printf("%-20s %-15s %-15s %-10s\n", "Name", "Vendor", "Model", "Type");
    printf("--------------------------------------------------------\n");

    const char *type_names[] = { "None",   "Accel",   "Gyro",     "Mag",
                                 "Orient", "Gravity", "LinAccel", "RotVec" };

    /* 遍历所有注册的传感器 */
    for (int type = SENSOR_TYPE_ACCELEROMETER; type <= SENSOR_TYPE_NOISE;
         type++) {
        sensor_device_t *sensor = sensor_find_by_type((sensor_type_t)type);
        if (sensor) {
            const sensor_info_t *info = sensor_get_info(sensor);
            const char *type_name = (type <= 7) ? type_names[type] : "Other";

            printf("%-20s %-15s %-15s %-10s\n", info->name, info->vendor,
                   info->model, type_name);
        }
    }

    printf("========================================================\n\n");
}

/**
 * @brief 主函数
 */
int main(void)
{
    void *i2c_bus = NULL; /* 需要初始化I2C */
    void *spi_bus = NULL; /* 需要初始化SPI */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║  Multi-Sensor Framework Demo          ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    /* 初始化所有传感器 */
    int sensor_count = sensors_init_all(i2c_bus, spi_bus);

    if (sensor_count == 0) {
        printf("No sensors initialized!\n");
        return -1;
    }

    /* 列出所有传感器 */
    sensors_list_all();

    /* 主循环 - 读取传感器数据 */
    for (int i = 0; i < 10; i++) {
        sensors_read_all();
        delay_ms(1000);
    }

#if SENSOR_ENABLE_FUSION && SENSOR_USE_FLOAT
    /* 运行传感器融合示例 */
    sensors_fusion_demo();
#endif

    printf("\nDemo completed!\n");

    return 0;
}