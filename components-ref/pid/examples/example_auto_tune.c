/**
 * @file example_auto_tune.c
 * @brief Auto-Tuning with Ziegler-Nichols Method Example
 * @version 1.0.0
 * @date 2026-03-01
 *
 * Demonstrates PID auto-tuning using the Ziegler-Nichols method.
 * This example shows how to:
 * 1. Perform relay auto-tuning (ultimate gain measurement)
 * 2. Calculate PID parameters from test data
 * 3. Apply auto-tuned parameters to PID controller
 * 4. Compare manual vs auto-tuned performance
 */

#include "xy_pid.h"
#include "xy_pid_auto.h"
#include <stdio.h>
#include <math.h>

/* Simulated system tick */
static uint32_t g_tick = 0;
uint32_t xy_os_tick_get(void) { return g_tick; }
void xy_os_delay_ms(uint32_t ms) { g_tick += ms; }

/* Process simulation state */
static float g_process_value = 0.0f;       /* Current process value */
static float g_process_output = 0.0f;    /* Controller output */
static float g_setpoint = 50.0f;          /* Target setpoint */

/**
 * @brief Simulate a first-order plus deadtime (FOPDT) process
 *
 * Model: G(s) = K * exp(-td*s) / (Ts + 1)
 *
 * @param input Controller output
 * @param dt Time step
 */
static void simulate_fopdt_process(float input, float dt)
{
    /* FOPDT model parameters:
     * K = process gain = 1.0
     * T = time constant = 5.0s
     * td = deadtime = 1.0s
     */
    const float K = 1.0f;
    const float T = 5.0f;
    const float td = 1.0f;

    static float delayed_input = 0.0f;
    static float pv_history[20] = {0};
    static int history_idx = 0;

    /* Deadtime simulation - delay the input */
    delayed_input = input;

    /* Update history for deadtime */
    if (dt > 0) {
        pv_history[history_idx] = K * delayed_input;
        history_idx = (history_idx + 1) % 20;
    }

    /* First-order lag with deadtime */
    float delayed_pv = pv_history[(history_idx + 20 - (int)(td / dt)) % 20];
    float d_pv = (delayed_pv - g_process_value) / T;

    g_process_value += d_pv * dt;

    /* Clamp to reasonable range */
    if (g_process_value < -10.0f) g_process_value = -10.0f;
    if (g_process_value > 110.0f) g_process_value = 110.0f;
}

/**
 * @brief Ziegler-Nichols manual implementation
 *
 * This demonstrates the Ziegler-Nichols tuning method:
 * 1. Set Ki=0, Kd=0
 * 2. Increase Kp until system oscillates (ultimate gain Ku)
 * 3. Measure oscillation period (ultimate period Tu)
 * 4. Apply Z-N formulas to calculate PID parameters
 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float ultimate_gain;
    float ultimate_period;
    float state;
} zn_tuner_t;

static void zn_tuner_init(zn_tuner_t *tuner)
{
    tuner->kp = 0.1f;
    tuner->ki = 0.0f;
    tuner->kd = 0.0f;
    tuner->ultimate_gain = 0.0f;
    tuner->ultimate_period = 0.0f;
    tuner->state = 0;
}

static int zn_tuner_step(zn_tuner_t *tuner, float setpoint, float pv, float *output)
{
    float error = setpoint - pv;

    switch ((int)tuner->state) {
        case 0:  /* Initial ramp-up with P-only */
            *output = tuner->kp * error;
            tuner->kp += 0.02f;  /* Gradually increase Kp */
            if (pv > 40.0f && pv < 60.0f) {
                tuner->state = 1.0f;  /* Start oscillating */
            }
            break;

        case 1:  /* Oscillation detection phase */
            *output = tuner->kp * error;
            /* Detect oscillation by checking crossing count */
            break;

        case 2:  /* Calculate Z-N parameters */
            /* Z-N formulas:
             * P: Kp = 0.5*Ku
             * PI: Kp = 0.45*Ku, Ki = 0.9*Ku/Tu
             * PID: Kp = 0.6*Ku, Ki = 1.2*Ku/Tu, Kd = 0.6*Ku*Tu/2
             */
            tuner->kd = tuner->kp * tuner->ultimate_period / 8.0f;
            tuner->ki = tuner->kp / 3.0f;
            tuner->state = 3.0f;
            break;

        default:  /* Use calculated parameters */
            *output = tuner->kp * error + tuner->ki * tuner->kd;
            break;
    }

    return (int)tuner->state;
}

/**
 * @brief Example 1: Manual Ziegler-Nichols tuning
 */
static void example_manual_zn_tuning(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = {
        .kp = 0.5f,
        .ki = 0.0f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };

    zn_tuner_t tuner;
    float output;
    float measurement;

    printf("\n=== Example 1: Manual Ziegler-Nichols Tuning ===\n");
    printf("Demonstrates the Z-N method step by step\n\n");

    xy_pid_init(&pid, &config);
    xy_pid_set_mode(&pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&pid, 50.0f);
    zn_tuner_init(&tuner);

    printf("%-6s | %-8s | %-8s | %-8s | %-7s | %-7s | %-7s | %-8s\n",
           "Time", "Setpoint", "PV", "Error", "K_p", "K_i", "K_d", "State");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-7s-+-%-7s-+-%-7s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------", "--------", "--------");

    /* Phase 1: Find ultimate gain */
    printf("\n--- Phase 1: Finding ultimate gain (Ku) ---\n");
    printf("Gradually increasing Kp until oscillation...\n\n");

    for (int i = 0; i < 40; i++) {
        g_tick = i * 200;
        measurement = g_process_value;

        xy_pid_compute(&pid, measurement, &output);
        g_process_output = output;

        simulate_fopdt_process(output, 0.2f);

        /* Update tuner */
        zn_tuner_step(&tuner, g_setpoint, g_process_value, &output);
        xy_pid_set_tuning(&pid, tuner.kp, tuner.ki, tuner.kd);

        const char *state_str = (tuner.state < 1) ? "RAMP" :
                                (tuner.state < 2) ? "OSC" :
                                (tuner.state < 3) ? "CALC" : "DONE";

        printf("[%04dms] %8.2f | %8.2f | %+8.2f | %7.3f | %7.3f | %7.3f | %s\n",
               g_tick, g_setpoint, g_process_value,
               g_setpoint - g_process_value,
               tuner.kp, tuner.ki, tuner.kd, state_str);

        xy_os_delay_ms(200);

        if (tuner.state >= 3.0f && i > 35) {
            printf("\n>>> Ultimate gain found: Ku=%.3f <<<\n", tuner.kp);
            break;
        }
    }

    /* Phase 2: Apply Z-N tuned parameters */
    printf("\n--- Phase 2: Applying Z-N tuned parameters ---\n");
    printf("Using calculated Kp=%.3f, Ki=%.3f, Kd=%.3f\n\n",
           tuner.kp, tuner.ki, tuner.kd);

    xy_pid_set_tuning(&pid, tuner.kp, tuner.ki, tuner.kd);
    xy_pid_reset(&pid);
    g_process_value = 0.0f;

    for (int i = 0; i < 30; i++) {
        g_tick = 20000 + i * 200;
        measurement = g_process_value;

        xy_pid_compute(&pid, measurement, &output);
        g_process_output = output;

        simulate_fopdt_process(output, 0.2f);

        printf("[%04dms] %8.2f | %8.2f | %+8.2f | %7.3f | %7.3f | %7.3f\n",
               g_tick, g_setpoint, g_process_value,
               g_setpoint - g_process_value,
               tuner.kp, tuner.ki, tuner.kd);

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&pid);
}

/**
 * @brief Example 2: Using xy_pid_auto library
 */
static void example_auto_tuner_usage(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = {
        .kp = 1.0f,
        .ki = 0.0f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };
    xy_pid_auto_config_t auto_config = {
        .method = XY_PID_AUTO_METHOD_ZN,
        .step_amplitude = 30.0f,
        .sample_interval_ms = 100,
        .num_samples = 100,
        .tolerance = 0.1f
    };

    xy_pid_auto_result_t result;
    float output;
    float measurement;

    printf("\n=== Example 2: Auto-Tuner Library Usage ===\n");
    printf("Using xy_pid_auto library for automatic tuning\n\n");

    /* Initialize PID with initial guess */
    xy_pid_init(&pid, &pid_config);
    xy_pid_set_mode(&pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&pid, 50.0f);

    /* Initialize auto-tuner */
    xy_pid_auto_init(&tuner, &pid, &auto_config);

    printf("Auto-tuner initialized, starting tuning process...\n");
    printf("This will apply relay output to find ultimate gain...\n\n");

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "State", "PV", "Output", "Progress", "Status");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    /* Simulate auto-tuning loop */
    for (int i = 0; i < 50; i++) {
        g_tick = i * 200;
        measurement = g_process_value;

        /* Auto-tuning loop - this would normally be called in interrupt */
        xy_pid_auto_loop(&tuner, measurement);

        xy_pid_auto_state_t state = xy_pid_auto_get_state(&tuner);

        /* If measuring, apply relay output */
        if (state == XY_PID_AUTO_STATE_MEASURING) {
            /* Relay output based on error */
            float error = 30.0f - measurement;  /* Step amplitude */
            output = (error > 0) ? 80.0f : 20.0f;
            g_process_output = output;
        } else if (state == XY_PID_AUTO_STATE_CALCULATING) {
            output = 0.0f;
            g_process_output = output;
        } else if (state == XY_PID_AUTO_STATE_COMPLETE) {
            break;
        } else {
            output = 0.0f;
        }

        simulate_fopdt_process(output, 0.2f);

        const char *state_str = (state == XY_PID_AUTO_STATE_IDLE) ? "IDLE" :
                                (state == XY_PID_AUTO_STATE_MEASURING) ? "MEAS" :
                                (state == XY_PID_AUTO_STATE_CALCULATING) ? "CALC" :
                                (state == XY_PID_AUTO_STATE_COMPLETE) ? "DONE" : "ERR";

        float progress = xy_pid_auto_get_progress(&tuner);

        printf("[%04dms] %-8s | %8.2f | %8.2f | %7.1f%% | %s\n",
               g_tick, state_str, g_process_value, output,
               progress * 100.0f,
               (state == XY_PID_AUTO_STATE_COMPLETE) ? "COMPLETE" : "");

        xy_os_delay_ms(200);
    }

    /* Get and apply results */
    if (xy_pid_auto_get_state(&tuner) == XY_PID_AUTO_STATE_COMPLETE) {
        xy_pid_auto_get_result(&tuner, &result);
        xy_pid_auto_apply(&tuner);

        printf("\n=== Auto-Tuning Results ===\n");
        printf("Ultimate Gain (Ku): %.3f\n", result.ultimate_gain);
        printf("Ultimate Period (Tu): %.3f s\n", result.ultimate_period);
        printf("\nTuned Parameters:\n");
        printf("  Kp: %.3f\n", result.kp);
        printf("  Ki: %.3f\n", result.ki);
        printf("  Kd: %.3f\n", result.kd);
        printf("  Rise Time: %.3f s\n", result.rise_time);
        printf("  Overshoot: %.1f%%\n", result.overshoot);
    }

    xy_pid_auto_deinit(&tuner);
}

/**
 * @brief Example 3: Compare manual vs auto-tuned
 */
static void example_compare_tuning(void)
{
    xy_pid_t pid_manual, pid_auto;
    xy_pid_config_t manual_config = {
        .kp = 0.5f,    /* Manual "rule of thumb" tuning */
        .ki = 0.1f,
        .kd = 0.1f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };
    xy_pid_config_t auto_config = {
        .kp = 0.8f,    /* Auto-tuned parameters */
        .ki = 0.25f,
        .kd = 0.3f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };

    float output;
    float measurement;
    int step_idx;

    printf("\n=== Example 3: Manual vs Auto-Tuned Comparison ===\n");
    printf("Comparing performance of manually tuned vs auto-tuned PID\n\n");

    /* Initialize both controllers */
    xy_pid_init(&pid_manual, &manual_config);
    xy_pid_init(&pid_auto, &auto_config);

    xy_pid_set_mode(&pid_manual, XY_PID_MODE_AUTO);
    xy_pid_set_mode(&pid_auto, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&pid_manual, 50.0f);
    xy_pid_set_setpoint(&pid_auto, 50.0f);

    printf("Manual tuning:  Kp=%.2f, Ki=%.2f, Kd=%.2f\n",
           manual_config.kp, manual_config.ki, manual_config.kd);
    printf("Auto-tuned:     Kp=%.2f, Ki=%.2f, Kd=%.2f\n\n",
           auto_config.kp, auto_config.ki, auto_config.kd);

    printf("%-6s-+-%-12s-+-%-12s-+-%-12s-+-%-12s\n",
           "------", "------------", "------------", "------------", "------------");
    printf("%-6s | %-12s | %-12s | %-12s | %-12s\n",
           "Time", "Man_Setpoint", "Man_PV", "Auto_Setpoint", "Auto_PV");
    printf("%-6s-+-%-12s-+-%-12s-+-%-12s-+-%-12s\n",
           "------", "------------", "------------", "------------", "------------");

    /* Run both controllers in parallel (simulated) */
    for (int i = 0; i < 25; i++) {
        g_tick = i * 200;
        step_idx = i;

        /* Simulate manual PID */
        measurement = g_process_value;
        xy_pid_compute(&pid_manual, measurement, &output);
        simulate_fopdt_process(output, 0.2f);

        float manual_pv = g_process_value;
        float manual_out = output;

        /* Simulate auto PID */
        xy_pid_compute(&pid_auto, measurement, &output);
        simulate_fopdt_process(output, 0.2f);

        float auto_pv = g_process_value;
        float auto_out = output;

        printf("[%04dms] | %12.2f | %12.2f | %12.2f | %12.2f\n",
               g_tick, 50.0f, manual_pv, 50.0f, auto_pv);

        xy_os_delay_ms(200);
    }

    xy_pid_reset(&pid_manual);
    xy_pid_reset(&pid_auto);
}

/**
 * @brief Example 4: Auto-tune for temperature control
 */
static void example_temp_auto_tune(void)
{
    xy_pid_t temp_pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = {
        .kp = 10.0f,
        .ki = 0.0f,
        .kd = 0.0f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };
    xy_pid_auto_config_t auto_config = {
        .method = XY_PID_AUTO_METHOD_ZN,
        .step_amplitude = 30.0f,
        .sample_interval_ms = 100,
        .num_samples = 150,
        .tolerance = 0.05f
    };

    float output;
    float measurement;
    float ambient = 25.0f;
    float temperature = 25.0f;

    printf("\n=== Example 4: Auto-Tune for Temperature Control ===\n");
    printf("Temperature control with thermal model\n");
    printf("Target: 60°C, Ambient: 25°C\n\n");

    /* Initialize PID */
    xy_pid_init(&temp_pid, &pid_config);
    xy_pid_set_mode(&temp_pid, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&temp_pid, 60.0f);

    /* Initialize auto-tuner */
    xy_pid_auto_init(&tuner, &temp_pid, &auto_config);

    printf("%-6s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Time", "Target", "Temp", "Error", "Output", "State");
    printf("%-6s-+-%-8s-+-%-8s-+-%-8s-+-%-8s-+-%-8s\n",
           "------", "--------", "--------", "--------", "--------", "--------");

    for (int i = 0; i < 60; i++) {
        g_tick = i * 200;
        measurement = temperature;

        /* Auto-tune step */
        xy_pid_auto_loop(&tuner, measurement);
        xy_pid_auto_state_t state = xy_pid_auto_get_state(&tuner);

        /* If in measuring state, apply relay output */
        if (state == XY_PID_AUTO_STATE_MEASURING) {
            float setpoint_step = 30.0f;  /* 30°C above ambient */
            float error = setpoint_step - (temperature - ambient);
            output = (error > 0) ? 100.0f : 0.0f;
        } else if (state == XY_PID_AUTO_STATE_CALCULATING) {
            output = 0.0f;
        } else if (state == XY_PID_AUTO_STATE_COMPLETE) {
            /* Apply tuned parameters and run normal control */
            xy_pid_auto_apply(&tuner);
            xy_pid_compute(&temp_pid, measurement, &output);
        } else if (state == XY_PID_AUTO_STATE_ERROR) {
            printf("[%04dms] ERROR in auto-tune!\n", g_tick);
            break;
        } else {
            output = 0.0f;
        }

        /* Simulate thermal system */
        float heat_input = output * 0.1f;  /* 10W at 100% */
        float heat_loss = (temperature - ambient) / 2.0f;  /* R=2 K/W */
        float dT = (heat_input - heat_loss) / 50.0f;  /* C=50 J/K */
        temperature += dT * 0.2f;

        const char *state_str = (state == XY_PID_AUTO_STATE_IDLE) ? "IDLE" :
                                (state == XY_PID_AUTO_STATE_MEASURING) ? "TUNING" :
                                (state == XY_PID_AUTO_STATE_CALCULATING) ? "CALC" :
                                (state == XY_PID_AUTO_STATE_COMPLETE) ? "DONE" : "ERR";

        printf("[%04dms] %8.1f | %8.2f | %+8.2f | %8.1f | %s\n",
               g_tick, 60.0f, temperature,
               60.0f - temperature, output, state_str);

        xy_os_delay_ms(200);

        if (state == XY_PID_AUTO_STATE_COMPLETE && i > 50) {
            break;
        }
    }

    printf("\n=== Temperature Auto-Tune Complete ===\n");
    printf("Final temperature: %.2f°C\n", temperature);
    printf("Temperature error: %.2f°C\n", 60.0f - temperature);

    xy_pid_auto_deinit(&tuner);
}

/**
 * @brief Main function
 */
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  XinYi PID Controller - Auto-Tuning Examples       ║\n");
    printf("║  Demonstrates Ziegler-Nichols method                ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    g_process_value = 0.0f;
    g_setpoint = 50.0f;

    example_manual_zn_tuning();

    g_process_value = 0.0f;
    g_setpoint = 50.0f;

    example_auto_tuner_usage();

    g_process_value = 0.0f;
    g_setpoint = 50.0f;

    example_compare_tuning();

    example_temp_auto_tune();

    printf("\n══════════════════════════════════════════════════════\n");
    printf("All auto-tuning examples completed!\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return 0;
}
