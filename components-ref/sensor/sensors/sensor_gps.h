/**
 * SPDX-License-Identifier: MIT
 * @file    xy_gps.h
 * @brief   GPS Sensor Driver Interface for XinYi Framework
 * @version 1.0.0
 *
 * 位置传感器 - GPS/GNSS 模块
 * 适用于 NMEA 协议输出的 GPS 模块（UBLOX、AT6558、LC86L 等）
 */

#ifndef __XY_GPS_H__
#define __XY_GPS_H__

#include "sensor_type.h"
#include "sensor_core.h"

#include <stdint.h>
#include <stdbool.h>

/* ==================== 常量定义 ==================== */
#define GPS_NAME_MAX_LEN       32
#define GPS_SATELLITE_MAX      64
#define GPS_NMEA_BUF_SIZE      256
#define GPSgga_MAX_FIELDS     15

/* ==================== 定位质量 ==================== */
typedef enum {
    GPS_QUALITY_INVALID      = 0,    ///< 无效定位
    GPS_QUALITY_GPS_SPS      = 1,    ///< GPS 单点定位
    GPS_QUALITY_DGPS         = 2,    ///< 差分 GPS
    GPS_QUALITY_PPS          = 3,    ///< 精密定位服务
    GPS_QUALITY_RTK_FIXED    = 4,    ///< RTK 固定解
    GPS_QUALITY_RTK_FLOAT    = 5,   ///< RTK 浮点解
    GPS_QUALITY_ESTIMATED    = 6,   ///< 估算模式
    GPS_QUALITY_MANUAL       = 7,   ///< 手动输入
    GPS_QUALITY_SIMULATION   = 8,   ///< 模拟模式
} gps_quality_t;

/* ==================== 定位模式 ==================== */
typedef enum {
    GPS_MODE_NONE            = 0,    ///< 无定位
    GPS_MODE_2D              = 1,    ///< 2D 定位
    GPS_MODE_3D              = 2,    ///< 3D 定位
    GPS_MODE_DGPS            = 3,    ///< 差分定位
    GPS_MODE_RTK             = 4,   ///< RTK 定位
} gps_mode_t;

/* ==================== NMEA 语句类型 ==================== */
typedef enum {
    GPS_NMEA_GGA             = 0x01,  ///< 定位数据
    GPS_NMEA_RMC             = 0x02,  ///< 推荐最小定位
    GPS_NMEA_GSV             = 0x04,  ///< 可见卫星
    GPS_NMEA_GLL             = 0x08,  ///< 地理坐标
    GPS_NMEA_GSA             = 0x10,  ///< 卫星状态
    GPS_NMEA_VTG             = 0x20,  ///< 地面速度
    GPS_NMEA_ZDA            = 0x40,  ///< 时间日期
} gps_nmea_type_t;

/* ==================== GPS 数据结构 ==================== */
typedef struct {
    /* 位置信息 */
    double latitude;                  ///< 纬度 (度，-90~90，北正南负)
    double longitude;                 ///< 经度 (度，-180~180，东正西负)
    float altitude;                   ///< 海拔高度 (米)
    float geoid_height;               ///< 大地水准面高度 (米)

    /* 运动信息 */
    float speed;                      ///< 速度 (米/秒)
    float course;                     ///< 航向角 (度，0~360，从北顺时针)
    float magnetic_variation;         ///< 磁偏角 (度)

    /* 时间信息 */
    uint32_t hour;                    ///< 小时 (0-23)
    uint32_t minute;                  ///< 分钟 (0-59)
    uint32_t second;                  ///< 秒 (0-59)
    uint32_t millisecond;             ///< 毫秒 (0-999)
    uint32_t day;                     ///< 日 (1-31)
    uint32_t month;                   ///< 月 (1-12)
    uint32_t year;                    ///< 年 (4位数)
    uint32_t timestamp;               ///< Unix 时间戳 (秒)

    /* 定位状态 */
    gps_quality_t quality;            ///< 定位质量
    gps_mode_t mode;                  ///< 定位模式
    uint8_t satellites_used;          ///< 使用的卫星数 (0-64)
    float hdop;                       ///< 水平精度因子
    float pdop;                       ///< 位置精度因子
    float vdop;                       ///< 垂直精度因子

    /* 状态标志 */
    bool data_valid;                  ///< 数据是否有效
    uint8_t status_flags;             ///< 状态标志位
} gps_data_t;

/* ==================== GPS 配置 ==================== */
typedef struct {
    /* 串口配置 */
    uint32_t baudrate;                ///< 波特率 (默认 9600)
    uint8_t data_bits;                ///< 数据位 (默认 8)
    uint8_t stop_bits;                ///< 停止位 (默认 1)
    char parity;                      ///< 校验位 ('N', 'O', 'E')

    /* GPS 配置 */
    uint32_t update_rate;             ///< 更新频率 (Hz，默认 1)
    gps_nmea_type_t nmea_output;     ///< 输出 NMEA 语句类型
    bool checksum_check;              ///< 是否校验 NMEA 校验和
    bool datum_set;                   ///< 是否设置坐标系
} gps_config_t;

/* ==================== GPS 设备扩展 ==================== */
typedef struct gps_device {
    /* 基础 sensor_device */
    sensor_device_t base;

    /* GPS 特定配置 */
    gps_config_t config;

    /* GPS 数据缓存 */
    gps_data_t data;
    gps_data_t last_valid_data;       ///< 上次有效数据

    /* NMEA 解析状态 */
    char nmea_buf[GPS_NMEA_BUF_SIZE];
    uint16_t nmea_buf_idx;
    uint32_t parse_errors;
    uint32_t frame_count;

    /* 卫星信息 */
    struct {
        uint8_t id;
        uint8_t elevation;            ///< 仰角 (度)
        uint8_t azimuth;              ///< 方位角 (度)
        uint8_t snr;                  ///< 信噪比 (dB-Hz)
    } satellites[GPS_SATELLITE_MAX];
    uint8_t satellite_count;

    /* 时间戳 */
    uint32_t last_update_time;
    uint32_t last_valid_time;
} gps_device_t;

/* ==================== GPS 操作接口 ==================== */
typedef struct gps_ops {
    /* 基础操作 (必须实现) */
    sensor_err_t (*init)(gps_device_t *dev);
    sensor_err_t (*deinit)(gps_device_t *dev);

    /* NMEA 数据输入 (必须实现) */
    sensor_err_t (*parse_nmea)(gps_device_t *dev, const char *nmea_str, uint16_t len);
    sensor_err_t (*parse_byte)(gps_device_t *dev, uint8_t byte);

    /* 数据读取 */
    sensor_err_t (*read)(gps_device_t *dev, gps_data_t *data);

    /* 配置控制 */
    sensor_err_t (*config)(gps_device_t *dev, const gps_config_t *config);
    sensor_err_t (*get_config)(gps_device_t *dev, gps_config_t *config);

    /* 功耗管理 */
    sensor_err_t (*sleep)(gps_device_t *dev);
    sensor_err_t (*wakeup)(gps_device_t *dev);
    sensor_err_t (*hot_start)(gps_device_t *dev);
    sensor_err_t (*warm_start)(gps_device_t *dev);
    sensor_err_t (*cold_start)(gps_device_t *dev);

    /* 定位查询 */
    sensor_err_t (*get_position)(gps_device_t *dev, double *lat, double *lon, float *alt);
    sensor_err_t (*get_speed_course)(gps_device_t *dev, float *speed, float *course);
    sensor_err_t (*get_time)(gps_device_t *dev, uint32_t *timestamp);
    sensor_err_t (*get_satellites)(gps_device_t *dev, uint8_t *count,
                                   uint8_t *satellites_used, uint8_t *satellites_visible);
    sensor_err_t (*get_dop)(gps_device_t *dev, float *hdop, float *pdop, float *vdop);

    /* 控制命令 */
    sensor_err_t (*send_command)(gps_device_t *dev, const char *cmd, uint16_t len);
    sensor_err_t (*set_baudrate)(gps_device_t *dev, uint32_t baudrate);
    sensor_err_t (*set_update_rate)(gps_device_t *dev, uint32_t rate_hz);
    sensor_err_t (*set_nmea_output)(gps_device_t *dev, gps_nmea_type_t types);

    /* 状态查询 */
    bool (*is_fixed)(gps_device_t *dev);
    uint32_t (*get_uptime)(gps_device_t *dev);
} gps_ops_t;

/* ==================== GPS 设备注册宏 ==================== */
#define GPS_DEVICE_INIT(_name, _ops, _bus, _priv) \
    {                                              \
        .base.ops          = (const sensor_ops_t *)_ops,  \
        .base.bus          = _bus,                 \
        .base.priv_data    = _priv,                \
        .base.info.name    = _name,                \
        .base.info.type    = SENSOR_TYPE_GPS,      \
        .base.info.unit    = SENSOR_UNIT_DEGREE_PER_SECOND, \
        .config.baudrate   = 9600,                 \
        .config.update_rate = 1,                   \
        .config.nmea_output = GPS_NMEA_GGA | GPS_NMEA_RMC | GPS_NMEA_GSV, \
        .config.checksum_check = true,             \
    }

/* ==================== 标准 NMEA 解析宏 ==================== */
#define GPS_NMEA_HEAD(_str, _talker, _type) \
    ((_str)[0] == '$' && \
     (_str)[1] == (_talker)[0] && \
     (_str)[2] == (_talker)[1] && \
     (_str)[3] == (_type)[0] && \
     (_str)[4] == (_type)[1])

#define GPS_NMEA_CHECKSUM(_str) \
    ({                                                         \
        const char *_s = (_str);                               \
        uint8_t _cs = 0;                                       \
        while (*_s && *_s != '*') _cs ^= (uint8_t)*_s++;       \
        _cs;                                                   \
    })

/* ==================== GPS 类型定义简化 ==================== */
/* 在 sensor_type.h 中已定义 SENSOR_TYPE_GPS = 0x22 */

/* ==================== 状态标志位 ==================== */
#define GPS_FLAG_FIXED         (1 << 0)   ///< 已定位
#define GPS_FLAG_DGPS          (1 << 1)   ///< 差分定位
#define GPS_FLAG_RTK           (1 << 2)   ///< RTK 定位
#define GPS_FLAG_SBAS          (1 << 3)   ///< SBAS 增强
#define GPS_FLAG_COLD_START    (1 << 4)   ///< 冷启动
#define GPS_FLAG_ANTENNA_SHORT (1 << 5)   ///< 天线短路
#define GPS_FLAG_ANTENNA_OPEN  (1 << 6)   ///< 天线开路

/* ==================== 常用 GPS 模块支持 ==================== */
/**
 * @brief 支持的 GPS 模块类型
 */
typedef enum {
    GPS_CHIP_UNKNOWN = 0,
    GPS_CHIP_UBLOX,          ///< U-Blox (NEO-6M, NEO-7M, NEO-M8N, etc.)
    GPS_CHIP_AT6558,         ///< AT6558 (华大北斗)
    GPS_CHIP_LC86L,          ///< LC86L (中科微)
    GPS_CHIP_MTK3339,        ///< MediaTek MTK3339
    GPS_CHIP_SIRF,           ///< SiRF Star III/IV
    GPS_CHIP_Quectel_L26,    ///< Quectel L26
    GPS_CHIP_SIMCOM_SIM68,   ///< SIMCom SIM68
} gps_chip_type_t;

/* ==================== GPS 工具函数 ==================== */

/**
 * @brief   将 NMEA 纬度格式转换为度
 * @param   dmm: 度分格式 (DDMM.MMMM)
 * @param   dir: 方向 ('N'/'S')
 * @return  度数
 */
static inline double gps_nmea_lat_to_deg(double dmm, char dir)
{
    double deg = (double)((uint32_t)(dmm / 100));
    double min = dmm - deg * 100;
    double result = deg + min / 60.0;
    return (dir == 'S' || dir == 's') ? -result : result;
}

/**
 * @brief   将 NMEA 经度格式转换为度
 * @param   dmm: 度分格式 (DDDMM.MMMM)
 * @param   dir: 方向 ('E'/'W')
 * @return  度数
 */
static inline double gps_nmea_lon_to_deg(double dmm, char dir)
{
    double deg = (double)((uint32_t)(dmm / 100));
    double min = dmm - deg * 100;
    double result = deg + min / 60.0;
    return (dir == 'W' || dir == 'w') ? -result : result;
}

/**
 * @brief   计算两点间的距离 (Haversine)
 * @param   lat1, lon1: 起点坐标
 * @param   lat2, lon2: 终点坐标
 * @return  距离 (米)
 */
float gps_distance(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief   计算方位角
 * @param   lat1, lon1: 起点坐标
 * @param   lat2, lon2: 终点坐标
 * @return  方位角 (度，0-360)
 */
float gps_bearing(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief   检查坐标是否有效
 * @param   lat: 纬度
 * @param   lon: 经度
 * @return  true=有效
 */
static inline bool gps_position_valid(double lat, double lon)
{
    return (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0);
}

/* ==================== GPS 驱动注册 ==================== */

/**
 * @brief   注册 GPS 设备
 * @param   dev: GPS 设备指针
 * @return  传感器错误码
 */
sensor_err_t gps_register(gps_device_t *dev);

/**
 * @brief   注销 GPS 设备
 * @param   dev: GPS 设备指针
 * @return  传感器错误码
 */
sensor_err_t gps_unregister(gps_device_t *dev);

/**
 * @brief   查找 GPS 设备
 * @param   name: 设备名
 * @return  GPS 设备指针
 */
gps_device_t *gps_find(const char *name);

/**
 * @brief   获取 GPS 设备数量
 * @return  已注册设备数
 */
uint8_t gps_get_count(void);

/* ==================== 标准 GPS 驱动适配 ==================== */

/* AT6558 / LC86L (NMEA 协议) */
extern const gps_ops_t gps_at6558_ops;

/* U-Blox (NMEA + UBX 协议) */
extern const gps_ops_t gps_ublox_ops;

/* 通用 NMEA 驱动 */
extern const gps_ops_t gps_generic_ops;

/* ==================== 示例: 注册 GPS 设备 ==================== */
/*
 * 使用示例:
 *
 * gps_device_t my_gps = {
 *     .base.ops = (const sensor_ops_t *)&gps_at6558_ops,
 *     .config.baudrate = 9600,
 *     .config.update_rate = 1,
 * };
 *
 * gps_register(&my_gps);
 *
 * // 在 UART 中断或轮询中:
 * uint8_t c;
 * while (uart_read_byte(&c) == 0) {
 *     gps_at6558_ops.parse_byte(&my_gps, c);
 * }
 *
 * // 读取数据:
 * gps_data_t data;
 * gps_at6558_ops.read(&my_gps, &data);
 * if (data.data_valid) {
 *     printf("Lat: %.6f, Lon: %.6f, Alt: %.1f\n",
 *            data.latitude, data.longitude, data.altitude);
 * }
 */

#endif /* __XY_GPS_H__ */