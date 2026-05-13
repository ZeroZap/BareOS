/**
 * @file xy_json.h
 * @brief JSON Parser Interface
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_JSON_H
#define XY_JSON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 类型定义 ==================== */

/**
 * @brief JSON 值类型
 */
typedef enum {
    XY_JSON_NULL = 0,
    XY_JSON_BOOL,
    XY_JSON_NUMBER,
    XY_JSON_STRING,
    XY_JSON_ARRAY,
    XY_JSON_OBJECT,
} xy_json_type_t;

/**
 * @brief JSON 值结构
 */
typedef struct xy_json {
    xy_json_type_t type;      /**< 值类型 */
    char *key;                /**< 键名 (对象成员) */
    
    union {
        bool bool_val;        /**< 布尔值 */
        double number_val;    /**< 数值 */
        char *string_val;     /**< 字符串 */
        struct {              /**< 数组 */
            struct xy_json **items;
            uint16_t count;
        } array;
        struct {              /**< 对象 */
            struct xy_json **members;
            uint16_t count;
        } object;
    } value;
    
    struct xy_json *next;     /**< 下一节点 (用于遍历) */
} xy_json_t;

/**
 * @brief JSON 解析器
 */
typedef struct {
    const char *json;         /**< JSON 字符串 */
    size_t pos;               /**< 当前位置 */
    size_t len;               /**< 总长度 */
    char error[64];           /**< 错误信息 */
} xy_json_parser_t;

/* ==================== 状态码 ==================== */

typedef enum {
    XY_JSON_OK = 0,
    XY_JSON_ERROR,
    XY_JSON_ERROR_INVALID_PARAM,
    XY_JSON_ERROR_PARSE,
    XY_JSON_ERROR_NOT_FOUND,
    XY_JSON_ERROR_NO_MEMORY,
} xy_json_status_t;

/* ==================== 核心 API ==================== */

/**
 * @brief 解析 JSON 字符串
 * @param json_str JSON 字符串
 * @return 解析后的 JSON 对象，NULL 表示失败
 */
xy_json_t* xy_json_parse(const char *json_str);

/**
 * @brief 释放 JSON 对象
 * @param json JSON 对象
 */
void xy_json_free(xy_json_t *json);

/**
 * @brief JSON 对象转字符串
 * @param json JSON 对象
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 实际长度
 */
size_t xy_json_stringify(xy_json_t *json, char *buffer, size_t size);

/* ==================== 对象操作 API ==================== */

/**
 * @brief 查找对象成员
 * @param obj JSON 对象
 * @param key 键名
 * @return 找到的值，NULL 表示未找到
 */
xy_json_t* xy_json_object_get(xy_json_t *obj, const char *key);

/**
 * @brief 设置对象成员
 * @param obj JSON 对象
 * @param key 键名
 * @param value 值
 * @return 状态码
 */
xy_json_status_t xy_json_object_set(xy_json_t *obj, const char *key, xy_json_t *value);

/**
 * @brief 删除对象成员
 * @param obj JSON 对象
 * @param key 键名
 * @return 状态码
 */
xy_json_status_t xy_json_object_delete(xy_json_t *obj, const char *key);

/**
 * @brief 遍历对象成员
 * @param obj JSON 对象
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void xy_json_object_foreach(xy_json_t *obj, 
                            void (*callback)(const char *key, xy_json_t *val, void *ud),
                            void *user_data);

/* ==================== 数组操作 API ==================== */

/**
 * @brief 获取数组元素
 * @param arr JSON 数组
 * @param index 索引
 * @return 元素值，NULL 表示越界
 */
xy_json_t* xy_json_array_get(xy_json_t *arr, uint16_t index);

/**
 * @brief 添加数组元素
 * @param arr JSON 数组
 * @param value 值
 * @return 状态码
 */
xy_json_status_t xy_json_array_append(xy_json_t *arr, xy_json_t *value);

/**
 * @brief 插入数组元素
 * @param arr JSON 数组
 * @param index 索引
 * @param value 值
 * @return 状态码
 */
xy_json_status_t xy_json_array_insert(xy_json_t *arr, uint16_t index, xy_json_t *value);

/**
 * @brief 删除数组元素
 * @param arr JSON 数组
 * @param index 索引
 * @return 状态码
 */
xy_json_status_t xy_json_array_remove(xy_json_t *arr, uint16_t index);

/**
 * @brief 获取数组长度
 * @param arr JSON 数组
 * @return 长度
 */
uint16_t xy_json_array_size(xy_json_t *arr);

/* ==================== 便捷 API ==================== */

/**
 * @brief 创建字符串值
 */
xy_json_t* xy_json_new_string(const char *str);

/**
 * @brief 创建数值值
 */
xy_json_t* xy_json_new_number(double num);

/**
 * @brief 创建布尔值
 */
xy_json_t* xy_json_new_bool(bool val);

/**
 * @brief 创建空对象
 */
xy_json_t* xy_json_new_object(void);

/**
 * @brief 创建空数组
 */
xy_json_t* xy_json_new_array(void);

/**
 * @brief 获取字符串值
 */
const char* xy_json_get_string(xy_json_t *json, const char *key, const char *default_val);

/**
 * @brief 获取数值值
 */
double xy_json_get_number(xy_json_t *json, const char *key, double default_val);

/**
 * @brief 获取布尔值
 */
bool xy_json_get_bool(xy_json_t *json, const char *key, bool default_val);

#ifdef __cplusplus
}
#endif

#endif /* XY_JSON_H */
