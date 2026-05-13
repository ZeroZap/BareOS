/**
 * @file xy_fuel_gauge.h
 * @brief Fuel Gauge Component - Battery Management
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Zephyr fuel_gauge drivers
 * https://github.com/zephyrproject-rtos/zephyr/tree/main/drivers/fuel_gauge
 */

#ifndef XY_FUEL_GAUGE_H
#define XY_FUEL_GAUGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 版本信息 ==================== */

#define XY_FUEL_GAUGE_VERSION_MAJOR     1
#define XY_FUEL_GAUGE_VERSION_MINOR     0
#define XY_FUEL_GAUGE_VERSION_PATCH     0

/* ==================== 状态码 ==================== */

typedef enum {
    XY_FG_OK = 0,
    XY_FG_ERROR,
    XY_FG_ERROR_INVALID_PARAM,
    XY_FG_ERROR_NOT_INITIALIZED,
    XY_FG_ERROR_NOT_SUPPORTED,
    XY_FG_ERROR_TIMEOUT,
    XY_FG_ERROR_NO_DATA,
    XY_FG_ERROR_BUSY,
} xy_fuel_gauge_status_t;

/* ==================== 数据结构 ==================== */

/**
 * @brief 电量计数据类型
 */
typedef enum {
    XY_FG_DATA_VOLTAGE = 0,       /* 电池电压 (mV) */
    XY_FG_DATA_CURRENT,           /* 电流 (mA, 正=充电，负=放电) */
    XY_FG_DATA_SOC,               /* 电量百分比 (0-100%) */
    XY_FG_DATA_SOH,               /* 健康度 (0-100%) */
    XY_FG_DATA_TEMPERATURE,       /* 温度 (0.1°C) */
    XY_FG_DATA_CYCLE_COUNT,       /* 循环次数 */
    XY_FG_DATA_FULL_CAPACITY,     /* 满充容量 (mAh) */
    XY_FG_DATA_REMAIN_CAPACITY,   /* 剩余容量 (mAh) */
    XY_FG_DATA_AVERAGE_CURRENT,   /* 平均电流 (mA) */
    XY_FG_DATA_TIME_TO_EMPTY,     /* 空电时间 (分钟) */
    XY_FG_DATA_TIME_TO_FULL,      /* 充满时间 (分钟) */
} xy_fuel_gauge_data_type_t;

/**
 * @brief 电量计数据
 */
typedef struct {
    uint16_t voltage_mv;          /* 电池电压 (mV) */
    int16_t  current_ma;          /* 电流 (mA) */
    uint8_t  soc;                 /* 电量百分比 (0-100%) */
    uint8_t  soh;                 /* 健康度 (0-100%) */
    int16_t  temperature_c;       /* 温度 (0.1°C) */
    uint32_t cycle_count;         /* 循环次数 */
    uint16_t full_capacity_mah;   /* 满充容量 (mAh) */
    uint16_t remain_capacity_mah; /* 剩余容量 (mAh) */
    uint32_t timestamp;           /* 时间戳 (ms) */
} xy_fuel_gauge_data_t;

/**
 * @brief 告警配置
 */
typedef struct {
    uint8_t  low_soc_threshold;   /* 低电量告警阈值 (%) */
    uint8_t  high_soc_threshold;  /* 高电量告警阈值 (%) */
    uint16_t low_voltage_mv;      /* 低电压告警阈值 (mV) */
    uint16_t high_voltage_mv;     /* 高电压告警阈值 (mV) */
    int16_t  over_current_ma;     /* 过流告警阈值 (mA) */
    int16_t  over_temp_c;         /* 过温告警阈值 (0.1°C) */
} xy_fuel_gauge_alert_t;

/**
 * @brief 电量计驱动 API
 */
typedef struct {
    int (*init)(struct xy_fuel_gauge *fg);
    int (*fetch)(struct xy_fuel_gauge *fg);
    int (*channel_get)(struct xy_fuel_gauge *fg, 
                       xy_fuel_gauge_data_type_t channel, 
                       int32_t *val);
    int (*alert_set)(struct xy_fuel_gauge *fg, 
                     const xy_fuel_gauge_alert_t *alert);
    int (*alert_get)(struct xy_fuel_gauge *fg, 
                     xy_fuel_gauge_alert_t *alert);
} xy_fuel_gauge_api_t;

/**
 * @brief 电量计设备
 */
typedef struct xy_fuel_gauge {
    const char *name;
    const xy_fuel_gauge_api_t *api;
    void *data;
    xy_fuel_gauge_data_t latest;
    bool initialized;
    struct xy_fuel_gauge *next;
} xy_fuel_gauge_t;

/* ==================== 核心 API ==================== */

/**
 * @brief 初始化电量计
 * @param fg 电量计设备
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_init(xy_fuel_gauge_t *fg);

/**
 * @brief 反初始化电量计
 * @param fg 电量计设备
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_deinit(xy_fuel_gauge_t *fg);

/**
 * @brief 获取最新数据
 * @param fg 电量计设备
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_fetch(xy_fuel_gauge_t *fg);

/**
 * @brief 读取指定数据
 * @param fg 电量计设备
 * @param type 数据类型
 * @param val 输出值
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_get(xy_fuel_gauge_t *fg,
                                         xy_fuel_gauge_data_type_t type,
                                         int32_t *val);

/**
 * @brief 读取电池电压
 * @param fg 电量计设备
 * @param voltage_mv 电压 (mV)
 * @return 状态码
 */
static inline xy_fuel_gauge_status_t 
xy_fuel_gauge_get_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv)
{
    int32_t val;
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &val);
    if (ret == XY_FG_OK && voltage_mv) {
        *voltage_mv = (uint16_t)val;
    }
    return ret;
}

/**
 * @brief 读取电流
 * @param fg 电量计设备
 * @param current_ma 电流 (mA)
 * @return 状态码
 */
static inline xy_fuel_gauge_status_t 
xy_fuel_gauge_get_current(xy_fuel_gauge_t *fg, int16_t *current_ma)
{
    int32_t val;
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &val);
    if (ret == XY_FG_OK && current_ma) {
        *current_ma = (int16_t)val;
    }
    return ret;
}

/**
 * @brief 读取电量百分比 (SOC)
 * @param fg 电量计设备
 * @param soc_pct 电量百分比 (0-100)
 * @return 状态码
 */
static inline xy_fuel_gauge_status_t 
xy_fuel_gauge_get_soc(xy_fuel_gauge_t *fg, uint8_t *soc_pct)
{
    int32_t val;
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &val);
    if (ret == XY_FG_OK && soc_pct) {
        *soc_pct = (uint8_t)val;
    }
    return ret;
}

/**
 * @brief 读取健康度 (SOH)
 * @param fg 电量计设备
 * @param soh_pct 健康度 (0-100)
 * @return 状态码
 */
static inline xy_fuel_gauge_status_t 
xy_fuel_gauge_get_soh(xy_fuel_gauge_t *fg, uint8_t *soh_pct)
{
    int32_t val;
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &val);
    if (ret == XY_FG_OK && soh_pct) {
        *soh_pct = (uint8_t)val;
    }
    return ret;
}

/**
 * @brief 读取温度
 * @param fg 电量计设备
 * @param temp_c 温度 (0.1°C)
 * @return 状态码
 */
static inline xy_fuel_gauge_status_t 
xy_fuel_gauge_get_temperature(xy_fuel_gauge_t *fg, int16_t *temp_c)
{
    int32_t val;
    xy_fuel_gauge_status_t ret = xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &val);
    if (ret == XY_FG_OK && temp_c) {
        *temp_c = (int16_t)val;
    }
    return ret;
}

/**
 * @brief 设置告警阈值
 * @param fg 电量计设备
 * @param alert 告警配置
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_set_alert(xy_fuel_gauge_t *fg,
                                               const xy_fuel_gauge_alert_t *alert);

/**
 * @brief 获取告警状态
 * @param fg 电量计设备
 * @param alert 告警配置
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_get_alert(xy_fuel_gauge_t *fg,
                                               xy_fuel_gauge_alert_t *alert);

/* ==================== 设备管理 ==================== */

/**
 * @brief 注册电量计设备
 * @param fg 电量计设备
 * @return 状态码
 */
xy_fuel_gauge_status_t xy_fuel_gauge_device_register(xy_fuel_gauge_t *fg);

/**
 * @brief 根据名称获取电量计设备
 * @param name 设备名称
 * @return 设备指针，NULL 表示未找到
 */
xy_fuel_gauge_t* xy_fuel_gauge_device_get(const char *name);

/**
 * @brief 遍历所有电量计设备
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void xy_fuel_gauge_device_foreach(void (*callback)(xy_fuel_gauge_t *, void *),
                                  void *user_data);

/**
 * @brief 获取电量计设备数量
 * @return 设备数量
 */
uint8_t xy_fuel_gauge_device_count(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_FUEL_GAUGE_H */
