/**
 * @file xy_gui_event.c
 * @brief GUI Event System Implementation - 事件系统实现
 * @version 1.0.0
 * @date 2026-03-14
 */

#include "xy_gui_event.h"
#include <string.h>
#include <stdlib.h>

/* ==================== 全局事件系统状态 ==================== */

static bool s_event_system_initialized = false;
static xy_gui_event_queue_t s_global_queue;

/* ==================== 事件系统核心 API ==================== */

int xy_gui_event_system_init(void)
{
    if (s_event_system_initialized) {
        return -1;  /* 已经初始化 */
    }
    
    memset(&s_global_queue, 0, sizeof(s_global_queue));
    xy_gui_event_queue_init(&s_global_queue);
    
    s_event_system_initialized = true;
    return 0;
}

void xy_gui_event_system_deinit(void)
{
    if (!s_event_system_initialized) {
        return;
    }
    
    xy_gui_event_queue_clear(&s_global_queue);
    s_event_system_initialized = false;
}

/* ==================== 事件队列实现 ==================== */

int xy_gui_event_queue_init(xy_gui_event_queue_t *queue)
{
    if (!queue) {
        return -1;
    }
    
    memset(queue, 0, sizeof(*queue));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    
    return 0;
}

int xy_gui_event_push(xy_gui_event_queue_t *queue, const xy_gui_event_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    if (queue->count >= XY_GUI_EVENT_QUEUE_SIZE) {
        return -1;  /* 队列满 */
    }
    
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1) % XY_GUI_EVENT_QUEUE_SIZE;
    queue->count++;
    
    return 0;
}

int xy_gui_event_pop(xy_gui_event_queue_t *queue, xy_gui_event_t *event)
{
    if (!queue || !event) {
        return -1;
    }
    
    if (queue->count == 0) {
        return -1;  /* 队列空 */
    }
    
    *event = queue->events[queue->head];
    queue->head = (queue->head + 1) % XY_GUI_EVENT_QUEUE_SIZE;
    queue->count--;
    
    return 0;
}

uint16_t xy_gui_event_queue_count(xy_gui_event_queue_t *queue)
{
    if (!queue) {
        return 0;
    }
    
    return queue->count;
}

void xy_gui_event_queue_clear(xy_gui_event_queue_t *queue)
{
    if (!queue) {
        return;
    }
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/* ==================== 事件创建辅助函数 ==================== */

xy_gui_event_t xy_gui_event_create_touch(xy_gui_event_type_t type, int16_t x, int16_t y)
{
    xy_gui_event_t event;
    memset(&event, 0, sizeof(event));
    
    event.type = type;
    event.timestamp = 0;  /* 需要外部设置 */
    event.data.point.x = x;
    event.data.point.y = y;
    event.data.point.dx = 0;
    event.data.point.dy = 0;
    event.data.point.pressure = 128;  /* 默认压力值 */
    event.data.point.finger_id = 0;
    event.handled = false;
    event.target = NULL;
    
    return event;
}

xy_gui_event_t xy_gui_event_create_key(xy_gui_key_t key, bool is_press)
{
    xy_gui_event_t event;
    memset(&event, 0, sizeof(event));
    
    event.type = is_press ? XY_GUI_EVENT_KEY_DOWN : XY_GUI_EVENT_KEY_UP;
    event.timestamp = 0;
    event.data.key.key = key;
    event.data.key.repeat_count = 0;
    event.handled = false;
    event.target = NULL;
    
    return event;
}

xy_gui_event_t xy_gui_event_create_click(int16_t x, int16_t y)
{
    xy_gui_event_t event;
    memset(&event, 0, sizeof(event));
    
    event.type = XY_GUI_EVENT_CLICK;
    event.timestamp = 0;
    event.data.point.x = x;
    event.data.point.y = y;
    event.handled = false;
    event.target = NULL;
    
    return event;
}

xy_gui_event_t xy_gui_event_create_value_changed(int32_t value)
{
    xy_gui_event_t event;
    memset(&event, 0, sizeof(event));
    
    event.type = XY_GUI_EVENT_VALUE_CHANGED;
    event.timestamp = 0;
    event.data.value = value;
    event.handled = false;
    event.target = NULL;
    
    return event;
}

/* ==================== 事件分发器 ==================== */

/**
 * @brief 事件分发器状态
 */
typedef struct {
    xy_gui_event_handler_t handler;
    void *user_data;
    uint8_t priority;
    bool enabled;
} xy_gui_event_listener_t;

#define XY_GUI_MAX_LISTENERS  8

static xy_gui_event_listener_t s_listeners[XY_GUI_MAX_LISTENERS];
static uint8_t s_listener_count = 0;

/**
 * @brief 注册事件监听器
 */
int xy_gui_event_register_listener(xy_gui_event_handler_t handler, void *user_data, uint8_t priority)
{
    if (!handler || s_listener_count >= XY_GUI_MAX_LISTENERS) {
        return -1;
    }
    
    s_listeners[s_listener_count].handler = handler;
    s_listeners[s_listener_count].user_data = user_data;
    s_listeners[s_listener_count].priority = priority;
    s_listeners[s_listener_count].enabled = true;
    s_listener_count++;
    
    return 0;
}

/**
 * @brief 注销事件监听器
 */
int xy_gui_event_unregister_listener(xy_gui_event_handler_t handler)
{
    if (!handler) {
        return -1;
    }
    
    for (uint8_t i = 0; i < s_listener_count; i++) {
        if (s_listeners[i].handler == handler) {
            /* 移动后续监听器 */
            for (uint8_t j = i; j < s_listener_count - 1; j++) {
                s_listeners[j] = s_listeners[j + 1];
            }
            s_listener_count--;
            return 0;
        }
    }
    
    return -1;  /* 未找到 */
}

/**
 * @brief 分发事件到所有监听器
 */
bool xy_gui_event_dispatch(xy_gui_event_t *event)
{
    if (!event) {
        return false;
    }
    
    bool handled = false;
    
    /* 按优先级分发 */
    for (uint8_t i = 0; i < s_listener_count; i++) {
        if (s_listeners[i].enabled && s_listeners[i].handler) {
            if (s_listeners[i].handler(event, s_listeners[i].user_data)) {
                handled = true;
                if (event->handled) {
                    break;  /* 事件已处理，停止分发 */
                }
            }
        }
    }
    
    return handled;
}
