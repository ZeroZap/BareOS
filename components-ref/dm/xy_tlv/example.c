/**
 * @file example_tlv.c
 * @brief XY TLV Usage Examples
 */

#include "xy_tlv.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Custom type definitions for examples */
#define CFG_DEVICE_ID     0x1001
#define CFG_DEVICE_NAME   0x1002
#define CFG_FIRMWARE_VER  0x1003
#define CFG_WIFI_SSID     0x1004
#define CFG_WIFI_PASSWORD 0x1005
#define CFG_WIFI_ENABLED  0x1006

#define SENSOR_TEMPERATURE 0x2001
#define SENSOR_HUMIDITY    0x2002
#define SENSOR_PRESSURE    0x2003
#define SENSOR_TIMESTAMP   0x2004
#define SENSOR_BATTERY     0x2005

#define MSG_COMMAND 0x3001
#define MSG_STATUS  0x3002
#define MSG_PAYLOAD 0x3003

/* ==================== Example 1: Basic Encoding/Decoding ====================
 */

void example_basic_encode_decode(void)
{
    printf("\n=== Example 1: Basic Encoding/Decoding ===\n");

    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;
    int ret;

    /* Initialize buffer */
    ret = xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    if (ret != XY_TLV_OK) {
        printf(
            "Failed to initialize buffer: %s\n", xy_tlv_get_error_string(ret));
        return;
    }

    /* Encode various types */
    xy_tlv_encode_uint32(&tlv_buf, CFG_DEVICE_ID, 0x12345678);
    xy_tlv_encode_string(&tlv_buf, CFG_DEVICE_NAME, "XY Device");
    xy_tlv_encode_uint16(&tlv_buf, CFG_FIRMWARE_VER, 0x0102);
    xy_tlv_encode_bool(&tlv_buf, CFG_WIFI_ENABLED, true);

    uint16_t encoded_size = xy_tlv_buffer_get_used(&tlv_buf);
    printf("Encoded %u bytes\n", encoded_size);

    /* Decode */
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;

    xy_tlv_iterator_init(&iter, buffer, encoded_size);

    while ((ret = xy_tlv_iterator_next(&iter, &tlv)) == XY_TLV_OK) {
        printf("TLV Type: 0x%04X (%s), Length: %u\n", tlv.type,
               xy_tlv_get_type_name(tlv.type), tlv.length);

        switch (tlv.type) {
        case CFG_DEVICE_ID: {
            uint32_t id;
            xy_tlv_decode_uint32(&tlv, &id);
            printf("  Device ID: 0x%08X\n", id);
            break;
        }
        case CFG_DEVICE_NAME: {
            char name[64];
            xy_tlv_decode_string(&tlv, name, sizeof(name));
            printf("  Device Name: %s\n", name);
            break;
        }
        case CFG_FIRMWARE_VER: {
            uint16_t ver;
            xy_tlv_decode_uint16(&tlv, &ver);
            printf("  Firmware Version: %u.%u\n", (ver >> 8), (ver & 0xFF));
            break;
        }
        case CFG_WIFI_ENABLED: {
            bool enabled;
            xy_tlv_decode_bool(&tlv, &enabled);
            printf("  WiFi Enabled: %s\n", enabled ? "Yes" : "No");
            break;
        }
        }
    }
}

/* ==================== Example 2: Sensor Data Packet ==================== */

void example_sensor_data(void)
{
    printf("\n=== Example 2: Sensor Data Packet ===\n");

    uint8_t sensor_buf[128];
    xy_tlv_buffer_t sensor;

    /* Encode sensor readings */
    xy_tlv_buffer_init(&sensor, sensor_buf, sizeof(sensor_buf));

    /* Temperature: 23.5°C (stored as 235 = 23.5 * 10) */
    xy_tlv_encode_int16(&sensor, SENSOR_TEMPERATURE, 235);

    /* Humidity: 65.2% (stored as 652 = 65.2 * 10) */
    xy_tlv_encode_uint16(&sensor, SENSOR_HUMIDITY, 652);

    /* Pressure: 101325 Pa */
    xy_tlv_encode_uint32(&sensor, SENSOR_PRESSURE, 101325);

    /* Timestamp: Unix time */
    xy_tlv_encode_uint32(&sensor, SENSOR_TIMESTAMP, (uint32_t)time(NULL));

    /* Battery: 87% */
    xy_tlv_encode_uint8(&sensor, SENSOR_BATTERY, 87);

    uint16_t size = xy_tlv_buffer_get_used(&sensor);
    printf("Sensor packet size: %u bytes\n", size);

    /* Add checksum for integrity */
    uint16_t crc = xy_tlv_checksum(sensor_buf, size);
    xy_tlv_encode_uint16(&sensor, XY_TLV_TYPE_CHECKSUM, crc);

    printf("Checksum: 0x%04X\n", crc);

    /* Decode and display */
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;

    xy_tlv_iterator_init(&iter, sensor_buf, xy_tlv_buffer_get_used(&sensor));

    while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
        switch (tlv.type) {
        case SENSOR_TEMPERATURE: {
            int16_t temp;
            xy_tlv_decode_int16(&tlv, &temp);
            printf("Temperature: %.1f°C\n", temp / 10.0);
            break;
        }
        case SENSOR_HUMIDITY: {
            uint16_t hum;
            xy_tlv_decode_uint16(&tlv, &hum);
            printf("Humidity: %.1f%%\n", hum / 10.0);
            break;
        }
        case SENSOR_PRESSURE: {
            uint32_t press;
            xy_tlv_decode_uint32(&tlv, &press);
            printf("Pressure: %u Pa\n", press);
            break;
        }
        case SENSOR_TIMESTAMP: {
            uint32_t ts;
            xy_tlv_decode_uint32(&tlv, &ts);
            printf("Timestamp: %u\n", ts);
            break;
        }
        case SENSOR_BATTERY: {
            uint8_t bat;
            xy_tlv_decode_uint8(&tlv, &bat);
            printf("Battery: %u%%\n", bat);
            break;
        }
        case XY_TLV_TYPE_CHECKSUM: {
            uint16_t recv_crc;
            xy_tlv_decode_uint16(&tlv, &recv_crc);
            printf("Received CRC: 0x%04X\n", recv_crc);
            break;
        }
        }
    }
}

/* ==================== Example 3: Configuration Storage ==================== */

typedef struct {
    uint32_t device_id;
    char device_name[32];
    char wifi_ssid[33];
    char wifi_password[65];
    bool wifi_enabled;
    uint16_t firmware_version;
} device_config_t;

int encode_device_config(const device_config_t *config, uint8_t *buffer,
                         uint16_t buffer_size)
{
    xy_tlv_buffer_t tlv_buf;

    xy_tlv_buffer_init(&tlv_buf, buffer, buffer_size);

    xy_tlv_encode_uint32(&tlv_buf, CFG_DEVICE_ID, config->device_id);
    xy_tlv_encode_string(&tlv_buf, CFG_DEVICE_NAME, config->device_name);
    xy_tlv_encode_string(&tlv_buf, CFG_WIFI_SSID, config->wifi_ssid);
    xy_tlv_encode_string(&tlv_buf, CFG_WIFI_PASSWORD, config->wifi_password);
    xy_tlv_encode_bool(&tlv_buf, CFG_WIFI_ENABLED, config->wifi_enabled);
    xy_tlv_encode_uint16(&tlv_buf, CFG_FIRMWARE_VER, config->firmware_version);

    return xy_tlv_buffer_get_used(&tlv_buf);
}

int decode_device_config(const uint8_t *buffer, uint16_t buffer_size,
                         device_config_t *config)
{
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    int ret;

    memset(config, 0, sizeof(device_config_t));

    ret = xy_tlv_iterator_init(&iter, buffer, buffer_size);
    if (ret != XY_TLV_OK) {
        return ret;
    }

    while ((ret = xy_tlv_iterator_next(&iter, &tlv)) == XY_TLV_OK) {
        switch (tlv.type) {
        case CFG_DEVICE_ID:
            xy_tlv_decode_uint32(&tlv, &config->device_id);
            break;
        case CFG_DEVICE_NAME:
            xy_tlv_decode_string(
                &tlv, config->device_name, sizeof(config->device_name));
            break;
        case CFG_WIFI_SSID:
            xy_tlv_decode_string(
                &tlv, config->wifi_ssid, sizeof(config->wifi_ssid));
            break;
        case CFG_WIFI_PASSWORD:
            xy_tlv_decode_string(
                &tlv, config->wifi_password, sizeof(config->wifi_password));
            break;
        case CFG_WIFI_ENABLED:
            xy_tlv_decode_bool(&tlv, &config->wifi_enabled);
            break;
        case CFG_FIRMWARE_VER:
            xy_tlv_decode_uint16(&tlv, &config->firmware_version);
            break;
        }
    }

    return XY_TLV_OK;
}

void example_config_storage(void)
{
    printf("\n=== Example 3: Configuration Storage ===\n");

    device_config_t config = {
        .device_id        = 0xABCD1234,
        .device_name      = "MyIoTDevice",
        .wifi_ssid        = "HomeNetwork",
        .wifi_password    = "SecurePass123",
        .wifi_enabled     = true,
        .firmware_version = 0x0203 // v2.3
    };

    uint8_t storage[512];
    int size;

    /* Encode configuration */
    size = encode_device_config(&config, storage, sizeof(storage));
    printf("Configuration encoded: %d bytes\n", size);

    /* Decode configuration */
    device_config_t decoded_config;
    int ret = decode_device_config(storage, size, &decoded_config);

    if (ret == XY_TLV_OK) {
        printf("Configuration decoded successfully:\n");
        printf("  Device ID: 0x%08X\n", decoded_config.device_id);
        printf("  Device Name: %s\n", decoded_config.device_name);
        printf("  WiFi SSID: %s\n", decoded_config.wifi_ssid);
        printf("  WiFi Password: %s\n", decoded_config.wifi_password);
        printf(
            "  WiFi Enabled: %s\n", decoded_config.wifi_enabled ? "Yes" : "No");
        printf("  Firmware: v%u.%u\n", (decoded_config.firmware_version >> 8),
               (decoded_config.firmware_version & 0xFF));
    }
}

/* ==================== Example 4: Finding Specific TLV ==================== */

void example_tlv_search(void)
{
    printf("\n=== Example 4: Finding Specific TLV ===\n");

    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;

    /* Encode mixed data */
    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    xy_tlv_encode_uint32(&tlv_buf, CFG_DEVICE_ID, 0x11111111);
    xy_tlv_encode_string(&tlv_buf, CFG_DEVICE_NAME, "Device1");
    xy_tlv_encode_uint32(&tlv_buf, CFG_DEVICE_ID, 0x22222222); // Duplicate type
    xy_tlv_encode_string(&tlv_buf, CFG_WIFI_SSID, "Network1");
    xy_tlv_encode_uint32(
        &tlv_buf, CFG_DEVICE_ID, 0x33333333); // Another duplicate

    uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);
    printf("Encoded %u bytes with multiple device IDs\n", size);

    /* Find first occurrence */
    xy_tlv_t found;
    if (xy_tlv_find(buffer, size, CFG_DEVICE_ID, &found) == XY_TLV_OK) {
        uint32_t id;
        xy_tlv_decode_uint32(&found, &id);
        printf("First Device ID found: 0x%08X\n", id);
    }

    /* Find all occurrences */
    xy_tlv_t found_array[10];
    uint16_t found_count = sizeof(found_array) / sizeof(found_array[0]);

    int ret =
        xy_tlv_find_all(buffer, size, CFG_DEVICE_ID, found_array, &found_count);
    if (ret >= 0) {
        printf("Found %u Device IDs:\n", found_count);
        for (uint16_t i = 0; i < found_count; i++) {
            uint32_t id;
            xy_tlv_decode_uint32(&found_array[i], &id);
            printf("  [%u] 0x%08X\n", i, id);
        }
    }

    /* Count total TLVs */
    int total = xy_tlv_count(buffer, size);
    printf("Total TLVs in buffer: %d\n", total);
}

/* ==================== Example 5: Validation and Error Handling
 * ==================== */

void example_validation(void)
{
    printf("\n=== Example 5: Validation and Error Handling ===\n");

    uint8_t buffer[128];
    xy_tlv_buffer_t tlv_buf;

    /* Create valid TLV data */
    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    xy_tlv_encode_uint32(&tlv_buf, CFG_DEVICE_ID, 0xDEADBEEF);
    xy_tlv_encode_string(&tlv_buf, CFG_DEVICE_NAME, "TestDevice");

    uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);

    /* Validate correct data */
    int ret = xy_tlv_validate(buffer, size);
    printf("Validation of correct data: %s\n", xy_tlv_get_error_string(ret));

    /* Corrupt the data (change length field) */
    buffer[2] = 0xFF; // Invalid length
    buffer[3] = 0xFF;

    ret = xy_tlv_validate(buffer, size);
    printf("Validation of corrupted data: %s\n", xy_tlv_get_error_string(ret));

    /* Test buffer overflow */
    xy_tlv_buffer_t small_buf;
    uint8_t tiny[10];

    xy_tlv_buffer_init(&small_buf, tiny, sizeof(tiny));
    ret = xy_tlv_encode_string(
        &small_buf, CFG_DEVICE_NAME, "This string is too long");
    printf("Encoding into small buffer: %s\n", xy_tlv_get_error_string(ret));

    /* Show available space */
    xy_tlv_buffer_init(&small_buf, tiny, sizeof(tiny));
    printf("Buffer capacity: %u bytes\n", sizeof(tiny));
    printf("Free space: %u bytes\n", xy_tlv_buffer_get_free(&small_buf));

    ret = xy_tlv_encode_uint32(&small_buf, CFG_DEVICE_ID, 12345);
    if (ret == XY_TLV_OK) {
        printf("After encoding uint32: Free space: %u bytes\n",
               xy_tlv_buffer_get_free(&small_buf));
    }
}

/* ==================== Example 6: Statistics ==================== */

void example_statistics(void)
{
    printf("\n=== Example 6: Statistics ===\n");

    xy_tlv_reset_stats();

    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;

    /* Perform some operations */
    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    xy_tlv_encode_uint32(&tlv_buf, 0x1001, 12345);
    xy_tlv_encode_string(&tlv_buf, 0x1002, "Test");
    xy_tlv_encode_bool(&tlv_buf, 0x1003, true);

    uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);

    /* Decode */
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;

    xy_tlv_iterator_init(&iter, buffer, size);
    while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
        /* Just iterate */
    }

    /* Get statistics */
    xy_tlv_stats_t stats;
    xy_tlv_get_stats(&stats);

    printf("TLV Statistics:\n");
    printf("  Total Encoded: %u\n", stats.total_encoded);
    printf("  Total Decoded: %u\n", stats.total_decoded);
    printf("  Bytes Encoded: %u\n", stats.bytes_encoded);
    printf("  Bytes Decoded: %u\n", stats.bytes_decoded);
    printf("  Encoding Errors: %u\n", stats.encoding_errors);
    printf("  Decoding Errors: %u\n", stats.decoding_errors);
}

/* ==================== Example 7: Binary Data ==================== */

void example_binary_data(void)
{
    printf("\n=== Example 7: Binary Data ===\n");

    uint8_t buffer[256];
    xy_tlv_buffer_t tlv_buf;

    /* Encode binary data (e.g., MAC address) */
    uint8_t mac_addr[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
    uint8_t uuid[16]    = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                            0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

    xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
    xy_tlv_encode_bytes(&tlv_buf, XY_TLV_TYPE_MAC_ADDR, mac_addr, 6);
    xy_tlv_encode_bytes(&tlv_buf, XY_TLV_TYPE_UUID, uuid, 16);

    uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);

    /* Decode binary data */
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;

    xy_tlv_iterator_init(&iter, buffer, size);

    while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
        if (tlv.type == XY_TLV_TYPE_MAC_ADDR) {
            uint8_t mac[6];
            uint16_t len = sizeof(mac);
            xy_tlv_decode_bytes(&tlv, mac, &len);
            printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                   mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else if (tlv.type == XY_TLV_TYPE_UUID) {
            uint8_t uid[16];
            uint16_t len = sizeof(uid);
            xy_tlv_decode_bytes(&tlv, uid, &len);
            printf("UUID: ");
            for (int i = 0; i < 16; i++) {
                printf("%02X", uid[i]);
                if (i == 3 || i == 5 || i == 7 || i == 9)
                    printf("-");
            }
            printf("\n");
        }
    }
}

/* ==================== Main ==================== */

int main(void)
{
    printf("XY TLV Management System - Examples\n");
    printf("====================================\n");

    example_basic_encode_decode();
    example_sensor_data();
    example_config_storage();
    example_tlv_search();
    example_validation();
    example_statistics();
    example_binary_data();

    printf("\n=== All examples completed ===\n");

    return 0;
}
