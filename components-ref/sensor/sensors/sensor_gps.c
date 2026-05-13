/**
 * SPDX-License-Identifier: MIT
 * @file    sensor_gps.c
 * @brief   GPS Sensor Driver Implementation
 * @version 1.0.0
 *
 * NMEA 协议解析驱动，适用于 AT6558、LC86L、UBLOX 等 GPS 模块
 */

#include "sensor_gps.h"
#include "sensor_core.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== 常量 ==================== */
#define GPS_PI          3.14159265358979323846
#define GPS_EARTH_R     6371000.0  ///< 地球半径 (米)

/* ==================== 静态变量 ==================== */
static gps_device_t *g_gps_devices[GPS_SATELLITE_MAX];
static uint8_t g_gps_count = 0;

/* ==================== NMEA 解析 ==================== */

/**
 * @brief   校验 NMEA 校验和
 */
static bool gps_validate_checksum(const char *nmea_str)
{
    const char *checksum_ptr = strchr(nmea_str, '*');
    if (checksum_ptr == NULL) {
        return false;
    }

    uint8_t expected = (uint8_t)strtol(checksum_ptr + 1, NULL, 16);
    uint8_t calculated = 0;

    const char *p = nmea_str + 1;  // skip '$'
    while (*p && *p != '*') {
        calculated ^= (uint8_t)*p;
        p++;
    }

    return (calculated == expected);
}

/**
 * @brief   解析 NMEA GGA 语句 (定位数据)
 */
static sensor_err_t gps_parse_gga(gps_device_t *dev, const char *gga_str)
{
    char fields[GPSgga_MAX_FIELDS][32];
    int field_count = 0;

    char *token = strtok((char *)gga_str, ",");
    while (token && field_count < GPSgga_MAX_FIELDS) {
        strncpy(fields[field_count], token, 31);
        fields[field_count][31] = '\0';
        field_count++;
        token = strtok(NULL, ",");
    }

    if (field_count < 11) {
        return SENSOR_ERROR;
    }

    /* 时间 (UTC) */
    if (strlen(fields[1]) > 0) {
        dev->data.hour = (fields[1][0] - '0') * 10 + (fields[1][1] - '0');
        dev->data.minute = (fields[1][2] - '0') * 10 + (fields[1][3] - '0');
        dev->data.second = (fields[1][4] - '0') * 10 + (fields[1][5] - '0');
        if (strlen(fields[1]) > 7) {
            dev->data.millisecond = atoi(fields[1] + 7);
        }
    }

    /* 纬度 */
    if (strlen(fields[2]) > 0) {
        double lat_dmm = atof(fields[2]);
        char ns = fields[3][0];
        dev->data.latitude = gps_nmea_lat_to_deg(lat_dmm, ns);
    }

    /* 经度 */
    if (strlen(fields[4]) > 0) {
        double lon_dmm = atof(fields[4]);
        char ew = fields[5][0];
        dev->data.longitude = gps_nmea_lon_to_deg(lon_dmm, ew);
    }

    /* 定位质量 */
    dev->data.quality = (gps_quality_t)atoi(fields[6]);
    dev->data.data_valid = (dev->data.quality > GPS_QUALITY_INVALID);

    /* 卫星数 */
    dev->data.satellites_used = (uint8_t)atoi(fields[7]);

    /* HDOP */
    if (strlen(fields[8]) > 0) {
        dev->data.hdop = (float)atof(fields[8]);
    }

    /* 海拔高度 */
    if (strlen(fields[9]) > 0) {
        dev->data.altitude = (float)atof(fields[9]);
    }

    /* 大地水准面高度 */
    if (strlen(fields[11]) > 0) {
        dev->data.geoid_height = (float)atof(fields[11]);
    }

    /* 更新状态标志 */
    if (dev->data.data_valid) {
        dev->data.status_flags |= GPS_FLAG_FIXED;
        dev->last_valid_data = dev->data;
        dev->last_valid_time = sensor_get_tick();
    }

    return SENSOR_EOK;
}

/**
 * @brief   解析 NMEA RMC 语句 (推荐最小定位)
 */
static sensor_err_t gps_parse_rmc(gps_device_t *dev, const char *rmc_str)
{
    char fields[12][32];
    int field_count = 0;

    char *token = strtok((char *)rmc_str, ",");
    while (token && field_count < 12) {
        strncpy(fields[field_count], token, 31);
        fields[field_count][31] = '\0';
        field_count++;
        token = strtok(NULL, ",");
    }

    if (field_count < 10) {
        return SENSOR_ERROR;
    }

    /* 时间 */
    if (strlen(fields[1]) > 0) {
        dev->data.hour = (fields[1][0] - '0') * 10 + (fields[1][1] - '0');
        dev->data.minute = (fields[1][2] - '0') * 10 + (fields[1][3] - '0');
        dev->data.second = (fields[1][4] - '0') * 10 + (fields[1][5] - '0');
    }

    /* 状态 (A=有效, V=无效) */
    if (fields[2][0] == 'A') {
        dev->data.data_valid = true;
    } else {
        dev->data.data_valid = false;
    }

    /* 纬度 */
    if (strlen(fields[3]) > 0) {
        double lat_dmm = atof(fields[3]);
        char ns = fields[4][0];
        dev->data.latitude = gps_nmea_lat_to_deg(lat_dmm, ns);
    }

    /* 经度 */
    if (strlen(fields[5]) > 0) {
        double lon_dmm = atof(fields[5]);
        char ew = fields[6][0];
        dev->data.longitude = gps_nmea_lon_to_deg(lon_dmm, ew);
    }

    /* 速度 (节) */
    if (strlen(fields[7]) > 0) {
        float speed_knots = (float)atof(fields[7]);
        dev->data.speed = speed_knots * 0.514444f;  // 转换为 m/s
    }

    /* 航向角 */
    if (strlen(fields[8]) > 0) {
        dev->data.course = (float)atof(fields[8]);
    }

    /* 日期 */
    if (strlen(fields[9]) > 0) {
        dev->data.day = (fields[9][0] - '0') * 10 + (fields[9][1] - '0');
        dev->data.month = (fields[9][2] - '0') * 10 + (fields[9][3] - '0');
        dev->data.year = 2000 + (fields[9][4] - '0') * 10 + (fields[9][5] - '0');
    }

    return SENSOR_EOK;
}

/**
 * @brief   解析 NMEA VTG 语句 (地面速度)
 */
static sensor_err_t gps_parse_vtg(gps_device_t *dev, const char *vtg_str)
{
    char fields[10][32];
    int field_count = 0;

    char *token = strtok((char *)vtg_str, ",");
    while (token && field_count < 10) {
        strncpy(fields[field_count], token, 31);
        fields[field_count][31] = '\0';
        field_count++;
        token = strtok(NULL, ",");
    }

    if (field_count < 9) {
        return SENSOR_ERROR;
    }

    /* 航向角 */
    if (strlen(fields[1]) > 0) {
        dev->data.course = (float)atof(fields[1]);
    }

    /* 速度 (km/h) */
    if (strlen(fields[7]) > 0) {
        float speed_kmh = (float)atof(fields[7]);
        dev->data.speed = speed_kmh / 3.6f;  // 转换为 m/s
    }

    return SENSOR_EOK;
}

/**
 * @brief   解析 NMEA GSV 语句 (可见卫星)
 */
static sensor_err_t gps_parse_gsv(gps_device_t *dev, const char *gsv_str)
{
    char fields[24][32];
    int field_count = 0;

    char *token = strtok((char *)gsv_str, ",");
    while (token && field_count < 24) {
        strncpy(fields[field_count], token, 31);
        fields[field_count][31] = '\0';
        field_count++;
        token = strtok(NULL, ",");
    }

    if (field_count < 8) {
        return SENSOR_ERROR;
    }

    /* 可见卫星总数 */
    uint8_t total_sats = (uint8_t)atoi(fields[3]);
    dev->satellite_count = total_sats;

    /* 解析每颗卫星的信息 */
    uint8_t msg_num = (uint8_t)atoi(fields[2]);
    uint8_t sat_idx = (msg_num - 1) * 4;
    int field_offset = 4;

    for (int i = 0; i < 4 && sat_idx < GPS_SATELLITE_MAX; i++) {
        if (field_offset + 3 < field_count) {
            dev->satellites[sat_idx].id = (uint8_t)atoi(fields[field_offset]);
            dev->satellites[sat_idx].elevation = (uint8_t)atoi(fields[field_offset + 1]);
            dev->satellites[sat_idx].azimuth = (uint8_t)atoi(fields[field_offset + 2]);
            dev->satellites[sat_idx].snr = (uint8_t)atoi(fields[field_offset + 3]);
            sat_idx++;
        }
        field_offset += 4;
    }

    return SENSOR_EOK;
}

/**
 * @brief   解析单个 NMEA 语句
 */
sensor_err_t gps_parse_nmea(gps_device_t *dev, const char *nmea_str, uint16_t len)
{
    if (dev == NULL || nmea_str == NULL || len == 0) {
        return SENSOR_EINVAL;
    }

    /* 校验和检查 (可选) */
    if (dev->config.checksum_check && !gps_validate_checksum(nmea_str)) {
        dev->parse_errors++;
        return SENSOR_ERROR;
    }

    /* 识别 NMEA 语句类型 */
    if (strncmp(nmea_str + 3, "GGA,", 4) == 0) {
        return gps_parse_gga(dev, nmea_str);
    } else if (strncmp(nmea_str + 3, "RMC,", 4) == 0) {
        return gps_parse_rmc(dev, nmea_str);
    } else if (strncmp(nmea_str + 3, "VTG,", 4) == 0) {
        return gps_parse_vtg(dev, nmea_str);
    } else if (strncmp(nmea_str + 3, "GSV,", 4) == 0) {
        return gps_parse_gsv(dev, nmea_str);
    }

    return SENSOR_EOK;
}

/**
 * @brief   按字节解析 NMEA 数据
 */
sensor_err_t gps_parse_byte(gps_device_t *dev, uint8_t byte)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    /* 寻找句子开始 */
    if (byte == '$') {
        dev->nmea_buf_idx = 0;
        memset(dev->nmea_buf, 0, GPS_NMEA_BUF_SIZE);
    }

    /* 填充缓冲区 */
    if (dev->nmea_buf_idx < GPS_NMEA_BUF_SIZE - 1) {
        dev->nmea_buf[dev->nmea_buf_idx++] = byte;
    }

    /* 检测句子结束 (换行) */
    if (byte == '\n') {
        dev->nmea_buf[dev->nmea_buf_idx] = '\0';

        /* 解析此句 */
        gps_parse_nmea(dev, dev->nmea_buf, dev->nmea_buf_idx);
        dev->frame_count++;

        dev->nmea_buf_idx = 0;
    }

    return SENSOR_EOK;
}

/* ==================== GPS 读取 ==================== */
sensor_err_t gps_read_data(gps_device_t *dev, gps_data_t *data)
{
    if (dev == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    *data = dev->data;
    return SENSOR_EOK;
}

/* ==================== GPS 工具函数 ==================== */
float gps_distance(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = (lat2 - lat1) * GPS_PI / 180.0;
    double dlon = (lon2 - lon1) * GPS_PI / 180.0;

    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(lat1 * GPS_PI / 180.0) * cos(lat2 * GPS_PI / 180.0) *
               sin(dlon / 2) * sin(dlon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return (float)(GPS_EARTH_R * c);
}

float gps_bearing(double lat1, double lon1, double lat2, double lon2)
{
    double dlon = (lon2 - lon1) * GPS_PI / 180.0;
    double lat1_rad = lat1 * GPS_PI / 180.0;
    double lat2_rad = lat2 * GPS_PI / 180.0;

    double y = sin(dlon) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) -
               sin(lat1_rad) * cos(lat2_rad) * cos(dlon);

    float bearing = (float)(atan2(y, x) * 180.0 / GPS_PI);
    if (bearing < 0) {
        bearing += 360.0f;
    }

    return bearing;
}

/* ==================== GPS 设备注册 ==================== */
sensor_err_t gps_register(gps_device_t *dev)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    if (g_gps_count >= GPS_SATELLITE_MAX) {
        return SENSOR_ENOMEM;
    }

    /* 检查重名 */
    for (int i = 0; i < g_gps_count; i++) {
        if (strcmp(g_gps_devices[i]->base.info.name, dev->base.info.name) == 0) {
            return SENSOR_EINVAL;
        }
    }

    g_gps_devices[g_gps_count++] = dev;
    dev->base.status = SENSOR_STATUS_READY;

    return SENSOR_EOK;
}

sensor_err_t gps_unregister(gps_device_t *dev)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    for (int i = 0; i < g_gps_count; i++) {
        if (g_gps_devices[i] == dev) {
            /* 移除并压缩数组 */
            for (int j = i; j < g_gps_count - 1; j++) {
                g_gps_devices[j] = g_gps_devices[j + 1];
            }
            g_gps_count--;
            dev->base.status = SENSOR_STATUS_IDLE;
            return SENSOR_EOK;
        }
    }

    return SENSOR_ENODEV;
}

gps_device_t *gps_find(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; i < g_gps_count; i++) {
        if (strcmp(g_gps_devices[i]->base.info.name, name) == 0) {
            return g_gps_devices[i];
        }
    }

    return NULL;
}

uint8_t gps_get_count(void)
{
    return g_gps_count;
}

/* ==================== GPS 功耗管理 ==================== */
sensor_err_t gps_sleep(gps_device_t *dev)
{
    /* 发送待机命令到 GPS 模块 */
    /* 具体实现依赖硬件 */
    dev->base.status = SENSOR_STATUS_IDLE;
    return SENSOR_EOK;
}

sensor_err_t gps_wakeup(gps_device_t *dev)
{
    /* 唤醒 GPS 模块 */
    dev->base.status = SENSOR_STATUS_READY;
    return SENSOR_EOK;
}

/* ==================== 查询函数 ==================== */
bool gps_is_fixed(gps_device_t *dev)
{
    if (dev == NULL) {
        return false;
    }
    return (dev->data.data_valid && dev->data.quality > GPS_QUALITY_INVALID);
}

sensor_err_t gps_get_position(gps_device_t *dev, double *lat, double *lon, float *alt)
{
    if (dev == NULL || !gps_is_fixed(dev)) {
        return SENSOR_ERROR;
    }

    *lat = dev->data.latitude;
    *lon = dev->data.longitude;
    *alt = dev->data.altitude;

    return SENSOR_EOK;
}

sensor_err_t gps_get_time(gps_device_t *dev, uint32_t *timestamp)
{
    if (dev == NULL || timestamp == NULL) {
        return SENSOR_EINVAL;
    }

    /* 构建 Unix 时间戳 (简化版) */
    /* 完整实现需要处理闰年和月份 */
    uint32_t year = dev->data.year;
    uint32_t month = dev->data.month;
    uint32_t day = dev->data.day;

    /* 简化: 从 2020-01-01 开始计算天数 */
    uint32_t days = (year - 2020) * 365 + (year - 2020) / 4;
    days += (month - 1) * 30 + day;
    days -= 1;  /* 修正偏移 */

    *timestamp = days * 86400UL + dev->data.hour * 3600UL +
                dev->data.minute * 60UL + dev->data.second;

    return SENSOR_EOK;
}

sensor_err_t gps_get_satellites(gps_device_t *dev, uint8_t *count,
                                uint8_t *satellites_used, uint8_t *satellites_visible)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    if (count) *count = dev->satellite_count;
    if (satellites_used) *satellites_used = dev->data.satellites_used;
    if (satellites_visible) *satellites_visible = dev->satellite_count;

    return SENSOR_EOK;
}

sensor_err_t gps_get_dop(gps_device_t *dev, float *hdop, float *pdop, float *vdop)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    if (hdop) *hdop = dev->data.hdop;
    if (pdop) *pdop = dev->data.pdop;
    if (vdop) *vdop = dev->data.vdop;

    return SENSOR_EOK;
}

/* ==================== GPS 命令发送 ==================== */
sensor_err_t gps_send_command(gps_device_t *dev, const char *cmd, uint16_t len)
{
    (void)dev;
    (void)cmd;
    (void)len;
    /* 实现依赖具体 UART 驱动 */
    return SENSOR_EOK;
}

sensor_err_t gps_set_baudrate(gps_device_t *dev, uint32_t baudrate)
{
    if (dev == NULL) {
        return SENSOR_EINVAL;
    }

    dev->config.baudrate = baudrate;

    /* 发送命令到 GPS 模块更改波特率 */
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "$PCFG,115200,8,N,1,0*...\r\n");
    /* 实际实现需要根据模块类型 */

    return SENSOR_EOK;
}

sensor_err_t gps_set_update_rate(gps_device_t *dev, uint32_t rate_hz)
{
    if (dev == NULL || rate_hz == 0 || rate_hz > 10) {
        return SENSOR_EINVAL;
    }

    dev->config.update_rate = rate_hz;

    /* 发送更新率命令到 GPS 模块 */
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "$PUBX,41,1,0007,0001,%d,0*..\r\n", rate_hz);

    return SENSOR_EOK;
}

/* ==================== 获取运行时间 ==================== */
uint32_t gps_get_uptime(gps_device_t *dev)
{
    if (dev == NULL) {
        return 0;
    }
    return sensor_get_tick() - dev->last_update_time;
}

/* ==================== 获取 sensor_device (兼容框架) ==================== */
sensor_err_t gps_sensor_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    gps_device_t *gps = (gps_device_t *)sensor;
    gps_data_t gps_data;

    gps_read_data(gps, &gps_data);

    data->type = SENSOR_TYPE_GPS;
    data->unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->timestamp = sensor_get_tick();

    if (gps_data.data_valid) {
        /* 存储为 3 轴格式: lat, lon, alt */
        data->value.val_3axis.x = (int32_t)(gps_data.latitude * 1e6);
        data->value.val_3axis.y = (int32_t)(gps_data.longitude * 1e6);
        data->value.val_3axis.z = (int32_t)(gps_data.altitude * 1000);
    } else {
        data->value.val_3axis.x = 0;
        data->value.val_3axis.y = 0;
        data->value.val_3axis.z = 0;
    }

    return SENSOR_EOK;
}