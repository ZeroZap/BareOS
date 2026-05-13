/**
 * @file xy_gui_event.h
 * @brief GUI Event System - 事件系统
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * GUI 阶段 2: 事件系统核心头文件
 */

#ifndef XY_GUI_EVENT_H
#define XY_GUI_EVENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 事件类型 ==================== */

typedef enum {
    XY_GUI_EVENT_NONE = 0,
    
    /* 输入事件 */
    XY_GUI_EVENT_KEY_DOWN,          /**< 按键按下 */
    XY_GUI_EVENT_KEY_UP,            /**< 按键释放 */
    XY_GUI_EVENT_KEY_REPEAT,        /**< 按键重复 */
    
    /* 触摸事件 */
    XY_GUI_EVENT_TOUCH_DOWN,        /**< 触摸按下 */
    XY_GUI_EVENT_TOUCH_UP,          /**< 触摸释放 */
    XY_GUI_EVENT_TOUCH_MOVE,        /**< 触摸移动 */
    
    /* 鼠标事件 */
    XY_GUI_EVENT_MOUSE_DOWN,        /**< 鼠标按下 */
    XY_GUI_EVENT_MOUSE_UP,          /**< 鼠标释放 */
    XY_GUI_EVENT_MOUSE_MOVE,        /**< 鼠标移动 */
    XY_GUI_EVENT_MOUSE_WHEEL,       /**< 鼠标滚轮 */
    
    /* 控件事件 */
    XY_GUI_EVENT_CLICK,             /**< 点击 */
    XY_GUI_EVENT_DOUBLE_CLICK,      /**< 双击 */
    XY_GUI_EVENT_LONG_PRESS,        /**< 长按 */
    XY_GUI_EVENT_PRESS,             /**< 按下 */
    XY_GUI_EVENT_RELEASE,           /**< 释放 */
    XY_GUI_EVENT_VALUE_CHANGED,     /**< 值改变 */
    XY_GUI_EVENT_TEXT_CHANGED,      /**< 文本改变 */
    
    /* 焦点事件 */
    XY_GUI_EVENT_FOCUS_GAIN,        /**< 获得焦点 */
    XY_GUI_EVENT_FOCUS_LOST,        /**< 失去焦点 */
    
    /* 手势事件 */
    XY_GUI_EVENT_GESTURE_SWIPE_LEFT,   /**< 左滑 */
    XY_GUI_EVENT_GESTURE_SWIPE_RIGHT,  /**< 右滑 */
    XY_GUI_EVENT_GESTURE_SWIPE_UP,     /**< 上滑 */
    XY_GUI_EVENT_GESTURE_SWIPE_DOWN,   /**< 下滑 */
    XY_GUI_EVENT_GESTURE_PINCH_IN,     /**< 捏合 */
    XY_GUI_EVENT_GESTURE_PINCH_OUT,    /**< 展开 */
    
    /* 系统事件 */
    XY_GUI_EVENT_REFRESH,           /**< 刷新请求 */
    XY_GUI_EVENT_RESIZE,            /**< 尺寸改变 */
    
} xy_gui_event_type_t;

/* ==================== 按键码 ==================== */

typedef enum {
    XY_GUI_KEY_NONE = 0,
    
    /* 方向键 */
    XY_GUI_KEY_UP,
    XY_GUI_KEY_DOWN,
    XY_GUI_KEY_LEFT,
    XY_GUI_KEY_RIGHT,
    
    /* 功能键 */
    XY_GUI_KEY_ENTER,
    XY_GUI_KEY_ESCAPE,
    XY_GUI_KEY_BACK,
    XY_GUI_KEY_MENU,
    XY_GUI_KEY_HOME,
    
    /* 数字键 */
    XY_GUI_KEY_0,
    XY_GUI_KEY_1,
    XY_GUI_KEY_2,
    XY_GUI_KEY_3,
    XY_GUI_KEY_4,
    XY_GUI_KEY_5,
    XY_GUI_KEY_6,
    XY_GUI_KEY_7,
    XY_GUI_KEY_8,
    XY_GUI_KEY_9,
    
    /* ASCII 字符 (0x20-0x7F) */
    XY_GUI_KEY_SPACE = 0x20,
    
} xy_gui_key_t;

/* ==================== 事件数据结构 ==================== */

/**
 * @brief 触摸/ pointing 数据
 */
typedef struct {
    int16_t x;              /**< X 坐标 */
    int16_t y;              /**< Y 坐标 */
    int16_t dx;             /**< X 变化量 */
    int16_t dy;             /**< Y 变化量 */
    uint8_t pressure;       /**< 压力值 (0-255) */
    uint8_t finger_id;      /**< 手指 ID (多点触摸) */
} xy_gui_point_data_t;

/**
 * @brief 滚轮数据
 */
typedef struct {
    int16_t delta;          /**< 滚轮变化量 */
} xy_gui_wheel_data_t;

/**
 * @brief 按键数据
 */
typedef struct {
    xy_gui_key_t key;       /**< 按键码 */
    uint8_t repeat_count;   /**< 重复次数 */
} xy_gui_key_data_t;

/**
 * @brief 手势数据
 */
typedef struct {
    uint8_t direction;      /**< 方向 */
    uint16_t distance;      /**< 距离 */
    uint16_t duration;      /**< 持续时间 (ms) */
} xy_gui_gesture_data_t;

/**
 * @brief 事件数据联合体
 */
typedef union {
    xy_gui_point_data_t point;      /**< 触摸/鼠标数据 */
    xy_gui_wheel_data_t wheel;      /**< 滚轮数据 */
    xy_gui_key_data_t key;          /**< 按键数据 */
    xy_gui_gesture_data_t gesture;  /**< 手势数据 */
    int32_t value;                  /**< 值 (VALUE_CHANGED) */
    char text[64];                  /**< 文本 (TEXT_CHANGED) */
} xy_gui_event_data_t;

/**
 * @brief GUI 事件结构
 */
typedef struct {
    xy_gui_event_type_t type;       /**< 事件类型 */
    uint32_t timestamp;             /**< 时间戳 (ms) */
    xy_gui_event_data_t data;       /**< 事件数据 */
    bool handled;                   /**< 是否已处理 */
    void *target;                   /**< 目标控件 */
} xy_gui_event_t;

/* ==================== 事件回调类型 ==================== */

/**
 * @brief 事件处理函数类型
 * @param event 事件指针
 * @param user_data 用户数据
 * @return true 表示事件已处理，false 表示继续传递
 */
typedef bool (*xy_gui_event_handler_t)(xy_gui_event_t *event, void *user_data);

/* ==================== 事件队列 ==================== */

#define XY_GUI_EVENT_QUEUE_SIZE  16

/**
 * @brief 事件队列结构
 */
typedef struct {
    xy_gui_event_t events[XY_GUI_EVENT_QUEUE_SIZE];
    uint16_t head;                  /**< 队首索引 */
    uint16_t tail;                  /**< 队尾索引 */
    uint16_t count;                 /**< 当前事件数 */
} xy_gui_event_queue_t;

/* ==================== 事件系统 API ==================== */

/**
 * @brief 初始化事件系统
 * @return 0 成功，负值失败
 */
int xy_gui_event_system_init(void);

/**
 * @brief 反初始化事件系统
 */
void xy_gui_event_system_deinit(void);

/**
 * @brief 初始化事件队列
 * @param queue 事件队列指针
 * @return 0 成功，负值失败
 */
int xy_gui_event_queue_init(xy_gui_event_queue_t *queue);

/**
 * @brief 推送事件到队列
 * @param queue 事件队列指针
 * @param event 事件指针
 * @return 0 成功，-1 队列满
 */
int xy_gui_event_push(xy_gui_event_queue_t *queue, const xy_gui_event_t *event);

/**
 * @brief 从队列弹出事件
 * @param queue 事件队列指针
 * @param event 事件指针 (输出)
 * @return 0 成功，-1 队列空
 */
int xy_gui_event_pop(xy_gui_event_queue_t *queue, xy_gui_event_t *event);

/**
 * @brief 获取队列中事件数量
 * @param queue 事件队列指针
 * @return 事件数量
 */
uint16_t xy_gui_event_queue_count(xy_gui_event_queue_t *queue);

/**
 * @brief 清空事件队列
 * @param queue 事件队列指针
 */
void xy_gui_event_queue_clear(xy_gui_event_queue_t *queue);

/**
 * @brief 创建事件
 * @param type 事件类型
 * @param x X 坐标
 * @param y Y 坐标
 * @return 初始化后的事件
 */
xy_gui_event_t xy_gui_event_create_touch(xy_gui_event_type_t type, int16_t x, int16_t y);

/**
 * @brief 创建按键事件
 * @param key 按键码
 * @param is_press true=按下，false=释放
 * @return 初始化后的事件
 */
xy_gui_event_t xy_gui_event_create_key(xy_gui_key_t key, bool is_press);

/**
 * @brief 创建点击事件
 * @param x X 坐标
 * @param y Y 坐标
 * @return 初始化后的事件
 */
xy_gui_event_t xy_gui_event_create_click(int16_t x, int16_t y);

/**
 * @brief 创建值改变事件
 * @param value 新值
 * @return 初始化后的事件
 */
xy_gui_event_t xy_gui_event_create_value_changed(int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_EVENT_H */
