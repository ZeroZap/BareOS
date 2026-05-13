/**
 * @file example_usage.c
 * @brief XY HAL STM32U5 Example Usage
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal.h"
#include "stm32u5_platform.h"

/* ==================== GPIO Example ==================== */

static void example_gpio(void)
{
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };

    /* Initialize PA5 as output */
    xy_hal_pin_init(GPIOA, 5, &config);

    /* Toggle LED */
    while (1) {
        xy_hal_pin_toggle(GPIOA, 5);
        xy_hal_delay_ms(500);
    }
}

/* ==================== UART Example ==================== */

static UART_HandleTypeDef g_uart1;

static void example_uart(void)
{
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };

    xy_hal_error_t ret = xy_hal_uart_init(&g_uart1, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    const char *msg = "Hello from XY HAL!\r\n";
    xy_hal_uart_send(&g_uart1, (uint8_t *)msg, strlen(msg), 1000);
}

/* ==================== I2C Example ==================== */

static I2C_HandleTypeDef g_i2c1;

static void example_i2c(void)
{
    xy_hal_i2c_config_t config = {
        .clock_speed = 100000,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0,
        .general_call_mode = 0,
    };

    xy_hal_error_t ret = xy_hal_i2c_init(&g_i2c1, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    /* Read from sensor */
    uint8_t data[2];
    uint16_t sensor_addr = 0x68;
    uint16_t reg_addr = 0x00;

    ret = xy_hal_i2c_mem_read(&g_i2c1, sensor_addr, reg_addr, data, 2, 1000);
    if (ret != XY_HAL_OK) {
        return;
    }
}

/* ==================== SPI Example ==================== */

static SPI_HandleTypeDef g_spi1;

static void example_spi(void)
{
    xy_hal_spi_config_t config = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_64,
        .is_master = 1,
    };

    xy_hal_error_t ret = xy_hal_spi_init(&g_spi1, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    /* Transmit data */
    uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04};
    ret = xy_hal_spi_transmit(&g_spi1, tx_data, sizeof(tx_data), 1000);
    if (ret != XY_HAL_OK) {
        return;
    }
}

/* ==================== PWM Example ==================== */

static TIM_HandleTypeDef g_tim2;

static void example_pwm(void)
{
    xy_hal_pwm_config_t config = {
        .frequency = 1000,
        .duty_cycle = 5000,
        .polarity = XY_HAL_PWM_POLARITY_HIGH,
    };

    xy_hal_error_t ret = xy_hal_pwm_init(&g_tim2, XY_HAL_PWM_CHANNEL_1, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    xy_hal_pwm_start(&g_tim2, XY_HAL_PWM_CHANNEL_1);

    /* Change duty cycle */
    xy_hal_pwm_set_duty_cycle(&g_tim2, XY_HAL_PWM_CHANNEL_1, 7500);
}

/* ==================== ADC Example ==================== */

static ADC_HandleTypeDef g_adc1;

static void example_adc(void)
{
    xy_hal_adc_config_t config = {
        .resolution = XY_HAL_ADC_RESOLUTION_12B,
        .align = XY_HAL_ADC_DATAALIGN_RIGHT,
        .scan_mode = XY_HAL_ADC_SCAN_DISABLE,
        .continuous = XY_HAL_ADC_CONTINUOUS_DISABLE,
        .trigger_src = XY_HAL_ADC_TRIGGER_SOFTWARE,
        .clock_div = ADC_CLOCK_SYNC_PCLK_DIV4,
        .sampling_time = ADC_SAMPLETIME_2CYCLES_5,
    };

    xy_hal_error_t ret = xy_hal_adc_init(&g_adc1, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    /* Read channel 0 */
    int value = xy_hal_adc_read(&g_adc1, 0, 1000);
    if (value >= 0) {
        /* Process ADC value */
        uint32_t mv = xy_hal_adc_value_to_mv(&g_adc1, (uint32_t)value, 3300);
        XY_UNUSED(mv);
    }
}

/* ==================== RTC Example ==================== */

static RTC_HandleTypeDef g_rtc;

static void example_rtc(void)
{
    xy_hal_error_t ret = xy_hal_rtc_init(&g_rtc);
    if (ret != XY_HAL_OK) {
        return;
    }

    /* Set time */
    xy_hal_rtc_time_t time = {
        .hours = 12,
        .minutes = 30,
        .seconds = 0,
    };
    xy_hal_rtc_date_t date = {
        .weekday = 1,
        .month = 1,
        .date = 1,
        .year = 24,
    };

    xy_hal_rtc_set_time(&g_rtc, &time, XY_HAL_RTC_FORMAT_BIN);
    xy_hal_rtc_set_date(&g_rtc, &date, XY_HAL_RTC_FORMAT_BIN);

    /* Get time */
    xy_hal_rtc_get_time(&g_rtc, &time, XY_HAL_RTC_FORMAT_BIN);
    xy_hal_rtc_get_date(&g_rtc, &date, XY_HAL_RTC_FORMAT_BIN);
}

/* ==================== Watchdog Example ==================== */

static IWDG_HandleTypeDef g_iwdg;

static void example_watchdog(void)
{
    xy_hal_iwdg_config_t config = {
        .prescaler = IWDG_PRESCALER_64,
        .reload = 0xFFF,
        .timeout_ms = 1000,
    };

    xy_hal_error_t ret = xy_hal_iwdg_init(&g_iwdg, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    xy_hal_iwdg_start(&g_iwdg);

    /* Feed watchdog in main loop */
    while (1) {
        xy_hal_iwdg_feed(&g_iwdg);
        xy_hal_delay_ms(500);
    }
}

/* ==================== RNG Example ==================== */

static RNG_HandleTypeDef g_rng;

static void example_rng(void)
{
    xy_hal_rng_config_t config = {
        .clock_enable = 1,
        .interrupt_enable = 0,
    };

    xy_hal_error_t ret = xy_hal_rng_init(&g_rng, &config);
    if (ret != XY_HAL_OK) {
        return;
    }

    /* Generate random numbers */
    uint32_t random_buf[4];
    ret = xy_hal_rng_get_buffer(&g_rng, random_buf, 4, 1000);
    if (ret != XY_HAL_OK) {
        return;
    }
}

/* ==================== Main ==================== */

int main(void)
{
    /* Initialize system */
    HAL_Init();

    /* Configure system clock */
    /* SystemClock_Config(); */

    /* Run examples */
    example_gpio();
    example_uart();
    example_i2c();
    example_spi();
    example_pwm();
    example_adc();
    example_rtc();
    example_watchdog();
    example_rng();

    return 0;
}
