/**
 * @file example_usage.c
 * @brief Example usage of XY HAL with STM32
 * @version 1.0
 * @date 2025-10-26
 *
 * This file demonstrates how to use the XY HAL abstraction layer
 * in an STM32 project. Compile with -DSTM32_HAL_ENABLED and
 * the appropriate STM32 family macro (e.g., -DSTM32F4).
 */

#include "../inc/xy_hal.h"
#include <stdio.h>
#include <string.h>

#ifdef STM32_HAL_ENABLED

/* External HAL handles (typically defined in main.c or init code) */
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;
extern RTC_HandleTypeDef hrtc;

/**
 * Example 1: GPIO LED Control
 */
void example_gpio_led(void)
{
    /* Configure PA5 as output (typically the user LED on STM32 boards) */
    xy_hal_pin_config_t led_config = {
        .mode  = XY_HAL_PIN_MODE_OUTPUT,
        .pull  = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };

    xy_hal_pin_init(GPIOA, 5, &led_config);

    /* Blink LED */
    for (int i = 0; i < 10; i++) {
        xy_hal_pin_toggle(GPIOA, 5);
        HAL_Delay(500);
    }
}

/**
 * Example 2: GPIO Button with Interrupt
 */
void button_callback(void *arg)
{
    /* Called when button is pressed */
    xy_hal_pin_toggle(GPIOA, 5); /* Toggle LED on button press */
}

void example_gpio_button(void)
{
    /* Configure PC13 as input with interrupt (typically user button) */
    xy_hal_pin_config_t button_config = {
        .mode  = XY_HAL_PIN_MODE_INPUT,
        .pull  = XY_HAL_PIN_PULL_UP,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };

    xy_hal_pin_init(GPIOC, 13, &button_config);

    /* Attach interrupt on falling edge */
    xy_hal_pin_attach_irq(
        GPIOC, 13, XY_HAL_PIN_IRQ_FALLING, button_callback, NULL);
}

/**
 * Example 3: UART Communication
 */
void example_uart(void)
{
    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
        .wordlen  = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity   = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode     = XY_HAL_UART_MODE_TX_RX,
    };

    xy_hal_uart_init(&huart1, &uart_config);

    /* Send message */
    const char *msg = "Hello from XY HAL!\r\n";
    xy_hal_uart_send(&huart1, (const uint8_t *)msg, strlen(msg), 1000);

    /* Receive data */
    uint8_t rx_buffer[64];
    int rx_len = xy_hal_uart_recv(&huart1, rx_buffer, sizeof(rx_buffer), 5000);
    if (rx_len > 0) {
        /* Process received data */
    }
}

/**
 * Example 4: SPI Communication
 */
void example_spi(void)
{
    xy_hal_spi_config_t spi_config = {
        .mode               = XY_HAL_SPI_MODE_0,
        .direction          = XY_HAL_SPI_DIR_2LINES,
        .datasize           = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit           = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss                = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,
        .is_master          = 1,
    };

    xy_hal_spi_init(&hspi1, &spi_config);

    /* Transmit data */
    uint8_t tx_data[] = { 0x01, 0x02, 0x03, 0x04 };
    xy_hal_spi_transmit(&hspi1, tx_data, sizeof(tx_data), 1000);

    /* Receive data */
    uint8_t rx_data[4];
    xy_hal_spi_receive(&hspi1, rx_data, sizeof(rx_data), 1000);

    /* Full duplex transfer */
    uint8_t tx_buf[] = { 0xAA, 0xBB };
    uint8_t rx_buf[2];
    xy_hal_spi_transmit_receive(&hspi1, tx_buf, rx_buf, 2, 1000);
}

/**
 * Example 5: I2C Sensor Reading
 */
void example_i2c_sensor(void)
{
    xy_hal_i2c_config_t i2c_config = {
        .clock_speed       = 100000, /* 100kHz standard mode */
        .addr_mode         = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle        = XY_HAL_I2C_DUTY_2,
        .own_address       = 0x00,
        .general_call_mode = 0,
    };

    xy_hal_i2c_init(&hi2c1, &i2c_config);

    /* Example: Read from MPU6050 accelerometer */
    uint16_t device_addr  = 0x68;
    uint16_t who_am_i_reg = 0x75;
    uint8_t who_am_i;

    /* Check if device is ready */
    if (xy_hal_i2c_is_device_ready(&hi2c1, device_addr, 3, 1000) == 0) {
        /* Read WHO_AM_I register */
        xy_hal_i2c_mem_read(
            &hi2c1, device_addr, who_am_i_reg, &who_am_i, 1, 1000);

        /* Read accelerometer data (6 bytes) */
        uint8_t accel_data[6];
        xy_hal_i2c_mem_read(&hi2c1, device_addr, 0x3B, accel_data, 6, 1000);
    }
}

/**
 * Example 6: PWM Motor Control
 */
void example_pwm_motor(void)
{
    xy_hal_pwm_config_t pwm_config = {
        .frequency  = 20000, /* 20kHz for motor control */
        .duty_cycle = 0,     /* Start at 0% */
        .polarity   = XY_HAL_PWM_POLARITY_HIGH,
    };

    xy_hal_pwm_init(&htim2, XY_HAL_PWM_CHANNEL_1, &pwm_config);
    xy_hal_pwm_start(&htim2, XY_HAL_PWM_CHANNEL_1);

    /* Ramp up motor speed */
    for (uint32_t duty = 0; duty <= 10000; duty += 100) {
        xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
        HAL_Delay(10);
    }

    /* Hold at full speed */
    HAL_Delay(2000);

    /* Ramp down */
    for (uint32_t duty = 10000; duty > 0; duty -= 100) {
        xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
        HAL_Delay(10);
    }

    xy_hal_pwm_stop(&htim2, XY_HAL_PWM_CHANNEL_1);
}

/**
 * Example 7: Timer Interrupt
 */
void timer_callback(void *timer, xy_hal_timer_event_t event, void *arg)
{
    if (event == XY_HAL_TIMER_EVENT_UPDATE) {
        /* Called every timer overflow */
        xy_hal_pin_toggle(GPIOA, 5); /* Toggle LED at timer frequency */
    }
}

void example_timer(void)
{
    xy_hal_timer_config_t timer_config = {
        .prescaler           = 7200 - 1,  /* For 72MHz clock -> 10kHz */
        .period              = 10000 - 1, /* 1Hz overflow */
        .mode                = XY_HAL_TIMER_COUNT_UP,
        .clock_div           = XY_HAL_TIMER_CKDIV_1,
        .auto_reload_preload = 1,
    };

    xy_hal_timer_init(&htim2, &timer_config);
    xy_hal_timer_register_callback(
        &htim2, XY_HAL_TIMER_EVENT_UPDATE, timer_callback, NULL);
    xy_hal_timer_enable_irq(&htim2, XY_HAL_TIMER_EVENT_UPDATE);
    xy_hal_timer_start(&htim2);
}

/**
 * Example 8: RTC Date/Time
 */
void example_rtc(void)
{
    xy_hal_rtc_init(&hrtc);

    /* Set initial time: 12:30:45 */
    xy_hal_rtc_time_t time = {
        .hours      = 12,
        .minutes    = 30,
        .seconds    = 45,
        .subseconds = 0,
    };
    xy_hal_rtc_set_time(&hrtc, &time, XY_HAL_RTC_FORMAT_BIN);

    /* Set initial date: 2025-10-26, Saturday */
    xy_hal_rtc_date_t date = {
        .weekday = 6, /* Saturday */
        .month   = 10,
        .date    = 26,
        .year    = 2025,
    };
    xy_hal_rtc_set_date(&hrtc, &date, XY_HAL_RTC_FORMAT_BIN);

    /* Set alarm for 12:31:00 */
    xy_hal_rtc_alarm_t alarm = {
        .time = {
            .hours = 12,
            .minutes = 31,
            .seconds = 0,
        },
        .alarm_mask = XY_HAL_RTC_ALARM_MASK_ALL & ~XY_HAL_RTC_ALARM_MASK_SECONDS,
    };
    xy_hal_rtc_set_alarm(&hrtc, &alarm, 'A');
    xy_hal_rtc_enable_alarm(&hrtc, 'A');

    /* Read current time */
    xy_hal_rtc_time_t current_time;
    xy_hal_rtc_date_t current_date;
    xy_hal_rtc_get_time(&hrtc, &current_time, XY_HAL_RTC_FORMAT_BIN);
    xy_hal_rtc_get_date(&hrtc, &current_date, XY_HAL_RTC_FORMAT_BIN);

    /* Get Unix timestamp */
    int64_t timestamp = xy_hal_rtc_get_timestamp(&hrtc);
}

/**
 * Example 9: Complete Application
 */
void xy_hal_example_application(void)
{
    /* Initialize GPIO LED */
    example_gpio_led();

    /* Setup button interrupt */
    example_gpio_button();

    /* Setup UART for debug output */
    example_uart();

    /* Setup I2C sensor */
    example_i2c_sensor();

    /* Setup PWM for LED brightness control */
    xy_hal_pwm_config_t pwm_config = {
        .frequency  = 1000,
        .duty_cycle = 2500, /* 25% brightness */
        .polarity   = XY_HAL_PWM_POLARITY_HIGH,
    };
    xy_hal_pwm_init(&htim2, XY_HAL_PWM_CHANNEL_1, &pwm_config);
    xy_hal_pwm_start(&htim2, XY_HAL_PWM_CHANNEL_1);

    /* Main loop */
    while (1) {
        /* Read sensor data every second */
        uint8_t sensor_data[6];
        xy_hal_i2c_mem_read(&hi2c1, 0x68, 0x3B, sensor_data, 6, 1000);

        /* Send data via UART */
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Sensor: %d %d %d\r\n",
                 (int16_t)((sensor_data[0] << 8) | sensor_data[1]),
                 (int16_t)((sensor_data[2] << 8) | sensor_data[3]),
                 (int16_t)((sensor_data[4] << 8) | sensor_data[5]));
        xy_hal_uart_send(&huart1, (uint8_t *)buffer, strlen(buffer), 1000);

        HAL_Delay(1000);
    }
}

#endif /* STM32_HAL_ENABLED */
