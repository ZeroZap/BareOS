#ifndef __SENSOR_CONFIG_H__
#define __SENSOR_CONFIG_H__

/* ==================== 基础配置 ==================== */
/* 最大传感器设备数量 */
#ifndef SENSOR_MAX_DEVICES
#define SENSOR_MAX_DEVICES 16
#endif

/* 设备名称最大长度 */
#ifndef SENSOR_NAME_MAX_LEN
#define SENSOR_NAME_MAX_LEN 32
#endif

/* ==================== 功能开关 ==================== */
/* FIFO功能 */
#ifndef SENSOR_ENABLE_FIFO
#define SENSOR_ENABLE_FIFO 1
#endif

#if SENSOR_ENABLE_FIFO
#ifndef SENSOR_DEFAULT_FIFO_SIZE
#define SENSOR_DEFAULT_FIFO_SIZE 64
#endif
#endif

/* 中断功能 */
#ifndef SENSOR_ENABLE_INTERRUPT
#define SENSOR_ENABLE_INTERRUPT 1
#endif

/* 校准功能 */
#ifndef SENSOR_ENABLE_CALIBRATION
#define SENSOR_ENABLE_CALIBRATION 1
#endif

#if SENSOR_ENABLE_CALIBRATION
#ifndef SENSOR_CALIBRATION_SAMPLES
#define SENSOR_CALIBRATION_SAMPLES 100
#endif
#endif

/* 滤波器功能 */
#ifndef SENSOR_ENABLE_FILTER
#define SENSOR_ENABLE_FILTER 1
#endif

#if SENSOR_ENABLE_FILTER
#ifndef SENSOR_DEFAULT_FILTER_SIZE
#define SENSOR_DEFAULT_FILTER_SIZE 8
#endif
#endif

/* DMA功能 */
#ifndef SENSOR_ENABLE_DMA
#define SENSOR_ENABLE_DMA 0
#endif

/* 传感器融合 */
#ifndef SENSOR_ENABLE_FUSION
#define SENSOR_ENABLE_FUSION 1
#endif

#if SENSOR_ENABLE_FUSION
#ifndef SENSOR_FUSION_SAMPLE_FREQ
#define SENSOR_FUSION_SAMPLE_FREQ 100.0f
#endif
#endif

/* 功耗管理 */
#ifndef SENSOR_ENABLE_POWER_MGMT
#define SENSOR_ENABLE_POWER_MGMT 1
#endif

/* 自测试功能 */
#ifndef SENSOR_ENABLE_SELF_TEST
#define SENSOR_ENABLE_SELF_TEST 1
#endif

/* 阈值检测 */
#ifndef SENSOR_ENABLE_THRESHOLD
#define SENSOR_ENABLE_THRESHOLD 1
#endif

/* 运动检测 */
#ifndef SENSOR_ENABLE_MOTION_DETECT
#define SENSOR_ENABLE_MOTION_DETECT 1
#endif

/* ==================== 系统适配 ==================== */
/* 文件系统支持 */
#ifndef SENSOR_USE_FILE_SYSTEM
#define SENSOR_USE_FILE_SYSTEM 0
#endif

/* OSAL支持 */
#ifndef SENSOR_USE_OSAL
#define SENSOR_USE_OSAL 0
#endif

/* 动态内存分配 */
#ifndef SENSOR_USE_MALLOC
#define SENSOR_USE_MALLOC 1
#endif

/* 浮点运算支持 */
#ifndef SENSOR_USE_FLOAT
#define SENSOR_USE_FLOAT 1
#endif

/* ==================== 调试配置 ==================== */
#ifndef SENSOR_DEBUG
#define SENSOR_DEBUG 0
#endif

#if SENSOR_DEBUG
#include <stdio.h>
#define SENSOR_LOG(fmt, ...) printf("[SENSOR] " fmt "\n", ##__VA_ARGS__)
#define SENSOR_ASSERT(cond)                                                \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("[SENSOR] Assert failed: %s:%d\n", __FILE__, __LINE__); \
            while (1)                                                      \
                ;                                                          \
        }                                                                  \
    } while (0)
#else
#define SENSOR_LOG(fmt, ...) ((void)0)
#define SENSOR_ASSERT(cond)  ((void)0)
#endif

/* ==================== 平台适配宏 ==================== */
#if SENSOR_USE_OSAL
/* OSAL互斥锁 */
#include "osal.h"
typedef osal_mutex_t sensor_mutex_t;
#define SENSOR_MUTEX_CREATE(mutex)  osal_mutex_create(&mutex)
#define SENSOR_MUTEX_LOCK(mutex)    osal_mutex_lock(&mutex)
#define SENSOR_MUTEX_UNLOCK(mutex)  osal_mutex_unlock(&mutex)
#define SENSOR_MUTEX_DESTROY(mutex) osal_mutex_destroy(&mutex)
#define SENSOR_DELAY_MS(ms)         osal_delay_ms(ms)
#define SENSOR_GET_TICK()           osal_get_tick()
#else
/* 裸机环境 - 使用中断禁用实现临界区 */
typedef uint32_t sensor_mutex_t;

static inline void sensor_enter_critical(sensor_mutex_t *state)
{
#ifdef __ARM_ARCH
    *state = __get_PRIMASK();
    __disable_irq();
#else
    (void)state;
#endif
}

static inline void sensor_exit_critical(sensor_mutex_t state)
{
#ifdef __ARM_ARCH
    __set_PRIMASK(state);
#else
    (void)state;
#endif
}

#define SENSOR_MUTEX_CREATE(mutex)  ((void)0)
#define SENSOR_MUTEX_LOCK(mutex)    sensor_enter_critical(&mutex)
#define SENSOR_MUTEX_UNLOCK(mutex)  sensor_exit_critical(mutex)
#define SENSOR_MUTEX_DESTROY(mutex) ((void)0)

/* 延时函数需要用户实现 */
extern void delay_ms(uint32_t ms);
extern uint32_t get_tick_ms(void);
#define SENSOR_DELAY_MS(ms)         delay_ms(ms)
#define SENSOR_GET_TICK()           get_tick_ms()
#endif

/* 内存分配 */
#if SENSOR_USE_MALLOC
#include <stdlib.h>
#define SENSOR_MALLOC(size)      malloc(size)
#define SENSOR_FREE(ptr)         free(ptr)
#define SENSOR_CALLOC(num, size) calloc(num, size)
#else
/* 使用静态内存池 */
extern void *sensor_mem_alloc(size_t size);
extern void sensor_mem_free(void *ptr);
#define SENSOR_MALLOC(size)      sensor_mem_alloc(size)
#define SENSOR_FREE(ptr)         sensor_mem_free(ptr)
#define SENSOR_CALLOC(num, size) sensor_mem_alloc((num) * (size))
#endif

#endif /* __SENSOR_CONFIG_H__ */