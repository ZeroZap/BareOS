#include <stdio.h>
#include <string.h>

#include "plb_app.h"
#include "plb_comm.h"
#include "plb_pc_bsp.h"
#include "xy_evtlog.h"
#include "xy_log.h"

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

static void run_plb_flow(void)
{
    plb_pc_bsp_init();
    xy_log_init();
    plb_app_init();
    plb_app_request_sos();

    for (int i = 0; i < 16 && plb_app_step(); i++) {
        plb_pc_tick_advance(1000u);
    }
}

static void run_plb_flow_with_results(const plb_comm_result_t *results, uint8_t count)
{
    plb_pc_bsp_init();
    xy_log_init();
    plb_app_init();
    plb_comm_mock_set_result_sequence(results, count, 1);
    plb_app_request_sos();

    for (int i = 0; i < 32 && plb_app_step(); i++) {
        plb_pc_tick_advance(1000u);
    }
}

static void test_direct_success(void)
{
    xy_evtlog_entry_t entries[8];

    plb_comm_mock_set_result(PLB_COMM_RESULT_OK, 1);
    run_plb_flow();

    int count = xy_evtlog_read_last(entries, 8);
    CHECK_EQ(count, 4);
    CHECK_EQ(entries[0].event_type, XY_EVT_POWER_ON);
    CHECK_EQ(entries[1].event_type, XY_EVT_SOS_TRIGGERED);
    CHECK_EQ(entries[2].event_type, XY_EVT_GNSS_FIX);
    CHECK_EQ(entries[3].event_type, XY_EVT_POS_TX_OK);
    CHECK_EQ(entries[3].status, 1);
    CHECK_TRUE(entries[2].lat_1e7 != 0);
    CHECK_TRUE(entries[2].lon_1e7 != 0);
    CHECK_EQ(plb_comm_send_count(), 1);
    CHECK_TRUE(plb_comm_done());
    CHECK_TRUE(plb_comm_ok());
    CHECK_TRUE(strstr(plb_comm_last_message(), "PLB SOS") != NULL);
}

static void test_retry_then_success(void)
{
    static const plb_comm_result_t results[] = {
        PLB_COMM_RESULT_SEND_FAIL,
        PLB_COMM_RESULT_TIMEOUT,
        PLB_COMM_RESULT_OK,
    };
    xy_evtlog_entry_t entries[8];

    run_plb_flow_with_results(results, 3);

    int count = xy_evtlog_read_last(entries, 8);
    CHECK_EQ(count, 6);
    CHECK_EQ(entries[0].event_type, XY_EVT_POWER_ON);
    CHECK_EQ(entries[1].event_type, XY_EVT_SOS_TRIGGERED);
    CHECK_EQ(entries[2].event_type, XY_EVT_GNSS_FIX);
    CHECK_EQ(entries[3].event_type, XY_EVT_POS_TX_FAIL);
    CHECK_EQ(entries[3].status, 1);
    CHECK_EQ(entries[4].event_type, XY_EVT_POS_TX_FAIL);
    CHECK_EQ(entries[4].status, 2);
    CHECK_EQ(entries[5].event_type, XY_EVT_POS_TX_OK);
    CHECK_EQ(entries[5].status, 3);
    CHECK_EQ(plb_comm_send_count(), 3);
    CHECK_TRUE(plb_comm_done());
    CHECK_TRUE(plb_comm_ok());
    CHECK_EQ(plb_comm_result(), PLB_COMM_RESULT_OK);
}

int main(void)
{
    test_direct_success();
    test_retry_then_success();

    fprintf(stderr, "PLB flow tests: %d checks, %d failures\n", s_cases, s_failures);
    return s_failures;
}
