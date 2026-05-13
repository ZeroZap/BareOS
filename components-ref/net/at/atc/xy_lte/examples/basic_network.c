/**
 * @file basic_network.c
 * @brief Basic LTE Network Connection Example
 * @version 1.0.0
 * @date 2025-10-30
 * 
 * This example demonstrates:
 * - LTE module initialization
 * - SIM status checking
 * - Network registration
 * - Signal quality monitoring
 * - Basic error handling
 */

#include "../xy_lte.h"
#include "../../../xy_clib/xy_stdio.h"
#include "../../../osal/xy_os.h"

/* Global handle */
static lte_handle_t g_lte_handle = NULL;

/**
 * @brief Network status callback
 */
static void on_network_status_changed(const lte_network_reg_t *status, void *user_data)
{
    const char *status_names[] = {
        "NOT_REGISTERED",
        "REGISTERED_HOME",
        "SEARCHING",
        "DENIED",
        "UNKNOWN",
        "REGISTERED_ROAMING"
    };
    
    const char *rat_names[] = {
        "GSM", "GSM_COMPACT", "UTRAN", "GSM+EGPRS",
        "UTRAN+HSDPA", "UTRAN+HSUPA", "UTRAN+HSPA",
        "LTE", "EC-GSM-IoT", "NB-IoT", "Cat-M1"
    };
    
    xy_printf("[Network] Status: %s, RAT: %s\n",
              status_names[status->status],
              rat_names[status->access_tech]);
    
    if (status->status == LTE_REG_REGISTERED_HOME || 
        status->status == LTE_REG_REGISTERED_ROAMING) {
        xy_printf("[Network] Cell ID: 0x%08X, LAC/TAC: 0x%04X\n",
                  status->ci, status->lac ? status->lac : status->tac);
    } else if (status->status == LTE_REG_DENIED) {
        xy_printf("[Network] Reject cause: %d\n", status->reject_cause);
    }
}

/**
 * @brief Signal quality callback
 */
static void on_signal_quality_updated(const lte_signal_quality_t *quality, void *user_data)
{
    xy_printf("[Signal] RSSI: %d dBm, RSRP: %d dBm, RSRQ: %d dB, Bars: %d/5\n",
              quality->rssi, quality->rsrp, quality->rsrq, quality->bars);
}

/**
 * @brief SIM status callback
 */
static void on_sim_status_changed(lte_sim_status_t status, void *user_data)
{
    const char *sim_status_names[] = {
        "READY", "NOT_INSERTED", "PIN_REQUIRED", "PUK_REQUIRED",
        "PIN2_REQUIRED", "PUK2_REQUIRED", "NETWORK_LOCKED", "ERROR"
    };
    
    xy_printf("[SIM] Status: %s\n", sim_status_names[status]);
}

/**
 * @brief Check and handle SIM status
 */
static lte_error_t handle_sim_status(void)
{
    lte_sim_info_t sim_info;
    lte_error_t ret;
    
    ret = lte_sim_get_status(g_lte_handle, &sim_info);
    if (ret != LTE_OK) {
        xy_printf("[ERROR] Failed to get SIM status: %s\n", lte_error_string(ret));
        return ret;
    }
    
    switch (sim_info.status) {
        case LTE_SIM_READY:
            xy_printf("[SIM] Ready\n");
            xy_printf("[SIM] IMSI: %s\n", sim_info.imsi);
            xy_printf("[SIM] ICCID: %s\n", sim_info.iccid);
            return LTE_OK;
            
        case LTE_SIM_PIN_REQUIRED:
            xy_printf("[SIM] PIN required (%d attempts remaining)\n", 
                      sim_info.pin_retry_count);
            // In real application, get PIN from secure storage or user input
            ret = lte_sim_enter_pin(g_lte_handle, "1234");
            if (ret == LTE_OK) {
                xy_printf("[SIM] PIN accepted\n");
                return LTE_OK;
            } else {
                xy_printf("[ERROR] PIN rejected: %s\n", lte_error_string(ret));
                return ret;
            }
            
        case LTE_SIM_PUK_REQUIRED:
            xy_printf("[ERROR] SIM locked, PUK required (%d attempts remaining)\n",
                      sim_info.puk_retry_count);
            // In real application, get PUK from user
            return LTE_ERROR_SIM_PUK;
            
        case LTE_SIM_NOT_INSERTED:
            xy_printf("[ERROR] SIM not inserted\n");
            return LTE_ERROR_NO_SIM;
            
        default:
            xy_printf("[ERROR] SIM failure: %d\n", sim_info.status);
            return LTE_ERROR_SIM_FAILURE;
    }
}

/**
 * @brief Display device information
 */
static void display_device_info(void)
{
    lte_device_info_t info;
    
    if (lte_module_get_device_info(g_lte_handle, &info) == LTE_OK) {
        xy_printf("\n========== Device Information ==========\n");
        xy_printf("IMEI:         %s\n", info.imei);
        xy_printf("Manufacturer: %s\n", info.manufacturer);
        xy_printf("Model:        %s\n", info.model);
        xy_printf("Firmware:     %s\n", info.firmware_version);
        xy_printf("========================================\n\n");
    }
}

/**
 * @brief Main application task
 */
static void lte_app_task(void *arg)
{
    lte_error_t ret;
    
    xy_printf("\n=== LTE Basic Network Connection Example ===\n\n");
    
    /* Step 1: Initialize LTE module */
    xy_printf("[1/5] Initializing LTE module...\n");
    
    lte_config_t config = {
        .uart_port = 1,
        .baudrate = 115200,
        .apn = "internet",          // Change to your carrier's APN
        .pin_code = "",             // Will handle PIN separately
        .auto_register = false,     // Manual control for demo
        .preferred_rat = LTE_RAT_AUTO,
        .network_search_timeout = 180000, // 3 minutes
        .response_timeout = 5000,
        .max_retry = 3
    };
    
    g_lte_handle = lte_module_init(&config);
    if (g_lte_handle == NULL) {
        xy_printf("[ERROR] LTE module initialization failed!\n");
        return;
    }
    
    xy_printf("[OK] LTE module initialized\n");
    
    /* Step 2: Display device information */
    xy_printf("\n[2/5] Retrieving device information...\n");
    display_device_info();
    
    /* Step 3: Check SIM status and handle PIN if needed */
    xy_printf("[3/5] Checking SIM status...\n");
    ret = handle_sim_status();
    if (ret != LTE_OK) {
        xy_printf("[ERROR] SIM not ready, cannot proceed\n");
        goto cleanup;
    }
    
    /* Step 4: Register callbacks */
    xy_printf("\n[4/5] Registering callbacks...\n");
    lte_network_register_callback(g_lte_handle, on_network_status_changed, NULL);
    lte_sim_register_callback(g_lte_handle, on_sim_status_changed, NULL);
    xy_printf("[OK] Callbacks registered\n");
    
    /* Step 5: Register to network */
    xy_printf("\n[5/5] Registering to network...\n");
    ret = lte_network_register(g_lte_handle);
    if (ret != LTE_OK) {
        xy_printf("[ERROR] Failed to start network registration: %s\n", 
                  lte_error_string(ret));
        goto cleanup;
    }
    
    /* Wait for registration (with timeout) */
    xy_printf("[INFO] Waiting for network registration (max 180s)...\n");
    ret = lte_network_wait_registered(g_lte_handle, 180000);
    
    if (ret == LTE_OK) {
        xy_printf("\n[SUCCESS] Registered to network!\n\n");
        
        /* Get current network status */
        lte_network_reg_t status;
        if (lte_network_get_status(g_lte_handle, &status) == LTE_OK) {
            on_network_status_changed(&status, NULL);
        }
        
        /* Start signal quality monitoring (every 10 seconds) */
        xy_printf("\n[INFO] Starting signal quality monitoring...\n");
        lte_signal_start_monitor(g_lte_handle, on_signal_quality_updated, 10000, NULL);
        
        /* Get initial signal quality */
        lte_signal_quality_t quality;
        if (lte_signal_get_quality(g_lte_handle, &quality) == LTE_OK) {
            on_signal_quality_updated(&quality, NULL);
        }
        
        /* Get current operator */
        lte_operator_info_t operator;
        if (lte_operator_get_current(g_lte_handle, &operator) == LTE_OK) {
            xy_printf("\n[Operator] %s (%s)\n", 
                      operator.operator_long, operator.operator_numeric);
        }
        
        xy_printf("\n[INFO] System running, monitoring network...\n");
        xy_printf("[INFO] Press Ctrl+C to exit\n\n");
        
        /* Keep running and processing events */
        while (1) {
            lte_module_process(g_lte_handle);
            xy_os_delay(50); // Process every 50ms
        }
        
    } else if (ret == LTE_ERROR_TIMEOUT) {
        xy_printf("\n[ERROR] Network registration timeout\n");
        xy_printf("[INFO] Possible causes:\n");
        xy_printf("  - No network coverage\n");
        xy_printf("  - SIM not activated\n");
        xy_printf("  - Incorrect APN\n");
        xy_printf("  - Antenna not connected\n");
    } else {
        xy_printf("\n[ERROR] Network registration failed: %s\n", lte_error_string(ret));
    }
    
cleanup:
    /* Cleanup */
    if (g_lte_handle) {
        xy_printf("\n[INFO] Shutting down...\n");
        lte_signal_stop_monitor(g_lte_handle);
        lte_network_deregister(g_lte_handle);
        lte_module_deinit(g_lte_handle);
    }
    
    xy_printf("[INFO] Example finished\n");
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize OS kernel (if using RTOS) */
    #ifdef XY_USE_RTOS
    xy_os_kernel_init();
    
    /* Create LTE application task */
    xy_os_thread_attr_t attr = {
        .name = "lte_app",
        .stack_size = 4096,
        .priority = XY_OS_PRIORITY_NORMAL
    };
    
    xy_os_thread_new(lte_app_task, NULL, &attr);
    
    /* Start OS scheduler */
    xy_os_kernel_start();
    #else
    /* Bare-metal: run directly */
    lte_app_task(NULL);
    #endif
    
    return 0;
}
