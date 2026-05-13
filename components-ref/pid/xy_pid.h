/**
 * @file xy_pid.h
 * @brief PID Controller with Advanced Features
 * @version 2.0.0
 * @date 2026-03-01 早晨
 */

#ifndef XY_PID_H
#define XY_PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief PID 模式
 */
typedef enum {
    XY_PID_MODE_MANUAL = 0,   /**< 手动模式 */
    XY_PID_MODE_AUTO = 1,     /**< 自动模式 */
} xy_pid_mode_t;

/**
 * @brief PID 配置
 */
typedef struct {
    float kp;                 /**< 比例增益 */
    float ki;                 /**< 积分增益 */
    float kd;                 /**< 微分增益 */
    float output_min;         /**< 输出最小值 */
    float output_max;         /**< 输出最大值 */
    float integral_min;       /**< 积分最小值 (抗饱和) */
    float integral_max;       /**< 积分最大值 (抗饱和) */
    float derivative_filter;  /**< 微分滤波器系数 (0-1) */
} xy_pid_config_t;

/**
 * @brief PID 控制器结构
 */
typedef struct {
    xy_pid_config_t config;   /**< 配置参数 */
    xy_pid_mode_t mode;       /**< 工作模式 */
    
    float setpoint;           /**< 设定值 */
    float input;              /**< 输入值 (反馈) */
    float output;             /**< 输出值 */
    
    float error;              /**< 当前误差 */
    float error_prev;         /**< 上次误差 */
    float error_sum;          /**< 误差积分 */
    float derivative;         /**< 误差微分 */
    
    float integral;           /**< 积分项 (抗饱和后) */
    float integral_raw;       /**< 原始积分项 */
    
    uint32_t last_update;     /**< 上次更新时间 (ms) */
    uint32_t update_count;    /**< 更新次数 */
    
    bool first_run;           /**< 首次运行标志 */
    bool anti_windup;         /**< 抗积分饱和启用 */
    bool derivative_filter;   /**< 微分滤波启用 */
} xy_pid_t;

/**
 * @brief 错误码
 */
#define XY_PID_OK             0
#define XY_PID_ERROR          (-1)
#define XY_PID_INVALID_PARAM  (-2)

/**
 * @brief 初始化 PID 控制器
 * @param pid PID 控制器句柄
 * @param config 配置参数
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_init(xy_pid_t *pid, const xy_pid_config_t *config);

/**
 * @brief 重置 PID 控制器
 * @param pid PID 控制器句柄
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_reset(xy_pid_t *pid);

/**
 * @brief 设置 PID 参数
 * @param pid PID 控制器句柄
 * @param kp 比例增益
 * @param ki 积分增益
 * @param kd 微分增益
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_set_tuning(xy_pid_t *pid, float kp, float ki, float kd);

/**
 * @brief 设置输出限幅
 * @param pid PID 控制器句柄
 * @param min 输出最小值
 * @param max 输出最大值
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_set_output_limits(xy_pid_t *pid, float min, float max);

/**
 * @brief 设置设定值
 * @param pid PID 控制器句柄
 * @param setpoint 设定值
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_set_setpoint(xy_pid_t *pid, float setpoint);

/**
 * @brief 设置输入值 (反馈)
 * @param pid PID 控制器句柄
 * @param input 输入值
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_set_input(xy_pid_t *pid, float input);

/**
 * @brief 计算 PID 输出
 * @param pid PID 控制器句柄
 * @param input 输入值
 * @param output 输出值指针
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_compute(xy_pid_t *pid, float input, float *output);

/**
 * @brief 获取当前误差
 * @param pid PID 控制器句柄
 * @return 误差值
 */
float xy_pid_get_error(const xy_pid_t *pid);

/**
 * @brief 获取积分项
 * @param pid PID 控制器句柄
 * @return 积分项值
 */
float xy_pid_get_integral(const xy_pid_t *pid);

/**
 * @brief 获取微分项
 * @param pid PID 控制器句柄
 * @return 微分项值
 */
float xy_pid_get_derivative(const xy_pid_t *pid);

/**
 * @brief 设置工作模式
 * @param pid PID 控制器句柄
 * @param mode 工作模式
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_set_mode(xy_pid_t *pid, xy_pid_mode_t mode);

/**
 * @brief 获取工作模式
 * @param pid PID 控制器句柄
 * @return 工作模式
 */
xy_pid_mode_t xy_pid_get_mode(const xy_pid_t *pid);

/**
 * @brief 启用抗积分饱和
 * @param pid PID 控制器句柄
 * @param enable 是否启用
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_enable_anti_windup(xy_pid_t *pid, bool enable);

/**
 * @brief 启用微分滤波
 * @param pid PID 控制器句柄
 * @param enable 是否启用
 * @param coef 滤波系数 (0-1)
 * @return XY_PID_OK 成功，其他值失败
 */
int xy_pid_enable_derivative_filter(xy_pid_t *pid, bool enable, float coef);

#ifdef __cplusplus
}
#endif

#endif /* XY_PID_H */
