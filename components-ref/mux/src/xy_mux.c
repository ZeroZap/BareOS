/**
 * @file xy_mux.c
 * @brief Universal MUX Interface Library Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_mux.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== 外设类型字符串 ==================== */

static const char *g_mux_type_strings[] = {
    [XY_MUX_TYPE_NONE] = "NONE",
    [XY_MUX_TYPE_GPIO] = "GPIO",
    [XY_MUX_TYPE_UART] = "UART",
    [XY_MUX_TYPE_I2C] = "I2C",
    [XY_MUX_TYPE_SPI] = "SPI",
    [XY_MUX_TYPE_PWM] = "PWM",
    [XY_MUX_TYPE_ADC] = "ADC",
    [XY_MUX_TYPE_DAC] = "DAC",
    [XY_MUX_TYPE_SENSOR] = "SENSOR",
    [XY_MUX_TYPE_LOG] = "LOG",
    [XY_MUX_TYPE_CONFIG] = "CONFIG",
};

/* ==================== 核心实现 ==================== */

int32_t xy_mux_init(xy_mux_manager_t *mgr, 
                    uint8_t *tx_buffer, uint8_t *rx_buffer,
                    size_t buffer_size)
{
    if (!mgr || !tx_buffer || !rx_buffer || buffer_size == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    memset(mgr, 0, sizeof(*mgr));
    mgr->devices = NULL;
    mgr->device_count = 0;
    mgr->max_packet_size = 256;
    mgr->tx_buffer = tx_buffer;
    mgr->rx_buffer = rx_buffer;
    mgr->buffer_size = buffer_size;
    
    xy_log_i("MUX manager initialized (buffer=%d bytes)\n", buffer_size);
    return XY_MUX_OK;
}

int32_t xy_mux_deinit(xy_mux_manager_t *mgr)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    /* 注销所有设备 */
    xy_mux_device_t *node = mgr->devices;
    while (node) {
        xy_mux_device_t *next = node->next;
        if (node->ops && node->ops->deinit) {
            node->ops->deinit(node->channel);
        }
        free(node);
        node = next;
    }
    
    mgr->devices = NULL;
    mgr->device_count = 0;
    
    xy_log_i("MUX manager deinitialized\n");
    return XY_MUX_OK;
}

int32_t xy_mux_register(xy_mux_manager_t *mgr,
                        xy_mux_type_t type, uint8_t channel,
                        const xy_mux_ops_t *ops, void *user_data)
{
    if (!mgr || !ops) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已注册 */
    xy_mux_device_t *node = mgr->devices;
    while (node) {
        if (node->type == type && node->channel == channel) {
            xy_log_w("MUX device already registered: type=%d, channel=%d\n",
                     type, channel);
            return XY_MUX_ERROR_BUSY;
        }
        node = node->next;
    }
    
    /* 分配新节点 */
    node = (xy_mux_device_t *)malloc(sizeof(xy_mux_device_t));
    if (!node) {
        return XY_MUX_ERROR_NO_MEMORY;
    }
    
    memset(node, 0, sizeof(*node));
    node->type = type;
    node->channel = channel;
    node->ops = ops;
    node->user_data = user_data;
    node->enabled = true;
    
    /* 插入链表头部 */
    node->next = mgr->devices;
    mgr->devices = node;
    mgr->device_count++;
    
    xy_log_i("MUX device registered: type=%s, channel=%d\n",
             xy_mux_type_to_string(type), channel);
    
    /* 调用初始化回调 */
    if (ops->init) {
        int32_t ret = ops->init(channel, NULL);
        if (ret != XY_MUX_OK) {
            xy_log_e("MUX device init failed: %d\n", ret);
        }
    }
    
    return XY_MUX_OK;
}

int32_t xy_mux_unregister(xy_mux_manager_t *mgr,
                          xy_mux_type_t type, uint8_t channel)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    xy_mux_device_t *prev = NULL;
    xy_mux_device_t *node = mgr->devices;
    
    while (node) {
        if (node->type == type && node->channel == channel) {
            /* 调用反初始化回调 */
            if (node->ops && node->ops->deinit) {
                node->ops->deinit(node->channel);
            }
            
            /* 从链表移除 */
            if (prev) {
                prev->next = node->next;
            } else {
                mgr->devices = node->next;
            }
            
            free(node);
            mgr->device_count--;
            
            xy_log_i("MUX device unregistered: type=%s, channel=%d\n",
                     xy_mux_type_to_string(type), channel);
            
            return XY_MUX_OK;
        }
        prev = node;
        node = node->next;
    }
    
    return XY_MUX_ERROR_NO_DEVICE;
}

xy_mux_device_t* xy_mux_find(xy_mux_manager_t *mgr,
                             xy_mux_type_t type, uint8_t channel)
{
    if (!mgr) {
        return NULL;
    }
    
    xy_mux_device_t *node = mgr->devices;
    while (node) {
        if (node->type == type && node->channel == channel) {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}

int32_t xy_mux_process_packet(xy_mux_manager_t *mgr,
                              const uint8_t *packet, size_t len)
{
    if (!mgr || !packet || len < sizeof(xy_mux_header_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    const xy_mux_header_t *header = (const xy_mux_header_t *)packet;
    const uint8_t *data = packet + sizeof(xy_mux_header_t);
    size_t data_len = len - sizeof(xy_mux_header_t);
    
    xy_log_d("MUX packet received: type=%d, channel=%d, len=%d\n",
             header->type, header->channel, header->length);
    
    /* 查找对应设备 */
    xy_mux_device_t *device = xy_mux_find(mgr, (xy_mux_type_t)header->type, 
                                          header->channel);
    if (!device) {
        xy_log_w("MUX device not found: type=%d, channel=%d\n",
                 header->type, header->channel);
        return XY_MUX_ERROR_NO_DEVICE;
    }
    
    if (!device->enabled) {
        xy_log_w("MUX device disabled: type=%d, channel=%d\n",
                 header->type, header->channel);
        return XY_MUX_ERROR;
    }
    
    /* 调用设备写回调 */
    if (device->ops && device->ops->write) {
        return device->ops->write(device->channel, data, data_len);
    }
    
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

int32_t xy_mux_build_packet(xy_mux_manager_t *mgr,
                            xy_mux_type_t type, uint8_t channel,
                            const void *data, size_t len,
                            uint8_t *out_packet, size_t *out_len)
{
    if (!mgr || !out_packet || !out_len) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    size_t total_len = sizeof(xy_mux_header_t) + len;
    if (total_len > mgr->buffer_size) {
        xy_log_e("Packet too large: %d > %d\n", total_len, mgr->buffer_size);
        return XY_MUX_ERROR_NO_MEMORY;
    }
    
    /* 构建包头 */
    xy_mux_header_t *header = (xy_mux_header_t *)out_packet;
    header->type = type;
    header->channel = channel;
    header->length = len;
    
    /* 复制数据 */
    if (data && len > 0) {
        memcpy(out_packet + sizeof(xy_mux_header_t), data, len);
    }
    
    *out_len = total_len;
    
    xy_log_d("MUX packet built: type=%s, channel=%d, len=%d\n",
             xy_mux_type_to_string(type), channel, total_len);
    
    return XY_MUX_OK;
}

int32_t xy_mux_read(xy_mux_manager_t *mgr,
                    xy_mux_type_t type, uint8_t channel,
                    void *data, size_t len)
{
    if (!mgr || !data) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    xy_mux_device_t *device = xy_mux_find(mgr, type, channel);
    if (!device) {
        return XY_MUX_ERROR_NO_DEVICE;
    }
    
    if (!device->enabled) {
        return XY_MUX_ERROR;
    }
    
    if (!device->ops || !device->ops->read) {
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
    
    return device->ops->read(channel, data, len);
}

int32_t xy_mux_write(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     const void *data, size_t len)
{
    if (!mgr || !data) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    xy_mux_device_t *device = xy_mux_find(mgr, type, channel);
    if (!device) {
        return XY_MUX_ERROR_NO_DEVICE;
    }
    
    if (!device->enabled) {
        return XY_MUX_ERROR;
    }
    
    if (!device->ops || !device->ops->write) {
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
    
    return device->ops->write(channel, data, len);
}

int32_t xy_mux_ioctl(xy_mux_manager_t *mgr,
                     xy_mux_type_t type, uint8_t channel,
                     int cmd, void *arg)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    
    xy_mux_device_t *device = xy_mux_find(mgr, type, channel);
    if (!device) {
        return XY_MUX_ERROR_NO_DEVICE;
    }
    
    if (!device->enabled) {
        return XY_MUX_ERROR;
    }
    
    if (!device->ops || !device->ops->ioctl) {
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
    
    return device->ops->ioctl(channel, cmd, arg);
}

/* ==================== 辅助函数 ==================== */

const char* xy_mux_type_to_string(xy_mux_type_t type)
{
    if (type >= 0 && type < sizeof(g_mux_type_strings) / sizeof(g_mux_type_strings[0])) {
        return g_mux_type_strings[type] ? g_mux_type_strings[type] : "UNKNOWN";
    }
    return "CUSTOM";
}

xy_mux_type_t xy_mux_string_to_type(const char *str)
{
    if (!str) {
        return XY_MUX_TYPE_NONE;
    }
    
    for (size_t i = 0; i < sizeof(g_mux_type_strings) / sizeof(g_mux_type_strings[0]); i++) {
        if (g_mux_type_strings[i] && strcmp(str, g_mux_type_strings[i]) == 0) {
            return (xy_mux_type_t)i;
        }
    }
    
    return XY_MUX_TYPE_CUSTOM;
}

uint16_t xy_mux_get_device_count(xy_mux_manager_t *mgr)
{
    if (!mgr) {
        return 0;
    }
    return mgr->device_count;
}

uint16_t xy_mux_get_device_list(xy_mux_manager_t *mgr,
                                xy_mux_device_t **devices,
                                uint16_t max_count)
{
    if (!mgr || !devices || max_count == 0) {
        return 0;
    }
    
    uint16_t count = 0;
    xy_mux_device_t *node = mgr->devices;
    
    while (node && count < max_count) {
        devices[count++] = node;
        node = node->next;
    }
    
    return count;
}
