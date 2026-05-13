/**
 * @file mb_slave.h
 * @brief Modbus RTU Slave Implementation for Microcontrollers
 * @version 1.0.0
 * @date 2025-10-27
 *
 * @note This implementation provides a lightweight Modbus RTU slave suitable
 * for resource-constrained embedded systems.
 */

#ifndef _MB_SLAVE_H_
#define _MB_SLAVE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef MB_SLAVE_ADDRESS
#define MB_SLAVE_ADDRESS 1 ///< Default slave address (1-247)
#endif

#ifndef MB_UART_BAUDRATE
#define MB_UART_BAUDRATE 9600 ///< Default baud rate
#endif

#ifndef MB_COIL_COUNT
#define MB_COIL_COUNT 64 ///< Number of coils (0x)
#endif

#ifndef MB_DISCRETE_COUNT
#define MB_DISCRETE_COUNT 64 ///< Number of discrete inputs (1x)
#endif

#ifndef MB_INPUT_REG_COUNT
#define MB_INPUT_REG_COUNT 32 ///< Number of input registers (3x)
#endif

#ifndef MB_HOLDING_REG_COUNT
#define MB_HOLDING_REG_COUNT 32 ///< Number of holding registers (4x)
#endif

#ifndef MB_RX_BUFFER_SIZE
#define MB_RX_BUFFER_SIZE 256 ///< Receive buffer size
#endif

#ifndef MB_TX_BUFFER_SIZE
#define MB_TX_BUFFER_SIZE 256 ///< Transmit buffer size
#endif

/* ==================== Modbus Function Codes ==================== */

#define MB_FC_READ_COILS               0x01
#define MB_FC_READ_DISCRETE_INPUTS     0x02
#define MB_FC_READ_HOLDING_REGISTERS   0x03
#define MB_FC_READ_INPUT_REGISTERS     0x04
#define MB_FC_WRITE_SINGLE_COIL        0x05
#define MB_FC_WRITE_SINGLE_REGISTER    0x06
#define MB_FC_WRITE_MULTIPLE_COILS     0x0F
#define MB_FC_WRITE_MULTIPLE_REGISTERS 0x10

/* ==================== Exception Codes ==================== */

typedef enum {
    MB_EX_NONE                 = 0x00,
    MB_EX_ILLEGAL_FUNCTION     = 0x01,
    MB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
    MB_EX_ILLEGAL_DATA_VALUE   = 0x03,
    MB_EX_SLAVE_DEVICE_FAILURE = 0x04,
    MB_EX_ACKNOWLEDGE          = 0x05,
    MB_EX_SLAVE_BUSY           = 0x06,
    MB_EX_MEMORY_PARITY_ERROR  = 0x08,
    MB_EX_GATEWAY_PATH_FAILED  = 0x0A,
    MB_EX_GATEWAY_TGT_FAILED   = 0x0B
} mb_exception_t;

/* ==================== Data Structures ==================== */

/**
 * @brief Modbus RTU slave context
 */
typedef struct {
    uint8_t address;   ///< Slave address (1-247)
    uint32_t baudrate; ///< UART baud rate

    // Data storage
    uint8_t coils[MB_COIL_COUNT / 8 + 1];        ///< Coil状态 (0x, RW)
    uint8_t discrete[MB_DISCRETE_COUNT / 8 + 1]; ///< Discrete inputs (1x, RO)
    uint16_t input_regs[MB_INPUT_REG_COUNT];     ///< Input registers (3x, RO)
    uint16_t holding_regs[MB_HOLDING_REG_COUNT]; ///< Holding registers (4x, RW)

    // Communication buffers
    uint8_t rx_buffer[MB_RX_BUFFER_SIZE];
    uint16_t rx_count;
    uint8_t tx_buffer[MB_TX_BUFFER_SIZE];
    uint16_t tx_count;

    // Timing
    uint32_t last_rx_time;  ///< Last receive timestamp (ms)
    uint32_t frame_timeout; ///< Frame timeout (ms)

    // Statistics
    uint32_t request_count;   ///< Total requests received
    uint32_t exception_count; ///< Total exceptions sent
    uint32_t crc_error_count; ///< CRC errors

    // Callbacks (optional)
    void (*on_coil_write)(uint16_t addr, bool value);
    void (*on_register_write)(uint16_t addr, uint16_t value);
} mb_slave_t;

/* ==================== Public Functions ==================== */

/**
 * @brief Initialize Modbus RTU slave
 * @param slave Pointer to slave context
 * @param address Slave address (1-247, 0 = broadcast)
 * @param baudrate UART baud rate
 * @return 0 on success, -1 on error
 */
int mb_slave_init(mb_slave_t *slave, uint8_t address, uint32_t baudrate);

/**
 * @brief Process received data (call from UART RX interrupt or polling)
 * @param slave Pointer to slave context
 * @param data Received byte
 */
void mb_slave_receive_byte(mb_slave_t *slave, uint8_t data);

/**
 * @brief Poll slave (call periodically from main loop)
 * @param slave Pointer to slave context
 * @param current_time Current system time in milliseconds
 */
void mb_slave_poll(mb_slave_t *slave, uint32_t current_time);

/**
 * @brief Process complete frame (called internally or manually)
 * @param slave Pointer to slave context
 */
void mb_slave_process_frame(mb_slave_t *slave);

/* ==================== Data Access Functions ==================== */

/**
 * @brief Set coil value
 * @param slave Pointer to slave context
 * @param address Coil address (0-based)
 * @param value Coil value (true/false)
 * @return 0 on success, -1 on error
 */
int mb_slave_set_coil(mb_slave_t *slave, uint16_t address, bool value);

/**
 * @brief Get coil value
 * @param slave Pointer to slave context
 * @param address Coil address (0-based)
 * @return Coil value (true/false)
 */
bool mb_slave_get_coil(mb_slave_t *slave, uint16_t address);

/**
 * @brief Set discrete input value
 * @param slave Pointer to slave context
 * @param address Discrete input address (0-based)
 * @param value Input value (true/false)
 * @return 0 on success, -1 on error
 */
int mb_slave_set_discrete(mb_slave_t *slave, uint16_t address, bool value);

/**
 * @brief Get discrete input value
 * @param slave Pointer to slave context
 * @param address Discrete input address (0-based)
 * @return Discrete input value (true/false)
 */
bool mb_slave_get_discrete(mb_slave_t *slave, uint16_t address);

/**
 * @brief Set input register value
 * @param slave Pointer to slave context
 * @param address Register address (0-based)
 * @param value Register value
 * @return 0 on success, -1 on error
 */
int mb_slave_set_input_register(mb_slave_t *slave, uint16_t address,
                                uint16_t value);

/**
 * @brief Get input register value
 * @param slave Pointer to slave context
 * @param address Register address (0-based)
 * @return Register value
 */
uint16_t mb_slave_get_input_register(mb_slave_t *slave, uint16_t address);

/**
 * @brief Set holding register value
 * @param slave Pointer to slave context
 * @param address Register address (0-based)
 * @param value Register value
 * @return 0 on success, -1 on error
 */
int mb_slave_set_holding_register(mb_slave_t *slave, uint16_t address,
                                  uint16_t value);

/**
 * @brief Get holding register value
 * @param slave Pointer to slave context
 * @param address Register address (0-based)
 * @return Register value
 */
uint16_t mb_slave_get_holding_register(mb_slave_t *slave, uint16_t address);

/**
 * @brief Set coil write callback
 * @param slave Pointer to slave context
 * @param callback Callback function (addr, value)
 */
void mb_slave_set_coil_callback(mb_slave_t *slave,
                                void (*callback)(uint16_t addr, bool value));

/**
 * @brief Set register write callback
 * @param slave Pointer to slave context
 * @param callback Callback function (addr, value)
 */
void mb_slave_set_register_callback(mb_slave_t *slave,
                                    void (*callback)(uint16_t addr,
                                                     uint16_t value));

/* ==================== Utility Functions ==================== */

/**
 * @brief Calculate Modbus RTU CRC16
 * @param buffer Data buffer
 * @param length Data length
 * @return CRC16 value
 */
uint16_t mb_crc16(const uint8_t *buffer, uint16_t length);

/**
 * @brief Get slave statistics
 * @param slave Pointer to slave context
 * @param requests Output: total requests
 * @param exceptions Output: total exceptions
 * @param crc_errors Output: CRC errors
 */
void mb_slave_get_stats(mb_slave_t *slave, uint32_t *requests,
                        uint32_t *exceptions, uint32_t *crc_errors);

/**
 * @brief Reset slave statistics
 * @param slave Pointer to slave context
 */
void mb_slave_reset_stats(mb_slave_t *slave);

/* ==================== Hardware Interface (User Must Implement)
 * ==================== */

/**
 * @brief Send byte via UART (user must implement)
 * @param data Byte to send
 */
extern void mb_uart_send_byte(uint8_t data);

/**
 * @brief Send buffer via UART (user must implement)
 * @param buffer Data buffer
 * @param length Data length
 */
extern void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length);

/**
 * @brief Enable/disable UART receiver (user must implement)
 * @param enable true to enable, false to disable
 */
extern void mb_uart_enable_rx(bool enable);

/**
 * @brief Get system time in milliseconds (user must implement)
 * @return Current time in milliseconds
 */
extern uint32_t mb_get_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* _MB_SLAVE_H_ */
