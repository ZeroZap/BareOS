/**
 * SPDX-License-Identifier: MIT
 * @file    gps_example.c
 * @brief   GPS Sensor Example - XinYi Framework
 * @version 1.0.0
 *
 * 演示如何使用 GPS Sensor 驱动
 */

#include "sensor_gps.h"
#include "xy_os.h"
#include "xy_trace.h"

/* ==================== 示例配置 ==================== */
#define GPS_UART_BAUD        9600
#define GPS_UPDATE_RATE_HZ   1
#define GPS_NMEA_OUTPUT      (GPS_NMEA_GGA | GPS_NMEA_RMC | GPS_NMEA_GSV)

/* ==================== 全局变量 ==================== */
static gps_device_t g_gps_dev;
static bool g_gps_fixed = false;

/* ==================== GPS 事件回调 ==================== */
void gps_data_callback(gps_device_t *dev, gps_data_t *data, void *user_data)
{
    (void)user_data;

    if (data->data_valid) {
        g_gps_fixed = true;

        /* 打印位置信息 */
        printf("\r\n=== GPS Data ===\r\n");
        printf("Time: %02u:%02u:%02u.%03u UTC\r\n",
               data->hour, data->minute, data->second, data->millisecond);
        printf("Date: %04u-%02u-%02u\r\n",
               data->year, data->month, data->day);
        printf("Position: %.6f, %.6f\r\n", data->latitude, data->longitude);
        printf("Altitude: %.1f m\r\n", data->altitude);
        printf("Speed: %.2f m/s\r\n", data->speed);
        printf("Course: %.1f deg\r\n", data->course);
        printf("Quality: %d, Satellites: %u\r\n",
               data->quality, data->satellites_used);
        printf("HDOP: %.1f\r\n", data->hdop);
    }
}

/* ==================== GPS 初始化示例 ==================== */
int gps_example_init(void)
{
    sensor_err_t err;

    /* 初始化 GPS 设备 */
    gps_device_t *dev = &g_gps_dev;

    /* 配置 GPS 参数 */
    dev->config.baudrate = GPS_UART_BAUD;
    dev->config.update_rate = GPS_UPDATE_RATE_HZ;
    dev->config.nmea_output = GPS_NMEA_OUTPUT;
    dev->config.checksum_check = true;

    /* 设置基础信息 */
    dev->base.info.name = "GPS_AT6558";
    dev->base.info.type = SENSOR_TYPE_GPS;
    dev->base.info.vendor = "AT";
    dev->base.info.model = "AT6558";

    /* 注册设备 */
    err = gps_register(dev);
    if (err != SENSOR_EOK) {
        printf("GPS register failed: %d\r\n", err);
        return -1;
    }

    printf("GPS device registered\r\n");

    /* 设置回调 */
    /* gps_set_callback(dev, gps_data_callback, NULL); */

    return 0;
}

/* ==================== UART 数据输入示例 ==================== */
/**
 * @brief   从 UART 接收 NMEA 数据
 *
 * 在实际应用中，这个函数应该在 UART 中断或轮询中调用
 */
void gps_uart_input(uint8_t byte)
{
    gps_parse_byte(&g_gps_dev, byte);
}

/**
 * @brief   处理完整的 NMEA 语句
 */
void gps_process_nmea_string(const char *nmea_str)
{
    uint16_t len = strlen(nmea_str);
    gps_parse_nmea(&g_gps_dev, nmea_str, len);
}

/* ==================== 读取 GPS 数据示例 ==================== */
void gps_read_example(void)
{
    gps_data_t data;

    /* 读取当前 GPS 数据 */
    sensor_err_t err = gps_read_data(&g_gps_dev, &data);
    if (err != SENSOR_EOK) {
        printf("GPS read failed\r\n");
        return;
    }

    if (data.data_valid) {
        printf("Lat: %.6f, Lon: %.6f, Alt: %.1f\r\n",
               data.latitude, data.longitude, data.altitude);
    } else {
        printf("GPS not fixed\r\n");
    }
}

/* ==================== 位置查询示例 ==================== */
void gps_position_example(void)
{
    double lat, lon;
    float alt;

    if (gps_is_fixed(&g_gps_dev)) {
        gps_get_position(&g_gps_dev, &lat, &lon, &alt);
        printf("Current position: %.6f, %.6f (%.1f m)\r\n", lat, lon, alt);
    }
}

/* ==================== 距离计算示例 ==================== */
void gps_distance_example(void)
{
    /* 北京天安门坐标 */
    double lat1 = 39.9088;
    double lon1 = 116.3974;

    /* 北京故宫坐标 */
    double lat2 = 39.9134;
    double lon2 = 116.3970;

    /* 计算距离 */
    float distance = gps_distance(lat1, lon1, lat2, lon2);
    printf("Distance from Tiananmen to Forbidden City: %.0f m\r\n", distance);

    /* 计算方位 */
    float bearing = gps_bearing(lat1, lon1, lat2, lon2);
    printf("Bearing: %.1f deg\r\n", bearing);
}

/* ==================== 功耗管理示例 ==================== */
void gps_power_example(void)
{
    /* 进入低功耗模式 */
    printf("GPS entering sleep mode...\r\n");
    gps_sleep(&g_gps_dev);

    /* 模拟延时 */
    xy_os_sleep(5000);  // 5秒

    /* 唤醒 */
    printf("GPS waking up...\r\n");
    gps_wakeup(&g_gps_dev);
}

/* ==================== 主任务示例 ==================== */
void gps_main_task(void *arg)
{
    (void)arg;

    /* 初始化 GPS */
    if (gps_example_init() != 0) {
        printf("GPS init failed\r\n");
        return;
    }

    /* 主循环 */
    while (1) {
        /* 读取 GPS 数据 */
        gps_read_example();

        /* 检查是否定位成功 */
        if (g_gps_fixed) {
            /* 执行距离计算示例 */
            gps_distance_example();
        }

        /* 延时 */
        xy_os_sleep(1000);  // 1秒
    }
}

/* ==================== 模拟测试数据 ==================== */
/**
 * @brief   模拟 NMEA 数据测试
 */
void gps_simulate_nmea(void)
{
    /* 模拟 GGA 语句 */
    const char *gga = "$GPGGA,123519.000,3954.4789,N,11623.7834,E,1,08,0.9,545.5,M,46.9,M,,*47";
    gps_process_nmea_string(gga);

    /* 模拟 RMC 语句 */
    const char *rmc = "$GPRMC,123519.000,A,3954.4789,N,11623.7834,E,0.0,0.0,300320,,,A*77";
    gps_process_nmea_string(rmc);

    /* 读取并验证 */
    gps_data_t data;
    gps_read_data(&g_gps_dev, &data);

    if (data.data_valid) {
        printf("\r\nSimulated GPS read:\r\n");
        printf("  Lat: %.6f (expected ~39.9)\r\n", data.latitude);
        printf("  Lon: %.6f (expected ~116.4)\r\n", data.longitude);
        printf("  Alt: %.1f m\r\n", data.altitude);
        printf("  Quality: %d\r\n", data.quality);
    }
}

/* ==================== 使用示例 ==================== */
/**
 * @code
 * // 1. 初始化
 * gps_example_init();
 *
 * // 2. 在 UART 中断中输入数据
 * void UART1_IRQHandler(void) {
 *     uint8_t c = UART1->DR;
 *     gps_uart_input(c);
 * }
 *
 * // 3. 定期读取
 * while (1) {
 *     gps_read_example();
 *     xy_os_sleep(1000);
 * }
 *
 * // 4. 距离计算
 * float d = gps_distance(39.9088, 116.3974, 40.0000, 116.5000);
 *
 * // 5. 低功耗
 * gps_sleep(&g_gps_dev);  // 进入休眠
 * xy_os_sleep(60000);     // 60秒后唤醒
 * gps_wakeup(&g_gps_dev);
 * @endcode
 */

/* ==================== 测试入口 ==================== */
#ifdef TEST_GPS
int main(void)
{
    printf("GPS Example Test\r\n");

    /* 初始化 */
    gps_example_init();

    /* 模拟 NMEA 数据 */
    gps_simulate_nmea();

    /* 距离计算测试 */
    gps_distance_example();

    printf("\r\nTest complete\r\n");
    return 0;
}
#endif