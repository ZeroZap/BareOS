/**
 * @file xy_hal_rtc.h
 * @brief RTC (Real-Time Clock) Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_RTC_H
#define XY_HAL_RTC_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* RTC Time Structure */
typedef struct {
    uint8_t hours;       /**< Hours (0-23) */
    uint8_t minutes;     /**< Minutes (0-59) */
    uint8_t seconds;     /**< Seconds (0-59) */
    uint16_t subseconds; /**< Sub-seconds */
} xy_hal_rtc_time_t;

/* RTC Date Structure */
typedef struct {
    uint8_t weekday; /**< Weekday (1-7, 1=Monday) */
    uint8_t month;   /**< Month (1-12) */
    uint8_t date;    /**< Date (1-31) */
    uint16_t year;   /**< Year (0-99 or full year) */
} xy_hal_rtc_date_t;

/* RTC Alarm Structure */
typedef struct {
    xy_hal_rtc_time_t time; /**< Alarm time */
    uint8_t date;           /**< Alarm date */
    uint8_t weekday;        /**< Alarm weekday */
    uint8_t alarm_mask;     /**< Alarm mask (which fields to match) */
} xy_hal_rtc_alarm_t;

/* RTC Alarm Mask Bits */
#define XY_HAL_RTC_ALARM_MASK_NONE    0x00
#include "xy_hal_error.h"
#define XY_HAL_RTC_ALARM_MASK_WEEKDAY 0x01
#include "xy_hal_error.h"
#define XY_HAL_RTC_ALARM_MASK_HOURS   0x02
#include "xy_hal_error.h"
#define XY_HAL_RTC_ALARM_MASK_MINUTES 0x04
#include "xy_hal_error.h"
#define XY_HAL_RTC_ALARM_MASK_SECONDS 0x08
#include "xy_hal_error.h"
#define XY_HAL_RTC_ALARM_MASK_ALL     0x0F
#include "xy_hal_error.h"

/* RTC Format */
typedef enum {
    XY_HAL_RTC_FORMAT_BIN = 0, /**< Binary format */
    XY_HAL_RTC_FORMAT_BCD,     /**< BCD format */
} xy_hal_rtc_format_t;

/* RTC Event Types */
typedef enum {
    XY_HAL_RTC_EVENT_ALARM_A = 0, /**< Alarm A event */
    XY_HAL_RTC_EVENT_ALARM_B,     /**< Alarm B event */
    XY_HAL_RTC_EVENT_WAKEUP,      /**< Wakeup timer event */
    XY_HAL_RTC_EVENT_TIMESTAMP,   /**< Timestamp event */
} xy_hal_rtc_event_t;

/* RTC callback */
typedef void (*xy_hal_rtc_callback_t)(xy_hal_rtc_event_t event, void *arg);

xy_hal_error_t xy_hal_rtc_init(void *rtc);
xy_hal_error_t xy_hal_rtc_deinit(void *rtc);
xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format);
xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format);
xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format);
xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format);
xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id);
xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id);
xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id);
xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id);
xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback,
                                            void *arg);
int64_t xy_hal_rtc_get_timestamp(void *rtc);
xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_RTC_H */