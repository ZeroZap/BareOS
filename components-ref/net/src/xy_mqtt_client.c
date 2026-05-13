/**
 * @file xy_mqtt_client.c
 * @brief MQTT 3.1.1 Client Implementation
 *
 * This file implements a complete MQTT 3.1.1 client with:
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

#include "xy_mqtt_client.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*============================================================================
 * Macro Definitions
 *============================================================================*/

#define XY_MQTT_BITFIELD(var, pos, len) (((var) >> (pos)) & ((1 << (len)) - 1))

/*============================================================================
 * MQTT Client Internal Structure
 *============================================================================*/

/**
 * @brief MQTT client internal structure
 */
struct xy_mqtt_client {
    /* Configuration */
    xy_mqtt_config_t config;

    /* State */
    xy_mqtt_state_t state;
    xy_mqtt_err_t last_error;

    /* Keep-alive */
    uint32_t last_send_time;
    uint32_t last_recv_time;
    uint16_t keepalive_interval;

    /* Connection info */
    char client_id[XY_MQTT_MAX_CLIENT_ID_LEN + 1];
    uint8_t session_present;

    /* Will message */
    char will_topic[XY_MQTT_MAX_TOPIC_LEN];
    char will_message[XY_MQTT_MAX_PAYLOAD_LEN];
    uint16_t will_qos;
    uint8_t will_retain;
    uint8_t will_flag;

    /* Packet ID management */
    uint16_t next_packet_id;
    xy_mqtt_packet_id_entry_t *pending_acks;

    /* Subscriptions */
    xy_mqtt_subscription_t *subscriptions;

    /* Buffers */
    uint8_t *tx_buffer;
    size_t tx_buffer_size;
    uint8_t *rx_buffer;
    size_t rx_buffer_size;
    size_t rx_buffer_pos;
};

/*============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Get current time in milliseconds
 * @return Current time in milliseconds
 */
static uint32_t xy_mqtt_get_time_ms(void)
{
    /* For embedded systems, this should use actual time source */
    /* Using a simple tick - in real implementation, use platform time */
    static uint32_t tick_count = 0;
    return tick_count++; /* Placeholder - replace with actual time */
}

/**
 * @brief Allocate new packet ID
 * @param mqtt MQTT client handle
 * @return New packet ID, or 0 if exhausted
 */
static uint16_t xy_mqtt_alloc_packet_id(xy_mqtt_client_t *mqtt)
{
    uint16_t packet_id = mqtt->next_packet_id;

    /* Search for unused packet ID */
    for (int i = 0; i < XY_MQTT_MAX_PACKET_ID; i++) {
        packet_id = (packet_id % XY_MQTT_MAX_PACKET_ID) + 1;

        /* Check if packet ID is not in use */
        xy_mqtt_packet_id_entry_t *entry = mqtt->pending_acks;
        bool in_use = false;
        while (entry) {
            if (entry->packet_id == packet_id) {
                in_use = true;
                break;
            }
            entry = entry->next;
        }

        if (!in_use) {
            mqtt->next_packet_id = packet_id;
            return packet_id;
        }
    }

    return 0; /* Packet ID exhausted */
}

/**
 * @brief Add pending ack entry
 * @param mqtt MQTT client handle
 * @param packet_id Packet ID
 * @param qos QoS level
 * @param context Context data
 * @return XY_MQTT_OK on success, error code otherwise
 */
static xy_mqtt_err_t xy_mqtt_add_pending_ack(xy_mqtt_client_t *mqtt,
                                              uint16_t packet_id,
                                              uint8_t qos,
                                              void *context)
{
    xy_mqtt_packet_id_entry_t *entry = (xy_mqtt_packet_id_entry_t *)
        malloc(sizeof(xy_mqtt_packet_id_entry_t));
    if (!entry) {
        return XY_MQTT_ERR_NO_MEMORY;
    }

    entry->packet_id = packet_id;
    entry->qos = qos;
    entry->context = context;
    entry->next = mqtt->pending_acks;
    mqtt->pending_acks = entry;

    return XY_MQTT_OK;
}

/**
 * @brief Remove pending ack entry
 * @param mqtt MQTT client handle
 * @param packet_id Packet ID
 * @return Pointer to removed entry, or NULL
 */
static void *xy_mqtt_remove_pending_ack(xy_mqtt_client_t *mqtt, uint16_t packet_id)
{
    xy_mqtt_packet_id_entry_t **entry_ptr = &mqtt->pending_acks;
    void *context = NULL;

    while (*entry_ptr) {
        if ((*entry_ptr)->packet_id == packet_id) {
            xy_mqtt_packet_id_entry_t *entry = *entry_ptr;
            context = entry->context;
            *entry_ptr = entry->next;
            free(entry);
            return context;
        }
        entry_ptr = &((*entry_ptr)->next);
    }

    return NULL;
}

/**
 * @brief Add subscription
 * @param mqtt MQTT client handle
 * @param topic_filter Topic filter
 * @param qos QoS level
 * @return XY_MQTT_OK on success, error code otherwise
 */
static xy_mqtt_err_t xy_mqtt_add_subscription(xy_mqtt_client_t *mqtt,
                                               const char *topic_filter,
                                               uint8_t qos)
{
    /* Check if already subscribed */
    xy_mqtt_subscription_t **sub_ptr = &mqtt->subscriptions;
    while (*sub_ptr) {
        if (strcmp((*sub_ptr)->topic_filter, topic_filter) == 0) {
            (*sub_ptr)->qos = qos;
            return XY_MQTT_OK;
        }
        sub_ptr = &((*sub_ptr)->next);
    }

    /* Allocate new subscription */
    xy_mqtt_subscription_t *sub = (xy_mqtt_subscription_t *)
        malloc(sizeof(xy_mqtt_subscription_t));
    if (!sub) {
        return XY_MQTT_ERR_NO_MEMORY;
    }

    strncpy(sub->topic_filter, topic_filter, XY_MQTT_MAX_TOPIC_LEN - 1);
    sub->topic_filter[XY_MQTT_MAX_TOPIC_LEN - 1] = '\0';
    sub->qos = qos;
    sub->user_data = NULL;
    sub->next = NULL;

    *sub_ptr = sub;
    return XY_MQTT_OK;
}

/**
 * @brief Remove subscription
 * @param mqtt MQTT client handle
 * @param topic_filter Topic filter
 * @return XY_MQTT_OK on success, error code otherwise
 */
static xy_mqtt_err_t xy_mqtt_remove_subscription(xy_mqtt_client_t *mqtt,
                                                   const char *topic_filter)
{
    xy_mqtt_subscription_t **sub_ptr = &mqtt->subscriptions;

    while (*sub_ptr) {
        if (strcmp((*sub_ptr)->topic_filter, topic_filter) == 0) {
            xy_mqtt_subscription_t *sub = *sub_ptr;
            *sub_ptr = sub->next;
            free(sub);
            return XY_MQTT_OK;
        }
        sub_ptr = &((*sub_ptr)->next);
    }

    return XY_MQTT_OK;
}

/*============================================================================
 * Remaining Length Encoding/Decoding
 *============================================================================*/

int xy_mqtt_encode_remaining_length(uint8_t *buf, uint32_t len)
{
    int count = 0;

    do {
        if (count >= XY_MQTT_MAX_REMAINING_LEN_ENCODED) {
            return -1; /* Invalid length */
        }

        uint8_t byte = len % 128;
        len /= 128;

        if (len > 0) {
            byte |= 0x80; /* Set continuation bit */
        }

        buf[count++] = byte;
    } while (len > 0);

    return count;
}

xy_mqtt_err_t xy_mqtt_decode_remaining_length(const uint8_t *buf, uint32_t *len, uint8_t *consumed)
{
    uint32_t multiplier = 1;
    uint32_t value = 0;
    uint8_t count = 0;
    uint8_t byte;

    do {
        byte = buf[count];
        value += (byte & 0x7F) * multiplier;
        multiplier *= 128;

        if (count >= XY_MQTT_MAX_REMAINING_LEN_ENCODED) {
            return XY_MQTT_ERR_INVALID_REMAINING_LEN;
        }

        count++;
    } while ((byte & 0x80) != 0);

    *len = value;
    *consumed = count;

    return XY_MQTT_OK;
}

/*============================================================================
 * Topic Matching
 *============================================================================*/

bool xy_mqtt_topic_match(const char *topic_filter, const char *topic)
{
    const char *filter_pos = topic_filter;
    const char *topic_pos = topic;
    const char * wildcard_pos;

    while (*filter_pos && *topic_pos) {
        if (*filter_pos == '#') {
            /* Multi-level wildcard matches rest of topic */
            return true;
        }

        if (*filter_pos == '+') {
            /* Single-level wildcard matches one topic level */
            wildcard_pos = topic_pos;

            /* Find next '/' in topic */
            while (*wildcard_pos && *wildcard_pos != '/') {
                wildcard_pos++;
            }

            /* Check if there's a '/' at current filter position */
            if (*filter_pos == '\0' || *filter_pos == '/') {
                topic_pos = wildcard_pos;
            } else if (*wildcard_pos == '/') {
                topic_pos = wildcard_pos;
            } else {
                return false;
            }
        } else if (*filter_pos != *topic_pos) {
            return false;
        } else {
            filter_pos++;
            topic_pos++;
        }
    }

    /* Check for end of strings */
    if (*filter_pos == '\0' && *topic_pos == '\0') {
        return true;
    }

    if (*filter_pos == '#' && (*topic_pos == '\0' || *(filter_pos - 1) == '/')) {
        return true;
    }

    /* Handle trailing + */
    if (*filter_pos == '+' && (*filter_pos == '\0' || *(filter_pos + 1) == '\0')) {
        /* Find if topic has more levels */
        while (*topic_pos) {
            if (*topic_pos == '/') {
                return false;
            }
            topic_pos++;
        }
        return true;
    }

    return false;
}

/*============================================================================
 * Packet Building Functions
 *============================================================================*/

/**
 * @brief Build CONNECT packet
 * @param mqtt MQTT client handle
 * @param client_id Client identifier
 * @param username Username
 * @param password Password
 * @param will_topic Will topic
 * @param will_message Will message
 * @param will_qos Will QoS
 * @param will_retain Will retain
 * @param clean_session Clean session flag
 * @return XY_MQTT_OK on success, error code otherwise
 */
static xy_mqtt_err_t xy_mqtt_send_connect(xy_mqtt_client_t *mqtt,
                                           const char *client_id,
                                           const char *username,
                                           const char *password,
                                           const char *will_topic,
                                           const char *will_message,
                                           uint8_t will_qos,
                                           uint8_t will_retain,
                                           uint8_t clean_session)
{
    uint8_t *buf = mqtt->tx_buffer;
    size_t pos = 0;
    uint8_t flags = 0;

    /* Fixed header */
    buf[pos++] = (XY_MQTT_TYPE_CONNECT << 4);

    /* Build connect flags */
    if (clean_session) {
        flags |= 0x02; /* Clean session bit */
    }
    if (will_topic && will_message) {
        flags |= 0x04; /* Will flag */
        flags |= (will_qos << 3) & 0x18; /* Will QoS */
    }
    if (will_retain) {
        flags |= 0x20; /* Will retain */
    }
    if (username) {
        flags |= 0x40; /* Username flag */
    }
    if (password) {
        flags |= 0x80; /* Password flag */
    }

    /* Calculate remaining length */
    uint32_t remaining = 0;
    remaining += 2 + 4;  /* Protocol name */
    remaining += 1;       /* Protocol level */
    remaining += 1;       /* Connect flags */
    remaining += 2;       /* Keep-alive */

    /* Client ID */
    size_t client_id_len = client_id ? strlen(client_id) : 0;
    if (client_id_len == 0) {
        /* Generate client ID if empty */
        snprintf(mqtt->client_id, sizeof(mqtt->client_id), "xy_mqtt_%04x",
                 (unsigned int)xy_mqtt_get_time_ms());
        client_id = mqtt->client_id;
        client_id_len = strlen(client_id);
    }
    remaining += 2 + client_id_len;

    /* Will topic and message */
    if (will_topic && will_message) {
        remaining += 2 + strlen(will_topic);
        remaining += 2 + strlen(will_message);
    }

    /* Username */
    if (username) {
        remaining += 2 + strlen(username);
    }

    /* Password */
    if (password) {
        remaining += 2 + strlen(password);
    }

    /* Encode remaining length */
    int len_bytes = xy_mqtt_encode_remaining_length(buf + pos, remaining);
    if (len_bytes < 0) {
        return XY_MQTT_ERR_INVALID_REMAINING_LEN;
    }
    pos += len_bytes;

    /* Variable header */
    buf[pos++] = 0; /* Protocol name length MSB */
    buf[pos++] = 4; /* Protocol name length LSB */
    memcpy(&buf[pos], "MQTT", 4);
    pos += 4;
    buf[pos++] = XY_MQTT_PROTOCOL_LEVEL; /* Protocol level 4 = 3.1.1 */
    buf[pos++] = flags;                   /* Connect flags */
    buf[pos++] = (uint8_t)((mqtt->keepalive_interval >> 8) & 0xFF); /* Keep-alive MSB */
    buf[pos++] = (uint8_t)(mqtt->keepalive_interval & 0xFF);       /* Keep-alive LSB */

    /* Payload: Client ID */
    buf[pos++] = (uint8_t)((client_id_len >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(client_id_len & 0xFF);
    memcpy(&buf[pos], client_id, client_id_len);
    pos += client_id_len;

    /* Will topic and message */
    if (will_topic && will_message) {
        size_t topic_len = strlen(will_topic);
        buf[pos++] = (uint8_t)((topic_len >> 8) & 0xFF);
        buf[pos++] = (uint8_t)(topic_len & 0xFF);
        memcpy(&buf[pos], will_topic, topic_len);
        pos += topic_len;

        size_t msg_len = strlen(will_message);
        buf[pos++] = (uint8_t)((msg_len >> 8) & 0xFF);
        buf[pos++] = (uint8_t)(msg_len & 0xFF);
        memcpy(&buf[pos], will_message, msg_len);
        pos += msg_len;
    }

    /* Username */
    if (username) {
        size_t username_len = strlen(username);
        buf[pos++] = (uint8_t)((username_len >> 8) & 0xFF);
        buf[pos++] = (uint8_t)(username_len & 0xFF);
        memcpy(&buf[pos], username, username_len);
        pos += username_len;
    }

    /* Password */
    if (password) {
        size_t password_len = strlen(password);
        buf[pos++] = (uint8_t)((password_len >> 8) & 0xFF);
        buf[pos++] = (uint8_t)(password_len & 0xFF);
        memcpy(&buf[pos], password, password_len);
        pos += password_len;
    }

    /* Send packet */
    int sent = mqtt->config.send(mqtt->config.transport_context, buf, pos);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    return XY_MQTT_OK;
}

/**
 * @brief Build and send CONNACK response parsing
 */
static xy_mqtt_err_t xy_mqtt_handle_connack(xy_mqtt_client_t *mqtt,
                                            const uint8_t *data,
                                            size_t len)
{
    (void)len;

    if (mqtt->state != XY_MQTT_STATE_MQTT_CONNECTING) {
        return XY_MQTT_OK; /* Ignore if not expecting CONNACK */
    }

    if (len < 4) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    /* Parse CONNACK */
    uint8_t session_present = (data[2] & 0x01);
    uint8_t return_code = data[3];

    mqtt->session_present = session_present;

    if (return_code == XY_MQTT_CONNACK_RC_ACCEPTED) {
        mqtt->state = XY_MQTT_STATE_CONNECTED;
        mqtt->last_error = XY_MQTT_OK;

        if (mqtt->config.connected_cb) {
            mqtt->config.connected_cb(mqtt, session_present, return_code,
                                      mqtt->config.user_data);
        }
    } else {
        mqtt->state = XY_MQTT_STATE_DISCONNECTED;
        mqtt->last_error = XY_MQTT_ERR_NOT_CONNECTED;

        if (mqtt->config.disconnected_cb) {
            mqtt->config.disconnected_cb(mqtt, return_code,
                                         mqtt->config.user_data);
        }
    }

    return XY_MQTT_OK;
}

/**
 * @brief Build and send PUBLISH packet
 */
static xy_mqtt_err_t xy_mqtt_send_publish(xy_mqtt_client_t *mqtt,
                                           const char *topic,
                                           const uint8_t *payload,
                                           size_t payload_len,
                                           uint8_t qos,
                                           uint8_t retain,
                                           uint8_t dup,
                                           uint16_t *packet_id)
{
    uint8_t *buf = mqtt->tx_buffer;
    size_t pos = 0;

    /* Fixed header */
    uint8_t flags = (dup << 3) | ((qos & 0x03) << 1) | (retain & 0x01);
    buf[pos++] = (XY_MQTT_TYPE_PUBLISH << 4) | flags;

    /* Calculate remaining length */
    size_t topic_len = strlen(topic);
    uint32_t remaining = 2 + topic_len + payload_len;
    if (qos > 0) {
        remaining += 2; /* Packet ID */
    }

    /* Encode remaining length */
    int len_bytes = xy_mqtt_encode_remaining_length(buf + pos, remaining);
    if (len_bytes < 0) {
        return XY_MQTT_ERR_INVALID_REMAINING_LEN;
    }
    pos += len_bytes;

    /* Variable header */
    buf[pos++] = (uint8_t)((topic_len >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(topic_len & 0xFF);
    memcpy(&buf[pos], topic, topic_len);
    pos += topic_len;

    /* Packet ID for QoS > 0 */
    uint16_t pid = 0;
    if (qos > 0) {
        pid = *packet_id ? *packet_id : xy_mqtt_alloc_packet_id(mqtt);
        if (pid == 0) {
            return XY_MQTT_ERR_PACKET_ID_EXHAUSTED;
        }
        buf[pos++] = (uint8_t)((pid >> 8) & 0xFF);
        buf[pos++] = (uint8_t)(pid & 0xFF);
        *packet_id = pid;

        /* Track pending acknowledgment */
        xy_mqtt_add_pending_ack(mqtt, pid, qos, NULL);
    }

    /* Payload */
    memcpy(&buf[pos], payload, payload_len);
    pos += payload_len;

    /* Send packet */
    int sent = mqtt->config.send(mqtt->config.transport_context, buf, pos);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    return XY_MQTT_OK;
}

/**
 * @brief Handle incoming PUBLISH packet
 */
static xy_mqtt_err_t xy_mqtt_handle_publish(xy_mqtt_client_t *mqtt,
                                            const uint8_t *data,
                                            size_t len)
{
    if (len < 5) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    /* Parse fixed header */
    uint8_t flags = (data[0] & 0x0F);
    uint8_t dup = (flags >> 3) & 0x01;
    uint8_t qos = (flags >> 1) & 0x03;
    uint8_t retain = flags & 0x01;
    (void)retain; /* Reserved for future use */

    /* Decode remaining length */
    uint32_t remaining_len;
    uint8_t consumed;
    xy_mqtt_err_t err = xy_mqtt_decode_remaining_length(data + 1, &remaining_len, &consumed);
    if (err != XY_MQTT_OK) {
        return err;
    }

    if (len < (size_t)(1 + consumed + remaining_len)) {
        return XY_MQTT_ERR_INCOMPLETE_PACKET;
    }

    size_t pos = 1 + consumed;

    /* Parse variable header */
    if (len < pos + 2) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    uint16_t topic_len = ((uint16_t)data[pos] << 8) | data[pos + 1];
    pos += 2;

    if (topic_len >= XY_MQTT_MAX_TOPIC_LEN || topic_len == 0) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    if (len < pos + topic_len) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    char topic[XY_MQTT_MAX_TOPIC_LEN];
    memcpy(topic, &data[pos], topic_len);
    topic[topic_len] = '\0';
    pos += topic_len;

    /* Packet ID for QoS > 0 */
    uint16_t packet_id = 0;
    if (qos > 0) {
        if (len < pos + 2) {
            return XY_MQTT_ERR_INVALID_PACKET;
        }
        packet_id = ((uint16_t)data[pos] << 8) | data[pos + 1];
        pos += 2;
    }

    /* Payload */
    size_t payload_len = remaining_len - (pos - 1 - consumed - 2) - topic_len - (qos > 0 ? 2 : 0);
    const uint8_t *payload = &data[pos];

    /* Find matching subscriptions and deliver message */
    xy_mqtt_subscription_t *sub = mqtt->subscriptions;
    while (sub) {
        if (xy_mqtt_topic_match(sub->topic_filter, topic)) {
            if (mqtt->config.message_cb) {
                /* Limit QoS to subscribed QoS */
                uint8_t deliver_qos = (qos < sub->qos) ? qos : sub->qos;
                mqtt->config.message_cb(mqtt, topic, payload, payload_len,
                                        deliver_qos, dup, sub->user_data);
            }
        }
        sub = sub->next;
    }

    /* Send PUBACK for QoS 1 */
    if (qos == 1 && packet_id != 0) {
        uint8_t puback_buf[4];
        puback_buf[0] = (XY_MQTT_TYPE_PUBACK << 4);
        puback_buf[1] = 2; /* Remaining length */
        puback_buf[2] = (uint8_t)((packet_id >> 8) & 0xFF);
        puback_buf[3] = (uint8_t)(packet_id & 0xFF);

        mqtt->config.send(mqtt->config.transport_context, puback_buf, 4);
        mqtt->last_send_time = xy_mqtt_get_time_ms();
    }

    return XY_MQTT_OK;
}

/**
 * @brief Handle PUBACK packet (QoS 1 publish acknowledgment)
 */
static xy_mqtt_err_t xy_mqtt_handle_puback(xy_mqtt_client_t *mqtt,
                                           const uint8_t *data,
                                           size_t len)
{
    if (len < 4) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    uint16_t packet_id = ((uint16_t)data[2] << 8) | data[3];

    /* Remove from pending acks */
    void *context = xy_mqtt_remove_pending_ack(mqtt, packet_id);

    /* Notify callback */
    if (mqtt->config.published_cb) {
        mqtt->config.published_cb(mqtt, packet_id, context ? context : mqtt->config.user_data);
    }

    return XY_MQTT_OK;
}

/**
 * @brief Build and send SUBSCRIBE packet
 */
static xy_mqtt_err_t xy_mqtt_send_subscribe(xy_mqtt_client_t *mqtt,
                                              const char *topic_filter,
                                              uint8_t qos,
                                              uint16_t *packet_id)
{
    uint8_t *buf = mqtt->tx_buffer;
    size_t pos = 0;

    /* Allocate packet ID */
    uint16_t pid = xy_mqtt_alloc_packet_id(mqtt);
    if (pid == 0) {
        return XY_MQTT_ERR_PACKET_ID_EXHAUSTED;
    }
    *packet_id = pid;

    /* Fixed header */
    buf[pos++] = (XY_MQTT_TYPE_SUBSCRIBE << 4) | 0x02; /* Reserved flags for SUBSCRIBE */

    /* Calculate remaining length */
    size_t topic_len = strlen(topic_filter);
    uint32_t remaining = 2 + 2 + topic_len + 1; /* Packet ID + Topic + QoS */

    /* Encode remaining length */
    int len_bytes = xy_mqtt_encode_remaining_length(buf + pos, remaining);
    if (len_bytes < 0) {
        return XY_MQTT_ERR_INVALID_REMAINING_LEN;
    }
    pos += len_bytes;

    /* Variable header: Packet ID */
    buf[pos++] = (uint8_t)((pid >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(pid & 0xFF);

    /* Payload: Topic filter + QoS */
    buf[pos++] = (uint8_t)((topic_len >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(topic_len & 0xFF);
    memcpy(&buf[pos], topic_filter, topic_len);
    pos += topic_len;
    buf[pos++] = qos & 0x03;

    /* Send packet */
    int sent = mqtt->config.send(mqtt->config.transport_context, buf, pos);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    /* Track pending acknowledgment */
    xy_mqtt_add_pending_ack(mqtt, pid, XY_MQTT_QOS_1, NULL);

    /* Add to subscription list */
    return xy_mqtt_add_subscription(mqtt, topic_filter, qos);
}

/**
 * @brief Handle SUBACK packet
 */
static xy_mqtt_err_t xy_mqtt_handle_suback(xy_mqtt_client_t *mqtt,
                                            const uint8_t *data,
                                            size_t len)
{
    if (len < 5) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    uint16_t packet_id = ((uint16_t)data[2] << 8) | data[3];

    /* Parse return codes (simplified - assumes single topic) */
    for (size_t i = 4; i < len; i++) {
        uint8_t return_code = data[i] & 0x03;

        if (return_code == 0x80) {
            /* Subscription failed */
            mqtt->last_error = XY_MQTT_ERR_SUBSCRIPTION_FAILED;
        }

        /* Notify callback */
        if (mqtt->config.subscribed_cb) {
            mqtt->config.subscribed_cb(mqtt, packet_id, return_code,
                                       mqtt->config.user_data);
        }
    }

    /* Remove from pending acks */
    xy_mqtt_remove_pending_ack(mqtt, packet_id);

    return XY_MQTT_OK;
}

/**
 * @brief Build and send UNSUBSCRIBE packet
 */
static xy_mqtt_err_t xy_mqtt_send_unsubscribe(xy_mqtt_client_t *mqtt,
                                               const char *topic_filter,
                                               uint16_t *packet_id)
{
    uint8_t *buf = mqtt->tx_buffer;
    size_t pos = 0;

    /* Allocate packet ID */
    uint16_t pid = xy_mqtt_alloc_packet_id(mqtt);
    if (pid == 0) {
        return XY_MQTT_ERR_PACKET_ID_EXHAUSTED;
    }
    *packet_id = pid;

    /* Fixed header */
    buf[pos++] = (XY_MQTT_TYPE_UNSUBSCRIBE << 4) | 0x02; /* Reserved flags */

    /* Calculate remaining length */
    size_t topic_len = strlen(topic_filter);
    uint32_t remaining = 2 + 2 + topic_len; /* Packet ID + Topic */

    /* Encode remaining length */
    int len_bytes = xy_mqtt_encode_remaining_length(buf + pos, remaining);
    if (len_bytes < 0) {
        return XY_MQTT_ERR_INVALID_REMAINING_LEN;
    }
    pos += len_bytes;

    /* Variable header: Packet ID */
    buf[pos++] = (uint8_t)((pid >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(pid & 0xFF);

    /* Payload: Topic filter */
    buf[pos++] = (uint8_t)((topic_len >> 8) & 0xFF);
    buf[pos++] = (uint8_t)(topic_len & 0xFF);
    memcpy(&buf[pos], topic_filter, topic_len);
    pos += topic_len;

    /* Send packet */
    int sent = mqtt->config.send(mqtt->config.transport_context, buf, pos);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    /* Track pending acknowledgment */
    xy_mqtt_add_pending_ack(mqtt, pid, XY_MQTT_QOS_1, NULL);

    /* Remove from subscription list */
    return xy_mqtt_remove_subscription(mqtt, topic_filter);
}

/**
 * @brief Handle UNSUBACK packet
 */
static xy_mqtt_err_t xy_mqtt_handle_unsuback(xy_mqtt_client_t *mqtt,
                                              const uint8_t *data,
                                              size_t len)
{
    if (len < 4) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    uint16_t packet_id = ((uint16_t)data[2] << 8) | data[3];

    /* Remove from pending acks */
    xy_mqtt_remove_pending_ack(mqtt, packet_id);

    /* Notify callback */
    if (mqtt->config.unsubscribed_cb) {
        mqtt->config.unsubscribed_cb(mqtt, packet_id, mqtt->config.user_data);
    }

    return XY_MQTT_OK;
}

/**
 * @brief Build and send PINGREQ packet
 */
static xy_mqtt_err_t xy_mqtt_send_pingreq(xy_mqtt_client_t *mqtt)
{
    uint8_t pingreq_buf[2];

    pingreq_buf[0] = (XY_MQTT_TYPE_PINGREQ << 4);
    pingreq_buf[1] = 0; /* Remaining length */

    int sent = mqtt->config.send(mqtt->config.transport_context, pingreq_buf, 2);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    return XY_MQTT_OK;
}

/**
 * @brief Handle PINGRESP packet
 */
static xy_mqtt_err_t xy_mqtt_handle_pingresp(xy_mqtt_client_t *mqtt,
                                              const uint8_t *data,
                                              size_t len)
{
    (void)mqtt;
    (void)data;

    if (len < 2) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    /* PINGRESP just indicates broker is alive */
    return XY_MQTT_OK;
}

/**
 * @brief Build and send DISCONNECT packet
 */
static xy_mqtt_err_t xy_mqtt_send_disconnect(xy_mqtt_client_t *mqtt)
{
    uint8_t disc_buf[2];

    disc_buf[0] = (XY_MQTT_TYPE_DISCONNECT << 4);
    disc_buf[1] = 0; /* Remaining length */

    int sent = mqtt->config.send(mqtt->config.transport_context, disc_buf, 2);
    if (sent < 0) {
        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    mqtt->last_send_time = xy_mqtt_get_time_ms();

    return XY_MQTT_OK;
}

/*============================================================================
 * Packet Parser
 *============================================================================*/

/**
 * @brief Parse and handle incoming packet
 * @param mqtt MQTT client handle
 * @param data Packet data
 * @param len Packet length
 * @return XY_MQTT_OK on success, error code otherwise
 */
static xy_mqtt_err_t xy_mqtt_parse_packet(xy_mqtt_client_t *mqtt,
                                           const uint8_t *data,
                                           size_t len)
{
    if (len < 2) {
        return XY_MQTT_ERR_INVALID_PACKET;
    }

    /* Extract packet type and flags */
    uint8_t type = (data[0] >> 4) & 0x0F;
    uint8_t flags = data[0] & 0x0F;
    (void)flags; /* Reserved for future use */

    /* Decode remaining length */
    uint32_t remaining_len;
    uint8_t consumed;
    xy_mqtt_err_t err = xy_mqtt_decode_remaining_length(data + 1, &remaining_len, &consumed);
    if (err != XY_MQTT_OK) {
        return err;
    }

    /* Validate total length */
    if (len < (size_t)(1 + consumed + remaining_len)) {
        return XY_MQTT_ERR_INCOMPLETE_PACKET;
    }

    /* Update last receive time */
    mqtt->last_recv_time = xy_mqtt_get_time_ms();

    /* Dispatch to handler */
    switch (type) {
        case XY_MQTT_TYPE_CONNACK:
            return xy_mqtt_handle_connack(mqtt, data, len);

        case XY_MQTT_TYPE_PUBLISH:
            return xy_mqtt_handle_publish(mqtt, data, len);

        case XY_MQTT_TYPE_PUBACK:
            return xy_mqtt_handle_puback(mqtt, data, len);

        case XY_MQTT_TYPE_SUBACK:
            return xy_mqtt_handle_suback(mqtt, data, len);

        case XY_MQTT_TYPE_UNSUBACK:
            return xy_mqtt_handle_unsuback(mqtt, data, len);

        case XY_MQTT_TYPE_PINGRESP:
            return xy_mqtt_handle_pingresp(mqtt, data, len);

        case XY_MQTT_TYPE_DISCONNECT:
            mqtt->state = XY_MQTT_STATE_DISCONNECTED;
            if (mqtt->config.disconnected_cb) {
                mqtt->config.disconnected_cb(mqtt, 0, mqtt->config.user_data);
            }
            return XY_MQTT_OK;

        default:
            return XY_MQTT_ERR_UNSUPPORTED_PACKET;
    }
}

/*============================================================================
 * MQTT Client API Implementation
 *============================================================================*/

xy_mqtt_client_t *xy_mqtt_client_new(const xy_mqtt_config_t *config)
{
    if (!config || !config->send || !config->recv) {
        return NULL;
    }

    xy_mqtt_client_t *mqtt = (xy_mqtt_client_t *)malloc(sizeof(xy_mqtt_client_t));
    if (!mqtt) {
        return NULL;
    }

    memset(mqtt, 0, sizeof(xy_mqtt_client_t));

    /* Copy configuration */
    memcpy(&mqtt->config, config, sizeof(xy_mqtt_config_t));

    /* Set defaults */
    if (mqtt->config.keepalive == 0) {
        mqtt->config.keepalive = XY_MQTT_DEFAULT_KEEPALIVE;
    }
    if (mqtt->config.tx_buffer_size == 0) {
        mqtt->config.tx_buffer_size = XY_MQTT_DEFAULT_TX_BUFFER_SIZE;
    }
    if (mqtt->config.rx_buffer_size == 0) {
        mqtt->config.rx_buffer_size = XY_MQTT_DEFAULT_RX_BUFFER_SIZE;
    }

    /* Allocate buffers */
    mqtt->tx_buffer = (uint8_t *)malloc(mqtt->config.tx_buffer_size);
    mqtt->rx_buffer = (uint8_t *)malloc(mqtt->config.rx_buffer_size);
    if (!mqtt->tx_buffer || !mqtt->rx_buffer) {
        free(mqtt->tx_buffer);
        free(mqtt->rx_buffer);
        free(mqtt);
        return NULL;
    }

    /* Initialize state */
    mqtt->state = XY_MQTT_STATE_DISCONNECTED;
    mqtt->keepalive_interval = mqtt->config.keepalive;
    mqtt->next_packet_id = 1;

    return mqtt;
}

void xy_mqtt_client_delete(xy_mqtt_client_t *mqtt)
{
    if (!mqtt) {
        return;
    }

    /* Disconnect if connected */
    if (mqtt->state == XY_MQTT_STATE_CONNECTED) {
        xy_mqtt_disconnect(mqtt);
    }

    /* Free pending acks */
    while (mqtt->pending_acks) {
        void *ctx = xy_mqtt_remove_pending_ack(mqtt, mqtt->pending_acks->packet_id);
        (void)ctx;
    }

    /* Free subscriptions */
    while (mqtt->subscriptions) {
        xy_mqtt_subscription_t *sub = mqtt->subscriptions;
        mqtt->subscriptions = sub->next;
        free(sub);
    }

    /* Free buffers */
    free(mqtt->tx_buffer);
    free(mqtt->rx_buffer);

    /* Free client */
    free(mqtt);
}

xy_mqtt_err_t xy_mqtt_connect_with_will(xy_mqtt_client_t *mqtt,
                                        const char *client_id,
                                        const char *username,
                                        const char *password,
                                        const char *will_topic,
                                        const char *will_message,
                                        uint8_t will_qos,
                                        uint8_t will_retain)
{
    if (!mqtt || mqtt->state != XY_MQTT_STATE_DISCONNECTED) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    /* Store will parameters */
    mqtt->will_flag = (will_topic && will_message) ? 1 : 0;
    if (will_topic) {
        strncpy(mqtt->will_topic, will_topic, XY_MQTT_MAX_TOPIC_LEN - 1);
        mqtt->will_topic[XY_MQTT_MAX_TOPIC_LEN - 1] = '\0';
    }
    if (will_message) {
        strncpy(mqtt->will_message, will_message, XY_MQTT_MAX_PAYLOAD_LEN - 1);
        mqtt->will_message[XY_MQTT_MAX_PAYLOAD_LEN - 1] = '\0';
    }
    mqtt->will_qos = will_qos;
    mqtt->will_retain = will_retain;

    /* Update state */
    mqtt->state = XY_MQTT_STATE_MQTT_CONNECTING;

    /* Send CONNECT packet */
    return xy_mqtt_send_connect(mqtt, client_id, username, password,
                                will_topic, will_message, will_qos, will_retain,
                                mqtt->config.clean_session);
}

xy_mqtt_err_t xy_mqtt_connect(xy_mqtt_client_t *mqtt,
                              const char *client_id,
                              const char *username,
                              const char *password)
{
    return xy_mqtt_connect_with_will(mqtt, client_id, username, password,
                                     NULL, NULL, 0, 0);
}

xy_mqtt_err_t xy_mqtt_disconnect(xy_mqtt_client_t *mqtt)
{
    if (!mqtt) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED &&
        mqtt->state != XY_MQTT_STATE_MQTT_CONNECTING) {
        return XY_MQTT_ERR_NOT_CONNECTED;
    }

    mqtt->state = XY_MQTT_STATE_DISCONNECTING;

    /* Send DISCONNECT packet */
    if (mqtt->state == XY_MQTT_STATE_CONNECTED) {
        xy_mqtt_send_disconnect(mqtt);
    }

    mqtt->state = XY_MQTT_STATE_DISCONNECTED;

    if (mqtt->config.disconnected_cb) {
        mqtt->config.disconnected_cb(mqtt, 0, mqtt->config.user_data);
    }

    return XY_MQTT_OK;
}

bool xy_mqtt_is_connected(xy_mqtt_client_t *mqtt)
{
    return mqtt && (mqtt->state == XY_MQTT_STATE_CONNECTED);
}

xy_mqtt_err_t xy_mqtt_publish(xy_mqtt_client_t *mqtt,
                              const char *topic,
                              const uint8_t *payload,
                              size_t payload_len,
                              uint8_t qos,
                              uint8_t retain,
                              uint16_t *packet_id)
{
    if (!mqtt || !topic || !payload) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED) {
        return XY_MQTT_ERR_NOT_CONNECTED;
    }

    if (qos > XY_MQTT_QOS_1) {
        qos = XY_MQTT_QOS_1; /* Only QoS 0 and 1 supported */
    }

    if (strlen(topic) >= XY_MQTT_MAX_TOPIC_LEN) {
        return XY_MQTT_ERR_TOPIC_TOO_LONG;
    }

    uint16_t pid = packet_id ? *packet_id : 0;

    return xy_mqtt_send_publish(mqtt, topic, payload, payload_len, qos, retain, 0, &pid);
}

xy_mqtt_err_t xy_mqtt_subscribe(xy_mqtt_client_t *mqtt,
                                const char *topic_filter,
                                uint8_t qos,
                                uint16_t *packet_id)
{
    if (!mqtt || !topic_filter) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED) {
        return XY_MQTT_ERR_NOT_CONNECTED;
    }

    if (strlen(topic_filter) >= XY_MQTT_MAX_TOPIC_LEN) {
        return XY_MQTT_ERR_TOPIC_TOO_LONG;
    }

    if (qos > XY_MQTT_QOS_2) {
        qos = XY_MQTT_QOS_2;
    }

    uint16_t pid = 0;

    xy_mqtt_err_t err = xy_mqtt_send_subscribe(mqtt, topic_filter, qos, &pid);
    if (err == XY_MQTT_OK && packet_id) {
        *packet_id = pid;
    }

    return err;
}

xy_mqtt_err_t xy_mqtt_unsubscribe(xy_mqtt_client_t *mqtt,
                                  const char *topic_filter,
                                  uint16_t *packet_id)
{
    if (!mqtt || !topic_filter) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED) {
        return XY_MQTT_ERR_NOT_CONNECTED;
    }

    uint16_t pid = 0;

    xy_mqtt_err_t err = xy_mqtt_send_unsubscribe(mqtt, topic_filter, &pid);
    if (err == XY_MQTT_OK && packet_id) {
        *packet_id = pid;
    }

    return err;
}

xy_mqtt_err_t xy_mqtt_process(xy_mqtt_client_t *mqtt, uint32_t timeout_ms)
{
    if (!mqtt) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED &&
        mqtt->state != XY_MQTT_STATE_MQTT_CONNECTING) {
        return XY_MQTT_ERR_NOT_CONNECTED;
    }

    /* Try to receive data */
    int received = mqtt->config.recv(mqtt->config.transport_context,
                                     mqtt->rx_buffer,
                                     mqtt->rx_buffer_size,
                                     timeout_ms);

    if (received < 0) {
        mqtt->state = XY_MQTT_STATE_DISCONNECTED;
        mqtt->last_error = XY_MQTT_ERR_TCP_DISCONNECTED;

        if (mqtt->config.disconnected_cb) {
            mqtt->config.disconnected_cb(mqtt, XY_MQTT_ERR_TCP_DISCONNECTED,
                                         mqtt->config.user_data);
        }

        return XY_MQTT_ERR_TCP_DISCONNECTED;
    }

    if (received == 0) {
        return XY_MQTT_OK; /* Timeout, no data */
    }

    /* Parse received packet */
    return xy_mqtt_parse_packet(mqtt, mqtt->rx_buffer, (size_t)received);
}

xy_mqtt_err_t xy_mqtt_keepalive_check(xy_mqtt_client_t *mqtt)
{
    if (!mqtt) {
        return XY_MQTT_ERR_INVALID_PARAM;
    }

    if (mqtt->state != XY_MQTT_STATE_CONNECTED) {
        return XY_MQTT_OK;
    }

    if (mqtt->keepalive_interval == 0) {
        return XY_MQTT_OK;
    }

    uint32_t now = xy_mqtt_get_time_ms();
    uint32_t elapsed = now - mqtt->last_send_time;

    /* Send PING if keep-alive timeout reached (1.5x interval) */
    if (elapsed >= (mqtt->keepalive_interval * 1500)) {
        return xy_mqtt_send_pingreq(mqtt);
    }

    return XY_MQTT_OK;
}

xy_mqtt_state_t xy_mqtt_get_state(xy_mqtt_client_t *mqtt)
{
    return mqtt ? mqtt->state : XY_MQTT_STATE_DISCONNECTED;
}

xy_mqtt_err_t xy_mqtt_get_error(xy_mqtt_client_t *mqtt)
{
    return mqtt ? mqtt->last_error : XY_MQTT_ERR_INVALID_PARAM;
}

/*============================================================================
 * Utility Functions Implementation
 *============================================================================*/

const char *xy_mqtt_connack_rc_string(uint8_t rc)
{
    switch (rc) {
        case XY_MQTT_CONNACK_RC_ACCEPTED:
            return "Connection accepted";
        case XY_MQTT_CONNACK_RC_UNACCEPTABLE_PROTO:
            return "Unacceptable protocol version";
        case XY_MQTT_CONNACK_RC_IDENTIFIER_REJECTED:
            return "Identifier rejected";
        case XY_MQTT_CONNACK_RC_SERVER_UNAVAILABLE:
            return "Server unavailable";
        case XY_MQTT_CONNACK_RC_BAD_USERNAME_PASS:
            return "Bad username or password";
        case XY_MQTT_CONNACK_RC_NOT_AUTHORIZED:
            return "Not authorized";
        default:
            return "Unknown error";
    }
}
