/**
 * @file xy_hal_sys.h
 * @brief XinYi System Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_SYS_H
#define XY_HAL_SYS_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 系统滴答频率 (Hz)
 */
#ifndef XY_HAL_SYS_TICK_FREQ
#define XY_HAL_SYS_TICK_FREQ 1000
#include "xy_hal_error.h"
#endif

/**
 * @brief 系统滴答周期 (ms)
 */
#define XY_HAL_SYS_TICK_PERIOD_MS (1000 / XY_HAL_SYS_TICK_FREQ)
#include "xy_hal_error.h"

/**
 * @brief 系统时钟信息结构
 */
typedef struct {
    uint32_t sysclk;      /**< 系统时钟频率 (Hz) */
    uint32_t hclk;        /**< AHB 总线时钟 (Hz) */
    uint32_t pclk1;       /**< APB1 总线时钟 (Hz) */
    uint32_t pclk2;       /**< APB2 总线时钟 (Hz) */
    uint32_t pclk3;       /**< APB3 总线时钟 (Hz) */
    uint8_t  hsi_ready;   /**< HSI 就绪标志 */
    uint8_t  hse_ready;   /**< HSE 就绪标志 */
    uint8_t  pll_ready;   /**< PLL 就绪标志 */
} xy_hal_sys_clock_info_t;

/* ==================== 系统控制 ==================== */

/**
 * @brief 系统初始化
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_init(void);

/**
 * @brief 系统去初始化
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_deinit(void);

/**
 * @brief 获取系统滴答计数
 * @return 当前滴答计数
 */
uint32_t xy_hal_sys_get_tick_count(void);

/**
 * @brief 获取系统滴答频率
 * @return 滴答频率 (Hz)
 */
uint32_t xy_hal_sys_get_tick_freq(void);

/**
 * @brief 获取系统时钟信息
 * @param[out] info 时钟信息输出结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_clock_info(xy_hal_sys_clock_info_t *info);

/**
 * @brief 系统复位
 */
void xy_hal_sys_reset(void);

/**
 * @brief 系统软件复位
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_software_reset(void);

/**
 * @brief 获取复位原因
 * @return 复位原因码
 */
uint32_t xy_hal_sys_get_reset_reason(void);

/**
 * @brief 清除复位原因
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_clear_reset_reason(void);

/**
 * @brief 获取芯片 ID
 * @return 芯片 ID，负值表示错误
 */
int32_t xy_hal_sys_get_chip_id(void);

/**
 * @brief 获取芯片版本
 * @return 芯片版本，负值表示错误
 */
int32_t xy_hal_sys_get_chip_version(void);

/**
 * @brief 获取芯片名称
 * @param[out] name 芯片名称输出缓冲区
 * @param[in] size 缓冲区大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_chip_name(char *name, size_t size);

/**
 * @brief 获取芯片序列号
 * @param[out] serial 序列号输出缓冲区
 * @param[in] size 缓冲区大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_chip_serial(char *serial, size_t size);

/**
 * @brief 获取系统时间 (毫秒)
 * @return 系统运行时间 (毫秒)
 */
uint64_t xy_hal_sys_get_uptime_ms(void);

/**
 * @brief 获取系统时间 (秒)
 * @return 系统运行时间 (秒)
 */
uint32_t xy_hal_sys_get_uptime_sec(void);

/* ==================== 电源管理 ==================== */

/**
 * @brief 系统运行模式
 */
typedef enum {
    XY_HAL_SYS_PWR_RUN = 0,          /**< 运行模式 */
    XY_HAL_SYS_PWR_SLEEP,            /**< 睡眠模式 */
    XY_HAL_SYS_PWR_STOP,             /**< 停机模式 */
    XY_HAL_SYS_PWR_STANDBY,          /**< 待机模式 */
    XY_HAL_SYS_PWR_SHUTDOWN,         /**< 关机模式 */
} xy_hal_sys_pwr_mode_t;

/**
 * @brief 电源域
 */
typedef enum {
    XY_HAL_SYS_PWR_DOMAIN_CORE = 0,  /**< 核心电源域 */
    XY_HAL_SYS_PWR_DOMAIN_PERI,      /**< 外设电源域 */
    XY_HAL_SYS_PWR_DOMAIN_RAM,       /**< RAM 电源域 */
    XY_HAL_SYS_PWR_DOMAIN_IO,        /**< IO 电源域 */
} xy_hal_sys_pwr_domain_t;

/**
 * @brief 电源配置结构
 */
typedef struct {
    xy_hal_sys_pwr_mode_t mode;       /**< 电源模式 */
    uint8_t enable_regulators;        /**< 使能调节器 */
    uint8_t enable_low_power_run;     /**< 使能低功耗运行 */
    uint8_t enable_low_power_sleep;   /**< 使能低功耗睡眠 */
    uint8_t enable_wakeup_pins;      /**< 使能唤醒引脚 */
    uint8_t enable_wakeup_timer;     /**< 使能唤醒定时器 */
    uint8_t enable_backup_domain;     /**< 使能备份域 */
    uint8_t retention_state;         /**< 保持状态 */
} xy_hal_sys_pwr_config_t;

/**
 * @brief 进入电源模式
 * @param mode 电源模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enter_pwr_mode(xy_hal_sys_pwr_mode_t mode);

/**
 * @brief 退出电源模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_exit_pwr_mode(void);

/**
 * @brief 设置电源域配置
 * @param domain 电源域
 * @param config 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_pwr_domain_config(xy_hal_sys_pwr_domain_t domain,
                                               const xy_hal_sys_pwr_config_t *config);

/**
 * @brief 获取电源域配置
 * @param domain 电源域
 * @param config 配置输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_pwr_domain_config(xy_hal_sys_pwr_domain_t domain,
                                               xy_hal_sys_pwr_config_t *config);

/**
 * @brief 使能电源域
 * @param domain 电源域
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_pwr_domain(xy_hal_sys_pwr_domain_t domain,
                                          uint8_t enable);

/**
 * @brief 获取电源域状态
 * @param domain 电源域
 * @return 1=使能，0=禁用，负值表示错误
 */
int32_t xy_hal_sys_get_pwr_domain_state(xy_hal_sys_pwr_domain_t domain);

/**
 * @brief 电源域电压调节
 * @param domain 电源域
 * @param voltage 电压值 (mV)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_pwr_voltage(xy_hal_sys_pwr_domain_t domain,
                                        uint32_t voltage);

/**
 * @brief 获取电源域电压
 * @param domain 电源域
 * @return 电压值 (mV)，负值表示错误
 */
int32_t xy_hal_sys_get_pwr_voltage(xy_hal_sys_pwr_domain_t domain);

/**
 * @brief 电源域电流监测
 * @param domain 电源域
 * @return 电流值 (mA)，负值表示错误
 */
int32_t xy_hal_sys_get_pwr_current(xy_hal_sys_pwr_domain_t domain);

/**
 * @brief 电源域功耗监测
 * @param domain 电源域
 * @return 功耗值 (mW)，负值表示错误
 */
int32_t xy_hal_sys_get_pwr_power(xy_hal_sys_pwr_domain_t domain);

/* ==================== 时钟管理 ==================== */

/**
 * @brief 时钟源
 */
typedef enum {
    XY_HAL_SYS_CLKSRC_HSI = 0,       /**< 内部高速时钟 */
    XY_HAL_SYS_CLKSRC_HSE,           /**< 外部高速时钟 */
    XY_HAL_SYS_CLKSRC_LSI,           /**< 内部低速时钟 */
    XY_HAL_SYS_CLKSRC_LSE,           /**< 外部低速时钟 */
    XY_HAL_SYS_CLKSRC_PLL,           /**< PLL 时钟 */
    XY_HAL_SYS_CLKSRC_PLL2,          /**< PLL2 时钟 */
    XY_HAL_SYS_CLKSRC_PLL3,          /**< PLL3 时钟 */
    XY_HAL_SYS_CLKSRC_MSI,           /**< MSI 时钟 */
    XY_HAL_SYS_CLKSRC_HSI48,         /**< 48MHz HSI */
} xy_hal_sys_clksrc_t;

/**
 * @brief 时钟域
 */
typedef enum {
    XY_HAL_SYS_CLKDOMAIN_SYS = 0,    /**< 系统时钟 */
    XY_HAL_SYS_CLKDOMAIN_AHB,        /**< AHB 总线时钟 */
    XY_HAL_SYS_CLKDOMAIN_APB1,       /**< APB1 总线时钟 */
    XY_HAL_SYS_CLKDOMAIN_APB2,       /**< APB2 总线时钟 */
    XY_HAL_SYS_CLKDOMAIN_APB3,       /**< APB3 总线时钟 */
    XY_HAL_SYS_CLKDOMAIN_APB4,       /**< APB4 总线时钟 */
    XY_HAL_SYS_CLKDOMAIN_RTC,        /**< RTC 时钟 */
    XY_HAL_SYS_CLKDOMAIN_ADC,        /**< ADC 时钟 */
    XY_HAL_SYS_CLKDOMAIN_USB,        /**< USB 时钟 */
    XY_HAL_SYS_CLKDOMAIN_RNG,        /**< RNG 时钟 */
} xy_hal_sys_clkdomain_t;

/**
 * @brief 时钟配置结构
 */
typedef struct {
    xy_hal_sys_clksrc_t source;       /**< 时钟源 */
    uint32_t frequency;              /**< 频率 (Hz) */
    uint8_t enable_pll;              /**< 使能 PLL */
    uint8_t pll_multiplier;          /**< PLL 倍频器 */
    uint8_t pll_divider;             /**< PLL 分频器 */
    uint8_t hse_bypass;              /**< HSE 旁路 */
    uint8_t hse_drive;               /**< HSE 驱动强度 */
    uint32_t ahb_prescaler;          /**< AHB 预分频 */
    uint32_t apb1_prescaler;         /**< APB1 预分频 */
    uint32_t apb2_prescaler;         /**< APB2 预分频 */
} xy_hal_sys_clk_config_t;

/**
 * @brief 设置时钟域频率
 * @param domain 时钟域
 * @param freq 频率 (Hz)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_clk_freq(xy_hal_sys_clkdomain_t domain, uint32_t freq);

/**
 * @brief 获取时钟域频率
 * @param domain 时钟域
 * @return 频率 (Hz)，负值表示错误
 */
int32_t xy_hal_sys_get_clk_freq(xy_hal_sys_clkdomain_t domain);

/**
 * @brief 设置时钟源
 * @param domain 时钟域
 * @param source 时钟源
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_clk_source(xy_hal_sys_clkdomain_t domain,
                                        xy_hal_sys_clksrc_t source);

/**
 * @brief 获取时钟源
 * @param domain 时钟域
 * @return 时钟源，负值表示错误
 */
int32_t xy_hal_sys_get_clk_source(xy_hal_sys_clkdomain_t domain);

/**
 * @brief 使能时钟域
 * @param domain 时钟域
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_clk_domain(xy_hal_sys_clkdomain_t domain,
                                           uint8_t enable);

/**
 * @brief 获取时钟域状态
 * @param domain 时钟域
 * @return 1=使能，0=禁用，负值表示错误
 */
int32_t xy_hal_sys_get_clk_domain_state(xy_hal_sys_clkdomain_t domain);

/**
 * @brief 时钟域配置
 * @param domain 时钟域
 * @param config 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_config_clk_domain(xy_hal_sys_clkdomain_t domain,
                                           const xy_hal_sys_clk_config_t *config);

/* ==================== 中断管理 ==================== */

/**
 * @brief 使能全局中断
 */
void xy_hal_sys_enable_irq(void);

/**
 * @brief 禁用全局中断
 */
void xy_hal_sys_disable_irq(void);

/**
 * @brief 进入临界区
 * @return 中断状态
 */
uint32_t xy_hal_sys_enter_critical(void);

/**
 * @brief 退出临界区
 * @param[in] primask 之前的中断状态
 */
void xy_hal_sys_exit_critical(uint32_t primask);

/**
 * @brief 保存中断状态
 * @return 中断状态
 */
uint32_t xy_hal_sys_save_irq_state(void);

/**
 * @brief 恢复中断状态
 * @param[in] primask 中断状态
 */
void xy_hal_sys_restore_irq_state(uint32_t primask);

/**
 * @brief 获取中断嵌套级别
 * @return 嵌套级别
 */
uint32_t xy_hal_sys_get_irq_nest_level(void);

/**
 * @brief 检查是否在中断上下文
 * @return 1=中断上下文，0=线程上下文
 */
int xy_hal_sys_in_isr(void);

/**
 * @brief 获取当前中断号
 * @return 中断号，负值表示错误
 */
int32_t xy_hal_sys_get_current_irq(void);

/**
 * @brief 设置中断优先级
 * @param irq_no 中断号
 * @param priority 优先级
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_irq_priority(uint32_t irq_no, uint32_t priority);

/**
 * @brief 获取中断优先级
 * @param irq_no 中断号
 * @return 优先级，负值表示错误
 */
int32_t xy_hal_sys_get_irq_priority(uint32_t irq_no);

/**
 * @brief 使能中断
 * @param irq_no 中断号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_irq_num(uint32_t irq_no);

/**
 * @brief 禁用中断
 * @param irq_no 中断号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_disable_irq_num(uint32_t irq_no);

/**
 * @brief 清除中断标志
 * @param irq_no 中断号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_clear_irq_flag(uint32_t irq_no);

/**
 * @brief 触发软件中断
 * @param irq_no 中断号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_trigger_software_irq(uint32_t irq_no);

/* ==================== 内存管理 ==================== */

/**
 * @brief 内存区域类型
 */
typedef enum {
    XY_HAL_SYS_MEM_TYPE_SRAM = 0,    /**< SRAM */
    XY_HAL_SYS_MEM_TYPE_FLASH,       /**< Flash */
    XY_HAL_SYS_MEM_TYPE_SDRAM,       /**< SDRAM */
    XY_HAL_SYS_MEM_TYPE_QSPI,        /**< QSPI Flash */
    XY_HAL_SYS_MEM_TYPE_NOR,         /**< NOR Flash */
    XY_HAL_SYS_MEM_TYPE_NAND,        /**< NAND Flash */
    XY_HAL_SYS_MEM_TYPE_OTP,         /**< OTP */
    XY_HAL_SYS_MEM_TYPE_BACKUP,      /**< 备份 RAM */
    XY_HAL_SYS_MEM_TYPE_RETENTION,   /**< 保持 RAM */
    XY_HAL_SYS_MEM_TYPE_CACHED,      /**< 缓存内存 */
    XY_HAL_SYS_MEM_TYPE_UNCACHED,    /**< 非缓存内存 */
} xy_hal_sys_mem_type_t;

/**
 * @brief 内存信息结构
 */
typedef struct {
    uint32_t start_addr;             /**< 起始地址 */
    uint32_t size;                   /**< 大小 (字节) */
    xy_hal_sys_mem_type_t type;      /**< 类型 */
    uint8_t is_secure;               /**< 安全区域 */
    uint8_t is_cached;               /**< 缓存属性 */
    uint8_t is_executable;           /**< 可执行 */
    uint8_t is_readable;             /**< 可读 */
    uint8_t is_writable;             /**< 可写 */
    uint8_t is_dirty;                /**< 脏标志 */
} xy_hal_sys_mem_info_t;

/**
 * @brief 获取内存信息
 * @param type 内存类型
 * @param info 信息输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_mem_info(xy_hal_sys_mem_type_t type,
                                      xy_hal_sys_mem_info_t *info);

/**
 * @brief 内存屏障
 */
void xy_hal_sys_memory_barrier(void);

/**
 * @brief 指令屏障
 */
void xy_hal_sys_instruction_barrier(void);

/**
 * @brief 数据缓存清理
 * @param addr 地址
 * @param size 大小 (字节)
 */
void xy_hal_sys_dcache_clean(void *addr, size_t size);

/**
 * @brief 数据缓存无效化
 * @param addr 地址
 * @param size 大小 (字节)
 */
void xy_hal_sys_dcache_invalidate(void *addr, size_t size);

/**
 * @brief 数据缓存清理和无效化
 * @param addr 地址
 * @param size 大小 (字节)
 */
void xy_hal_sys_dcache_clean_invalidate(void *addr, size_t size);

/**
 * @brief 指令缓存无效化
 * @param addr 地址
 * @param size 大小 (字节)
 */
void xy_hal_sys_icache_invalidate(void *addr, size_t size);

/**
 * @brief 使能 FPU
 */
void xy_hal_sys_enable_fpu(void);

/**
 * @brief 禁用 FPU
 */
void xy_hal_sys_disable_fpu(void);

/**
 * @brief 获取 FPU 状态
 * @return 1=FPU 使能，0=FPU 禁用
 */
int xy_hal_sys_is_fpu_enabled(void);

/* ==================== 外设管理 ==================== */

/**
 * @brief 外设 ID
 */
typedef enum {
    XY_HAL_SYS_PERIPH_GPIOA = 0,     /**< GPIOA */
    XY_HAL_SYS_PERIPH_GPIOB,         /**< GPIOB */
    XY_HAL_SYS_PERIPH_GPIOC,         /**< GPIOC */
    XY_HAL_SYS_PERIPH_GPIOD,         /**< GPIOD */
    XY_HAL_SYS_PERIPH_GPIOE,         /**< GPIOE */
    XY_HAL_SYS_PERIPH_GPIOF,         /**< GPIOF */
    XY_HAL_SYS_PERIPH_GPIOG,         /**< GPIOG */
    XY_HAL_SYS_PERIPH_GPIOH,         /**< GPIOH */
    XY_HAL_SYS_PERIPH_GPIOI,         /**< GPIOI */
    XY_HAL_SYS_PERIPH_UART1,         /**< UART1 */
    XY_HAL_SYS_PERIPH_UART2,         /**< UART2 */
    XY_HAL_SYS_PERIPH_UART3,         /**< UART3 */
    XY_HAL_SYS_PERIPH_SPI1,          /**< SPI1 */
    XY_HAL_SYS_PERIPH_SPI2,          /**< SPI2 */
    XY_HAL_SYS_PERIPH_SPI3,          /**< SPI3 */
    XY_HAL_SYS_PERIPH_I2C1,          /**< I2C1 */
    XY_HAL_SYS_PERIPH_I2C2,          /**< I2C2 */
    XY_HAL_SYS_PERIPH_I2C3,          /**< I2C3 */
    XY_HAL_SYS_PERIPH_DMA1,          /**< DMA1 */
    XY_HAL_SYS_PERIPH_DMA2,          /**< DMA2 */
    XY_HAL_SYS_PERIPH_ADC1,          /**< ADC1 */
    XY_HAL_SYS_PERIPH_ADC2,          /**< ADC2 */
    XY_HAL_SYS_PERIPH_TIM1,          /**< TIM1 */
    XY_HAL_SYS_PERIPH_TIM2,          /**< TIM2 */
    XY_HAL_SYS_PERIPH_RTC,           /**< RTC */
    XY_HAL_SYS_PERIPH_WWDG,          /**< WWDG */
    XY_HAL_SYS_PERIPH_IWDG,          /**< IWDG */
    XY_HAL_SYS_PERIPH_CRC,           /**< CRC */
    XY_HAL_SYS_PERIPH_RNG,           /**< RNG */
    XY_HAL_SYS_PERIPH_FLASH,         /**< Flash */
    XY_HAL_SYS_PERIPH_PWR,           /**< PWR */
    XY_HAL_SYS_PERIPH_SYSCFG,        /**< SYSCFG */
    XY_HAL_SYS_PERIPH_DBGMCU,        /**< DBGMCU */
    XY_HAL_SYS_PERIPH_BKPSRAM,       /**< Backup SRAM */
    XY_HAL_SYS_PERIPH_MAX
} xy_hal_sys_periph_id_t;

/**
 * @brief 使能外设时钟
 * @param periph_id 外设 ID
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_periph_clock(xy_hal_sys_periph_id_t periph_id);

/**
 * @brief 禁用外设时钟
 * @param periph_id 外设 ID
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_disable_periph_clock(xy_hal_sys_periph_id_t periph_id);

/**
 * @brief 重置外设
 * @param periph_id 外设 ID
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_reset_periph(xy_hal_sys_periph_id_t periph_id);

/**
 * @brief 获取外设时钟频率
 * @param periph_id 外设 ID
 * @return 频率 (Hz)，负值表示错误
 */
int32_t xy_hal_sys_get_periph_clock_freq(xy_hal_sys_periph_id_t periph_id);

/**
 * @brief 获取外设基地址
 * @param periph_id 外设 ID
 * @return 基地址，NULL 表示错误
 */
void *xy_hal_sys_get_periph_base_addr(xy_hal_sys_periph_id_t periph_id);

/**
 * @brief 获取外设中断号
 * @param periph_id 外设 ID
 * @return 中断号，负值表示错误
 */
int32_t xy_hal_sys_get_periph_irq_num(xy_hal_sys_periph_id_t periph_id);

/* ==================== 系统信息 ==================== */

/**
 * @brief 系统信息结构
 */
typedef struct {
    uint32_t cpu_freq_hz;            /**< CPU 频率 (Hz) */
    uint32_t hclk_freq_hz;           /**< AHB 频率 (Hz) */
    uint32_t pclk1_freq_hz;          /**< APB1 频率 (Hz) */
    uint32_t pclk2_freq_hz;          /**< APB2 频率 (Hz) */
    uint32_t tick_freq_hz;           /**< 滴答频率 (Hz) */
    uint32_t flash_size_kb;          /**< Flash 大小 (KB) */
    uint32_t sram_size_kb;           /**< SRAM 大小 (KB) */
    uint32_t sram2_size_kb;          /**< SRAM2 大小 (KB) */
    uint32_t chip_revision;          /**< 芯片修订版本 */
    uint32_t chip_package;           /**< 芯片封装 */
    uint32_t uid[3];                 /**< 唯一 ID */
    char chip_name[32];              /**< 芯片名称 */
    char mcu_name[32];               /**< MCU 名称 */
    char vendor[16];                 /**< 供应商 */
    char core[16];                   /**< 核心 */
    char fpu[16];                    /**< FPU 类型 */
    uint8_t nvic_irq_count;          /**< NVIC 中断数量 */
    uint8_t fpu_present;             /**< FPU 存在标志 */
    uint8_t mpu_present;             /**< MPU 存在标志 */
    uint8_t dcache_present;          /**< DCACHE 存在标志 */
    uint8_t icache_present;          /**< ICACHE 存在标志 */
} xy_hal_sys_info_t;

/**
 * @brief 获取系统信息
 * @param[out] info 系统信息输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_info(xy_hal_sys_info_t *info);

/**
 * @brief 获取 CPU 频率
 * @return CPU 频率 (Hz)
 */
uint32_t xy_hal_sys_get_cpu_freq(void);

/**
 * @brief 获取 Flash 大小
 * @return Flash 大小 (KB)
 */
uint32_t xy_hal_sys_get_flash_size(void);

/**
 * @brief 获取 RAM 大小
 * @return RAM 大小 (KB)
 */
uint32_t xy_hal_sys_get_ram_size(void);

/**
 * @brief 获取可用内存
 * @return 可用内存 (KB)
 */
uint32_t xy_hal_sys_get_available_ram(void);

/**
 * @brief 获取堆使用情况
 * @param[out] total 总堆大小
 * @param[out] used 已使用堆大小
 * @param[out] free 空闲堆大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_heap_usage(uint32_t *total, uint32_t *used, uint32_t *free);

/**
 * @brief 获取栈使用情况
 * @param[out] total 总栈大小
 * @param[out] used 已使用栈大小
 * @param[out] free 空闲栈大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_get_stack_usage(uint32_t *total, uint32_t *used, uint32_t *free);

/* ==================== 安全功能 ==================== */

/**
 * @brief 安全状态
 */
typedef enum {
    XY_HAL_SYS_SECURITY_STATE_SECURE = 0,   /**< 安全状态 */
    XY_HAL_SYS_SECURITY_STATE_NONSECURE,    /**< 非安全状态 */
    XY_HAL_SYS_SECURITY_STATE_TRUSTED,      /**< 可信状态 */
    XY_HAL_SYS_SECURITY_STATE_UNTRUSTED,    /**< 不可信状态 */
} xy_hal_sys_security_state_t;

/**
 * @brief 获取安全状态
 * @return 安全状态
 */
xy_hal_sys_security_state_t xy_hal_sys_get_security_state(void);

/**
 * @brief 使能安全功能
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_security(uint8_t enable);

/**
 * @brief 使能 TrustZone
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_trustzone(uint8_t enable);

/**
 * @brief 获取 TrustZone 状态
 * @return 1=使能，0=禁用
 */
int xy_hal_sys_is_trustzone_enabled(void);

/* ==================== 调试功能 ==================== */

/**
 * @brief 系统调试功能
 */
typedef enum {
    XY_HAL_SYS_DBG_TRACE = 0,        /**< 跟踪调试 */
    XY_HAL_SYS_DBG_ASSERT,           /**< 断言调试 */
    XY_HAL_SYS_DBG_PROFILING,        /**< 性能分析 */
    XY_HAL_SYS_DBG_MEMORY,           /**< 内存调试 */
    XY_HAL_SYS_DBG_PERFORMANCE,      /**< 性能调试 */
    XY_HAL_SYS_DBG_SECURITY,         /**< 安全调试 */
} xy_hal_sys_dbg_feature_t;

/**
 * @brief 使能调试功能
 * @param feature 调试功能
 * @param enable 使能标志
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_enable_debug_feature(xy_hal_sys_dbg_feature_t feature,
                                              uint8_t enable);

/**
 * @brief 获取调试功能状态
 * @param feature 调试功能
 * @return 1=使能，0=禁用，负值表示错误
 */
int32_t xy_hal_sys_get_debug_feature_state(xy_hal_sys_dbg_feature_t feature);

/**
 * @brief 设置调试级别
 * @param level 调试级别
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_sys_set_debug_level(uint8_t level);

/**
 * @brief 获取调试级别
 * @return 调试级别，负值表示错误
 */
int32_t xy_hal_sys_get_debug_level(void);

/* ==================== 系统控制命令 ==================== */

/**
 * @brief 系统控制命令
 */
#define XY_HAL_SYS_CMD_SET_PWR_MODE           0x01
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_PWR_MODE           0x02
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_SET_CLK_FREQ           0x03
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_CLK_FREQ           0x04
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_SET_IRQ_PRIORITY       0x05
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_IRQ_PRIORITY       0x06
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_MEM_INFO           0x07
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_SYS_INFO           0x08
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_CHIP_ID            0x09
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_CHIP_VERSION       0x0A
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_UPTIME             0x0B
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_RESET_REASON       0x0C
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_CLEAR_RESET_REASON     0x0D
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_ENABLE_PERIPH_CLK      0x0E
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_DISABLE_PERIPH_CLK     0x0F
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_RESET_PERIPH           0x10
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_PERIPH_CLK_FREQ    0x11
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_PERIPH_BASE_ADDR   0x12
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_PERIPH_IRQ_NUM     0x13
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_ENABLE_SECURITY        0x14
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_SECURITY_STATE     0x15
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_ENABLE_TRUSTZONE       0x16
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_TRUSTZONE_STATE    0x17
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_SET_DEBUG_LEVEL        0x18
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_DEBUG_LEVEL        0x19
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_ENABLE_DBG_FEATURE     0x1A
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_DBG_FEATURE_STATE  0x1B
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_HEAP_USAGE         0x1C
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_STACK_USAGE        0x1D
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_AVAILABLE_RAM      0x1E
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_FLASH_SIZE         0x1F
#include "xy_hal_error.h"
#define XY_HAL_SYS_CMD_GET_RAM_SIZE           0x20
#include "xy_hal_error.h"

/* ==================== 便利函数 ==================== */

/**
 * @brief 延时 (毫秒)
 * @param ms 毫秒数
 */
static inline void xy_hal_sys_delay_ms(uint32_t ms)
{
    uint32_t start = xy_hal_sys_get_tick_count();
    while ((xy_hal_sys_get_tick_count() - start) < ms) {
        /* 空循环 */
    }
}

/**
 * @brief 延时 (微秒)
 * @param us 微秒数
 */
void xy_hal_sys_delay_us(uint32_t us);

/**
 * @brief 检查是否经过指定时间 (毫秒)
 * @param start 开始时间
 * @param interval 间隔时间 (ms)
 * @return 1=已超时，0=未超时
 */
static inline int xy_hal_sys_elapsed_ms(uint32_t start, uint32_t interval)
{
    return (xy_hal_sys_get_tick_count() - start) >= interval;
}

/**
 * @brief 检查是否经过指定时间 (微秒)
 * @param start 开始时间
 * @param interval 间隔时间 (us)
 * @return 1=已超时，0=未超时
 */
int xy_hal_sys_elapsed_us(uint32_t start, uint64_t interval);

/**
 * @brief 获取时间戳 (微秒)
 * @return 时间戳 (us)
 */
uint64_t xy_hal_sys_get_timestamp_us(void);

/**
 * @brief 获取时间戳 (毫秒)
 * @return 时间戳 (ms)
 */
uint64_t xy_hal_sys_get_timestamp_ms(void);

/**
 * @brief 计算时间差 (毫秒)
 * @param start 开始时间戳
 * @param end 结束时间戳
 * @return 时间差 (ms)
 */
static inline uint32_t xy_hal_sys_diff_ms(uint32_t start, uint32_t end)
{
    return (start <= end) ? (end - start) : (UINT32_MAX - start + end + 1);
}

/**
 * @brief 计算时间差 (微秒)
 * @param start 开始时间戳
 * @param end 结束时间戳
 * @return 时间差 (us)
 */
uint64_t xy_hal_sys_diff_us(uint64_t start, uint64_t end);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_SYS_H */

/* Platform definitions */
#ifndef XY_HAL_SYS_TICK_FREQ
#define XY_HAL_SYS_TICK_FREQ 1000
#endif
