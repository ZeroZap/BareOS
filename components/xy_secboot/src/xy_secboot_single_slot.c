/**
 * @file xy_secboot_single_slot.c
 * @brief Single-slot secure boot framework
 */

#include "xy_secboot_config.h"
#include "xy_secboot_guard.h"
#include "xy_secboot_single_slot.h"

static int check_ctx(const xy_secboot_single_ctx_t *ctx)
{
    if (!ctx || !ctx->partition_table || !ctx->crypto || !ctx->port) {
        return -1;
    }

    if (!ctx->port->flash_read || !ctx->crypto->hash_init ||
        !ctx->crypto->hash_update || !ctx->crypto->hash_final) {
        return -1;
    }

    return 0;
}

static int ct_compare(const xy_secboot_crypto_ops_t *crypto,
                      const uint8_t *a, const uint8_t *b, size_t len)
{
    if (crypto && crypto->ct_compare) {
        return crypto->ct_compare(a, b, len);
    }

    return xy_secboot_ct_compare(a, b, len);
}

static void secure_zero(const xy_secboot_crypto_ops_t *crypto,
                        void *buf, size_t len)
{
    if (crypto && crypto->secure_zero) {
        crypto->secure_zero(buf, len);
        return;
    }

    xy_secboot_secure_zero(buf, len);
}

static size_t manifest_signed_len(void)
{
    return offsetof(xy_secboot_manifest_t, signature);
}

static int read_active_manifest(const xy_secboot_single_ctx_t *ctx,
                                xy_secboot_manifest_t *manifest)
{
    const xy_secboot_partition_t *part;

    part = xy_secboot_partition_find(ctx->partition_table,
                                     XY_SECBOOT_PART_APP_A_MANIFEST);
    if (!part) {
        return XY_SECBOOT_ERR_STATE;
    }

    if (part->size < sizeof(*manifest)) {
        return XY_SECBOOT_ERR_STATE;
    }

    if (ctx->port->flash_read(part->address,
                              (uint8_t *)manifest,
                              sizeof(*manifest)) != 0) {
        return XY_SECBOOT_ERR_HW;
    }

    return XY_SECBOOT_OK;
}

static int check_manifest_range(const xy_secboot_single_ctx_t *ctx,
                                const xy_secboot_manifest_t *manifest)
{
    const xy_secboot_partition_t *image_part;

    if (manifest->magic != XY_SECBOOT_MANIFEST_MAGIC) {
        return XY_SECBOOT_ERR_VERIFY;
    }

    if (manifest->header_version != XY_SECBOOT_MANIFEST_VERSION ||
        manifest->header_len < manifest_signed_len() ||
        manifest->header_len > sizeof(*manifest)) {
        return XY_SECBOOT_ERR_VERIFY;
    }

    if (manifest->product_id != ctx->product_id) {
        return XY_SECBOOT_ERR_VERIFY;
    }

    if (manifest->image_type != (uint32_t)XY_SECBOOT_IMAGE_APP) {
        return XY_SECBOOT_ERR_VERIFY;
    }

#if XY_SECBOOT_MAX_IMAGE_SIZE > 0u
    if (manifest->image_size > XY_SECBOOT_MAX_IMAGE_SIZE) {
        return XY_SECBOOT_ERR_VERIFY;
    }
#endif

    image_part = xy_secboot_partition_find(ctx->partition_table,
                                           XY_SECBOOT_PART_APP_A_IMAGE);
    if (!image_part) {
        return XY_SECBOOT_ERR_STATE;
    }

    if (xy_secboot_partition_check_range(image_part,
                                         manifest->image_addr,
                                         manifest->image_size) != 0) {
        return XY_SECBOOT_ERR_VERIFY;
    }

    if (xy_secboot_partition_check_range(image_part,
                                         manifest->entry_addr,
                                         4u) != 0) {
        return XY_SECBOOT_ERR_VERIFY;
    }

    return XY_SECBOOT_OK;
}

static int hash_image(const xy_secboot_single_ctx_t *ctx,
                      const xy_secboot_manifest_t *manifest,
                      const xy_secboot_suite_t *suite,
                      uint8_t *digest,
                      size_t *digest_len)
{
    uint8_t hash_ctx[XY_SECBOOT_HASH_CTX_MAX_SIZE];
    uint8_t buf[XY_SECBOOT_READ_BUFFER_SIZE];
    uint32_t remaining;
    uint32_t address;
    int rc;

    rc = ctx->crypto->hash_init(hash_ctx, suite->hash_alg);
    if (rc != 0) {
        secure_zero(ctx->crypto, hash_ctx, sizeof(hash_ctx));
        return XY_SECBOOT_ERR_HW;
    }

    remaining = manifest->image_size;
    address = manifest->image_addr;

    while (remaining > 0u) {
        size_t chunk = (remaining > (uint32_t)sizeof(buf)) ?
                       sizeof(buf) : (size_t)remaining;

        if (ctx->port->flash_read(address, buf, chunk) != 0) {
            secure_zero(ctx->crypto, hash_ctx, sizeof(hash_ctx));
            secure_zero(ctx->crypto, buf, sizeof(buf));
            return XY_SECBOOT_ERR_HW;
        }

        if (ctx->crypto->hash_update(hash_ctx, buf, chunk) != 0) {
            secure_zero(ctx->crypto, hash_ctx, sizeof(hash_ctx));
            secure_zero(ctx->crypto, buf, sizeof(buf));
            return XY_SECBOOT_ERR_HW;
        }

        address += (uint32_t)chunk;
        remaining -= (uint32_t)chunk;
    }

    *digest_len = XY_SECBOOT_HASH_MAX_SIZE;
    rc = ctx->crypto->hash_final(hash_ctx, digest, digest_len);

    secure_zero(ctx->crypto, hash_ctx, sizeof(hash_ctx));
    secure_zero(ctx->crypto, buf, sizeof(buf));

    return (rc == 0) ? XY_SECBOOT_OK : XY_SECBOOT_ERR_HW;
}

static int verify_signature_or_mac(const xy_secboot_single_ctx_t *ctx,
                                   const xy_secboot_manifest_t *manifest,
                                   const xy_secboot_suite_t *suite)
{
    uint8_t key[XY_SECBOOT_SIG_MAX_SIZE];
    size_t key_len = sizeof(key);
    int rc;

    if (suite->sig_alg != XY_SECBOOT_ALG_NONE) {
        if (!ctx->crypto->get_key || !ctx->crypto->verify) {
            return XY_SECBOOT_ERR_UNSUPPORTED;
        }

        rc = ctx->crypto->get_key(XY_SECBOOT_KEY_BOOT_PUBLIC,
                                  manifest->key_id,
                                  XY_SECBOOT_KEY_ID_MAX_SIZE,
                                  key,
                                  &key_len);
        if (rc != 0) {
            secure_zero(ctx->crypto, key, sizeof(key));
            return XY_SECBOOT_ERR_AUTH;
        }

        rc = ctx->crypto->verify(suite->sig_alg,
                                 key,
                                 key_len,
                                 (const uint8_t *)manifest,
                                 manifest_signed_len(),
                                 manifest->signature,
                                 suite->sig_size);
        secure_zero(ctx->crypto, key, sizeof(key));
        return (rc == 0) ? XY_SECBOOT_OK : XY_SECBOOT_ERR_AUTH;
    }

    if (suite->firmware_mac_alg != XY_SECBOOT_ALG_NONE) {
        uint8_t tag[XY_SECBOOT_MAC_MAX_SIZE];
        size_t tag_len = sizeof(tag);

        if (!ctx->crypto->get_key || !ctx->crypto->mac) {
            return XY_SECBOOT_ERR_UNSUPPORTED;
        }

        rc = ctx->crypto->get_key(XY_SECBOOT_KEY_BOOT_MAC,
                                  manifest->key_id,
                                  XY_SECBOOT_KEY_ID_MAX_SIZE,
                                  key,
                                  &key_len);
        if (rc != 0) {
            secure_zero(ctx->crypto, key, sizeof(key));
            return XY_SECBOOT_ERR_AUTH;
        }

        rc = ctx->crypto->mac(suite->firmware_mac_alg,
                              key,
                              key_len,
                              (const uint8_t *)manifest,
                              manifest_signed_len(),
                              tag,
                              &tag_len);
        secure_zero(ctx->crypto, key, sizeof(key));

        if (rc != 0 || tag_len != suite->mac_size) {
            secure_zero(ctx->crypto, tag, sizeof(tag));
            return XY_SECBOOT_ERR_AUTH;
        }

        rc = ct_compare(ctx->crypto, tag, manifest->signature, tag_len);
        secure_zero(ctx->crypto, tag, sizeof(tag));
        return (rc == 0) ? XY_SECBOOT_OK : XY_SECBOOT_ERR_AUTH;
    }

    return XY_SECBOOT_ERR_UNSUPPORTED;
}

int xy_secboot_single_init(xy_secboot_single_ctx_t *ctx)
{
    if (check_ctx(ctx) != 0) {
        return XY_SECBOOT_ERR_PARAM;
    }

    if (ctx->crypto->init && ctx->crypto->init() != 0) {
        return XY_SECBOOT_ERR_HW;
    }

    return XY_SECBOOT_OK;
}

int xy_secboot_single_verify_active(xy_secboot_single_ctx_t *ctx,
                                    xy_secboot_manifest_t *manifest)
{
    const xy_secboot_suite_t *suite;
    uint8_t digest[XY_SECBOOT_HASH_MAX_SIZE];
    size_t digest_len;
    uint32_t rollback_counter;
    int rc;

    if (check_ctx(ctx) != 0 || !manifest) {
        return XY_SECBOOT_ERR_PARAM;
    }

    suite = xy_secboot_get_suite();
    if (!suite || suite->hash_size > XY_SECBOOT_HASH_MAX_SIZE) {
        return XY_SECBOOT_ERR_STATE;
    }

    rc = check_manifest_range(ctx, manifest);
    if (rc != XY_SECBOOT_OK) {
        return rc;
    }

    rc = hash_image(ctx, manifest, suite, digest, &digest_len);
    if (rc != XY_SECBOOT_OK) {
        secure_zero(ctx->crypto, digest, sizeof(digest));
        return rc;
    }

    if (digest_len != suite->hash_size ||
        ct_compare(ctx->crypto, digest, manifest->image_hash, digest_len) != 0) {
        secure_zero(ctx->crypto, digest, sizeof(digest));
        return XY_SECBOOT_ERR_VERIFY;
    }
    secure_zero(ctx->crypto, digest, sizeof(digest));

    rc = verify_signature_or_mac(ctx, manifest, suite);
    if (rc != XY_SECBOOT_OK) {
        return rc;
    }

    if (suite->require_anti_rollback) {
        if (!ctx->crypto->rollback_read) {
            return XY_SECBOOT_ERR_UNSUPPORTED;
        }

        if (ctx->crypto->rollback_read(&rollback_counter) != 0) {
            return XY_SECBOOT_ERR_ROLLBACK;
        }

        if (manifest->security_counter < rollback_counter) {
            return XY_SECBOOT_ERR_ROLLBACK;
        }
    }

    if (suite->require_canary && ctx->crypto->canary_check &&
        ctx->crypto->canary_check() != 0) {
        return XY_SECBOOT_ERR_STATE;
    }

    return XY_SECBOOT_OK;
}

xy_secboot_boot_action_t xy_secboot_single_check_boot(
    xy_secboot_single_ctx_t *ctx,
    xy_secboot_manifest_t *manifest)
{
    int rc;

    if (check_ctx(ctx) != 0 || !manifest) {
        return XY_SECBOOT_BOOT_ACTION_ENTER_RECOVERY;
    }

    rc = read_active_manifest(ctx, manifest);
    if (rc != XY_SECBOOT_OK) {
        return XY_SECBOOT_BOOT_ACTION_ENTER_RECOVERY;
    }

    rc = xy_secboot_single_verify_active(ctx, manifest);

    if (rc == XY_SECBOOT_OK) {
        return XY_SECBOOT_BOOT_ACTION_JUMP_APP;
    }

    return XY_SECBOOT_BOOT_ACTION_ENTER_RECOVERY;
}

int xy_secboot_single_receive_uart(xy_secboot_single_ctx_t *ctx)
{
    if (check_ctx(ctx) != 0 || !ctx->port->uart_read ||
        !ctx->port->uart_write || !ctx->port->flash_write ||
        !ctx->port->flash_erase) {
        return XY_SECBOOT_ERR_PARAM;
    }

    return XY_SECBOOT_ERR_UNSUPPORTED;
}
