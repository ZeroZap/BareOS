/**
 * @file example_basic.c
 * @brief Basic PID Controller Usage Example (Position Form)
 * @version 1.0.0
 * @date 2026-03-01
 *
 * Demonstrates basic PID controller usage in position form.
 * This example shows how to:
 * 1. Initialize a PID controller
 * 2. Compute PID output based on setpoint and measurement
 * 3. Adjust PID parameters online
 * 4. Reset the PID controller
 */

#include "xy_pid.h"
#include <stdio.h>
#include <math.h>

/* Simulated system tick for demonstration */
static uint32_t g_tick = 0;
uint32_t xy_os_tick_get(void) { return g_tick; }
void xy_os_delay_ms(uint32_t ms) { g_tick += ms; }

/* Simulated process variable (temperature sensor) */
static float g_process_variable = 25.0f;  /* Initial temperature: 25°C */
static float g_heater_power = 0.0f;        /* Heater power output */

/**
 * @brief Simulate a simple thermal system
 *
 * This is a simplified thermal model for demonstration.
 * In real applications, this would be replaced with actual sensor readings.
 *
 * @param heater_power Heater power (0-100%)
 * @param ambient_temp Ambient temperature
 * @param dt Time step in seconds
 * @return Simulated temperature
 */
static float simulate_thermal_system(float heater_power, float ambient_temp, float dt)
{
    /* Simplified thermal model:
     * - Temperature rises when heater is on
     * - Temperature falls toward ambient when heater is off
     * - Rate depends on heater power and temperature difference
     */
    float thermal_constant = 0.05f;  /* Thermal time constant */
    float heat_input = heater_power * thermal_constant * dt;
    float heat_loss = (g_process_variable - ambient_temp) * thermal_constant * dt * 0.1f;

    g_process_variable += heat_input - heat_loss;

    /* Clamp to reasonable range */
    if (g_process_variable < ambient_temp - 10.0f) {
        g_process_variable = ambient_temp - 10.0f;
    }
    if (g_process_variable > 100.0f) {
        g_process_variable = 100.0f;
    }

    return g_process_variable;
}

/**
 * @brief Apply control output to actuator
 *
 * In real application, this would control hardware like:
 * - PWM signal to heater
 * - Valve position for flow control
 * - Motor driver for position control
 */
static void apply_output(float output)
{
    g_heater_power = output;

    /* For simulation, we also update the thermal model */
    printf("  [ACTUATOR] Heater power: %.2f%%\n", output);
}

/**
 * @brief Read sensor measurement
 *
 * In real application, this would read from:
 * - ADC for temperature sensor
 * - Encoder for position
 * - Pressure sensor for flow control
 */
static float read_measurement(void)
{
    return g_process_variable;
}

/**
 * @brief Example 1: Basic temperature control with fixed parameters
 */
static void example_basic_temperature_control(void)
{
    xy_pid_t temperature_pid;
    xy_pid_config_t config = {
        .kp = 2.0f,
        .ki = 0.1f,
        .kd = 0.5f,
        .output_min = 0.0f,
        .output_max = 100.0f,
        .integral_min = 0.0f,
        .integral_max = 50.0f,
        .derivative_filter = 0.1f
    };

    float setpoint = 50.0f;  /* Target temperature: 50°C */
    float ambient_temp = 25.0f;  /* Room temperature */
    float measurement;
    float output;

    printf("\n=== Example 1: Basic Temperature Control ===\n");
    printf("Target temperature: %.1f°C, Ambient: %.1f°C\n", setpoint, ambient_temp);

    /* Initialize PID controller */
    xy_pid_init(&temperature_pid, &config);
    xy_pid_set_mode(&temperature_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&temperature_pid, setpoint);

    /* Enable advanced features */
    xy_pid_enable_anti_windup(&temperature_pid, true);
    xy_pid_enable_derivative_filter(&temperature_pid, true, 0.1f);

    printf("PID initialized: Kp=%.2f, Ki=%.2f, Kd=%.2f\n",
           config.kp, config.ki, config.kd);
    printf("Running control loop...\n\n");

    /* Control loop simulation */
    for (int i = 0; i < 20; i++) {
        g_tick = i * 100;  /* 100ms intervals */

        measurement = read_measurement();
        printf("[%03dms] PV=%.2f°C | ", g_tick, measurement);

        /* Compute PID output */
        xy_pid_compute(&temperature_pid, measurement, &output);

        printf("SP=%.1f | Error=%.2f | Output=%.2f\n",
               setpoint, xy_pid_get_error(&temperature_pid), output);

        /* Apply output to actuator */
        apply_output(output);

        /* Simulate thermal response */
        simulate_thermal_system(output, ambient_temp, 0.1f);

        xy_os_delay_ms(100);
    }

    printf("\nFinal temperature: %.2f°C\n", g_process_variable);
    xy_pid_reset(&temperature_pid);
}

/**
 * @brief Example 2: Online parameter adjustment
 */
static void example_online_tuning(void)
{
    xy_pid_t process_pid;
    xy_pid_config_t config = {
        .kp = 1.0f,
        .ki = 0.0f,
        .kd = 0.0f,
        .output_min = -100.0f,
        .output_max = 100.0f
    };

    float setpoint = 100.0f;
    float measurement = 0.0f;
    float output;

    printf("\n=== Example 2: Online Parameter Adjustment ===\n");

    /* Initialize with P-only initially */
    xy_pid_init(&process_pid, &config);
    xy_pid_set_mode(&process_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&process_pid, setpoint);

    printf("Phase 1: P-only control (Kp=1.0)\n");
    for (int i = 0; i < 5; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&process_pid, measurement, &output);
        printf("  [%03dms] PV=%.2f, Error=%.2f, Output=%.2f\n",
               g_tick, measurement, xy_pid_get_error(&process_pid), output);
        xy_os_delay_ms(100);
    }

    /* Add integral term */
    printf("\nPhase 2: Adding Integral (Ki=0.1)\n");
    xy_pid_set_tuning(&process_pid, 1.0f, 0.1f, 0.0f);

    for (int i = 5; i < 10; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&process_pid, measurement, &output);
        printf("  [%03dms] PV=%.2f, Error=%.2f, Output=%.2f, Integral=%.2f\n",
               g_tick, measurement, xy_pid_get_error(&process_pid), output,
               xy_pid_get_integral(&process_pid));
        xy_os_delay_ms(100);
    }

    /* Add derivative term */
    printf("\nPhase 3: Adding Derivative (Kd=0.3)\n");
    xy_pid_set_tuning(&process_pid, 1.0f, 0.1f, 0.3f);

    for (int i = 10; i < 15; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&process_pid, measurement, &output);
        printf("  [%03dms] PV=%.2f, Error=%.2f, Output=%.2f, Derivative=%.4f\n",
               g_tick, measurement, xy_pid_get_error(&process_pid), output,
               xy_pid_get_derivative(&process_pid));
        xy_os_delay_ms(100);
    }

    xy_pid_reset(&process_pid);
}

/**
 * @brief Example 3: Output limiting demonstration
 */
static void example_output_limiting(void)
{
    xy_pid_t limited_pid;
    xy_pid_config_t config = {
        .kp = 5.0f,
        .ki = 1.0f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 30.0f,  /* Limited to 30% */
        .integral_min = 0.0f,
        .integral_max = 20.0f
    };

    float setpoint = 100.0f;
    float measurement;
    float output;

    printf("\n=== Example 3: Output Limiting ===\n");
    printf("Output limited to: %.1f - %.1f\n", config.output_min, config.output_max);

    xy_pid_init(&limited_pid, &config);
    xy_pid_set_mode(&limited_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&limited_pid, setpoint);
    xy_pid_enable_anti_windup(&limited_pid, true);

    for (int i = 0; i < 10; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&limited_pid, measurement, &output);
        printf("[%03dms] Error=%.2f, Output=%.2f, Integral=%.2f\n",
               g_tick, xy_pid_get_error(&limited_pid), output,
               xy_pid_get_integral(&limited_pid));
        xy_os_delay_ms(100);
    }

    xy_pid_reset(&limited_pid);
}

/**
 * @brief Example 4: Manual/Auto mode switching
 */
static void example_mode_switching(void)
{
    xy_pid_t mode_pid;
    xy_pid_config_t config = {
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };

    float setpoint = 50.0f;
    float measurement;
    float output;

    printf("\n=== Example 4: Manual/Auto Mode Switching ===\n");

    xy_pid_init(&mode_pid, &config);
    xy_pid_set_setpoint(&mode_pid, setpoint);

    printf("Phase 1: AUTO mode\n");
    xy_pid_set_mode(&mode_pid, XY_PID_MODE_AUTO);

    for (int i = 0; i < 3; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&mode_pid, measurement, &output);
        printf("  [%03dms] Mode=AUTO, Output=%.2f\n", g_tick, output);
        xy_os_delay_ms(100);
    }

    printf("\nPhase 2: MANUAL mode (output fixed at 50)\n");
    xy_pid_set_mode(&mode_pid, XY_PID_MODE_MANUAL);
    xy_pid_set_output_limits(&mode_pid, 50.0f, 50.0f);  /* Fixed output */

    for (int i = 3; i < 6; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&mode_pid, measurement, &output);
        printf("  [%03dms] Mode=MANUAL, Output=%.2f\n", g_tick, output);
        xy_os_delay_ms(100);
    }

    printf("\nPhase 3: Back to AUTO mode\n");
    xy_pid_set_output_limits(&mode_pid, 0.0f, 100.0f);
    xy_pid_set_mode(&mode_pid, XY_PID_MODE_AUTO);

    for (int i = 6; i < 9; i++) {
        g_tick = i * 100;
        measurement = read_measurement();
        xy_pid_compute(&mode_pid, measurement, &output);
        printf("  [%03dms] Mode=AUTO, Output=%.2f\n", g_tick, output);
        xy_os_delay_ms(100);
    }

    xy_pid_reset(&mode_pid);
}

/**
 * @brief Main function - run all examples
 */
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  XinYi PID Controller - Basic Examples              ║\n");
    printf("║  Demonstrates position form PID control             ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    /* Run all examples */
    example_basic_temperature_control();
    example_online_tuning();
    example_output_limiting();
    example_mode_switching();

    printf("\n══════════════════════════════════════════════════════\n");
    printf("All examples completed successfully!\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return 0;
}
