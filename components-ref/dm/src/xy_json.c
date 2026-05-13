/**
 * @file xy_json.c
 * @brief JSON Parser Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

/* ==================== 内部函数 ==================== */

static void skip_whitespace(xy_json_parser_t *parser)
{
    while (parser->pos < parser->len) {
        char c = parser->json[parser->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            parser->pos++;
        } else {
            break;
        }
    }
}

static char peek(xy_json_parser_t *parser)
{
    if (parser->pos >= parser->len) {
        return '\0';
    }
    return parser->json[parser->pos];
}

static char advance(xy_json_parser_t *parser)
{
    if (parser->pos >= parser->len) {
        return '\0';
    }
    return parser->json[parser->pos++];
}

static bool match(xy_json_parser_t *parser, const char *str)
{
    size_t len = strlen(str);
    if (parser->pos + len > parser->len) {
        return false;
    }
    
    if (strncmp(&parser->json[parser->pos], str, len) == 0) {
        parser->pos += len;
        return true;
    }
    return false;
}

static char* parse_string_raw(xy_json_parser_t *parser)
{
    if (advance(parser) != '"') {
        return NULL;
    }
    
    size_t start = parser->pos;
    while (parser->pos < parser->len && parser->json[parser->pos] != '"') {
        if (parser->json[parser->pos] == '\\') {
            parser->pos++;
        }
        parser->pos++;
    }
    
    size_t len = parser->pos - start;
    char *str = (char *)malloc(len + 1);
    if (!str) {
        return NULL;
    }
    
    memcpy(str, &parser->json[start], len);
    str[len] = '\0';
    
    /* 跳过结束引号 */
    advance(parser);
    
    return str;
}

static xy_json_t* parse_value(xy_json_parser_t *parser);

static xy_json_t* parse_object(xy_json_parser_t *parser)
{
    if (advance(parser) != '{') {
        return NULL;
    }
    
    xy_json_t *obj = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!obj) {
        return NULL;
    }
    
    obj->type = XY_JSON_OBJECT;
    
    skip_whitespace(parser);
    
    /* 空对象 */
    if (peek(parser) == '}') {
        advance(parser);
        return obj;
    }
    
    /* 解析成员 */
    while (1) {
        skip_whitespace(parser);
        
        /* 解析键 */
        char *key = parse_string_raw(parser);
        if (!key) {
            free(obj);
            return NULL;
        }
        
        skip_whitespace(parser);
        
        /* 跳过冒号 */
        if (advance(parser) != ':') {
            free(key);
            free(obj);
            return NULL;
        }
        
        skip_whitespace(parser);
        
        /* 解析值 */
        xy_json_t *value = parse_value(parser);
        if (!value) {
            free(key);
            free(obj);
            return NULL;
        }
        
        value->key = key;
        
        /* 添加到对象 */
        obj->value.object.count++;
        obj->value.object.members = (xy_json_t **)realloc(
            obj->value.object.members,
            obj->value.object.count * sizeof(xy_json_t *)
        );
        obj->value.object.members[obj->value.object.count - 1] = value;
        
        skip_whitespace(parser);
        
        /* 检查是否有下一个成员 */
        if (peek(parser) == '}') {
            advance(parser);
            break;
        }
        
        if (advance(parser) != ',') {
            free(obj);
            return NULL;
        }
    }
    
    return obj;
}

static xy_json_t* parse_array(xy_json_parser_t *parser)
{
    if (advance(parser) != '[') {
        return NULL;
    }
    
    xy_json_t *arr = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!arr) {
        return NULL;
    }
    
    arr->type = XY_JSON_ARRAY;
    
    skip_whitespace(parser);
    
    /* 空数组 */
    if (peek(parser) == ']') {
        advance(parser);
        return arr;
    }
    
    /* 解析元素 */
    while (1) {
        skip_whitespace(parser);
        
        xy_json_t *item = parse_value(parser);
        if (!item) {
            free(arr);
            return NULL;
        }
        
        /* 添加到数组 */
        arr->value.array.count++;
        arr->value.array.items = (xy_json_t **)realloc(
            arr->value.array.items,
            arr->value.array.count * sizeof(xy_json_t *)
        );
        arr->value.array.items[arr->value.array.count - 1] = item;
        
        skip_whitespace(parser);
        
        /* 检查是否有下一个元素 */
        if (peek(parser) == ']') {
            advance(parser);
            break;
        }
        
        if (advance(parser) != ',') {
            free(arr);
            return NULL;
        }
    }
    
    return arr;
}

static xy_json_t* parse_value(xy_json_parser_t *parser)
{
    skip_whitespace(parser);
    
    char c = peek(parser);
    
    /* 字符串 */
    if (c == '"') {
        xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
        if (!json) return NULL;
        
        json->type = XY_JSON_STRING;
        json->value.string_val = parse_string_raw(parser);
        if (!json->value.string_val) {
            free(json);
            return NULL;
        }
        return json;
    }
    
    /* 对象 */
    if (c == '{') {
        return parse_object(parser);
    }
    
    /* 数组 */
    if (c == '[') {
        return parse_array(parser);
    }
    
    /* 布尔值 true */
    if (match(parser, "true")) {
        xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
        if (!json) return NULL;
        json->type = XY_JSON_BOOL;
        json->value.bool_val = true;
        return json;
    }
    
    /* 布尔值 false */
    if (match(parser, "false")) {
        xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
        if (!json) return NULL;
        json->type = XY_JSON_BOOL;
        json->value.bool_val = false;
        return json;
    }
    
    /* null */
    if (match(parser, "null")) {
        xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
        if (!json) return NULL;
        json->type = XY_JSON_NULL;
        return json;
    }
    
    /* 数值 */
    if (c == '-' || (c >= '0' && c <= '9')) {
        xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
        if (!json) return NULL;
        
        json->type = XY_JSON_NUMBER;
        
        size_t start = parser->pos;
        if (c == '-') parser->pos++;
        
        while (parser->pos < parser->len) {
            char ch = parser->json[parser->pos];
            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E' || 
                ch == '+' || ch == '-') {
                parser->pos++;
            } else {
                break;
            }
        }
        
        char num_str[64];
        size_t len = parser->pos - start;
        if (len >= sizeof(num_str)) len = sizeof(num_str) - 1;
        
        memcpy(num_str, &parser->json[start], len);
        num_str[len] = '\0';
        
        json->value.number_val = atof(num_str);
        return json;
    }
    
    return NULL;
}

/* ==================== 核心 API 实现 ==================== */

/**
 * @brief 解析 JSON 字符串
 */
xy_json_t* xy_json_parse(const char *json_str)
{
    if (!json_str) {
        return NULL;
    }
    
    xy_json_parser_t parser = {0};
    parser.json = json_str;
    parser.len = strlen(json_str);
    
    xy_json_t *result = parse_value(&parser);
    
    if (!result) {
        xy_log_e("JSON parse failed\n");
    }
    
    return result;
}

/**
 * @brief 释放 JSON 对象
 */
void xy_json_free(xy_json_t *json)
{
    if (!json) return;
    
    /* 释放键名 */
    if (json->key) {
        free(json->key);
    }
    
    /* 根据类型释放值 */
    switch (json->type) {
        case XY_JSON_STRING:
            if (json->value.string_val) {
                free(json->value.string_val);
            }
            break;
            
        case XY_JSON_ARRAY:
            /* 递归释放数组元素 */
            if (json->value.array.items) {
                for (uint16_t i = 0; i < json->value.array.count; i++) {
                    if (json->value.array.items[i]) {
                        xy_json_free(json->value.array.items[i]);
                    }
                }
                free(json->value.array.items);
            }
            break;

        case XY_JSON_OBJECT:
            /* 递归释放对象成员 */
            if (json->value.object.members) {
                for (uint16_t i = 0; i < json->value.object.count; i++) {
                    if (json->value.object.members[i]) {
                        xy_json_free(json->value.object.members[i]);
                    }
                }
                free(json->value.object.members);
            }
            break;
            
        default:
            break;
    }
    
    free(json);
}

/**
 * @brief JSON 对象转字符串
 */
size_t xy_json_stringify(xy_json_t *json, char *buffer, size_t size)
{
    if (!json || !buffer || size == 0) {
        return 0;
    }
    
    /* 简化实现 */
    snprintf(buffer, size, "{}");
    return strlen(buffer);
}

/* ==================== 对象操作 API 实现 ==================== */

/**
 * @brief 查找对象成员
 */
xy_json_t* xy_json_object_get(xy_json_t *obj, const char *key)
{
    if (!obj || obj->type != XY_JSON_OBJECT || !key) {
        return NULL;
    }
    
    /* 遍历成员查找匹配键 */
    for (uint16_t i = 0; i < obj->value.object.count; i++) {
        xy_json_t *member = obj->value.object.members[i];
        if (member && member->key && strcmp(member->key, key) == 0) {
            return member;
        }
    }

    return NULL;
}

/**
 * @brief 设置对象成员
 */
xy_json_status_t xy_json_object_set(xy_json_t *obj, const char *key, xy_json_t *value)
{
    if (!obj || obj->type != XY_JSON_OBJECT || !key || !value) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    value->key = strdup(key);
    
    obj->value.object.count++;
    obj->value.object.members = (xy_json_t **)realloc(
        obj->value.object.members,
        obj->value.object.count * sizeof(xy_json_t *)
    );
    obj->value.object.members[obj->value.object.count - 1] = value;
    
    return XY_JSON_OK;
}

/**
 * @brief 删除对象成员
 */
xy_json_status_t xy_json_object_delete(xy_json_t *obj, const char *key)
{
    if (!obj || obj->type != XY_JSON_OBJECT || !key) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    /* 查找并删除 */
    for (uint16_t i = 0; i < obj->value.object.count; i++) {
        if (strcmp(obj->value.object.members[i]->key, key) == 0) {
            xy_json_free(obj->value.object.members[i]);
            
            /* 移动后续元素 */
            for (uint16_t j = i; j < obj->value.object.count - 1; j++) {
                obj->value.object.members[j] = obj->value.object.members[j + 1];
            }
            
            obj->value.object.count--;
            return XY_JSON_OK;
        }
    }
    
    return XY_JSON_ERROR_NOT_FOUND;
}

/**
 * @brief 遍历对象成员
 */
void xy_json_object_foreach(xy_json_t *obj,
                            void (*callback)(const char *key, xy_json_t *val, void *ud),
                            void *user_data)
{
    if (!obj || obj->type != XY_JSON_OBJECT || !callback) {
        return;
    }
    
    for (uint16_t i = 0; i < obj->value.object.count; i++) {
        xy_json_t *member = obj->value.object.members[i];
        callback(member->key, member, user_data);
    }
}

/* ==================== 数组操作 API 实现 ==================== */

/**
 * @brief 获取数组元素
 */
xy_json_t* xy_json_array_get(xy_json_t *arr, uint16_t index)
{
    if (!arr || arr->type != XY_JSON_ARRAY) {
        return NULL;
    }
    
    /* 边界检查 */
    if (index >= arr->value.array.count) {
        return NULL;
    }

    return arr->value.array.items[index];
}

/**
 * @brief 添加数组元素
 */
xy_json_status_t xy_json_array_append(xy_json_t *arr, xy_json_t *value)
{
    if (!arr || arr->type != XY_JSON_ARRAY || !value) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    arr->value.array.count++;
    arr->value.array.items = (xy_json_t **)realloc(
        arr->value.array.items,
        arr->value.array.count * sizeof(xy_json_t *)
    );
    arr->value.array.items[arr->value.array.count - 1] = value;
    
    return XY_JSON_OK;
}

/**
 * @brief 插入数组元素
 */
xy_json_status_t xy_json_array_insert(xy_json_t *arr, uint16_t index, xy_json_t *value)
{
    if (!arr || arr->type != XY_JSON_ARRAY || !value) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    if (index > arr->value.array.count) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    arr->value.array.count++;
    arr->value.array.items = (xy_json_t **)realloc(
        arr->value.array.items,
        arr->value.array.count * sizeof(xy_json_t *)
    );
    
    /* 移动后续元素 */
    for (uint16_t i = arr->value.array.count - 1; i > index; i--) {
        arr->value.array.items[i] = arr->value.array.items[i - 1];
    }
    
    arr->value.array.items[index] = value;
    
    return XY_JSON_OK;
}

/**
 * @brief 删除数组元素
 */
xy_json_status_t xy_json_array_remove(xy_json_t *arr, uint16_t index)
{
    if (!arr || arr->type != XY_JSON_ARRAY) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    if (index >= arr->value.array.count) {
        return XY_JSON_ERROR_INVALID_PARAM;
    }
    
    xy_json_free(arr->value.array.items[index]);
    
    /* 移动后续元素 */
    for (uint16_t i = index; i < arr->value.array.count - 1; i++) {
        arr->value.array.items[i] = arr->value.array.items[i + 1];
    }
    
    arr->value.array.count--;
    return XY_JSON_OK;
}

/**
 * @brief 获取数组长度
 */
uint16_t xy_json_array_size(xy_json_t *arr)
{
    if (!arr || arr->type != XY_JSON_ARRAY) {
        return 0;
    }
    return arr->value.array.count;
}

/* ==================== 便捷 API 实现 ==================== */

xy_json_t* xy_json_new_string(const char *str)
{
    if (!str) return NULL;
    
    xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!json) return NULL;
    
    json->type = XY_JSON_STRING;
    json->value.string_val = strdup(str);
    
    return json;
}

xy_json_t* xy_json_new_number(double num)
{
    xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!json) return NULL;
    
    json->type = XY_JSON_NUMBER;
    json->value.number_val = num;
    
    return json;
}

xy_json_t* xy_json_new_bool(bool val)
{
    xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!json) return NULL;
    
    json->type = XY_JSON_BOOL;
    json->value.bool_val = val;
    
    return json;
}

xy_json_t* xy_json_new_object(void)
{
    xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!json) return NULL;
    
    json->type = XY_JSON_OBJECT;
    return json;
}

xy_json_t* xy_json_new_array(void)
{
    xy_json_t *json = (xy_json_t *)calloc(1, sizeof(xy_json_t));
    if (!json) return NULL;
    
    json->type = XY_JSON_ARRAY;
    return json;
}

const char* xy_json_get_string(xy_json_t *json, const char *key, const char *default_val)
{
    if (!json) return default_val;
    
    xy_json_t *val = json->type == XY_JSON_OBJECT ? xy_json_object_get(json, key) : json;
    if (!val || val->type != XY_JSON_STRING) return default_val;
    
    return val->value.string_val;
}

double xy_json_get_number(xy_json_t *json, const char *key, double default_val)
{
    if (!json) return default_val;
    
    xy_json_t *val = json->type == XY_JSON_OBJECT ? xy_json_object_get(json, key) : json;
    if (!val || val->type != XY_JSON_NUMBER) return default_val;
    
    return val->value.number_val;
}

bool xy_json_get_bool(xy_json_t *json, const char *key, bool default_val)
{
    if (!json) return default_val;
    
    xy_json_t *val = json->type == XY_JSON_OBJECT ? xy_json_object_get(json, key) : json;
    if (!val || val->type != XY_JSON_BOOL) return default_val;
    
    return val->value.bool_val;
}
