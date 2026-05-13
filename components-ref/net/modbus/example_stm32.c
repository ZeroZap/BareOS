/**
 * @file example_stm32.c
 * @brief Modbus RTU Slave Example for STM32 (Bare-metal)
 * @note Tested on STM32F103, adaptable to other STM32 series
 */

#include "mb_slave.h"

/* ==================== Platform Specific (STM32F1) ==================== */

#ifdef STM32F1
#include "stm32f1xx.h"
#define UART_PERIPHERAL USART1
#define UART_IRQn       USART1_IRQn
#define UART_SR_RXNE    USART_SR_RXNE
#define UART_SR_TXE     USART_SR_TXE
#define UART_DR         DR
#endif

/* ==================== Global Variables ==================== */

static mb_slave_t g_modbus_slave;
static volatile uint32_t g_system_ticks_ms = 0;

/* ==================== System Tick (1ms) ==================== */

void SysTick_Handler(void)
{
    g_system_ticks_ms++;
}

/* ==================== HAL Interface Implementation ==================== */

void mb_uart_send_byte(uint8_t data)
{
#ifdef STM32F1
    USART1->DR = data;
    while (!(USART1->SR & USART_SR_TXE))
        ;
#else
    // Add your platform UART send here
#endif
}

void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        mb_uart_send_byte(buffer[i]);
    }
}

void mb_uart_enable_rx(bool enable)
{
#ifdef STM32F1
    if (enable) {
        USART1->CR1 |= USART_CR1_RXNEIE;
    } else {
        USART1->CR1 &= ~USART_CR1_RXNEIE;
    }
#else
    (void)enable;
#endif
}

uint32_t mb_get_time_ms(void)
{
    return g_system_ticks_ms;
}

/* ==================== UART Interrupt Handler ==================== */

void USART1_IRQHandler(void)
{
#ifdef STM32F1
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t data = USART1->DR;
        mb_slave_receive_byte(&g_modbus_slave, data);
    }
#endif
}

/* ==================== Application Callbacks ==================== */

void on_coil_write_callback(uint16_t address, bool value)
{
    // Example: Control LED on coil 0
    if (address == 0) {
#ifdef STM32F1
        if (value) {
            GPIOC->BSRR = GPIO_BSRR_BS13; // LED ON
        } else {
            GPIOC->BSRR = GPIO_BSRR_BR13; // LED OFF
        }
#endif
    }
}

void on_register_write_callback(uint16_t address, uint16_t value)
{
    // Example: Update PWM duty cycle on register 0
    if (address == 0) {
        // Update your PWM here
        // TIM2->CCR1 = value;
        (void)value;
    }
}

/* ==================== Hardware Initialization ==================== */

static void system_init(void)
{
#ifdef STM32F1
    // Configure SysTick for 1ms @ 72MHz
    SysTick_Config(72000);

    // Enable clocks
    RCC->APB2ENR |=
        RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;

    // Configure PA9 (TX) as alternate function push-pull
    GPIOA->CRH &= ~(0xF << 4);
    GPIOA->CRH |= (0xB << 4); // AF output push-pull, 50MHz

    // Configure PA10 (RX) as input floating
    GPIOA->CRH &= ~(0xF << 8);
    GPIOA->CRH |= (0x4 << 8); // Floating input

    // Configure PC13 (LED) as output push-pull
    GPIOC->CRH &= ~(0xF << 20);
    GPIOC->CRH |= (0x2 << 20); // Output push-pull, 2MHz

    // Configure USART1: 9600 baud, 8N1
    USART1->BRR = 0x1D4C; // 72MHz / 9600 = 7500 = 0x1D4C
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    // Enable USART1 interrupt
    NVIC_EnableIRQ(USART1_IRQn);
#endif
}

/* ==================== Application Logic ==================== */

static void update_sensors(void)
{
    static uint32_t last_update = 0;
    uint32_t now                = g_system_ticks_ms;

    // Update every 100ms
    if ((now - last_update) >= 100) {
        last_update = now;

        // Example: Read ADC (simulated here)
        uint16_t adc_value = (uint16_t)(now % 4096); // Simulated ADC
        mb_slave_set_input_register(&g_modbus_slave, 0, adc_value);

        // Example: Temperature sensor (simulated)
        uint16_t temperature = 250; // 25.0°C
        mb_slave_set_input_register(&g_modbus_slave, 1, temperature);

        // Example: Button state (simulated)
#ifdef STM32F1
        bool button_pressed = !(GPIOA->IDR & GPIO_IDR_IDR0);
        mb_slave_set_discrete(&g_modbus_slave, 0, button_pressed);
#endif
    }
}

/* ==================== Main Function ==================== */

int main(void)
{
    // Initialize hardware
    system_init();

    // Initialize Modbus slave (address 1, 9600 baud)
    mb_slave_init(&g_modbus_slave, 1, 9600);

    // Register callbacks
    mb_slave_set_coil_callback(&g_modbus_slave, on_coil_write_callback);
    mb_slave_set_register_callback(&g_modbus_slave, on_register_write_callback);

    // Set initial holding register values
    mb_slave_set_holding_register(&g_modbus_slave, 0, 1000); // PWM duty
    mb_slave_set_holding_register(&g_modbus_slave, 1, 500);  // Some parameter
    mb_slave_set_holding_register(&g_modbus_slave, 2, 100); // Another parameter

    // Main loop
    while (1) {
        // Poll Modbus for complete frames
        mb_slave_poll(&g_modbus_slave, g_system_ticks_ms);

        // Update sensor readings
        update_sensors();

        // Blink LED to show activity
        static uint32_t last_blink = 0;
        if ((g_system_ticks_ms - last_blink) >= 1000) {
            last_blink = g_system_ticks_ms;
#ifdef STM32F1
            GPIOC->ODR ^= GPIO_ODR_ODR13; // Toggle LED
#endif
        }
    }

    return 0;
}

/* ==================== Helper Functions ==================== */

/**
 * @brief Print statistics via debug UART
 * @note Call this from a debug console command
 */
void modbus_print_stats(void)
{
    uint32_t requests, exceptions, crc_errors;
    mb_slave_get_stats(&g_modbus_slave, &requests, &exceptions, &crc_errors);

    // Print via debug UART (not shown here)
    // printf("Modbus Stats: Req=%lu, Exc=%lu, CRC=%lu\n",
    //        requests, exceptions, crc_errors);
}
