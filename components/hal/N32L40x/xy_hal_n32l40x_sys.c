#include "xy_hal_n32l40x_internal.h"

void xy_hal_system_reset(void)
{
    NVIC_SystemReset();
}

void xy_hal_watchdog_kick(void)
{
    IWDG_ReloadKey();
}

int xy_hal_get_chip_id(uint8_t *buf, size_t len)
{
    size_t n;
    const uint8_t *uid = (const uint8_t *)UID_BASE;
    if (!buf && len) return XY_HAL_INVALID_PARAM;
    n = len < UID_LENGTH ? len : UID_LENGTH;
    memcpy(buf, uid, n);
    return (int)n;
}
