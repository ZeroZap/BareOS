#include <stdio.h>
#include <string.h>

#include "at_chat.h"
#include "xy_mem.h"
#include "xy_tick.h"
#include "xy_wifi.h"

XY_MEM_POOL_DECLARE(s_test_mem, 8192u);

static int s_checks;
static int s_failures;
static unsigned char s_tx[1024];
static unsigned int s_tx_len;
static unsigned char s_rx[512];
static unsigned int s_rx_len;
static unsigned int s_rx_pos;

#define CHECK(condition) \
    do { \
        s_checks++; \
        if (!(condition)) { \
            s_failures++; \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        } \
    } while (0)

#define CHECK_STR(actual, expected) \
    do { \
        s_checks++; \
        if (strcmp((actual), (expected)) != 0) { \
            s_failures++; \
            fprintf(stderr, "FAIL %s:%d: got '%s' want '%s'\n", \
                    __FILE__, __LINE__, (actual), (expected)); \
        } \
    } while (0)

static unsigned int fake_write(const void *buf, unsigned int len)
{
    unsigned int copy = len;

    if (copy > sizeof(s_tx) - s_tx_len) copy = sizeof(s_tx) - s_tx_len;
    memcpy(&s_tx[s_tx_len], buf, copy);
    s_tx_len += copy;
    return copy;
}

static unsigned int fake_read(void *buf, unsigned int len)
{
    unsigned int available = s_rx_len - s_rx_pos;

    if (available > len) available = len;
    if (available != 0u) {
        memcpy(buf, &s_rx[s_rx_pos], available);
        s_rx_pos += available;
    }
    return available;
}

static void inject_bytes(const void *data, unsigned int len)
{
    CHECK(len <= sizeof(s_rx));
    if (len > sizeof(s_rx)) return;
    memcpy(s_rx, data, len);
    s_rx_len = len;
    s_rx_pos = 0u;
}

static void inject(const char *text)
{
    inject_bytes(text, (unsigned int)strlen(text));
}

static void process(at_obj_t *at)
{
    at_obj_process(at);
    xy_tick_advance(1u);
}

static void expect_tx(const char *text)
{
    unsigned int len = (unsigned int)strlen(text);

    CHECK(s_tx_len >= len);
    if (s_tx_len >= len) CHECK(memcmp(&s_tx[s_tx_len - len], text, len) == 0);
}

int main(void)
{
    static const at_adapter_t adapter = {
        .lock = NULL,
        .unlock = NULL,
        .write = fake_write,
        .read = fake_read,
        .error = NULL,
        .debug = NULL,
#if AT_URC_WARCH_EN
        .urc_bufsize = 256u,
#endif
        .recv_bufsize = 256u,
        .rx_pending = NULL,
        .tx_idle = NULL,
    };
    static const unsigned char ipd_payload[] = {'A', '\0', 'B', '\r', '\n'};
    static const unsigned char expected_payload[] = {'A', '\0', 'B', '\r', '\n'};
    static const unsigned char send_payload[] = {'X', '\0', 'Y'};
    unsigned char received[8] = {0};
    const xy_wifi_t *info;
    const xy_wifi_ap_t *ap;
    at_obj_t *at;

    XY_MEM_POOL_INIT(s_test_mem);
    xy_tick_init();
    at = at_obj_create(&adapter);
    CHECK(at != NULL);
    if (at == NULL) return 1;
    xy_wifi_init(at);

    CHECK(xy_wifi_op_done());
    CHECK(!xy_wifi_busy());
    CHECK(xy_wifi_last_error() == WIFI_ERR_OK);
    CHECK(xy_wifi_start_init());
    CHECK(!xy_wifi_start_connect("busy", "busy"));
    CHECK(xy_wifi_last_error() == WIFI_ERR_BUSY);
    process(at);
    expect_tx("ATE0\r\n");
    inject("\r\nOK\r\n");
    process(at);
    expect_tx("AT+CWMODE=1\r\n");
    inject("\r\nOK\r\n");
    process(at);
    expect_tx("AT+CIPMUX=1\r\n");
    inject("\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_done());
    CHECK(xy_wifi_op_ok());
    CHECK(xy_wifi_get_state() == WIFI_ST_INIT);
    CHECK_STR(xy_wifi_last_error_str(), "OK");

    CHECK(xy_wifi_start_query_state());
    process(at);
    expect_tx("AT+CWSTATE?\r\n");
    inject("\r\n+CWSTATE:3,\"LabAP\"\r\n\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_ok());
    CHECK(xy_wifi_is_connected());
    CHECK_STR(xy_wifi_get_info()->ssid, "LabAP");

    CHECK(xy_wifi_start_query_ip());
    process(at);
    expect_tx("AT+CIPSTA?\r\n");
    inject("\r\n+CIPSTA:ip,\"192.168.1.22\"\r\n+CIPSTA:gateway,\"192.168.1.1\"\r\n+CIPSTA:netmask,\"255.255.255.0\"\r\n\r\nOK\r\n");
    process(at);
    info = xy_wifi_get_info();
    CHECK(xy_wifi_op_ok());
    CHECK(xy_wifi_has_ip());
    CHECK_STR(info->ip, "192.168.1.22");
    CHECK_STR(info->gateway, "192.168.1.1");
    CHECK_STR(info->netmask, "255.255.255.0");

    CHECK(xy_wifi_start_query_mac());
    process(at);
    expect_tx("AT+CIPSTAMAC?\r\n");
    inject("\r\n+CIPSTAMAC:\"de:ad:be:ef:00:01\"\r\n\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_ok());
    CHECK_STR(xy_wifi_get_info()->mac, "de:ad:be:ef:00:01");

    CHECK(xy_wifi_start_query_rssi());
    process(at);
    expect_tx("AT+CWJAP?\r\n");
    inject("\r\n+CWJAP:\"LabAP\",\"aa:bb:cc:dd:ee:ff\",6,-55\r\n\r\nOK\r\n");
    process(at);
    info = xy_wifi_get_info();
    CHECK(xy_wifi_op_ok());
    CHECK_STR(info->ssid, "LabAP");
    CHECK_STR(info->bssid, "aa:bb:cc:dd:ee:ff");
    CHECK(info->rssi_dbm == -55);

    CHECK(xy_wifi_start_scan());
    process(at);
    expect_tx("AT+CWLAP\r\n");
    inject("\r\n+CWLAP:(3,\"LabAP\",-55,\"aa:bb:cc:dd:ee:ff\",6)\r\n+CWLAP:(0,\"OpenAP\",-80,\"11:22:33:44:55:66\",11)\r\n\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_ok());
    CHECK(xy_wifi_scan_count() == 2);
    ap = xy_wifi_scan_result(0);
    CHECK(ap != NULL);
    if (ap != NULL) {
        CHECK_STR(ap->ssid, "LabAP");
        CHECK_STR(ap->bssid, "aa:bb:cc:dd:ee:ff");
        CHECK(ap->rssi_dbm == -55);
        CHECK(ap->channel == 6);
        CHECK(ap->auth == WIFI_AUTH_WPA2_PSK);
    }
    CHECK(xy_wifi_scan_result(2) == NULL);

    CHECK(xy_wifi_start_set_dhcp(true));
    process(at);
    expect_tx("AT+CWDHCP=1,1\r\n");
    inject("\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_ok());

    CHECK(!xy_wifi_start_set_static_ip("999.1.1.1", "192.168.1.1", "255.255.255.0"));
    CHECK(xy_wifi_last_error() == WIFI_ERR_PARAM);
    CHECK(xy_wifi_start_set_static_ip("192.168.1.50", "192.168.1.1", "255.255.255.0"));
    process(at);
    expect_tx("AT+CIPSTA=\"192.168.1.50\",\"192.168.1.1\",\"255.255.255.0\"\r\n");
    inject("\r\nOK\r\n");
    process(at);
    info = xy_wifi_get_info();
    CHECK(xy_wifi_op_ok());
    CHECK_STR(info->ip, "192.168.1.50");

    CHECK(!xy_wifi_start_set_dns("bad", NULL));
    CHECK(xy_wifi_last_error() == WIFI_ERR_PARAM);
    CHECK(xy_wifi_start_set_dns("8.8.8.8", "1.1.1.1"));
    process(at);
    expect_tx("AT+CIPDNS=1,\"8.8.8.8\",\"1.1.1.1\"\r\n");
    inject("\r\nOK\r\n");
    process(at);
    info = xy_wifi_get_info();
    CHECK(xy_wifi_op_ok());
    CHECK_STR(info->dns1, "8.8.8.8");
    CHECK_STR(info->dns2, "1.1.1.1");

    CHECK(!xy_wifi_start_sock_open(-1, "TCP", "example.com", 80u));
    CHECK(!xy_wifi_start_sock_open(0, NULL, "example.com", 80u));
    CHECK(xy_wifi_start_sock_open(0, "TCP", "example.com", 80u));
    process(at);
    expect_tx("AT+CIPSTART=0,\"TCP\",\"example.com\",80\r\n");
    inject("\r\n0,CONNECT\r\n\r\nOK\r\n");
    process(at);
    CHECK(xy_wifi_op_done());
    CHECK(xy_wifi_op_ok());
    CHECK(xy_wifi_sock_is_open(0));
    process(at);

    inject("\r\n+IPD,0,5:");
    process(at);
    inject_bytes(ipd_payload, sizeof(ipd_payload));
    process(at);
    CHECK(xy_wifi_sock_recv(0, received, sizeof(received)) == 5);
    CHECK(memcmp(received, expected_payload, sizeof(expected_payload)) == 0);

    CHECK(xy_wifi_start_sock_send(0, send_payload, sizeof(send_payload)));
    process(at);
    expect_tx("AT+CIPSEND=0,3\r\n");
    inject("\r\n> ");
    process(at);
    CHECK(s_tx_len >= sizeof(send_payload));
    CHECK(memcmp(&s_tx[s_tx_len - sizeof(send_payload)], send_payload,
                 sizeof(send_payload)) == 0);
    inject("\r\nSEND OK\r\n");
    process(at);
    CHECK(xy_wifi_op_done());
    CHECK(xy_wifi_op_ok());

    inject("\r\nWIFI DISCONNECT\r\n");
    process(at);
    CHECK(xy_wifi_get_state() == WIFI_ST_INIT);
    CHECK(!xy_wifi_sock_is_open(0));
    CHECK(!xy_wifi_start_sock_send(0, send_payload, sizeof(send_payload)));

    CHECK(!xy_wifi_start_connect(NULL, "pass"));
    CHECK(!xy_wifi_start_mqtt_publish("topic", send_payload,
                                      sizeof(send_payload), 3u));

    at_obj_destroy(at);
    fprintf(stderr, "ESP32 ESP-AT tests: %d checks, %d failures\n",
            s_checks, s_failures);
    return s_failures;
}
