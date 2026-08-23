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
static uint8_t g_async_tx_data[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_async_tx_len;
static int g_async_tx_mode;
static int g_de_mode;
static bool g_de_state;
static uint32_t g_de_calls;

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

static int async_tx_start(const uint8_t *data, uint16_t len) {
  memcpy(g_async_tx_data, data, len);
  g_async_tx_len = len;
  return g_async_tx_mode == 0 ? len : (int)len - 1;
}

static int rs485_set_de(bool transmit) {
  g_de_calls++;
  g_de_state = transmit;
  return g_de_mode == 0 ? 0 : -1;
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

static void test_rtu_receive_queue(void) {
  mb_tiny_rtu_rx_queue_t queue;
  mb_tiny_rtu_rx_slot_t slots[10];
  mb_tiny_rtu_rx_t rx;
  uint8_t frame[8];
  uint16_t frame_len;
  uint16_t i;

  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 1U), MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 10U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 4U), MB_TINY_OK);
  CHECK(mb_tiny_rtu_rx_queue_is_idle(&queue, &rx));

  for (i = 0U; i < 4U; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, (uint8_t)(0x10U + i), i),
             MB_TINY_OK);
  }
  for (i = 0U; i < 4U; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, (uint8_t)(0x20U + i),
                                           (uint32_t)(10U + i)),
             MB_TINY_OK);
  }
  CHECK_EQ(mb_tiny_rtu_rx_queue_pending(&queue), 8U);
  CHECK_EQ(
      mb_tiny_rtu_rx_queue_process(&queue, &rx, 20U, frame, 3U, &frame_len),
      MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(mb_tiny_rtu_rx_queue_pending(&queue), 4U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 20U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[0], 0x10U);
  CHECK_EQ(frame[3], 0x13U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_pending(&queue), 3U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 20U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[0], 0x20U);
  CHECK_EQ(frame[3], 0x23U);
  CHECK(mb_tiny_rtu_rx_queue_is_idle(&queue, &rx));

  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 1U, 30U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 2U, 31U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xA0U, 40U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xA1U, 41U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xA2U, 42U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xA3U, 43U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 50U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_ERROR);
  CHECK_EQ(mb_tiny_rtu_rx_queue_pending(&queue), 4U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 50U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame[0], 0xA0U);
  CHECK_EQ(frame[3], 0xA3U);

  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 4U), MB_TINY_OK);
  mb_tiny_rtu_rx_reset(&rx);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 1U, 60U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 2U, 61U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 3U, 62U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 4U, 63U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(queue.dropped_bytes, 1U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 70U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_ERROR);
  CHECK_EQ(mb_tiny_rtu_rx_queue_pending(&queue), 0U);
  CHECK(mb_tiny_rtu_rx_queue_is_idle(&queue, &rx));

  for (i = 0U; i < 4U; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, (uint8_t)(0xB0U + i),
                                           (uint32_t)(80U + i)),
             i < 3U ? MB_TINY_OK : MB_TINY_BUFFER_TOO_SMALL);
  }
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 90U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_ERROR);

  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xC0U, 100U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xC1U, 101U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xC2U, 102U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 103U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_IGNORED);
  CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, 0xC3U, 103U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_process(&queue, &rx, 107U, frame, sizeof(frame),
                                        &frame_len),
           MB_TINY_FRAME_READY);
  CHECK_EQ(frame_len, 4U);
  CHECK_EQ(frame[0], 0xC0U);
  CHECK_EQ(frame[3], 0xC3U);
}

static void test_rtu_nonblocking_tx(void) {
  mb_tiny_rtu_tx_t tx;
  uint8_t data[4] = {1U, 2U, 3U, 4U};

  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, NULL, rs485_set_de, 10U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, async_tx_start, rs485_set_de, 0U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, async_tx_start, rs485_set_de, 10U),
           MB_TINY_OK);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));

  g_async_tx_mode = 0;
  g_de_mode = 0;
  g_de_state = false;
  g_de_calls = 0U;
  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 100U), MB_TINY_OK);
  CHECK(g_de_state);
  CHECK_EQ(g_de_calls, 1U);
  CHECK_EQ(g_async_tx_len, sizeof(data));
  CHECK_EQ(memcmp(g_async_tx_data, data, sizeof(data)), 0);
  data[0] = 0xFFU;
  CHECK_EQ(tx.data[0], 1U);
  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 101U), MB_TINY_BUSY);
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 109U), MB_TINY_BUSY);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK(g_de_state);
  CHECK_EQ(g_de_calls, 1U);
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 109U), MB_TINY_OK);
  CHECK(!g_de_state);
  CHECK_EQ(g_de_calls, 2U);
  CHECK_EQ(tx.completed_count, 1U);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 110U), MB_TINY_IGNORED);

  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), UINT32_MAX - 4U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 4U), MB_TINY_BUSY);
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 5U), MB_TINY_TIMEOUT);
  CHECK(!g_de_state);
  CHECK_EQ(tx.error_count, 1U);

  g_async_tx_mode = 1;
  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 200U),
           MB_TINY_IO_ERROR);
  CHECK(!g_de_state);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));
  CHECK_EQ(tx.error_count, 2U);
  g_async_tx_mode = 0;

  g_de_mode = 1;
  g_de_calls = 0U;
  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 300U),
           MB_TINY_IO_ERROR);
  CHECK_EQ(tx.error_count, 3U);
  CHECK_EQ(g_de_calls, 2U);
  g_de_mode = 0;

  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 350U), MB_TINY_OK);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK_EQ(mb_tiny_rtu_tx_process(&tx, 360U), MB_TINY_OK);
  CHECK_EQ(tx.completed_count, 2U);

  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, sizeof(data), 400U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_tx_abort(&tx), MB_TINY_OK);
  CHECK(!g_de_state);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));

  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, NULL, sizeof(data), 500U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_tx_start(&tx, data, 0U, 500U), MB_TINY_INVALID_PARAM);
}

static void queue_frame(mb_tiny_rtu_rx_queue_t *queue, const uint8_t *frame,
                        uint16_t len, uint32_t start_ms) {
  uint16_t i;

  for (i = 0U; i < len; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(queue, frame[i], start_ms + i),
             MB_TINY_OK);
  }
}

static int complete_master_with_response(
    mb_tiny_rtu_master_t *master, mb_tiny_rtu_rx_queue_t *queue,
    mb_tiny_rtu_tx_t *tx, const uint8_t *response, uint16_t response_len,
    uint32_t tx_done_ms, uint32_t response_start_ms, uint32_t process_ms) {
  mb_tiny_rtu_tx_complete_isr(tx);
  CHECK_EQ(mb_tiny_rtu_master_process(master, tx_done_ms), MB_TINY_BUSY);
  queue_frame(queue, response, response_len, response_start_ms);
  return mb_tiny_rtu_master_process(master, process_ms);
}

static void test_rtu_nonblocking_master(void) {
  mb_tiny_rtu_rx_queue_t queue;
  mb_tiny_rtu_rx_slot_t slots[16];
  mb_tiny_rtu_rx_t rx;
  mb_tiny_rtu_tx_t tx;
  mb_tiny_rtu_master_t master;
  uint8_t request[8] = {1U, MB_FUNC_READ_HOLDING, 0U, 0U, 0U, 1U};
  uint8_t response[16];
  uint8_t copied[16];
  uint16_t request_len;
  uint16_t response_len;

  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 16U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 4U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, async_tx_start, rs485_set_de, 20U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_init(&master, &queue, &rx, &tx, 50U), MB_TINY_OK);
  g_async_tx_mode = 0;
  g_de_mode = 0;
  g_de_state = false;

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 100U),
           MB_TINY_OK);
  CHECK_EQ(master.state, MB_TINY_RTU_MASTER_TRANSMITTING);
  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 101U),
           MB_TINY_BUSY);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 101U), MB_TINY_BUSY);
  mb_tiny_rtu_master_reset(&master);
  CHECK_EQ(master.state, MB_TINY_RTU_MASTER_TRANSMITTING);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 102U), MB_TINY_BUSY);
  CHECK_EQ(master.state, MB_TINY_RTU_MASTER_WAITING_RESPONSE);
  mb_tiny_rtu_master_reset(&master);
  CHECK_EQ(master.state, MB_TINY_RTU_MASTER_WAITING_RESPONSE);
  CHECK(!g_de_state);

  response[0] = 1U;
  response[1] = MB_FUNC_READ_HOLDING;
  response[2] = 2U;
  response[3] = 0x12U;
  response[4] = 0x34U;
  response_len = append_crc(response, 5U);
  queue_frame(&queue, response, response_len, 110U);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 120U), MB_TINY_OK);
  CHECK_EQ(master.state, MB_TINY_RTU_MASTER_DONE);
  CHECK_EQ(master.completed_count, 1U);
  CHECK_EQ(mb_tiny_rtu_master_get_response(&master, copied, 6U, &response_len),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(mb_tiny_rtu_master_get_response(&master, copied, sizeof(copied),
                                           &response_len),
           MB_TINY_OK);
  CHECK_EQ(response_len, 7U);
  CHECK_EQ(copied[3], 0x12U);
  CHECK_EQ(copied[4], 0x34U);
  mb_tiny_rtu_master_reset(&master);
  CHECK(mb_tiny_rtu_master_is_idle(&master));

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 200U),
           MB_TINY_OK);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 201U), MB_TINY_BUSY);
  response[0] = 1U;
  response[1] = (uint8_t)(MB_FUNC_READ_HOLDING | 0x80U);
  response[2] = MB_ERR_ILLEGAL_DATA_ADDR;
  response_len = append_crc(response, 3U);
  queue_frame(&queue, response, response_len, 210U);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 220U), MB_TINY_EXCEPTION);
  CHECK_EQ(master.last_exception, MB_ERR_ILLEGAL_DATA_ADDR);
  CHECK_EQ(master.completed_count, 2U);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 230U),
           MB_TINY_OK);
  response[0] = 2U;
  response[1] = MB_FUNC_READ_HOLDING;
  response[2] = 2U;
  response[3] = 0x12U;
  response[4] = 0x34U;
  response_len = append_crc(response, 5U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 231U, 240U, 250U),
           MB_TINY_FRAME_ERROR);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 260U),
           MB_TINY_OK);
  response[0] = 1U;
  response[1] = MB_FUNC_READ_INPUT;
  response_len = append_crc(response, 5U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 261U, 270U, 280U),
           MB_TINY_FRAME_ERROR);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 290U),
           MB_TINY_OK);
  response[1] = MB_FUNC_READ_HOLDING;
  response_len = append_crc(response, 5U);
  response[response_len - 1U] ^= 0x01U;
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 291U, 300U, 310U),
           MB_TINY_CRC_ERROR);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 320U),
           MB_TINY_OK);
  response[1] = (uint8_t)(MB_FUNC_READ_HOLDING | 0x80U);
  response[2] = MB_ERR_ILLEGAL_DATA_VALUE;
  response[3] = 0xAAU;
  response_len = append_crc(response, 4U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 321U, 330U, 340U),
           MB_TINY_FRAME_ERROR);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(master.error_count, 4U);
  CHECK_EQ(
      mb_tiny_rtu_master_start(&master, request, request_len, UINT32_MAX - 10U),
      MB_TINY_OK);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, UINT32_MAX - 9U), MB_TINY_BUSY);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 39U), MB_TINY_BUSY);
  CHECK_EQ(mb_tiny_rtu_master_process(&master, 40U), MB_TINY_TIMEOUT);
  CHECK_EQ(master.error_count, 5U);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 400U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_abort(&master), MB_TINY_OK);
  CHECK(mb_tiny_rtu_master_is_idle(&master));
  CHECK(!g_de_state);

  request[0] = 0U;
  request_len = append_crc(request, 6U);
  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 500U),
           MB_TINY_INVALID_PARAM);
  request[0] = 1U;
  request_len = append_crc(request, 6U);
  request[7] ^= 0x01U;
  CHECK_EQ(mb_tiny_rtu_master_start(&master, request, request_len, 500U),
           MB_TINY_CRC_ERROR);
}

static void test_rtu_master_convenience(void) {
  mb_tiny_rtu_rx_queue_t queue;
  mb_tiny_rtu_rx_slot_t slots[32];
  mb_tiny_rtu_rx_t rx;
  mb_tiny_rtu_tx_t tx;
  mb_tiny_rtu_master_t master;
  uint16_t registers[2] = {0U, 0U};
  uint16_t write_registers[2] = {0x1234U, 0x5678U};
  uint8_t bits[2] = {0U, 0U};
  uint8_t write_bits[2] = {0xA5U, 0xFFU};
  uint8_t response[16];
  uint16_t response_len;

  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 32U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 4U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, async_tx_start, rs485_set_de, 20U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_init(&master, &queue, &rx, &tx, 50U), MB_TINY_OK);
  g_async_tx_mode = 0;
  g_de_mode = 0;

  CHECK_EQ(
      mb_tiny_rtu_master_read_holding_start(&master, 1U, 0x0010U, 2U, 100U),
      MB_TINY_OK);
  CHECK_EQ(g_async_tx_len, 8U);
  CHECK_EQ(g_async_tx_data[1], MB_FUNC_READ_HOLDING);
  CHECK_EQ(g_async_tx_data[3], 0x10U);
  CHECK_EQ(g_async_tx_data[5], 2U);
  CHECK_EQ(
      mb_tiny_crc16(g_async_tx_data, 6U),
      (uint16_t)(g_async_tx_data[6] | ((uint16_t)g_async_tx_data[7] << 8U)));
  response[0] = 1U;
  response[1] = MB_FUNC_READ_HOLDING;
  response[2] = 4U;
  response[3] = 0x12U;
  response[4] = 0x34U;
  response[5] = 0x56U;
  response[6] = 0x78U;
  response_len = append_crc(response, 7U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 101U, 110U, 125U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_read_holding_result(&master, registers, 1U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(mb_tiny_rtu_master_read_holding_result(&master, registers, 2U),
           MB_TINY_OK);
  CHECK_EQ(registers[0], 0x1234U);
  CHECK_EQ(registers[1], 0x5678U);
  CHECK_EQ(mb_tiny_rtu_master_read_input_result(&master, registers, 2U),
           MB_TINY_FRAME_ERROR);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_read_input_start(&master, 2U, 3U, 1U, 200U),
           MB_TINY_OK);
  response[0] = 2U;
  response[1] = MB_FUNC_READ_INPUT;
  response[2] = 3U;
  response[3] = 0xABU;
  response[4] = 0xCDU;
  response[5] = 0xEFU;
  response_len = append_crc(response, 6U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 201U, 210U, 225U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_read_input_result(&master, registers, 2U),
           MB_TINY_FRAME_ERROR);
  master.response[2] = 2U;
  master.response_len = append_crc(master.response, 5U);
  CHECK_EQ(mb_tiny_rtu_master_read_input_result(&master, registers, 1U),
           MB_TINY_OK);
  CHECK_EQ(registers[0], 0xABCDU);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_read_coils_start(&master, 1U, 0U, 10U, 300U),
           MB_TINY_OK);
  response[0] = 1U;
  response[1] = MB_FUNC_READ_COILS;
  response[2] = 2U;
  response[3] = 0xA5U;
  response[4] = 0xFFU;
  response_len = append_crc(response, 5U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 301U, 310U, 325U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_read_coils_result(&master, bits, 1U),
           MB_TINY_BUFFER_TOO_SMALL);
  CHECK_EQ(mb_tiny_rtu_master_read_coils_result(&master, bits, 2U), MB_TINY_OK);
  CHECK_EQ(bits[0], 0xA5U);
  CHECK_EQ(bits[1], 0x03U);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_read_discrete_start(&master, 1U, 7U, 8U, 400U),
           MB_TINY_OK);
  response[0] = 1U;
  response[1] = MB_FUNC_READ_DISCRETE;
  response[2] = 1U;
  response[3] = 0x5AU;
  response_len = append_crc(response, 4U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 401U, 410U, 425U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_read_discrete_result(&master, bits, 1U),
           MB_TINY_OK);
  CHECK_EQ(bits[0], 0x5AU);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(
      mb_tiny_rtu_master_write_reg_start(&master, 1U, 0x20U, 0xBEEFU, 500U),
      MB_TINY_OK);
  memcpy(response, g_async_tx_data, 6U);
  response_len = append_crc(response, 6U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 501U, 510U, 525U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_write_reg_result(&master), MB_TINY_OK);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_write_coil_start(&master, 1U, 9U, 0xFF00U, 600U),
           MB_TINY_OK);
  memcpy(response, g_async_tx_data, 6U);
  response_len = append_crc(response, 6U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 601U, 610U, 625U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_write_coil_result(&master), MB_TINY_OK);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_write_regs_start(&master, 1U, 0x30U, 2U,
                                               write_registers, 700U),
           MB_TINY_OK);
  CHECK_EQ(g_async_tx_len, 13U);
  CHECK_EQ(g_async_tx_data[6], 4U);
  CHECK_EQ(g_async_tx_data[7], 0x12U);
  CHECK_EQ(g_async_tx_data[10], 0x78U);
  memcpy(response, g_async_tx_data, 6U);
  response_len = append_crc(response, 6U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 701U, 710U, 725U),
           MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_master_write_regs_result(&master), MB_TINY_OK);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_write_coils_start(&master, 1U, 0x40U, 10U,
                                                write_bits, 800U),
           MB_TINY_OK);
  CHECK_EQ(g_async_tx_len, 11U);
  CHECK_EQ(g_async_tx_data[6], 2U);
  CHECK_EQ(g_async_tx_data[7], 0xA5U);
  CHECK_EQ(g_async_tx_data[8], 0x03U);
  memcpy(response, g_async_tx_data, 6U);
  response_len = append_crc(response, 6U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 801U, 810U, 825U),
           MB_TINY_OK);
  response[5] = 9U;
  memcpy(master.response, response, 6U);
  master.response_len = append_crc(master.response, 6U);
  CHECK_EQ(mb_tiny_rtu_master_write_coils_result(&master), MB_TINY_FRAME_ERROR);
  memcpy(master.response, g_async_tx_data, 6U);
  master.response_len = append_crc(master.response, 6U);
  CHECK_EQ(mb_tiny_rtu_master_write_coils_result(&master), MB_TINY_OK);
  mb_tiny_rtu_master_reset(&master);

  CHECK_EQ(mb_tiny_rtu_master_read_holding_start(&master, 1U, 0U, 0U, 900U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_master_write_coil_start(&master, 1U, 0U, 1U, 900U),
           MB_TINY_INVALID_PARAM);
  CHECK_EQ(mb_tiny_rtu_master_write_regs_start(&master, 1U, 0U, 1U, NULL, 900U),
           MB_TINY_INVALID_PARAM);

  CHECK_EQ(mb_tiny_rtu_master_read_holding_start(&master, 1U, 0U, 1U, 910U),
           MB_TINY_OK);
  response[0] = 1U;
  response[1] = (uint8_t)(MB_FUNC_READ_HOLDING | 0x80U);
  response[2] = MB_ERR_DEVICE_BUSY;
  response_len = append_crc(response, 3U);
  CHECK_EQ(complete_master_with_response(&master, &queue, &tx, response,
                                         response_len, 911U, 920U, 935U),
           MB_TINY_EXCEPTION);
  CHECK_EQ(mb_tiny_rtu_master_read_holding_result(&master, registers, 2U),
           MB_TINY_EXCEPTION);
  mb_tiny_rtu_master_reset(&master);
}

static void test_rtu_slave_service(void) {
  mb_tiny_slave_t slave;
  mb_tiny_rtu_rx_queue_t queue;
  mb_tiny_rtu_rx_slot_t slots[16];
  mb_tiny_rtu_rx_t rx;
  mb_tiny_rtu_tx_t tx;
  uint16_t holding = 0x1234U;
  uint8_t request[MB_TINY_MAX_ADU_SIZE];
  uint8_t response[MB_TINY_MAX_ADU_SIZE];
  uint16_t request_len;
  uint16_t i;

  CHECK_EQ(mb_tiny_slave_init(&slave, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_slave_config_holding(&slave, &holding, 0U, 1U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_queue_init(&queue, slots, 16U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_rx_init(&rx, 4U), MB_TINY_OK);
  CHECK_EQ(mb_tiny_rtu_tx_init(&tx, async_tx_start, rs485_set_de, 20U),
           MB_TINY_OK);
  g_async_tx_mode = 0;
  g_de_mode = 0;
  g_de_state = false;

  request[0] = 1U;
  request[1] = MB_FUNC_READ_HOLDING;
  request[2] = 0U;
  request[3] = 0U;
  request[4] = 0U;
  request[5] = 1U;
  request_len = append_crc(request, 6U);
  for (i = 0U; i < request_len; i++) {
    CHECK_EQ(mb_tiny_rtu_rx_queue_push_isr(&queue, request[i], i), MB_TINY_OK);
  }
  CHECK_EQ(mb_tiny_rtu_slave_poll(&slave, &queue, &rx, &tx, 20U, request,
                                  sizeof(request), response, sizeof(response)),
           MB_TINY_RESPONSE_STARTED);
  CHECK(g_de_state);
  CHECK_EQ(g_async_tx_len, 7U);
  CHECK_EQ(g_async_tx_data[0], 1U);
  CHECK_EQ(g_async_tx_data[1], MB_FUNC_READ_HOLDING);
  CHECK_EQ(g_async_tx_data[3], 0x12U);
  CHECK_EQ(g_async_tx_data[4], 0x34U);
  CHECK_EQ(mb_tiny_rtu_slave_poll(&slave, &queue, &rx, &tx, 21U, request,
                                  sizeof(request), response, sizeof(response)),
           MB_TINY_BUSY);
  mb_tiny_rtu_tx_complete_isr(&tx);
  CHECK_EQ(mb_tiny_rtu_slave_poll(&slave, &queue, &rx, &tx, 22U, request,
                                  sizeof(request), response, sizeof(response)),
           MB_TINY_OK);
  CHECK(!g_de_state);

  request[0] = 0U;
  request[1] = MB_FUNC_WRITE_SINGLE_REG;
  request[4] = 0xABU;
  request[5] = 0xCDU;
  request_len = append_crc(request, 6U);
  for (i = 0U; i < request_len; i++) {
    CHECK_EQ(
        mb_tiny_rtu_rx_queue_push_isr(&queue, request[i], (uint32_t)(30U + i)),
        MB_TINY_OK);
  }
  CHECK_EQ(mb_tiny_rtu_slave_poll(&slave, &queue, &rx, &tx, 50U, request,
                                  sizeof(request), response, sizeof(response)),
           MB_TINY_NO_RESPONSE);
  CHECK_EQ(holding, 0xABCDU);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));

  request[0] = 2U;
  request[1] = MB_FUNC_READ_HOLDING;
  request[4] = 0U;
  request[5] = 1U;
  request_len = append_crc(request, 6U);
  for (i = 0U; i < request_len; i++) {
    CHECK_EQ(
        mb_tiny_rtu_rx_queue_push_isr(&queue, request[i], (uint32_t)(60U + i)),
        MB_TINY_OK);
  }
  CHECK_EQ(mb_tiny_rtu_slave_poll(&slave, &queue, &rx, &tx, 80U, request,
                                  sizeof(request), response, sizeof(response)),
           MB_TINY_IGNORED);
  CHECK(mb_tiny_rtu_tx_is_idle(&tx));
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
  test_rtu_receive_queue();
  test_rtu_nonblocking_tx();
  test_rtu_nonblocking_master();
  test_rtu_master_convenience();
  test_rtu_slave_service();
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
