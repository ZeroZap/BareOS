/**
 * @file mb_tiny.c
 * @brief Nano Modbus Tiny implementation - bounded RTU ADU processing
 * @version 1.1.0
 * @date 2026-08-23
 */

#include "mb_tiny.h"
#include "xy_string.h"

/* ==================== CRC16 ==================== */

uint16_t mb_tiny_crc16(const uint8_t *data, uint16_t len) {
  uint16_t crc;
  uint16_t i;
  uint8_t bit;

  if (data == NULL && len != 0U) {
    return 0U;
  }

  crc = 0xFFFFU;
  for (i = 0U; i < len; i++) {
    crc ^= data[i];
    for (bit = 0U; bit < 8U; bit++) {
      if ((crc & 1U) != 0U) {
        crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
      } else {
        crc >>= 1U;
      }
    }
  }
  return crc;
}

/* ==================== Common helpers ==================== */

static uint16_t mb_get_u16_be(const uint8_t *data) {
  return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void mb_put_u16_be(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8U);
  data[1] = (uint8_t)value;
}

static bool mb_unit_id_is_valid(uint8_t unit_id) {
  return unit_id >= 1U && unit_id <= 247U;
}

static bool mb_range_contains(uint16_t map_start, uint16_t map_count,
                              uint16_t req_start, uint16_t req_count) {
  uint32_t map_end;
  uint32_t req_end;

  if (map_count == 0U || req_count == 0U || req_start < map_start) {
    return false;
  }

  map_end = (uint32_t)map_start + (uint32_t)map_count;
  req_end = (uint32_t)req_start + (uint32_t)req_count;
  return map_end <= 0x10000UL && req_end <= map_end;
}

static bool mb_frame_crc_is_valid(const uint8_t *data, uint16_t len) {
  uint16_t received;

  if (data == NULL || len < MB_TINY_MIN_ADU_SIZE) {
    return false;
  }

  received = (uint16_t)(((uint16_t)data[len - 1U] << 8U) | data[len - 2U]);
  return received == mb_tiny_crc16(data, (uint16_t)(len - 2U));
}

static uint16_t mb_append_crc(uint8_t *data, uint16_t payload_len) {
  uint16_t crc;

  crc = mb_tiny_crc16(data, payload_len);
  data[payload_len] = (uint8_t)crc;
  data[payload_len + 1U] = (uint8_t)(crc >> 8U);
  return (uint16_t)(payload_len + 2U);
}

static bool mb_coil_get(const mb_tiny_coils_t *map, uint16_t address) {
  uint16_t offset;

  offset = (uint16_t)(address - map->start_addr);
  return (map->data[offset / 8U] & (uint8_t)(1U << (offset % 8U))) != 0U;
}

static void mb_coil_set(mb_tiny_coils_t *map, uint16_t address, bool value) {
  uint16_t offset;
  uint8_t mask;

  offset = (uint16_t)(address - map->start_addr);
  mask = (uint8_t)(1U << (offset % 8U));
  if (value) {
    map->data[offset / 8U] |= mask;
  } else {
    map->data[offset / 8U] &= (uint8_t)~mask;
  }
}

/* ==================== RTU receive framing ==================== */

static void mb_tiny_compiler_barrier(void) {
#if defined(__GNUC__) || defined(__clang__)
  __asm__ volatile("" ::: "memory");
#endif
}

static uint16_t mb_tiny_rtu_gap_ms(uint32_t baud_rate, uint8_t bits_per_char,
                                   uint16_t chars_x1000) {
  uint32_t numerator;
  uint32_t gap_ms;

  if (baud_rate == 0U || bits_per_char < 8U || bits_per_char > 12U) {
    return 0U;
  }
  numerator = (uint32_t)bits_per_char * chars_x1000;
  gap_ms = (numerator + baud_rate - 1U) / baud_rate;
  return gap_ms <= 0xFFFFUL ? (uint16_t)gap_ms : 0U;
}

uint16_t mb_tiny_rtu_frame_gap_ms(uint32_t baud_rate, uint8_t bits_per_char) {
  if (baud_rate > 19200UL) {
    return bits_per_char >= 8U && bits_per_char <= 12U ? 2U : 0U;
  }
  return mb_tiny_rtu_gap_ms(baud_rate, bits_per_char, 3500U);
}

uint16_t mb_tiny_rtu_inter_char_gap_ms(uint32_t baud_rate,
                                       uint8_t bits_per_char) {
  uint16_t gap_ms;

  if (baud_rate > 19200UL) {
    return 0U;
  }
  gap_ms = mb_tiny_rtu_gap_ms(baud_rate, bits_per_char, 1500U);
  return gap_ms != 0U && gap_ms < 0xFFFFU ? (uint16_t)(gap_ms + 1U) : 0U;
}

int mb_tiny_rtu_rx_init_ex(mb_tiny_rtu_rx_t *rx, uint16_t inter_char_gap_ms,
                           uint16_t frame_gap_ms) {
  if (rx == NULL || frame_gap_ms == 0U || inter_char_gap_ms >= frame_gap_ms) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(rx, 0, sizeof(*rx));
  rx->inter_char_gap_ms = inter_char_gap_ms;
  rx->frame_gap_ms = frame_gap_ms;
  return MB_TINY_OK;
}

int mb_tiny_rtu_rx_init(mb_tiny_rtu_rx_t *rx, uint16_t frame_gap_ms) {
  return mb_tiny_rtu_rx_init_ex(rx, 0U, frame_gap_ms);
}

void mb_tiny_rtu_rx_reset(mb_tiny_rtu_rx_t *rx) {
  if (rx != NULL) {
    rx->len = 0U;
    rx->receiving = false;
    rx->overflow = false;
    rx->invalid = false;
  }
}

bool mb_tiny_rtu_rx_is_idle(const mb_tiny_rtu_rx_t *rx) {
  return rx != NULL && !rx->receiving;
}

int mb_tiny_rtu_rx_feed(mb_tiny_rtu_rx_t *rx, uint8_t byte, uint32_t now_ms) {
  uint32_t silence_ms;
  bool missed_frame;
  bool inter_char_error;

  if (rx == NULL || rx->frame_gap_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }

  silence_ms = (uint32_t)(now_ms - rx->last_byte_ms);
  missed_frame = rx->receiving && silence_ms >= rx->frame_gap_ms;
  inter_char_error = rx->receiving && !rx->invalid && !missed_frame &&
                     rx->inter_char_gap_ms != 0U &&
                     silence_ms >= rx->inter_char_gap_ms;
  if (missed_frame) {
    rx->dropped_frames++;
    rx->last_error = MB_TINY_RTU_RX_ERROR_MISSED_FRAME;
    mb_tiny_rtu_rx_reset(rx);
  } else if (inter_char_error) {
    rx->invalid = true;
    rx->last_error = MB_TINY_RTU_RX_ERROR_INTER_CHAR;
  }

  rx->receiving = true;
  rx->last_byte_ms = now_ms;
  if (inter_char_error) {
    return MB_TINY_FRAME_ERROR;
  }
  if (rx->invalid) {
    return MB_TINY_IGNORED;
  }
  if (rx->overflow) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  if (rx->len >= MB_TINY_MAX_ADU_SIZE) {
    rx->overflow = true;
    rx->last_error = MB_TINY_RTU_RX_ERROR_ADU_OVERFLOW;
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  rx->data[rx->len++] = byte;
  return missed_frame ? MB_TINY_FRAME_ERROR : MB_TINY_OK;
}

int mb_tiny_rtu_rx_poll(mb_tiny_rtu_rx_t *rx, uint32_t now_ms, uint8_t *frame,
                        uint16_t frame_capacity, uint16_t *frame_len) {
  if (rx == NULL || frame == NULL || frame_len == NULL ||
      rx->frame_gap_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }
  *frame_len = 0U;
  if (!rx->receiving ||
      (uint32_t)(now_ms - rx->last_byte_ms) < rx->frame_gap_ms) {
    return MB_TINY_IGNORED;
  }
  if (rx->overflow) {
    rx->dropped_frames++;
    mb_tiny_rtu_rx_reset(rx);
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  if (rx->invalid) {
    rx->dropped_frames++;
    mb_tiny_rtu_rx_reset(rx);
    return MB_TINY_FRAME_ERROR;
  }
  if (rx->len < MB_TINY_MIN_ADU_SIZE) {
    rx->dropped_frames++;
    rx->last_error = MB_TINY_RTU_RX_ERROR_SHORT_FRAME;
    mb_tiny_rtu_rx_reset(rx);
    return MB_TINY_FRAME_ERROR;
  }
  if (frame_capacity < rx->len) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }

  memcpy(frame, rx->data, rx->len);
  *frame_len = rx->len;
  mb_tiny_rtu_rx_reset(rx);
  return MB_TINY_FRAME_READY;
}

int mb_tiny_rtu_rx_queue_init(mb_tiny_rtu_rx_queue_t *queue,
                              mb_tiny_rtu_rx_slot_t *slots,
                              uint16_t slot_count) {
  if (queue == NULL || slots == NULL || slot_count < 2U) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(queue, 0, sizeof(*queue));
  queue->slots = slots;
  queue->capacity = slot_count;
  return MB_TINY_OK;
}

uint16_t mb_tiny_rtu_rx_queue_pending(const mb_tiny_rtu_rx_queue_t *queue) {
  uint16_t head;
  uint16_t tail;

  if (queue == NULL || queue->slots == NULL || queue->capacity < 2U) {
    return 0U;
  }
  head = queue->head;
  tail = queue->tail;
  return head >= tail ? (uint16_t)(head - tail)
                      : (uint16_t)(queue->capacity - tail + head);
}

bool mb_tiny_rtu_rx_queue_is_idle(const mb_tiny_rtu_rx_queue_t *queue,
                                  const mb_tiny_rtu_rx_t *rx) {
  return queue != NULL && rx != NULL &&
         mb_tiny_rtu_rx_queue_pending(queue) == 0U &&
         mb_tiny_rtu_rx_is_idle(rx);
}

int mb_tiny_rtu_rx_queue_push_isr(mb_tiny_rtu_rx_queue_t *queue, uint8_t byte,
                                  uint32_t timestamp_ms) {
  uint16_t head;
  uint16_t next;

  if (queue == NULL || queue->slots == NULL || queue->capacity < 2U) {
    return MB_TINY_INVALID_PARAM;
  }

  head = queue->head;
  next = (uint16_t)(head + 1U);
  if (next >= queue->capacity) {
    next = 0U;
  }
  if (next == queue->tail) {
    queue->dropped_bytes++;
    return MB_TINY_BUFFER_TOO_SMALL;
  }

  queue->slots[head].timestamp_ms = timestamp_ms;
  queue->slots[head].byte = byte;
  mb_tiny_compiler_barrier();
  queue->head = next;
  return MB_TINY_OK;
}

static bool mb_tiny_rtu_rx_queue_peek(const mb_tiny_rtu_rx_queue_t *queue,
                                      mb_tiny_rtu_rx_slot_t *slot) {
  if (queue->tail == queue->head) {
    return false;
  }
  mb_tiny_compiler_barrier();
  *slot = queue->slots[queue->tail];
  return true;
}

static void mb_tiny_rtu_rx_queue_consume(mb_tiny_rtu_rx_queue_t *queue) {
  uint16_t tail;

  tail = (uint16_t)(queue->tail + 1U);
  if (tail >= queue->capacity) {
    tail = 0U;
  }
  queue->tail = tail;
}

int mb_tiny_rtu_rx_queue_process(mb_tiny_rtu_rx_queue_t *queue,
                                 mb_tiny_rtu_rx_t *rx, uint32_t now_ms,
                                 uint8_t *frame, uint16_t frame_capacity,
                                 uint16_t *frame_len) {
  mb_tiny_rtu_rx_slot_t slot;
  int ret;

  if (queue == NULL || rx == NULL || frame == NULL || frame_len == NULL ||
      queue->slots == NULL || queue->capacity < 2U || rx->frame_gap_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }
  *frame_len = 0U;

  if (queue->handled_dropped_bytes != queue->dropped_bytes) {
    queue->handled_dropped_bytes = queue->dropped_bytes;
    queue->tail = queue->head;
    mb_tiny_rtu_rx_reset(rx);
    rx->dropped_frames++;
    rx->last_error = MB_TINY_RTU_RX_ERROR_QUEUE_OVERFLOW;
    return MB_TINY_FRAME_ERROR;
  }

  while (mb_tiny_rtu_rx_queue_peek(queue, &slot)) {
    if (rx->receiving &&
        (uint32_t)(slot.timestamp_ms - rx->last_byte_ms) >= rx->frame_gap_ms) {
      ret = mb_tiny_rtu_rx_poll(rx, slot.timestamp_ms, frame, frame_capacity,
                                frame_len);
      if (ret != MB_TINY_FRAME_READY) {
        return ret;
      }
      ret = mb_tiny_rtu_rx_feed(rx, slot.byte, slot.timestamp_ms);
      if (ret != MB_TINY_OK) {
        return ret;
      }
      mb_tiny_rtu_rx_queue_consume(queue);
      return MB_TINY_FRAME_READY;
    }

    ret = mb_tiny_rtu_rx_feed(rx, slot.byte, slot.timestamp_ms);
    mb_tiny_rtu_rx_queue_consume(queue);
    if (ret != MB_TINY_OK && ret != MB_TINY_IGNORED) {
      return ret;
    }
  }

  return mb_tiny_rtu_rx_poll(rx, now_ms, frame, frame_capacity, frame_len);
}

/* ==================== Non-blocking RS-485 TX ==================== */

int mb_tiny_rtu_tx_init(mb_tiny_rtu_tx_t *tx, mb_tiny_tx_start_cb_t start_cb,
                        mb_tiny_rs485_de_cb_t de_cb, uint32_t timeout_ms) {
  if (tx == NULL || start_cb == NULL || de_cb == NULL || timeout_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(tx, 0, sizeof(*tx));
  tx->start_cb = start_cb;
  tx->de_cb = de_cb;
  tx->timeout_ms = timeout_ms;
  tx->initialized = true;
  return MB_TINY_OK;
}

bool mb_tiny_rtu_tx_is_idle(const mb_tiny_rtu_tx_t *tx) {
  return tx != NULL && tx->initialized && !tx->transmitting;
}

int mb_tiny_rtu_tx_start(mb_tiny_rtu_tx_t *tx, const uint8_t *data,
                         uint16_t len, uint32_t now_ms) {
  int ret;

  if (tx == NULL || data == NULL || len == 0U || len > MB_TINY_MAX_ADU_SIZE) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!tx->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (tx->transmitting) {
    return MB_TINY_BUSY;
  }

  memcpy(tx->data, data, len);
  tx->len = len;
  tx->complete = false;
  if (tx->de_cb(true) != 0) {
    tx->error_count++;
    (void)tx->de_cb(false);
    return MB_TINY_IO_ERROR;
  }

  tx->started_ms = now_ms;
  tx->transmitting = true;
  ret = tx->start_cb(tx->data, len);
  if (ret != (int)len) {
    tx->transmitting = false;
    tx->complete = false;
    tx->error_count++;
    (void)tx->de_cb(false);
    return MB_TINY_IO_ERROR;
  }
  return MB_TINY_OK;
}

void mb_tiny_rtu_tx_complete_isr(mb_tiny_rtu_tx_t *tx) {
  if (tx != NULL && tx->initialized && tx->transmitting) {
    mb_tiny_compiler_barrier();
    tx->complete = true;
  }
}

int mb_tiny_rtu_tx_abort(mb_tiny_rtu_tx_t *tx) {
  int ret;

  if (tx == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!tx->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }

  tx->transmitting = false;
  tx->complete = false;
  ret = tx->de_cb(false);
  if (ret != 0) {
    tx->error_count++;
    return MB_TINY_IO_ERROR;
  }
  return MB_TINY_OK;
}

int mb_tiny_rtu_tx_process(mb_tiny_rtu_tx_t *tx, uint32_t now_ms) {
  int ret;
  bool completed;
  bool timed_out;

  if (tx == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!tx->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (!tx->transmitting) {
    return MB_TINY_IGNORED;
  }

  mb_tiny_compiler_barrier();
  completed = tx->complete;
  timed_out =
      !completed && (uint32_t)(now_ms - tx->started_ms) >= tx->timeout_ms;
  if (!completed && !timed_out) {
    return MB_TINY_BUSY;
  }

  tx->transmitting = false;
  tx->complete = false;
  ret = tx->de_cb(false);
  if (ret != 0) {
    tx->error_count++;
    return MB_TINY_IO_ERROR;
  }
  if (timed_out) {
    tx->error_count++;
    return MB_TINY_TIMEOUT;
  }

  tx->completed_count++;
  return MB_TINY_OK;
}

/* ==================== Non-blocking RTU master ==================== */

int mb_tiny_rtu_master_init(mb_tiny_rtu_master_t *master,
                            mb_tiny_rtu_rx_queue_t *queue, mb_tiny_rtu_rx_t *rx,
                            mb_tiny_rtu_tx_t *tx,
                            uint32_t response_timeout_ms) {
  if (master == NULL || queue == NULL || rx == NULL || tx == NULL ||
      response_timeout_ms == 0U || queue->slots == NULL ||
      queue->capacity < 2U || rx->frame_gap_ms == 0U || !tx->initialized) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(master, 0, sizeof(*master));
  master->queue = queue;
  master->rx = rx;
  master->tx = tx;
  master->response_timeout_ms = response_timeout_ms;
  master->state = MB_TINY_RTU_MASTER_IDLE;
  master->initialized = true;
  return MB_TINY_OK;
}

bool mb_tiny_rtu_master_is_idle(const mb_tiny_rtu_master_t *master) {
  return master != NULL && master->initialized &&
         master->state == MB_TINY_RTU_MASTER_IDLE;
}

void mb_tiny_rtu_master_reset(mb_tiny_rtu_master_t *master) {
  if (master != NULL && master->initialized &&
      master->state == MB_TINY_RTU_MASTER_DONE) {
    master->response_len = 0U;
    master->last_exception = 0U;
    master->typed_request = false;
    master->result = MB_TINY_OK;
    master->state = MB_TINY_RTU_MASTER_IDLE;
  }
}

static int mb_tiny_rtu_master_finish(mb_tiny_rtu_master_t *master, int result) {
  master->result = result;
  master->state = MB_TINY_RTU_MASTER_DONE;
  if (result == MB_TINY_OK || result == MB_TINY_EXCEPTION) {
    master->completed_count++;
  } else {
    master->error_count++;
  }
  return result;
}

int mb_tiny_rtu_master_start(mb_tiny_rtu_master_t *master,
                             const uint8_t *request, uint16_t request_len,
                             uint32_t now_ms) {
  int ret;

  if (master == NULL || request == NULL || request_len < MB_TINY_MIN_ADU_SIZE ||
      request_len > MB_TINY_MAX_ADU_SIZE) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->state != MB_TINY_RTU_MASTER_IDLE) {
    return MB_TINY_BUSY;
  }
  if (request[0] == 0U || !mb_unit_id_is_valid(request[0])) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!mb_frame_crc_is_valid(request, request_len)) {
    return MB_TINY_CRC_ERROR;
  }
  if (!mb_tiny_rtu_rx_queue_is_idle(master->queue, master->rx) ||
      !mb_tiny_rtu_tx_is_idle(master->tx)) {
    return MB_TINY_BUSY;
  }

  master->slave_id = request[0];
  master->function = request[1];
  master->last_exception = 0U;
  master->response_len = 0U;
  master->typed_request = false;
  master->result = MB_TINY_BUSY;
  ret = mb_tiny_rtu_tx_start(master->tx, request, request_len, now_ms);
  if (ret != MB_TINY_OK) {
    master->error_count++;
    return ret;
  }
  master->state = MB_TINY_RTU_MASTER_TRANSMITTING;
  return MB_TINY_OK;
}

int mb_tiny_rtu_master_abort(mb_tiny_rtu_master_t *master) {
  int ret;

  if (master == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }

  ret = MB_TINY_OK;
  if (master->state == MB_TINY_RTU_MASTER_TRANSMITTING) {
    ret = mb_tiny_rtu_tx_abort(master->tx);
  }
  mb_tiny_rtu_rx_reset(master->rx);
  master->queue->tail = master->queue->head;
  master->queue->handled_dropped_bytes = master->queue->dropped_bytes;
  master->response_len = 0U;
  master->last_exception = 0U;
  master->typed_request = false;
  master->result = ret;
  master->state = MB_TINY_RTU_MASTER_IDLE;
  return ret;
}

int mb_tiny_rtu_master_process(mb_tiny_rtu_master_t *master, uint32_t now_ms) {
  uint16_t response_len;
  int ret;

  if (master == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->state == MB_TINY_RTU_MASTER_IDLE) {
    return MB_TINY_IGNORED;
  }
  if (master->state == MB_TINY_RTU_MASTER_DONE) {
    return master->result;
  }

  if (master->state == MB_TINY_RTU_MASTER_TRANSMITTING) {
    ret = mb_tiny_rtu_tx_process(master->tx, now_ms);
    if (ret == MB_TINY_BUSY) {
      return MB_TINY_BUSY;
    }
    if (ret != MB_TINY_OK) {
      return mb_tiny_rtu_master_finish(master, ret);
    }
    master->response_started_ms = now_ms;
    master->state = MB_TINY_RTU_MASTER_WAITING_RESPONSE;
  }

  ret = mb_tiny_rtu_rx_queue_process(master->queue, master->rx, now_ms,
                                     master->response, MB_TINY_MAX_ADU_SIZE,
                                     &response_len);
  if (ret == MB_TINY_FRAME_READY) {
    master->response_len = response_len;
    if (!mb_frame_crc_is_valid(master->response, response_len)) {
      return mb_tiny_rtu_master_finish(master, MB_TINY_CRC_ERROR);
    }
    if (master->response[0] != master->slave_id) {
      return mb_tiny_rtu_master_finish(master, MB_TINY_FRAME_ERROR);
    }
    if (master->response[1] == (uint8_t)(master->function | 0x80U)) {
      if (response_len != 5U) {
        return mb_tiny_rtu_master_finish(master, MB_TINY_FRAME_ERROR);
      }
      master->last_exception = master->response[2];
      return mb_tiny_rtu_master_finish(master, MB_TINY_EXCEPTION);
    }
    if (master->response[1] != master->function) {
      return mb_tiny_rtu_master_finish(master, MB_TINY_FRAME_ERROR);
    }
    return mb_tiny_rtu_master_finish(master, MB_TINY_OK);
  }
  if (ret != MB_TINY_IGNORED) {
    return mb_tiny_rtu_master_finish(master, ret);
  }
  if ((uint32_t)(now_ms - master->response_started_ms) >=
      master->response_timeout_ms) {
    mb_tiny_rtu_rx_reset(master->rx);
    return mb_tiny_rtu_master_finish(master, MB_TINY_TIMEOUT);
  }
  return MB_TINY_BUSY;
}

int mb_tiny_rtu_master_get_response(const mb_tiny_rtu_master_t *master,
                                    uint8_t *response,
                                    uint16_t response_capacity,
                                    uint16_t *response_len) {
  if (master == NULL || response == NULL || response_len == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  *response_len = 0U;
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->state != MB_TINY_RTU_MASTER_DONE) {
    return MB_TINY_BUSY;
  }
  if (master->response_len == 0U) {
    return master->result;
  }
  if (response_capacity < master->response_len) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }

  memcpy(response, master->response, master->response_len);
  *response_len = master->response_len;
  return master->result;
}

static int mb_tiny_rtu_master_prepare_request(mb_tiny_rtu_master_t *master,
                                              uint8_t slave_id) {
  if (master == NULL || !mb_unit_id_is_valid(slave_id)) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->state != MB_TINY_RTU_MASTER_IDLE) {
    return MB_TINY_BUSY;
  }
  return MB_TINY_OK;
}

static int mb_tiny_rtu_master_start_fixed(mb_tiny_rtu_master_t *master,
                                          uint8_t slave_id, uint8_t function,
                                          uint16_t address, uint16_t value,
                                          uint32_t now_ms) {
  int ret;

  ret = mb_tiny_rtu_master_prepare_request(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  master->response[0] = slave_id;
  master->response[1] = function;
  mb_put_u16_be(&master->response[2], address);
  mb_put_u16_be(&master->response[4], value);
  mb_append_crc(master->response, 6U);
  ret = mb_tiny_rtu_master_start(master, master->response, 8U, now_ms);
  if (ret == MB_TINY_OK) {
    master->request_addr = address;
    master->request_value = value;
    master->typed_request = true;
  }
  return ret;
}

static int mb_tiny_rtu_master_read_start(mb_tiny_rtu_master_t *master,
                                         uint8_t slave_id, uint8_t function,
                                         uint16_t addr, uint16_t count,
                                         uint16_t max_count, uint32_t now_ms) {
  if (count == 0U || count > max_count) {
    return MB_TINY_INVALID_PARAM;
  }
  return mb_tiny_rtu_master_start_fixed(master, slave_id, function, addr, count,
                                        now_ms);
}

int mb_tiny_rtu_master_read_holding_start(mb_tiny_rtu_master_t *master,
                                          uint8_t slave_id, uint16_t addr,
                                          uint16_t count, uint32_t now_ms) {
  return mb_tiny_rtu_master_read_start(master, slave_id, MB_FUNC_READ_HOLDING,
                                       addr, count, MB_TINY_MAX_READ_REGS,
                                       now_ms);
}

int mb_tiny_rtu_master_read_input_start(mb_tiny_rtu_master_t *master,
                                        uint8_t slave_id, uint16_t addr,
                                        uint16_t count, uint32_t now_ms) {
  return mb_tiny_rtu_master_read_start(master, slave_id, MB_FUNC_READ_INPUT,
                                       addr, count, MB_TINY_MAX_READ_REGS,
                                       now_ms);
}

int mb_tiny_rtu_master_read_coils_start(mb_tiny_rtu_master_t *master,
                                        uint8_t slave_id, uint16_t addr,
                                        uint16_t count, uint32_t now_ms) {
  return mb_tiny_rtu_master_read_start(master, slave_id, MB_FUNC_READ_COILS,
                                       addr, count, MB_TINY_MAX_READ_BITS,
                                       now_ms);
}

int mb_tiny_rtu_master_read_discrete_start(mb_tiny_rtu_master_t *master,
                                           uint8_t slave_id, uint16_t addr,
                                           uint16_t count, uint32_t now_ms) {
  return mb_tiny_rtu_master_read_start(master, slave_id, MB_FUNC_READ_DISCRETE,
                                       addr, count, MB_TINY_MAX_READ_BITS,
                                       now_ms);
}

int mb_tiny_rtu_master_write_reg_start(mb_tiny_rtu_master_t *master,
                                       uint8_t slave_id, uint16_t addr,
                                       uint16_t value, uint32_t now_ms) {
  return mb_tiny_rtu_master_start_fixed(
      master, slave_id, MB_FUNC_WRITE_SINGLE_REG, addr, value, now_ms);
}

int mb_tiny_rtu_master_write_coil_start(mb_tiny_rtu_master_t *master,
                                        uint8_t slave_id, uint16_t addr,
                                        uint16_t value, uint32_t now_ms) {
  if (value != 0x0000U && value != 0xFF00U) {
    return MB_TINY_INVALID_PARAM;
  }
  return mb_tiny_rtu_master_start_fixed(
      master, slave_id, MB_FUNC_WRITE_SINGLE_COIL, addr, value, now_ms);
}

int mb_tiny_rtu_master_write_regs_start(mb_tiny_rtu_master_t *master,
                                        uint8_t slave_id, uint16_t addr,
                                        uint16_t count, const uint16_t *data,
                                        uint32_t now_ms) {
  uint16_t request_len;
  uint16_t i;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_WRITE_REGS) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_rtu_master_prepare_request(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  master->response[0] = slave_id;
  master->response[1] = MB_FUNC_WRITE_MULTIPLE_REGS;
  mb_put_u16_be(&master->response[2], addr);
  mb_put_u16_be(&master->response[4], count);
  master->response[6] = (uint8_t)(count * 2U);
  for (i = 0U; i < count; i++) {
    mb_put_u16_be(&master->response[7U + i * 2U], data[i]);
  }
  request_len = mb_append_crc(master->response, (uint16_t)(7U + count * 2U));
  ret = mb_tiny_rtu_master_start(master, master->response, request_len, now_ms);
  if (ret == MB_TINY_OK) {
    master->request_addr = addr;
    master->request_value = count;
    master->typed_request = true;
  }
  return ret;
}

int mb_tiny_rtu_master_write_coils_start(mb_tiny_rtu_master_t *master,
                                         uint8_t slave_id, uint16_t addr,
                                         uint16_t count, const uint8_t *data,
                                         uint32_t now_ms) {
  uint16_t byte_count;
  uint16_t request_len;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_WRITE_BITS) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_rtu_master_prepare_request(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  byte_count = (uint16_t)((count + 7U) / 8U);
  master->response[0] = slave_id;
  master->response[1] = MB_FUNC_WRITE_MULTIPLE_COILS;
  mb_put_u16_be(&master->response[2], addr);
  mb_put_u16_be(&master->response[4], count);
  master->response[6] = (uint8_t)byte_count;
  memcpy(&master->response[7], data, byte_count);
  if ((count % 8U) != 0U) {
    master->response[7U + byte_count - 1U] &=
        (uint8_t)((1U << (count % 8U)) - 1U);
  }
  request_len = mb_append_crc(master->response, (uint16_t)(7U + byte_count));
  ret = mb_tiny_rtu_master_start(master, master->response, request_len, now_ms);
  if (ret == MB_TINY_OK) {
    master->request_addr = addr;
    master->request_value = count;
    master->typed_request = true;
  }
  return ret;
}

static int mb_tiny_rtu_master_result_ready(const mb_tiny_rtu_master_t *master,
                                           uint8_t function) {
  if (master == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->state != MB_TINY_RTU_MASTER_DONE) {
    return MB_TINY_BUSY;
  }
  if (master->result != MB_TINY_OK) {
    return master->result;
  }
  if (!master->typed_request || master->function != function) {
    return MB_TINY_FRAME_ERROR;
  }
  return MB_TINY_OK;
}

static int
mb_tiny_rtu_master_read_registers_result(const mb_tiny_rtu_master_t *master,
                                         uint8_t function, uint16_t *data,
                                         uint16_t data_capacity) {
  uint16_t count;
  uint16_t expected_len;
  uint16_t i;
  int ret;

  if (data == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_rtu_master_result_ready(master, function);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  count = master->request_value;
  if (count == 0U || count > MB_TINY_MAX_READ_REGS) {
    return MB_TINY_FRAME_ERROR;
  }
  if (data_capacity < count) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  expected_len = (uint16_t)(5U + count * 2U);
  if (master->response_len != expected_len ||
      master->response[2] != (uint8_t)(count * 2U)) {
    return MB_TINY_FRAME_ERROR;
  }
  for (i = 0U; i < count; i++) {
    data[i] = mb_get_u16_be(&master->response[3U + i * 2U]);
  }
  return MB_TINY_OK;
}

int mb_tiny_rtu_master_read_holding_result(const mb_tiny_rtu_master_t *master,
                                           uint16_t *data,
                                           uint16_t data_capacity) {
  return mb_tiny_rtu_master_read_registers_result(master, MB_FUNC_READ_HOLDING,
                                                  data, data_capacity);
}

int mb_tiny_rtu_master_read_input_result(const mb_tiny_rtu_master_t *master,
                                         uint16_t *data,
                                         uint16_t data_capacity) {
  return mb_tiny_rtu_master_read_registers_result(master, MB_FUNC_READ_INPUT,
                                                  data, data_capacity);
}

static int
mb_tiny_rtu_master_read_bits_result(const mb_tiny_rtu_master_t *master,
                                    uint8_t function, uint8_t *data,
                                    uint16_t data_capacity) {
  uint16_t count;
  uint16_t byte_count;
  int ret;

  if (data == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_rtu_master_result_ready(master, function);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  count = master->request_value;
  if (count == 0U || count > MB_TINY_MAX_READ_BITS) {
    return MB_TINY_FRAME_ERROR;
  }
  byte_count = (uint16_t)((count + 7U) / 8U);
  if (data_capacity < byte_count) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  if (master->response_len != (uint16_t)(5U + byte_count) ||
      master->response[2] != (uint8_t)byte_count) {
    return MB_TINY_FRAME_ERROR;
  }
  memcpy(data, &master->response[3], byte_count);
  if ((count % 8U) != 0U) {
    data[byte_count - 1U] &= (uint8_t)((1U << (count % 8U)) - 1U);
  }
  return MB_TINY_OK;
}

int mb_tiny_rtu_master_read_coils_result(const mb_tiny_rtu_master_t *master,
                                         uint8_t *data,
                                         uint16_t data_capacity) {
  return mb_tiny_rtu_master_read_bits_result(master, MB_FUNC_READ_COILS, data,
                                             data_capacity);
}

int mb_tiny_rtu_master_read_discrete_result(const mb_tiny_rtu_master_t *master,
                                            uint8_t *data,
                                            uint16_t data_capacity) {
  return mb_tiny_rtu_master_read_bits_result(master, MB_FUNC_READ_DISCRETE,
                                             data, data_capacity);
}

static int mb_tiny_rtu_master_write_result(const mb_tiny_rtu_master_t *master,
                                           uint8_t function) {
  int ret;

  ret = mb_tiny_rtu_master_result_ready(master, function);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  if (master->response_len != 8U ||
      mb_get_u16_be(&master->response[2]) != master->request_addr ||
      mb_get_u16_be(&master->response[4]) != master->request_value) {
    return MB_TINY_FRAME_ERROR;
  }
  return MB_TINY_OK;
}

int mb_tiny_rtu_master_write_reg_result(const mb_tiny_rtu_master_t *master) {
  return mb_tiny_rtu_master_write_result(master, MB_FUNC_WRITE_SINGLE_REG);
}

int mb_tiny_rtu_master_write_coil_result(const mb_tiny_rtu_master_t *master) {
  return mb_tiny_rtu_master_write_result(master, MB_FUNC_WRITE_SINGLE_COIL);
}

int mb_tiny_rtu_master_write_regs_result(const mb_tiny_rtu_master_t *master) {
  return mb_tiny_rtu_master_write_result(master, MB_FUNC_WRITE_MULTIPLE_REGS);
}

int mb_tiny_rtu_master_write_coils_result(const mb_tiny_rtu_master_t *master) {
  return mb_tiny_rtu_master_write_result(master, MB_FUNC_WRITE_MULTIPLE_COILS);
}

int mb_tiny_rtu_slave_poll(mb_tiny_slave_t *slave,
                           mb_tiny_rtu_rx_queue_t *queue, mb_tiny_rtu_rx_t *rx,
                           mb_tiny_rtu_tx_t *tx, uint32_t now_ms,
                           uint8_t *request, uint16_t request_capacity,
                           uint8_t *response, uint16_t response_capacity) {
  uint16_t request_len;
  uint16_t response_len;
  int ret;

  if (slave == NULL || queue == NULL || rx == NULL || tx == NULL ||
      request == NULL || response == NULL || request == response ||
      request_capacity == 0U || response_capacity == 0U) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!slave->initialized || !tx->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }

  if (!mb_tiny_rtu_tx_is_idle(tx)) {
    return mb_tiny_rtu_tx_process(tx, now_ms);
  }

  ret = mb_tiny_rtu_rx_queue_process(queue, rx, now_ms, request,
                                     request_capacity, &request_len);
  if (ret != MB_TINY_FRAME_READY) {
    return ret;
  }

  ret = mb_tiny_slave_process(slave, request, request_len, response,
                              response_capacity, &response_len);
  if (ret == MB_TINY_IGNORED || ret == MB_TINY_NO_RESPONSE) {
    return ret;
  }
  if (ret != MB_TINY_OK && ret != MB_TINY_EXCEPTION) {
    return ret;
  }

  ret = mb_tiny_rtu_tx_start(tx, response, response_len, now_ms);
  return ret == MB_TINY_OK ? MB_TINY_RESPONSE_STARTED : ret;
}

/* ==================== Slave implementation ==================== */

int mb_tiny_slave_init(mb_tiny_slave_t *slave, uint8_t slave_id) {
  if (slave == NULL || !mb_unit_id_is_valid(slave_id)) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(slave, 0, sizeof(*slave));
  slave->slave_id = slave_id;
  slave->initialized = true;
  return MB_TINY_OK;
}

int mb_tiny_slave_config_holding(mb_tiny_slave_t *slave, uint16_t *data,
                                 uint16_t start_addr, uint16_t count) {
  if (slave == NULL || !slave->initialized || data == NULL || count == 0U ||
      (uint32_t)start_addr + count > 0x10000UL) {
    return MB_TINY_INVALID_PARAM;
  }

  slave->holding.data = data;
  slave->holding.start_addr = start_addr;
  slave->holding.count = count;
  return MB_TINY_OK;
}

int mb_tiny_slave_config_coils(mb_tiny_slave_t *slave, uint8_t *data,
                               uint16_t start_addr, uint16_t count) {
  if (slave == NULL || !slave->initialized || data == NULL || count == 0U ||
      (uint32_t)start_addr + count > 0x10000UL) {
    return MB_TINY_INVALID_PARAM;
  }

  slave->coils.data = data;
  slave->coils.start_addr = start_addr;
  slave->coils.count = count;
  return MB_TINY_OK;
}

int mb_tiny_slave_config_input(mb_tiny_slave_t *slave, uint16_t *data,
                               uint16_t start_addr, uint16_t count) {
  if (slave == NULL || !slave->initialized || data == NULL || count == 0U ||
      (uint32_t)start_addr + count > 0x10000UL) {
    return MB_TINY_INVALID_PARAM;
  }

  slave->input.data = data;
  slave->input.start_addr = start_addr;
  slave->input.count = count;
  return MB_TINY_OK;
}

int mb_tiny_slave_config_discrete(mb_tiny_slave_t *slave, uint8_t *data,
                                  uint16_t start_addr, uint16_t count) {
  if (slave == NULL || !slave->initialized || data == NULL || count == 0U ||
      (uint32_t)start_addr + count > 0x10000UL) {
    return MB_TINY_INVALID_PARAM;
  }

  slave->discrete.data = data;
  slave->discrete.start_addr = start_addr;
  slave->discrete.count = count;
  return MB_TINY_OK;
}

void mb_tiny_slave_set_send(mb_tiny_slave_t *slave, mb_tiny_send_cb_t send_cb) {
  if (slave != NULL) {
    slave->send_cb = send_cb;
  }
}

static int mb_tiny_slave_send(mb_tiny_slave_t *slave, const uint8_t *data,
                              uint16_t len) {
  int ret;

  if (slave->send_cb == NULL) {
    return MB_TINY_IO_ERROR;
  }

  ret = slave->send_cb(data, len);
  return ret == (int)len ? MB_TINY_OK : MB_TINY_IO_ERROR;
}

static int mb_tiny_slave_finish(mb_tiny_slave_t *slave, uint8_t *response,
                                uint16_t response_capacity,
                                uint16_t payload_len, uint16_t *response_len,
                                bool broadcast) {
  if (broadcast) {
    *response_len = 0U;
    slave->request_count++;
    return MB_TINY_NO_RESPONSE;
  }
  if ((uint32_t)payload_len + 2U > response_capacity) {
    slave->error_count++;
    return MB_TINY_BUFFER_TOO_SMALL;
  }

  *response_len = mb_append_crc(response, payload_len);
  slave->request_count++;
  return MB_TINY_OK;
}

static int mb_tiny_slave_exception(mb_tiny_slave_t *slave, uint8_t function,
                                   uint8_t exception, bool broadcast,
                                   uint8_t *response,
                                   uint16_t response_capacity,
                                   uint16_t *response_len) {
  slave->error_count++;
  if (broadcast) {
    *response_len = 0U;
    return MB_TINY_NO_RESPONSE;
  }
  if (response_capacity < 5U) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }

  response[0] = slave->slave_id;
  response[1] = (uint8_t)(function | 0x80U);
  response[2] = exception;
  *response_len = mb_append_crc(response, 3U);
  return MB_TINY_EXCEPTION;
}

int mb_tiny_slave_process(mb_tiny_slave_t *slave, const uint8_t *data,
                          uint16_t len, uint8_t *response,
                          uint16_t response_capacity, uint16_t *response_len) {
  uint8_t function;
  uint16_t address;
  uint16_t quantity;
  uint16_t i;
  bool broadcast;

  if (slave == NULL || data == NULL || response == NULL ||
      response_len == NULL || response_capacity == 0U || response == data) {
    return MB_TINY_INVALID_PARAM;
  }
  *response_len = 0U;
  if (!slave->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (len < MB_TINY_MIN_ADU_SIZE || len > MB_TINY_MAX_ADU_SIZE) {
    slave->error_count++;
    return MB_TINY_FRAME_ERROR;
  }
  if (!mb_frame_crc_is_valid(data, len)) {
    slave->error_count++;
    return MB_TINY_CRC_ERROR;
  }

  broadcast = data[0] == 0U;
  if (!broadcast && data[0] != slave->slave_id) {
    return MB_TINY_IGNORED;
  }

  function = data[1];
  switch (function) {
  case MB_FUNC_READ_HOLDING:
  case MB_FUNC_READ_INPUT: {
    const mb_tiny_holding_t *map;
    uint16_t payload_len;

    map = function == MB_FUNC_READ_HOLDING ? &slave->holding : &slave->input;

    if (broadcast) {
      return MB_TINY_IGNORED;
    }
    if (len != 8U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    quantity = mb_get_u16_be(&data[4]);
    if (quantity == 0U || quantity > MB_TINY_MAX_READ_REGS) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_VALUE,
                                     false, response, response_capacity,
                                     response_len);
    }
    if (map->data == NULL ||
        !mb_range_contains(map->start_addr, map->count, address, quantity)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     false, response, response_capacity,
                                     response_len);
    }

    payload_len = (uint16_t)(3U + quantity * 2U);
    if ((uint32_t)payload_len + 2U > response_capacity) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }
    response[0] = slave->slave_id;
    response[1] = function;
    response[2] = (uint8_t)(quantity * 2U);
    for (i = 0U; i < quantity; i++) {
      uint16_t value;
      value = map->data[(uint16_t)(address - map->start_addr + i)];
      mb_put_u16_be(&response[3U + i * 2U], value);
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, payload_len,
                                response_len, false);
  }

  case MB_FUNC_WRITE_SINGLE_REG:
    if (len != 8U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    if (slave->holding.data == NULL ||
        !mb_range_contains(slave->holding.start_addr, slave->holding.count,
                           address, 1U)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (!broadcast && response_capacity < 8U) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }
    slave->holding.data[address - slave->holding.start_addr] =
        mb_get_u16_be(&data[4]);
    if (!broadcast) {
      memcpy(response, data, 6U);
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, 6U,
                                response_len, broadcast);

  case MB_FUNC_WRITE_MULTIPLE_REGS: {
    uint8_t byte_count;
    uint32_t expected_len;

    if (len < 9U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    quantity = mb_get_u16_be(&data[4]);
    byte_count = data[6];
    expected_len = 9UL + (uint32_t)byte_count;
    if (expected_len != len) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    if (quantity == 0U || quantity > MB_TINY_MAX_WRITE_REGS ||
        (uint16_t)byte_count != (uint16_t)(quantity * 2U)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_VALUE,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (slave->holding.data == NULL ||
        !mb_range_contains(slave->holding.start_addr, slave->holding.count,
                           address, quantity)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (!broadcast && response_capacity < 8U) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }

    for (i = 0U; i < quantity; i++) {
      slave->holding.data[(uint16_t)(address - slave->holding.start_addr + i)] =
          mb_get_u16_be(&data[7U + i * 2U]);
    }
    if (!broadcast) {
      response[0] = slave->slave_id;
      response[1] = function;
      memcpy(&response[2], &data[2], 4U);
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, 6U,
                                response_len, broadcast);
  }

  case MB_FUNC_READ_COILS:
  case MB_FUNC_READ_DISCRETE: {
    const mb_tiny_coils_t *map;
    uint16_t byte_count;
    uint16_t payload_len;

    map = function == MB_FUNC_READ_COILS ? &slave->coils : &slave->discrete;

    if (broadcast) {
      return MB_TINY_IGNORED;
    }
    if (len != 8U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    quantity = mb_get_u16_be(&data[4]);
    if (quantity == 0U || quantity > MB_TINY_MAX_READ_BITS) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_VALUE,
                                     false, response, response_capacity,
                                     response_len);
    }
    if (map->data == NULL ||
        !mb_range_contains(map->start_addr, map->count, address, quantity)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     false, response, response_capacity,
                                     response_len);
    }

    byte_count = (uint16_t)((quantity + 7U) / 8U);
    payload_len = (uint16_t)(3U + byte_count);
    if ((uint32_t)payload_len + 2U > response_capacity) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }
    response[0] = slave->slave_id;
    response[1] = function;
    response[2] = (uint8_t)byte_count;
    memset(&response[3], 0, byte_count);
    for (i = 0U; i < quantity; i++) {
      if (mb_coil_get(map, (uint16_t)(address + i))) {
        response[3U + i / 8U] |= (uint8_t)(1U << (i % 8U));
      }
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, payload_len,
                                response_len, false);
  }

  case MB_FUNC_WRITE_MULTIPLE_COILS: {
    uint8_t byte_count;
    uint16_t expected_byte_count;
    uint32_t expected_len;

    if (len < 10U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    quantity = mb_get_u16_be(&data[4]);
    byte_count = data[6];
    expected_len = 9UL + (uint32_t)byte_count;
    if (expected_len != len) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    expected_byte_count = (uint16_t)((quantity + 7U) / 8U);
    if (quantity == 0U || quantity > MB_TINY_MAX_WRITE_BITS ||
        byte_count != expected_byte_count) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_VALUE,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (slave->coils.data == NULL ||
        !mb_range_contains(slave->coils.start_addr, slave->coils.count, address,
                           quantity)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (!broadcast && response_capacity < 8U) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }

    for (i = 0U; i < quantity; i++) {
      bool value;
      value = (data[7U + i / 8U] & (uint8_t)(1U << (i % 8U))) != 0U;
      mb_coil_set(&slave->coils, (uint16_t)(address + i), value);
    }
    if (!broadcast) {
      response[0] = slave->slave_id;
      response[1] = function;
      memcpy(&response[2], &data[2], 4U);
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, 6U,
                                response_len, broadcast);
  }

  case MB_FUNC_WRITE_SINGLE_COIL: {
    uint16_t value;

    if (len != 8U) {
      slave->error_count++;
      return MB_TINY_FRAME_ERROR;
    }
    address = mb_get_u16_be(&data[2]);
    value = mb_get_u16_be(&data[4]);
    if (value != 0xFF00U && value != 0x0000U) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_VALUE,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (slave->coils.data == NULL ||
        !mb_range_contains(slave->coils.start_addr, slave->coils.count, address,
                           1U)) {
      return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_DATA_ADDR,
                                     broadcast, response, response_capacity,
                                     response_len);
    }
    if (!broadcast && response_capacity < 8U) {
      slave->error_count++;
      return MB_TINY_BUFFER_TOO_SMALL;
    }

    mb_coil_set(&slave->coils, address, value == 0xFF00U);
    if (!broadcast) {
      memcpy(response, data, 6U);
    }
    return mb_tiny_slave_finish(slave, response, response_capacity, 6U,
                                response_len, broadcast);
  }

  default:
    return mb_tiny_slave_exception(slave, function, MB_ERR_ILLEGAL_FUNC,
                                   broadcast, response, response_capacity,
                                   response_len);
  }
}

int mb_tiny_slave_handle(mb_tiny_slave_t *slave, const uint8_t *data,
                         uint16_t len) {
  uint16_t response_len;
  int ret;

  if (slave == NULL) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_slave_process(slave, data, len, slave->rx_buf,
                              MB_TINY_MAX_ADU_SIZE, &response_len);
  if (ret == MB_TINY_IGNORED || ret == MB_TINY_NO_RESPONSE) {
    return MB_TINY_OK;
  }
  if (ret != MB_TINY_OK && ret != MB_TINY_EXCEPTION) {
    return ret;
  }
  if (mb_tiny_slave_send(slave, slave->rx_buf, response_len) != MB_TINY_OK) {
    slave->error_count++;
    return MB_TINY_IO_ERROR;
  }
  return ret;
}

/* ==================== Master implementation ==================== */

int mb_tiny_master_init(mb_tiny_master_t *master) {
  if (master == NULL) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(master, 0, sizeof(*master));
  master->timeout_ms = MB_TINY_TIMEOUT_MS;
  master->initialized = true;
  return MB_TINY_OK;
}

void mb_tiny_master_set_uart(mb_tiny_master_t *master,
                             mb_tiny_send_cb_t send_cb,
                             mb_tiny_recv_cb_t recv_cb) {
  if (master != NULL) {
    master->send_cb = send_cb;
    master->recv_cb = recv_cb;
  }
}

void mb_tiny_master_set_timeout(mb_tiny_master_t *master, uint32_t timeout_ms) {
  if (master != NULL && timeout_ms != 0U) {
    master->timeout_ms = timeout_ms;
  }
}

static int mb_tiny_master_send(mb_tiny_master_t *master, uint16_t len) {
  int ret;

  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  if (master->send_cb == NULL) {
    return MB_TINY_IO_ERROR;
  }

  ret = master->send_cb(master->tx_buf, len);
  return ret == (int)len ? MB_TINY_OK : MB_TINY_IO_ERROR;
}

static int mb_tiny_master_receive(mb_tiny_master_t *master) {
  int ret;

  if (master->recv_cb == NULL) {
    return MB_TINY_IO_ERROR;
  }

  ret =
      master->recv_cb(master->rx_buf, MB_TINY_MAX_ADU_SIZE, master->timeout_ms);
  if (ret == 0) {
    return MB_TINY_TIMEOUT;
  }
  if (ret < 0) {
    return MB_TINY_IO_ERROR;
  }
  if (ret > (int)MB_TINY_MAX_ADU_SIZE || ret < (int)MB_TINY_MIN_ADU_SIZE) {
    return MB_TINY_FRAME_ERROR;
  }

  master->rx_len = (uint16_t)ret;
  if (!mb_frame_crc_is_valid(master->rx_buf, master->rx_len)) {
    return MB_TINY_CRC_ERROR;
  }
  return MB_TINY_OK;
}

static int mb_tiny_master_validate_header(mb_tiny_master_t *master,
                                          uint8_t slave_id, uint8_t function) {
  if (master->rx_buf[0] != slave_id) {
    return MB_TINY_FRAME_ERROR;
  }
  if (master->rx_buf[1] == (uint8_t)(function | 0x80U)) {
    if (master->rx_len != 5U) {
      return MB_TINY_FRAME_ERROR;
    }
    master->last_exception = master->rx_buf[2];
    return MB_TINY_EXCEPTION;
  }
  if (master->rx_buf[1] != function) {
    return MB_TINY_FRAME_ERROR;
  }
  return MB_TINY_OK;
}

static int mb_tiny_master_exchange(mb_tiny_master_t *master, uint16_t tx_len,
                                   uint8_t slave_id, uint8_t function) {
  int ret;

  master->slave_id = slave_id;
  master->tx_len = tx_len;
  master->rx_len = 0U;
  master->last_exception = 0U;

  ret = mb_tiny_master_send(master, tx_len);
  if (ret != MB_TINY_OK) {
    master->error_count++;
    return ret;
  }
  ret = mb_tiny_master_receive(master);
  if (ret != MB_TINY_OK) {
    master->error_count++;
    return ret;
  }
  ret = mb_tiny_master_validate_header(master, slave_id, function);
  if (ret != MB_TINY_OK) {
    master->error_count++;
  }
  return ret;
}

static int mb_tiny_master_validate_args(mb_tiny_master_t *master,
                                        uint8_t slave_id) {
  if (master == NULL || !mb_unit_id_is_valid(slave_id)) {
    return MB_TINY_INVALID_PARAM;
  }
  if (!master->initialized) {
    return MB_TINY_NOT_INITIALIZED;
  }
  return MB_TINY_OK;
}

static void mb_tiny_master_build_fixed_request(mb_tiny_master_t *master,
                                               uint8_t slave_id,
                                               uint8_t function,
                                               uint16_t address,
                                               uint16_t value) {
  master->tx_buf[0] = slave_id;
  master->tx_buf[1] = function;
  mb_put_u16_be(&master->tx_buf[2], address);
  mb_put_u16_be(&master->tx_buf[4], value);
  mb_append_crc(master->tx_buf, 6U);
}

static int mb_tiny_master_read_registers(mb_tiny_master_t *master,
                                         uint8_t slave_id, uint8_t function,
                                         uint16_t addr, uint16_t count,
                                         uint16_t *data,
                                         uint16_t data_capacity) {
  uint16_t expected_len;
  uint16_t i;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_READ_REGS) {
    return MB_TINY_INVALID_PARAM;
  }
  if (data_capacity < count) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  mb_tiny_master_build_fixed_request(master, slave_id, function, addr, count);
  ret = mb_tiny_master_exchange(master, 8U, slave_id, function);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  expected_len = (uint16_t)(5U + count * 2U);
  if (master->rx_len != expected_len ||
      master->rx_buf[2] != (uint8_t)(count * 2U)) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }
  for (i = 0U; i < count; i++) {
    data[i] = mb_get_u16_be(&master->rx_buf[3U + i * 2U]);
  }

  master->request_count++;
  return MB_TINY_OK;
}

int mb_tiny_master_read_holding(mb_tiny_master_t *master, uint8_t slave_id,
                                uint16_t addr, uint16_t count, uint16_t *data) {
  return mb_tiny_master_read_holding_ex(master, slave_id, addr, count, data,
                                        count);
}

int mb_tiny_master_read_holding_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                   uint16_t addr, uint16_t count,
                                   uint16_t *data, uint16_t data_capacity) {
  return mb_tiny_master_read_registers(master, slave_id, MB_FUNC_READ_HOLDING,
                                       addr, count, data, data_capacity);
}

int mb_tiny_master_read_input(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint16_t *data) {
  return mb_tiny_master_read_input_ex(master, slave_id, addr, count, data,
                                      count);
}

int mb_tiny_master_read_input_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint16_t *data,
                                 uint16_t data_capacity) {
  return mb_tiny_master_read_registers(master, slave_id, MB_FUNC_READ_INPUT,
                                       addr, count, data, data_capacity);
}

int mb_tiny_master_write_reg(mb_tiny_master_t *master, uint8_t slave_id,
                             uint16_t addr, uint16_t value) {
  int ret;

  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  mb_tiny_master_build_fixed_request(master, slave_id, MB_FUNC_WRITE_SINGLE_REG,
                                     addr, value);
  ret = mb_tiny_master_exchange(master, 8U, slave_id, MB_FUNC_WRITE_SINGLE_REG);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  if (master->rx_len != 8U || memcmp(master->rx_buf, master->tx_buf, 6U) != 0) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }

  master->request_count++;
  return MB_TINY_OK;
}

int mb_tiny_master_write_regs(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count,
                              const uint16_t *data) {
  uint16_t tx_len;
  uint16_t i;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_WRITE_REGS) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  master->tx_buf[0] = slave_id;
  master->tx_buf[1] = MB_FUNC_WRITE_MULTIPLE_REGS;
  mb_put_u16_be(&master->tx_buf[2], addr);
  mb_put_u16_be(&master->tx_buf[4], count);
  master->tx_buf[6] = (uint8_t)(count * 2U);
  for (i = 0U; i < count; i++) {
    mb_put_u16_be(&master->tx_buf[7U + i * 2U], data[i]);
  }
  tx_len = mb_append_crc(master->tx_buf, (uint16_t)(7U + count * 2U));

  ret = mb_tiny_master_exchange(master, tx_len, slave_id,
                                MB_FUNC_WRITE_MULTIPLE_REGS);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  if (master->rx_len != 8U ||
      memcmp(&master->rx_buf[2], &master->tx_buf[2], 4U) != 0) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }

  master->request_count++;
  return MB_TINY_OK;
}

static int mb_tiny_master_read_bits(mb_tiny_master_t *master, uint8_t slave_id,
                                    uint8_t function, uint16_t addr,
                                    uint16_t count, uint8_t *data,
                                    uint16_t data_capacity) {
  uint16_t byte_count;
  uint16_t expected_len;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_READ_BITS) {
    return MB_TINY_INVALID_PARAM;
  }
  byte_count = (uint16_t)((count + 7U) / 8U);
  if (data_capacity < byte_count) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  mb_tiny_master_build_fixed_request(master, slave_id, function, addr, count);
  ret = mb_tiny_master_exchange(master, 8U, slave_id, function);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  expected_len = (uint16_t)(5U + byte_count);
  if (master->rx_len != expected_len ||
      master->rx_buf[2] != (uint8_t)byte_count) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }
  memcpy(data, &master->rx_buf[3], byte_count);
  if ((count % 8U) != 0U) {
    data[byte_count - 1U] &= (uint8_t)((1U << (count % 8U)) - 1U);
  }

  master->request_count++;
  return MB_TINY_OK;
}

int mb_tiny_master_read_coils(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t count, uint8_t *data) {
  return mb_tiny_master_read_coils_ex(master, slave_id, addr, count, data,
                                      (uint16_t)((count + 7U) / 8U));
}

int mb_tiny_master_read_coils_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint8_t *data,
                                 uint16_t data_capacity) {
  return mb_tiny_master_read_bits(master, slave_id, MB_FUNC_READ_COILS, addr,
                                  count, data, data_capacity);
}

int mb_tiny_master_read_discrete(mb_tiny_master_t *master, uint8_t slave_id,
                                 uint16_t addr, uint16_t count, uint8_t *data) {
  return mb_tiny_master_read_discrete_ex(master, slave_id, addr, count, data,
                                         (uint16_t)((count + 7U) / 8U));
}

int mb_tiny_master_read_discrete_ex(mb_tiny_master_t *master, uint8_t slave_id,
                                    uint16_t addr, uint16_t count,
                                    uint8_t *data, uint16_t data_capacity) {
  return mb_tiny_master_read_bits(master, slave_id, MB_FUNC_READ_DISCRETE, addr,
                                  count, data, data_capacity);
}

int mb_tiny_master_write_coils(mb_tiny_master_t *master, uint8_t slave_id,
                               uint16_t addr, uint16_t count,
                               const uint8_t *data) {
  uint16_t byte_count;
  uint16_t tx_len;
  int ret;

  if (data == NULL || count == 0U || count > MB_TINY_MAX_WRITE_BITS) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  byte_count = (uint16_t)((count + 7U) / 8U);
  master->tx_buf[0] = slave_id;
  master->tx_buf[1] = MB_FUNC_WRITE_MULTIPLE_COILS;
  mb_put_u16_be(&master->tx_buf[2], addr);
  mb_put_u16_be(&master->tx_buf[4], count);
  master->tx_buf[6] = (uint8_t)byte_count;
  memcpy(&master->tx_buf[7], data, byte_count);
  if ((count % 8U) != 0U) {
    master->tx_buf[7U + byte_count - 1U] &=
        (uint8_t)((1U << (count % 8U)) - 1U);
  }
  tx_len = mb_append_crc(master->tx_buf, (uint16_t)(7U + byte_count));

  ret = mb_tiny_master_exchange(master, tx_len, slave_id,
                                MB_FUNC_WRITE_MULTIPLE_COILS);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  if (master->rx_len != 8U ||
      memcmp(&master->rx_buf[2], &master->tx_buf[2], 4U) != 0) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }

  master->request_count++;
  return MB_TINY_OK;
}

int mb_tiny_master_write_coil(mb_tiny_master_t *master, uint8_t slave_id,
                              uint16_t addr, uint16_t value) {
  int ret;

  if (value != 0x0000U && value != 0xFF00U) {
    return MB_TINY_INVALID_PARAM;
  }
  ret = mb_tiny_master_validate_args(master, slave_id);
  if (ret != MB_TINY_OK) {
    return ret;
  }

  mb_tiny_master_build_fixed_request(master, slave_id,
                                     MB_FUNC_WRITE_SINGLE_COIL, addr, value);
  ret =
      mb_tiny_master_exchange(master, 8U, slave_id, MB_FUNC_WRITE_SINGLE_COIL);
  if (ret != MB_TINY_OK) {
    return ret;
  }
  if (master->rx_len != 8U || memcmp(master->rx_buf, master->tx_buf, 6U) != 0) {
    master->error_count++;
    return MB_TINY_FRAME_ERROR;
  }

  master->request_count++;
  return MB_TINY_OK;
}
