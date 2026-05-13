/**
 * @file cellular_config.h
 * @brief Cellular Configuration for XinYi Framework
 * 
 * This file provides the configuration for FreeRTOS-Cellular-Interface
 * to work with XinYi Framework instead of FreeRTOS.
 */

#ifndef CELLULAR_CONFIG_H
#define CELLULAR_CONFIG_H

/* ==================== Memory Management ==================== */
#include <stdlib.h>
#define pvPortMalloc malloc
#define vPortFree free

/* ==================== Logging ==================== */
#include "xy_log.h"
#define LogError(message) XY_LOG_E message
#define LogWarn(message) XY_LOG_W message  
#define LogInfo(message) XY_LOG_I message
#define LogDebug(message) XY_LOG_D message

/* ==================== Platform Abstraction ==================== */
#include "xy_osal.h"

/* OS Tick type */
#define TickType_t uint32_t
#define portMAX_DELAY UINT32_MAX

/* OS functions */
#define xTaskDelayUntil(pxPreviousWakeTime, xTimeIncrement) \
    do { \
        xy_os_delay(xTimeIncrement); \
        *(pxPreviousWakeTime) = xy_os_kernel_get_tick_count(); \
    } while(0)

#define xTaskGetTickCount() xy_os_kernel_get_tick_count()

/* Mutex */
#define SemaphoreHandle_t void*
#define xSemaphoreCreateMutex() xy_os_mutex_new(NULL)
#define xSemaphoreTake(xSemaphore, xBlockTime) \
    (xy_os_mutex_lock((xy_os_mutex_id_t)(xSemaphore), (xBlockTime)) == XY_OS_OK)
#define xSemaphoreGive(xSemaphore) \
    (xy_os_mutex_unlock((xy_os_mutex_id_t)(xSemaphore)) == XY_OS_OK)
#define vSemaphoreDelete(xSemaphore) \
    xy_os_mutex_delete((xy_os_mutex_id_t)(xSemaphore))

/* Queue */
#define QueueHandle_t void*
#define xQueueCreate(uxQueueLength, uxItemSize) \
    xy_os_queue_new(uxQueueLength, uxItemSize, NULL)
#define xQueueSendToBack(xQueue, pvItemToQueue, xTicksToWait) \
    (xy_os_queue_send((xy_os_queue_id_t)(xQueue), pvItemToQueue, (xTicksToWait)) == XY_OS_OK)
#define xQueueReceive(xQueue, pvBuffer, xTicksToWait) \
    (xy_os_queue_recv((xy_os_queue_id_t)(xQueue), pvBuffer, (xTicksToWait)) == XY_OS_OK)
#define vQueueDelete(xQueue) \
    xy_os_queue_delete((xy_os_queue_id_t)(xQueue))

/* ==================== Network Configuration ==================== */
/* Disable complex network features for now */
#define CELLULAR_DO_NOT_USE_TLS 1

#endif /* CELLULAR_CONFIG_H */