#include "at_client.h"
#include "at_power.h"
#include "at_4g.h"

/* 实际的硬件控制函数 */
static int hardware_pwr_ctrl(at_device_t *dev, pwr_cmd_t cmd, void *param)
{
    at_power_private_t *priv = (at_power_private_t *)dev->user_data;

    switch (cmd) {
    case PWR_CMD_POWER_ON:
        /* 控制PWR_KEY引脚拉低2秒 */
        printf("Power ON: Pull PWR_KEY low for 2s\n");
        // HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);
        // HAL_Delay(2000);
        // HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_SET);

        /* 等待模块启动 */
        HAL_Delay(5000);
        break;

    case PWR_CMD_POWER_OFF:
        /* 关机：PWR_KEY拉低2秒 */
        printf("Power OFF: Pull PWR_KEY low for 2s\n");
        // HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);
        // HAL_Delay(2000);
        // HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_SET);
        break;

    case PWR_CMD_RESET:
        /* 复位：RESET引脚拉低1秒 */
        printf("Reset: Pull RESET low for 1s\n");
        // HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_RESET);
        // HAL_Delay(1000);
        // HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_SET);

        /* 等待模块重启 */
        HAL_Delay(10000);
        break;

    case PWR_CMD_WAKEUP:
        /* 唤醒：WAKEUP引脚脉冲100ms */
        printf("Wakeup: Pulse WAKEUP pin\n");
        // HAL_GPIO_WritePin(WAKEUP_GPIO_Port, WAKEUP_Pin, GPIO_PIN_SET);
        // HAL_Delay(100);
        // HAL_GPIO_WritePin(WAKEUP_GPIO_Port, WAKEUP_Pin, GPIO_PIN_RESET);
        break;

    default:
        return -1;
    }

    return 0;
}

/* 电源管理状态机 */
void power_management_state_machine(at_device_t *dev)
{
    static enum {
        STATE_INIT,
        STATE_POWER_ON,
        STATE_NETWORK_REG,
        STATE_DATA_TRANSFER,
        STATE_IDLE,
        STATE_SLEEP,
        STATE_POWER_OFF
    } current_state = STATE_INIT;

    at_power_private_t *priv = (at_power_private_t *)dev->user_data;
    uint32_t current_tick    = dev->get_tick();

    switch (current_state) {
    case STATE_INIT:
        /* 开机 */
        at_power_control(dev, PWR_CMD_POWER_ON, NULL);
        current_state = STATE_POWER_ON;
        break;

    case STATE_POWER_ON:
        /* 检查模块是否就绪 */
        if (current_tick - priv->on_start_time
            > priv->config.timing.boot_delay) {
            /* 发送AT命令测试 */
            at_resp_type_t resp = at_send_cmd("AT", NULL, NULL, 1000);
            if (resp == AT_RESP_OK) {
                printf("Module ready\n");
                current_state = STATE_NETWORK_REG;
            }
        }
        break;

    case STATE_NETWORK_REG:
        /* 检查网络注册 */
        network_info_t netinfo;
        if (at_4g_get_network_status(dev, &netinfo) == 0) {
            if (netinfo.status >= NET_REGISTERED_HOME) {
                printf("Network registered\n");
                current_state = STATE_DATA_TRANSFER;
            }
        }
        break;

    case STATE_DATA_TRANSFER:
        /* 数据传输状态 */
        if (current_tick - priv->last_activity_time > 60000) { /* 空闲60秒 */
            printf("Entering idle state\n");
            current_state = STATE_IDLE;
        }
        break;

    case STATE_IDLE:
        /* 空闲状态，准备进入睡眠 */
        if (current_tick - priv->last_activity_time > 300000) { /* 空闲5分钟 */
            printf("Entering sleep state\n");
            at_power_control(dev, PWR_CMD_SLEEP, NULL);
            current_state = STATE_SLEEP;
        }
        break;

    case STATE_SLEEP:
        /* 睡眠状态，定时唤醒 */
        if (current_tick - priv->sleep_start_time > 300000) { /* 睡眠5分钟 */
            printf("Waking up\n");
            at_power_control(dev, PWR_CMD_WAKEUP, NULL);
            current_state = STATE_NETWORK_REG;
        }
        break;

    case STATE_POWER_OFF:
        /* 关机状态 */
        break;
    }
}

/* 完整的4G模块电源管理示例 */
void at_power_complete_example(void)
{
    at_device_t *dev;

    /* 1. 创建设备 */
    dev = at_device_register("4G_POWER_DEMO", uart_read_byte, uart_write_data,
                             get_system_tick, NULL);

    /* 2. 配置电源管理 */
    at_power_config_t power_config = {
        .gpio              = { .pwr_key_pin = 10,
                               .reset_pin   = 11,
                               .wakeup_pin  = 12,
                               .status_pin  = 13 },
        .timing            = { .power_on_time = 2000,
                               .reset_time    = 1000,
                               .wakeup_time   = 100,
                               .boot_delay    = 5000 },
        .ctrl_callback     = hardware_pwr_ctrl,
        .callback_param    = NULL,
        .auto_power_manage = true,
        .power_config = { .sleep_timeout   = 60000, /* 1分钟后进入睡眠 */
                          .wakeup_interval = 300000, /* 5分钟后唤醒 */
                          .power_level     = 3 }
    };

    /* 3. 初始化电源管理 */
    at_power_init(dev, &power_config);

    /* 4. 初始化4G模块 */
    at_4g_init(dev, MODULE_UNKNOWN);

    /* 5. 主循环 */
    while (1) {
        /* 更新电源管理状态 */
        at_power_update(dev);

        /* 运行电源状态机 */
        power_management_state_machine(dev);

        /* 处理URC */
        at_urc_poll(dev);

        /* 记录活动时间 */
        static uint32_t last_report = 0;
        uint32_t current_tick       = get_system_tick();
        if (current_tick - last_report > 60000) {
            /* 每分钟报告一次状态 */
            pwr_state_t state = at_power_get_status(dev);
            printf("Current power state: %d\n", state);

            /* 获取功耗统计 */
            uint32_t on_time, sleep_time, cycles;
            at_power_get_statistics(dev, &on_time, &sleep_time, &cycles);
            printf("On: %lu ms, Sleep: %lu ms, Cycles: %lu\n", on_time,
                   sleep_time, cycles);

            last_report = current_tick;
        }

        HAL_Delay(100);
    }
}

/* NB-IoT省电模式示例 */
void nbiot_power_saving_example(at_device_t *dev)
{
    /* 1. 设置PSM（省电模式）参数 */
    /* T3412（周期性TAU）：2小时，T3324（Active Timer）：20秒 */
    at_4g_set_psm(dev, 0x1B, 0x02418001); /* 十六进制表示时间 */

    /* 2. 设置eDRX参数 */
    at_4g_set_drx(dev, 5, true); /* 5.12秒eDRX周期 */

    /* 3. 设置低功耗APN */
    at_4g_activate_pdp(dev, "nbiot", "", "");

    /* 4. 降低发射功率 */
    at_4g_set_tx_power(dev, 1); /* 最低功率 */

    /* 5. 启用智能电源管理 */
    at_4g_smart_power_manage(dev);

    /* 6. 进入深度睡眠 */
    at_4g_deep_sleep(dev);

    printf("NB-IoT module in ultra-low power mode\n");
}

/* 紧急情况处理 */
void emergency_power_management(at_device_t *dev)
{
    /* 获取电池电量 */
    uint16_t voltage;
    uint8_t battery_level;
    at_4g_get_power_info(dev, &voltage, NULL, &battery_level);

    if (battery_level < 10) { /* 电量低于10% */
        printf("Low battery: %d%%, entering emergency mode\n", battery_level);

        /* 1. 关闭所有不必要的功能 */
        at_send_cmd("AT+CFUN=0", NULL, NULL, 1000); /* 最小功能模式 */

        /* 2. 关闭射频 */
        at_send_cmd("AT+CFUN=4", NULL, NULL, 1000); /* 飞行模式 */

        /* 3. 降低工作频率 */
        at_send_cmd("AT+CSCLK=1", NULL, NULL, 1000); /* 低速时钟 */

        /* 4. 发送最后的报警信息 */
        at_4g_send_sms(dev, "13800138000",
                       "Low battery alert! Device will shutdown soon.");

        /* 5. 进入深度睡眠等待充电 */
        at_power_control(dev, PWR_CMD_SLEEP, NULL);
    } else if (voltage < 3500) { /* 电压低于3.5V */
        printf("Low voltage: %dmV, forcing shutdown\n", voltage);

        /* 强制关机 */
        at_power_control(dev, PWR_CMD_POWER_OFF, NULL);
    }
}

/* 太阳能供电系统电源管理 */
void solar_power_management(at_device_t *dev)
{
    /* 获取太阳能电池电压 */
    uint16_t solar_voltage = read_solar_voltage();
    uint16_t battery_voltage;
    at_4g_get_power_info(dev, &battery_voltage, NULL, NULL);

    /* 根据光照条件调整工作模式 */
    if (solar_voltage > 5000) { /* 强光照，可以全功率工作 */
        printf("Strong sunlight, full power mode\n");
        at_power_control(dev, PWR_CMD_FULL_POWER, NULL);

    } else if (solar_voltage > 3000) { /* 中等光照，正常模式 */
        printf("Moderate sunlight, normal mode\n");
        /* 启用智能电源管理 */
        at_4g_smart_power_manage(dev);

    } else { /* 弱光照或夜间，省电模式 */
        printf("Low light, power saving mode\n");

        /* 延长睡眠时间 */
        at_4g_set_psm(dev, 0x0A, 0x02418001); /* 更长的睡眠周期 */

        /* 降低数据传输频率 */
        at_send_cmd("AT+CREG=0", NULL, NULL, 1000); /* 禁用网络状态报告 */

        /* 进入深度睡眠，定时唤醒 */
        at_4g_scheduled_wakeup(dev, 3600000); /* 每小时唤醒一次 */
    }

    /* 充电管理 */
    if (solar_voltage > battery_voltage + 500) { /* 太阳能可以充电 */
        enable_charging(true);
    } else {
        enable_charging(false);
    }
}