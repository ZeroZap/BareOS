#include "sensor_core.h"
#include "sensor_mpu6050.h"
#include <stdio.h>

static sensor_device_t *g_accel = NULL;
static sensor_device_t *g_gyro  = NULL;

/**
 * @brief 数据回调函数
 */
void sensor_data_callback(sensor_device_t *sensor, sensor_data_t *data,
                          void *user_data)
{
    printf("[Callback] %s: X=%d, Y=%d, Z=%d\n", sensor->info.name,
           data->value.val_3axis.x, data->value.val_3axis.y,
           data->value.val_3axis.z);
}

#if SENSOR_ENABLE_INTERRUPT
/**
 * @brief 中断回调函数
 */
void sensor_interrupt_callback(sensor_device_t *sensor, uint32_t int_type)
{
    printf("[Interrupt] Type: 0x%02X\n", int_type);

#if SENSOR_ENABLE_FIFO
    if (int_type & SENSOR_INT_FIFO_WATERMARK) {
        printf("FIFO watermark reached\n");

        sensor_data_t batch_data[32];
        uint32_t read_count = 0;

        sensor_fifo_read(sensor, batch_data, 32, &read_count);
        printf("Read %u samples from FIFO\n", read_count);
    }
#endif

#if SENSOR_ENABLE_MOTION_DETECT
    if (int_type & SENSOR_INT_MOTION_DETECT) {
        printf("Motion detected!\n");
    }
#endif
}
#endif

#if SENSOR_ENABLE_FIFO
/**
 * @brief FIFO示例
 */
void example_fifo(void)
{
    printf("\n=== FIFO Example ===\n");

    /* 初始化FIFO */
    if (sensor_fifo_init(g_accel, 64) != SENSOR_EOK) {
        printf("Failed to init FIFO\n");
        return;
    }

    sensor_fifo_set_watermark(g_accel, 32);

#if SENSOR_ENABLE_INTERRUPT
    /* 配置中断 */
    sensor_interrupt_config_t int_cfg = { .type = SENSOR_INT_FIFO_WATERMARK,
                                          .active_high = true,
                                          .latch       = false,
                                          .handler =
                                              sensor_interrupt_callback };
    sensor_interrupt_init(g_accel, &int_cfg);
    sensor_interrupt_enable(g_accel, SENSOR_INT_FIFO_WATERMARK, true);
#endif

    sensor_fifo_enable(g_accel, true);

    /* 模拟数据采集 */
    for (int i = 0; i < 50; i++) {
        sensor_data_t data;
        if (sensor_read(g_accel, &data) == SENSOR_EOK) {
            sensor_fifo_push(g_accel, &data);
        }
        delay_ms(10);
    }

    /* 读取FIFO数据 */
    uint32_t count;
    sensor_fifo_get_count(g_accel, &count);
    printf("FIFO count: %u\n", count);

    sensor_data_t batch_data[64];
    uint32_t read_count;
    sensor_fifo_read(g_accel, batch_data, count, &read_count);
    printf("Read %u samples from FIFO\n", read_count);

    /* 清理 */
    sensor_fifo_deinit(g_accel);
}
#endif

#if SENSOR_ENABLE_CALIBRATION
/**
 * @brief 校准示例
 */
void example_calibration(void)
{
    printf("\n=== Calibration Example ===\n");

    printf("Please keep sensor stationary...\n");
    delay_ms(2000);

    printf("Calibrating...\n");
    sensor_err_t ret = sensor_calibrate_offset(g_accel, 100);

    if (ret == SENSOR_EOK) {
        printf("Calibration successful\n");

        sensor_calibration_data_t calib;
        sensor_get_calibration(g_accel, &calib);

        printf("Offset: X=%d, Y=%d, Z=%d\n", calib.offset.val_3axis.x,
               calib.offset.val_3axis.y, calib.offset.val_3axis.z);

#if SENSOR_USE_FILE_SYSTEM
        /* 保存校准数据 */
        sensor_save_calibration(g_accel, "/calib/accel.dat");
#endif
    } else {
        printf("Calibration failed: %d\n", ret);
    }
}
#endif

#if SENSOR_ENABLE_FILTER
/**
 * @brief 滤波器示例
 */
void example_filter(void)
{
    printf("\n=== Filter Example ===\n");

    /* 配置移动平均滤波器 */
    sensor_filter_config_t filter_cfg = { .type = SENSOR_FILTER_MOVING_AVERAGE,
                                          .window_size = 8,
                                          .enable      = true };

    if (sensor_filter_init(g_accel, &filter_cfg) != SENSOR_EOK) {
        printf("Failed to init filter\n");
        return;
    }

    printf(
        "Moving average filter enabled (window=%d)\n", filter_cfg.window_size);

    /* 读取并滤波数据 */
    for (int i = 0; i < 20; i++) {
        sensor_data_t data;
        sensor_read(g_accel, &data);

        printf("[%d] Filtered: X=%d, Y=%d, Z=%d mg\n", i,
               data.value.val_3axis.x, data.value.val_3axis.y,
               data.value.val_3axis.z);

        delay_ms(50);
    }

    sensor_filter_deinit(g_accel);
}
#endif

#if SENSOR_ENABLE_FUSION && SENSOR_USE_FLOAT
/**
 * @brief 传感器融合示例
 */
void example_fusion(void)
{
    printf("\n=== Sensor Fusion Example (6DOF) ===\n");

    /* 配置6轴融合 */
    sensor_fusion_config_t fusion_cfg = { .type         = SENSOR_FUSION_6DOF,
                                          .beta         = 0.1f,
                                          .enable       = true,
                                          .sensor_count = 2 };

    fusion_cfg.sensors[0] = g_accel;
    fusion_cfg.sensors[1] = g_gyro;

    if (sensor_fusion_init(&fusion_cfg) != SENSOR_EOK) {
        printf("Failed to init fusion\n");
        return;
    }

    /* 持续更新姿态 */
    for (int i = 0; i < 100; i++) {
        sensor_data_t orientation;

        if (sensor_fusion_update(&fusion_cfg, &orientation) == SENSOR_EOK) {
            printf("Attitude: Roll=%.2f°, Pitch=%.2f°, Yaw=%.2f°\n",
                   orientation.value.val_3axis.x / 100.0f,
                   orientation.value.val_3axis.y / 100.0f,
                   orientation.value.val_3axis.z / 100.0f);
        }

        delay_ms(10);
    }

    sensor_fusion_deinit(&fusion_cfg);
}
#endif

#if SENSOR_ENABLE_MOTION_DETECT
/**
 * @brief 运动检测示例
 */
void example_motion_detection(void)
{
    printf("\n=== Motion Detection Example ===\n");

    /* 配置运动检测 */
    sensor_motion_config_t motion_cfg = { .threshold = 500,
                                          .duration  = 100,
                                          .enable    = true };

    sensor_motion_detect_init(g_accel, &motion_cfg);

#if SENSOR_ENABLE_INTERRUPT
    /* 配置中断 */
    sensor_interrupt_config_t int_cfg = { .type = SENSOR_INT_MOTION_DETECT,
                                          .active_high = true,
                                          .latch       = false,
                                          .handler =
                                              sensor_interrupt_callback };
    sensor_interrupt_init(g_accel, &int_cfg);
    sensor_interrupt_enable(g_accel, SENSOR_INT_MOTION_DETECT, true);
#endif

    printf("Motion detection enabled. Move the sensor...\n");

    /* 主循环 */
    for (int i = 0; i < 100; i++) {
        sensor_data_t data;
        if (sensor_read(g_accel, &data) == SENSOR_EOK) {
            sensor_motion_detect_process(g_accel, &data);
            sensor_freefall_detect_process(g_accel, &data);
            sensor_tap_detect_process(g_accel, &data);
        }
        delay_ms(10);
    }
}
#endif

#if SENSOR_ENABLE_POWER_MGMT
/**
 * @brief 功耗管理示例
 */
void example_power_management(void)
{
    printf("\n=== Power Management Example ===\n");

    /* 正常模式运行 */
    printf("Running in normal mode...\n");
    sensor_set_power_mode(g_accel, SENSOR_POWER_MODE_NORMAL);

    for (int i = 0; i < 50; i++) {
        sensor_data_t data;
        sensor_read(g_accel, &data);
        delay_ms(20);
    }

    /* 切换到低功耗模式 */
    printf("Switching to low power mode...\n");
    sensor_set_power_mode(g_accel, SENSOR_POWER_MODE_LOW_POWER);
    delay_ms(1000);

    /* 获取功耗统计 */
    sensor_power_stats_t stats;
    sensor_get_power_stats(g_accel, &stats);

    printf("Power Statistics:\n");
    printf("  Active time: %u ms\n", stats.active_time);
    printf("  Sleep time: %u ms\n", stats.sleep_time);
#if SENSOR_USE_FLOAT
    printf("  Average current: %.2f mA\n", stats.average_current);
    printf("  Energy consumed: %.2f mWh\n", stats.energy_consumed);
#else
    printf("  Average current: %u uA\n", stats.average_current_ua);
    printf("  Energy consumed: %u uWh\n", stats.energy_consumed_uwh);
#endif

    /* 恢复正常模式 */
    sensor_set_power_mode(g_accel, SENSOR_POWER_MODE_NORMAL);
}
#endif

#if SENSOR_ENABLE_SELF_TEST
/**
 * @brief 自测试示例
 */
void example_self_test(void)
{
    printf("\n=== Self Test Example ===\n");

    sensor_self_test_result_t result;

    printf("Running self test...\n");
    sensor_err_t ret = sensor_self_test(g_accel, &result);

    if (ret == SENSOR_EOK && result.passed) {
        printf("✓ Self test PASSED: %s\n", result.message);
    } else {
        printf("✗ Self test FAILED (code %u): %s\n", result.error_code,
               result.message);
    }
}
#endif

#if SENSOR_ENABLE_THRESHOLD
/**
 * @brief 阈值检测示例
 */
void example_threshold(void)
{
    printf("\n=== Threshold Detection Example ===\n");

    /* 配置阈值 */
    sensor_threshold_config_t threshold_cfg = { .enable   = true,
                                                .duration = 100 };

    /* 设置阈值范围 */
    threshold_cfg.low_threshold.val_3axis.x = -1500;
    threshold_cfg.low_threshold.val_3axis.y = -1500;
    threshold_cfg.low_threshold.val_3axis.z = -1500;

    threshold_cfg.high_threshold.val_3axis.x = 1500;
    threshold_cfg.high_threshold.val_3axis.y = 1500;
    threshold_cfg.high_threshold.val_3axis.z = 1500;

    sensor_threshold_init(g_accel, &threshold_cfg);

    printf("Threshold detection enabled\n");

    /* 读取数据并检查阈值 */
    for (int i = 0; i < 50; i++) {
        sensor_data_t data;
        if (sensor_read(g_accel, &data) == SENSOR_EOK) {
            if (sensor_threshold_check(g_accel, &data)) {
                printf("Threshold exceeded!\n");
            }
        }
        delay_ms(50);
    }
}
#endif

/**
 * @brief 主函数
 */
int main(void)
{
    void *i2c_bus = NULL; /* 需要初始化I2C */

    printf("=== Sensor Framework Advanced Examples ===\n");
    printf("Compile-time configuration:\n");
    printf("  FIFO: %s\n", SENSOR_ENABLE_FIFO ? "Enabled" : "Disabled");
    printf(
        "  Interrupt: %s\n", SENSOR_ENABLE_INTERRUPT ? "Enabled" : "Disabled");
    printf("  Calibration: %s\n",
           SENSOR_ENABLE_CALIBRATION ? "Enabled" : "Disabled");
    printf("  Filter: %s\n", SENSOR_ENABLE_FILTER ? "Enabled" : "Disabled");
    printf("  DMA: %s\n", SENSOR_ENABLE_DMA ? "Enabled" : "Disabled");
    printf("  Fusion: %s\n", SENSOR_ENABLE_FUSION ? "Enabled" : "Disabled");
    printf("  Power Mgmt: %s\n",
           SENSOR_ENABLE_POWER_MGMT ? "Enabled" : "Disabled");
    printf(
        "  Self Test: %s\n", SENSOR_ENABLE_SELF_TEST ? "Enabled" : "Disabled");
    printf("  Float: %s\n", SENSOR_USE_FLOAT ? "Enabled" : "Disabled");
    printf("  Malloc: %s\n", SENSOR_USE_MALLOC ? "Enabled" : "Disabled");

    /* 创建传感器设备 */
    g_accel = mpu6050_create_accel("accel0", i2c_bus);
    g_gyro  = mpu6050_create_gyro("gyro0", i2c_bus);

    if (g_accel == NULL || g_gyro == NULL) {
        printf("Failed to create sensors\n");
        return -1;
    }

    /* 注册设备 */
    sensor_register(g_accel);
    sensor_register(g_gyro);

    /* 初始化设备 */
    if (sensor_init(g_accel) != SENSOR_EOK) {
        printf("Failed to init accelerometer\n");
        return -1;
    }

    if (sensor_init(g_gyro) != SENSOR_EOK) {
        printf("Failed to init gyroscope\n");
        return -1;
    }

    /* 设置回调 */
    sensor_set_callback(g_accel, sensor_data_callback, NULL);

    /* 运行各种示例 */
#if SENSOR_ENABLE_SELF_TEST
    example_self_test();
#endif

#if SENSOR_ENABLE_CALIBRATION
    example_calibration();
#endif

#if SENSOR_ENABLE_FILTER
    example_filter();
#endif

#if SENSOR_ENABLE_THRESHOLD
    example_threshold();
#endif

#if SENSOR_ENABLE_MOTION_DETECT
    example_motion_detection();
#endif

#if SENSOR_ENABLE_FIFO
    example_fifo();
#endif

#if SENSOR_ENABLE_POWER_MGMT
    example_power_management();
#endif

#if SENSOR_ENABLE_FUSION && SENSOR_USE_FLOAT
    example_fusion();
#endif

    printf("\nAll examples completed!\n");

    /* 清理 */
    sensor_deinit(g_accel);
    sensor_deinit(g_gyro);
    sensor_unregister(g_accel);
    sensor_unregister(g_gyro);

    return 0;
}