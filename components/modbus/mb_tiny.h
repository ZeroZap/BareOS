/**
 * @file mb_tiny.h
 * @brief Nano Modbus Tiny - RTU ADU codec with synchronous master wrappers
 * @version 1.1.0
 * @date 2026-08-23
 */

#ifndef MB_TINY_H
#define MB_TINY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_typedef.h"
#include <stdint.h>

/* ==================== Configuration ==================== */

#ifndef MB_TINY_MAX_ADU_SIZE
#define MB_TINY_MAX_ADU_SIZE 128U
#endif

#ifndef MB_TINY_TIMEOUT_MS
#define MB_TINY_TIMEOUT_MS 1000U
#endif

#define MB_TINY_MIN_ADU_SIZE 4U
#define MB_TINY_MAX_READ_REGS ((MB_TINY_MAX_ADU_SIZE - 5U) / 2U)
#define MB_TINY_MAX_READ_BITS ((MB_TINY_MAX_ADU_SIZE - 5U) * 8U)
#define MB_TINY_MAX_WRITE_REGS ((MB_TINY_MAX_ADU_SIZE - 9U) / 2U)
#define MB_TINY_MAX_WRITE_BITS ((MB_TINY_MAX_ADU_SIZE - 9U) * 8U)

/* ==================== Local status codes ==================== */

#define MB_TINY_OK 0
#define MB_TINY_ERROR (-1)
#define MB_TINY_INVALID_PARAM (-2)
#define MB_TINY_TIMEOUT (-3)
#define MB_TINY_CRC_ERROR (-4)
#define MB_TINY_FRAME_ERROR (-5)
#define MB_TINY_IO_ERROR (-6)
#define MB_TINY_EXCEPTION (-7)
#define MB_TINY_NOT_INITIALIZED (-8)
#define MB_TINY_BUFFER_TOO_SMALL (-9)

#define MB_TINY_IGNORED 1
#define MB_TINY_NO_RESPONSE 2
#define MB_TINY_FRAME_READY 3

/* ==================== Function codes ==================== */

#define MB_FUNC_READ_COILS 0x01U
#define MB_FUNC_READ_DISCRETE 0x02U
#define MB_FUNC_READ_HOLDING 0x03U
#define MB_FUNC_READ_INPUT 0x04U
#define MB_FUNC_WRITE_SINGLE_COIL 0x05U
#define MB_FUNC_WRITE_SINGLE_REG 0x06U
#define MB_FUNC_WRITE_MULTIPLE_COILS 0x0FU
#define MB_FUNC_WRITE_MULTIPLE_REGS 0x10U

/* ==================== Modbus exception codes ==================== */

#define MB_ERR_ILLEGAL_FUNC 0x01U
#define MB_ERR_ILLEGAL_DATA_ADDR 0x02U
#define MB_ERR_ILLEGAL_DATA_VALUE 0x03U
#define MB_ERR_DEVICE_FAILURE 0x04U
#define MB_ERR_DEVICE_BUSY 0x06U

/* ==================== Serial callbacks ==================== */

/**
 * The callback must consume or copy all data before returning. A positive
 * return value is the number of bytes sent; short writes are treated as IO
 * errors by the synchronous API.
 */
typedef int (*mb_tiny_send_cb_t)(const uint8_t *data, uint16_t len);

/**
 * Receive one complete RTU ADU into data. len is the buffer capacity.
 * Return the received length, zero on timeout, or a negative driver error.
 */
typedef int (*mb_tiny_recv_cb_t)(uint8_t *data, uint16_t len,
                                 uint32_t timeout_ms);

/* ==================== Data maps ==================== */

typedef struct {
  uint16_t *data;
  uint16_t start_addr;
  uint16_t count;
} mb_tiny_holding_t;

typedef mb_tiny_holding_t mb_tiny_input_t;

/**
 * Bit-packed coil map. data[0] bit 0 represents start_addr, bit 1 represents
 * start_addr + 1, and so on.
 */
typedef struct {
  uint8_t *data;
  uint16_t start_addr;
  uint16_t count;
} mb_tiny_coils_t;

typedef mb_tiny_coils_t mb_tiny_discrete_t;

/* ==================== Instances ==================== */

typedef struct {
  uint8_t slave_id;
  mb_tiny_holding_t holding;
  mb_tiny_input_t input;
  mb_tiny_coils_t coils;
  mb_tiny_discrete_t discrete;

  /* Retained for source compatibility and future RTU stream transport. */
  uint8_t rx_buf[MB_TINY_MAX_ADU_SIZE];
  uint16_t rx_len;

  mb_tiny_send_cb_t send_cb;
  uint32_t request_count;
  uint32_t error_count;
  bool initialized;
} mb_tiny_slave_t;

typedef struct {
  uint8_t data[MB_TINY_MAX_ADU_SIZE];
  uint16_t len;
  uint16_t frame_gap_ms;
  uint32_t last_byte_ms;
  uint32_t dropped_frames;
  bool receiving;
  bool overflow;
} mb_tiny_rtu_rx_t;

typedef struct {
  uint8_t slave_id;
  uint32_t timeout_ms;

  /* Retained for source compatibility and future non-blocking transport. */
  uint8_t tx_buf[MB_TINY_MAX_ADU_SIZE];
  uint8_t rx_buf[MB_TINY_MAX_ADU_SIZE];
  uint16_t tx_len;
  uint16_t rx_len;

  mb_tiny_send_cb_t send_cb;
  mb_tiny_recv_cb_t recv_cb;
  uint8_t last_exception;
  uint32_t request_count;
  uint32_t error_count;
  bool initialized;
} mb_tiny_master_t;

/* ==================== RTU receive framing API ==================== */

/**
 * Return a conservative whole-millisecond t3.5 interval. bits_per_char includes
 * start, data, parity (when used), and stop bits and must be in 8..12.
 */
uint16_t mb_tiny_rtu_frame_gap_ms(uint32_t baud_rate, uint8_t bits_per_char);

/**
 * Initialize a main-loop RTU receiver. frame_gap_ms is the rounded-up t3.5
 * silence interval and must be nonzero.
 */
int mb_tiny_rtu_rx_init(mb_tiny_rtu_rx_t *rx, uint16_t frame_gap_ms);

/**
 * Feed one UART byte in main-loop context with its receive timestamp. The UART
 * ISR should only place bytes and timestamps in an application-owned ring.
 */
int mb_tiny_rtu_rx_feed(mb_tiny_rtu_rx_t *rx, uint8_t byte, uint32_t now_ms);

/**
 * Finish a frame after t3.5 silence. Returns MB_TINY_FRAME_READY and copies the
 * ADU on success, MB_TINY_IGNORED while receiving, or a local error.
 */
int mb_tiny_rtu_rx_poll(mb_tiny_rtu_rx_t *rx, uint32_t now_ms, uint8_t *frame,
                        uint16_t frame_capacity, uint16_t *frame_len);

void mb_tiny_rtu_rx_reset(mb_tiny_rtu_rx_t *rx);
bool mb_tiny_rtu_rx_is_idle(const mb_tiny_rtu_rx_t *rx);

/* ==================== Slave API ==================== */

int mb_tiny_slave_init(mb_tiny_slave_t *slave, uint8_t slave_id);
int mb_tiny_slave_config_holding(mb_tiny_slave_t *slave, uint16_t *data,
                                 uint16_t start_addr, uint16_t count);
int mb_tiny_slave_config_coils(mb_tiny_slave_t *slave, uint8_t *data,
                               uint16_t start_addr, uint16_t count);
int mb_tiny_slave_config_input(mb_tiny_slave_t *slave, uint16_t *data,
                               uint16_t start_addr, uint16_t count);
int mb_tiny_slave_config_discrete(mb_tiny_slave_t *slave, uint8_t *data,
                                  uint16_t start_addr, uint16_t count);
void mb_tiny_slave_set_send(mb_tiny_slave_t *slave, mb_tiny_send_cb_t send_cb);

/**
 * Process one complete RTU request ADU without performing any IO.
 *
 * response must not overlap request. On MB_TINY_OK or MB_TINY_EXCEPTION,
 * response_len contains the generated ADU length. MB_TINY_IGNORED and
 * MB_TINY_NO_RESPONSE return a zero response length.
 */
int mb_tiny_slave_process(mb_tiny_slave_t *slave, const uint8_t *request,
                          uint16_t request_len, uint8_t *response,
                          uint16_t response_capacity, uint16_t *response_len);

/**
 * Compatibility wrapper around mb_tiny_slave_process(). Generated responses
 * are sent through the instance send callback.
 */
int mb_tiny_slave_handle(mb_tiny_slave_t *slave, const uint8_t *data,
                         uint16_t len);

/* ==================== Master API ==================== */

int mb_tiny_master_init(mb_tiny_master_t *master);
void mb_tiny_master_set_uart(mb_tiny_master_t *master,
                             mb_tiny_send_cb_t send_cb,
                             mb_tiny_recv_cb_t recv_cb);
void mb_tiny_master_set_timeout(mb_tiny_master_t *master, uint32_t timeout_ms);

int mb_tiny_master_read_holding(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, uint16_t *data);
int mb_tiny_master_read_holding_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                   uint16_t addr, uint16_t count,
                                   uint16_t *data, uint16_t data_capacity);
int mb_tiny_master_read_input(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint16_t *data);
int mb_tiny_master_read_input_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data,
                                 uint16_t data_capacity);
int mb_tiny_master_write_reg(mb_tiny_master_t *master, uint8_t slave_id,
                             uint16_t addr, uint16_t value);
int mb_tiny_master_write_regs(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count,
                              const uint16_t *data);
int mb_tiny_master_read_coils(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint8_t *data);
int mb_tiny_master_read_coils_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint8_t *data,
                                 uint16_t data_capacity);
int mb_tiny_master_read_discrete(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint8_t *data);
int mb_tiny_master_read_discrete_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count,
                                    uint8_t *data, uint16_t data_capacity);
int mb_tiny_master_write_coil(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t value);
int mb_tiny_master_write_coils(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count,
                               const uint8_t *data);

/* ==================== Utility API ==================== */

uint16_t mb_tiny_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* MB_TINY_H */
