/**
 * @file xy_broker.h
 * @brief XY Broker System - Fixed-ID Message Distribution for Inter-Domain
 * Communication
 *
 * This broker system provides efficient message passing between different
 * system domains using fixed integer IDs instead of dynamic strings for better
 * performance and predictability in embedded systems.
 *
 * Features:
 * - Fixed Server IDs and Message IDs (no dynamic allocation)
 * - Topic-based publish/subscribe pattern
 * - Request/response pattern support
 * - Priority-based message delivery
 * - Lightweight and efficient for embedded systems
 *
 * @author XY Team
 * @date 2025
 */

#ifndef XY_BROKER_H
#define XY_BROKER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_BROKER_MAX_SERVERS
#define XY_BROKER_MAX_SERVERS 16 /**< Maximum number of broker servers */
#endif

#ifndef XY_BROKER_MAX_SUBSCRIBERS
#define XY_BROKER_MAX_SUBSCRIBERS 32 /**< Maximum subscribers per topic */
#endif

#ifndef XY_BROKER_MAX_TOPICS
#define XY_BROKER_MAX_TOPICS 64 /**< Maximum number of topics */
#endif

#ifndef XY_BROKER_MSG_QUEUE_SIZE
#define XY_BROKER_MSG_QUEUE_SIZE 32 /**< Message queue size per server */
#endif

#ifndef XY_BROKER_MAX_MSG_SIZE
#define XY_BROKER_MAX_MSG_SIZE 256 /**< Maximum message payload size */
#endif

/* ==================== Return Codes ==================== */

#define XY_BROKER_OK             0  /**< Success */
#define XY_BROKER_ERROR          -1 /**< General error */
#define XY_BROKER_INVALID_PARAM  -2 /**< Invalid parameter */
#define XY_BROKER_NO_MEMORY      -3 /**< Out of memory */
#define XY_BROKER_QUEUE_FULL     -4 /**< Message queue full */
#define XY_BROKER_NOT_FOUND      -5 /**< Server/topic not found */
#define XY_BROKER_TIMEOUT        -6 /**< Operation timeout */
#define XY_BROKER_ALREADY_EXISTS -7 /**< Already registered */

/* ==================== Fixed Server IDs ==================== */

/**
 * @brief Predefined broker server IDs
 *
 * These fixed IDs represent different system domains/services.
 * Add your domain-specific server IDs here.
 */
typedef enum {
    XY_BROKER_SERVER_SYSTEM    = 0x0001, /**< System management server */
    XY_BROKER_SERVER_POWER     = 0x0002, /**< Power management server */
    XY_BROKER_SERVER_COMM      = 0x0003, /**< Communication server */
    XY_BROKER_SERVER_SENSOR    = 0x0004, /**< Sensor data server */
    XY_BROKER_SERVER_STORAGE   = 0x0005, /**< Storage management server */
    XY_BROKER_SERVER_DISPLAY   = 0x0006, /**< Display server */
    XY_BROKER_SERVER_NETWORK   = 0x0007, /**< Network server */
    XY_BROKER_SERVER_SECURITY  = 0x0008, /**< Security/crypto server */
    XY_BROKER_SERVER_TIMER     = 0x0009, /**< Timer/alarm server */
    XY_BROKER_SERVER_LOG       = 0x000A, /**< Logging server */
    XY_BROKER_SERVER_DEBUG     = 0x000B, /**< Debug server */
    XY_BROKER_SERVER_USER_BASE = 0x0100, /**< User-defined servers start */
    XY_BROKER_SERVER_USER_END  = 0xFFFF  /**< User-defined servers end */
} xy_broker_server_id_t;

/* ==================== Fixed Message IDs ==================== */

/**
 * @brief Predefined message IDs
 *
 * Fixed message types for efficient routing without string comparison.
 */
typedef enum {
    /* System messages (0x0000 - 0x00FF) */
    XY_BROKER_MSG_SYSTEM_INIT     = 0x0001,
    XY_BROKER_MSG_SYSTEM_SHUTDOWN = 0x0002,
    XY_BROKER_MSG_SYSTEM_RESET    = 0x0003,
    XY_BROKER_MSG_SYSTEM_STATUS   = 0x0004,
    XY_BROKER_MSG_SYSTEM_CONFIG   = 0x0005,

    /* Power messages (0x0100 - 0x01FF) */
    XY_BROKER_MSG_POWER_ON      = 0x0101,
    XY_BROKER_MSG_POWER_OFF     = 0x0102,
    XY_BROKER_MSG_POWER_SLEEP   = 0x0103,
    XY_BROKER_MSG_POWER_WAKEUP  = 0x0104,
    XY_BROKER_MSG_POWER_BATTERY = 0x0105,

    /* Communication messages (0x0200 - 0x02FF) */
    XY_BROKER_MSG_COMM_SEND       = 0x0201,
    XY_BROKER_MSG_COMM_RECEIVE    = 0x0202,
    XY_BROKER_MSG_COMM_CONNECT    = 0x0203,
    XY_BROKER_MSG_COMM_DISCONNECT = 0x0204,
    XY_BROKER_MSG_COMM_STATUS     = 0x0205,

    /* Sensor messages (0x0300 - 0x03FF) */
    XY_BROKER_MSG_SENSOR_DATA      = 0x0301,
    XY_BROKER_MSG_SENSOR_CALIBRATE = 0x0302,
    XY_BROKER_MSG_SENSOR_CONFIG    = 0x0303,
    XY_BROKER_MSG_SENSOR_ALARM     = 0x0304,

    /* Storage messages (0x0400 - 0x04FF) */
    XY_BROKER_MSG_STORAGE_READ   = 0x0401,
    XY_BROKER_MSG_STORAGE_WRITE  = 0x0402,
    XY_BROKER_MSG_STORAGE_ERASE  = 0x0403,
    XY_BROKER_MSG_STORAGE_FORMAT = 0x0404,

    /* User-defined messages */
    XY_BROKER_MSG_USER_BASE = 0x1000,
    XY_BROKER_MSG_USER_END  = 0xFFFF
} xy_broker_msg_id_t;

/* ==================== Fixed Topic IDs ==================== */

/**
 * @brief Predefined topic IDs for pub/sub pattern
 *
 * These are provided as reference values. Users can define their own
 * topic IDs without being restricted to these constants.
 */
#define XY_BROKER_TOPIC_SYSTEM_EVENT  0x0001
#define XY_BROKER_TOPIC_POWER_EVENT   0x0002
#define XY_BROKER_TOPIC_SENSOR_DATA   0x0003
#define XY_BROKER_TOPIC_NETWORK_EVENT 0x0004
#define XY_BROKER_TOPIC_ALARM_EVENT   0x0005
#define XY_BROKER_TOPIC_LOG_EVENT     0x0006
#define XY_BROKER_TOPIC_USER_BASE     0x0100
#define XY_BROKER_TOPIC_USER_END      0xFFFF

/* ==================== Message Priority ==================== */

/**
 * @brief Message priority levels
 *
 * These are provided as reference values. Users can define their own
 * priority levels without being restricted to these constants.
 */
#define XY_BROKER_PRIORITY_LOW      0
#define XY_BROKER_PRIORITY_NORMAL   1
#define XY_BROKER_PRIORITY_HIGH     2
#define XY_BROKER_PRIORITY_CRITICAL 3

/* ==================== Message Flags ==================== */

#define XY_BROKER_FLAG_NONE         0x00
#define XY_BROKER_FLAG_ACK_REQUIRED 0x01 /**< Acknowledgment required */
#define XY_BROKER_FLAG_BROADCAST    0x02 /**< Broadcast to all subscribers */
#define XY_BROKER_FLAG_PERSISTENT   0x04 /**< Message should be persisted */
#define XY_BROKER_FLAG_ENCRYPTED    0x08 /**< Message is encrypted */

/* ==================== Data Structures ==================== */

/**
 * @brief Broker message structure
 */
typedef struct {
    uint16_t msg_id;                         /**< Message ID (fixed value) */
    uint16_t src_server;                     /**< Source server ID */
    uint16_t dst_server;                     /**< Destination server ID */
    uint16_t topic_id;                       /**< Topic ID (for pub/sub) */
    uint8_t priority;                        /**< Message priority */
    uint8_t flags;                           /**< Message flags */
    uint16_t seq_num;                        /**< Sequence number */
    uint32_t timestamp;                      /**< Timestamp (ms) */
    uint16_t payload_len;                    /**< Payload length */
    uint8_t payload[XY_BROKER_MAX_MSG_SIZE]; /**< Message payload */
} xy_broker_msg_t;

/**
 * @brief Message handler callback
 *
 * @param msg Received message
 * @param user_data User-provided context data
 * @return 0 on success, negative error code otherwise
 */
typedef int (*xy_broker_msg_handler_t)(const xy_broker_msg_t *msg,
                                       void *user_data);

/**
 * @brief Topic subscriber information
 */
typedef struct {
    uint16_t server_id;              /**< Subscriber server ID */
    xy_broker_msg_handler_t handler; /**< Message handler callback */
    void *user_data;                 /**< User context data */
    uint8_t active;                  /**< Subscription active flag */
} xy_broker_subscriber_t;

/**
 * @brief Topic information
 */
typedef struct {
    uint16_t topic_id; /**< Topic ID */
    xy_broker_subscriber_t subscribers[XY_BROKER_MAX_SUBSCRIBERS];
    uint8_t subscriber_count; /**< Number of active subscribers */
    uint32_t msg_count;       /**< Total messages published */
} xy_broker_topic_t;

/**
 * @brief Broker server structure
 */
typedef struct {
    uint16_t server_id;              /**< Server ID */
    xy_broker_msg_handler_t handler; /**< Default message handler */
    void *user_data;                 /**< User context data */
    xy_broker_msg_t msg_queue[XY_BROKER_MSG_QUEUE_SIZE];
    uint16_t queue_head;   /**< Queue head index */
    uint16_t queue_tail;   /**< Queue tail index */
    uint16_t queue_count;  /**< Messages in queue */
    uint8_t active;        /**< Server active flag */
    uint32_t msg_received; /**< Total messages received */
    uint32_t msg_sent;     /**< Total messages sent */
} xy_broker_server_t;

/**
 * @brief Broker statistics
 */
typedef struct {
    uint32_t total_msg_sent;       /**< Total messages sent */
    uint32_t total_msg_delivered;  /**< Total messages delivered */
    uint32_t total_msg_dropped;    /**< Total messages dropped */
    uint32_t queue_overflow_count; /**< Queue overflow count */
    uint32_t active_servers;       /**< Number of active servers */
    uint32_t active_topics;        /**< Number of active topics */
} xy_broker_stats_t;

/* ==================== Core API ==================== */

/**
 * @brief Initialize the broker system
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_init(void);

/**
 * @brief Deinitialize the broker system
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_deinit(void);

/**
 * @brief Register a broker server
 *
 * @param server_id Fixed server ID
 * @param handler Default message handler for this server
 * @param user_data User context data passed to handler
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_register_server(uint16_t server_id,
                              xy_broker_msg_handler_t handler, void *user_data);

/**
 * @brief Unregister a broker server
 *
 * @param server_id Server ID to unregister
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_unregister_server(uint16_t server_id);

/**
 * @brief Send a message to a specific server
 *
 * @param src_server Source server ID
 * @param dst_server Destination server ID
 * @param msg_id Message ID
 * @param payload Message payload
 * @param payload_len Payload length
 * @param priority Message priority
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_send_msg(uint16_t src_server, uint16_t dst_server,
                       uint16_t msg_id, const void *payload,
                       uint16_t payload_len, uint8_t priority);

/**
 * @brief Process pending messages for a server
 *
 * @param server_id Server ID to process messages for
 * @param max_msgs Maximum number of messages to process (0 = all)
 * @return Number of messages processed, or error code if negative
 */
int xy_broker_process_msgs(uint16_t server_id, uint16_t max_msgs);

/* ==================== Pub/Sub API ==================== */

/**
 * @brief Create a topic
 *
 * @param topic_id Fixed topic ID
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_create_topic(uint16_t topic_id);

/**
 * @brief Subscribe to a topic
 *
 * @param topic_id Topic ID to subscribe to
 * @param server_id Subscriber server ID
 * @param handler Message handler for this topic
 * @param user_data User context data
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_subscribe(uint16_t topic_id, uint16_t server_id,
                        xy_broker_msg_handler_t handler, void *user_data);

/**
 * @brief Unsubscribe from a topic
 *
 * @param topic_id Topic ID
 * @param server_id Subscriber server ID
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_unsubscribe(uint16_t topic_id, uint16_t server_id);

/**
 * @brief Publish a message to a topic
 *
 * @param src_server Source server ID
 * @param topic_id Topic ID
 * @param msg_id Message ID
 * @param payload Message payload
 * @param payload_len Payload length
 * @param priority Message priority
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_publish(uint16_t src_server, uint16_t topic_id, uint16_t msg_id,
                      const void *payload, uint16_t payload_len,
                      uint8_t priority);

/* ==================== Request/Response API ==================== */

/**
 * @brief Send a request and wait for response
 *
 * @param src_server Source server ID
 * @param dst_server Destination server ID
 * @param msg_id Message ID
 * @param request_payload Request payload
 * @param request_len Request length
 * @param response_msg Response message (output)
 * @param timeout_ms Timeout in milliseconds
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_request(uint16_t src_server, uint16_t dst_server, uint16_t msg_id,
                      const void *request_payload, uint16_t request_len,
                      xy_broker_msg_t *response_msg, uint32_t timeout_ms);

/**
 * @brief Send a response to a request
 *
 * @param request_msg Original request message
 * @param response_payload Response payload
 * @param response_len Response length
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_respond(const xy_broker_msg_t *request_msg,
                      const void *response_payload, uint16_t response_len);

/* ==================== Utility API ==================== */

/**
 * @brief Get broker statistics
 *
 * @param stats Statistics structure to fill (output)
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_get_stats(xy_broker_stats_t *stats);

/**
 * @brief Check if server is registered
 *
 * @param server_id Server ID to check
 * @return 1 if registered, 0 otherwise
 */
int xy_broker_is_server_registered(uint16_t server_id);

/**
 * @brief Get pending message count for a server
 *
 * @param server_id Server ID
 * @return Number of pending messages, or error code if negative
 */
int xy_broker_get_pending_count(uint16_t server_id);

/**
 * @brief Clear message queue for a server
 *
 * @param server_id Server ID
 * @return XY_BROKER_OK on success, error code otherwise
 */
int xy_broker_clear_queue(uint16_t server_id);

/**
 * @brief Get server name (for debugging)
 *
 * @param server_id Server ID
 * @return Server name string
 */
const char *xy_broker_get_server_name(uint16_t server_id);

/**
 * @brief Get message name (for debugging)
 *
 * @param msg_id Message ID
 * @return Message name string
 */
const char *xy_broker_get_msg_name(uint16_t msg_id);

/**
 * @brief Get topic name (for debugging)
 *
 * @param topic_id Topic ID
 * @return Topic name string
 */
const char *xy_broker_get_topic_name(uint16_t topic_id);

#ifdef __cplusplus
}
#endif

#endif /* XY_BROKER_H */
