/**
 * @file xy_broker.c
 * @brief XY Broker System Implementation
 */

#include "xy_broker.h"
#include <string.h>

/* ==================== Internal Data Structures ==================== */

static struct {
    xy_broker_server_t servers[XY_BROKER_MAX_SERVERS];
    xy_broker_topic_t topics[XY_BROKER_MAX_TOPICS];
    xy_broker_stats_t stats;
    uint16_t seq_counter;
    uint8_t initialized;
} g_broker;

/* ==================== Internal Functions ==================== */

/**
 * @brief 获取时间戳 (毫秒)
 * 
 * 平台适配说明:
 * - 使用 xy_os_tick_get() 获取系统 tick
 * - 如需毫秒精度，可在 xy_os_cfg.h 配置 XY_OS_TICK_RATE
 * - 替代方案：使用硬件定时器/RTC
 */
static uint32_t broker_get_timestamp(void)
{
    /* 使用 OS tick 作为时间戳 */
    return xy_os_tick_get();
}

/**
 * @brief Find server by ID
 */
static xy_broker_server_t *broker_find_server(uint16_t server_id)
{
    for (int i = 0; i < XY_BROKER_MAX_SERVERS; i++) {
        if (g_broker.servers[i].active
            && g_broker.servers[i].server_id == server_id) {
            return &g_broker.servers[i];
        }
    }
    return NULL;
}

/**
 * @brief Find free server slot
 */
static xy_broker_server_t *broker_alloc_server(void)
{
    for (int i = 0; i < XY_BROKER_MAX_SERVERS; i++) {
        if (!g_broker.servers[i].active) {
            return &g_broker.servers[i];
        }
    }
    return NULL;
}

/**
 * @brief Find topic by ID
 */
static xy_broker_topic_t *broker_find_topic(uint16_t topic_id)
{
    for (int i = 0; i < XY_BROKER_MAX_TOPICS; i++) {
        if (g_broker.topics[i].topic_id == topic_id
            && g_broker.topics[i].subscriber_count > 0) {
            return &g_broker.topics[i];
        }
    }
    return NULL;
}

/**
 * @brief Find free topic slot
 */
static xy_broker_topic_t *broker_alloc_topic(void)
{
    for (int i = 0; i < XY_BROKER_MAX_TOPICS; i++) {
        if (g_broker.topics[i].subscriber_count == 0) {
            return &g_broker.topics[i];
        }
    }
    return NULL;
}

/**
 * @brief Enqueue message to server
 */
static int broker_enqueue_msg(xy_broker_server_t *server,
                              const xy_broker_msg_t *msg)
{
    if (!server || !msg)
        return XY_BROKER_INVALID_PARAM;

    if (server->queue_count >= XY_BROKER_MSG_QUEUE_SIZE) {
        g_broker.stats.queue_overflow_count++;
        g_broker.stats.total_msg_dropped++;
        return XY_BROKER_QUEUE_FULL;
    }

    memcpy(
        &server->msg_queue[server->queue_tail], msg, sizeof(xy_broker_msg_t));
    server->queue_tail = (server->queue_tail + 1) % XY_BROKER_MSG_QUEUE_SIZE;
    server->queue_count++;
    server->msg_received++;

    return XY_BROKER_OK;
}

/**
 * @brief Dequeue message from server
 */
static int broker_dequeue_msg(xy_broker_server_t *server, xy_broker_msg_t *msg)
{
    if (!server || !msg)
        return XY_BROKER_INVALID_PARAM;

    if (server->queue_count == 0)
        return XY_BROKER_NOT_FOUND;

    memcpy(
        msg, &server->msg_queue[server->queue_head], sizeof(xy_broker_msg_t));
    server->queue_head = (server->queue_head + 1) % XY_BROKER_MSG_QUEUE_SIZE;
    server->queue_count--;

    return XY_BROKER_OK;
}

/* ==================== Core API Implementation ==================== */

int xy_broker_init(void)
{
    if (g_broker.initialized)
        return XY_BROKER_OK;

    memset(&g_broker, 0, sizeof(g_broker));
    g_broker.initialized = 1;

    return XY_BROKER_OK;
}

int xy_broker_deinit(void)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    memset(&g_broker, 0, sizeof(g_broker));

    return XY_BROKER_OK;
}

int xy_broker_register_server(uint16_t server_id,
                              xy_broker_msg_handler_t handler, void *user_data)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    if (server_id == 0)
        return XY_BROKER_INVALID_PARAM;

    // Check if already registered
    if (broker_find_server(server_id))
        return XY_BROKER_ALREADY_EXISTS;

    // Allocate server slot
    xy_broker_server_t *server = broker_alloc_server();
    if (!server)
        return XY_BROKER_NO_MEMORY;

    // Initialize server
    memset(server, 0, sizeof(xy_broker_server_t));
    server->server_id = server_id;
    server->handler   = handler;
    server->user_data = user_data;
    server->active    = 1;

    g_broker.stats.active_servers++;

    return XY_BROKER_OK;
}

int xy_broker_unregister_server(uint16_t server_id)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    xy_broker_server_t *server = broker_find_server(server_id);
    if (!server)
        return XY_BROKER_NOT_FOUND;

    server->active = 0;
    g_broker.stats.active_servers--;

    return XY_BROKER_OK;
}

int xy_broker_send_msg(uint16_t src_server, uint16_t dst_server,
                       uint16_t msg_id, const void *payload,
                       uint16_t payload_len, uint8_t priority)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    if (payload_len > XY_BROKER_MAX_MSG_SIZE)
        return XY_BROKER_INVALID_PARAM;

    xy_broker_server_t *dst = broker_find_server(dst_server);
    if (!dst)
        return XY_BROKER_NOT_FOUND;

    // Create message
    xy_broker_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_id      = msg_id;
    msg.src_server  = src_server;
    msg.dst_server  = dst_server;
    msg.priority    = priority;
    msg.seq_num     = g_broker.seq_counter++;
    msg.timestamp   = broker_get_timestamp();
    msg.payload_len = payload_len;

    if (payload && payload_len > 0) {
        memcpy(msg.payload, payload, payload_len);
    }

    // Enqueue message
    int ret = broker_enqueue_msg(dst, &msg);
    if (ret == XY_BROKER_OK) {
        g_broker.stats.total_msg_sent++;

        // Update source server stats if registered
        xy_broker_server_t *src = broker_find_server(src_server);
        if (src) {
            src->msg_sent++;
        }
    }

    return ret;
}

int xy_broker_process_msgs(uint16_t server_id, uint16_t max_msgs)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    xy_broker_server_t *server = broker_find_server(server_id);
    if (!server)
        return XY_BROKER_NOT_FOUND;

    if (!server->handler)
        return XY_BROKER_ERROR;

    int processed = 0;
    xy_broker_msg_t msg;

    while (server->queue_count > 0) {
        if (max_msgs > 0 && processed >= max_msgs)
            break;

        if (broker_dequeue_msg(server, &msg) == XY_BROKER_OK) {
            server->handler(&msg, server->user_data);
            g_broker.stats.total_msg_delivered++;
            processed++;
        }
    }

    return processed;
}

/* ==================== Pub/Sub API Implementation ==================== */

int xy_broker_create_topic(uint16_t topic_id)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    if (broker_find_topic(topic_id))
        return XY_BROKER_ALREADY_EXISTS;

    xy_broker_topic_t *topic = broker_alloc_topic();
    if (!topic)
        return XY_BROKER_NO_MEMORY;

    memset(topic, 0, sizeof(xy_broker_topic_t));
    topic->topic_id = topic_id;

    return XY_BROKER_OK;
}

int xy_broker_subscribe(uint16_t topic_id, uint16_t server_id,
                        xy_broker_msg_handler_t handler, void *user_data)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    if (!handler)
        return XY_BROKER_INVALID_PARAM;

    xy_broker_topic_t *topic = broker_find_topic(topic_id);
    if (!topic) {
        // Auto-create topic
        if (xy_broker_create_topic(topic_id) != XY_BROKER_OK)
            return XY_BROKER_NO_MEMORY;
        topic = broker_find_topic(topic_id);
    }

    // Check if already subscribed
    for (int i = 0; i < XY_BROKER_MAX_SUBSCRIBERS; i++) {
        if (topic->subscribers[i].active
            && topic->subscribers[i].server_id == server_id) {
            return XY_BROKER_ALREADY_EXISTS;
        }
    }

    // Find free subscriber slot
    for (int i = 0; i < XY_BROKER_MAX_SUBSCRIBERS; i++) {
        if (!topic->subscribers[i].active) {
            topic->subscribers[i].server_id = server_id;
            topic->subscribers[i].handler   = handler;
            topic->subscribers[i].user_data = user_data;
            topic->subscribers[i].active    = 1;
            topic->subscriber_count++;

            if (topic->subscriber_count == 1) {
                g_broker.stats.active_topics++;
            }

            return XY_BROKER_OK;
        }
    }

    return XY_BROKER_NO_MEMORY;
}

int xy_broker_unsubscribe(uint16_t topic_id, uint16_t server_id)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    xy_broker_topic_t *topic = broker_find_topic(topic_id);
    if (!topic)
        return XY_BROKER_NOT_FOUND;

    for (int i = 0; i < XY_BROKER_MAX_SUBSCRIBERS; i++) {
        if (topic->subscribers[i].active
            && topic->subscribers[i].server_id == server_id) {
            topic->subscribers[i].active = 0;
            topic->subscriber_count--;

            if (topic->subscriber_count == 0) {
                g_broker.stats.active_topics--;
            }

            return XY_BROKER_OK;
        }
    }

    return XY_BROKER_NOT_FOUND;
}

int xy_broker_publish(uint16_t src_server, uint16_t topic_id, uint16_t msg_id,
                      const void *payload, uint16_t payload_len,
                      uint8_t priority)
{
    if (!g_broker.initialized)
        return XY_BROKER_ERROR;

    if (payload_len > XY_BROKER_MAX_MSG_SIZE)
        return XY_BROKER_INVALID_PARAM;

    xy_broker_topic_t *topic = broker_find_topic(topic_id);
    if (!topic || topic->subscriber_count == 0)
        return XY_BROKER_NOT_FOUND;

    // Create message
    xy_broker_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_id      = msg_id;
    msg.src_server  = src_server;
    msg.topic_id    = topic_id;
    msg.priority    = priority;
    msg.seq_num     = g_broker.seq_counter++;
    msg.timestamp   = broker_get_timestamp();
    msg.payload_len = payload_len;
    msg.flags       = XY_BROKER_FLAG_BROADCAST;

    if (payload && payload_len > 0) {
        memcpy(msg.payload, payload, payload_len);
    }

    // Deliver to all subscribers
    int delivered = 0;
    for (int i = 0; i < XY_BROKER_MAX_SUBSCRIBERS; i++) {
        if (topic->subscribers[i].active) {
            if (topic->subscribers[i].handler) {
                // Direct callback
                topic->subscribers[i].handler(
                    &msg, topic->subscribers[i].user_data);
                delivered++;
            }
        }
    }

    if (delivered > 0) {
        topic->msg_count++;
        g_broker.stats.total_msg_sent++;
        g_broker.stats.total_msg_delivered += delivered;
    }

    return XY_BROKER_OK;
}

/* ==================== Request/Response API Implementation ====================
 */

int xy_broker_request(uint16_t src_server, uint16_t dst_server, uint16_t msg_id,
                      const void *request_payload, uint16_t request_len,
                      xy_broker_msg_t *response_msg, uint32_t timeout_ms)
{
    if (!g_broker.initialized || !response_msg)
        return XY_BROKER_ERROR;

    // Send request
    int ret =
        xy_broker_send_msg(src_server, dst_server, msg_id, request_payload,
                           request_len, XY_BROKER_PRIORITY_NORMAL);
    if (ret != XY_BROKER_OK)
        return ret;

    /* 等待响应 (轮询实现 + 超时机制) */
    uint32_t start_time = broker_get_timestamp();
    xy_broker_server_t *src = broker_find_server(src_server);

    /* 轮询检查响应队列 */
    while ((broker_get_timestamp() - start_time) < timeout_ms) {
        if (src && src->queue_count > 0) {
            if (broker_dequeue_msg(src, response_msg) == XY_BROKER_OK) {
                return XY_BROKER_OK;
            }
        }
        /* 短暂延时，避免忙等 */
        xy_os_delay(1);
    }

    return XY_BROKER_TIMEOUT;
}

int xy_broker_respond(const xy_broker_msg_t *request_msg,
                      const void *response_payload, uint16_t response_len)
{
    if (!g_broker.initialized || !request_msg)
        return XY_BROKER_ERROR;

    // Send response back to source
    return xy_broker_send_msg(request_msg->dst_server, request_msg->src_server,
                              request_msg->msg_id, response_payload,
                              response_len, request_msg->priority);
}

/* ==================== Utility API Implementation ==================== */

int xy_broker_get_stats(xy_broker_stats_t *stats)
{
    if (!g_broker.initialized || !stats)
        return XY_BROKER_ERROR;

    memcpy(stats, &g_broker.stats, sizeof(xy_broker_stats_t));
    return XY_BROKER_OK;
}

int xy_broker_is_server_registered(uint16_t server_id)
{
    return broker_find_server(server_id) != NULL ? 1 : 0;
}

int xy_broker_get_pending_count(uint16_t server_id)
{
    xy_broker_server_t *server = broker_find_server(server_id);
    if (!server)
        return XY_BROKER_NOT_FOUND;

    return server->queue_count;
}

int xy_broker_clear_queue(uint16_t server_id)
{
    xy_broker_server_t *server = broker_find_server(server_id);
    if (!server)
        return XY_BROKER_NOT_FOUND;

    server->queue_head  = 0;
    server->queue_tail  = 0;
    server->queue_count = 0;

    return XY_BROKER_OK;
}

/* ==================== Debug Helper Functions ==================== */

const char *xy_broker_get_server_name(uint16_t server_id)
{
    switch (server_id) {
    case XY_BROKER_SERVER_SYSTEM:
        return "SYSTEM";
    case XY_BROKER_SERVER_POWER:
        return "POWER";
    case XY_BROKER_SERVER_COMM:
        return "COMM";
    case XY_BROKER_SERVER_SENSOR:
        return "SENSOR";
    case XY_BROKER_SERVER_STORAGE:
        return "STORAGE";
    case XY_BROKER_SERVER_DISPLAY:
        return "DISPLAY";
    case XY_BROKER_SERVER_NETWORK:
        return "NETWORK";
    case XY_BROKER_SERVER_SECURITY:
        return "SECURITY";
    case XY_BROKER_SERVER_TIMER:
        return "TIMER";
    case XY_BROKER_SERVER_LOG:
        return "LOG";
    case XY_BROKER_SERVER_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

const char *xy_broker_get_msg_name(uint16_t msg_id)
{
    switch (msg_id) {
    case XY_BROKER_MSG_SYSTEM_INIT:
        return "SYSTEM_INIT";
    case XY_BROKER_MSG_SYSTEM_SHUTDOWN:
        return "SYSTEM_SHUTDOWN";
    case XY_BROKER_MSG_POWER_ON:
        return "POWER_ON";
    case XY_BROKER_MSG_POWER_OFF:
        return "POWER_OFF";
    case XY_BROKER_MSG_SENSOR_DATA:
        return "SENSOR_DATA";
    case XY_BROKER_MSG_COMM_SEND:
        return "COMM_SEND";
    case XY_BROKER_MSG_COMM_RECEIVE:
        return "COMM_RECEIVE";
    default:
        return "UNKNOWN";
    }
}

const char *xy_broker_get_topic_name(uint16_t topic_id)
{
    switch (topic_id) {
    case XY_BROKER_TOPIC_SYSTEM_EVENT:
        return "SYSTEM_EVENT";
    case XY_BROKER_TOPIC_POWER_EVENT:
        return "POWER_EVENT";
    case XY_BROKER_TOPIC_SENSOR_DATA:
        return "SENSOR_DATA";
    case XY_BROKER_TOPIC_NETWORK_EVENT:
        return "NETWORK_EVENT";
    case XY_BROKER_TOPIC_ALARM_EVENT:
        return "ALARM_EVENT";
    case XY_BROKER_TOPIC_LOG_EVENT:
        return "LOG_EVENT";
    default:
        return "UNKNOWN";
    }
}
