/**
 * @file sensor_demo.c
 * @brief Sensor Framework Demo
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include "xy_os.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* ==================== 示例 1: 轮询模式 ==================== */

void demo_polling(void)
{
    xy_log_i("=== Demo: Polling Mode ===\n");
    
    /* 获取传感器 */
    xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
    if (!sensor) {
        xy_log_e("Sensor not found\n");
        return;
    }
    
    /* 初始化 */
    xy_sensor_init(sensor);
    
    xy_sensor_value_t temp, humidity;
    
    while (1) {
        /* 获取采样 */
        xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
        
        /* 读取数据 */
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &temp);
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_HUMIDITY, &humidity);
        
        /* 转换并打印 */
        float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);
        float humidity_pct = XY_SENSOR_VALUE_TO_FLOAT(humidity);
        
        xy_log_i("T: %.2f°C, H: %.2f%%\n", temp_c, humidity_pct);
        
        xy_os_delay(1000);
    }
}

/* ==================== 示例 2: 触发模式 ==================== */

/* 触发回调 */
void motion_callback(xy_sensor_device_t *dev, const xy_sensor_trigger_t *trigger)
{
    xy_sensor_value_t val;
    xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_X, &val);
    
    float accel = XY_SENSOR_VALUE_TO_FLOAT(val);
    xy_log_i("Motion detected! Accel: %.2f mG\n", accel);
}

void demo_trigger(void)
{
    xy_log_i("=== Demo: Trigger Mode ===\n");
    
    /* 获取 MPU6050 */
    xy_sensor_device_t *mpu = xy_sensor_device_get("MPU6050");
    if (!mpu) {
        xy_log_e("Sensor not found\n");
        return;
    }
    
    xy_sensor_init(mpu);
    
    /* 配置触发器 */
    xy_sensor_trigger_t trigger = {
        .type = XY_SENSOR_TRIG_MOTION,
        .channel = XY_SENSOR_CHAN_ACCEL_X,
        .trigger_handler = motion_callback,
    };
    
    xy_sensor_trigger_set(mpu, &trigger);
    
    xy_log_i("Motion trigger enabled\n");
    
    /* 主循环 */
    while (1) {
        xy_os_delay(1000);
    }
}

/* ==================== 示例 3: 多传感器 ==================== */

void demo_multi_sensor(void)
{
    xy_log_i("=== Demo: Multi-Sensor ===\n");
    
    /* 遍历所有传感器 */
    xy_sensor_device_foreach([](xy_sensor_device_t *dev, void *ud) {
        xy_log_i("Found sensor: %s (type=%d)\n", dev->name, dev->type);
    }, NULL);
    
    /* 获取所有温度传感器 */
    uint8_t index = 0;
    while (1) {
        xy_sensor_device_t *temp_sensor = xy_sensor_device_get_by_type(
            XY_SENSOR_TYPE_TEMP, index++);
        
        if (!temp_sensor) break;
        
        xy_sensor_init(temp_sensor);
        
        xy_sensor_value_t val;
        xy_sensor_channel_get(temp_sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &val);
        
        float temp = XY_SENSOR_VALUE_TO_FLOAT(val);
        xy_log_i("%s: %.2f°C\n", temp_sensor->name, temp);
    }
}

/* ==================== 示例 4: 低功耗 ==================== */

void demo_low_power(void)
{
    xy_log_i("=== Demo: Low Power ===\n");
    
    xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
    if (!sensor) return;
    
    xy_sensor_init(sensor);
    
    while (1) {
        /* 唤醒 */
        xy_sensor_wakeup(sensor);
        
        /* 读取数据 */
        xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
        
        xy_sensor_value_t temp;
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &temp);
        
        float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);
        xy_log_i("T: %.2f°C\n", temp_c);
        
        /* 睡眠 */
        xy_sensor_sleep(sensor);
        
        /* 等待 10 秒 */
        xy_os_delay(10000);
    }
}

/* ==================== 示例 5: 阈值告警 ==================== */

void temp_alarm_callback(xy_sensor_device_t *dev, const xy_sensor_trigger_t *trigger)
{
    xy_log_w("Temperature alarm!\n");
}

void demo_threshold(void)
{
    xy_log_i("=== Demo: Threshold Alarm ===\n");
    
    xy_sensor_device_t *sensor = xy_sensor_device_get("SHT30");
    if (!sensor) return;
    
    xy_sensor_init(sensor);
    
    /* 配置阈值 */
    xy_sensor_thresh_config_t thresh = {
        .lower = {20, 0},       /* 20°C */
        .upper = {35, 0},       /* 35°C */
        .slope = {1, 0},        /* 1°C/s */
        .duration = 1000,       /* 1 秒 */
    };
    
    xy_sensor_trigger_set_thresh(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &thresh);
    
    /* 配置触发器 */
    xy_sensor_trigger_t trigger = {
        .type = XY_SENSOR_TRIG_THRESHOLD,
        .channel = XY_SENSOR_CHAN_AMBIENT_TEMP,
        .trigger_handler = temp_alarm_callback,
    };
    
    xy_sensor_trigger_set(sensor, &trigger);
    
    xy_log_i("Threshold alarm enabled (20-35°C)\n");
    
    while (1) {
        xy_os_delay(1000);
    }
}

/* ==================== 主函数 ==================== */

int main(void)
{
    xy_log_i("=== Sensor Framework Demo ===\n");
    
    /* 打印传感器信息 */
    xy_log_i("Total sensors: %d\n", xy_sensor_device_count());
    xy_sensor_print_all();
    
    /* 选择示例 */
    #if 0
        demo_polling();         /* 轮询模式 */
    #elif 0
        demo_trigger();         /* 触发模式 */
    #elif 0
        demo_multi_sensor();    /* 多传感器 */
    #elif 0
        demo_low_power();       /* 低功耗 */
    #else
        demo_threshold();       /* 阈值告警 */
    #endif
    
    return 0;
}
