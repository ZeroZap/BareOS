/**
 * @file main.c
 * @author N32cube
 */
 //!!!!!!!!!!!!!!!!NOTE!!!!!!!!!!!!!!!
 // Code cannot be added between /* NTFx CODE START xxxxx*/ and /* NTFx CODE END xxxxx*/
/* NTFx CODE START Include*/
#include "main.h"
#include "plb_n32_flash_fee.h"
#include "plb_n32_at.h"
#include "xy_log.h"
#include "xy_pm.h"
#include "xy_hal.h"
#include "xy_tick.h"
#include <stdio.h>
#include <stdint.h>
/* NTFx CODE END Include*/

void plb_algo_demo_run(void);
extern __IO uint32_t mwTick;

#ifndef PLB_N32_ENABLE_TICKLESS
#define PLB_N32_ENABLE_TICKLESS 0
#endif

#ifndef PLB_N32_ENABLE_PM_LOCK_TEST
#define PLB_N32_ENABLE_PM_LOCK_TEST 0
#endif

#define PLB_N32_UART5_RX_WINDOW_MS 1000u

#ifndef PLB_N32_PM_LOG_INTERVAL_MS
#define PLB_N32_PM_LOG_INTERVAL_MS 60000u
#endif

static uint32_t s_next_heartbeat_ms;
static volatile bool s_lptim_selftest_done;
static uint32_t s_lptim_selftest_actual_ms;
static uint32_t s_lptim_selftest_start_ms;
static uint32_t s_lptim_selftest_count_ms;
static int s_lptim_selftest_start_rc;

static void plb_n32_lptim_selftest_cb(void *timer, void *user)
{
    (void)timer;
    (void)user;
    s_lptim_selftest_done = true;
}

static bool plb_n32_lptim_selftest(void)
{
    uint32_t start = mwTick;
    uint32_t started;

    s_lptim_selftest_done = false;
    s_lptim_selftest_start_rc = xy_hal_lptimer_start(
        LPTIM, 100u, plb_n32_lptim_selftest_cb, NULL);
    if (s_lptim_selftest_start_rc != XY_HAL_OK) {
        s_lptim_selftest_start_ms = (uint32_t)(mwTick - start);
        return false;
    }
    started = mwTick;
    s_lptim_selftest_start_ms = (uint32_t)(started - start);
    while (!s_lptim_selftest_done && (uint32_t)(mwTick - start) < 500u) {
        IWDG_ReloadKey();
    }
    s_lptim_selftest_actual_ms = (uint32_t)(mwTick - start);
    s_lptim_selftest_count_ms = (uint32_t)(mwTick - started);
    (void)xy_hal_lptimer_stop(LPTIM);
    return s_lptim_selftest_done;
}

static xy_tick_t plb_n32_pm_timeout(void *arg)
{
    uint32_t now_ms = xy_tick_now_ms();

    (void)arg;
    if ((int32_t)(now_ms - s_next_heartbeat_ms) >= 0) {
        return 0u;
    }
    return xy_tick_from_ms(s_next_heartbeat_ms - now_ms);
}

static void plb_n32_pm_init(void)
{
    xy_pm_config_t pm_cfg;
    xy_hal_power_policy_t policy;

    policy.deepest_mode = XY_HAL_POWER_STOP;
    policy.wake_sources = XY_HAL_WAKE_LPTIMER | XY_HAL_WAKE_GPIO | XY_HAL_WAKE_UART;
    policy.min_residency_ms = 5u;
    policy.max_latency_ms = 0u;
    policy.keep_sram = true;
    policy.keep_rtc = true;
    (void)xy_hal_power_configure(&policy);

    pm_cfg.lptimer = LPTIM;
    pm_cfg.deepest_mode = XY_HAL_POWER_STOP;
    pm_cfg.wake_sources = XY_HAL_WAKE_LPTIMER | XY_HAL_WAKE_GPIO | XY_HAL_WAKE_UART;
    pm_cfg.min_sleep_ms = 5u;
    pm_cfg.max_sleep_ms = n32_lptim_max_timeout_ms();
    xy_pm_init(&pm_cfg);
    (void)xy_pm_register_timeout(plb_n32_pm_timeout, NULL);
}

#if PLB_N32_ENABLE_PM_LOCK_TEST
static void plb_n32_tickless_lock_test(uint32_t now_ms)
{
    static uint8_t stage;

    if (stage == 0u && now_ms >= 3000u) {
        (void)xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP);
        (void)xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP);
        stage = 1u;
        xy_log_i("PLB-N32 PM test STOP lock acquired twice count=%u",
                 (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP));
    } else if (stage == 1u && now_ms >= 6000u) {
        (void)xy_pm_release_lock(XY_HAL_PM_LOCK_STOP);
        stage = 2u;
        xy_log_i("PLB-N32 PM test STOP lock released once count=%u",
                 (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP));
    } else if (stage == 2u && now_ms >= 9000u) {
        (void)xy_pm_release_lock(XY_HAL_PM_LOCK_STOP);
        stage = 3u;
        xy_log_i("PLB-N32 PM test STOP lock fully released count=%u",
                 (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP));
    }
}
#endif

static void plb_log_reset_flags(void)
{
    xy_log_i("PLB-N32 reset flags: PIN=%u POR=%u SFTRST=%u IWDG=%u WWDG=%u LPWR=%u",
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_PINRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_PORRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_SFTRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_IWDGRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_WWDGRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_LPWRRSTF));
    RCC_ClrFlag();
}

/**
 * @brief  Main program.
 */
int main(void)
{
    uint32_t next_heartbeat;
    uint32_t boot_count = 0;
    uint32_t last_uart5_rx_count = 0u;
    uint32_t last_uart5_io_wake_count = 0u;
    uint32_t uart5_rx_deadline_ms = 0u;
    bool uart5_rx_window_open = false;
    plb_n32_server_endpoint_t server;
    int boot_count_update;
    bool pm_ready = false;
#if PLB_N32_ENABLE_AT_SELFTEST
    bool at_ready = false;
#endif

    /* NTFx CODE START Config*/
    RCC_Configuration();
    GPIO_Configuration();
#if 0
    NVIC_Configuration();
#endif
    USART_Configuration();
    xy_log_init();
    xy_log_i("PLB-N32 UART4 log ready");
    xy_log_i("%s", PLB_N32_VERSION_STR);
    n32_uart5_secboot_init();
    xy_log_i("PLB-N32 UART5 reserved for sec-boot development");
    xy_log_i("PLB-N32 STOP2 PB5/EXTI5 IO wake, UART5 115200 RX window=%u ms ready",
             (unsigned int)PLB_N32_UART5_RX_WINDOW_MS);
    plb_log_reset_flags();
    IWDG_Configuration();
    /* USB is unused by this validation image and must not wake STOP2. */
    if (LPTIM_Configuration()) {
        if (plb_n32_lptim_selftest()) {
            plb_n32_pm_init();
            pm_ready = true;
            xy_log_i("PLB-N32 LPTIM clock=%u Hz src=%u selftest=OK requested=100 start=%u count=%u total=%u PM ready",
                     (unsigned int)n32_lptim_clock_hz(),
                     (unsigned int)RCC_GetLPTIMClkSrc(),
                     (unsigned int)s_lptim_selftest_start_ms,
                     (unsigned int)s_lptim_selftest_count_ms,
                     (unsigned int)s_lptim_selftest_actual_ms);
        } else {
            xy_log_w("PLB-N32 LPTIM selftest failed rc=%d start=%u core=%u src=%u RDCTRL=%x CTRL=%x CFG=%x ARR=%x CNT=%x INTSTS=%x; tickless disabled",
                     s_lptim_selftest_start_rc,
                     (unsigned int)s_lptim_selftest_start_ms,
                     (unsigned int)SystemCoreClock,
                     (unsigned int)RCC_GetLPTIMClkSrc(),
                     (unsigned int)RCC->RDCTRL,
                     (unsigned int)LPTIM->CTRL,
                     (unsigned int)LPTIM->CFG,
                     (unsigned int)LPTIM->ARR,
                     (unsigned int)LPTIM->CNT,
                     (unsigned int)LPTIM->INTSTS);
        }
    } else {
        xy_log_w("PLB-N32 LPTIM init failed; tickless disabled");
    }
    if (plb_n32_at_init()) {
#if PLB_N32_ENABLE_AT_SELFTEST
        at_ready = true;
#endif
        if (pm_ready) {
            (void)xy_pm_register_sleep_check(at_obj_pm_can_sleep,
                                             plb_n32_at_obj());
        }
#if PLB_N32_ENABLE_AT_SELFTEST
        xy_log_i("PLB-N32 AT client ready on UART5, selftest enabled");
#else
        xy_log_i("PLB-N32 AT client ready on UART5, automatic commands disabled");
#endif
    } else {
        xy_log_w("PLB-N32 AT client init failed");
    }
    xy_log_i("PLB-N32 FEE base=%x size=%x init=%d",
             (unsigned int)PLB_N32_FEE_BASE_ADDR,
             (unsigned int)PLB_N32_FEE_TOTAL_SIZE,
             (int)plb_n32_fee_init());
    xy_log_i("PLB-N32 EEPROM base=%x size=%x init=%d",
             (unsigned int)PLB_N32_EEPROM_BASE_ADDR,
             (unsigned int)PLB_N32_EEPROM_TOTAL_SIZE,
             (int)plb_n32_eeprom_init());
    boot_count_update = (int)plb_n32_boot_count_update(&boot_count);
    xy_log_i("PLB-N32 boot_count=%u update=%d",
             (unsigned int)boot_count,
             boot_count_update);
    xy_log_i("PLB-N32 secboot confirm=%d",
             (int)plb_n32_secboot_confirm_app());
#if PLB_N32_ENABLE_AT_SELFTEST
    if (at_ready && !plb_n32_at_selftest_start()) {
        xy_log_w("PLB-N32 AT selftest start failed");
    }
#endif
    if (plb_n32_server_endpoint_load(&server) == EFLASH_OK) {
        xy_log_i("PLB-N32 server ip=%u.%u.%u.%u port=%u",
                 (unsigned int)server.ip[0],
                 (unsigned int)server.ip[1],
                 (unsigned int)server.ip[2],
                 (unsigned int)server.ip[3],
                 (unsigned int)server.port);
    }
    plb_algo_demo_run();
    /* Keep the algorithm validation image minimal. Enabling unused generated
     * peripherals here can introduce pending interrupts or board-level resets. */
#if 0
    RTC_Configuration();
    ADC_Configuration();
    SPI_Configuration();
    I2C_Configuration();
    DMA_Configuration();
#endif
    /* NTFx CODE END Config*/
    xy_log_i("PLB-N32 main loop start");
    next_heartbeat = xy_tick_now_ms() + PLB_N32_PM_LOG_INTERVAL_MS;
    s_next_heartbeat_ms = next_heartbeat;
    while(1)
    {
        uint32_t now_ms = xy_tick_now_ms();

        IWDG_ReloadKey();
        plb_n32_at_process();
        n32_uart5_secboot_poll();
        if (g_n32_uart5_io_wake_count != last_uart5_io_wake_count) {
            last_uart5_io_wake_count = g_n32_uart5_io_wake_count;
            uart5_rx_deadline_ms = now_ms + PLB_N32_UART5_RX_WINDOW_MS;
            if (!uart5_rx_window_open &&
                xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_OK) {
                uart5_rx_window_open = true;
            }
            xy_log_i("PLB-N32 UART5 IO wake count=%u RX window open=%u",
                     (unsigned int)g_n32_uart5_io_wake_count,
                     (unsigned int)uart5_rx_window_open);
        }
        if (g_n32_uart5_rx_count != last_uart5_rx_count) {
            last_uart5_rx_count = g_n32_uart5_rx_count;
            xy_log_i("PLB-N32 UART5 RX count=%u rb=%u drop=%u last=%x",
                     (unsigned int)g_n32_uart5_rx_count,
                     (unsigned int)g_n32_uart5_rb_pending,
                     (unsigned int)g_n32_uart5_rx_drop_count,
                     (unsigned int)g_n32_uart5_last_rx);
        }
        if (uart5_rx_window_open &&
            (int32_t)(now_ms - uart5_rx_deadline_ms) >= 0) {
            uint32_t key = xy_hal_irq_save();

            EXTI_ClrITPendBit(EXTI_LINE5);
            NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
            EXTI->IMASK |= EXTI_LINE5;
            xy_hal_irq_restore(key);
            (void)xy_pm_release_lock(XY_HAL_PM_LOCK_STOP);
            uart5_rx_window_open = false;
            xy_log_i("PLB-N32 UART5 RX window closed, STOP2 allowed");
        }
#if PLB_N32_ENABLE_PM_LOCK_TEST
        plb_n32_tickless_lock_test(now_ms);
#endif
        if ((int32_t)(now_ms - next_heartbeat) >= 0) {
            xy_pm_stats_t stats;

            xy_pm_get_stats(&stats);
            xy_log_i("PLB-N32 PM tick=%u idle=%u sleep=%u shallow=%u abort=%u plan=%u elapsed=%u total=%u max=%u early=%u wake=%x uart=%u gpio=%u lptim=%u rtc=%u wdg=%u unknown=%u locks=%u/%u/%u/%u",
                     (unsigned int)now_ms,
                     (unsigned int)stats.idle_calls,
                     (unsigned int)stats.sleep_count,
                     (unsigned int)stats.shallow_sleep_count,
                     (unsigned int)stats.abort_count,
                     (unsigned int)stats.last_planned_ms,
                     (unsigned int)stats.last_elapsed_ms,
                     (unsigned int)stats.total_deep_sleep_ms,
                     (unsigned int)stats.max_deep_sleep_ms,
                     (unsigned int)stats.early_wake_count,
                     (unsigned int)stats.last_wake_sources,
                     (unsigned int)stats.uart_wake_count,
                     (unsigned int)stats.gpio_wake_count,
                     (unsigned int)stats.lptimer_wake_count,
                     (unsigned int)stats.rtc_wake_count,
                     (unsigned int)stats.wdg_wake_count,
                     (unsigned int)stats.unknown_wake_count,
                     (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_CPU),
                     (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_SLEEP),
                     (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP),
                     (unsigned int)xy_pm_get_lock_count(XY_HAL_PM_LOCK_STANDBY));
            xy_log_i("PLB-N32 UART4 heartbeat UART5 io=%u rx=%u tx=%u rb=%u drop=%u last=%x",
                     (unsigned int)g_n32_uart5_io_wake_count,
                     (unsigned int)g_n32_uart5_rx_count,
                     (unsigned int)g_n32_uart5_tx_count,
                     (unsigned int)g_n32_uart5_rb_pending,
                     (unsigned int)g_n32_uart5_rx_drop_count,
                     (unsigned int)g_n32_uart5_last_rx);
            next_heartbeat += PLB_N32_PM_LOG_INTERVAL_MS;
            s_next_heartbeat_ms = next_heartbeat;
        }
#if PLB_N32_ENABLE_TICKLESS
        if (pm_ready) {
            xy_pm_tickless_idle();
        }
#endif
    }
}


