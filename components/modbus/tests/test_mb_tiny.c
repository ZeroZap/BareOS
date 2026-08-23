#include <stdio.h>
#include <string.h>

#include "mb_tiny.h"

static int g_failures;
static uint8_t g_slave_tx_a[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_slave_tx_a_len;
static uint8_t g_slave_tx_b[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_slave_tx_b_len;
static uint8_t g_master_response[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_master_response_len;
static uint8_t g_master_request[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_master_request_len;
static int g_master_send_mode;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);              \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))

static uint16_t append_crc(uint8_t *frame, uint16_t payload_len) {
  uint16_t crc = mb_tiny_crc16(frame, payload_len);
  frame[payload_len] = (uint8_t)crc;
  frame[payload_len + 1U] = (uint8_t)(crc >> 8U);
  return (uint16_t)(payload_len + 2U);
}

static int slave_send_a(const uint8_t *data, uint16_t len) {
  memcpy(g_slave_tx_a, data, len);
  g_slave_tx_a_len = len;
  return len;
}

static int slave_send_b(const uint8_t *data, uint16_t len) {
  memcpy(g_slave_tx_b, data, len);
  g_slave_tx_b_len = len;
  return len;
}

static int master_send(const uint8_t *data, uint16_t len) {
  memcpy(g_master_request, data, len);
  g_master_request_len = len;
  if (g_master_send_mode != 0) {
    return (int)len - 1;
  }
  return len;
}

static int master_recv(uint8_t *data, uint16_t capacity, uint32_t timeout_ms) {
  (void)timeout_ms;
  if (g_master_response_len > capacity) {
    return -1;
  }
  memcpy(data, g_master_response, g_master_response_len);
  return g_master_response_len;
}

static void set_master_response(const uint8_t *payload, uint16_t payload_len) {
  memcpy(g_master_response, payload, payload_len);
  g_master_response_len = append_crc(g_master_response, payload_len);
}

static void test_crc(void) {
  static const uint8_t request[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
  CHECK_EQ(mb_tiny_crc16(request, sizeof(request)), 0xCDC5U);
}

static void test_rtu_receive_framing(void) {
  mb_tiny_rtu_rx_t rx;
  uint8_t frame[MB_TINY_MAX_ADU_SIZE];
  uint16_t frame_len;
  uint16_t i;

  CHECK_EQ(mb_tiny_rtu_frame_gap_ms(0U, 11U), 0U);
  CHECK_EQ(mb_tiny_rtu_frame_gap_ms(9600U, 7U), 0U);
  CHECK_EQ(mb_tiny_rtu_frame_gap_ms(9600U, 11U), 5U);
  CHECK_EQ(mb_tiny_rtu_frame_gap_ms(19200U, 11U), 3U);
  CHECK_EQ(mb_tiny_rtu_frame_gap_ms(38400U, 11U), 2U);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 0U), MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 4U), MB_TINY_OK);
  CHECK(mb_tiny_rtu_rx_is_idle(&rx));
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 1U, 10U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 3U, 11U), MB_TINY_OK);
  CHECK(!mb_tiny_rtu_rx_is_idle(&rx));
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 14U, frame, sizeof(frame), &frame_len),
           MB_TINY_IGNORED);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0U, 14U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0U, 15U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 19U, frame, 3U, &frame_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK(!mb_tiny_rtu_rx_is_idle(&rx));
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 19U, frame, sizeof(frame), &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[0], 1U);
  CHECK_EQ(frame[1], 3U);
  CHECK(mb_tiny_rtu_rx_is_idle(&rx));

  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0xAAU, UINT32_MAX - 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0xBBU, UINT32_MAX), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0xCCU, 0U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0xDDU, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 5U, frame, sizeof(frame), &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[3], 0xDDU);

  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 1U, 20U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 2U, 21U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 25U, frame, sizeof(frame), &frame_len),
           MB_TINY_FRAME_ERROR);
  CHECK_EQ(rx.dropped_frames, 1U);

  for (i = 0U; i < MB_TINY_MAX_ADU_SIZE; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, (uint8_t)i, (uint32_t)(30U + i)),
             MB_TINY_OK);
  }
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, (uint32_t)(34U + MB_TINY_MAX_ADU_SIZE),
                               frame, sizeof(frame), &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, MB_TINY_MAX_ADU_SIZE);
  CHECK_EQ(frame[MB_TINY_MAX_ADU_SIZE - 1U],
           (uint8_t)(MB_TINY_MAX_ADU_SIZE - 1U));

  for (i = 0U; i <= MB_TINY_MAX_ADU_SIZE; i++) {
    int ret = mb_tiny_rtu_rx_feed(&rx, (uint8_t)i, (uint32_t)(200U + i));
    CHECK_EQ(ret,
             i < MB_TINY_MAX_ADU_SIZE ? MB_TINY_OK : MB_TINY_BUFFER_TOO_SMALL);
  }
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, (uint32_t)(204U + MB_TINY_MAX_ADU_SIZE),
                               frame, sizeof(frame), &frame_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(rx.dropped_frames, 2U);
  CHECK(mb_tiny_rtu_rx_is_idle(&rx));

  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x11U, 400U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x22U, 401U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x33U, 402U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x44U, 406U), MB_TINY_FRAME_ERROR);
  CHECK_EQ(rx.dropped_frames, 3U);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x55U, 407U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x66U, 408U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_feed(&rx, 0x77U, 409U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_poll(&rx, 413U, frame, sizeof(frame), &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[0], 0x44U);
  CHECK_EQ(frame[3], 0x77U);
}

static void test_slave_registers_and_limits(void) {
  mb_tiny_slave_t slave;
  uint16_t registers[64];
  uint8_t request[MB_TINY_MAX_ADU_SIZE];
  uint16_t len;
  uint16_t i;

  for (i = 0U; i < 64U; i++) {
    registers[i] = (uint16_t)(0x1000U + i);
  }
  CHECK_EQ(mb_tiny_slave_init(&slave, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave, registers, 0U, 64U),
           MB_TINY_OK);
  mb_tiny_slave_set_send(&slave, slave_send_a);

  request[0] = 1U;
  request[1] = MB_FUNC_READ_HOLDING;
  request[2] = 0U;
  request[3] = 0U;
  request[4] = 0U;
  request[5] = (uint8_t)MB_TINY_MAX_READ_REGS;
  len = append_crc(request, 6U);
  g_slave_tx_a_len = 0U;
  CHECK_EQ(mb_tiny_slave_handle(&slave, request, len), MB_TINY_OK);
  CHECK_EQ(g_slave_tx_a_len, (uint16_t)(5U + MB_TINY_MAX_READ_REGS * 2U));
  CHECK_EQ(g_slave_tx_a[2], (uint8_t)(MB_TINY_MAX_READ_REGS * 2U));

  request[5] = (uint8_t)(MB_TINY_MAX_READ_REGS + 1U);
  len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, request, len), MB_TINY_EXCEPTION);
  CHECK_EQ(g_slave_tx_a_len, 5U);
  CHECK_EQ(g_slave_tx_a[1], (uint8_t)(MB_FUNC_READ_HOLDING | 0x80U));
  CHECK_EQ(g_slave_tx_a[2], MB_ERR_ILLEGAL_DATA_VALUE);
}

static void test_slave_coil_alignment_and_validation(void) {
  mb_tiny_slave_t slave;
  uint8_t coils[2] = {0x0DU, 0U};
  uint8_t request[8];
  uint16_t len;

  CHECK_EQ(mb_tiny_slave_init(&slave, 7U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_coils(&slave, coils, 3U, 9U), MB_TINY_OK);
  mb_tiny_slave_set_send(&slave, slave_send_a);

  request[0] = 7U;
  request[1] = MB_FUNC_READ_COILS;
  request[2] = 0U;
  request[3] = 3U;
  request[4] = 0U;
  request[5] = 5U;
  len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, request, len), MB_TINY_OK);
  CHECK_EQ(g_slave_tx_a_len, 6U);
  CHECK_EQ(g_slave_tx_a[2], 1U);
  CHECK_EQ(g_slave_tx_a[3], 0x0DU);

  request[1] = MB_FUNC_WRITE_SINGLE_COIL;
  request[3] = 4U;
  request[4] = 0xFFU;
  request[5] = 0U;
  len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, request, len), MB_TINY_OK);
  CHECK((coils[0] & 0x02U) != 0U);

  request[4] = 0x01U;
  request[5] = 0U;
  len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, request, len), MB_TINY_EXCEPTION);
  CHECK_EQ(g_slave_tx_a[2], MB_ERR_ILLEGAL_DATA_VALUE);
  CHECK((coils[0] & 0x02U) != 0U);
}

static void test_slave_read_only_maps(void) {
  mb_tiny_slave_t slave;
  uint16_t inputs[3] = {0x1234U, 0xABCDU, 0x5678U};
  uint8_t discrete[2] = {0xB2U, 0x01U};
  uint8_t request[8];
  uint8_t response[16];
  uint16_t request_len;
  uint16_t response_len;

  CHECK_EQ(mb_tiny_slave_init(&slave, 9U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_input(&slave, inputs, 100U, 3U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_discrete(&slave, discrete, 5U, 10U),
           MB_TINY_OK);

  request[0] = 9U;
  request[1] = MB_FUNC_READ_INPUT;
  request[2] = 0U;
  request[3] = 101U;
  request[4] = 0U;
  request[5] = 2U;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_OK);
  CHECK_EQ(response_len, 9U);
  CHECK_EQ(response[1], MB_FUNC_READ_INPUT);
  CHECK_EQ(response[2], 4U);
  CHECK_EQ(response[3], 0xABU);
  CHECK_EQ(response[4], 0xCDU);
  CHECK_EQ(response[5], 0x56U);
  CHECK_EQ(response[6], 0x78U);

  request[1] = MB_FUNC_READ_DISCRETE;
  request[3] = 8U;
  request[5] = 7U;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_OK);
  CHECK_EQ(response_len, 6U);
  CHECK_EQ(response[1], MB_FUNC_READ_DISCRETE);
  CHECK_EQ(response[2], 1U);
  CHECK_EQ(response[3], 0x36U);
}

static void test_slave_write_multiple_coils(void) {
  mb_tiny_slave_t slave;
  uint8_t coils[119];
  uint8_t request[MB_TINY_MAX_ADU_SIZE];
  uint8_t response[8];
  uint16_t request_len;
  uint16_t response_len;
  uint16_t i;

  memset(coils, 0xAA, sizeof(coils));
  CHECK_EQ(mb_tiny_slave_init(&slave, 4U), MB_TINY_OK);
  CHECK_EQ(
      mb_tiny_slave_config_coils(&slave, coils, 3U, MB_TINY_MAX_WRITE_BITS),
      MB_TINY_OK);

  request[0] = 4U;
  request[1] = MB_FUNC_WRITE_MULTIPLE_COILS;
  request[2] = 0U;
  request[3] = 6U;
  request[4] = 0U;
  request[5] = 10U;
  request[6] = 2U;
  request[7] = 0x4DU;
  request[8] = 0x03U;
  request_len = append_crc(request, 9U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_OK);
  CHECK_EQ(response_len, 8U);
  CHECK_EQ(memcmp(&response[2], &request[2], 4U), 0);
  for (i = 0U; i < 10U; i++) {
    uint16_t offset = (uint16_t)(6U - 3U + i);
    bool actual = (coils[offset / 8U] & (uint8_t)(1U << (offset % 8U))) != 0U;
    bool expected = (request[7U + i / 8U] & (uint8_t)(1U << (i % 8U))) != 0U;
    CHECK_EQ(actual, expected);
  }

  memset(coils, 0U, sizeof(coils));
  request[2] = 0U;
  request[3] = 3U;
  request[4] = (uint8_t)(MB_TINY_MAX_WRITE_BITS >> 8U);
  request[5] = (uint8_t)MB_TINY_MAX_WRITE_BITS;
  request[6] = (uint8_t)(MB_TINY_MAX_WRITE_BITS / 8U);
  memset(&request[7], 0xFF, request[6]);
  request_len = append_crc(request, (uint16_t)(7U + request[6]));
  CHECK_EQ(request_len, MB_TINY_MAX_ADU_SIZE);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_OK);
  CHECK_EQ(coils[0], 0xFFU);
  CHECK_EQ(coils[118], 0xFFU);

  request[4] = (uint8_t)((MB_TINY_MAX_WRITE_BITS + 1U) >> 8U);
  request[5] = (uint8_t)(MB_TINY_MAX_WRITE_BITS + 1U);
  request_len = append_crc(request, (uint16_t)(7U + request[6]));
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_EXCEPTION);
  CHECK_EQ(response[2], MB_ERR_ILLEGAL_DATA_VALUE);

  request[4] = 0U;
  request[5] = 8U;
  request[6] = 2U;
  request_len = append_crc(request, 9U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_EXCEPTION);
  CHECK_EQ(response[2], MB_ERR_ILLEGAL_DATA_VALUE);

  request[6] = 1U;
  request_len = append_crc(request, 9U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_FRAME_ERROR);

  memset(coils, 0U, sizeof(coils));
  request[3] = 3U;
  request[5] = 8U;
  request[6] = 1U;
  request[7] = 0xA5U;
  request_len = append_crc(request, 8U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 7U,
                                 &response_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(coils[0], 0U);

  request[0] = 0U;
  request_len = append_crc(request, 8U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 1U,
                                 &response_len),
           MB_TINY_NO_RESPONSE);
  CHECK_EQ(coils[0], 0xA5U);
  CHECK_EQ(response_len, 0U);
}

static void test_slave_malformed_frames(void) {
  mb_tiny_slave_t slave;
  uint16_t registers[2] = {0x1111U, 0x2222U};
  uint8_t frame[16] = {
      1U, MB_FUNC_WRITE_MULTIPLE_REGS, 0U, 0U, 0U, 2U, 4U, 0x12U, 0x34U};
  uint16_t len;

  CHECK_EQ(mb_tiny_slave_init(&slave, 1U), MB_TINY_OK);
  mb_tiny_slave_set_send(&slave, slave_send_a);

  frame[1] = MB_FUNC_READ_HOLDING;
  len = append_crc(frame, 4U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, frame, len), MB_TINY_FRAME_ERROR);

  frame[0] = 1U;
  frame[1] = MB_FUNC_READ_HOLDING;
  frame[2] = 0U;
  frame[3] = 0U;
  frame[4] = 0U;
  frame[5] = 1U;
  len = append_crc(frame, 6U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, frame, len), MB_TINY_EXCEPTION);
  CHECK_EQ(g_slave_tx_a[2], MB_ERR_ILLEGAL_DATA_ADDR);

  CHECK_EQ(mb_tiny_slave_config_holding(&slave, registers, 0U, 2U), MB_TINY_OK);
  frame[1] = MB_FUNC_WRITE_MULTIPLE_REGS;
  frame[4] = 0U;
  frame[5] = 2U;
  frame[6] = 4U;
  frame[7] = 0xAAU;
  frame[8] = 0xAAU;
  len = append_crc(frame, 9U);
  CHECK_EQ(mb_tiny_slave_handle(&slave, frame, len), MB_TINY_FRAME_ERROR);
  CHECK_EQ(registers[0], 0x1111U);
  CHECK_EQ(registers[1], 0x2222U);
}

static void test_slave_instance_callbacks(void) {
  mb_tiny_slave_t slave_a;
  mb_tiny_slave_t slave_b;
  uint16_t reg_a = 0x1234U;
  uint16_t reg_b = 0x5678U;
  uint8_t request[8];
  uint16_t len;

  CHECK_EQ(mb_tiny_slave_init(&slave_a, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_init(&slave_b, 2U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave_a, &reg_a, 0U, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave_b, &reg_b, 0U, 1U), MB_TINY_OK);
  mb_tiny_slave_set_send(&slave_a, slave_send_a);
  mb_tiny_slave_set_send(&slave_b, slave_send_b);

  request[0] = 1U;
  request[1] = MB_FUNC_READ_HOLDING;
  request[2] = 0U;
  request[3] = 0U;
  request[4] = 0U;
  request[5] = 1U;
  len = append_crc(request, 6U);
  g_slave_tx_a_len = 0U;
  g_slave_tx_b_len = 0U;
  CHECK_EQ(mb_tiny_slave_handle(&slave_a, request, len), MB_TINY_OK);
  CHECK(g_slave_tx_a_len != 0U);
  CHECK_EQ(g_slave_tx_b_len, 0U);
}

static void test_slave_core_api(void) {
  mb_tiny_slave_t slave;
  uint16_t holding = 0x1234U;
  uint8_t request[8];
  uint8_t response[16];
  uint16_t request_len;
  uint16_t response_len;

  CHECK_EQ(mb_tiny_slave_init(&slave, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave, &holding, 0U, 1U), MB_TINY_OK);

  request[0] = 1U;
  request[1] = MB_FUNC_READ_HOLDING;
  request[2] = 0U;
  request[3] = 0U;
  request[4] = 0U;
  request[5] = 1U;
  request_len = append_crc(request, 6U);
  response_len = 0U;
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_OK);
  CHECK_EQ(response_len, 7U);
  CHECK_EQ(response[3], 0x12U);
  CHECK_EQ(response[4], 0x34U);

  request[0] = 2U;
  request_len = append_crc(request, 6U);
  response_len = 99U;
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_IGNORED);
  CHECK_EQ(response_len, 0U);

  request[0] = 0U;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response,
                                 sizeof(response), &response_len),
           MB_TINY_IGNORED);
  CHECK_EQ(response_len, 0U);

  request[0] = 1U;
  request[1] = MB_FUNC_WRITE_SINGLE_REG;
  request[4] = 0xABU;
  request[5] = 0xCDU;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 7U,
                                 &response_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(holding, 0x1234U);
  CHECK_EQ(response_len, 0U);

  request[0] = 0U;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 1U,
                                 &response_len),
           MB_TINY_NO_RESPONSE);
  CHECK_EQ(holding, 0xABCDU);
  CHECK_EQ(response_len, 0U);

  request[0] = 1U;
  request[1] = 0x7FU;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 4U,
                                 &response_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(response_len, 0U);
  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, response, 5U,
                                 &response_len),
           MB_TINY_EXCEPTION);
  CHECK_EQ(response_len, 5U);
  CHECK_EQ(response[2], MB_ERR_ILLEGAL_FUNC);

  CHECK_EQ(mb_tiny_slave_process(&slave, request, request_len, request,
                                 sizeof(request), &response_len),
           MB_TINY_INVALID_PARAM);
}

static void test_master_validation(void) {
  mb_tiny_master_t master;
  uint16_t registers[2] = {0U, 0U};
  uint8_t response[16];
  uint8_t coils[1] = {0U};

  CHECK_EQ(mb_tiny_master_init(&master), MB_TINY_OK);
  mb_tiny_master_set_uart(&master, master_send, master_recv);

  response[0] = 1U;
  response[1] = MB_FUNC_READ_HOLDING;
  response[2] = 4U;
  response[3] = 0x12U;
  response[4] = 0x34U;
  response[5] = 0x56U;
  response[6] = 0x78U;
  set_master_response(response, 7U);
  CHECK_EQ(mb_tiny_master_read_holding(&master, 1U, 0U, 2U, registers),
           MB_TINY_OK);
  CHECK_EQ(registers[0], 0x1234U);
  CHECK_EQ(registers[1], 0x5678U);

  response[0] = 2U;
  set_master_response(response, 7U);
  CHECK_EQ(mb_tiny_master_read_holding(&master, 1U, 0U, 2U, registers),
           MB_TINY_FRAME_ERROR);

  response[0] = 1U;
  response[2] = 2U;
  set_master_response(response, 5U);
  CHECK_EQ(mb_tiny_master_read_holding(&master, 1U, 0U, 2U, registers),
           MB_TINY_FRAME_ERROR);

  response[0] = 1U;
  response[1] = (uint8_t)(MB_FUNC_READ_HOLDING | 0x80U);
  response[2] = MB_ERR_ILLEGAL_DATA_ADDR;
  set_master_response(response, 3U);
  CHECK_EQ(mb_tiny_master_read_holding(&master, 1U, 0U, 2U, registers),
           MB_TINY_EXCEPTION);
  CHECK_EQ(master.last_exception, MB_ERR_ILLEGAL_DATA_ADDR);

  response[0] = 1U;
  response[1] = MB_FUNC_READ_COILS;
  response[2] = 1U;
  response[3] = 0xFFU;
  set_master_response(response, 4U);
  CHECK_EQ(mb_tiny_master_read_coils(&master, 1U, 0U, 5U, coils), MB_TINY_OK);
  CHECK_EQ(coils[0], 0x1FU);

  g_master_send_mode = 1;
  CHECK_EQ(mb_tiny_master_write_reg(&master, 1U, 0U, 0x1234U),
           MB_TINY_IO_ERROR);
  g_master_send_mode = 0;

  CHECK_EQ(mb_tiny_master_write_coil(&master, 1U, 0U, 0x0100U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_master_write_regs(&master, 1U, 0U,
                                     MB_TINY_MAX_WRITE_REGS + 1U, registers),
           MB_TINY_INVALID_PARAM);
}

static void test_master_extended_functions(void) {
  mb_tiny_master_t master;
  uint16_t registers[2] = {0U, 0U};
  uint8_t bits[2] = {0U, 0U};
  uint8_t response[8];
  uint8_t write_bits[2] = {0x5AU, 0xFFU};

  CHECK_EQ(mb_tiny_master_init(&master), MB_TINY_OK);
  mb_tiny_master_set_uart(&master, master_send, master_recv);

  response[0] = 3U;
  response[1] = MB_FUNC_READ_INPUT;
  response[2] = 4U;
  response[3] = 0x01U;
  response[4] = 0x02U;
  response[5] = 0xA0U;
  response[6] = 0xB0U;
  set_master_response(response, 7U);
  CHECK_EQ(mb_tiny_master_read_input(&master, 3U, 0x1234U, 2U, registers),
           MB_TINY_OK);
  CHECK_EQ(registers[0], 0x0102U);
  CHECK_EQ(registers[1], 0xA0B0U);
  CHECK_EQ(g_master_request_len, 8U);
  CHECK_EQ(g_master_request[1], MB_FUNC_READ_INPUT);
  CHECK_EQ(g_master_request[2], 0x12U);
  CHECK_EQ(g_master_request[3], 0x34U);
  CHECK_EQ(g_master_request[5], 2U);

  response[1] = MB_FUNC_READ_DISCRETE;
  response[2] = 2U;
  response[3] = 0xA5U;
  response[4] = 0xFFU;
  set_master_response(response, 5U);
  CHECK_EQ(mb_tiny_master_read_discrete(&master, 3U, 7U, 10U, bits),
           MB_TINY_OK);
  CHECK_EQ(bits[0], 0xA5U);
  CHECK_EQ(bits[1], 0x03U);
  CHECK_EQ(g_master_request[1], MB_FUNC_READ_DISCRETE);
  CHECK_EQ(g_master_request[3], 7U);
  CHECK_EQ(g_master_request[5], 10U);

  response[0] = 3U;
  response[1] = MB_FUNC_WRITE_MULTIPLE_COILS;
  response[2] = 0U;
  response[3] = 9U;
  response[4] = 0U;
  response[5] = 10U;
  set_master_response(response, 6U);
  CHECK_EQ(mb_tiny_master_write_coils(&master, 3U, 9U, 10U, write_bits),
           MB_TINY_OK);
  CHECK_EQ(g_master_request_len, 11U);
  CHECK_EQ(g_master_request[1], MB_FUNC_WRITE_MULTIPLE_COILS);
  CHECK_EQ(g_master_request[6], 2U);
  CHECK_EQ(g_master_request[7], 0x5AU);
  CHECK_EQ(g_master_request[8], 0x03U);

  response[5] = 9U;
  set_master_response(response, 6U);
  CHECK_EQ(mb_tiny_master_write_coils(&master, 3U, 9U, 10U, write_bits),
           MB_TINY_FRAME_ERROR);

  CHECK_EQ(mb_tiny_master_write_coils(&master, 3U, 0U,
                                      MB_TINY_MAX_WRITE_BITS + 1U, write_bits),
           MB_TINY_INVALID_PARAM);
}

static void test_master_output_capacity(void) {
  mb_tiny_master_t master;
  uint16_t registers[2] = {0xAAAAU, 0xBBBBU};
  uint8_t bits[2] = {0xAAU, 0xBBU};
  uint8_t response[8];

  CHECK_EQ(mb_tiny_master_init(&master), MB_TINY_OK);
  mb_tiny_master_set_uart(&master, master_send, master_recv);
  g_master_request_len = 0U;

  CHECK_EQ(mb_tiny_master_read_holding_ex(&master, 1U, 0U, 2U, registers, 1U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(registers[0], 0xAAAAU);
  CHECK_EQ(registers[1], 0xBBBBU);
  CHECK_EQ(g_master_request_len, 0U);

  CHECK_EQ(mb_tiny_master_read_input_ex(&master, 1U, 0U, 2U, registers, 0U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(g_master_request_len, 0U);

  CHECK_EQ(mb_tiny_master_read_coils_ex(&master, 1U, 0U, 9U, bits, 1U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(bits[0], 0xAAU);
  CHECK_EQ(bits[1], 0xBBU);
  CHECK_EQ(g_master_request_len, 0U);

  CHECK_EQ(mb_tiny_master_read_discrete_ex(&master, 1U, 0U, 9U, bits, 0U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(g_master_request_len, 0U);

  response[0] = 1U;
  response[1] = MB_FUNC_READ_COILS;
  response[2] = 2U;
  response[3] = 0x55U;
  response[4] = 0xFFU;
  set_master_response(response, 5U);
  CHECK_EQ(mb_tiny_master_read_coils_ex(&master, 1U, 0U, 9U, bits, 2U),
           MB_TINY_OK);
  CHECK_EQ(bits[0], 0x55U);
  CHECK_EQ(bits[1], 0x01U);
}

static void test_initialization_and_address_validation(void) {
  mb_tiny_slave_t slave;
  mb_tiny_master_t master;
  uint16_t data;

  memset(&slave, 0, sizeof(slave));
  CHECK_EQ(mb_tiny_slave_init(&slave, 0U), MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_slave_init(&slave, 248U), MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_slave_init(&slave, 247U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave, &data, 0xFFFFU, 2U),
           MB_TINY_INVALID_PARAM);

  memset(&master, 0, sizeof(master));
  CHECK_EQ(mb_tiny_master_write_reg(&master, 1U, 0U, 1U),
           MB_TINY_NOT_INITIALIZED);
}

int main(void) {
  test_crc();
  test_rtu_receive_framing();
  test_slave_registers_and_limits();
  test_slave_coil_alignment_and_validation();
  test_slave_read_only_maps();
  test_slave_write_multiple_coils();
  test_slave_malformed_frames();
  test_slave_instance_callbacks();
  test_slave_core_api();
  test_master_validation();
  test_master_extended_functions();
  test_master_output_capacity();
  test_initialization_and_address_validation();

  if (g_failures != 0) {
    printf("mb_tiny tests failed: %d\n", g_failures);
    return 1;
  }

  printf("mb_tiny tests passed\n");
  return 0;
}
