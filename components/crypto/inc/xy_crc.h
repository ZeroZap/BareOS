/**
 * @file xy_crc.h
 * @brief CRC Calculation
 */

#ifndef XY_CRC_H
#define XY_CRC_H

#include <stdint.h>

uint32_t xy_crc32(const uint8_t *data, uint32_t length);
uint16_t xy_crc16(const uint8_t *data, uint32_t length);
uint8_t xy_crc8(const uint8_t *data, uint32_t length);

#endif /* XY_CRC_H */
