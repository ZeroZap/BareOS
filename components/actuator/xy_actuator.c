/**
 * @file xy_actuator.c
 * @brief Actuator core — device registry, control API, pattern player.
 */

#include "xy_actuator.h"
#include "xy_string.h"

/* ── Registry ────────────────────────────────────────────────────────── */

static xy_actuator_device_t *s_head;   /* intrusive singly-linked list */
static uint8_t               s_count;

xy_actuator_status_t xy_actuator_register(xy_actuator_device_t *dev)
{
    if (!dev || !dev->api) return XY_ACTUATOR_ERROR_INVALID_PARAM;

    dev->next = s_head;
    s_head    = dev;
    s_count++;
    return XY_ACTUATOR_OK;
}

/* ── Lifecycle ───────────────────────────────────────────────────────── */

xy_actuator_status_t xy_actuator_init(xy_actuator_device_t *dev)
{
    if (!dev || !dev->api) return XY_ACTUATOR_ERROR_INVALID_PARAM;
    if (dev->initialized)  return XY_ACTUATOR_OK;

    if (dev->api->init) {
        if (dev->api->init(dev) != 0) return XY_ACTUATOR_ERROR;
    }
    dev->initialized = true;
    return XY_ACTUATOR_OK;
}

xy_actuator_status_t xy_actuator_deinit(xy_actuator_device_t *dev)
{
    if (!dev || !dev->api) return XY_ACTUATOR_ERROR_INVALID_PARAM;
    if (!dev->initialized)  return XY_ACTUATOR_OK;

    if (dev->api->deinit) {
        if (dev->api->deinit(dev) != 0) return XY_ACTUATOR_ERROR;
    }
    dev->initialized = false;
    return XY_ACTUATOR_OK;
}

/* ── Control ─────────────────────────────────────────────────────────── */

xy_actuator_status_t xy_actuator_set(xy_actuator_device_t *dev,
                                     xy_actuator_channel_t ch, int32_t val)
{
    if (!dev || !dev->api || !dev->api->set) return XY_ACTUATOR_ERROR_INVALID_PARAM;
    if (!dev->initialized) return XY_ACTUATOR_ERROR_NOT_INITIALIZED;
    return (dev->api->set(dev, ch, val) == 0) ? XY_ACTUATOR_OK : XY_ACTUATOR_ERROR;
}

xy_actuator_status_t xy_actuator_get(xy_actuator_device_t *dev,
                                     xy_actuator_channel_t ch, int32_t *val)
{
    if (!dev || !dev->api || !dev->api->get || !val)
        return XY_ACTUATOR_ERROR_INVALID_PARAM;
    if (!dev->initialized) return XY_ACTUATOR_ERROR_NOT_INITIALIZED;
    return (dev->api->get(dev, ch, val) == 0) ? XY_ACTUATOR_OK : XY_ACTUATOR_ERROR;
}

xy_actuator_status_t xy_actuator_on(xy_actuator_device_t *dev)
{
    return xy_actuator_set(dev, XY_ACTUATOR_CHAN_STATE, 1);
}

xy_actuator_status_t xy_actuator_off(xy_actuator_device_t *dev)
{
    return xy_actuator_set(dev, XY_ACTUATOR_CHAN_STATE, 0);
}

xy_actuator_status_t xy_actuator_toggle(xy_actuator_device_t *dev)
{
    if (!dev || !dev->initialized) return XY_ACTUATOR_ERROR_NOT_INITIALIZED;

    int32_t cur = 0;
    if (dev->api->get && dev->api->get(dev, XY_ACTUATOR_CHAN_STATE, &cur) == 0) {
        return xy_actuator_set(dev, XY_ACTUATOR_CHAN_STATE, cur ? 0 : 1);
    }
    /* get not implemented — can't toggle reliably */
    return XY_ACTUATOR_ERROR_NOT_SUPPORTED;
}

/* ── Device lookup ───────────────────────────────────────────────────── */

xy_actuator_device_t *xy_actuator_device_get(const char *name)
{
    if (!name) return NULL;
    for (xy_actuator_device_t *d = s_head; d; d = d->next) {
        if (xy_strcmp(d->name, name) == 0) return d;
    }
    return NULL;
}

xy_actuator_device_t *xy_actuator_device_get_by_type(xy_actuator_type_t type,
                                                      uint8_t index)
{
    uint8_t n = 0;
    for (xy_actuator_device_t *d = s_head; d; d = d->next) {
        if (d->type == type) {
            if (n == index) return d;
            n++;
        }
    }
    return NULL;
}

void xy_actuator_device_foreach(bool (*cb)(xy_actuator_device_t *, void *),
                                void *user)
{
    if (!cb) return;
    for (xy_actuator_device_t *d = s_head; d; d = d->next) {
        if (!cb(d, user)) break;
    }
}

uint8_t xy_actuator_device_count(void)
{
    return s_count;
}

/* ── Pre-defined step tables ─────────────────────────────────────────── */

const xy_actuator_step_t xy_actuator_steps_blink_slow[2] = {
    { 1, 500u },
    { 0, 500u },
};

const xy_actuator_step_t xy_actuator_steps_blink_fast[2] = {
    { 1, 100u },
    { 0, 100u },
};

const xy_actuator_step_t xy_actuator_steps_beep_once[2] = {
    { 1, 200u },
    { 0, 800u },
};

/*
 * SOS morse:  · · ·  — — —  · · ·
 * Dot  = 100 ms on,  100 ms off
 * Dash = 300 ms on,  100 ms off
 * Inter-letter gap = 200 ms extra off (total 300 ms after last symbol)
 * Inter-word gap   = 600 ms extra off (total 700 ms after last letter)
 */
const xy_actuator_step_t xy_actuator_steps_sos[18] = {
    /* S: dot dot dot */
    { 1, 100u }, { 0, 100u },
    { 1, 100u }, { 0, 100u },
    { 1, 100u }, { 0, 300u },   /* inter-letter gap */
    /* O: dash dash dash */
    { 1, 300u }, { 0, 100u },
    { 1, 300u }, { 0, 100u },
    { 1, 300u }, { 0, 300u },   /* inter-letter gap */
    /* S: dot dot dot */
    { 1, 100u }, { 0, 100u },
    { 1, 100u }, { 0, 100u },
    { 1, 100u }, { 0, 700u },   /* inter-word gap before repeat */
};

/* ── Pattern player ──────────────────────────────────────────────────── */

static void _pattern_cb(void *arg)
{
    xy_actuator_pattern_t *pat = (xy_actuator_pattern_t *)arg;

    pat->pos++;

    if (pat->pos >= pat->count) {
        /* End of one full play */
        if (pat->plays_left < 0) {
            /* Infinite: wrap around */
            pat->pos = 0;
        } else if (pat->plays_left <= 1) {
            /* Last play finished — turn off and stop */
            xy_actuator_set(pat->dev, pat->channel, 0);
            pat->plays_left = 0;
            return;   /* ctimer not rearmed → pattern stops */
        } else {
            pat->plays_left--;
            pat->pos = 0;
        }
    }

    xy_actuator_set(pat->dev, pat->channel, pat->steps[pat->pos].value);
    ctimer_set(&pat->ct, pat->steps[pat->pos].duration_ms, _pattern_cb, pat);
}

void xy_actuator_pattern_start(xy_actuator_pattern_t    *pat,
                               xy_actuator_device_t     *dev,
                               xy_actuator_channel_t     channel,
                               const xy_actuator_step_t *steps,
                               uint8_t                   count,
                               int8_t                    plays)
{
    if (!pat || !dev || !steps || count == 0u) return;

    ctimer_stop(&pat->ct);   /* cancel any in-flight timer first */

    pat->dev        = dev;
    pat->channel    = channel;
    pat->steps      = steps;
    pat->count      = count;
    pat->pos        = 0;
    pat->plays_left = (plays == 0) ? 1 : plays;  /* 0 treated as "once" */

    /* Apply first step immediately, then arm the timer. */
    xy_actuator_set(dev, channel, steps[0].value);
    ctimer_set(&pat->ct, steps[0].duration_ms, _pattern_cb, pat);
}

void xy_actuator_pattern_stop(xy_actuator_pattern_t *pat)
{
    if (!pat) return;
    ctimer_stop(&pat->ct);
    if (pat->dev) xy_actuator_set(pat->dev, pat->channel, 0);
    pat->plays_left = 0;
}

bool xy_actuator_pattern_active(const xy_actuator_pattern_t *pat)
{
    if (!pat) return false;
    return !ctimer_expired(&pat->ct);
}
