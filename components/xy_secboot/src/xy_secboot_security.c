/**
 * @file xy_secboot_security.c
 * @brief Secure boot hardware security settings abstraction
 */

#include "xy_secboot_security.h"

static int security_config_valid(const xy_secboot_security_config_t *config)
{
    if (!config) {
        return 0;
    }
    if (config->rdp_level > XY_SECBOOT_RDP_LEVEL_2) {
        return 0;
    }
    if (config->wrp_range_count != 0u && !config->wrp_ranges) {
        return 0;
    }
    if (config->rdp_level == XY_SECBOOT_RDP_LEVEL_2 &&
        (config->flags & XY_SECBOOT_SECURITY_FLAG_ALLOW_RDP2) == 0u) {
        return 0;
    }
    return 1;
}

int xy_secboot_security_get_status(const xy_secboot_security_ops_t *ops,
                                   xy_secboot_security_status_t *status)
{
    if (!ops || !ops->get_status || !status) {
        return -1;
    }
    return ops->get_status(status);
}

int xy_secboot_security_apply(const xy_secboot_security_ops_t *ops,
                              const xy_secboot_security_config_t *config)
{
    if (!ops || !ops->apply || !security_config_valid(config)) {
        return -1;
    }
    return ops->apply(config);
}
