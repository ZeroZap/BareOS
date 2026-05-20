/**
 * @file xy_io.c
 * @brief IO abstraction implementation.
 */

#include "xy_io.h"

static bool s_led[XY_LED_COUNT];

void xy_io_init(void)
{
    for (int i = 0; i < (int)XY_LED_COUNT; i++) {
        s_led[i] = false;
        xy_io_led_hw_set((xy_led_id_t)i, false);
    }
}

void xy_io_led_set(xy_led_id_t id, bool on)
{
    if (id >= XY_LED_COUNT) return;
    if (s_led[id] == on) return;
    s_led[id] = on;
    xy_io_led_hw_set(id, on);
}

void xy_io_led_toggle(xy_led_id_t id)
{
    if (id >= XY_LED_COUNT) return;
    xy_io_led_set(id, !s_led[id]);
}

bool xy_io_led_get(xy_led_id_t id)
{
    if (id >= XY_LED_COUNT) return false;
    return s_led[id];
}

bool xy_io_btn_pressed(xy_btn_id_t id)
{
    if (id >= XY_BTN_COUNT) return false;
    return xy_io_btn_hw_read(id);
}

/* ── Weak BSP stubs ─────────────────────────────────────────────────── */

__attribute__((weak)) void xy_io_led_hw_set(xy_led_id_t id, bool on)
{
    (void)id; (void)on;
}

__attribute__((weak)) bool xy_io_btn_hw_read(xy_btn_id_t id)
{
    (void)id;
    return false;
}
