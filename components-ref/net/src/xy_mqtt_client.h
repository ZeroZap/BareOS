/**
 * @file xy_mqtt_client.h
 * @brief MQTT 3.1.1 Client Implementation
 *
 * This file provides a complete MQTT 3.1.1 client implementation with:
 * - CONNECT/CONNACK handling with username/password authentication
 * - PUBLISH with QoS 0 and QoS 1
 * - PUBACK for QoS 1 acknowledgment
 * - PINGREQ/PINGRESP for keep-alive
 * - SUBSCRIBE/SUBACK for topic subscription
 * - UNSUBSCRIBE/UNSUBACK for topic unsubscription
 * - DISCONNECT for graceful disconnection
 * - Topic matching with wildcards (+, #)
 * - Callback-based message handling
 */

#ifndef _XY_MQTT_CLIENT_H_
#define _XY_MQTT_CLIENT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*============================================================================
 * MQTT Constants
 *============================================================================*/

/** @brief MQTT protocol name */
#define XY_MQTT_PROTOCOL_NAME     "MQTT"

/** @brief MQTT protocol level (3.1.1 = 4) */
#define XY_MQTT_PROTOCOL_LEVEL    4

/** @brief MQTT protocol level (3.1 = 3) */
#define XY_MQTT_PROTOCOL_LEVEL_3  3

/** @brief Maximum topic length */
#define XY_MQTT_MAX_TOPIC_LEN     256

/** @brief Maximum payload length */
#define XY_MQTT_MAX_PAYLOAD_LEN   65535

/** @brief Maximum client ID length */
#define XY_MQTT_MAX_CLIENT_ID_LEN 23

/** @brief Maximum packet identifier */
#define XY_MQTT_MAX_PACKET_ID     65535

/** @brief Maximum topic filters per SUBSCRIBE */
#define XY_MQTT_MAX_SUBSCRIPTIONS 10

/** @brief Default keep-alive interval in seconds */
#define XY_MQTT_DEFAULT_KEEPALIVE 60

/** @brief Minimum keep-alive interval in seconds */
#define XY_MQTT_MIN_KEEPALIVE      5

/** @brief Maximum keep-alive interval in seconds */
#define XY_MQTT_MAX_KEEPALIVE      65535

/** @brief Maximum remaining length encoding bytes */
#define XY_MQTT_MAX_REMAINING_LEN_ENCODED  4

/** @brief Default receive buffer size */
#define XY_MQTT_DEFAULT_RX_BUFFER_SIZE    1024

/** @brief Default transmit buffer size */
#define XY_MQTT_DEFAULT_TX_BUFFER_SIZE    1024

/*============================================================================
 * MQTT Packet Types (MQTT 3.1.1)
 *============================================================================*/

typedef enum {
    XY_MQTT_TYPE_RESERVED    = 0,   /**< Reserved */
    XY_MQTT_TYPE_CONNECT     = 1,   /**< Client request to connect to Server */
    XY_MQTT_TYPE_CONNACK     = 2,   /**< Connect acknowledgment */
    XY_MQTT_TYPE_PUBLISH     = 3,   /**< Publish message */
    XY_MQTT_TYPE_PUBACK      = 4,   /**< Publish acknowledgment (QoS 1) */
    XY_MQTT_TYPE_PUBREC      = 5,   /**< Publish received (QoS 2) */
    XY_MQTT_TYPE_PUBREL      = 6,   /**< Publish release (QoS 2) */
    XY_MQTT_TYPE_PUBCOMP     = 7,   /**< Publish complete (QoS 2) */
    XY_MQTT_TYPE_SUBSCRIBE   = 8,   /**< Subscribe request */
    XY_MQTT_TYPE_SUBACK      = 9,   /**< Subscribe acknowledgment */
    XY_MQTT_TYPE_UNSUBSCRIBE = 10,  /**< Unsubscribe request */
    XY_MQTT_TYPE_UNSUBACK    = 11,  /**< Unsubscribe acknowledgment */
    XY_MQTT_TYPE_PINGREQ     = 12,  /**< PING request */
    XY_MQTT_TYPE_PINGRESP    = 13,  /**< PING response */
    XY_MQTT_TYPE_DISCONNECT  = 14,  /**< Client disconnection */
    XY_MQTT_TYPE_AUTH        = 15,  /**< Authentication exchange */
} xy_mqtt_type_t;

/*============================================================================
 * MQTT QoS Levels
 *============================================================================*/

typedef enum {
    XY_MQTT_QOS_0 = 0,  /**< At most once delivery */
    XY_MQTT_QOS_1 = 1,  /**< At least once delivery */
    XY_MQTT_QOS_2 = 2,  /**< Exactly once delivery */
} xy_mqtt_qos_t;

/*============================================================================
 * MQTT Connection Return Codes
 *============================================================================*/

typedef enum {
    XY_MQTT_CONNACK_RC_ACCEPTED           = 0,    /**< Connection accepted */
    XY_MQTT_CONNACK_RC_UNACCEPTABLE_PROTO  = 1,   /**< Server does not support MQTT protocol */
    XY_MQTT_CONNACK_RC_IDENTIFIER_REJECTED = 2,   /**< Client identifier rejected */
    XY_MQTT_CONNACK_RC_SERVER_UNAVAILABLE   = 3,   /**< Server unavailable */
    XY_MQTT_CONNACK_RC_BAD_USERNAME_PASS   = 4,   /**< Username or password malformed */
    XY_MQTT_CONNACK_RC_NOT_AUTHORIZED       = 5,   /**< Not authorized */
} xy_mqtt_connack_rc_t;

/*============================================================================
 * MQTT Error Codes
 *============================================================================*/

typedef enum {
    XY_MQTT_OK = 0,
    XY_MQTT_ERR_INVALID_PARAM      = -1,  /**< Invalid parameter */
    XY_MQTT_ERR_INVALID_PACKET     = -2,  /**< Invalid packet format */
    XY_MQTT_ERR_INVALID_REMAINING_LEN = -3, /**< Invalid remaining length */
    XY_MQTT_ERR_INCOMPLETE_PACKET   = -4,  /**< Incomplete packet */
    XY_MQTT_ERR_UNSUPPORTED_PACKET  = -5,  /**< Unsupported packet type */
    XY_MQTT_ERR_TCP_DISCONNECTED    = -6,  /**< TCP connection lost */
    XY_MQTT_ERR_NOT_CONNECTED       = -7,  /**< Not connected to broker */
    XY_MQTT_ERR_NO_MEMORY           = -8,  /**< Memory allocation failed */
    XY_MQTT_ERR_PACKET_ID_EXHAUSTED = -9,  /**< Packet ID pool exhausted */
    XY_MQTT_ERR_TIMEOUT             = -10, /**< Operation timed out */
    XY_MQTT_ERR_TOPIC_TOO_LONG      = -11, /**< Topic exceeds max length */
    XY_MQTT_ERR_SUBSCRIPTION_FAILED = -12, /**< Subscription failed */
} xy_mqtt_err_t;

/*============================================================================
 * MQTT Client States
 *============================================================================*/

typedef enum {
    XY_MQTT_STATE_DISCONNECTED       = 0,  /**< Disconnected */
    XY_MQTT_STATE_TCP_CONNECTING     = 1,  /**< TCP connection in progress */
    XY_MQTT_STATE_MQTT_CONNECTING     = 2,  /**< MQTT CONNECT sent, waiting for CONNACK */
    XY_MQTT_STATE_CONNECTED          = 3,  /**< Connected to broker */
    XY_MQTT_STATE_DISCONNECTING      = 4,  /**< Disconnect in progress */
} xy_mqtt_state_t;

/*============================================================================
 * MQTT Client Flags
 *============================================================================*/

typedef enum {
    XY_MQTT_FLAG_CLEAN_SESSION    = (1 << 0),  /**< Clean session flag */
    XY_MQTT_FLAG_WILL_RETAIN      = (1 << 1),  /**< Will message retain flag */
    XY_MQTT_FLAG_PASSWORD          = (1 << 2),  /**< Password present flag */
    XY_MQTT_FLAG_USERNAME         = (1 << 3),  /**< Username present flag */
} xy_mqtt_flags_t;

/*============================================================================
 * MQTT Fixed Header
 *============================================================================*/

/**
 * @brief MQTT fixed header structure
 */
typedef struct {
    uint8_t type;       /**< Packet type (4 bits) */
    uint8_t flags;      /**< Packet-specific flags (4 bits) */
    uint32_t remaining_len;  /**< Remaining length */
} xy_mqtt_fixed_header_t;

/*============================================================================
 * MQTT CONNECT Packet
 *============================================================================*/

/**
 * @brief MQTT CONNECT variable header
 */
typedef struct {
    uint16_t protocol_name_len;
    char protocol_name[4];
    uint8_t protocol_level;
    uint8_t connect_flags;
    uint16_t keepalive;
} xy_mqtt_connect_varheader_t;

/**
 * @brief MQTT CONNECT payload
 */
typedef struct {
    char *client_id;
    char *will_topic;
    char *will_message;
    char *username;
    char *password;
} xy_mqtt_connect_payload_t;

/**
 * @brief Full MQTT CONNECT packet
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    xy_mqtt_connect_varheader_t varheader;
    xy_mqtt_connect_payload_t payload;
} xy_mqtt_connect_packet_t;

/*============================================================================
 * MQTT CONNACK Packet
 *============================================================================*/

/**
 * @brief MQTT CONNACK packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint8_t session_present;  /**< Session present flag (bit 0) */
    uint8_t return_code;       /**< Connect return code */
} xy_mqtt_connack_packet_t;

/*============================================================================
 * MQTT PUBLISH Packet
 *============================================================================*/

/**
 * @brief MQTT PUBLISH packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t topic_length;
    char topic[XY_MQTT_MAX_TOPIC_LEN];
    uint16_t packet_id;        /**< Packet identifier (QoS > 0) */
    uint8_t *payload;
    size_t payload_len;
} xy_mqtt_publish_packet_t;

/*============================================================================
 * MQTT PUBACK/PUBREC/PUBREL/PUBCOMP Packets
 *============================================================================*/

/**
 * @brief MQTT PUBACK/PUBREC/PUBREL/PUBCOMP packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t packet_id;
} xy_mqtt_ack_packet_t;

/*============================================================================
 * MQTT SUBSCRIBE Packet
 *============================================================================*/

/**
 * @brief Topic filter with QoS
 */
typedef struct {
    char topic[XY_MQTT_MAX_TOPIC_LEN];
    uint8_t qos;
} xy_mqtt_topic_filter_t;

/**
 * @brief MQTT SUBSCRIBE packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t packet_id;
    xy_mqtt_topic_filter_t filters[XY_MQTT_MAX_SUBSCRIPTIONS];
    uint8_t filter_count;
} xy_mqtt_subscribe_packet_t;

/*============================================================================
 * MQTT SUBACK Packet
 *============================================================================*/

/**
 * @brief MQTT SUBACK packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t packet_id;
    uint8_t return_codes[XY_MQTT_MAX_SUBSCRIPTIONS];
    uint8_t return_code_count;
} xy_mqtt_suback_packet_t;

/*============================================================================
 * MQTT UNSUBSCRIBE Packet
 *============================================================================*/

/**
 * @brief MQTT UNSUBSCRIBE packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t packet_id;
    char topics[XY_MQTT_MAX_SUBSCRIPTIONS][XY_MQTT_MAX_TOPIC_LEN];
    uint8_t topic_count;
} xy_mqtt_unsubscribe_packet_t;

/*============================================================================
 * MQTT UNSUBACK Packet
 *============================================================================*/

/**
 * @brief MQTT UNSUBACK packet structure
 */
typedef struct {
    xy_mqtt_fixed_header_t fixed_header;
    uint16_t packet_id;
} xy_mqtt_unsuback_packet_t;

/*============================================================================
 * Packet Identifier Management
 *============================================================================*/

/**
 * @brief Packet ID entry for tracking outgoing packets
 */
typedef struct xy_mqtt_packet_id_entry {
    uint16_t packet_id;
    uint8_t qos;
    void *context;
    struct xy_mqtt_packet_id_entry *next;
} xy_mqtt_packet_id_entry_t;

/*============================================================================
 * Topic Subscription
 *============================================================================*/

/**
 * @brief Topic subscription entry
 */
typedef struct xy_mqtt_subscription {
    char topic_filter[XY_MQTT_MAX_TOPIC_LEN];
    uint8_t qos;
    void *user_data;
    struct xy_mqtt_subscription *next;
} xy_mqtt_subscription_t;

/*============================================================================
 * MQTT Transport Layer Interface
 *============================================================================*/

/**
 * @brief Transport layer send function pointer
 * @param context Transport context (e.g., socket)
 * @param data Data to send
 * @param len Length of data
 * @return Number of bytes sent, or negative on error
 */
typedef int (*xy_mqtt_transport_send_t)(void *context, const uint8_t *data, size_t len);

/**
 * @brief Transport layer receive function pointer
 * @param context Transport context (e.g., socket)
 * @param data Buffer to receive into
 * @param len Maximum length to receive
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received, 0 on timeout, negative on error
 */
typedef int (*xy_mqtt_transport_recv_t)(void *context, uint8_t *data, size_t len, uint32_t timeout_ms);

/*============================================================================
 * MQTT Client Callbacks
 *============================================================================*/

/**
 * @brief Connected callback - called when connected to broker
 * @param mqtt MQTT client handle
 * @param session_present Session present flag
 * @param return_code Connect return code
 * @param user_data User data
 */
typedef void (*xy_mqtt_connected_cb_t)(void *mqtt, uint8_t session_present, uint8_t return_code, void *user_data);

/**
 * @brief Disconnected callback - called when disconnected from broker
 * @param mqtt MQTT client handle
 * @param reason Disconnect reason
 * @param user_data User data
 */
typedef void (*xy_mqtt_disconnected_cb_t)(void *mqtt, int reason, void *user_data);

/**
 * @brief Message callback - called when PUBLISH received
 * @param mqtt MQTT client handle
 * @param topic Topic name
 * @param payload Message payload
 * @param payload_len Payload length
 * @param qos QoS level
 * @param dup Duplicate flag
 * @param user_data User data
 */
typedef void (*xy_mqtt_message_cb_t)(void *mqtt, const char *topic, const uint8_t *payload,
                                     size_t payload_len, uint8_t qos, uint8_t dup, void *user_data);

/**
 * @brief Published callback - called when PUBLISH acknowledged (QoS 1/2)
 * @param mqtt MQTT client handle
 * @param packet_id Packet identifier
 * @param user_data User data
 */
typedef void (*xy_mqtt_published_cb_t)(void *mqtt, uint16_t packet_id, void *user_data);

/**
 * @brief Subscribed callback - called when SUBSCRIBE acknowledged
 * @param mqtt MQTT client handle
 * @param packet_id Packet identifier
 * @param qos QoS level granted
 * @param user_data User data
 */
typedef void (*xy_mqtt_subscribed_cb_t)(void *mqtt, uint16_t packet_id, uint8_t qos, void *user_data);

/**
 * @brief Unsubscribed callback - called when UNSUBSCRIBE acknowledged
 * @param mqtt MQTT client handle
 * @param packet_id Packet identifier
 * @param user_data User data
 */
typedef void (*xy_mqtt_unsubscribed_cb_t)(void *mqtt, uint16_t packet_id, void *user_data);

/*============================================================================
 * MQTT Client Configuration
 *============================================================================*/

/**
 * @brief MQTT client configuration structure
 */
typedef struct {
    uint16_t keepalive;              /**< Keep-alive interval in seconds */
    uint8_t clean_session;           /**< Clean session flag */
    uint8_t qos_default;             /**< Default QoS for publish */

    size_t tx_buffer_size;           /**< TX buffer size */
    size_t rx_buffer_size;           /**< RX buffer size */

    void *transport_context;         /**< Transport context (socket, etc.) */
    xy_mqtt_transport_send_t send;   /**< Send function */
    xy_mqtt_transport_recv_t recv;   /**< Receive function */

    xy_mqtt_connected_cb_t connected_cb;
    xy_mqtt_disconnected_cb_t disconnected_cb;
    xy_mqtt_message_cb_t message_cb;
    xy_mqtt_published_cb_t published_cb;
    xy_mqtt_subscribed_cb_t subscribed_cb;
    xy_mqtt_unsubscribed_cb_t unsubscribed_cb;
    void *user_data;
} xy_mqtt_config_t;

/*============================================================================
 * MQTT Client Handle (Opaque)
 *============================================================================*/

/**
 * @brief MQTT client handle (opaque structure)
 */
typedef struct xy_mqtt_client xy_mqtt_client_t;

/*============================================================================
 * MQTT Client API
 *============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create MQTT client instance
 * @param config Client configuration
 * @return MQTT client handle, or NULL on failure
 */
xy_mqtt_client_t *xy_mqtt_client_new(const xy_mqtt_config_t *config);

/**
 * @brief Delete MQTT client instance
 * @param mqtt MQTT client handle
 */
void xy_mqtt_client_delete(xy_mqtt_client_t *mqtt);

/**
 * @brief Connect to MQTT broker
 * @param mqtt MQTT client handle
 * @param client_id Client identifier (NULL for auto-generated)
 * @param username Username (NULL if not used)
 * @param password Password (NULL if not used)
 * @param will_topic Will topic (NULL if not used)
 * @param will_message Will message (NULL if not used)
 * @param will_qos Will QoS (0, 1, or 2)
 * @param will_retain Will retain flag
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_connect_with_will(xy_mqtt_client_t *mqtt,
                                        const char *client_id,
                                        const char *username,
                                        const char *password,
                                        const char *will_topic,
                                        const char *will_message,
                                        uint8_t will_qos,
                                        uint8_t will_retain);

/**
 * @brief Connect to MQTT broker (simplified)
 * @param mqtt MQTT client handle
 * @param client_id Client identifier
 * @param username Username (NULL if not used)
 * @param password Password (NULL if not used)
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_connect(xy_mqtt_client_t *mqtt,
                              const char *client_id,
                              const char *username,
                              const char *password);

/**
 * @brief Disconnect from MQTT broker
 * @param mqtt MQTT client handle
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_disconnect(xy_mqtt_client_t *mqtt);

/**
 * @brief Check if MQTT client is connected
 * @param mqtt MQTT client handle
 * @return true if connected, false otherwise
 */
bool xy_mqtt_is_connected(xy_mqtt_client_t *mqtt);

/**
 * @brief Publish message to topic
 * @param mqtt MQTT client handle
 * @param topic Topic name
 * @param payload Message payload
 * @param payload_len Payload length
 * @param qos QoS level (0 or 1)
 * @param retain Retain flag
 * @param packet_id Output packet identifier (can be NULL)
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_publish(xy_mqtt_client_t *mqtt,
                              const char *topic,
                              const uint8_t *payload,
                              size_t payload_len,
                              uint8_t qos,
                              uint8_t retain,
                              uint16_t *packet_id);

/**
 * @brief Subscribe to topic
 * @param mqtt MQTT client handle
 * @param topic_filter Topic filter (can include + and # wildcards)
 * @param qos Maximum QoS level
 * @param packet_id Output packet identifier (can be NULL)
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_subscribe(xy_mqtt_client_t *mqtt,
                                const char *topic_filter,
                                uint8_t qos,
                                uint16_t *packet_id);

/**
 * @brief Unsubscribe from topic
 * @param mqtt MQTT client handle
 * @param topic_filter Topic filter to unsubscribe
 * @param packet_id Output packet identifier (can be NULL)
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_unsubscribe(xy_mqtt_client_t *mqtt,
                                  const char *topic_filter,
                                  uint16_t *packet_id);

/**
 * @brief Process incoming packets
 * @param mqtt MQTT client handle
 * @param timeout_ms Timeout in milliseconds
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_process(xy_mqtt_client_t *mqtt, uint32_t timeout_ms);

/**
 * @brief Check keep-alive and send PING if needed
 * @param mqtt MQTT client handle
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_keepalive_check(xy_mqtt_client_t *mqtt);

/**
 * @brief Get current client state
 * @param mqtt MQTT client handle
 * @return Client state
 */
xy_mqtt_state_t xy_mqtt_get_state(xy_mqtt_client_t *mqtt);

/**
 * @brief Get last error code
 * @param mqtt MQTT client handle
 * @return Last error code
 */
xy_mqtt_err_t xy_mqtt_get_error(xy_mqtt_client_t *mqtt);

/*============================================================================
 * Utility Functions (Public)
 *============================================================================*/

/**
 * @brief Encode remaining length (MQTT style)
 * @param buf Buffer to write encoded length
 * @param len Length to encode
 * @return Number of bytes written, or -1 on error
 */
int xy_mqtt_encode_remaining_length(uint8_t *buf, uint32_t len);

/**
 * @brief Decode remaining length (MQTT style)
 * @param buf Buffer to read encoded length from
 * @param len Output decoded length
 * @param consumed Output number of bytes consumed
 * @return XY_MQTT_OK on success, error code otherwise
 */
xy_mqtt_err_t xy_mqtt_decode_remaining_length(const uint8_t *buf, uint32_t *len, uint8_t *consumed);

/**
 * @brief Match topic against topic filter (supports + and # wildcards)
 * @param topic_filter Topic filter
 * @param topic Topic name
 * @return true if match, false otherwise
 */
bool xy_mqtt_topic_match(const char *topic_filter, const char *topic);

/**
 * @brief Get MQTT return code string
 * @param rc Return code
 * @return String description
 */
const char *xy_mqtt_connack_rc_string(uint8_t rc);

#ifdef __cplusplus
}
#endif

#endif /* _XY_MQTT_CLIENT_H_ */
