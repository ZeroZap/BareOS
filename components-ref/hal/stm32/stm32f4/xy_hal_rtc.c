/**
 * @file xy_hal_rtc_stm32.c
 * @brief RTC HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_rtc.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <time.h>

// static uint32_t // xy_to_stm32_format(xy_hal_rtc_format_t format)
{
    return (format == XY_HAL_RTC_FORMAT_BIN) ? RTC_FORMAT_BIN : RTC_FORMAT_BCD;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_init(void *rtc)
{
    if (!rtc) {
        return -1;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)rtc;

    hrtc->Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc->Init.AsynchPrediv   = 127;
    hrtc->Init.SynchPrediv    = 255;
    hrtc->Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(hrtc) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_deinit(void *rtc)
{
    if (!rtc) {
        return -1;
    }

    if (HAL_RTC_DeInit((RTC_HandleTypeDef *)rtc) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return -1;
    }

    RTC_TimeTypeDef sTime = { 0 };
    sTime.Hours           = time->hours;
    sTime.Minutes         = time->minutes;
    sTime.Seconds         = time->seconds;
    sTime.SubSeconds      = time->subseconds;
    sTime.DayLightSaving  = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation  = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(
            (RTC_HandleTypeDef *)rtc, &sTime, // xy_to_stm32_format(format))
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return -1;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (HAL_RTC_GetTime(
            (RTC_HandleTypeDef *)rtc, &sTime, // xy_to_stm32_format(format))
        != HAL_OK) {
        return -1;
    }

    time->hours      = sTime.Hours;
    time->minutes    = sTime.Minutes;
    time->seconds    = sTime.Seconds;
    time->subseconds = sTime.SubSeconds;

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return -1;
    }

    RTC_DateTypeDef sDate = { 0 };
    sDate.WeekDay         = date->weekday;
    sDate.Month           = date->month;
    sDate.Date            = date->date;
    sDate.Year            = date->year % 100; /* STM32 uses 2-digit year */

    if (HAL_RTC_SetDate(
            (RTC_HandleTypeDef *)rtc, &sDate, // xy_to_stm32_format(format))
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return -1;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (HAL_RTC_GetDate(
            (RTC_HandleTypeDef *)rtc, &sDate, // xy_to_stm32_format(format))
        != HAL_OK) {
        return -1;
    }

    date->weekday = sDate.WeekDay;
    date->month   = sDate.Month;
    date->date    = sDate.Date;
    date->year    = sDate.Year + 2000; /* Convert back to 4-digit */

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                         char alarm_id)
{
    if (!rtc || !alarm) {
        return -1;
    }

    RTC_AlarmTypeDef sAlarm     = { 0 };
    sAlarm.AlarmTime.Hours      = alarm->time.hours;
    sAlarm.AlarmTime.Minutes    = alarm->time.minutes;
    sAlarm.AlarmTime.Seconds    = alarm->time.seconds;
    sAlarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    sAlarm.AlarmDateWeekDay     = alarm->date;
    sAlarm.Alarm                = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_SetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    if (!rtc || !alarm) {
        return -1;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };
    sAlarm.Alarm            = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_GetAlarm(
            (RTC_HandleTypeDef *)rtc, &sAlarm, sAlarm.Alarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return -1;
    }

    alarm->time.hours      = sAlarm.AlarmTime.Hours;
    alarm->time.minutes    = sAlarm.AlarmTime.Minutes;
    alarm->time.seconds    = sAlarm.AlarmTime.Seconds;
    alarm->time.subseconds = sAlarm.AlarmTime.SubSeconds;
    alarm->date            = sAlarm.AlarmDateWeekDay;

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    if (!rtc) {
        return -1;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };
    sAlarm.Alarm            = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_SetAlarm_IT((RTC_HandleTypeDef *)rtc, &sAlarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    if (!rtc) {
        return -1;
    }

    uint32_t alarm = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_DeactivateAlarm((RTC_HandleTypeDef *)rtc, alarm) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                 xy_hal_rtc_callback_t callback, void *arg)
{
    /* Store callback in context */
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    if (!rtc) {
        return -1;
    }

    xy_hal_rtc_time_t time;
    xy_hal_rtc_date_t date;

    if (xy_hal_rtc_get_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != 0) {
        return -1;
    }

    if (xy_hal_rtc_get_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != 0) {
        return -1;
    }

    struct tm timeinfo = { 0 };
    timeinfo.tm_year   = date.year - 1900;
    timeinfo.tm_mon    = date.month - 1;
    timeinfo.tm_mday   = date.date;
    timeinfo.tm_hour   = time.hours;
    timeinfo.tm_min    = time.minutes;
    timeinfo.tm_sec    = time.seconds;

    return (int64_t)mktime(&timeinfo);

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    if (!rtc) {
        return -1;
    }

    time_t ts           = (time_t)timestamp;
    struct tm *timeinfo = localtime(&ts);

    xy_hal_rtc_time_t time = { .hours      = timeinfo->tm_hour,
                               .minutes    = timeinfo->tm_min,
                               .seconds    = timeinfo->tm_sec,
                               .subseconds = 0 };

    xy_hal_rtc_date_t date = { .weekday = timeinfo->tm_wday == 0
                                              ? 7
                                              : timeinfo->tm_wday,
                               .month   = timeinfo->tm_mon + 1,
                               .date    = timeinfo->tm_mday,
                               .year    = timeinfo->tm_year + 1900 };

    if (xy_hal_rtc_set_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != 0) {
        return -1;
    }

    if (xy_hal_rtc_set_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != 0) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* STM32_HAL_ENABLED */
