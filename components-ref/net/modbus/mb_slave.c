/**
 * @file mb_slave.c
 * @brief Modbus RTU Slave Implementation
 * @version 1.0.0
 */

#include "mb_slave.h"
#include <string.h>

/* ==================== Private Functions ==================== */

static void mb_send_exception(mb_slave_t *slave, uint8_t function,
                              mb_exception_t exception);
static void mb_send_response(mb_slave_t *slave, uint16_t length);
static mb_exception_t mb_process_read_coils(mb_slave_t *slave);
static mb_exception_t mb_process_read_discrete(mb_slave_t *slave);
static mb_exception_t mb_process_read_holding_registers(mb_slave_t *slave);
static mb_exception_t mb_process_read_input_registers(mb_slave_t *slave);
static mb_exception_t mb_process_write_single_coil(mb_slave_t *slave);
static mb_exception_t mb_process_write_single_register(mb_slave_t *slave);
static mb_exception_t mb_process_write_multiple_coils(mb_slave_t *slave);
static mb_exception_t mb_process_write_multiple_registers(mb_slave_t *slave);

/* ==================== CRC16 Calculation ==================== */

uint16_t mb_crc16(const uint8_t *buffer, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/* ==================== Initialization ==================== */

int mb_slave_init(mb_slave_t *slave, uint8_t address, uint32_t baudrate)
{
    if (!slave || address > 247) {
        return -1;
    }

    memset(slave, 0, sizeof(mb_slave_t));

    slave->address  = address;
    slave->baudrate = baudrate;

    // Calculate frame timeout based on baud rate
    // T3.5 = 3.5 * (11 bits / baudrate) * 1000 ms
    slave->frame_timeout = (3500 * 11) / baudrate + 1;
    if (slave->frame_timeout < 2)
        slave->frame_timeout = 2;

    return 0;
}

/* ==================== Data Reception ==================== */

void mb_slave_receive_byte(mb_slave_t *slave, uint8_t data)
{
    uint32_t current_time = mb_get_time_ms();

    // Check for frame timeout (new frame)
    if (slave->rx_count > 0) {
        if ((current_time - slave->last_rx_time) > slave->frame_timeout) {
            slave->rx_count = 0; // Reset buffer
        }
    }

    slave->last_rx_time = current_time;

    // Add byte to buffer
    if (slave->rx_count < MB_RX_BUFFER_SIZE) {
        slave->rx_buffer[slave->rx_count++] = data;
    } else {
        // Buffer overflow, reset
        slave->rx_count = 0;
    }
}

/* ==================== Frame Processing ==================== */

void mb_slave_poll(mb_slave_t *slave, uint32_t current_time)
{
    // Check if frame is complete (no data for frame_timeout)
    if (slave->rx_count > 0) {
        if ((current_time - slave->last_rx_time) >= slave->frame_timeout) {
            mb_slave_process_frame(slave);
            slave->rx_count = 0;
        }
    }
}

void mb_slave_process_frame(mb_slave_t *slave)
{
    // Minimum frame: Address(1) + Function(1) + CRC(2) = 4 bytes
    if (slave->rx_count < 4) {
        return;
    }

    uint8_t address = slave->rx_buffer[0];

    // Check if frame is for this slave (or broadcast)
    if (address != slave->address && address != 0) {
        return;
    }

    // Verify CRC
    uint16_t received_crc = (slave->rx_buffer[slave->rx_count - 1] << 8)
                            | slave->rx_buffer[slave->rx_count - 2];
    uint16_t calculated_crc = mb_crc16(slave->rx_buffer, slave->rx_count - 2);

    if (received_crc != calculated_crc) {
        slave->crc_error_count++;
        return;
    }

    slave->request_count++;

    // Don't respond to broadcast (address 0)
    if (address == 0) {
        return;
    }

    uint8_t function         = slave->rx_buffer[1];
    mb_exception_t exception = MB_EX_NONE;

    // Process function code
    switch (function) {
    case MB_FC_READ_COILS:
        exception = mb_process_read_coils(slave);
        break;

    case MB_FC_READ_DISCRETE_INPUTS:
        exception = mb_process_read_discrete(slave);
        break;

    case MB_FC_READ_HOLDING_REGISTERS:
        exception = mb_process_read_holding_registers(slave);
        break;

    case MB_FC_READ_INPUT_REGISTERS:
        exception = mb_process_read_input_registers(slave);
        break;

    case MB_FC_WRITE_SINGLE_COIL:
        exception = mb_process_write_single_coil(slave);
        break;

    case MB_FC_WRITE_SINGLE_REGISTER:
        exception = mb_process_write_single_register(slave);
        break;

    case MB_FC_WRITE_MULTIPLE_COILS:
        exception = mb_process_write_multiple_coils(slave);
        break;

    case MB_FC_WRITE_MULTIPLE_REGISTERS:
        exception = mb_process_write_multiple_registers(slave);
        break;

    default:
        exception = MB_EX_ILLEGAL_FUNCTION;
        break;
    }

    if (exception != MB_EX_NONE) {
        mb_send_exception(slave, function, exception);
        slave->exception_count++;
    }
}

/* ==================== Function Implementations ==================== */

static mb_exception_t mb_process_read_coils(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (quantity < 1 || quantity > 2000)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_COIL_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    uint8_t byte_count = (quantity + 7) / 8;

    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_READ_COILS;
    slave->tx_buffer[2] = byte_count;

    // Pack coil values into bytes
    for (uint16_t i = 0; i < byte_count; i++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < 8 && (i * 8 + bit) < quantity; bit++) {
            if (mb_slave_get_coil(slave, start_addr + i * 8 + bit)) {
                byte |= (1 << bit);
            }
        }
        slave->tx_buffer[3 + i] = byte;
    }

    mb_send_response(slave, 3 + byte_count);
    return MB_EX_NONE;
}

static mb_exception_t mb_process_read_discrete(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (quantity < 1 || quantity > 2000)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_DISCRETE_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    uint8_t byte_count = (quantity + 7) / 8;

    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_READ_DISCRETE_INPUTS;
    slave->tx_buffer[2] = byte_count;

    // Pack discrete input values into bytes
    for (uint16_t i = 0; i < byte_count; i++) {
        uint8_t byte = 0;
        for (uint8_t bit = 0; bit < 8 && (i * 8 + bit) < quantity; bit++) {
            if (mb_slave_get_discrete(slave, start_addr + i * 8 + bit)) {
                byte |= (1 << bit);
            }
        }
        slave->tx_buffer[3 + i] = byte;
    }

    mb_send_response(slave, 3 + byte_count);
    return MB_EX_NONE;
}

static mb_exception_t mb_process_read_holding_registers(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (quantity < 1 || quantity > 125)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_HOLDING_REG_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_READ_HOLDING_REGISTERS;
    slave->tx_buffer[2] = quantity * 2;

    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t value = mb_slave_get_holding_register(slave, start_addr + i);
        slave->tx_buffer[3 + i * 2] = (value >> 8) & 0xFF;
        slave->tx_buffer[4 + i * 2] = value & 0xFF;
    }

    mb_send_response(slave, 3 + quantity * 2);
    return MB_EX_NONE;
}

static mb_exception_t mb_process_read_input_registers(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (quantity < 1 || quantity > 125)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_INPUT_REG_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_READ_INPUT_REGISTERS;
    slave->tx_buffer[2] = quantity * 2;

    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t value = mb_slave_get_input_register(slave, start_addr + i);
        slave->tx_buffer[3 + i * 2] = (value >> 8) & 0xFF;
        slave->tx_buffer[4 + i * 2] = value & 0xFF;
    }

    mb_send_response(slave, 3 + quantity * 2);
    return MB_EX_NONE;
}

static mb_exception_t mb_process_write_single_coil(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t address = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t value   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (value != 0x0000 && value != 0xFF00)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (address >= MB_COIL_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    mb_slave_set_coil(slave, address, value == 0xFF00);

    // Echo request as response
    memcpy(slave->tx_buffer, slave->rx_buffer, 6);
    mb_send_response(slave, 6);

    return MB_EX_NONE;
}

static mb_exception_t mb_process_write_single_register(mb_slave_t *slave)
{
    if (slave->rx_count < 6)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t address = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t value   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];

    if (address >= MB_HOLDING_REG_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;

    mb_slave_set_holding_register(slave, address, value);

    // Echo request as response
    memcpy(slave->tx_buffer, slave->rx_buffer, 6);
    mb_send_response(slave, 6);

    return MB_EX_NONE;
}

static mb_exception_t mb_process_write_multiple_coils(mb_slave_t *slave)
{
    if (slave->rx_count < 7)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];
    uint8_t byte_count  = slave->rx_buffer[6];

    if (quantity < 1 || quantity > 1968)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (byte_count != (quantity + 7) / 8)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_COIL_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;
    if (slave->rx_count < 7 + byte_count)
        return MB_EX_ILLEGAL_DATA_VALUE;

    // Write coil values
    for (uint16_t i = 0; i < quantity; i++) {
        uint8_t byte_idx = i / 8;
        uint8_t bit_idx  = i % 8;
        bool value = (slave->rx_buffer[7 + byte_idx] & (1 << bit_idx)) != 0;
        mb_slave_set_coil(slave, start_addr + i, value);
    }

    // Response: Address + Function + Start Address + Quantity
    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_WRITE_MULTIPLE_COILS;
    slave->tx_buffer[2] = (start_addr >> 8) & 0xFF;
    slave->tx_buffer[3] = start_addr & 0xFF;
    slave->tx_buffer[4] = (quantity >> 8) & 0xFF;
    slave->tx_buffer[5] = quantity & 0xFF;

    mb_send_response(slave, 6);
    return MB_EX_NONE;
}

static mb_exception_t mb_process_write_multiple_registers(mb_slave_t *slave)
{
    if (slave->rx_count < 7)
        return MB_EX_ILLEGAL_DATA_VALUE;

    uint16_t start_addr = (slave->rx_buffer[2] << 8) | slave->rx_buffer[3];
    uint16_t quantity   = (slave->rx_buffer[4] << 8) | slave->rx_buffer[5];
    uint8_t byte_count  = slave->rx_buffer[6];

    if (quantity < 1 || quantity > 123)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (byte_count != quantity * 2)
        return MB_EX_ILLEGAL_DATA_VALUE;
    if (start_addr + quantity > MB_HOLDING_REG_COUNT)
        return MB_EX_ILLEGAL_DATA_ADDRESS;
    if (slave->rx_count < 7 + byte_count)
        return MB_EX_ILLEGAL_DATA_VALUE;

    // Write register values
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t value =
            (slave->rx_buffer[7 + i * 2] << 8) | slave->rx_buffer[8 + i * 2];
        mb_slave_set_holding_register(slave, start_addr + i, value);
    }

    // Response: Address + Function + Start Address + Quantity
    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = MB_FC_WRITE_MULTIPLE_REGISTERS;
    slave->tx_buffer[2] = (start_addr >> 8) & 0xFF;
    slave->tx_buffer[3] = start_addr & 0xFF;
    slave->tx_buffer[4] = (quantity >> 8) & 0xFF;
    slave->tx_buffer[5] = quantity & 0xFF;

    mb_send_response(slave, 6);
    return MB_EX_NONE;
}

/* ==================== Response Functions ==================== */

static void mb_send_exception(mb_slave_t *slave, uint8_t function,
                              mb_exception_t exception)
{
    slave->tx_buffer[0] = slave->address;
    slave->tx_buffer[1] = function | 0x80; // Set MSB for exception
    slave->tx_buffer[2] = exception;

    mb_send_response(slave, 3);
}

static void mb_send_response(mb_slave_t *slave, uint16_t length)
{
    // Add CRC
    uint16_t crc                 = mb_crc16(slave->tx_buffer, length);
    slave->tx_buffer[length]     = crc & 0xFF;
    slave->tx_buffer[length + 1] = (crc >> 8) & 0xFF;

    slave->tx_count = length + 2;

    // Send via UART
    mb_uart_send_buffer(slave->tx_buffer, slave->tx_count);
}

/* ==================== Data Access Functions ==================== */

int mb_slave_set_coil(mb_slave_t *slave, uint16_t address, bool value)
{
    if (address >= MB_COIL_COUNT)
        return -1;

    uint16_t byte_idx = address / 8;
    uint8_t bit_idx   = address % 8;

    if (value) {
        slave->coils[byte_idx] |= (1 << bit_idx);
    } else {
        slave->coils[byte_idx] &= ~(1 << bit_idx);
    }

    if (slave->on_coil_write) {
        slave->on_coil_write(address, value);
    }

    return 0;
}

bool mb_slave_get_coil(mb_slave_t *slave, uint16_t address)
{
    if (address >= MB_COIL_COUNT)
        return false;

    uint16_t byte_idx = address / 8;
    uint8_t bit_idx   = address % 8;

    return (slave->coils[byte_idx] & (1 << bit_idx)) != 0;
}

int mb_slave_set_discrete(mb_slave_t *slave, uint16_t address, bool value)
{
    if (address >= MB_DISCRETE_COUNT)
        return -1;

    uint16_t byte_idx = address / 8;
    uint8_t bit_idx   = address % 8;

    if (value) {
        slave->discrete[byte_idx] |= (1 << bit_idx);
    } else {
        slave->discrete[byte_idx] &= ~(1 << bit_idx);
    }

    return 0;
}

bool mb_slave_get_discrete(mb_slave_t *slave, uint16_t address)
{
    if (address >= MB_DISCRETE_COUNT)
        return false;

    uint16_t byte_idx = address / 8;
    uint8_t bit_idx   = address % 8;

    return (slave->discrete[byte_idx] & (1 << bit_idx)) != 0;
}

int mb_slave_set_input_register(mb_slave_t *slave, uint16_t address,
                                uint16_t value)
{
    if (address >= MB_INPUT_REG_COUNT)
        return -1;

    slave->input_regs[address] = value;
    return 0;
}

uint16_t mb_slave_get_input_register(mb_slave_t *slave, uint16_t address)
{
    if (address >= MB_INPUT_REG_COUNT)
        return 0;

    return slave->input_regs[address];
}

int mb_slave_set_holding_register(mb_slave_t *slave, uint16_t address,
                                  uint16_t value)
{
    if (address >= MB_HOLDING_REG_COUNT)
        return -1;

    slave->holding_regs[address] = value;

    if (slave->on_register_write) {
        slave->on_register_write(address, value);
    }

    return 0;
}

uint16_t mb_slave_get_holding_register(mb_slave_t *slave, uint16_t address)
{
    if (address >= MB_HOLDING_REG_COUNT)
        return 0;

    return slave->holding_regs[address];
}

/* ==================== Callback Functions ==================== */

void mb_slave_set_coil_callback(mb_slave_t *slave,
                                void (*callback)(uint16_t addr, bool value))
{
    slave->on_coil_write = callback;
}

void mb_slave_set_register_callback(mb_slave_t *slave,
                                    void (*callback)(uint16_t addr,
                                                     uint16_t value))
{
    slave->on_register_write = callback;
}

/* ==================== Statistics ==================== */

void mb_slave_get_stats(mb_slave_t *slave, uint32_t *requests,
                        uint32_t *exceptions, uint32_t *crc_errors)
{
    if (requests)
        *requests = slave->request_count;
    if (exceptions)
        *exceptions = slave->exception_count;
    if (crc_errors)
        *crc_errors = slave->crc_error_count;
}

void mb_slave_reset_stats(mb_slave_t *slave)
{
    slave->request_count   = 0;
    slave->exception_count = 0;
    slave->crc_error_count = 0;
}
