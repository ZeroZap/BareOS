/**
 * @file example_temperature.c
 * @brief Temperature Control with PID Example
 * @version 1.0.0
 * @date 2026-03-01
 *
 * Demonstrates temperature control using PID for:
 * 1. Heating control with PWM output
 * 2. Cooling control with dual-stage output (heat/cool)
 * 3. Bang-bang plus PID hybrid control
 * 4. Thermal model with overshoot and settling behavior
 */

#include "xy_pid.h"
#include <stdio.h>
#include <math.h>

/* Simulated system tick */
static uint32_t g_tick = 0;
uint32_t xy_os_tick_get(void) { return g_tick; }
void xy_os_delay_ms(uint32_t ms) { g_tick += ms; }

/* Temperature system state */
typedef struct {
    float temperature;       /* Current temperature */
    float target_temp;       /* Target temperature */
    float ambient_temp;      /* Ambient temperature */
    float heater_power;      /* Heater output (0-100%) */
    float cooler_power;      /* Cooler output (0-100%) */
    float heat_capacity;     /* Thermal mass */
    float thermal_resistance;/* Thermal resistance to ambient */
    float heat_source;       /* Internal heat source */
} temperature_system_t;

static temperature_system_t g_temp_sys = {
    .temperature = 25.0f,
    .target_temp = 0.0f,
    .ambient_temp = 25.0f,
    .heater_power = 0.0f,
    .cooler_power = 0.0f,
    .heat_capacity = 50.0f,      /* J/K */
    .thermal_resistance = 2.0f,   /* K/W */
    .heat_source = 0.0f
};

/**
 * @brief Simulate temperature dynamics
 *
 * Uses a first-order thermal model:
 * C * dT/dt = Q_heater - Q_loss + Q_source
 * Q_loss = (T - T_ambient) / R
 */
static void simulate_temperature(float dt)
{
    /* Heat input from heater */
    float heat_input = g_temp_sys.heater_power * 10.0f;  /* 10W at 100% */

    /* Cooling (if available) */
    float cool_input = g_temp_sys.cooler_power * 8.0f;   /* 8W cooling at 100% */

    /* Heat loss to ambient */
    float heat_loss = (g_temp_sys.temperature - g_temp_sys.ambient_temp) /
                      g_temp_sys.thermal_resistance;

    /* Temperature change rate */
    float dT = (heat_input - cool_input - heat_loss + g_temp_sys.heat_source) /
               g_temp_sys.heat_capacity;

    g_temp_sys.temperature += dT * dt;

    /* Safety clamps */
    if (g_temp_sys.temperature < 0.0f) {
        g_temp_sys.temperature = 0.0f;
    }
    if (g_temp_sys.temperature > 150.0f) {
        g_temp_sys.temperature = 150.0f;
    }
}

/**
 * @brief Example 1: Simple heating control with PWM
 */
static void example_heating_control(void)
{
    xy_pid_t heater_pid;
    xy_pid_config_t config = {
        .kp = 15.0f,
        .ki = 0.5f,
        .kd = 10.0f,
        .output_min = 0.0f,
        .output_max = 100.0f,
        .integral_min = 0.0f,
        .integral_max = 80.0f,
        .derivative_filter = 0.15f
    };

    float setpoint = 60.0f;  /* Target: 60°C */
    float measurement;
    float output;

    printf("\n=== Example 1: Heating Control ===\n");
    printf("Target: %.1f°C, Ambient: %.1f°C\n", setpoint, g_temp_sys.ambient_temp);

    xy_pid_init(&heater_pid, &config);
    xy_pid_set_mode(&heater_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&heater_pid, setpoint);
    xy_pid_enable_anti_windup(&heater_pid, true);
    xy_pid_enable_derivative_filter(&heater_pid, true, 0.15f);

    printf("\n%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Temp", "Error", "Heater", "Action");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 30; i++) {
        g_tick = i * 200;
        measurement = g_temp_sys.temperature;

        xy_pid_compute(&heater_pid, measurement, &output);

        g_temp_sys.heater_power = output;
        g_temp_sys.cooler_power = 0.0f;

        const char *action = (output > 50.0f) ? "HIGH" :
                             (output > 20.0f) ? "MED" : "LOW";

        printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %7.1f%% | %s\n",
               g_tick, setpoint, measurement,
               setpoint - measurement, output, action);

        simulate_temperature(0.2f);

        xy_os_delay_ms(200);

        /* Stop if settled */
        if (i > 15 && fabsf(measurement - setpoint) < 0.5f) {
            printf("\n>>> Temperature settled at %.2f°C <<<\n", measurement);
            break;
        }
    }

    xy_pid_reset(&heater_pid);
}

/**
 * @brief Example 2: Dual-stage heating/cooling control
 */
static void example_heating_cooling(void)
{
    xy_pid_t hvac_pid;
    xy_pid_config_t config = {
        .kp = 20.0f,
        .ki = 1.0f,
        .kd = 5.0f,
        .output_min = -100.0f,
        .output_max = 100.0f,
        .integral_min = -50.0f,
        .integral_max = 50.0f
    };

    float setpoint = 35.0f;  /* Target: 35°C (middle of range) */
    float measurement;
    float output;

    printf("\n=== Example 2: Heating/Cooling Control ===\n");
    printf("Target: %.1f°C, Ambient: %.1f°C\n", setpoint, g_temp_sys.ambient_temp);

    xy_pid_init(&hvac_pid, &config);
    xy_pid_set_mode(&hvac_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&hvac_pid, setpoint);
    xy_pid_enable_anti_windup(&hvac_pid, true);

    printf("\n%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Temp", "Error", "Heat", "Cool");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 30; i++) {
        g_tick = i * 200;
        measurement = g_temp_sys.temperature;

        xy_pid_compute(&hvac_pid, measurement, &output);

        /* Split output into heating and cooling */
        if (output > 0) {
            g_temp_sys.heater_power = output;
            g_temp_sys.cooler_power = 0.0f;
        } else {
            g_temp_sys.heater_power = 0.0f;
            g_temp_sys.cooler_power = -output;
        }

        printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %6.1f%% | %6.1f%%\n",
               g_tick, setpoint, measurement,
               setpoint - measurement,
               g_temp_sys.heater_power,
               g_temp_sys.cooler_power);

        simulate_temperature(0.2f);

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&hvac_pid);
}

/**
 * @brief Example 3: Setpoint changes (tracking)
 */
static void example_setpoint_changes(void)
{
    xy_pid_t tracking_pid;
    xy_pid_config_t config = {
        .kp = 12.0f,
        .ki = 0.3f,
        .kd = 8.0f,
        .output_min = 0.0f,
        .output_max = 100.0f,
        .integral_min = 0.0f,
        .integral_max = 60.0f,
        .derivative_filter = 0.1f
    };

    float setpoints[] = {30.0f, 50.0f, 40.0f, 65.0f, 35.0f};
    int num_setpoints = 5;
    float measurement;
    float output;

    printf("\n=== Example 3: Setpoint Tracking ===\n");
    printf("Following setpoint changes: 30->50->40->65->35°C\n\n");

    xy_pid_init(&tracking_pid, &config);
    xy_pid_set_mode(&tracking_pid, XY_PID_MODE_AUTO);
    xy_pid_enable_anti_windup(&tracking_pid, true);
    xy_pid_enable_derivative_filter(&tracking_pid, true, 0.1f);

    for (int sp_idx = 0; sp_idx < num_setpoints; sp_idx++) {
        float sp = setpoints[sp_idx];
        xy_pid_set_setpoint(&tracking_pid, sp);

        printf("-- Setpoint: %.1f°C --\n", sp);

        for (int i = 0; i < 15; i++) {
            g_tick = (sp_idx * 15 + i) * 200;
            measurement = g_temp_sys.temperature;

            xy_pid_compute(&tracking_pid, measurement, &output);

            g_temp_sys.heater_power = output;
            g_temp_sys.cooler_power = 0.0f;

            printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %6.1f%%\n",
                   g_tick, sp, measurement,
                   sp - measurement, output);

            simulate_temperature(0.2f);

            xy_os_delay_ms(200);
        }
    }

    xy_pid_reset(&tracking_pid);
}

/**
 * @brief Example 4: Disturbance rejection (opening door)
 */
static void example_disturbance_rejection(void)
{
    xy_pid_t door_pid;
    xy_pid_config_t config = {
        .kp = 20.0f,
        .ki = 0.8f,
        .kd = 6.0f,
        .output_min = 0.0f,
        .output_max = 100.0f,
        .integral_min = 0.0f,
        .integral_max = 80.0f
    };

    float setpoint = 45.0f;
    float measurement;
    float output;
    float door_open_time = 1000;  /* ms */

    printf("\n=== Example 4: Disturbance Rejection (Door Opening) ===\n");
    printf("Target: %.1f°C, Door opens at %dms\n\n", setpoint, door_open_time);

    xy_pid_init(&door_pid, &config);
    xy_pid_set_mode(&door_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&door_pid, setpoint);
    xy_pid_enable_anti_windup(&door_pid, true);

    printf("\n%-6s | %-8s | %-8s | %-8s | %-8s | Note\n",
           "Time", "Setpoint", "Temp", "Error", "Heat");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+----------------\n",
           "------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 25; i++) {
        g_tick = i * 200;
        measurement = g_temp_sys.temperature;

        xy_pid_compute(&door_pid, measurement, &output);

        g_temp_sys.heater_power = output;

        /* Disturbance: door opens at t=1000ms, increases heat loss */
        float door_factor = (g_tick >= door_open_time) ? 3.0f : 1.0f;

        printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %6.1f%% | %s\n",
               g_tick, setpoint, measurement,
               setpoint - measurement, output,
               (g_tick >= door_open_time) ? "DOOR OPEN" : "");

        /* Modify thermal model to simulate door */
        float saved_R = g_temp_sys.thermal_resistance;
        g_temp_sys.thermal_resistance = saved_R / door_factor;

        simulate_temperature(0.2f);

        g_temp_sys.thermal_resistance = saved_R;

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&door_pid);
}

/**
 * @brief Example 5: Thermal overshoot and derivative tuning
 */
static void example_overshoot_demonstration(void)
{
    xy_pid_t tune_pid;
    xy_pid_config_t config = {
        .kp = 25.0f,
        .ki = 2.0f,
        .kd = 0.0f,  /* No derivative initially */
        .output_min = 0.0f,
        .output_max = 100.0f
    };

    float setpoint = 55.0f;
    float measurement;
    float output;

    printf("\n=== Example 5: Overshoot Demonstration ===\n");
    printf("Phase 1: PID without derivative (high overshoot)\n");
    printf("Target: %.1f°C\n\n", setpoint);

    xy_pid_init(&tune_pid, &config);
    xy_pid_set_mode(&tune_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&tune_pid, setpoint);
    xy_pid_enable_anti_windup(&tune_pid, true);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Setpoint", "Temp", "Error", "Heat");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 20; i++) {
        g_tick = i * 200;
        measurement = g_temp_sys.temperature;

        xy_pid_compute(&tune_pid, measurement, &output);

        g_temp_sys.heater_power = output;

        printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %6.1f%%\n",
               g_tick, setpoint, measurement,
               setpoint - measurement, output);

        simulate_temperature(0.2f);

        xy_os_delay_ms(200);
    }

    /* Phase 2: Add derivative to reduce overshoot */
    printf("\nPhase 2: Adding derivative (Kd=5.0) to reduce overshoot\n\n");

    xy_pid_set_tuning(&tune_pid, 25.0f, 2.0f, 5.0f);
    xy_pid_reset(&tune_pid);

    for (int i = 0; i < 20; i++) {
        g_tick = 10000 + i * 200;
        measurement = g_temp_sys.temperature;

        xy_pid_compute(&tune_pid, measurement, &output);

        g_temp_sys.heater_power = output;

        printf("[%04dms] %7.1f°C | %7.2f°C | %+7.2f | %6.1f%%\n",
               g_tick, setpoint, measurement,
               setpoint - measurement, output);

        simulate_temperature(0.2f);

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&tune_pid);
}

/**
 * @brief Main function
 */
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  XinYi PID Controller - Temperature Control Examples║\n");
    printf("║  Demonstrates thermal control with PID             ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    /* Reset temperature to room temp */
    g_temp_sys.temperature = 25.0f;
    g_temp_sys.ambient_temp = 25.0f;

    example_heating_control();

    /* Reset for next test */
    g_temp_sys.temperature = 25.0f;
    example_heating_cooling();

    g_temp_sys.temperature = 25.0f;
    example_setpoint_changes();

    g_temp_sys.temperature = 25.0f;
    example_disturbance_rejection();

    g_temp_sys.temperature = 25.0f;
    example_overshoot_demonstration();

    printf("\n══════════════════════════════════════════════════════\n");
    printf("All temperature control examples completed!\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return 0;
}
