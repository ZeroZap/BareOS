#include "xy_hal_n32l40x_internal.h"

typedef struct {
    I2C_Module *i2c;
    xy_hal_i2c_recovery_config_t recovery;
    xy_hal_i2c_stats_t stats;
} xy_hal_n32_i2c_state_t;

static xy_hal_n32_i2c_state_t g_i2c_state[2];

static xy_hal_n32_i2c_state_t *i2c_state(I2C_Module *i2c)
{
    if (i2c == I2C1) return &g_i2c_state[0];
    if (i2c == I2C2) return &g_i2c_state[1];
    return NULL;
}

static void set_stage(xy_hal_n32_i2c_state_t *state, xy_hal_i2c_stage_t stage)
{
    if (state) state->stats.last_stage = stage;
}

static bool has_bus_error(I2C_Module *i2c)
{
    return I2C_GetFlag(i2c, I2C_FLAG_ACKFAIL) != RESET ||
           I2C_GetFlag(i2c, I2C_FLAG_ARLOST) != RESET ||
           I2C_GetFlag(i2c, I2C_FLAG_BUSERR) != RESET ||
           I2C_GetFlag(i2c, I2C_FLAG_TIMOUT) != RESET ||
           I2C_GetFlag(i2c, I2C_FLAG_OVERRUN) != RESET;
}

static void clear_bus_error(I2C_Module *i2c)
{
    I2C_ClrFlag(i2c, I2C_FLAG_ACKFAIL);
    I2C_ClrFlag(i2c, I2C_FLAG_ARLOST);
    I2C_ClrFlag(i2c, I2C_FLAG_BUSERR);
    I2C_ClrFlag(i2c, I2C_FLAG_TIMOUT);
    I2C_ClrFlag(i2c, I2C_FLAG_OVERRUN);
}

static void abort_transfer(I2C_Module *i2c)
{
    I2C_ConfigAck(i2c, ENABLE);
    I2C_GenerateStop(i2c, ENABLE);
    clear_bus_error(i2c);
}

static int wait_event(I2C_Module *i2c, uint32_t event, uint32_t start,
                      uint32_t timeout_ms, xy_hal_n32_i2c_state_t *state,
                      xy_hal_i2c_stage_t stage)
{
    set_stage(state, stage);
    while (I2C_CheckEvent(i2c, event) == ERROR) {
        if (has_bus_error(i2c)) {
            abort_transfer(i2c);
            return XY_HAL_ERROR;
        }
        if (xy_hal_n32_expired(start, timeout_ms)) return XY_HAL_TIMEOUT;
    }
    return XY_HAL_OK;
}

static int wait_flag_clear(I2C_Module *i2c, uint32_t flag, uint32_t start,
                           uint32_t timeout_ms, xy_hal_n32_i2c_state_t *state,
                           xy_hal_i2c_stage_t stage)
{
    set_stage(state, stage);
    while (I2C_GetFlag(i2c, flag) != RESET) {
        if (has_bus_error(i2c)) {
            abort_transfer(i2c);
            return XY_HAL_ERROR;
        }
        if (xy_hal_n32_expired(start, timeout_ms)) return XY_HAL_BUSY;
    }
    return XY_HAL_OK;
}

static int recover_if_configured(xy_hal_n32_i2c_state_t *state, int ret)
{
    if (!state || !state->recovery.ops) return ret;
    if (ret != XY_HAL_TIMEOUT && ret != XY_HAL_BUSY) return ret;
    state->stats.last_stage = XY_HAL_I2C_STAGE_RECOVERY;
    return xy_hal_i2c_recover_bus(&state->recovery, &state->stats);
}

int xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    I2C_Module *i = (I2C_Module *)i2c;
    I2C_InitType init;
    if (!i || !config) return XY_HAL_INVALID_PARAM;
    if (config->mode == XY_HAL_IO_DMA && (!config->rx_dma || !config->tx_dma)) return XY_HAL_INVALID_PARAM;
    if (!i2c_state(i)) return XY_HAL_INVALID_PARAM;
    xy_hal_n32_enable_i2c_clock(i);
    I2C_InitStruct(&init);
    init.ClkSpeed = config->speed_hz ? config->speed_hz : 100000u;
    init.BusMode = I2C_BUSMODE_I2C;
    init.FmDutyCycle = I2C_FMDUTYCYCLE_2;
    init.OwnAddr1 = config->own_address;
    init.AckEnable = I2C_ACKEN;
    init.AddrMode = I2C_ADDR_MODE_7BIT;
    I2C_Init(i, &init);
    I2C_EnableDMA(i, config->mode == XY_HAL_IO_DMA ? ENABLE : DISABLE);
    I2C_Enable(i, ENABLE);
    i2c_state(i)->i2c = i;
    return XY_HAL_OK;
}

int xy_hal_i2c_deinit(void *i2c)
{
    if (!i2c) return XY_HAL_INVALID_PARAM;
    I2C_DeInit((I2C_Module *)i2c);
    return XY_HAL_OK;
}

static int i2c_transfer_once(I2C_Module *i, uint8_t dev_addr, const uint8_t *tx,
                             uint16_t tx_len, uint8_t *rx, uint16_t rx_len,
                             uint32_t timeout_ms, xy_hal_n32_i2c_state_t *state)
{
    uint32_t start = xy_hal_time_ms();
    int ret;

    ret = wait_flag_clear(i, I2C_FLAG_BUSY, start, timeout_ms, state, XY_HAL_I2C_STAGE_BUSY);
    if (ret != XY_HAL_OK) return ret;

    if (tx_len) {
        I2C_GenerateStart(i, ENABLE);
        ret = wait_event(i, I2C_EVT_MASTER_MODE_FLAG, start, timeout_ms, state, XY_HAL_I2C_STAGE_START);
        if (ret != XY_HAL_OK) return ret;
        I2C_SendAddr7bit(i, (uint8_t)(dev_addr << 1), I2C_DIRECTION_SEND);
        ret = wait_event(i, I2C_EVT_MASTER_TXMODE_FLAG, start, timeout_ms, state, XY_HAL_I2C_STAGE_ADDR);
        if (ret != XY_HAL_OK) return ret;
        while (tx_len--) {
            I2C_SendData(i, *tx++);
            ret = wait_event(i, I2C_EVT_MASTER_DATA_SENDED, start, timeout_ms, state, XY_HAL_I2C_STAGE_TX);
            if (ret != XY_HAL_OK) return ret;
        }
    }
    if (rx_len) {
        I2C_GenerateStart(i, ENABLE);
        ret = wait_event(i, I2C_EVT_MASTER_MODE_FLAG, start, timeout_ms, state, XY_HAL_I2C_STAGE_START);
        if (ret != XY_HAL_OK) return ret;
        I2C_SendAddr7bit(i, (uint8_t)(dev_addr << 1), I2C_DIRECTION_RECV);
        ret = wait_event(i, I2C_EVT_MASTER_RXMODE_FLAG, start, timeout_ms, state, XY_HAL_I2C_STAGE_ADDR);
        if (ret != XY_HAL_OK) return ret;
        while (rx_len--) {
            if (rx_len == 0u) I2C_ConfigAck(i, DISABLE);
            ret = wait_event(i, I2C_EVT_MASTER_DATA_RECVD_FLAG, start, timeout_ms, state, XY_HAL_I2C_STAGE_RX);
            if (ret != XY_HAL_OK) {
                I2C_ConfigAck(i, ENABLE);
                return ret;
            }
            *rx++ = I2C_RecvData(i);
        }
        I2C_ConfigAck(i, ENABLE);
    }
    set_stage(state, XY_HAL_I2C_STAGE_STOP);
    I2C_GenerateStop(i, ENABLE);
    clear_bus_error(i);
    set_stage(state, XY_HAL_I2C_STAGE_NONE);
    return XY_HAL_OK;
}

int xy_hal_i2c_transfer(void *i2c, uint8_t dev_addr, const uint8_t *tx,
                        uint16_t tx_len, uint8_t *rx, uint16_t rx_len,
                        uint32_t timeout_ms)
{
    I2C_Module *i = (I2C_Module *)i2c;
    xy_hal_n32_i2c_state_t *state = i2c_state(i);
    uint8_t retries;
    uint8_t attempt;
    int ret;

    if (!i || !state || (!tx && tx_len) || (!rx && rx_len)) return XY_HAL_INVALID_PARAM;

    state->stats.xfer_total++;
    state->stats.last_addr = dev_addr;
    retries = state->recovery.ops ? state->recovery.max_retries : 0u;

    for (attempt = 0u; attempt <= retries; attempt++) {
        ret = i2c_transfer_once(i, dev_addr, tx, tx_len, rx, rx_len, timeout_ms, state);
        if (ret == XY_HAL_OK) return XY_HAL_OK;

        abort_transfer(i);
        if (ret == XY_HAL_TIMEOUT) state->stats.timeout_count++;
        if (ret == XY_HAL_BUSY) state->stats.busy_count++;
        if (attempt == retries) return ret;
        if (recover_if_configured(state, ret) != XY_HAL_I2C_RECOVER_OK) return ret;
    }

    return XY_HAL_ERROR;
}

int xy_hal_i2c_read(void *i2c, uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    return xy_hal_i2c_transfer(i2c, dev_addr, &reg, 1u, data, len, 100u);
}

int xy_hal_i2c_write(void *i2c, uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t tmp[32];
    if (len + 1u > sizeof(tmp)) return XY_HAL_INVALID_PARAM;
    tmp[0] = reg;
    if (len) memcpy(&tmp[1], data, len);
    return xy_hal_i2c_transfer(i2c, dev_addr, tmp, (uint16_t)(len + 1u), NULL, 0u, 100u);
}

void xy_hal_i2c_irq_handler(void *i2c)
{
    (void)i2c;
}

int xy_hal_i2c_set_recovery(void *i2c, const xy_hal_i2c_recovery_config_t *config)
{
    xy_hal_n32_i2c_state_t *state = i2c_state((I2C_Module *)i2c);
    if (!state) return XY_HAL_INVALID_PARAM;
    if (!config) {
        memset(&state->recovery, 0, sizeof(state->recovery));
        return XY_HAL_OK;
    }
    state->recovery = *config;
    return XY_HAL_OK;
}

int xy_hal_i2c_get_stats(void *i2c, xy_hal_i2c_stats_t *stats)
{
    xy_hal_n32_i2c_state_t *state = i2c_state((I2C_Module *)i2c);
    if (!state || !stats) return XY_HAL_INVALID_PARAM;
    *stats = state->stats;
    return XY_HAL_OK;
}
