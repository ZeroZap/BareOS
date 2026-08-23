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

uint16_t mb_tiny_rtu_frame_gap_ms(uint32_t baud_rate, uint8_t bits_per_char) {
  uint32_t numerator;
  uint32_t gap_ms;

  if (baud_rate == 0U || bits_per_char < 8U || bits_per_char > 12U) {
    return 0U;
  }
  if (baud_rate > 19200UL) {
    return 2U;
  }

  numerator = (uint32_t)bits_per_char * 3500UL;
  gap_ms = (numerator + baud_rate - 1U) / baud_rate;
  return gap_ms <= 0xFFFFUL ? (uint16_t)gap_ms : 0U;
}

int mb_tiny_rtu_rx_init(mb_tiny_rtu_rx_t *rx, uint16_t frame_gap_ms) {
  if (rx == NULL || frame_gap_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }

  memset(rx, 0, sizeof(*rx));
  rx->frame_gap_ms = frame_gap_ms;
  return MB_TINY_OK;
}

void mb_tiny_rtu_rx_reset(mb_tiny_rtu_rx_t *rx) {
  if (rx != NULL) {
    rx->len = 0U;
    rx->receiving = false;
    rx->overflow = false;
  }
}

bool mb_tiny_rtu_rx_is_idle(const mb_tiny_rtu_rx_t *rx) {
  return rx != NULL && !rx->receiving;
}

int mb_tiny_rtu_rx_feed(mb_tiny_rtu_rx_t *rx, uint8_t byte, uint32_t now_ms) {
  bool missed_frame;

  if (rx == NULL || rx->frame_gap_ms == 0U) {
    return MB_TINY_INVALID_PARAM;
  }

  missed_frame = rx->receiving &&
                 (uint32_t)(now_ms - rx->last_byte_ms) >= rx->frame_gap_ms;
  if (missed_frame) {
    rx->dropped_frames++;
    mb_tiny_rtu_rx_reset(rx);
  }

  rx->receiving = true;
  rx->last_byte_ms = now_ms;
  if (rx->overflow) {
    return MB_TINY_BUFFER_TOO_SMALL;
  }
  if (rx->len >= MB_TINY_MAX_ADU_SIZE) {
    rx->overflow = true;
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
  if (rx->len < MB_TINY_MIN_ADU_SIZE) {
    rx->dropped_frames++;
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
