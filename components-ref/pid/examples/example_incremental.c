/**
 * @file example_incremental.c
 * @brief Incremental PID Controller Example
 * @version 1.0.0
 * @date 2026-03-01
 *
 * Demonstrates incremental (velocity form) PID controller.
 * This is useful for systems where:
 * 1. Actuator output is cumulative (e.g., motor position)
 * 2. Actuator saturation needs to be handled gracefully
 * 3. Bumpless transfer between manual and automatic is needed
 *
 * The incremental form calculates the change in output rather
 * than the absolute output value.
 */

#include "xy_pid.h"
#include <stdio.h>
#include <math.h>

/* Simulated system tick */
static uint32_t g_tick = 0;
uint32_t xy_os_tick_get(void) { return g_tick; }
void xy_os_delay_ms(uint32_t ms) { g_tick += ms; }

/* Simulated process state */
static float g_position = 0.0f;      /* Current motor position */
static float g_velocity = 0.0f;     /* Motor velocity */
static float g_motor_torque = 0.0f; /* Motor torque output */

/**
 * @brief Incremental PID controller structure
 *
 * This implements the velocity form of PID:
 * Δu(k) = u(k) - u(k-1)
 * Δu(k) = Kp[e(k) - e(k-1)] + Ki*e(k) + Kd[e(k) - 2*e(k-1) + e(k-2)]
 *
 * The controller outputs the CHANGE in control value, not the absolute value.
 */
typedef struct {
    xy_pid_config_t config;

    float setpoint;
    float output;

    /* Error history for incremental calculation */
    float error;           /* e(k) */
    float error_prev;      /* e(k-1) */
    float error_prev2;     /* e(k-2) */

    /* Incremental output */
    float output_prev;     /* u(k-1) */
    float output_increment; /* Δu(k) */

    /* Internal state */
    bool first_run;
} xy_incremental_pid_t;

/**
 * @brief Initialize incremental PID
 */
static void incremental_pid_init(xy_incremental_pid_t *pid,
                                  const xy_pid_config_t *config)
{
    if (!pid || !config) return;

    pid->config = *config;
    pid->setpoint = 0;
    pid->output = 0;
    pid->error = 0;
    pid->error_prev = 0;
    pid->error_prev2 = 0;
    pid->output_prev = 0;
    pid->output_increment = 0;
    pid->first_run = true;

    /* Default output to midpoint if symmetric limits */
    if (config->output_min < 0 && config->output_max > 0) {
        pid->output = (config->output_max + config->output_min) / 2;
        pid->output_prev = pid->output;
    }
}

/**
 * @brief Reset incremental PID
 */
static void incremental_pid_reset(xy_incremental_pid_t *pid)
{
    if (!pid) return;

    pid->error = 0;
    pid->error_prev = 0;
    pid->error_prev2 = 0;
    pid->output_increment = 0;
    pid->first_run = true;
}

/**
 * @brief Compute incremental PID output
 *
 * @param pid Incremental PID handle
 * @param setpoint Target value
 * @param measurement Current measured value
 * @param output Pointer to store output value
 * @return Output increment
 */
static float incremental_pid_compute(xy_incremental_pid_t *pid,
                                      float setpoint,
                                      float measurement,
                                      float *output)
{
    float dt = 0.1f;  /* Assume 100ms sample time */
    float p_increment, i_increment, d_increment;

    pid->setpoint = setpoint;

    if (pid->first_run) {
        pid->error = setpoint - measurement;
        pid->error_prev = pid->error;
        pid->error_prev2 = pid->error;
        pid->first_run = false;
        *output = pid->output;
        return 0;
    }

    /* Calculate error */
    pid->error = setpoint - measurement;

    /* Incremental PID formula:
     * Δu(k) = Kp[e(k) - e(k-1)] + Ki*e(k)*dt + Kd[e(k) - 2*e(k-1) + e(k-2)]/dt
     */
    p_increment = pid->config.kp * (pid->error - pid->error_prev);
    i_increment = pid->config.ki * pid->error * dt;
    d_increment = pid->config.kd *
                  (pid->error - 2.0f * pid->error_prev + pid->error_prev2) / dt;

    /* Calculate total increment */
    pid->output_increment = p_increment + i_increment + d_increment;

    /* Update previous errors */
    pid->error_prev2 = pid->error_prev;
    pid->error_prev = pid->error;

    /* Add increment to previous output */
    pid->output = pid->output_prev + pid->output_increment;

    /* Apply output limits */
    if (pid->output > pid->config.output_max) {
        pid->output = pid->config.output_max;
        pid->output_increment = pid->output - pid->output_prev;
    }
    if (pid->output < pid->config.output_min) {
        pid->output = pid->config.output_min;
        pid->output_increment = pid->output - pid->output_prev;
    }

    pid->output_prev = pid->output;
    *output = pid->output;

    return pid->output_increment;
}

/**
 * @brief Simulate DC motor with load
 *
 * @param torque Motor torque input
 * @param load External load (disturbance)
 * @param dt Time step
 */
static void simulate_motor(float torque, float load, float dt)
{
    float inertia = 1.0f;      /* Motor inertia */
    float friction = 0.5f;     /* Friction coefficient */
    float damping = 0.2f;      /* Velocity damping */

    /* Motor equation: J*dv/dt = torque - friction*v - damping*load */
    float acceleration = (torque - friction * g_velocity - damping * load) / inertia;

    g_velocity += acceleration * dt;
    g_position += g_velocity * dt;

    /* Clamp values */
    if (fabsf(g_velocity) < 0.001f) g_velocity = 0;
}

/**
 * @brief Example 1: Basic incremental position control
 */
static void example_incremental_position_control(void)
{
    xy_incremental_pid_t position_pid;
    xy_pid_config_t config = {
        .kp = 5.0f,
        .ki = 0.2f,
        .kd = 1.0f,
        .output_min = -100.0f,
        .output_max = 100.0f
    };

    float setpoint = 50.0f;  /* Target position */
    float measurement;
    float output;
    float increment;

    printf("\n=== Example 1: Incremental Position Control ===\n");
    printf("Target position: %.1f, Initial position: %.1f\n\n",
           setpoint, g_position);

    incremental_pid_init(&position_pid, &config);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Position", "Velocity", "Output", "Increment");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 20; i++) {
        g_tick = i * 100;
        measurement = g_position;

        increment = incremental_pid_compute(&position_pid, setpoint, measurement, &output);

        printf("[%04dms] %8.2f | %8.2f | %8.2f | %8.2f | %8.2f | %+8.2f\n",
               g_tick, setpoint, g_position, g_velocity, output, increment);

        /* Apply motor torque (output is torque command) */
        simulate_motor(output, 0.0f, 0.1f);

        xy_os_delay_ms(100);
    }

    printf("\nFinal position: %.2f, Final velocity: %.2f\n",
           g_position, g_velocity);
}

/**
 * @brief Example 2: Disturbance rejection
 */
static void example_disturbance_rejection(void)
{
    xy_incremental_pid_t disturbance_pid;
    xy_pid_config_t config = {
        .kp = 3.0f,
        .ki = 0.1f,
        .kd = 0.5f,
        .output_min = -50.0f,
        .output_max = 50.0f
    };

    float setpoint = 0.0f;
    float measurement;
    float output;
    float load;

    printf("\n=== Example 2: Disturbance Rejection ===\n");
    printf("Setpoint: %.1f, Applying disturbance at t=500ms\n\n", setpoint);

    incremental_pid_init(&disturbance_pid, &config);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Position", "Load", "Output", "Vel");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 15; i++) {
        g_tick = i * 100;
        measurement = g_position;

        /* Apply step disturbance at t=500ms */
        load = (i == 5) ? 30.0f : 0.0f;

        incremental_pid_compute(&disturbance_pid, setpoint, measurement, &output);

        printf("[%04dms] %8.2f | %8.2f | %8.2f | %8.2f | %8.2f | %8.2f\n",
               g_tick, setpoint, g_position, load, output, g_velocity);

        simulate_motor(output, load, 0.1f);

        xy_os_delay_ms(100);
    }

    /* Reset for next example */
    g_position = 0;
    g_velocity = 0;
}

/**
 * @brief Example 3: Reference tracking
 */
static void example_reference_tracking(void)
{
    xy_incremental_pid_t tracking_pid;
    xy_pid_config_t config = {
        .kp = 2.0f,
        .ki = 0.05f,
        .kd = 0.2f,
        .output_min = -100.0f,
        .output_max = 100.0f
    };

    float setpoint;
    float measurement;
    float output;

    printf("\n=== Example 3: Reference Tracking (Ramp Input) ===\n");
    printf("Tracking a ramp reference signal\n\n");

    incremental_pid_init(&tracking_pid, &config);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Position", "Error", "Output");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 20; i++) {
        g_tick = i * 100;

        /* Generate ramp reference: SP = 2*t */
        setpoint = (float)i * 2.0f;
        measurement = g_position;

        incremental_pid_compute(&tracking_pid, setpoint, measurement, &output);

        printf("[%04dms] %8.2f | %8.2f | %8.2f | %8.2f\n",
               g_tick, setpoint, g_position,
               setpoint - measurement, output);

        simulate_motor(output, 0.0f, 0.1f);

        xy_os_delay_ms(100);
    }

    /* Reset for next example */
    g_position = 0;
    g_velocity = 0;
}

/**
 * @brief Example 4: Velocity control (motor speed regulation)
 */
static void example_velocity_control(void)
{
    xy_incremental_pid_t velocity_pid;
    xy_pid_config_t config = {
        .kp = 10.0f,
        .ki = 1.0f,
        .kd = 2.0f,
        .output_min = -100.0f,
        .output_max = 100.0f
    };

    float setpoint = 20.0f;  /* Target velocity */
    float measurement;
    float output;

    printf("\n=== Example 4: Velocity Control (Speed Regulation) ===\n");
    printf("Target velocity: %.1f\n\n", setpoint);

    incremental_pid_init(&velocity_pid, &config);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Velocity", "Error", "Output");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 15; i++) {
        g_tick = i * 100;
        measurement = g_velocity;

        incremental_pid_compute(&velocity_pid, setpoint, measurement, &output);

        printf("[%04dms] %8.2f | %8.2f | %8.2f | %8.2f\n",
               g_tick, setpoint, g_velocity,
               setpoint - g_velocity, output);

        simulate_motor(output, 0.0f, 0.1f);

        xy_os_delay_ms(100);
    }

    /* Reset for next example */
    g_position = 0;
    g_velocity = 0;
}

/**
 * @brief Main function
 */
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  XinYi PID Controller - Incremental Examples        ║\n");
    printf("║  Demonstrates velocity form PID control            ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    example_incremental_position_control();
    example_disturbance_rejection();
    example_reference_tracking();
    example_velocity_control();

    printf("\n══════════════════════════════════════════════════════\n");
    printf("All incremental PID examples completed!\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return 0;
}
