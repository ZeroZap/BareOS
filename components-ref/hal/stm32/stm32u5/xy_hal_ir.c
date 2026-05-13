/**
 * @file xy_hal_ir.c
 * @brief IR (Infrared) Hardware Abstraction Layer STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_ir.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* IR context structure */
typedef struct {
    void *timer;
    xy_hal_ir_callback_t callback;
    void *arg;
    uint8_t initialized;
    uint8_t tx_pin_port;
    uint8_t tx_pin_num;
} ir_ctx_t;

#define MAX_IR_INSTANCES 4
static ir_ctx_t g_ir_ctx[MAX_IR_INSTANCES];

static ir_ctx_t *find_ir_ctx(void *ir)
{
    for (size_t i = 0; i < MAX_IR_INSTANCES; i++) {
        if (g_ir_ctx[i].timer == ir) {
            return &g_ir_ctx[i];
        }
    }
    return NULL;
}

static ir_ctx_t *alloc_ir_ctx(void)
{
    for (size_t i = 0; i < MAX_IR_INSTANCES; i++) {
        if (g_ir_ctx[i].timer == NULL) {
            return &g_ir_ctx[i];
        }
    }
    return NULL;
}

xy_hal_error_t xy_hal_ir_init(void *ir, const xy_hal_ir_config_t *config)
{
    if (!ir || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ir_ctx_t *ctx = find_ir_ctx(ir);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_ir_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    ctx->timer       = ir;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    XY_UNUSED(config);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_ir_deinit(void *ir)
{
    if (!ir) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ir_ctx_t *ctx = find_ir_ctx(ir);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ctx->initialized = 0;
    ctx->timer       = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_ir_send(void *ir, const uint32_t *data, size_t len)
{
    if (!ir || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ir_ctx_t *ctx = find_ir_ctx(ir);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Send IR data using PWM modulation */
    /* Implementation depends on hardware setup */
    XY_UNUSED(len);

    for (size_t i = 0; i < len; i++) {
        uint32_t code = data[i];
        /* Send 38kHz modulated signal */
        for (int j = 0; j < 32; j++) {
            if (code & (1UL << j)) {
                /* Send '1' - longer pulse */
            } else {
                /* Send '0' - shorter pulse */
            }
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_ir_receive(void *ir, uint32_t *data, size_t len,
                                 uint32_t timeout)
{
    if (!ir || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ir_ctx_t *ctx = find_ir_ctx(ir);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    XY_UNUSED(timeout);
    XY_UNUSED(len);

    /* Receive IR data using input capture */
    /* Implementation depends on hardware setup */

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_ir_register_callback(void *ir, xy_hal_ir_callback_t callback,
                                           void *arg)
{
    if (!ir) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ir_ctx_t *ctx = find_ir_ctx(ir);
    if (ctx == NULL) {
        ctx = alloc_ir_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->timer = ir;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

#endif /* STM32U5 || STM32U5xx */
