/**
 * @file xy_hal_rtc.c
 * @brief RTC HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_rtc.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* RTC context structure */
typedef struct {
    RTC_HandleTypeDef *hrtc;
    xy_hal_rtc_callback_t callback;
    void *arg;
    uint8_t initialized;
} rtc_ctx_t;

/* Maximum RTC instances (typically only 1) */
#define MAX_RTC_INSTANCES 2
static rtc_ctx_t g_rtc_ctx[MAX_RTC_INSTANCES];

/**
 * @brief Find RTC context by handle
 */
static rtc_ctx_t *find_rtc_ctx(void *rtc)
{
    for (size_t i = 0; i < MAX_RTC_INSTANCES; i++) {
        if (g_rtc_ctx[i].hrtc == rtc) {
            return &g_rtc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new RTC context
 */
static rtc_ctx_t *alloc_rtc_ctx(void)
{
    for (size_t i = 0; i < MAX_RTC_INSTANCES; i++) {
        if (g_rtc_ctx[i].hrtc == NULL) {
            return &g_rtc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert binary to BCD
 */
static uint8_t bin_to_bcd(uint8_t value)
{
    return ((value / 10) << 4) | (value % 10);
}

/**
 * @brief Convert BCD to binary
 */
static uint8_t bcd_to_bin(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

xy_hal_error_t xy_hal_rtc_init(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)rtc;

    /* Check if already initialized */
    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_rtc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* Configure RTC */
    hrtc->Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc->Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
    hrtc->Init.SynchPrediv    = RTC_SYNCH_PREDIV;
    hrtc->Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc->Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    hrtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(hrtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hrtc        = hrtc;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_RTC_DeInit((RTC_HandleTypeDef *)rtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hrtc        = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        sTime.Hours   = bin_to_bcd(time->hours);
        sTime.Minutes = bin_to_bcd(time->minutes);
        sTime.Seconds = bin_to_bcd(time->seconds);
    } else {
        sTime.Hours   = time->hours;
        sTime.Minutes = time->minutes;
        sTime.Seconds = time->seconds;
    }

    sTime.SubSeconds    = time->subseconds;
    sTime.TimeFormat    = RTC_HOURFORMAT12_AM;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime((RTC_HandleTypeDef *)rtc, &sTime,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (HAL_RTC_GetTime((RTC_HandleTypeDef *)rtc, &sTime,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        time->hours       = bcd_to_bin(sTime.Hours);
        time->minutes     = bcd_to_bin(sTime.Minutes);
        time->seconds     = bcd_to_bin(sTime.Seconds);
        time->subseconds  = sTime.SubSeconds;
    } else {
        time->hours       = sTime.Hours;
        time->minutes     = sTime.Minutes;
        time->seconds     = sTime.Seconds;
        time->subseconds  = sTime.SubSeconds;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        sDate.WeekDay = bin_to_bcd(date->weekday);
        sDate.Month   = bin_to_bcd(date->month);
        sDate.Date    = bin_to_bcd(date->date);
        sDate.Year    = bin_to_bcd(date->year);
    } else {
        sDate.WeekDay = date->weekday;
        sDate.Month   = date->month;
        sDate.Date    = date->date;
        sDate.Year    = date->year;
    }

    if (HAL_RTC_SetDate((RTC_HandleTypeDef *)rtc, &sDate,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (HAL_RTC_GetDate((RTC_HandleTypeDef *)rtc, &sDate,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        date->weekday = bcd_to_bin(sDate.WeekDay);
        date->month   = bcd_to_bin(sDate.Month);
        date->date    = bcd_to_bin(sDate.Date);
        date->year    = bcd_to_bin(sDate.Year);
    } else {
        date->weekday = sDate.WeekDay;
        date->month   = sDate.Month;
        date->date    = sDate.Date;
        date->year    = sDate.Year;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id)
{
    XY_UNUSED(alarm_id);
    if (!rtc || !alarm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };

    sAlarm.AlarmTime.Hours   = bin_to_bcd(alarm->time.hours);
    sAlarm.AlarmTime.Minutes = bin_to_bcd(alarm->time.minutes);
    sAlarm.AlarmTime.Seconds = bin_to_bcd(alarm->time.seconds);
    sAlarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    sAlarm.AlarmMask = alarm->alarm_mask;
    sAlarm.Alarm = RTC_ALARM_A;

    if (HAL_RTC_SetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm,
                         RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id)
{
    XY_UNUSED(alarm_id);
    if (!rtc || !alarm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };

    if (HAL_RTC_GetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm,
                         RTC_ALARM_A, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    alarm->time.hours    = bcd_to_bin(sAlarm.AlarmTime.Hours);
    alarm->time.minutes  = bcd_to_bin(sAlarm.AlarmTime.Minutes);
    alarm->time.seconds  = bcd_to_bin(sAlarm.AlarmTime.Seconds);
    alarm->time.subseconds = sAlarm.AlarmTime.SubSeconds;
    alarm->alarm_mask    = sAlarm.AlarmMask;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    XY_UNUSED(alarm_id);
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_RTC_AlarmIRQCommand((RTC_HandleTypeDef *)rtc, RTC_ALARM_A,
                                ENABLE) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    XY_UNUSED(alarm_id);
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_RTC_AlarmIRQCommand((RTC_HandleTypeDef *)rtc, RTC_ALARM_A,
                                DISABLE) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(event);
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL) {
        ctx = alloc_rtc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hrtc = (RTC_HandleTypeDef *)rtc;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    xy_hal_rtc_time_t time = { 0 };
    xy_hal_rtc_date_t date = { 0 };

    if (xy_hal_rtc_get_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    if (xy_hal_rtc_get_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Simple timestamp calculation (not accounting for leap years) */
    int64_t timestamp = 0;
    timestamp += (int64_t)(date.year - 70) * 365 * 24 * 3600;
    timestamp += (int64_t)(date.month - 1) * 30 * 24 * 3600;
    timestamp += (int64_t)(date.date - 1) * 24 * 3600;
    timestamp += (int64_t)time.hours * 3600;
    timestamp += (int64_t)time.minutes * 60;
    timestamp += (int64_t)time.seconds;

    return timestamp;
}

xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Convert timestamp to date/time - simplified implementation */
    XY_UNUSED(timestamp);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief RTC Alarm callback
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(hrtc);
    if (ctx && ctx->callback) {
        ctx->callback(XY_HAL_RTC_EVENT_ALARM_A, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
