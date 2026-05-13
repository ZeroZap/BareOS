/**
 * @file example_charging.c
 * @brief Charging Control with PID Example
 * @version 1.0.0
 * @date 2026-03-01
 *
 * Demonstrates PID control for battery charging applications:
 * 1. CC-CV (Constant Current - Constant Voltage) charging
 * 2. Power bank output control
 * 3. Charge rate optimization
 * 4. Temperature-aware charging with safety limits
 */

#include "xy_pid.h"
#include <stdio.h>
#include <math.h>

/* Simulated system tick */
static uint32_t g_tick = 0;
uint32_t xy_os_tick_get(void) { return g_tick; }
void xy_os_delay_ms(uint32_t ms) { g_tick += ms; }

/**
 * @brief Battery state structure
 */
typedef struct {
    float voltage;           /* Battery voltage (V) */
    float current;          /* Charge/discharge current (A) */
    float capacity;         /* Current capacity (Ah) */
    float max_capacity;     /* Maximum capacity (Ah) */
    float temperature;      /* Battery temperature (°C) */
    float soc;              /* State of charge (%) */
    float internal_resistance; /* Internal resistance (mΩ) */
    float target_current;    /* Target charge current */
    float target_voltage;   /* Target charge voltage */
} battery_state_t;

static battery_state_t g_battery = {
    .voltage = 3.0f,            /* Initial voltage (empty LiPo ~3.0V) */
    .current = 0.0f,
    .capacity = 0.0f,
    .max_capacity = 3.0f,       /* 3000mAh battery */
    .temperature = 25.0f,
    .soc = 0.0f,
    .internal_resistance = 50.0f, /* 50mΩ internal resistance */
    .target_current = 1.0f,
    .target_voltage = 4.2f
};

/* Charging state machine */
typedef enum {
    CHARGE_STATE_CC = 0,     /* Constant Current phase */
    CHARGE_STATE_CV,        /* Constant Voltage phase */
    CHARGE_STATE_FLOAT,     /* Float charge (trickle) */
    CHARGE_STATE_DONE,      /* Charging complete */
    CHARGE_STATE_ERROR      /* Error (overtemp, etc.) */
} charge_state_t;

/**
 * @brief Simulate battery charging dynamics
 *
 * Simplified battery model:
 * - Voltage increases with SOC
 * - Internal resistance causes voltage drop under load
 * - Temperature rises with current and internal resistance
 *
 * @param target_voltage Target voltage from charger
 * @param target_current Target current from charger
 * @param dt Time step
 */
static void simulate_battery_charge(float target_voltage, float target_current, float dt)
{
    /* Battery model constants */
    const float v_bat_empty = 3.0f;
    const float v_bat_full = 4.2f;
    const float r_internal = 0.05f;  /* 50mΩ -> Ω */

    /* Determine actual current based on target and battery state */
    float voltage_delta = target_voltage - g_battery.voltage;

    /* Limit current based on voltage difference */
    if (voltage_delta < 0.1f) {
        /* Battery close to target, reduce current */
        g_battery.current = target_current * 0.1f;
    } else {
        g_battery.current = (voltage_delta > 0) ?
                           fminf(target_current, voltage_delta / r_internal) : 0;
    }

    /* Calculate capacity added */
    float delta_capacity = g_battery.current * dt / 3600.0f;  /* Convert A*h */
    g_battery.capacity += delta_capacity;

    /* Clamp capacity */
    if (g_battery.capacity > g_battery.max_capacity) {
        g_battery.capacity = g_battery.max_capacity;
    }

    /* Update SOC */
    g_battery.soc = (g_battery.capacity / g_battery.max_capacity) * 100.0f;

    /* Update battery voltage based on SOC */
    g_battery.voltage = v_bat_empty + (v_bat_full - v_bat_empty) *
                        (g_battery.soc / 100.0f);

    /* Add voltage drop due to internal resistance */
    g_battery.voltage += g_battery.current * r_internal;

    /* Temperature rise due to I^2*R heating */
    float heat_generated = g_battery.current * g_battery.current * r_internal;
    float heat_capacity = 50.0f;  /* Thermal mass */
    float heat_loss = (g_battery.temperature - 25.0f) * 0.5f;  /* Cooling to ambient */

    g_battery.temperature += (heat_generated - heat_loss) / heat_capacity * dt;

    /* Safety clamp */
    g_battery.voltage = (g_battery.voltage < v_bat_empty) ? v_bat_empty :
                        (g_battery.voltage > v_bat_full + 0.2f) ? v_bat_full + 0.2f : g_battery.voltage;
    g_battery.temperature = (g_battery.temperature < 0) ? 0 :
                            (g_battery.temperature > 60) ? 60 : g_battery.temperature;
}

/**
 * @brief Example 1: CC-CV charging control
 */
static void example_cc_cv_charging(void)
{
    xy_pid_t voltage_pid, current_pid;
    xy_pid_config_t v_config = {
        .kp = 5.0f,
        .ki = 0.2f,
        .kd = 0.5f,
        .output_min = 3.8f,
        .output_max = 4.25f
    };
    xy_pid_config_t i_config = {
        .kp = 2.0f,
        .ki = 0.1f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 1.5f
    };

    charge_state_t charge_state = CHARGE_STATE_CC;
    float target_voltage = 0;
    float target_current = 0;
    float measurement;
    float output;

    printf("\n=== Example 1: CC-CV Charging Control ===\n");
    printf("Battery: %.0fmAh, CV target: %.2fV\n",
           g_battery.max_capacity * 1000, g_battery.target_voltage);
    printf("CC target: %.2fA, Transition at: %.0f%% SOC\n\n",
           g_battery.target_current, 80.0f);

    xy_pid_init(&voltage_pid, &v_config);
    xy_pid_init(&current_pid, &i_config);
    xy_pid_set_mode(&voltage_pid, XY_PID_MODE_AUTO);
    xy_pid_set_mode(&current_pid, XY_PID_MODE_AUTO);

    printf("%-6s | %-4s | %-6s | %-6s | %-8s | %-6s | %-6s | %-8s\n",
           "Time", "State", "SOC", "Voltage", "V_Target", "Current", "C_Target", "Temp");
    printf("%-6s-+-%-4s-+-%-6s-+-%-6s-+-%-8s-+-%-6s-+-%-6s-+-%-8s\n",
           "------", "----", "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 40; i++) {
        g_tick = i * 500;

        measurement = g_battery.voltage;

        /* State machine logic */
        switch (charge_state) {
            case CHARGE_STATE_CC:
                target_current = 1.0f;  /* 1A constant current */
                target_voltage = g_battery.voltage;  /* No voltage target yet */
                xy_pid_set_setpoint(&current_pid, target_current);
                xy_pid_compute(&current_pid, g_battery.current, &output);
                target_voltage = 3.0f + output;  /* Control voltage to achieve current */

                if (g_battery.soc >= 80.0f) {
                    charge_state = CHARGE_STATE_CV;
                    xy_pid_reset(&current_pid);
                    printf("\n*** Transitioning to CV phase ***\n\n");
                }
                break;

            case CHARGE_STATE_CV:
                target_voltage = 4.2f;  /* Hold at 4.2V */
                xy_pid_set_setpoint(&voltage_pid, target_voltage);
                xy_pid_compute(&voltage_pid, measurement, &output);
                target_current = output;  /* Current tapers off */

                if (g_battery.current < 0.1f) {
                    charge_state = CHARGE_STATE_DONE;
                    printf("\n*** Charging complete! ***\n\n");
                }
                break;

            case CHARGE_STATE_DONE:
            default:
                target_voltage = g_battery.target_voltage;
                target_current = 0.0f;
                break;
        }

        /* Simulate with combined voltage/current control */
        float actual_target_v = charge_state == CHARGE_STATE_CC ?
                                target_voltage : g_battery.target_voltage;
        float actual_target_i = charge_state == CHARGE_STATE_CV ?
                                target_current : g_battery.target_current;

        simulate_battery_charge(actual_target_v, actual_target_i, 0.5f);

        const char *state_str = (charge_state == CHARGE_STATE_CC) ? "CC" :
                                (charge_state == CHARGE_STATE_CV) ? "CV" :
                                (charge_state == CHARGE_STATE_DONE) ? "DONE" : "ERR";

        printf("[%04dms] %4s | %5.1f%% | %6.2fV | %7.2fV | %6.2fA | %7.2fA | %6.1f°C\n",
               g_tick, state_str, g_battery.soc, g_battery.voltage,
               actual_target_v, g_battery.current, actual_target_i,
               g_battery.temperature);

        xy_os_delay_ms(500);

        /* Safety check */
        if (g_battery.temperature > 50.0f) {
            printf("\n*** Overtemperature protection triggered! ***\n");
            break;
        }
    }

    printf("\nFinal SOC: %.1f%%, Temperature: %.1f°C\n",
           g_battery.soc, g_battery.temperature);
}

/**
 * @brief Example 2: Power bank output control
 */
static void example_power_bank_control(void)
{
    xy_pid_t power_pid;
    xy_pid_config_t config = {
        .kp = 3.0f,
        .ki = 0.1f,
        .kd = 0.2f,
        .output_min = 0.0f,
        .output_max = 5.0f,  /* Max 5V output */
        .integral_min = 0.0f,
        .integral_max = 3.0f
    };

    float target_voltage = 5.0f;
    float load_resistance = 10.0f;  /* 10Ω load */
    float measurement;
    float output;

    printf("\n=== Example 2: Power Bank Output Control ===\n");
    printf("Target output: %.1fV, Load: %.1fΩ\n\n", target_voltage, load_resistance);

    xy_pid_init(&power_pid, &config);
    xy_pid_set_mode(&power_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&power_pid, target_voltage);
    xy_pid_enable_anti_windup(&power_pid, true);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Target", "Output", "Error", "PWM", "Load_A");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    /* Reset battery for power bank simulation */
    g_battery.voltage = 4.0f;
    g_battery.current = 0.0f;

    for (int i = 0; i < 20; i++) {
        g_tick = i * 200;
        measurement = g_battery.voltage;

        xy_pid_compute(&power_pid, measurement, &output);

        g_battery.voltage = output;  /* Simulated output voltage */

        /* Load current */
        float load_current = g_battery.voltage / load_resistance;

        printf("[%04dms] %7.2fV | %7.2fV | %+7.4fV | %7.2f%% | %7.4fA\n",
               g_tick, target_voltage, g_battery.voltage,
               target_voltage - g_battery.voltage, output, load_current);

        /* Simulate voltage droop under load */
        float droop = load_current * 0.01f;  /* Small droop effect */
        g_battery.voltage -= droop;

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&power_pid);
}

/**
 * @brief Example 3: Charge rate optimization
 */
static void example_charge_rate_optimization(void)
{
    xy_pid_t rate_pid;
    xy_pid_config_t config = {
        .kp = 2.0f,
        .ki = 0.05f,
        .kd = 0.5f,
        .output_min = 0.2f,   /* Minimum 0.2A to keep charging active */
        .output_max = 2.0f    /* Max 2A for battery longevity */
    };

    float max_charge_rate = 1.0f;  /* 1C rate (1A for 1Ah battery) */
    float battery_capacity = 3.0f; /* 3Ah battery */
    float target_current;
    float measurement;
    float output;

    printf("\n=== Example 3: Charge Rate Optimization ===\n");
    printf("Battery: %.0fmAh, Max rate: %.0fC (%.1fA max)\n\n",
           battery_capacity * 1000, max_charge_rate, battery_capacity * max_charge_rate);

    xy_pid_init(&rate_pid, &config);
    xy_pid_set_mode(&rate_pid, XY_PID_MODE_AUTO);
    xy_pid_enable_anti_windup(&rate_pid, true);

    printf("%-6s | %-6s | %-7s | %-7s | %-7s | %-8s\n",
           "Time", "SOC", "Target", "Actual", "Error", "Output");
    printf("%-6s-+-%-6s-+-%-7s-+-%-7s-+-%-7s-+-%-8s\n",
           "------", "------", "--------", "--------", "--------", "--------");

    /* Simulate varying charge rates as battery fills */
    for (int i = 0; i < 25; i++) {
        g_tick = i * 300;

        /* Target current decreases as battery fills (taper) */
        target_current = battery_capacity * max_charge_rate *
                         (1.0f - g_battery.soc / 100.0f) * 0.8f + 0.2f;

        /* Clamp to reasonable range */
        if (target_current > battery_capacity * max_charge_rate) {
            target_current = battery_capacity * max_charge_rate;
        }
        if (target_current < 0.1f) {
            target_current = 0.1f;
        }

        xy_pid_set_setpoint(&rate_pid, target_current);
        measurement = g_battery.current;

        xy_pid_compute(&rate_pid, measurement, &output);

        /* Simulate battery response to charge current */
        float charge_voltage = 4.2f;
        simulate_battery_charge(charge_voltage, output, 0.3f);

        printf("[%04dms] %5.1f%% | %6.2fA | %6.2fA | %+6.3fA | %6.2fA\n",
               g_tick, g_battery.soc, target_current, g_battery.current,
               target_current - g_battery.current, output);

        xy_os_delay_ms(300);

        if (g_battery.soc >= 99.0f) {
            printf("\n*** Battery nearly full, entering top-up mode ***\n");
            break;
        }
    }

    xy_pid_reset(&rate_pid);
}

/**
 * @brief Example 4: Temperature-aware charging
 */
static void example_temp_aware_charging(void)
{
    xy_pid_t temp_pid;
    xy_pid_config_t config = {
        .kp = 2.0f,
        .ki = 0.1f,
        .kd = 0.3f,
        .output_min = 0.0f,
        .output_max = 2.0f,
        .integral_min = 0.0f,
        .integral_max = 1.5f
    };

    float target_current = 1.0f;
    float max_temp = 40.0f;  /* Safety limit */
    float measurement;
    float output;

    printf("\n=== Example 4: Temperature-Aware Charging ===\n");
    printf("Target current: %.1fA, Max temperature: %.1f°C\n\n", target_current, max_temp);

    xy_pid_init(&temp_pid, &config);
    xy_pid_set_mode(&temp_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&temp_pid, target_current);
    xy_pid_enable_anti_windup(&temp_pid, true);

    printf("%-6s | %-8s | %-7s | %-8s | %-8s | %-8s\n",
           "Time", "Target", "Actual", "Battery", "Error", "Reduced");
    printf("%-6s-+-%-8s-+-%-7s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    /* Set high initial temperature to trigger derating */
    g_battery.temperature = 35.0f;

    for (int i = 0; i < 25; i++) {
        g_tick = i * 400;

        /* Adjust target based on temperature */
        float temp_error = g_battery.temperature - 25.0f;  /* Baseline temp */
        if (temp_error > 5.0f) {
            /* Reduce charge current as temperature rises */
            target_current = 1.0f * (1.0f - (temp_error - 5.0f) / 20.0f);
            if (target_current < 0.2f) target_current = 0.2f;
        } else {
            target_current = 1.0f;
        }

        xy_pid_set_setpoint(&temp_pid, target_current);
        measurement = g_battery.current;

        xy_pid_compute(&temp_pid, measurement, &output);

        printf("[%04dms] %7.2fA | %6.2fA | %7.2fA | %6.1f°C | %6.2fA\n",
               g_tick, target_current, g_battery.current,
               g_battery.temperature, output);

        float charge_voltage = 4.2f;
        simulate_battery_charge(charge_voltage, output, 0.4f);

        xy_os_delay_ms(400);

        if (g_battery.temperature > max_temp) {
            printf("\n*** Temperature limit reached, reducing charge rate ***\n");
        }
    }

    xy_pid_reset(&temp_pid);
}

/**
 * @brief Main function
 */
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  XinYi PID Controller - Charging Control Examples   ║\n");
    printf("║  Demonstrates battery charging with PID             ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    /* Reset battery state */
    g_battery.voltage = 3.0f;
    g_battery.current = 0.0f;
    g_battery.capacity = 0.0f;
    g_battery.soc = 0.0f;
    g_battery.temperature = 25.0f;

    example_cc_cv_charging();

    /* Reset for next example */
    g_battery.voltage = 3.0f;
    g_battery.capacity = 0.0f;
    g_battery.soc = 0.0f;
    g_battery.temperature = 25.0f;

    example_power_bank_control();

    g_battery.voltage = 3.0f;
    g_battery.capacity = 0.0f;
    g_battery.soc = 0.0f;
    g_battery.temperature = 25.0f;

    example_charge_rate_optimization();

    g_battery.voltage = 3.0f;
    g_battery.capacity = 0.0f;
    g_battery.soc = 0.0f;
    g_battery.temperature = 25.0f;

    example_temp_aware_charging();

    printf("\n══════════════════════════════════════════════════════\n");
    printf("All charging control examples completed!\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return 0;
}
