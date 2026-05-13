/**
 * @file xy_pid_temp_ctrl.c
 * @brief PID Temperature Control for Charging System
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_log.h"
#include <stdint.h>
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* PID 控制器结构 */
typedef struct {
    float Kp;           /* 比例系数 */
    float Ki;           /* 积分系数 */
    float Kd;           /* 微分系数 */
    float setpoint;     /* 目标值 */
    float integral;     /* 积分累积 */
    float prev_error;   /* 上次误差 */
    float output_min;   /* 输出最小值 */
    float output_max;   /* 输出最大值 */
    float integral_min; /* 积分最小值 (抗饱和) */
    float integral_max; /* 积分最大值 (抗饱和) */
} xy_pid_ctrl_t;

/* 温控系统状态 */
typedef struct {
    bool initialized;
    bool enabled;
    xy_pid_ctrl_t pid;
    
    /* 温度参数 */
    int32_t current_temp;      /* 当前温度 (°C*100) */
    int32_t target_temp;       /* 目标温度 (°C*100) */
    int32_t temp_rate;         /* 温度变化率 (°C/s*100) */
    
    /* 充电控制 */
    uint32_t charge_current;   /* 充电电流 (mA) */
    uint32_t charge_voltage;   /* 充电电压 (mV) */
    
    /* 温度区间 */
    int32_t temp_min;          /* 最低工作温度 */
    int32_t temp_max;          /* 最高工作温度 */
    int32_t temp_critical;     /* 临界温度 (停止充电) */
    
    /* 状态 */
    uint8_t thermal_zone;      /* 温度区间 (0=过冷，1=低温，2=正常，3=高温，4=过热) */
    bool charging_allowed;     /* 允许充电 */
    bool thermal_shutdown;     /* 热关断 */
} xy_temp_ctrl_t;

static xy_temp_ctrl_t s_temp_ctrl;

/* 温度区间定义 (°C*100) */
#define TEMP_COLD_LIMIT       0      /* 0°C 过冷 */
#define TEMP_COLD_HYSTERESIS  500    /* 5°C 回滞 */
#define TEMP_LOW_MIN          0      /* 0°C 低温下限 */
#define TEMP_LOW_MAX          1000   /* 10°C 低温上限 */
#define TEMP_NORMAL_MIN       1000   /* 10°C 正常下限 */
#define TEMP_NORMAL_MAX       4500   /* 45°C 正常上限 */
#define TEMP_HIGH_MIN         4500   /* 45°C 高温下限 */
#define TEMP_HIGH_MAX         6000   /* 60°C 高温上限 */
#define TEMP_HOT_LIMIT        6000   /* 60°C 过热 */
#define TEMP_HOT_HYSTERESIS   5500   /* 55°C 过热回滞 */

/* PID 参数默认值 */
#define PID_KP_DEFAULT  2.0f
#define PID_KI_DEFAULT  0.1f
#define PID_KD_DEFAULT  0.5f

/* ==================== PID 核心算法 ==================== */

static void pid_init(xy_pid_ctrl_t *pid, float Kp, float Ki, float Kd)
{
    memset(pid, 0, sizeof(xy_pid_ctrl_t));
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->output_min = 0;
    pid->output_max = 100;
    pid->integral_min = -1000;
    pid->integral_max = 1000;
}

static float pid_compute(xy_pid_ctrl_t *pid, float measured, float dt)
{
    if (dt <= 0) dt = 0.001f; /* 防止除零 */
    
    /* 计算误差 */
    float error = pid->setpoint - measured;
    
    /* 比例项 */
    float P = pid->Kp * error;
    
    /* 积分项 (带抗饱和) */
    pid->integral += error * dt;
    if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    } else if (pid->integral < pid->integral_min) {
        pid->integral = pid->integral_min;
    }
    float I = pid->Ki * pid->integral;
    
    /* 微分项 */
    float derivative = (error - pid->prev_error) / dt;
    float D = pid->Kd * derivative;
    
    pid->prev_error = error;
    
    /* 计算输出 */
    float output = P + I + D;
    
    /* 输出限幅 */
    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;
    
    return output;
}

static void pid_reset(xy_pid_ctrl_t *pid)
{
    pid->integral = 0;
    pid->prev_error = 0;
}

static void pid_setpoint(xy_pid_ctrl_t *pid, float sp)
{
    pid->setpoint = sp;
}

/* ==================== 温度区间判断 ==================== */

static uint8_t get_thermal_zone(int32_t temp_celsius_x100)
{
    /* 温度区间状态机 */
    static uint8_t last_zone = 2; /* 默认正常 */
    
    if (temp_celsius_x100 < TEMP_COLD_LIMIT) {
        /* 过冷区：T < 0°C */
        last_zone = 0;
    } else if (temp_celsius_x100 < TEMP_LOW_MAX) {
        /* 低温区：0°C ≤ T < 10°C */
        last_zone = 1;
    } else if (temp_celsius_x100 < TEMP_NORMAL_MAX) {
        /* 正常区：10°C ≤ T < 45°C */
        last_zone = 2;
    } else if (temp_celsius_x100 < TEMP_HIGH_MAX) {
        /* 高温区：45°C ≤ T < 60°C */
        last_zone = 3;
    } else {
        /* 过热区：T ≥ 60°C */
        last_zone = 4;
    }
    
    /* 应用回滞逻辑 */
    if (last_zone == 0 && temp_celsius_x100 > TEMP_COLD_HYSTERESIS) {
        last_zone = 1; /* 过冷→低温回滞 */
    } else if (last_zone == 4 && temp_celsius_x100 < TEMP_HOT_HYSTERESIS) {
        last_zone = 3; /* 过热→高温回滞 */
    }
    
    return last_zone;
}

/* ==================== 充电电流控制 ==================== */

static uint32_t get_charge_current_for_zone(uint8_t zone, uint32_t base_current)
{
    switch (zone) {
        case 0: /* 过冷区：禁止充电 */
            return 0;
        
        case 1: /* 低温区：小电流充电 (20%) */
            return base_current / 5;
        
        case 2: /* 正常区：正常充电 (100%) */
            return base_current;
        
        case 3: /* 高温区：降流充电 (50%) */
            return base_current / 2;
        
        case 4: /* 过热区：停止充电 */
            return 0;
        
        default:
            return 0;
    }
}

/* ==================== 公开 API ==================== */

int xy_temp_ctrl_init(void)
{
    memset(&s_temp_ctrl, 0, sizeof(s_temp_ctrl));
    
    /* 初始化 PID */
    pid_init(&s_temp_ctrl.pid, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
    pid_setpoint(&s_temp_ctrl.pid, 2500); /* 目标温度 25°C */
    
    /* 默认温度范围 */
    s_temp_ctrl.temp_min = TEMP_COLD_LIMIT;
    s_temp_ctrl.temp_max = TEMP_HOT_LIMIT;
    s_temp_ctrl.temp_critical = TEMP_HIGH_MAX;
    
    /* 默认充电参数 */
    s_temp_ctrl.charge_current = 1000; /* 1A */
    s_temp_ctrl.charge_voltage = 4200; /* 4.2V */
    
    s_temp_ctrl.initialized = true;
    s_temp_ctrl.enabled = true;
    
    xy_log_i("Temp Ctrl initialized: target=25°C, current=%dmA\n",
             s_temp_ctrl.charge_current);
    
    return 0;
}

int xy_temp_ctrl_update(int32_t temp_celsius_x100)
{
    if (!s_temp_ctrl.initialized) return -1;
    
    s_temp_ctrl.current_temp = temp_celsius_x100;
    
    /* 判断温度区间 */
    s_temp_ctrl.thermal_zone = get_thermal_zone(temp_celsius_x100);
    
    /* 根据温度区间调整充电电流 */
    uint32_t adjusted_current = get_charge_current_for_zone(
        s_temp_ctrl.thermal_zone, s_temp_ctrl.charge_current);
    
    /* PID 控制 (用于精确温度控制场景) */
    float pid_output = pid_compute(&s_temp_ctrl.pid, 
                                   (float)temp_celsius_x100 / 100.0f,
                                   1.0f);
    
    /* 充电允许判断 */
    s_temp_ctrl.charging_allowed = (s_temp_ctrl.thermal_zone != 0 && 
                                    s_temp_ctrl.thermal_zone != 4);
    s_temp_ctrl.thermal_shutdown = (s_temp_ctrl.thermal_zone == 4);
    
    xy_log_d("Temp Ctrl: T=%d.%d°C zone=%d I=%dmA allowed=%d\n",
             temp_celsius_x100/100, temp_celsius_x100%100,
             s_temp_ctrl.thermal_zone, adjusted_current,
             s_temp_ctrl.charging_allowed);
    
    return adjusted_current;
}

int xy_temp_ctrl_set_pid(float Kp, float Ki, float Kd)
{
    if (!s_temp_ctrl.initialized) return -1;
    
    s_temp_ctrl.pid.Kp = Kp;
    s_temp_ctrl.pid.Ki = Ki;
    s_temp_ctrl.pid.Kd = Kd;
    
    xy_log_d("PID params updated: Kp=%.2f Ki=%.2f Kd=%.2f\n", Kp, Ki, Kd);
    return 0;
}

int xy_temp_ctrl_set_target_temp(int32_t temp_celsius_x100)
{
    if (!s_temp_ctrl.initialized) return -1;
    
    s_temp_ctrl.target_temp = temp_celsius_x100;
    pid_setpoint(&s_temp_ctrl.pid, (float)temp_celsius_x100 / 100.0f);
    
    return 0;
}

int xy_temp_ctrl_set_charge_current(uint32_t current_mA)
{
    if (!s_temp_ctrl.initialized) return -1;
    
    s_temp_ctrl.charge_current = current_mA;
    return 0;
}

uint8_t xy_temp_ctrl_get_thermal_zone(void)
{
    return s_temp_ctrl.initialized ? s_temp_ctrl.thermal_zone : 0;
}

bool xy_temp_ctrl_is_charging_allowed(void)
{
    return s_temp_ctrl.initialized && s_temp_ctrl.charging_allowed;
}

bool xy_temp_ctrl_is_thermal_shutdown(void)
{
    return s_temp_ctrl.initialized && s_temp_ctrl.thermal_shutdown;
}

int32_t xy_temp_ctrl_get_current_temp(void)
{
    return s_temp_ctrl.initialized ? s_temp_ctrl.current_temp : 0;
}
