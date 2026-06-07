#include "secboot_n32_v1.h"

#include "n32l40x_cfg.h"
#include "n32l40x_flash.h"
#include "xy_log.h"
#include "xy_secboot.h"
#include "xy_secboot_guard.h"
#include "xy_sha256.h"
#include "xy_string.h"

typedef struct {
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint16_t seq;
    uint32_t session_id;
    uint32_t offset;
    uint16_t length;
} secboot_uart_frame_t;

static const xy_secboot_partition_t s_secboot_n32_parts[] = {
    { XY_SECBOOT_PART_BOOTLOADER,
      XY_SECBOOT_STORAGE_INTERNAL_FLASH,
      XY_SECBOOT_SLOT_NONE,
      SECBOOT_N32_BOOT_BASE_ADDR,
      SECBOOT_N32_BOOT_TOTAL_SIZE,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      XY_SECBOOT_PART_FLAG_READ_ONLY | XY_SECBOOT_PART_FLAG_BOOT_PRIV },

    { XY_SECBOOT_PART_BOOT_STATE,
      XY_SECBOOT_STORAGE_INTERNAL_FLASH,
      XY_SECBOOT_SLOT_NONE,
      SECBOOT_N32_STATE_BASE_ADDR,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      XY_SECBOOT_PART_FLAG_BOOT_PRIV },

    { XY_SECBOOT_PART_ROLLBACK,
      XY_SECBOOT_STORAGE_INTERNAL_FLASH,
      XY_SECBOOT_SLOT_NONE,
      SECBOOT_N32_ROLLBACK_BASE_ADDR,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      XY_SECBOOT_PART_FLAG_BOOT_PRIV | XY_SECBOOT_PART_FLAG_ROLLBACK },

    { XY_SECBOOT_PART_APP_A_MANIFEST,
      XY_SECBOOT_STORAGE_INTERNAL_FLASH,
      XY_SECBOOT_SLOT_A,
      SECBOOT_N32_APP_MANIFEST_ADDR,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      XY_SECBOOT_PART_FLAG_SIGNED },

    { XY_SECBOOT_PART_APP_A_IMAGE,
      XY_SECBOOT_STORAGE_INTERNAL_FLASH,
      XY_SECBOOT_SLOT_A,
      SECBOOT_N32_APP_IMAGE_ADDR,
      SECBOOT_N32_APP_IMAGE_SIZE,
      SECBOOT_N32_FLASH_PAGE_SIZE,
      XY_SECBOOT_PART_FLAG_EXECUTABLE | XY_SECBOOT_PART_FLAG_SIGNED },
};

static const xy_secboot_partition_table_t s_secboot_n32_table = {
    s_secboot_n32_parts,
    sizeof(s_secboot_n32_parts) / sizeof(s_secboot_n32_parts[0]),
};

static xy_secboot_manifest_t s_manifest;
static uint8_t s_manifest_valid;
static uint8_t s_download_active;
static uint16_t s_expected_seq;
static uint32_t s_expected_offset;
static uint32_t s_session_id;
static uint8_t s_payload[SECBOOT_N32_UART_V1_MAX_PAYLOAD];
static xy_secboot_single_ctx_t s_secboot_ctx;

static uint16_t le16_read(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32_read(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void le16_write(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void le32_write(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xffffu;

    while (len-- > 0u) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0u; i < 8u; i++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    crc = ~crc;
    while (len-- > 0u) {
        crc ^= *data++;
        for (uint8_t i = 0u; i < 8u; i++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1) ^ 0xedb88320u;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

static int flash_range_ok(uint32_t address, size_t len)
{
    uint32_t end = address + (uint32_t)len;

    if (end < address) {
        return 0;
    }
    if (address < SECBOOT_N32_FLASH_BASE_ADDR) {
        return 0;
    }
    if (end > (SECBOOT_N32_FLASH_BASE_ADDR + SECBOOT_N32_FLASH_TOTAL_SIZE)) {
        return 0;
    }
    return 1;
}

static int secboot_flash_read(uint32_t address, uint8_t *data, size_t len)
{
    if ((data == NULL && len != 0u) || !flash_range_ok(address, len)) {
        return -1;
    }
    memcpy(data, (const void *)address, len);
    return 0;
}

static int secboot_flash_erase(uint32_t address, size_t len)
{
    uint32_t end;

    if ((address & (SECBOOT_N32_FLASH_PAGE_SIZE - 1u)) != 0u ||
        !flash_range_ok(address, len)) {
        return -1;
    }

    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return -1;
    }

    end = address + (uint32_t)len;
    FLASH_Unlock();
    while (address < end) {
        if (FLASH_EraseOnePage(address) != FLASH_COMPL) {
            FLASH_Lock();
            return -1;
        }
        IWDG_ReloadKey();
        address += SECBOOT_N32_FLASH_PAGE_SIZE;
    }
    FLASH_Lock();
    return 0;
}

static int secboot_flash_write(uint32_t address, const uint8_t *data, size_t len)
{
    size_t i;

    if ((data == NULL && len != 0u) || !flash_range_ok(address, len) ||
        ((address | len) & 0x3u) != 0u) {
        return -1;
    }

    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return -1;
    }

    FLASH_Unlock();
    for (i = 0u; i < len; i += 4u) {
        uint32_t word;
        memcpy(&word, data + i, sizeof(word));
        if (FLASH_ProgramWord(address + (uint32_t)i, word) != FLASH_COMPL) {
            FLASH_Lock();
            return -1;
        }
        IWDG_ReloadKey();
    }
    FLASH_Lock();
    return 0;
}

static int secboot_hash_init(void *ctx, xy_secboot_alg_t alg)
{
    if (alg != XY_SECBOOT_ALG_HASH_SHA256) {
        return -1;
    }
    return xy_sha256_init((xy_sha256_ctx_t *)ctx);
}

static int secboot_hash_update(void *ctx, const uint8_t *data, size_t len)
{
    return xy_sha256_update((xy_sha256_ctx_t *)ctx, data, len);
}

static int secboot_hash_final(void *ctx, uint8_t *digest, size_t *digest_len)
{
    if (digest_len == NULL || *digest_len < XY_SHA256_DIGEST_SIZE) {
        return -1;
    }
    if (xy_sha256_final((xy_sha256_ctx_t *)ctx, digest) != 0) {
        return -1;
    }
    *digest_len = XY_SHA256_DIGEST_SIZE;
    return 0;
}

static int secboot_unsupported_verify(xy_secboot_alg_t alg,
                                      const uint8_t *pub_key,
                                      size_t pub_key_len,
                                      const uint8_t *msg,
                                      size_t msg_len,
                                      const uint8_t *sig,
                                      size_t sig_len)
{
    (void)alg;
    (void)pub_key;
    (void)pub_key_len;
    (void)msg;
    (void)msg_len;
    (void)sig;
    (void)sig_len;
    return -1;
}

static int secboot_get_key(xy_secboot_key_type_t type,
                           const uint8_t *key_id,
                           size_t key_id_len,
                           uint8_t *key,
                           size_t *key_len)
{
    (void)type;
    (void)key_id;
    (void)key_id_len;
    (void)key;
    (void)key_len;
    return -1;
}

static int secboot_rollback_read(uint32_t *counter)
{
    if (counter == NULL) {
        return -1;
    }
    *counter = 0u;
    return 0;
}

static const xy_secboot_crypto_ops_t s_secboot_crypto_ops = {
    NULL,
    secboot_hash_init,
    secboot_hash_update,
    secboot_hash_final,
    secboot_unsupported_verify,
    NULL,
    NULL,
    NULL,
    secboot_get_key,
    secboot_rollback_read,
    NULL,
    xy_secboot_ct_compare,
    xy_secboot_secure_zero,
    NULL,
};

static const xy_secboot_single_port_t s_secboot_port = {
    secboot_flash_read,
    secboot_flash_write,
    secboot_flash_erase,
    n32_uart5_secboot_read,
    n32_uart5_secboot_write,
    NULL,
    NULL,
};

static int manifest_basic_check(const xy_secboot_manifest_t *manifest)
{
    if (manifest->magic != XY_SECBOOT_MANIFEST_MAGIC ||
        manifest->header_version != XY_SECBOOT_MANIFEST_VERSION ||
        manifest->header_len < sizeof(xy_secboot_manifest_t) ||
        manifest->product_id != SECBOOT_N32_PRODUCT_ID ||
        manifest->image_type != (uint32_t)XY_SECBOOT_IMAGE_APP) {
        return -1;
    }
    if (manifest->image_addr != SECBOOT_N32_APP_IMAGE_ADDR ||
        manifest->image_size == 0u ||
        manifest->image_size > SECBOOT_N32_APP_IMAGE_SIZE) {
        return -1;
    }
    if (manifest->entry_addr < SECBOOT_N32_APP_IMAGE_ADDR ||
        manifest->entry_addr >= (SECBOOT_N32_APP_IMAGE_ADDR + manifest->image_size)) {
        return -1;
    }
    return 0;
}

static void make_header(uint8_t type, uint16_t seq, uint32_t session_id,
                        uint32_t offset, uint16_t length, uint8_t *header)
{
    memset(header, 0, SECBOOT_N32_UART_V1_HEADER_SIZE);
    header[0] = 'S';
    header[1] = 'B';
    header[2] = SECBOOT_N32_UART_V1_VERSION;
    header[3] = type;
    le16_write(&header[6], seq);
    le32_write(&header[8], session_id);
    le32_write(&header[12], offset);
    le16_write(&header[16], length);
    le16_write(&header[18], crc16_ccitt(header, SECBOOT_N32_UART_V1_HEADER_SIZE));
}

static void send_frame(uint8_t type, uint16_t seq, uint32_t offset,
                       const uint8_t *payload, uint16_t length)
{
    uint8_t header[SECBOOT_N32_UART_V1_HEADER_SIZE];
    uint8_t crc_buf[4];
    uint32_t payload_crc = crc32_update(0u, payload, length);

    make_header(type, seq, s_session_id, offset, length, header);
    (void)n32_uart5_secboot_write(header, sizeof(header), 1000u);
    if (length != 0u) {
        (void)n32_uart5_secboot_write(payload, length, 1000u);
    }
    le32_write(crc_buf, payload_crc);
    (void)n32_uart5_secboot_write(crc_buf, sizeof(crc_buf), 1000u);
}

static void send_status(uint8_t type, uint16_t seq, uint16_t reason,
                        uint32_t next_offset, uint32_t detail)
{
    uint8_t ack[SECBOOT_N32_UART_V1_ACK_SIZE];

    le16_write(&ack[0], seq);
    le16_write(&ack[2], reason);
    le32_write(&ack[4], next_offset);
    le32_write(&ack[8], detail);
    send_frame(type, seq, next_offset, ack, sizeof(ack));
}

static void send_ack(uint16_t seq, uint32_t detail)
{
    send_status(SECBOOT_N32_UART_PKT_ACK, seq, SECBOOT_N32_UART_REASON_OK,
                s_expected_offset, detail);
}

static void send_nack(uint16_t seq, uint16_t reason, uint32_t detail)
{
    send_status(SECBOOT_N32_UART_PKT_NACK, seq, reason, s_expected_offset, detail);
}

static void send_caps(uint16_t seq)
{
    uint8_t caps[24];
    const xy_secboot_suite_t *suite = xy_secboot_get_suite();

    caps[0] = SECBOOT_N32_UART_V1_VERSION;
    caps[1] = 0u;
    le16_write(&caps[2], SECBOOT_N32_UART_V1_MAX_PAYLOAD);
    le32_write(&caps[4], SECBOOT_N32_PRODUCT_ID);
    le32_write(&caps[8], suite ? suite->suite_id : 0u);
    le32_write(&caps[12], SECBOOT_N32_APP_IMAGE_ADDR);
    le32_write(&caps[16], SECBOOT_N32_APP_IMAGE_SIZE);
    le32_write(&caps[20], SECBOOT_N32_APP_MANIFEST_ADDR);
    send_frame(SECBOOT_N32_UART_PKT_CAPS, seq, s_expected_offset, caps, sizeof(caps));
}

static int read_frame(secboot_uart_frame_t *frame, uint8_t first,
                      uint8_t *payload, uint32_t timeout_ms)
{
    uint8_t header[SECBOOT_N32_UART_V1_HEADER_SIZE];
    uint8_t crc_buf[4];
    uint16_t header_crc;

    header[0] = first;
    if (n32_uart5_secboot_read(&header[1], 1u, timeout_ms) != 1) {
        return -1;
    }
    if (header[0] != 'S' || header[1] != 'B') {
        return -1;
    }
    if (n32_uart5_secboot_read(&header[2], sizeof(header) - 2u, timeout_ms) !=
        (int)(sizeof(header) - 2u)) {
        return -1;
    }

    header_crc = le16_read(&header[18]);
    header[18] = 0u;
    header[19] = 0u;
    if (crc16_ccitt(header, sizeof(header)) != header_crc) {
        return -2;
    }

    frame->version = header[2];
    frame->type = header[3];
    frame->flags = header[4];
    frame->seq = le16_read(&header[6]);
    frame->session_id = le32_read(&header[8]);
    frame->offset = le32_read(&header[12]);
    frame->length = le16_read(&header[16]);

    if (frame->version != SECBOOT_N32_UART_V1_VERSION ||
        frame->length > SECBOOT_N32_UART_V1_MAX_PAYLOAD) {
        return -3;
    }
    if (n32_uart5_secboot_read(payload, frame->length, timeout_ms) != frame->length) {
        return -1;
    }
    if (n32_uart5_secboot_read(crc_buf, sizeof(crc_buf), timeout_ms) !=
        (int)sizeof(crc_buf)) {
        return -1;
    }
    if (crc32_update(0u, payload, frame->length) != le32_read(crc_buf)) {
        return -4;
    }

    return 1;
}

static void handle_hello(const secboot_uart_frame_t *frame)
{
    s_session_id = frame->session_id;
    send_caps(frame->seq);
}

static void handle_manifest(const secboot_uart_frame_t *frame,
                            const uint8_t *payload)
{
    if (frame->length != sizeof(s_manifest)) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_LENGTH, frame->length);
        return;
    }

    memcpy(&s_manifest, payload, sizeof(s_manifest));
    if (manifest_basic_check(&s_manifest) != 0) {
        memset(&s_manifest, 0, sizeof(s_manifest));
        s_manifest_valid = 0u;
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_MANIFEST, 0u);
        return;
    }

    if (secboot_flash_erase(SECBOOT_N32_APP_IMAGE_ADDR,
                            SECBOOT_N32_APP_IMAGE_SIZE) != 0) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_FLASH_ERASE_FAILED, 0u);
        return;
    }

    s_manifest_valid = 1u;
    s_download_active = 1u;
    s_expected_seq = (uint16_t)(frame->seq + 1u);
    s_expected_offset = 0u;
    s_session_id = frame->session_id;
    send_ack(frame->seq, s_manifest.image_size);
}

static void handle_data(const secboot_uart_frame_t *frame, const uint8_t *payload)
{
    uint8_t verify[SECBOOT_N32_UART_V1_MAX_PAYLOAD];

    if (!s_download_active || !s_manifest_valid) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_MANIFEST, 0u);
        return;
    }
    if (frame->seq == (uint16_t)(s_expected_seq - 1u) &&
        frame->offset + frame->length == s_expected_offset) {
        send_ack(frame->seq, 0u);
        return;
    }
    if (frame->seq != s_expected_seq) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_SEQ, s_expected_seq);
        return;
    }
    if (frame->offset != s_expected_offset) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_OFFSET, frame->offset);
        return;
    }
    if (frame->length == 0u || (frame->length & 0x3u) != 0u ||
        frame->offset + frame->length > s_manifest.image_size) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_LENGTH, frame->length);
        return;
    }

    if (secboot_flash_write(SECBOOT_N32_APP_IMAGE_ADDR + frame->offset,
                            payload,
                            frame->length) != 0) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_FLASH_WRITE_FAILED,
                  frame->offset);
        return;
    }

    if (secboot_flash_read(SECBOOT_N32_APP_IMAGE_ADDR + frame->offset,
                           verify,
                           frame->length) != 0 ||
        memcmp(verify, payload, frame->length) != 0) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_FLASH_VERIFY_FAILED,
                  frame->offset);
        return;
    }

    s_expected_offset += frame->length;
    s_expected_seq++;
    send_ack(frame->seq, 0u);
}

static void handle_end(const secboot_uart_frame_t *frame)
{
    int rc;

    if (!s_download_active || !s_manifest_valid ||
        s_expected_offset != s_manifest.image_size) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_BAD_OFFSET,
                  s_manifest.image_size);
        return;
    }

    rc = xy_secboot_single_verify_active(&s_secboot_ctx, &s_manifest);
    if (rc != XY_SECBOOT_OK) {
        xy_log_w("SecBoot-N32 verify failed rc=%d", rc);
        send_status(SECBOOT_N32_UART_PKT_ERROR,
                    frame->seq,
                    SECBOOT_N32_UART_REASON_IMAGE_VERIFY_FAILED,
                    s_expected_offset,
                    (uint32_t)rc);
        return;
    }

    if (secboot_flash_erase(SECBOOT_N32_APP_MANIFEST_ADDR,
                            SECBOOT_N32_FLASH_PAGE_SIZE) != 0 ||
        secboot_flash_write(SECBOOT_N32_APP_MANIFEST_ADDR,
                            (const uint8_t *)&s_manifest,
                            sizeof(s_manifest)) != 0) {
        send_nack(frame->seq, SECBOOT_N32_UART_REASON_FLASH_WRITE_FAILED,
                  SECBOOT_N32_APP_MANIFEST_ADDR);
        return;
    }

    s_download_active = 0u;
    send_ack(frame->seq, s_manifest.entry_addr);
}

static void handle_abort(const secboot_uart_frame_t *frame)
{
    s_download_active = 0u;
    s_manifest_valid = 0u;
    s_expected_seq = 0u;
    s_expected_offset = 0u;
    memset(&s_manifest, 0, sizeof(s_manifest));
    send_ack(frame->seq, 0u);
}

void secboot_n32_v1_init(void)
{
    const xy_secboot_suite_t *suite;
    int rc;

    xy_secboot_set_crypto_ops(&s_secboot_crypto_ops);
    s_secboot_ctx.partition_table = &s_secboot_n32_table;
    s_secboot_ctx.crypto = &s_secboot_crypto_ops;
    s_secboot_ctx.port = &s_secboot_port;
    s_secboot_ctx.product_id = SECBOOT_N32_PRODUCT_ID;
    s_secboot_ctx.recovery_timeout_ms = 10000u;
    s_secboot_ctx.packet_timeout_ms = 1000u;
    rc = xy_secboot_single_init(&s_secboot_ctx);

    suite = xy_secboot_get_suite();
    xy_log_i("SecBoot-N32 V1 init product=%x suite=%u rc=%d",
             (unsigned int)SECBOOT_N32_PRODUCT_ID,
             suite ? (unsigned int)suite->suite_id : 0u,
             rc);
    xy_log_i("SecBoot-N32 partition count=%u",
             (unsigned int)s_secboot_n32_table.count);
}

void secboot_n32_v1_send_banner(void)
{
    static const char banner[] =
        "XY_SECBOOT_N32_V1 UART5 READY: send SBv1 HELLO or '?'\r\n";
    (void)n32_uart5_secboot_write((const uint8_t *)banner,
                                  sizeof(banner) - 1u,
                                  1000u);
}

void secboot_n32_v1_print_layout(void)
{
    xy_log_i("SecBoot-N32 boot=%x+%x state=%x rollback=%x manifest=%x app=%x+%x",
             (unsigned int)SECBOOT_N32_BOOT_BASE_ADDR,
             (unsigned int)SECBOOT_N32_BOOT_TOTAL_SIZE,
             (unsigned int)SECBOOT_N32_STATE_BASE_ADDR,
             (unsigned int)SECBOOT_N32_ROLLBACK_BASE_ADDR,
             (unsigned int)SECBOOT_N32_APP_MANIFEST_ADDR,
             (unsigned int)SECBOOT_N32_APP_IMAGE_ADDR,
             (unsigned int)SECBOOT_N32_APP_IMAGE_SIZE);
}

void secboot_n32_v1_poll(void)
{
    secboot_uart_frame_t frame;
    uint8_t first;
    int rc;

    if (n32_uart5_secboot_read(&first, 1u, 0u) != 1) {
        return;
    }
    if (first != 'S') {
        if (first == '?') {
            secboot_n32_v1_send_banner();
        } else if (first == 'p') {
            secboot_n32_v1_print_layout();
        }
        return;
    }

    rc = read_frame(&frame, first, s_payload, 1000u);
    if (rc < 0) {
        send_status(SECBOOT_N32_UART_PKT_NACK,
                    s_expected_seq,
                    rc == -2 ? SECBOOT_N32_UART_REASON_BAD_HEADER_CRC :
                    rc == -4 ? SECBOOT_N32_UART_REASON_BAD_PAYLOAD_CRC :
                               SECBOOT_N32_UART_REASON_BAD_LENGTH,
                    s_expected_offset,
                    (uint32_t)(-rc));
        return;
    }

    switch (frame.type) {
    case SECBOOT_N32_UART_PKT_HELLO:
        handle_hello(&frame);
        break;
    case SECBOOT_N32_UART_PKT_MANIFEST:
        handle_manifest(&frame, s_payload);
        break;
    case SECBOOT_N32_UART_PKT_DATA:
        handle_data(&frame, s_payload);
        break;
    case SECBOOT_N32_UART_PKT_END:
        handle_end(&frame);
        break;
    case SECBOOT_N32_UART_PKT_ABORT:
        handle_abort(&frame);
        break;
    case SECBOOT_N32_UART_PKT_RESET:
        send_ack(frame.seq, 0u);
        NVIC_SystemReset();
        break;
    default:
        send_nack(frame.seq, SECBOOT_N32_UART_REASON_BAD_LENGTH, frame.type);
        break;
    }
}
