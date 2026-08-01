#include "xy_hal_n32l40x_internal.h"

static uint8_t rtc_year_to_n32(uint16_t year)
{
    return (uint8_t)(year >= 2000u ? (year - 2000u) : year);
}

static uint16_t rtc_year_from_n32(uint8_t year)
{
    return (uint16_t)(2000u + year);
}

int xy_hal_rtc_init(void *rtc, const xy_hal_rtc_config_t *config)
{
    RTC_InitType init;
    bool use_lse = config ? config->use_lse : true;
    (void)rtc;

    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    PWR_BackupAccessEnable(ENABLE);

    if (use_lse) {
        RCC_ConfigLse(RCC_LSE_ENABLE, 0u);
        RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSE);
    } else {
        RCC_EnableLsi(ENABLE);
        RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSI);
    }
    RCC_EnableRtcClk(ENABLE);

    RTC_StructInit(&init);
    init.RTC_HourFormat = RTC_24HOUR_FORMAT;
    init.RTC_AsynchPrediv = 127u;
    init.RTC_SynchPrediv = 255u;
    if (!use_lse) {
        init.RTC_SynchPrediv = 312u;
    }
    if (RTC_Init(&init) != SUCCESS) return XY_HAL_ERROR;
    if (RTC_WaitForSynchro() != SUCCESS) return XY_HAL_ERROR;
    return XY_HAL_OK;
}

int xy_hal_rtc_deinit(void *rtc)
{
    (void)rtc;
    return RTC_DeInit() == SUCCESS ? XY_HAL_OK : XY_HAL_ERROR;
}

int xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time)
{
    RTC_TimeType t;
    RTC_DateType d;
    (void)rtc;
    if (!time) return XY_HAL_INVALID_PARAM;
    RTC_GetTime(RTC_FORMAT_BIN, &t);
    RTC_GetDate(RTC_FORMAT_BIN, &d);
    time->year = rtc_year_from_n32(d.Year);
    time->month = d.Month;
    time->day = d.Date;
    time->weekday = d.WeekDay;
    time->hour = t.Hours;
    time->minute = t.Minutes;
    time->second = t.Seconds;
    return XY_HAL_OK;
}

int xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time)
{
    RTC_TimeType t;
    RTC_DateType d;
    (void)rtc;
    if (!time || time->month == 0u || time->day == 0u ||
        time->hour > 23u || time->minute > 59u || time->second > 59u) {
        return XY_HAL_INVALID_PARAM;
    }
    RTC_TimeStructInit(&t);
    RTC_DateStructInit(&d);
    t.Hours = time->hour;
    t.Minutes = time->minute;
    t.Seconds = time->second;
    t.H12 = RTC_AM_H12;
    d.Year = rtc_year_to_n32(time->year);
    d.Month = time->month;
    d.Date = time->day;
    d.WeekDay = time->weekday ? time->weekday : 1u;
    if (RTC_ConfigTime(RTC_FORMAT_BIN, &t) != SUCCESS) return XY_HAL_ERROR;
    if (RTC_SetDate(RTC_FORMAT_BIN, &d) != SUCCESS) return XY_HAL_ERROR;
    return XY_HAL_OK;
}

int xy_hal_rtc_set_wakeup(void *rtc, uint32_t timeout_s)
{
    (void)rtc;
    if (timeout_s == 0u || timeout_s > 0xffffu) return XY_HAL_INVALID_PARAM;
    RTC_EnableWakeUp(DISABLE);
    RTC_ConfigWakeUpClock(RTC_WKUPCLK_CK_SPRE_16BITS);
    RTC_SetWakeUpCounter(timeout_s);
    RTC_ClrFlag(RTC_FLAG_WTF);
    RTC_ConfigInt(RTC_INT_WUT, ENABLE);
    return RTC_EnableWakeUp(ENABLE) == SUCCESS ? XY_HAL_OK : XY_HAL_ERROR;
}

int xy_hal_rtc_cancel_wakeup(void *rtc)
{
    (void)rtc;
    RTC_ConfigInt(RTC_INT_WUT, DISABLE);
    return RTC_EnableWakeUp(DISABLE) == SUCCESS ? XY_HAL_OK : XY_HAL_ERROR;
}

void xy_hal_rtc_irq_handler(void *rtc)
{
    (void)rtc;
    if (RTC_GetITStatus(RTC_INT_WUT) != RESET) {
        RTC_ClrIntPendingBit(RTC_INT_WUT);
        RTC_ClrFlag(RTC_FLAG_WTF);
    }
}
