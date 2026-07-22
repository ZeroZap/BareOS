/**
 * @file xy_secboot_bootcfg.c
 * @brief Redundant A/B boot configuration storage
 */

#include "xy_secboot_bootcfg.h"

static uint32_t bootcfg_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;

    if (!data && len != 0u) {
        return 0u;
    }

    for (i = 0u; i < len; i++) {
        uint32_t byte = data[i];
        uint8_t bit;
        crc ^= byte;
        for (bit = 0u; bit < 8u; bit++) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }

    return ~crc;
}

static int ctx_valid(const xy_secboot_bootcfg_ctx_t *ctx)
{
    if (!ctx || !ctx->port || !ctx->port->flash_read ||
        !ctx->port->flash_erase || !ctx->port->flash_write) {
        return 0;
    }
    if (ctx->copy_size <= sizeof(xy_secboot_bootcfg_header_t)) {
        return 0;
    }
    return 1;
}

static uint32_t header_crc(const xy_secboot_bootcfg_header_t *hdr)
{
    xy_secboot_bootcfg_header_t tmp;

    if (!hdr) {
        return 0u;
    }

    tmp = *hdr;
    tmp.header_crc32 = 0u;
    return bootcfg_crc32((const uint8_t *)&tmp, sizeof(tmp));
}

static int read_copy(const xy_secboot_bootcfg_ctx_t *ctx,
                     uint32_t addr,
                     xy_secboot_bootcfg_header_t *hdr,
                     uint8_t *payload,
                     size_t payload_cap)
{
    if (ctx->port->flash_read(addr, (uint8_t *)hdr, sizeof(*hdr)) != 0) {
        return -1;
    }
    if (hdr->magic != XY_SECBOOT_BOOTCFG_MAGIC ||
        hdr->format_version != XY_SECBOOT_BOOTCFG_FORMAT_VERSION ||
        hdr->header_size != sizeof(*hdr) ||
        hdr->seq != ~hdr->seq_inv ||
        hdr->payload_len > (ctx->copy_size - sizeof(*hdr)) ||
        hdr->payload_len > payload_cap ||
        hdr->header_crc32 != header_crc(hdr)) {
        return -1;
    }

    if (ctx->port->flash_read(addr + sizeof(*hdr), payload, hdr->payload_len) != 0) {
        return -1;
    }
    if (bootcfg_crc32(payload, hdr->payload_len) != hdr->payload_crc32) {
        return -1;
    }

    return 0;
}

static int read_header_valid(const xy_secboot_bootcfg_ctx_t *ctx,
                             uint32_t addr,
                             xy_secboot_bootcfg_header_t *hdr)
{
    if (ctx->port->flash_read(addr, (uint8_t *)hdr, sizeof(*hdr)) != 0) {
        return -1;
    }
    if (hdr->magic != XY_SECBOOT_BOOTCFG_MAGIC ||
        hdr->format_version != XY_SECBOOT_BOOTCFG_FORMAT_VERSION ||
        hdr->header_size != sizeof(*hdr) ||
        hdr->seq != ~hdr->seq_inv ||
        hdr->payload_len > (ctx->copy_size - sizeof(*hdr)) ||
        hdr->header_crc32 != header_crc(hdr)) {
        return -1;
    }
    return 0;
}

static int copy_b_newer(uint32_t seq_a, uint32_t seq_b)
{
    return (int32_t)(seq_b - seq_a) > 0;
}

static void make_header(xy_secboot_bootcfg_header_t *hdr,
                        uint32_t seq,
                        const uint8_t *payload,
                        size_t payload_len)
{
    hdr->magic = XY_SECBOOT_BOOTCFG_MAGIC;
    hdr->format_version = XY_SECBOOT_BOOTCFG_FORMAT_VERSION;
    hdr->header_size = sizeof(*hdr);
    hdr->seq = seq;
    hdr->seq_inv = ~seq;
    hdr->payload_len = (uint32_t)payload_len;
    hdr->payload_crc32 = bootcfg_crc32(payload, payload_len);
    hdr->header_crc32 = 0u;
    hdr->header_crc32 = header_crc(hdr);
}

int xy_secboot_bootcfg_load(const xy_secboot_bootcfg_ctx_t *ctx,
                            uint8_t *payload,
                            size_t payload_cap,
                            size_t *payload_len,
                            xy_secboot_bootcfg_info_t *info)
{
    xy_secboot_bootcfg_header_t hdr_a;
    xy_secboot_bootcfg_header_t hdr_b;
    int valid_a;
    int valid_b;
    uint8_t *tmp;

    if (!ctx_valid(ctx) || !payload || !payload_len) {
        return -1;
    }

    valid_a = read_copy(ctx, ctx->copy_a_addr, &hdr_a, payload, payload_cap) == 0;
    if (valid_a) {
        *payload_len = hdr_a.payload_len;
    }

    tmp = payload;
    valid_b = read_copy(ctx, ctx->copy_b_addr, &hdr_b, tmp, payload_cap) == 0;
    if (valid_b && (!valid_a || copy_b_newer(hdr_a.seq, hdr_b.seq))) {
        *payload_len = hdr_b.payload_len;
        if (info) {
            info->active_copy = XY_SECBOOT_BOOTCFG_COPY_B;
            info->seq = hdr_b.seq;
            info->payload_len = hdr_b.payload_len;
        }
        return 0;
    }

    if (valid_a) {
        if (read_copy(ctx, ctx->copy_a_addr, &hdr_a, payload, payload_cap) != 0) {
            return -1;
        }
        *payload_len = hdr_a.payload_len;
        if (info) {
            info->active_copy = XY_SECBOOT_BOOTCFG_COPY_A;
            info->seq = hdr_a.seq;
            info->payload_len = hdr_a.payload_len;
        }
        return 0;
    }

    if (valid_b) {
        *payload_len = hdr_b.payload_len;
        if (info) {
            info->active_copy = XY_SECBOOT_BOOTCFG_COPY_B;
            info->seq = hdr_b.seq;
            info->payload_len = hdr_b.payload_len;
        }
        return 0;
    }

    if (info) {
        info->active_copy = XY_SECBOOT_BOOTCFG_COPY_NONE;
        info->seq = 0u;
        info->payload_len = 0u;
    }
    return -1;
}

int xy_secboot_bootcfg_save(const xy_secboot_bootcfg_ctx_t *ctx,
                            const uint8_t *payload,
                            size_t payload_len,
                            xy_secboot_bootcfg_info_t *info)
{
    xy_secboot_bootcfg_header_t hdr;
    xy_secboot_bootcfg_header_t hdr_a;
    xy_secboot_bootcfg_header_t hdr_b;
    xy_secboot_bootcfg_copy_t active_copy = XY_SECBOOT_BOOTCFG_COPY_NONE;
    uint32_t dst;
    uint32_t seq;
    int valid_a;
    int valid_b;

    if (!ctx_valid(ctx) || (!payload && payload_len != 0u) ||
        payload_len > (ctx->copy_size - sizeof(hdr)) ||
        (payload_len & 0x3u) != 0u) {
        return -1;
    }

    valid_a = read_header_valid(ctx, ctx->copy_a_addr, &hdr_a) == 0;
    valid_b = read_header_valid(ctx, ctx->copy_b_addr, &hdr_b) == 0;
    if (valid_a && (!valid_b || !copy_b_newer(hdr_a.seq, hdr_b.seq))) {
        active_copy = XY_SECBOOT_BOOTCFG_COPY_A;
        seq = hdr_a.seq + 1u;
    } else if (valid_b) {
        active_copy = XY_SECBOOT_BOOTCFG_COPY_B;
        seq = hdr_b.seq + 1u;
    } else {
        seq = 1u;
    }

    dst = (active_copy == XY_SECBOOT_BOOTCFG_COPY_A) ?
          ctx->copy_b_addr : ctx->copy_a_addr;
    make_header(&hdr, seq, payload, payload_len);

    if (ctx->port->flash_erase(dst, ctx->copy_size) != 0) {
        return -1;
    }
    if (payload_len != 0u &&
        ctx->port->flash_write(dst + sizeof(hdr), payload, payload_len) != 0) {
        return -1;
    }
    if (ctx->port->flash_write(dst, (const uint8_t *)&hdr, sizeof(hdr)) != 0) {
        return -1;
    }

    if (info) {
        info->active_copy = (dst == ctx->copy_a_addr) ?
                            XY_SECBOOT_BOOTCFG_COPY_A : XY_SECBOOT_BOOTCFG_COPY_B;
        info->seq = seq;
        info->payload_len = (uint32_t)payload_len;
    }
    return 0;
}
