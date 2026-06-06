/**
 * @file xy_secboot_config.h
 * @brief Secure boot policy selection
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_CONFIG_H
#define XY_SECBOOT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Security Suite Selection ==================== */

#define XY_SECBOOT_SUITE_MINIMAL       1u
#define XY_SECBOOT_SUITE_MODERN        2u
#define XY_SECBOOT_SUITE_MARKET        3u
#define XY_SECBOOT_SUITE_GM            4u
#define XY_SECBOOT_SUITE_PLB_GM        5u
#define XY_SECBOOT_SUITE_MODEL_SECURE  6u
#define XY_SECBOOT_SUITE_CUSTOM        255u

#ifndef XY_SECBOOT_SUITE
#define XY_SECBOOT_SUITE XY_SECBOOT_SUITE_PLB_GM
#endif

/* ==================== Optional Security Functions ==================== */

#ifndef XY_SECBOOT_ENABLE_MODEL_PROTECT
#define XY_SECBOOT_ENABLE_MODEL_PROTECT 0u
#endif

#ifndef XY_SECBOOT_ENABLE_TRANSPORT_AEAD
#define XY_SECBOOT_ENABLE_TRANSPORT_AEAD 0u
#endif

#ifndef XY_SECBOOT_ENABLE_OUTPUT_AUTH
#define XY_SECBOOT_ENABLE_OUTPUT_AUTH 0u
#endif

#ifndef XY_SECBOOT_ENABLE_CANARY
#define XY_SECBOOT_ENABLE_CANARY 1u
#endif

#ifndef XY_SECBOOT_ENABLE_DOUBLE_RUN
#define XY_SECBOOT_ENABLE_DOUBLE_RUN 0u
#endif

#ifndef XY_SECBOOT_ENABLE_KEY_ROTATION
#define XY_SECBOOT_ENABLE_KEY_ROTATION 1u
#endif

/* ==================== Resource Limits ==================== */

#ifndef XY_SECBOOT_READ_BUFFER_SIZE
#define XY_SECBOOT_READ_BUFFER_SIZE 512u
#endif

#ifndef XY_SECBOOT_MAX_IMAGE_SIZE
#define XY_SECBOOT_MAX_IMAGE_SIZE 0u
#endif

#ifndef XY_SECBOOT_MAX_MANIFEST_SIZE
#define XY_SECBOOT_MAX_MANIFEST_SIZE 512u
#endif

/* ==================== Partition Defaults ==================== */

#ifndef XY_SECBOOT_PARTITION_ALIGNMENT
#define XY_SECBOOT_PARTITION_ALIGNMENT 4096u
#endif

#ifndef XY_SECBOOT_ENABLE_AB_SLOT
#define XY_SECBOOT_ENABLE_AB_SLOT 1u
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_CONFIG_H */
