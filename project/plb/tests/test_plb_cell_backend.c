#include <stdio.h>
#include <string.h>

#include "at_chat.h"
#include "plb_comm.h"
#include "plb_comm_cell_backend.h"
#include "plb_pc_bsp.h"
#include "xy_gnss.h"
#include "xy_log.h"
#include "xy_mem.h"

static int s_failures;
static int s_cases;

#define CHECK_TRUE(cond) \
    do { \
        s_cases++; \
        if (!(cond)) { \
            s_failures++; \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        } \
    } while (0)

#define CHECK_EQ(actual, expected) \
    do { \
        int a_ = (int)(actual); \
        int e_ = (int)(expected); \
        s_cases++; \
        if (a_ != e_) { \
            s_failures++; \
            fprintf(stderr, "FAIL %s:%d: got %d want %d\n", __FILE__, __LINE__, a_, e_); \
        } \
    } while (0)

static char s_tx[512];
static unsigned int s_tx_len;
static const char *s_rx;
static unsigned int s_rx_len;
static unsigned int s_rx_pos;

static unsigned int fake_write(const void *buf, unsigned int len)
{
    unsigned int copy = len;
    if (s_tx_len + copy >= sizeof(s_tx)) {
        copy = (unsigned int)(sizeof(s_tx) - s_tx_len - 1u);
    }
    memcpy(&s_tx[s_tx_len], buf, copy);
    s_tx_len += copy;
    s_tx[s_tx_len] = '\0';
    return len;
}

static unsigned int fake_read(void *buf, unsigned int len)
{
    unsigned int avail;
    if (!s_rx) return 0;
    avail = s_rx_len - s_rx_pos;
    if (avail > len) avail = len;
    if (avail > 0) {
        memcpy(buf, s_rx + s_rx_pos, avail);
        s_rx_pos += avail;
    }
    return avail;
}

static void fake_inject(const char *resp)
{
    s_rx = resp;
    s_rx_len = (unsigned int)strlen(resp);
    s_rx_pos = 0;
}

static void drive(at_obj_t *at, unsigned int step_ms)
{
    at_obj_process(at);
    plb_comm_process();
    plb_pc_tick_advance(step_ms);
}

static void test_cell_backend_send_ok(void)
{
    static at_adapter_t adap = {
        NULL,
        NULL,
        fake_write,
        fake_read,
        NULL,
        NULL,
#if AT_URC_WARCH_EN
        128,
#endif
        256,
    };
    xy_gnss_pos_t pos;

    memset(&pos, 0, sizeof(pos));
    pos.valid = true;
    pos.lat_1e7 = 312375100;
    pos.lon_1e7 = 1215242183;
    pos.hour = 8;
    pos.minute = 59;
    pos.second = 59;

    plb_pc_bsp_init();
    xy_log_init();
    plb_comm_init();

    void *probe = xy_malloc(64);
    CHECK_TRUE(probe != NULL);
    xy_free(probe);

    at_obj_t *at = at_obj_create(&adap);
    CHECK_TRUE(at != NULL);
    if (!at) return;

    plb_comm_cell_backend_init(at, CELL_MDM_EC2X, 0);
    plb_comm_set_backend(plb_comm_cell_backend());

    CHECK_TRUE(plb_comm_start_distress_send(&pos, 1));
    drive(at, 10);
    CHECK_TRUE(strstr(s_tx, "AT+QISEND=0,") != NULL);

    fake_inject(">\r\n");
    drive(at, 10);
    CHECK_TRUE(strstr(s_tx, "PLB SOS") != NULL);

    fake_inject("SEND OK\r\n");
    for (int i = 0; i < 4 && !plb_comm_done(); i++) {
        drive(at, 10);
    }

    CHECK_TRUE(plb_comm_done());
    CHECK_TRUE(plb_comm_ok());
    CHECK_EQ(plb_comm_result(), PLB_COMM_RESULT_OK);

    at_obj_destroy(at);
}

int main(void)
{
    test_cell_backend_send_ok();
    fprintf(stderr, "PLB cell backend tests: %d checks, %d failures\n", s_cases, s_failures);
    return s_failures;
}
