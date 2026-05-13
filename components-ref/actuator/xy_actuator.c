/**
 * SPDX-License-Identifier: MIT
 * @file    xy_actuator.c
 * @brief   Actuator Framework Implementation
 * @version 1.0.0
 *
 * 执行器框架实现 - 继电器、舵机等
 */

#include "xy_actuator.h"
#include <stdio.h>
#include <string.h>

/* ==================== 静态变量 ==================== */
#define MAX_ACTUATORS  32

static actuator_device_t *g_actuators[MAX_ACTUATORS];
static uint8_t g_actuator_count = 0;

/* ==================== 工具函数 ==================== */
const char *actuator_type_str(actuator_type_t type)
{
    switch (type) {
        case ACTUATOR_TYPE_RELAY:       return "Relay";
        case ACTUATOR_TYPE_SERVO:        return "Servo";
        case ACTUATOR_TYPE_MOTOR_DC:     return "DC Motor";
        case ACTUATOR_TYPE_MOTOR_STEPPER: return "Stepper Motor";
        case ACTUATOR_TYPE_SOLENOID:     return "Solenoid";
        case ACTUATOR_TYPE_PWM:          return "PWM";
        case ACTUATOR_TYPE_LED:          return "LED";
        case ACTUATOR_TYPE_BUZZER:       return "Buzzer";
        case ACTUATOR_TYPE_VALVE:        return "Valve";
        default:                         return "Unknown";
    }
}

const char *actuator_status_str(actuator_status_t status)
{
    switch (status) {
        case ACTUATOR_STATUS_IDLE:     return "Idle";
        case ACTUATOR_STATUS_READY:    return "Ready";
        case ACTUATOR_STATUS_BUSY:     return "Busy";
        case ACTUATOR_STATUS_ERROR:    return "Error";
        case ACTUATOR_STATUS_DISABLED: return "Disabled";
        default:                        return "Unknown";
    }
}

const char *actuator_err_str(actuator_err_t err)
{
    switch (err) {
        case ACTUATOR_EOK:       return "OK";
        case ACTUATOR_ERROR:     return "Error";
        case ACTUATOR_EINVAL:    return "Invalid argument";
        case ACTUATOR_ENODEV:    return "Device not found";
        case ACTUATOR_EBUSY:     return "Device busy";
        case ACTUATOR_ETIMEOUT:  return "Timeout";
        case ACTUATOR_ENOMEM:    return "Out of memory";
        case ACTUATOR_ENOSYS:    return "Not implemented";
        case ACTUATOR_EIO:       return "I/O error";
        case ACTUATOR_ELIMIT:    return "Limit exceeded";
        case ACTUATOR_EHW:       return "Hardware error";
        default:                 return "Unknown";
    }
}

/* ==================== 设备管理 ==================== */
actuator_err_t actuator_register(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    if (g_actuator_count >= MAX_ACTUATORS) {
        return ACTUATOR_ENOMEM;
    }

    /* 检查重名 */
    for (int i = 0; i < g_actuator_count; i++) {
        if (strcmp(g_actuators[i]->name, dev->name) == 0) {
            return ACTUATOR_EINVAL;
        }
    }

    dev->id = g_actuator_count;
    g_actuators[g_actuator_count++] = dev;
    dev->status = ACTUATOR_STATUS_READY;

    return ACTUATOR_EOK;
}

actuator_err_t actuator_unregister(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i] == dev) {
            for (int j = i; j < g_actuator_count - 1; j++) {
                g_actuators[j] = g_actuators[j + 1];
            }
            g_actuator_count--;
            dev->status = ACTUATOR_STATUS_IDLE;
            return ACTUATOR_EOK;
        }
    }

    return ACTUATOR_ENODEV;
}

actuator_device_t *actuator_find(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; i < g_actuator_count; i++) {
        if (strcmp(g_actuators[i]->name, name) == 0) {
            return g_actuators[i];
        }
    }

    return NULL;
}

actuator_device_t *actuator_find_by_type(actuator_type_t type)
{
    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i]->type == type) {
            return g_actuators[i];
        }
    }
    return NULL;
}

uint8_t actuator_get_count(void)
{
    return g_actuator_count;
}

uint8_t actuator_get_count_by_type(actuator_type_t type)
{
    uint8_t count = 0;
    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i]->type == type) {
            count++;
        }
    }
    return count;
}

/* ==================== 基础 API ==================== */
actuator_err_t actuator_init(actuator_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->init == NULL) {
        return ACTUATOR_EINVAL;
    }

    actuator_err_t err = dev->ops->init(dev);
    if (err == ACTUATOR_EOK) {
        dev->status = ACTUATOR_STATUS_READY;
    }

    return err;
}

actuator_err_t actuator_deinit(actuator_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->deinit == NULL) {
        return ACTUATOR_EINVAL;
    }

    actuator_err_t err = dev->ops->deinit(dev);
    if (err == ACTUATOR_EOK) {
        dev->status = ACTUATOR_STATUS_IDLE;
    }

    return err;
}

actuator_err_t actuator_enable(actuator_device_t *dev, bool enable)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->enable == NULL) {
        return ACTUATOR_EINVAL;
    }

    return dev->ops->enable(dev, enable);
}

actuator_err_t actuator_write(actuator_device_t *dev, const actuator_value_t *value)
{
    if (dev == NULL || value == NULL) {
        return ACTUATOR_EINVAL;
    }

    if (dev->ops == NULL || dev->ops->write == NULL) {
        return ACTUATOR_ENOSYS;
    }

    dev->value = *value;
    dev->status = ACTUATOR_STATUS_BUSY;

    actuator_err_t err = dev->ops->write(dev, value);

    if (err == ACTUATOR_EOK) {
        dev->status = ACTUATOR_STATUS_READY;
    } else {
        dev->status = ACTUATOR_STATUS_ERROR;
    }

    return err;
}

actuator_err_t actuator_read(actuator_device_t *dev, actuator_value_t *value)
{
    if (dev == NULL || value == NULL) {
        return ACTUATOR_EINVAL;
    }

    if (dev->ops == NULL || dev->ops->read == NULL) {
        return ACTUATOR_ENOSYS;
    }

    return dev->ops->read(dev, value);
}

actuator_err_t actuator_reset(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    if (dev->ops == NULL || dev->ops->reset == NULL) {
        /* 默认复位行为: 关闭输出 */
        if (dev->type == ACTUATOR_TYPE_RELAY) {
            return relay_off(dev);
        } else if (dev->type == ACTUATOR_TYPE_SERVO) {
            return servo_center(dev);
        }
        return ACTUATOR_ENOSYS;
    }

    return dev->ops->reset(dev);
}

actuator_err_t actuator_emergency_stop(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    if (dev->ops == NULL || dev->ops->emergency_stop == NULL) {
        /* 默认急停: 关闭输出 */
        if (dev->type == ACTUATOR_TYPE_RELAY) {
            return relay_off(dev);
        } else if (dev->type == ACTUATOR_TYPE_SERVO) {
            /* 舵机急停保持当前位置 */
            return ACTUATOR_EOK;
        }
        return ACTUATOR_ENOSYS;
    }

    return dev->ops->emergency_stop(dev);
}

actuator_status_t actuator_get_status(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_STATUS_ERROR;
    }

    if (dev->ops && dev->ops->get_status) {
        return dev->ops->get_status(dev);
    }

    return dev->status;
}

bool actuator_is_ready(actuator_device_t *dev)
{
    return (dev != NULL && dev->status == ACTUATOR_STATUS_READY);
}

/* ==================== 继电器 API ==================== */
actuator_err_t relay_init(actuator_device_t *dev)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    /* 初始化 GPIO */
    /* 具体实现依赖 HAL */

    /* 默认关闭继电器 */
    dev->value.relay.state = 0;

    return ACTUATOR_EOK;
}

actuator_err_t relay_set(actuator_device_t *dev, uint8_t state)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_RELAY) {
        return ACTUATOR_EINVAL;
    }

    /* 设置 GPIO 电平 */
    bool active_high = dev->config.active_high;
    bool gpio_level = (state != 0) ? active_high : !active_high;

    /* 实际实现: HAL_GPIO_WritePin(dev->config.gpio_port, dev->config.gpio_pin, gpio_level); */

    dev->value.relay.state = state;

    return ACTUATOR_EOK;
}

actuator_err_t relay_on(actuator_device_t *dev)
{
    return relay_set(dev, 1);
}

actuator_err_t relay_off(actuator_device_t *dev)
{
    return relay_set(dev, 0);
}

actuator_err_t relay_toggle(actuator_device_t *dev)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_RELAY) {
        return ACTUATOR_EINVAL;
    }

    return relay_set(dev, !dev->value.relay.state);
}

actuator_err_t relay_get(actuator_device_t *dev, uint8_t *state)
{
    if (dev == NULL || state == NULL || dev->type != ACTUATOR_TYPE_RELAY) {
        return ACTUATOR_EINVAL;
    }

    *state = dev->value.relay.state;

    /* 实际实现可读回 GPIO 状态 */
    return ACTUATOR_EOK;
}

actuator_err_t relay_pulse(actuator_device_t *dev, uint32_t pulse_width_ms)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_RELAY) {
        return ACTUATOR_EINVAL;
    }

    /* 接通 */
    relay_on(dev);

    /* 延时 */
    /* 实际实现使用 OS 延时或硬件定时器 */
    /* os_delay_ms(pulse_width_ms); */

    /* 断开 */
    relay_off(dev);

    return ACTUATOR_EOK;
}

/* ==================== 舵机 API ==================== */
actuator_err_t servo_init(actuator_device_t *dev)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    /* 初始化 PWM */
    /* 具体实现依赖 HAL PWM */

    /* 默认居中 */
    dev->value.servo.target_angle = 0.0f;
    dev->value.servo.current_angle = 0.0f;

    return ACTUATOR_EOK;
}

actuator_err_t servo_set_angle(actuator_device_t *dev, float angle)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    /* 限制角度范围 */
    if (angle < dev->config.servo_min_angle) {
        angle = dev->config.servo_min_angle;
    }
    if (angle > dev->config.servo_max_angle) {
        angle = dev->config.servo_max_angle;
    }

    /* 转换为 PWM 占空比 */
    uint16_t pwm_duty = servo_angle_to_pwm(
        angle,
        dev->config.servo_min_angle,
        dev->config.servo_max_angle,
        dev->config.servo_pwm_min,
        dev->config.servo_pwm_max
    );

    /* 设置 PWM */
    /* 实际实现: HAL_PWM_SetDuty(dev->config.pwm_channel, pwm_duty); */

    dev->value.servo.target_angle = angle;

    return ACTUATOR_EOK;
}

actuator_err_t servo_get_angle(actuator_device_t *dev, float *angle)
{
    if (dev == NULL || angle == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    *angle = dev->value.servo.current_angle;

    return ACTUATOR_EOK;
}

actuator_err_t servo_set_range(actuator_device_t *dev, float min_angle, float max_angle)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    if (min_angle >= max_angle) {
        return ACTUATOR_EINVAL;
    }

    dev->config.servo_min_angle = min_angle;
    dev->config.servo_max_angle = max_angle;

    return ACTUATOR_EOK;
}

actuator_err_t servo_set_speed(actuator_device_t *dev, uint32_t speed)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    dev->config.servo_speed = speed;

    return ACTUATOR_EOK;
}

actuator_err_t servo_sweep(actuator_device_t *dev, float start, float end, uint32_t step_ms)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    if (start == end) {
        return ACTUATOR_EINVAL;
    }

    float direction = (end > start) ? 1.0f : -1.0f;
    uint32_t step_count = (uint32_t)fabsf((end - start) / 1.0f);  // 1度每步

    for (uint32_t i = 0; i < step_count; i++) {
        float current = start + direction * i;
        servo_set_angle(dev, current);

        /* 延时 */
        /* os_delay_ms(step_ms); */
    }

    /* 最终位置 */
    servo_set_angle(dev, end);

    return ACTUATOR_EOK;
}

actuator_err_t servo_stop(actuator_device_t *dev)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    /* 停止 PWM 输出 */
    /* 实际实现: HAL_PWM_Stop(dev->config.pwm_channel); */

    return ACTUATOR_EOK;
}

actuator_err_t servo_center(actuator_device_t *dev)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_SERVO) {
        return ACTUATOR_EINVAL;
    }

    /* 设置到 0 度 (居中) */
    return servo_set_angle(dev, 0.0f);
}

/* ==================== PWM API ==================== */
actuator_err_t pwm_set_duty(actuator_device_t *dev, uint16_t duty)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_PWM) {
        return ACTUATOR_EINVAL;
    }

    /* 实际实现: HAL_PWM_SetDuty(dev->config.pwm_channel, duty); */

    dev->value.pwm.duty = duty;

    return ACTUATOR_EOK;
}

actuator_err_t pwm_set_frequency(actuator_device_t *dev, uint32_t freq)
{
    if (dev == NULL || dev->type != ACTUATOR_TYPE_PWM) {
        return ACTUATOR_EINVAL;
    }

    dev->config.pwm_freq = freq;

    /* 实际实现: HAL_PWM_SetFrequency(dev->config.pwm_channel, freq); */

    return ACTUATOR_EOK;
}

/* ==================== 批量操作 ==================== */
actuator_err_t actuator_all_off(void)
{
    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i]->type == ACTUATOR_TYPE_RELAY) {
            relay_off(g_actuators[i]);
        } else if (g_actuators[i]->type == ACTUATOR_TYPE_PWM) {
            pwm_set_duty(g_actuators[i], 0);
        }
    }

    return ACTUATOR_EOK;
}

actuator_err_t actuator_emergency_stop_all(void)
{
    for (int i = 0; i < g_actuator_count; i++) {
        actuator_emergency_stop(g_actuators[i]);
    }

    return ACTUATOR_EOK;
}

/* ==================== 默认操作实现 ==================== */
static actuator_err_t default_init(actuator_device_t *dev)
{
    (void)dev;
    return ACTUATOR_EOK;
}

static actuator_err_t default_deinit(actuator_device_t *dev)
{
    (void)dev;
    return ACTUATOR_EOK;
}

static actuator_err_t default_write(actuator_device_t *dev, const actuator_value_t *value)
{
    (void)dev;
    (void)value;
    return ACTUATOR_EOK;
}

static actuator_err_t default_enable(actuator_device_t *dev, bool enable)
{
    if (dev == NULL) {
        return ACTUATOR_EINVAL;
    }

    dev->status = enable ? ACTUATOR_STATUS_READY : ACTUATOR_STATUS_DISABLED;
    return ACTUATOR_EOK;
}

/* ==================== 默认操作表 ==================== */
const actuator_ops_t relay_default_ops = {
    .init = default_init,
    .deinit = default_deinit,
    .write = (actuator_err_t (*)(actuator_device_t *, const actuator_value_t *))relay_set,
    .read = NULL,
    .enable = default_enable,
    .config = NULL,
    .get_config = NULL,
    .get_status = NULL,
    .is_ready = NULL,
    .sleep = NULL,
    .wakeup = NULL,
    .reset = (actuator_err_t (*)(actuator_device_t *))relay_off,
    .emergency_stop = (actuator_err_t (*)(actuator_device_t *))relay_off,
};

const actuator_ops_t servo_default_ops = {
    .init = default_init,
    .deinit = default_deinit,
    .write = (actuator_err_t (*)(actuator_device_t *, const actuator_value_t *))servo_set_angle,
    .read = (actuator_err_t (*)(actuator_device_t *, actuator_value_t *))servo_get_angle,
    .enable = default_enable,
    .config = NULL,
    .get_config = NULL,
    .get_status = NULL,
    .is_ready = NULL,
    .sleep = NULL,
    .wakeup = NULL,
    .reset = (actuator_err_t (*)(actuator_device_t *))servo_center,
    .emergency_stop = NULL,  // 舵机急停保持位置
};

/* ==================== 示例代码 ==================== */
/*
 * 使用示例:
 *
 * // 1. 定义继电器设备
 * actuator_device_t relay1 = {
 *     .name = "relay_1",
 *     .type = ACTUATOR_TYPE_RELAY,
 *     .ops = &relay_default_ops,
 *     .config.gpio_pin = 5,
 *     .config.gpio_port = 0,
 *     .config.active_high = true,
 * };
 *
 * // 2. 注册
 * actuator_register(&relay1);
 *
 * // 3. 控制
 * relay_on(&relay1);
 * os_delay_ms(500);
 * relay_off(&relay1);
 *
 * // 4. 定义舵机设备
 * actuator_device_t servo1 = {
 *     .name = "servo_1",
 *     .type = ACTUATOR_TYPE_SERVO,
 *     .ops = &servo_default_ops,
 *     .config.pwm_channel = 2,
 *     .config.servo_min_angle = -90.0f,
 *     .config.servo_max_angle = 90.0f,
 *     .config.servo_speed = 100,  // 100度/秒
 * };
 *
 * // 5. 注册并控制
 * actuator_register(&servo1);
 * servo_set_angle(&servo1, 45.0f);
 * servo_sweep(&servo1, -90.0f, 90.0f, 50);
 *
 * // 6. 急停所有
 * actuator_emergency_stop_all();
 */