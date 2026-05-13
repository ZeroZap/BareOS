/**
 * @file at_client.h
 * @brief 兼容RTOS/裸机的AT客户端框架
 */

#ifndef AT_CLIENT_H
#define AT_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 配置选项 */
#define AT_MAX_DEVICES     4    /* 最大AT设备数量 */
#define AT_MAX_CMD_LEN     256  /* 最大AT命令长度 */
#define AT_MAX_RESP_LEN    1024 /* 最大响应长度 */
#define AT_DEFAULT_TIMEOUT 3000 /* 默认超时(ms) */

/* 平台抽象层 - 通过宏定义选择RTOS或裸机 */
#ifdef USE_RTOS
#include "FreeRTOS.h"
#include "semphr.h"
#define AT_MUTEX_DECLARE(mutex) SemaphoreHandle_t mutex
#define AT_MUTEX_INIT(mutex)    mutex = xSemaphoreCreateMutex()
#define AT_MUTEX_LOCK(mutex)    xSemaphoreTake(mutex, portMAX_DELAY)
#define AT_MUTEX_UNLOCK(mutex)  xSemaphoreGive(mutex)
#define AT_DELAY_MS(ms)         vTaskDelay(pdMS_TO_TICKS(ms))
#else
#define AT_MUTEX_DECLARE(mutex)
#define AT_MUTEX_INIT(mutex)
#define AT_MUTEX_LOCK(mutex)
#define AT_MUTEX_UNLOCK(mutex)
#define AT_DELAY_MS(ms) /* 裸机延时需要用户实现 */
#endif

/* 系统tick类型 */
typedef uint32_t at_tick_t;

/* 设备数据读取回调 */
typedef struct at_device at_device_t;
typedef int (*at_read_byte_t)(at_device_t *dev);
typedef void (*at_write_t)(at_device_t *dev, const uint8_t *data, uint32_t len);

/* 响应处理器 */
typedef enum {
    AT_RESP_OK = 0,  /* OK响应 */
    AT_RESP_ERROR,   /* ERROR响应 */
    AT_RESP_CUSTOM,  /* 自定义响应 */
    AT_RESP_TIMEOUT, /* 超时 */
    AT_RESP_NONE     /* 无响应 */
} at_resp_type_t;

typedef bool (*at_resp_handler_t)(at_device_t *dev, const char *resp_line,
                                  void *user_data, at_resp_type_t *type);

/* 响应缓存 */
typedef struct {
    char buffer[AT_MAX_RESP_LEN];
    uint32_t length;
    bool overflow;
} at_resp_buffer_t;

/* AT设备结构体 */
struct at_device {
    /* 设备标识 */
    uint8_t id;
    char name[16];

    /* 数据接口 */
    at_read_byte_t read_byte; /* 读取一个字节 */
    at_write_t write_data;    /* 写入数据 */
    void *user_data;          /* 用户数据(如ringbuffer句柄) */

    /* 超时处理 */
    at_tick_t (*get_tick)(void); /* 获取系统tick函数 */
    uint32_t timeout_ms;         /* 超时时间(ms) */

    /* 响应处理 */
    at_resp_buffer_t resp_buf;
    at_resp_handler_t resp_handler;
    void *resp_user_data;

    /* 状态 */
    bool is_busy;
    AT_MUTEX_DECLARE(mutex)

    /* 统计信息 */
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
};

/* AT客户端管理 */
typedef struct {
    at_device_t *devices[AT_MAX_DEVICES];
    uint8_t device_count;
    uint8_t default_device;
} at_client_t;

/* API函数声明 */

/**
 * @brief 初始化AT客户端
 */
at_client_t *at_client_init(void);

/**
 * @brief 注册AT设备
 * @param name 设备名称
 * @param read_byte 读取字节函数
 * @param write_data 写入数据函数
 * @param get_tick 获取tick函数
 * @param user_data 用户数据
 * @return 设备指针，失败返回NULL
 */
at_device_t *at_device_register(const char *name, at_read_byte_t read_byte,
                                at_write_t write_data,
                                at_tick_t (*get_tick)(void), void *user_data);

/**
 * @brief 设置默认设备
 */
int at_set_default_device(at_device_t *dev);

/**
 * @brief 发送AT命令并等待响应
 * @param dev AT设备
 * @param cmd AT命令
 * @param handler 响应处理器
 * @param user_data 用户数据
 * @param timeout 超时时间(ms)
 * @return 响应类型
 */
at_resp_type_t at_send_command(at_device_t *dev, const char *cmd,
                               at_resp_handler_t handler, void *user_data,
                               uint32_t timeout);

/**
 * @brief 发送AT命令(使用默认设备)
 */
at_resp_type_t at_send_cmd(const char *cmd, at_resp_handler_t handler,
                           void *user_data, uint32_t timeout);

/**
 * @brief 发送原始数据
 */
int at_send_raw(at_device_t *dev, const uint8_t *data, uint32_t len);

/**
 * @brief 读取响应行
 * @return 读取到的行长度，0表示无数据，-1表示错误
 */
int at_readline(at_device_t *dev, char *buffer, uint32_t size);

/**
 * @brief 等待特定响应
 */
bool at_expect(at_device_t *dev, const char *expect, uint32_t timeout);

/**
 * @brief 获取设备统计信息
 */
void at_get_stats(at_device_t *dev, uint32_t *tx, uint32_t *rx, uint32_t *err);
#endif /* AT_CLIENT_H */