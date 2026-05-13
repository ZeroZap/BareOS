/**
 * @file at_power.h
 * @brief AT设备电源管理框架
 */

#ifndef AT_POWER_H
#define AT_POWER_H

#include "at_client.h"

/* 电源控制命令 */
typedef enum {
    PWR_CMD_POWER_ON = 0,   /* 开机 */
    PWR_CMD_POWER_OFF,      /* 关机 */
    PWR_CMD_RESET,          /* 复位 */
    PWR_CMD_SLEEP,          /* 进入睡眠 */
    PWR_CMD_WAKEUP,         /* 唤醒 */
    PWR_CMD_LOW_POWER,      /* 进入低功耗模式 */
    PWR_CMD_FULL_POWER,     /* 进入全功耗模式 */
    PWR_CMD_AIRPLANE_ON,    /* 飞行模式开启 */
    PWR_CMD_AIRPLANE_OFF,   /* 飞行模式关闭 */
    PWR_CMD_MIN_FUNCTIONAL, /* 最小功能模式 */
    PWR_CMD_GET_STATUS      /* 获取电源状态 */
} pwr_cmd_t;

/* 电源状态 */
typedef enum {
    PWR_STATE_OFF = 0,    /* 关机状态 */
    PWR_STATE_ON,         /* 开机状态 */
    PWR_STATE_SLEEP,      /* 睡眠状态 */
    PWR_STATE_DEEP_SLEEP, /* 深度睡眠 */
    PWR_STATE_LOW_POWER,  /* 低功耗状态 */
    PWR_STATE_AIRPLANE,   /* 飞行模式 */
    PWR_STATE_ERROR       /* 错误状态 */
} pwr_state_t;

/* 电源回调函数类型 */
typedef int (*pwr_ctrl_callback_t)(at_device_t *dev, pwr_cmd_t cmd,
                                   void *param);

/* 电源管理配置 */
typedef struct {
    /* 硬件控制引脚 */
    struct {
        uint8_t pwr_key_pin; /* 电源键引脚 */
        uint8_t reset_pin;   /* 复位引脚 */
        uint8_t wakeup_pin;  /* 唤醒引脚 */
        uint8_t status_pin;  /* 状态引脚 */
    } gpio;

    /* 定时参数（单位：毫秒） */
    struct {
        uint32_t power_on_time; /* 开机持续时间 */
        uint32_t reset_time;    /* 复位持续时间 */
        uint32_t wakeup_time;   /* 唤醒持续时间 */
        uint32_t boot_delay;    /* 启动延迟时间 */
    } timing;

    /* 电源控制回调 */
    pwr_ctrl_callback_t ctrl_callback;
    void *callback_param;

    /* 状态管理 */
    pwr_state_t current_state;
    pwr_state_t target_state;
    uint32_t last_state_change;
    bool auto_power_manage; /* 自动电源管理 */

    /* 功耗配置 */
    struct {
        uint32_t sleep_timeout;   /* 睡眠超时时间 */
        uint32_t wakeup_interval; /* 唤醒间隔 */
        uint8_t power_level;      /* 功率等级 0-最大 9-最小 */
    } power_config;
} at_power_config_t;

/* API函数声明 */

/**
 * @brief 初始化电源管理
 */
int at_power_init(at_device_t *dev, at_power_config_t *config);

/**
 * @brief 设置电源控制回调函数
 */
int at_power_set_callback(at_device_t *dev, pwr_ctrl_callback_t callback,
                          void *param);

/**
 * @brief 控制设备电源
 */
int at_power_control(at_device_t *dev, pwr_cmd_t cmd, void *param);

/**
 * @brief 获取电源状态
 */
pwr_state_t at_power_get_status(at_device_t *dev);

/**
 * @brief 设置自动电源管理
 */
int at_power_set_auto_manage(at_device_t *dev, bool enable,
                             uint32_t sleep_timeout, uint32_t wakeup_interval);

/**
 * @brief 更新电源管理状态
 */
int at_power_update(at_device_t *dev);

/**
 * @brief 硬件复位设备
 */
int at_power_hard_reset(at_device_t *dev);

/**
 * @brief 软件复位设备
 */
int at_power_soft_reset(at_device_t *dev);

/**
 * @brief 获取功耗统计
 */
int at_power_get_statistics(at_device_t *dev, uint32_t *total_on_time,
                            uint32_t *total_sleep_time, uint32_t *power_cycles);

/**
 * @brief 进入低功耗模式
 */
int at_power_enter_low_power(at_device_t *dev);

/**
 * @brief 退出低功耗模式
 */
int at_power_exit_low_power(at_device_t *dev);

/**
 * @brief 强制关机
 */
int at_power_force_off(at_device_t *dev);

#endif /* AT_POWER_H */