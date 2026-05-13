/**
 * @file xy_os_sys.h
 * @brief XY OSAL System Functions
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_OS_SYS_H
#define XY_OS_SYS_H

#include "xy_os_types.h"
#include "xy_os_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System tick frequency (Hz)
 */
#ifndef XY_OS_TICK_FREQ
#define XY_OS_TICK_FREQ 1000
#endif

/**
 * @brief System tick period (ms)
 */
#define XY_OS_TICK_PERIOD_MS (1000 / XY_OS_TICK_FREQ)

/* System Information Structures */

/**
 * @brief System information structure
 */
typedef struct {
    uint32_t cpu_freq_hz;      /**< CPU frequency in Hz */
    uint32_t tick_freq_hz;     /**< Tick frequency in Hz */
    uint32_t heap_total;       /**< Total heap size */
    uint32_t heap_free;        /**< Free heap size */
    uint32_t stack_used;       /**< Used stack size */
    uint32_t stack_total;      /**< Total stack size */
} xy_os_sys_info_t;

/**
 * @brief System power mode
 */
typedef enum {
    XY_OS_POWER_RUN = 0,       /**< Run mode */
    XY_OS_POWER_SLEEP,         /**< Sleep mode */
    XY_OS_POWER_STOP,          /**< Stop mode */
    XY_OS_POWER_STANDBY,       /**< Standby mode */
    XY_OS_POWER_SHUTDOWN,      /**< Shutdown mode */
} xy_os_power_mode_t;

/**
 * @brief System reset reason
 */
typedef enum {
    XY_OS_RESET_UNKNOWN = 0,   /**< Unknown reset */
    XY_OS_RESET_POWER_ON,      /**< Power-on reset */
    XY_OS_RESET_PIN,           /**< Pin reset */
    XY_OS_RESET_WATCHDOG,      /**< Watchdog reset */
    XY_OS_RESET_SOFTWARE,      /**< Software reset */
    XY_OS_RESET_BROWNOUT,      /**< Brown-out reset */
} xy_os_reset_reason_t;

/* System Functions */

/**
 * @brief Get system information
 * @param[out] info System information structure
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_info(xy_os_sys_info_t *info);

/**
 * @brief Get current system tick count
 * @return Current tick count
 */
uint32_t xy_os_sys_get_tick_count(void);

/**
 * @brief Get system tick frequency
 * @return Tick frequency in Hz
 */
uint32_t xy_os_sys_get_tick_freq(void);

/**
 * @brief Get CPU frequency
 * @return CPU frequency in Hz
 */
uint32_t xy_os_sys_get_cpu_freq(void);

/**
 * @brief Enter sleep mode
 * @param mode Power mode to enter
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_power_mode(xy_os_power_mode_t mode);

/**
 * @brief Get reset reason
 * @return Reset reason
 */
xy_os_reset_reason_t xy_os_sys_get_reset_reason(void);

/**
 * @brief System reset
 */
void xy_os_sys_reset(void);

/**
 * @brief Get system uptime in milliseconds
 * @return Uptime in milliseconds
 */
uint64_t xy_os_sys_uptime_ms(void);

/**
 * @brief Get system uptime in seconds
 * @return Uptime in seconds
 */
uint32_t xy_os_sys_uptime_sec(void);

/**
 * @brief Get heap information
 * @param[out] total Total heap size
 * @param[out] free Free heap size
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_heap_info(uint32_t *total, uint32_t *free);

/**
 * @brief Get stack usage for current thread
 * @param[out] used Stack used in bytes
 * @param[out] total Total stack in bytes
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_stack_usage(uint32_t *used, uint32_t *total);

/**
 * @brief Get number of running threads
 * @return Number of running threads
 */
uint32_t xy_os_sys_get_thread_count(void);

/**
 * @brief Get thread list
 * @param[out] thread_list Array to store thread IDs
 * @param[in] max_count Maximum number of threads to return
 * @return Number of threads returned
 */
uint32_t xy_os_sys_get_thread_list(xy_os_thread_id_t *thread_list, uint32_t max_count);

/**
 * @brief Enable interrupts
 */
void xy_os_sys_enable_irq(void);

/**
 * @brief Disable interrupts
 */
void xy_os_sys_disable_irq(void);

/**
 * @brief Critical section enter
 */
void xy_os_sys_enter_critical(void);

/**
 * @brief Critical section exit
 */
void xy_os_sys_exit_critical(void);

/**
 * @brief Enter critical section and return previous state
 * @return Previous interrupt state
 */
uint32_t xy_os_sys_save_irq_state(void);

/**
 * @brief Restore interrupt state
 * @param state State to restore
 */
void xy_os_sys_restore_irq_state(uint32_t state);

/**
 * @brief Memory barrier
 */
void xy_os_sys_memory_barrier(void);

/**
 * @brief Instruction barrier
 */
void xy_os_sys_instruction_barrier(void);

/**
 * @brief Data cache clean
 */
void xy_os_sys_dcache_clean(void);

/**
 * @brief Data cache invalidate
 */
void xy_os_sys_dcache_invalidate(void);

/**
 * @brief Data cache clean and invalidate
 */
void xy_os_sys_dcache_clean_invalidate(void);

/**
 * @brief Instruction cache invalidate
 */
void xy_os_sys_icache_invalidate(void);

/**
 * @brief Enable FPU (if available)
 */
void xy_os_sys_enable_fpu(void);

/**
 * @brief Disable FPU (if available)
 */
void xy_os_sys_disable_fpu(void);

/**
 * @brief Get unique device ID
 * @param[out] id Buffer to store ID (12 bytes recommended)
 * @param[in] size Size of buffer
 * @return Number of bytes written
 */
uint32_t xy_os_sys_get_unique_id(uint8_t *id, uint32_t size);

/**
 * @brief Get device serial number
 * @param[out] serial Buffer to store serial number
 * @param[in] size Size of buffer
 * @return Number of bytes written
 */
uint32_t xy_os_sys_get_serial_number(char *serial, uint32_t size);

/**
 * @brief Get chip revision
 * @return Chip revision
 */
uint32_t xy_os_sys_get_chip_revision(void);

/**
 * @brief Get chip family
 * @return Chip family identifier
 */
uint32_t xy_os_sys_get_chip_family(void);

/**
 * @brief Get chip package
 * @return Chip package identifier
 */
uint32_t xy_os_sys_get_chip_package(void);

/**
 * @brief Get flash size
 * @return Flash size in KB
 */
uint32_t xy_os_sys_get_flash_size(void);

/**
 * @brief Get RAM size
 * @return RAM size in KB
 */
uint32_t xy_os_sys_get_ram_size(void);

/**
 * @brief Get peripheral count
 * @return Number of available peripherals
 */
uint32_t xy_os_sys_get_peripheral_count(void);

/**
 * @brief Get peripheral clock frequency
 * @param peripheral_id Peripheral ID
 * @return Clock frequency in Hz
 */
uint32_t xy_os_sys_get_peripheral_clock(uint32_t peripheral_id);

/**
 * @brief Enable peripheral clock
 * @param peripheral_id Peripheral ID
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_peripheral_clock(uint32_t peripheral_id);

/**
 * @brief Disable peripheral clock
 * @param peripheral_id Peripheral ID
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_disable_peripheral_clock(uint32_t peripheral_id);

/**
 * @brief Get system load average
 * @param[out] load Average load (0-10000 = 0.00% - 100.00%)
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_load_average(uint32_t *load);

/**
 * @brief Get system temperature (if available)
 * @return Temperature in 0.01°C units
 */
int32_t xy_os_sys_get_temperature(void);

/**
 * @brief Get system voltage (if available)
 * @return Voltage in mV
 */
uint32_t xy_os_sys_get_voltage(void);

/**
 * @brief Get system current (if available)
 * @return Current in mA
 */
uint32_t xy_os_sys_get_current(void);

/**
 * @brief Get system power consumption (if available)
 * @return Power in mW
 */
uint32_t xy_os_sys_get_power(void);

/**
 * @brief Set system performance level
 * @param level Performance level (0-100)
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_performance_level(uint8_t level);

/**
 * @brief Get system performance level
 * @return Performance level (0-100)
 */
uint8_t xy_os_sys_get_performance_level(void);

/**
 * @brief Enter low power mode with wake-up sources
 * @param mode Power mode
 * @param wake_sources Wake-up sources bitmap
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enter_low_power(xy_os_power_mode_t mode, uint32_t wake_sources);

/**
 * @brief Configure wake-up source
 * @param source Wake-up source ID
 * @param enable Enable/disable wake-up source
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_configure_wakeup_source(uint32_t source, uint8_t enable);

/**
 * @brief Get system error code
 * @return Last system error code
 */
xy_os_error_t xy_os_sys_get_last_error(void);

/**
 * @brief Clear system error code
 */
void xy_os_sys_clear_last_error(void);

/**
 * @brief Get system status
 * @return System status string
 */
const char *xy_os_sys_get_status(void);

/**
 * @brief System diagnostic
 * @param[out] diag Diagnostic information
 * @param[in] size Size of diagnostic buffer
 * @return Number of bytes written
 */
uint32_t xy_os_sys_diagnostic(char *diag, uint32_t size);

/**
 * @brief Get system statistics
 * @param[out] stats Statistics buffer
 * @param[in] size Size of statistics buffer
 * @return Number of bytes written
 */
uint32_t xy_os_sys_get_statistics(char *stats, uint32_t size);

/**
 * @brief System watchdog feed
 */
void xy_os_sys_watchdog_feed(void);

/**
 * @brief Get system watchdog timeout
 * @return Watchdog timeout in milliseconds
 */
uint32_t xy_os_sys_get_watchdog_timeout(void);

/**
 * @brief Set system watchdog timeout
 * @param timeout_ms Timeout in milliseconds
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_watchdog_timeout(uint32_t timeout_ms);

/**
 * @brief Enable system watchdog
 * @param timeout_ms Timeout in milliseconds
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_watchdog(uint32_t timeout_ms);

/**
 * @brief Disable system watchdog
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_disable_watchdog(void);

/**
 * @brief Get system tick resolution
 * @return Tick resolution in nanoseconds
 */
uint32_t xy_os_sys_get_tick_resolution(void);

/**
 * @brief Get system timer count
 * @return Timer count value
 */
uint64_t xy_os_sys_get_timer_count(void);

/**
 * @brief Get system timer frequency
 * @return Timer frequency in Hz
 */
uint32_t xy_os_sys_get_timer_freq(void);

/**
 * @brief Set system timer callback
 * @param callback Timer callback function
 * @param period Period in ticks
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_timer_callback(xy_os_callback_t callback, uint32_t period);

/**
 * @brief Get system tick since boot
 * @return Tick count since boot
 */
uint64_t xy_os_sys_get_tick_since_boot(void);

/**
 * @brief Get system uptime in ticks
 * @return Uptime in ticks
 */
static inline uint64_t xy_os_sys_uptime_ticks(void)
{
    return xy_os_sys_get_tick_since_boot();
}

/**
 * @brief Convert milliseconds to system ticks
 * @param ms Milliseconds
 * @return Equivalent ticks
 */
static inline uint32_t xy_os_sys_ms_to_ticks(uint32_t ms)
{
    return (ms * XY_OS_TICK_FREQ) / 1000;
}

/**
 * @brief Convert system ticks to milliseconds
 * @param ticks System ticks
 * @return Equivalent milliseconds
 */
static inline uint32_t xy_os_sys_ticks_to_ms(uint32_t ticks)
{
    return (ticks * 1000) / XY_OS_TICK_FREQ;
}

/**
 * @brief Get current thread ID safely
 * @return Current thread ID or NULL if not in thread context
 */
xy_os_thread_id_t xy_os_sys_get_current_thread_id(void);

/**
 * @brief Check if currently in interrupt context
 * @return 1 if in interrupt, 0 otherwise
 */
int xy_os_sys_in_isr(void);

/**
 * @brief Get interrupt nesting level
 * @return Nesting level
 */
uint32_t xy_os_sys_get_isr_nest_level(void);

/**
 * @brief Get system memory map
 * @param[out] map Memory map structure
 * @param[in] size Size of map structure
 * @return Number of entries filled
 */
uint32_t xy_os_sys_get_memory_map(void *map, uint32_t size);

/**
 * @brief Get system capabilities
 * @return Capability bitmap
 */
uint32_t xy_os_sys_get_capabilities(void);

/**
 * @brief Set system capability
 * @param capability Capability ID
 * @param enable Enable/disable
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_capability(uint32_t capability, uint8_t enable);

/**
 * @brief Get system version
 * @return Version encoded as major.minor.patch
 */
uint32_t xy_os_sys_get_version(void);

/**
 * @brief Get system build date
 * @return Build date string
 */
const char *xy_os_sys_get_build_date(void);

/**
 * @brief Get system build time
 * @return Build time string
 */
const char *xy_os_sys_get_build_time(void);

/**
 * @brief Get system configuration
 * @return Configuration string
 */
const char *xy_os_sys_get_config(void);

/**
 * @brief Get system features
 * @return Feature bitmap
 */
uint32_t xy_os_sys_get_features(void);

/**
 * @brief Get system limits
 * @param[out] limits Limits structure
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_limits(void *limits);

/**
 * @brief Get system counters
 * @param[out] counters Counters structure
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_counters(void *counters);

/**
 * @brief Reset system counters
 */
void xy_os_sys_reset_counters(void);

/**
 * @brief Get system event count
 * @param event Event type
 * @return Event count
 */
uint32_t xy_os_sys_get_event_count(uint32_t event);

/**
 * @brief Reset system event count
 * @param event Event type
 */
void xy_os_sys_reset_event_count(uint32_t event);

/**
 * @brief Get system performance counters
 * @param[out] perf Performance structure
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_get_performance(void *perf);

/**
 * @brief Get system security status
 * @return Security status
 */
uint32_t xy_os_sys_get_security_status(void);

/**
 * @brief Set system security level
 * @param level Security level (0-100)
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_security_level(uint8_t level);

/**
 * @brief Get system security level
 * @return Security level (0-100)
 */
uint8_t xy_os_sys_get_security_level(void);

/**
 * @brief Get system trust zone status
 * @return Trust zone status
 */
uint32_t xy_os_sys_get_trust_zone_status(void);

/**
 * @brief Set system trust zone
 * @param zone Trust zone ID
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_trust_zone(uint32_t zone);

/**
 * @brief Get system privilege level
 * @return Privilege level
 */
uint32_t xy_os_sys_get_privilege_level(void);

/**
 * @brief Set system privilege level
 * @param level Privilege level
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_set_privilege_level(uint32_t level);

/**
 * @brief Get system debug status
 * @return Debug status
 */
uint32_t xy_os_sys_get_debug_status(void);

/**
 * @brief Enable system debug
 * @param enable Enable/disable debug
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_debug(uint8_t enable);

/**
 * @brief Get system trace status
 * @return Trace status
 */
uint32_t xy_os_sys_get_trace_status(void);

/**
 * @brief Enable system trace
 * @param enable Enable/disable trace
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_trace(uint8_t enable);

/**
 * @brief Get system profiling status
 * @return Profiling status
 */
uint32_t xy_os_sys_get_profiling_status(void);

/**
 * @brief Enable system profiling
 * @param enable Enable/disable profiling
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_profiling(uint8_t enable);

/**
 * @brief Get system instrumentation status
 * @return Instrumentation status
 */
uint32_t xy_os_sys_get_instrumentation_status(void);

/**
 * @brief Enable system instrumentation
 * @param enable Enable/disable instrumentation
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_instrumentation(uint8_t enable);

/**
 * @brief Get system audit status
 * @return Audit status
 */
uint32_t xy_os_sys_get_audit_status(void);

/**
 * @brief Enable system audit
 * @param enable Enable/disable audit
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_audit(uint8_t enable);

/**
 * @brief Get system compliance status
 * @return Compliance status
 */
uint32_t xy_os_sys_get_compliance_status(void);

/**
 * @brief Enable system compliance checking
 * @param enable Enable/disable compliance checking
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_compliance(uint8_t enable);

/**
 * @brief Get system certification status
 * @return Certification status
 */
uint32_t xy_os_sys_get_certification_status(void);

/**
 * @brief Enable system certification checking
 * @param enable Enable/disable certification checking
 * @return XY_OS_OK on success, error code on failure
 */
xy_os_error_t xy_os_sys_enable_certification(uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif /* XY_OS_SYS_H */
