#include "xy_hal.h"

#define XY_HAL_I2C_RECOVERY_DEFAULT_PULSES   16u
#define XY_HAL_I2C_RECOVERY_DEFAULT_LOW_US   10u
#define XY_HAL_I2C_RECOVERY_DEFAULT_HIGH_US  10u
#define XY_HAL_I2C_RECOVERY_DEFAULT_SCL_US   100u

static uint8_t recovery_pulses(const xy_hal_i2c_recovery_config_t *config)
{
    if (!config->max_pulses) return XY_HAL_I2C_RECOVERY_DEFAULT_PULSES;
    return config->max_pulses;
}

static uint16_t low_us(const xy_hal_i2c_recovery_config_t *config)
{
    return config->pulse_low_us ? config->pulse_low_us : XY_HAL_I2C_RECOVERY_DEFAULT_LOW_US;
}

static uint16_t high_us(const xy_hal_i2c_recovery_config_t *config)
{
    return config->pulse_high_us ? config->pulse_high_us : XY_HAL_I2C_RECOVERY_DEFAULT_HIGH_US;
}

static uint16_t scl_wait_us(const xy_hal_i2c_recovery_config_t *config)
{
    return config->scl_wait_us ? config->scl_wait_us : XY_HAL_I2C_RECOVERY_DEFAULT_SCL_US;
}

static int wait_scl_high(const xy_hal_i2c_recovery_config_t *config)
{
    const xy_hal_i2c_recovery_ops_t *ops = config->ops;
    uint16_t i;
    for (i = 0u; i < scl_wait_us(config); i++) {
        if (ops->read_scl(config->ctx)) return 0;
        ops->delay_us(config->ctx, 1u);
    }
    return -1;
}

static int generate_stop(const xy_hal_i2c_recovery_config_t *config)
{
    const xy_hal_i2c_recovery_ops_t *ops = config->ops;
    ops->drive_sda_low(config->ctx);
    ops->delay_us(config->ctx, low_us(config));
    ops->release_scl(config->ctx);
    if (wait_scl_high(config) != 0) return -1;
    ops->delay_us(config->ctx, high_us(config));
    ops->release_sda(config->ctx);
    ops->delay_us(config->ctx, high_us(config));
    return ops->read_sda(config->ctx) ? 0 : -1;
}

static bool valid_ops(const xy_hal_i2c_recovery_ops_t *ops)
{
    return ops && ops->prepare && ops->unprepare && ops->gpio_od_init &&
           ops->release_scl && ops->drive_scl_low &&
           ops->release_sda && ops->drive_sda_low &&
           ops->read_scl && ops->read_sda && ops->delay_us;
}

int xy_hal_i2c_recover_bus(const xy_hal_i2c_recovery_config_t *config,
                           xy_hal_i2c_stats_t *stats)
{
    const xy_hal_i2c_recovery_ops_t *ops;
    int ret;
    uint8_t i;

    if (!config || !valid_ops(config->ops)) return XY_HAL_I2C_RECOVER_INVALID_ARG;
    ops = config->ops;
    if (stats) stats->recover_attempt++;

    ret = ops->prepare(config->ctx);
    if (ret != 0) {
        if (stats) stats->last_recover = XY_HAL_I2C_RECOVER_PREPARE_FAIL;
        return XY_HAL_I2C_RECOVER_PREPARE_FAIL;
    }

    ret = ops->gpio_od_init(config->ctx);
    if (ret != 0) {
        (void)ops->unprepare(config->ctx);
        if (stats) stats->last_recover = XY_HAL_I2C_RECOVER_GPIO_FAIL;
        return XY_HAL_I2C_RECOVER_GPIO_FAIL;
    }

    ops->release_scl(config->ctx);
    ops->release_sda(config->ctx);
    ops->delay_us(config->ctx, high_us(config));
    if (stats) {
        stats->before.scl_high = ops->read_scl(config->ctx);
        stats->before.sda_high = ops->read_sda(config->ctx);
    }

    if (wait_scl_high(config) != 0) {
        ret = XY_HAL_I2C_RECOVER_SCL_STUCK;
        if (stats) stats->recover_fail_scl++;
        goto fallback;
    }

    for (i = 0u; !ops->read_sda(config->ctx) && i < recovery_pulses(config); i++) {
        ops->drive_scl_low(config->ctx);
        ops->delay_us(config->ctx, low_us(config));
        ops->release_scl(config->ctx);
        if (wait_scl_high(config) != 0) {
            ret = XY_HAL_I2C_RECOVER_SCL_STUCK;
            if (stats) stats->recover_fail_scl++;
            goto fallback;
        }
        ops->delay_us(config->ctx, high_us(config));
    }
    if (stats) stats->last_pulses = i;

    if (!ops->read_sda(config->ctx)) {
        ret = XY_HAL_I2C_RECOVER_SDA_STUCK;
        if (stats) stats->recover_fail_sda++;
        goto fallback;
    }

    if (generate_stop(config) != 0) {
        ret = XY_HAL_I2C_RECOVER_STOP_FAIL;
        goto fallback;
    }

    ret = ops->unprepare(config->ctx);
    if (ret != 0) {
        if (stats) stats->last_recover = XY_HAL_I2C_RECOVER_UNPREPARE_FAIL;
        return XY_HAL_I2C_RECOVER_UNPREPARE_FAIL;
    }
    if (stats) {
        stats->after.scl_high = ops->read_scl(config->ctx);
        stats->after.sda_high = ops->read_sda(config->ctx);
        stats->recover_success++;
        stats->last_recover = XY_HAL_I2C_RECOVER_OK;
    }
    return XY_HAL_I2C_RECOVER_OK;

fallback:
    if (ops->reset_target) (void)ops->reset_target(config->ctx);
    (void)ops->unprepare(config->ctx);
    if (stats) {
        stats->after.scl_high = ops->read_scl(config->ctx);
        stats->after.sda_high = ops->read_sda(config->ctx);
        stats->last_recover = (xy_hal_i2c_recover_result_t)ret;
    }
    return ret;
}
