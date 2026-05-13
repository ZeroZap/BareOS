/**
 * @file at_4g_power.c
 * @brief 4G模块专用电源管理
 */

/**
 * @file at_power.c
 * @brief 电源管理实现
 */

#include "at_power.h"
#include <stdlib.h>
#include <string.h>

/* 电源管理私有数据 */
typedef struct {
    at_power_config_t config;
    bool initialized;
    uint32_t on_start_time;
    uint32_t sleep_start_time;
    uint32_t power_cycle_count;
    uint32_t last_activity_time;
} at_power_private_t;

/* 默认电源控制回调 */
static int default_pwr_callback(at_device_t *dev, pwr_cmd_t cmd, void *param);

/* 初始化电源管理 */
int at_power_init(at_device_t *dev, at_power_config_t *config)
{
    if (!dev) {
        return -1;
    }

    /* 分配私有数据 */
    at_power_private_t *priv =
        (at_power_private_t *)malloc(sizeof(at_power_private_t));
    if (!priv) {
        return -1;
    }

    memset(priv, 0, sizeof(at_power_private_t));

    /* 复制配置 */
    if (config) {
        memcpy(&priv->config, config, sizeof(at_power_config_t));
    } else {
        /* 使用默认配置 */
        memset(&priv->config, 0, sizeof(at_power_config_t));
        priv->config.timing.power_on_time = 2000; /* 2秒开机时间 */
        priv->config.timing.reset_time    = 1000; /* 1秒复位时间 */
        priv->config.timing.wakeup_time   = 100;  /* 100ms唤醒时间 */
        priv->config.timing.boot_delay    = 5000; /* 5秒启动延迟 */
        priv->config.ctrl_callback        = default_pwr_callback;
        priv->config.auto_power_manage    = false;
    }

    priv->config.current_state = PWR_STATE_OFF;
    priv->config.target_state  = PWR_STATE_OFF;
    priv->initialized          = true;
    priv->on_start_time        = 0;
    priv->sleep_start_time     = 0;
    priv->power_cycle_count    = 0;
    priv->last_activity_time   = dev->get_tick();

    /* 将私有数据存储到设备 */
    if (!dev->user_data) {
        /* 如果没有用户数据，直接使用 */
        dev->user_data = priv;
    } else {
        /* 如果已有用户数据，需要整合 */
        // 这里可以设计一个更复杂的结构来整合数据
        // 暂时先简单处理
        void *old_data = dev->user_data;
        dev->user_data = priv;
        priv->config.callback_param = old_data; /* 将原数据保存到回调参数 */
    }

    return 0;
}

/* 控制设备电源 */
int at_power_control(at_device_t *dev, pwr_cmd_t cmd, void *param)
{
    at_power_private_t *priv = (at_power_private_t *)dev->user_data;
    if (!priv || !priv->initialized) {
        return -1;
    }

    int ret               = -1;
    uint32_t current_tick = dev->get_tick();

    switch (cmd) {
    case PWR_CMD_POWER_ON:
        if (priv->config.current_state == PWR_STATE_OFF) {
            if (priv->config.ctrl_callback) {
                ret = priv->config.ctrl_callback(dev, cmd, param);
                if (ret == 0) {
                    priv->config.current_state = PWR_STATE_ON;
                    priv->config.target_state  = PWR_STATE_ON;
                    priv->on_start_time        = current_tick;
                    priv->power_cycle_count++;
                    priv->last_activity_time = current_tick;
                }
            }
        } else {
            ret = 0; /* 已经在开机状态 */
        }
        break;

    case PWR_CMD_POWER_OFF:
        if (priv->config.current_state != PWR_STATE_OFF) {
            if (priv->config.ctrl_callback) {
                ret = priv->config.ctrl_callback(dev, cmd, param);
                if (ret == 0) {
                    priv->config.current_state = PWR_STATE_OFF;
                    priv->config.target_state  = PWR_STATE_OFF;
                    /* 更新开机时间统计 */
                    if (priv->on_start_time > 0) {
                        // 可以在这里记录开机时长
                    }
                }
            }
        } else {
            ret = 0; /* 已经在关机状态 */
        }
        break;

    case PWR_CMD_RESET:
        if (priv->config.ctrl_callback) {
            ret = priv->config.ctrl_callback(dev, cmd, param);
            if (ret == 0) {
                priv->config.current_state = PWR_STATE_OFF;
                priv->config.target_state  = PWR_STATE_ON;
                priv->on_start_time        = current_tick;
                priv->power_cycle_count++;
                priv->last_activity_time = current_tick;
            }
        }
        break;

    case PWR_CMD_SLEEP:
        if (priv->config.current_state == PWR_STATE_ON) {
            /* 先发送AT命令进入睡眠 */
            at_send_cmd("AT+CSCLK=2", NULL, NULL, 1000); /* 深度睡眠模式 */

            if (priv->config.ctrl_callback) {
                ret = priv->config.ctrl_callback(dev, cmd, param);
                if (ret == 0) {
                    priv->config.current_state = PWR_STATE_SLEEP;
                    priv->config.target_state  = PWR_STATE_SLEEP;
                    priv->sleep_start_time     = current_tick;
                }
            }
        }
        break;

    case PWR_CMD_WAKEUP:
        if (priv->config.current_state == PWR_STATE_SLEEP
            || priv->config.current_state == PWR_STATE_DEEP_SLEEP) {
            if (priv->config.ctrl_callback) {
                ret = priv->config.ctrl_callback(dev, cmd, param);
                if (ret == 0) {
                    /* 发送唤醒字符 */
                    dev->write_data(dev, (uint8_t *)"AT\r\n", 4);
                    priv->config.current_state = PWR_STATE_ON;
                    priv->config.target_state  = PWR_STATE_ON;
                    priv->last_activity_time   = current_tick;
                }
            }
        }
        break;

    case PWR_CMD_LOW_POWER:
        /* 设置低功耗模式 */
        at_send_cmd("AT+CFUN=0", NULL, NULL, 1000); /* 最小功能模式 */
        if (priv->config.ctrl_callback) {
            ret = priv->config.ctrl_callback(dev, cmd, param);
            if (ret == 0) {
                priv->config.current_state = PWR_STATE_LOW_POWER;
            }
        }
        break;

    case PWR_CMD_FULL_POWER:
        /* 恢复全功能模式 */
        at_send_cmd("AT+CFUN=1", NULL, NULL, 1000); /* 全功能模式 */
        if (priv->config.ctrl_callback) {
            ret = priv->config.ctrl_callback(dev, cmd, param);
            if (ret == 0) {
                priv->config.current_state = PWR_STATE_ON;
            }
        }
        break;

    case PWR_CMD_AIRPLANE_ON:
        /* 开启飞行模式 */
        at_send_cmd("AT+CFUN=4", NULL, NULL, 1000);
        if (priv->config.ctrl_callback) {
            ret = priv->config.ctrl_callback(dev, cmd, param);
            if (ret == 0) {
                priv->config.current_state = PWR_STATE_AIRPLANE;
            }
        }
        break;

    case PWR_CMD_AIRPLANE_OFF:
        /* 关闭飞行模式 */
        at_send_cmd("AT+CFUN=1", NULL, NULL, 1000);
        if (priv->config.ctrl_callback) {
            ret = priv->config.ctrl_callback(dev, cmd, param);
            if (ret == 0) {
                priv->config.current_state = PWR_STATE_ON;
            }
        }
        break;

    case PWR_CMD_GET_STATUS:
        /* 通过AT命令获取状态 */
        at_send_cmd("AT+CPAS", NULL, NULL, 1000);
        ret = 0;
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

/* 默认电源控制回调 */
static int default_pwr_callback(at_device_t *dev, pwr_cmd_t cmd, void *param)
{
    at_power_private_t *priv = (at_power_private_t *)dev->user_data;
    if (!priv) {
        return -1;
    }

    /* 模拟GPIO控制，实际应用需要根据硬件实现 */
    switch (cmd) {
    case PWR_CMD_POWER_ON:
        /* PWR_KEY引脚拉低一段时间 */
        printf("PWR_KEY: LOW for %d ms\n", priv->config.timing.power_on_time);
        // GPIO_SetLow(priv->config.gpio.pwr_key_pin);
        // delay_ms(priv->config.timing.power_on_time);
        // GPIO_SetHigh(priv->config.gpio.pwr_key_pin);
        break;

    case PWR_CMD_POWER_OFF:
        /* PWR_KEY引脚拉低一段时间 */
        printf("PWR_KEY: LOW for %d ms (power off)\n",
               priv->config.timing.power_on_time);
        // GPIO_SetLow(priv->config.gpio.pwr_key_pin);
        // delay_ms(priv->config.timing.power_on_time);
        // GPIO_SetHigh(priv->config.gpio.pwr_key_pin);
        break;

    case PWR_CMD_RESET:
        /* RESET引脚拉低一段时间 */
        printf("RESET: LOW for %d ms\n", priv->config.timing.reset_time);
        // GPIO_SetLow(priv->config.gpio.reset_pin);
        // delay_ms(priv->config.timing.reset_time);
        // GPIO_SetHigh(priv->config.gpio.reset_pin);
        break;

    case PWR_CMD_WAKEUP:
        /* WAKEUP引脚触发 */
        printf("WAKEUP: Trigger\n");
        // GPIO_Pulse(priv->config.gpio.wakeup_pin,
        // priv->config.timing.wakeup_time);
        break;

    default:
        return -1;
    }

    return 0;
}

/* 硬件复位设备 */
int at_power_hard_reset(at_device_t *dev)
{
    return at_power_control(dev, PWR_CMD_RESET, NULL);
}

/* 软件复位设备 */
int at_power_soft_reset(at_device_t *dev)
{
    /* 发送AT命令复位 */
    at_resp_type_t resp = at_send_cmd("AT+CRESET", NULL, NULL, 10000);
    if (resp == AT_RESP_OK) {
        /* 更新状态 */
        at_power_private_t *priv = (at_power_private_t *)dev->user_data;
        if (priv) {
            priv->config.current_state = PWR_STATE_OFF;
            priv->config.target_state  = PWR_STATE_ON;
            priv->power_cycle_count++;
        }
        return 0;
    }
    return -1;
}

/* 更新电源管理状态 */
int at_power_update(at_device_t *dev)
{
    at_power_private_t *priv = (at_power_private_t *)dev->user_data;
    if (!priv || !priv->config.auto_power_manage) {
        return 0;
    }

    uint32_t current_tick = dev->get_tick();
    uint32_t idle_time    = current_tick - priv->last_activity_time;

    /* 检查是否需要进入睡眠 */
    if (priv->config.current_state == PWR_STATE_ON
        && idle_time > priv->config.power_config.sleep_timeout) {

        printf("Device idle for %d ms, entering sleep mode\n", idle_time);
        at_power_control(dev, PWR_CMD_SLEEP, NULL);
    }

    /* 检查是否需要唤醒 */
    if ((priv->config.current_state == PWR_STATE_SLEEP
         || priv->config.current_state == PWR_STATE_DEEP_SLEEP)
        && current_tick - priv->sleep_start_time
               > priv->config.power_config.wakeup_interval) {

        printf("Wakeup interval reached, waking up device\n");
        at_power_control(dev, PWR_CMD_WAKEUP, NULL);
    }

    return 0;
}

/* 获取功耗统计 */
int at_power_get_statistics(at_device_t *dev, uint32_t *total_on_time,
                            uint32_t *total_sleep_time, uint32_t *power_cycles)
{
    at_power_private_t *priv = (at_power_private_t *)dev->user_data;
    if (!priv) {
        return -1;
    }

    uint32_t current_tick = dev->get_tick();

    if (total_on_time) {
        *total_on_time = 0; /* 这里需要累计历史数据 */
        if (priv->config.current_state == PWR_STATE_ON
            && priv->on_start_time > 0) {
            *total_on_time += (current_tick - priv->on_start_time);
        }
    }

    if (total_sleep_time) {
        *total_sleep_time = 0; /* 这里需要累计历史数据 */
        if (priv->config.current_state == PWR_STATE_SLEEP
            && priv->sleep_start_time > 0) {
            *total_sleep_time += (current_tick - priv->sleep_start_time);
        }
    }

    if (power_cycles) {
        *power_cycles = priv->power_cycle_count;
    }

    return 0;
}


#include "at_4g.h"
#include "at_power.h"

/* 4G模块电源管理扩展 */
typedef struct {
    /* 模块专用电源状态 */
    struct {
        bool charging;         /* 是否在充电 */
        uint16_t voltage;      /* 电压(mV) */
        uint8_t battery_level; /* 电池电量百分比 */
        int8_t temperature;    /* 温度(℃) */
    } power;

    /* 射频功耗控制 */
    struct {
        uint8_t tx_power;       /* 发射功率等级 */
        uint8_t rx_sensitivity; /* 接收灵敏度 */
        bool antenna_present;   /* 天线是否连接 */
    } rf;

    /* 网络功耗优化 */
    struct {
        uint32_t paging_cycle; /* 寻呼周期 */
        uint8_t drx_cycle;     /* 不连续接收周期 */
        bool cdrx_enabled;     /* 是否启用CDRX */
    } network;
} at_4g_power_private_t;

/* 4G模块电源控制函数 */

/**
 * @brief 4G模块电源初始化
 */
int at_4g_power_init(at_device_t *dev)
{
    /* 创建4G电源私有数据 */
    at_4g_power_private_t *power_priv =
        (at_4g_power_private_t *)malloc(sizeof(at_4g_power_private_t));
    if (!power_priv) {
        return -1;
    }

    memset(power_priv, 0, sizeof(at_4g_power_private_t));

    /* 设置默认值 */
    power_priv->power.battery_level = 100;
    power_priv->rf.tx_power         = 5; /* 中等功率 */
    power_priv->network.drx_cycle   = 1; /* 默认DRX周期 */

    /* 将私有数据附加到设备 */
    // 这里需要整合到设备的用户数据中

    return 0;
}

/**
 * @brief 设置4G模块发射功率
 */
int at_4g_set_tx_power(at_device_t *dev, uint8_t power_level)
{
    char cmd[32];

    /* 不同模块有不同的命令 */
    at_4g_private_t *priv = get_4g_private(dev);
    if (!priv) {
        return -1;
    }

    if (priv->type == MODULE_SIM7600) {
        snprintf(cmd, sizeof(cmd), "AT+CRFTX=%d", power_level);
    } else if (priv->type == MODULE_EC200) {
        snprintf(cmd, sizeof(cmd), "AT+QSCLK=%d", power_level);
    } else {
        return -1;
    }

    at_resp_type_t resp = at_send_cmd(cmd, NULL, NULL, 2000);
    if (resp == AT_RESP_OK) {
        /* 更新私有数据 */
        at_4g_power_private_t *power_priv = get_4g_power_private(dev);
        if (power_priv) {
            power_priv->rf.tx_power = power_level;
        }
        return 0;
    }

    return -1;
}

/**
 * @brief 获取模块电压和温度
 */
int at_4g_get_power_info(at_device_t *dev, uint16_t *voltage,
                         int8_t *temperature, uint8_t *battery_level)
{
    char response[128];

    /* 发送命令获取电源信息 */
    at_resp_type_t resp = at_send_cmd("AT+CBC", NULL, response, 2000);
    if (resp == AT_RESP_OK) {
        /* 解析响应: +CBC: <bcs>,<bcl>,<voltage> */
        int bcs, bcl, volt;
        if (sscanf(response, "+CBC: %d,%d,%d", &bcs, &bcl, &volt) == 3) {
            if (voltage)
                *voltage = volt;
            if (battery_level)
                *battery_level = bcl;
        }
    }

    /* 获取温度 */
    resp = at_send_cmd("AT+CMUTEMP", NULL, response, 2000);
    if (resp == AT_RESP_OK) {
        int temp;
        if (sscanf(response, "+CMUTEMP: %d", &temp) == 1) {
            if (temperature)
                *temperature = temp;
        }
    }

    return 0;
}

/**
 * @brief 设置DRX（不连续接收）参数
 */
int at_4g_set_drx(at_device_t *dev, uint8_t drx_cycle, bool enable)
{
    char cmd[32];

    if (enable) {
        snprintf(cmd, sizeof(cmd), "AT+EDRX=%d", drx_cycle);
    } else {
        strcpy(cmd, "AT+EDRX=0");
    }

    at_resp_type_t resp = at_send_cmd(cmd, NULL, NULL, 2000);
    if (resp == AT_RESP_OK) {
        /* 更新私有数据 */
        at_4g_power_private_t *power_priv = get_4g_power_private(dev);
        if (power_priv) {
            power_priv->network.drx_cycle    = drx_cycle;
            power_priv->network.cdrx_enabled = enable;
        }
        return 0;
    }

    return -1;
}

/**
 * @brief 设置PSM（省电模式）参数
 */
int at_4g_set_psm(at_device_t *dev, uint32_t active_time, uint32_t periodic_tau)
{
    char cmd[64];

    /* 设置PSM参数 */
    snprintf(cmd, sizeof(cmd), "AT+CPSMS=1,,,\"%08X\",\"%08X\"", active_time,
             periodic_tau);

    at_resp_type_t resp = at_send_cmd(cmd, NULL, NULL, 2000);
    return (resp == AT_RESP_OK) ? 0 : -1;
}

/**
 * @brief 智能电源管理：根据信号强度调整功耗
 */
int at_4g_smart_power_manage(at_device_t *dev)
{
    signal_info_t signal;

    if (at_4g_get_signal(dev, &signal) != 0) {
        return -1;
    }

    /* 根据信号强度调整发射功率 */
    uint8_t tx_power_level;
    if (signal.rssi >= 20) {        /* 信号强 */
        tx_power_level = 1;         /* 低功率 */
    } else if (signal.rssi >= 10) { /* 信号中等 */
        tx_power_level = 5;         /* 中功率 */
    } else {                        /* 信号弱 */
        tx_power_level = 9;         /* 高功率 */
    }

    at_4g_set_tx_power(dev, tx_power_level);

    /* 根据信号质量调整DRX */
    if (signal.rssi < 5) {            /* 信号很差 */
        at_4g_set_drx(dev, 0, false); /* 关闭DRX，连续接收 */
    } else {
        at_4g_set_drx(dev, 1, true); /* 启用DRX */
    }

    return 0;
}

/**
 * @brief 深度睡眠模式（保持网络注册）
 */
int at_4g_deep_sleep(at_device_t *dev)
{
    /* 1. 保存网络上下文 */
    at_send_cmd("AT+CSCON=0", NULL, NULL, 1000); /* 禁用网络状态URC */

    /* 2. 进入深度睡眠 */
    at_send_cmd("AT+CSCLK=2", NULL, NULL, 1000);

    /* 3. 调用硬件睡眠回调 */
    at_power_control(dev, PWR_CMD_SLEEP, NULL);

    return 0;
}

/**
 * @brief 定时唤醒模式
 */
int at_4g_scheduled_wakeup(at_device_t *dev, uint32_t interval_ms)
{
    char cmd[64];

    /* 设置自动唤醒间隔 */
    snprintf(cmd, sizeof(cmd), "AT+WAKETIM=%lu", interval_ms / 1000);
    at_send_cmd(cmd, NULL, NULL, 1000);

    /* 启用定时唤醒 */
    at_send_cmd("AT+CSCLK=1", NULL, NULL, 1000);

    return 0;
}