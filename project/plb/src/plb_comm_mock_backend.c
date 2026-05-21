#include "plb_comm_backend.h"

#include "xy_log.h"

typedef enum {
    MOCK_AT_IDLE = 0,
    MOCK_AT_WAIT_PROMPT,
    MOCK_AT_SEND_PAYLOAD,
    MOCK_AT_WAIT_RESULT,
    MOCK_AT_DONE,
} mock_at_state_t;

static mock_at_state_t s_state;
static uint8_t s_delay_ticks;
static uint8_t s_ticks;
static plb_comm_result_t s_default_result;
static plb_comm_result_t s_current_result;
static plb_comm_result_t s_results[8];
static uint8_t s_result_count;
static uint8_t s_result_index;

static plb_comm_result_t next_result(void)
{
    if (s_result_index < s_result_count) {
        return s_results[s_result_index++];
    }
    return s_default_result;
}

void plb_comm_mock_backend_init(void)
{
    s_state = MOCK_AT_IDLE;
    s_delay_ticks = 2;
    s_ticks = 0;
    s_default_result = PLB_COMM_RESULT_OK;
    s_current_result = PLB_COMM_RESULT_OK;
    s_result_count = 0;
    s_result_index = 0;
}

void plb_comm_mock_backend_set_result(plb_comm_result_t result, uint8_t delay_ticks)
{
    s_default_result = result;
    s_delay_ticks = delay_ticks ? delay_ticks : 1u;
    s_result_count = 0;
    s_result_index = 0;
}

void plb_comm_mock_backend_set_result_sequence(const plb_comm_result_t *results,
                                               uint8_t count,
                                               uint8_t delay_ticks)
{
    uint8_t n = count;
    if (n > (uint8_t)(sizeof(s_results) / sizeof(s_results[0]))) {
        n = (uint8_t)(sizeof(s_results) / sizeof(s_results[0]));
    }

    for (uint8_t i = 0; i < n; i++) {
        s_results[i] = results[i];
    }

    s_default_result = PLB_COMM_RESULT_OK;
    s_delay_ticks = delay_ticks ? delay_ticks : 1u;
    s_result_count = n;
    s_result_index = 0;
}

static bool mock_start_send(const char *payload, uint16_t len)
{
    (void)payload;
    if (s_state != MOCK_AT_IDLE && s_state != MOCK_AT_DONE) return false;

    s_current_result = next_result();
    s_ticks = s_delay_ticks;
    s_state = MOCK_AT_WAIT_PROMPT;
    xy_log_i("AT> AT+CIPSEND=%u", (unsigned int)len);
    return true;
}

static void mock_process(void)
{
    if (s_state == MOCK_AT_IDLE || s_state == MOCK_AT_DONE) return;

    switch (s_state) {
    case MOCK_AT_WAIT_PROMPT:
        xy_log_i("AT< >");
        s_state = MOCK_AT_SEND_PAYLOAD;
        break;
    case MOCK_AT_SEND_PAYLOAD:
        xy_log_i("AT> payload");
        s_state = MOCK_AT_WAIT_RESULT;
        break;
    case MOCK_AT_WAIT_RESULT:
        if (s_ticks > 0) s_ticks--;
        if (s_ticks == 0) {
            if (s_current_result == PLB_COMM_RESULT_OK) {
                xy_log_i("AT< SEND OK");
            } else if (s_current_result == PLB_COMM_RESULT_SEND_FAIL) {
                xy_log_i("AT< SEND FAIL");
            } else {
                xy_log_i("AT< timeout");
            }
            s_state = MOCK_AT_DONE;
        }
        break;
    default:
        break;
    }
}

static bool mock_done(void)
{
    return s_state == MOCK_AT_DONE;
}

static plb_comm_result_t mock_result(void)
{
    return s_current_result;
}

static const plb_comm_backend_t s_backend = {
    mock_start_send,
    mock_process,
    mock_done,
    mock_result,
};

const plb_comm_backend_t *plb_comm_mock_backend(void)
{
    return &s_backend;
}
