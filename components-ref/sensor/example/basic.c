#include "sensor_core.h"
#include "sensor_mpu6050.h"
#include <stdio.h>

/* 用户需要实现的HAL函数 */
void delay_ms(uint32_t ms)
{
    /* 实现延时 */
}

uint32_t get_tick_ms(void)
{
    /* 返回系统时间戳(ms) */
    return 0;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                     uint16_t len)
{
    /* 实现I2C读取 */
    return 0;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                      uint16_t len)
{
    /* 实现I2C写入 */
    return 0;
}

/**
 * @brief 基础使用示例
 */
void example_basic(void)
{
    void *i2c_bus = NULL; /* I2C总线句柄 */

    printf("\n=== Basic Example ===\n");

    /* 创建传感器设备 */
    sensor_device_t *accel = mpu6050_create_accel("accel0", i2c_bus);
    if (accel == NULL) {
        printf("Failed to create accelerometer\n");
        return;
    }

    /* 注册设备 */
    if (sensor_register(accel) != SENSOR_EOK) {
        printf("Failed to register sensor\n");
        return;
    }

    /* 初始化设备 */
    if (sensor_init(accel) != SENSOR_EOK) {
        printf("Failed to initialize sensor\n");
        return;
    }

    /* 读取数据 */
    for (int i = 0; i < 10; i++) {
        sensor_data_t data;

        if (sensor_read(accel, &data) == SENSOR_EOK) {
            printf("[%u] Accel: X=%d, Y=%d, Z=%d mg\n", data.timestamp,
                   data.value.val_3axis.x, data.value.val_3axis.y,
                   data.value.val_3axis.z);
        }

        delay_ms(100);
    }

    /* 反初始化 */
    sensor_deinit(accel);
    sensor_unregister(accel);
}

int main(void)
{
    example_basic();
    return 0;
}