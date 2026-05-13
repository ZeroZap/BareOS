/**
 * @file example.c
 * @brief XY Broker usage examples
 */

#include "xy_broker.h"
#include <stdio.h>
#include <string.h>

/* ==================== Example 1: Basic Point-to-Point Messaging
 * ==================== */

static int sensor_msg_handler(const xy_broker_msg_t *msg, void *user_data)
{
    printf("[SENSOR] Received message ID: 0x%04X from server: 0x%04X\n",
           msg->msg_id, msg->src_server);

    switch (msg->msg_id) {
    case XY_BROKER_MSG_SENSOR_CALIBRATE:
        printf("[SENSOR] Calibrating sensor...\n");
        break;
    case XY_BROKER_MSG_SENSOR_CONFIG:
        printf("[SENSOR] Configuring sensor...\n");
        break;
    default:
        printf("[SENSOR] Unknown message\n");
        break;
    }

    return 0;
}

void example_basic_messaging(void)
{
    printf("\n=== Example 1: Basic Point-to-Point Messaging ===\n");

    // Initialize broker
    xy_broker_init();

    // Register sensor server
    xy_broker_register_server(
        XY_BROKER_SERVER_SENSOR, sensor_msg_handler, NULL);

    // Send calibration command
    xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_SENSOR,
                       XY_BROKER_MSG_SENSOR_CALIBRATE, NULL, 0,
                       XY_BROKER_PRIORITY_NORMAL);

    // Process messages
    xy_broker_process_msgs(XY_BROKER_SERVER_SENSOR, 0);

    // Cleanup
    xy_broker_deinit();
}

/* ==================== Example 2: Pub/Sub Pattern ==================== */

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

static int storage_topic_handler(const xy_broker_msg_t *msg, void *user_data)
{
    if (msg->topic_id == XY_BROKER_TOPIC_SENSOR_DATA) {
        sensor_data_t *data = (sensor_data_t *)msg->payload;
        printf("[STORAGE] Storing sensor data: T=%.1f°C, H=%.1f%%\n",
               data->temperature, data->humidity);
    }
    return 0;
}

static int display_topic_handler(const xy_broker_msg_t *msg, void *user_data)
{
    if (msg->topic_id == XY_BROKER_TOPIC_SENSOR_DATA) {
        sensor_data_t *data = (sensor_data_t *)msg->payload;
        printf("[DISPLAY] Showing sensor data: T=%.1f°C, H=%.1f%%\n",
               data->temperature, data->humidity);
    }
    return 0;
}

void example_pub_sub(void)
{
    printf("\n=== Example 2: Publish/Subscribe Pattern ===\n");

    xy_broker_init();

    // Create topic
    xy_broker_create_topic(XY_BROKER_TOPIC_SENSOR_DATA);

    // Subscribe multiple servers
    xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA, XY_BROKER_SERVER_STORAGE,
                        storage_topic_handler, NULL);

    xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA, XY_BROKER_SERVER_DISPLAY,
                        display_topic_handler, NULL);

    // Publish data
    sensor_data_t data = { .temperature = 25.5,
                           .humidity    = 60.0,
                           .timestamp   = 12345 };

    printf("[SENSOR] Publishing sensor data...\n");
    xy_broker_publish(XY_BROKER_SERVER_SENSOR, XY_BROKER_TOPIC_SENSOR_DATA,
                      XY_BROKER_MSG_SENSOR_DATA, &data, sizeof(data),
                      XY_BROKER_PRIORITY_NORMAL);

    xy_broker_deinit();
}

/* ==================== Example 3: Priority Messaging ==================== */

static int priority_handler(const xy_broker_msg_t *msg, void *user_data)
{
    const char *priority_name[] = { "LOW", "NORMAL", "HIGH", "CRITICAL" };
    printf("[HANDLER] Message priority: %s, ID: 0x%04X\n",
           priority_name[msg->priority], msg->msg_id);
    return 0;
}

void example_priority(void)
{
    printf("\n=== Example 3: Priority Messaging ===\n");

    xy_broker_init();
    xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, priority_handler, NULL);

    // Send messages with different priorities
    printf("Sending messages with different priorities...\n");

    xy_broker_send_msg(XY_BROKER_SERVER_TIMER, XY_BROKER_SERVER_SYSTEM,
                       XY_BROKER_MSG_SYSTEM_STATUS, NULL, 0,
                       XY_BROKER_PRIORITY_LOW);

    xy_broker_send_msg(XY_BROKER_SERVER_TIMER, XY_BROKER_SERVER_SYSTEM,
                       XY_BROKER_MSG_SYSTEM_CONFIG, NULL, 0,
                       XY_BROKER_PRIORITY_NORMAL);

    xy_broker_send_msg(XY_BROKER_SERVER_TIMER, XY_BROKER_SERVER_SYSTEM,
                       XY_BROKER_MSG_SYSTEM_RESET, NULL, 0,
                       XY_BROKER_PRIORITY_CRITICAL);

    xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 0);

    xy_broker_deinit();
}

/* ==================== Example 4: Message Queue Management ====================
 */

void example_queue_management(void)
{
    printf("\n=== Example 4: Queue Management ===\n");

    xy_broker_init();
    xy_broker_register_server(XY_BROKER_SERVER_STORAGE, NULL, NULL);

    // Fill queue with messages
    for (int i = 0; i < 10; i++) {
        uint8_t data = i;
        xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_STORAGE,
                           XY_BROKER_MSG_STORAGE_WRITE, &data, sizeof(data),
                           XY_BROKER_PRIORITY_NORMAL);
    }

    // Check pending count
    int pending = xy_broker_get_pending_count(XY_BROKER_SERVER_STORAGE);
    printf("Pending messages: %d\n", pending);

    // Clear queue
    xy_broker_clear_queue(XY_BROKER_SERVER_STORAGE);
    pending = xy_broker_get_pending_count(XY_BROKER_SERVER_STORAGE);
    printf("After clear: %d\n", pending);

    xy_broker_deinit();
}

/* ==================== Example 5: Statistics ==================== */

void example_statistics(void)
{
    printf("\n=== Example 5: Broker Statistics ===\n");

    xy_broker_init();

    // Register multiple servers
    xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, NULL, NULL);
    xy_broker_register_server(XY_BROKER_SERVER_SENSOR, NULL, NULL);
    xy_broker_register_server(XY_BROKER_SERVER_STORAGE, NULL, NULL);

    // Send some messages
    for (int i = 0; i < 5; i++) {
        xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_SENSOR,
                           XY_BROKER_MSG_SENSOR_DATA, NULL, 0,
                           XY_BROKER_PRIORITY_NORMAL);
    }

    // Get statistics
    xy_broker_stats_t stats;
    xy_broker_get_stats(&stats);

    printf("Broker Statistics:\n");
    printf("  Active servers: %lu\n", stats.active_servers);
    printf("  Total messages sent: %lu\n", stats.total_msg_sent);
    printf("  Total messages delivered: %lu\n", stats.total_msg_delivered);
    printf("  Total messages dropped: %lu\n", stats.total_msg_dropped);

    xy_broker_deinit();
}

/* ==================== Example 6: Custom Domain Example ==================== */

// Custom server IDs for audio system
#define MY_SERVER_AUDIO_DSP    (XY_BROKER_SERVER_USER_BASE + 1)
#define MY_SERVER_AUDIO_CODEC  (XY_BROKER_SERVER_USER_BASE + 2)
#define MY_SERVER_AUDIO_OUTPUT (XY_BROKER_SERVER_USER_BASE + 3)

// Custom message IDs
#define MY_MSG_AUDIO_PLAY   (XY_BROKER_MSG_USER_BASE + 1)
#define MY_MSG_AUDIO_STOP   (XY_BROKER_MSG_USER_BASE + 2)
#define MY_MSG_AUDIO_VOLUME (XY_BROKER_MSG_USER_BASE + 3)

// Custom topic IDs
#define MY_TOPIC_AUDIO_EVENT (XY_BROKER_TOPIC_USER_BASE + 1)

typedef struct {
    uint8_t volume;
    uint8_t mute;
} audio_volume_t;

static int audio_dsp_handler(const xy_broker_msg_t *msg, void *user_data)
{
    switch (msg->msg_id) {
    case MY_MSG_AUDIO_PLAY:
        printf("[AUDIO_DSP] Start playing audio\n");
        break;
    case MY_MSG_AUDIO_STOP:
        printf("[AUDIO_DSP] Stop playing audio\n");
        break;
    case MY_MSG_AUDIO_VOLUME: {
        audio_volume_t *vol = (audio_volume_t *)msg->payload;
        printf("[AUDIO_DSP] Set volume: %d%%, mute: %s\n", vol->volume,
               vol->mute ? "yes" : "no");
        break;
    }
    }
    return 0;
}

void example_custom_domain(void)
{
    printf("\n=== Example 6: Custom Audio Domain ===\n");

    xy_broker_init();

    // Register audio DSP server
    xy_broker_register_server(MY_SERVER_AUDIO_DSP, audio_dsp_handler, NULL);

    // Send play command
    xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, MY_SERVER_AUDIO_DSP,
                       MY_MSG_AUDIO_PLAY, NULL, 0, XY_BROKER_PRIORITY_NORMAL);

    // Send volume command
    audio_volume_t vol = { .volume = 75, .mute = 0 };
    xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, MY_SERVER_AUDIO_DSP,
                       MY_MSG_AUDIO_VOLUME, &vol, sizeof(vol),
                       XY_BROKER_PRIORITY_NORMAL);

    // Process messages
    xy_broker_process_msgs(MY_SERVER_AUDIO_DSP, 0);

    xy_broker_deinit();
}

/* ==================== Main Function ==================== */

int main(void)
{
    printf("===========================================\n");
    printf("    XY Broker System Examples\n");
    printf("===========================================\n");

    example_basic_messaging();
    example_pub_sub();
    example_priority();
    example_queue_management();
    example_statistics();
    example_custom_domain();

    printf("\n===========================================\n");
    printf("    All examples completed!\n");
    printf("===========================================\n");

    return 0;
}
