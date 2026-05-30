#include "plb_comm.h"

#include "plb_config.h"

#if PLB_COMM_BACKEND == PLB_COMM_BACKEND_CELL
#include "plb_comm_cell_backend.h"
#endif

#include "xy_log.h"
#include "xy_stdio.h"
#include "xy_string.h"

static char s_last_msg[160];
static uint8_t s_send_count;
static uint8_t s_busy;
static uint8_t s_done;
static uint8_t s_ok;
static plb_comm_result_t s_result;
static const plb_comm_backend_t *s_backend;

void plb_comm_init(void)
{
    s_last_msg[0] = '\0';
    s_send_count = 0;
    s_busy = 0;
    s_done = 0;
    s_ok = 0;
    s_result = PLB_COMM_RESULT_OK;
#if PLB_COMM_BACKEND == PLB_COMM_BACKEND_MOCK
    plb_comm_mock_backend_init();
    s_backend = plb_comm_mock_backend();
#elif PLB_COMM_BACKEND == PLB_COMM_BACKEND_CELL
    s_backend = plb_comm_cell_backend();
#endif
}

bool plb_comm_start_distress_send(const xy_gnss_pos_t *pos, uint8_t attempt)
{
    char pos_msg[96];
    uint16_t len;

    if (s_busy || !s_backend || !s_backend->start_send) return false;

    xy_gnss_format_pos(pos, pos_msg, (int)sizeof(pos_msg));
    xy_snprintf(s_last_msg, sizeof(s_last_msg),
                "PLB SOS MMSI=412345678 TRY=%u %s",
                (unsigned int)attempt,
                pos_msg);

    len = (uint16_t)strlen(s_last_msg);
    if (!s_backend->start_send(s_last_msg, len)) return false;

    s_send_count++;
    s_busy = 1;
    s_done = 0;
    s_ok = 0;
    xy_log_i("TX start: %s", s_last_msg);
    return true;
}

void plb_comm_process(void)
{
    if (!s_busy || s_done) return;

    if (s_backend && s_backend->process) s_backend->process();

    if (s_backend && s_backend->done && s_backend->done()) {
        s_result = s_backend->result ? s_backend->result() : PLB_COMM_RESULT_TIMEOUT;
        s_busy = 0;
        s_done = 1;
        s_ok = (s_result == PLB_COMM_RESULT_OK);
        xy_log_i("TX done: %s", s_ok ? "ok" : "fail");
    }
}

bool plb_comm_done(void)
{
    return s_done != 0;
}

bool plb_comm_ok(void)
{
    return s_done && s_ok;
}

plb_comm_result_t plb_comm_result(void)
{
    return s_result;
}

void plb_comm_set_backend(const plb_comm_backend_t *backend)
{
    if (backend) s_backend = backend;
}

void plb_comm_mock_set_result(plb_comm_result_t result, uint8_t delay_ticks)
{
    plb_comm_mock_backend_set_result(result, delay_ticks);
}

void plb_comm_mock_set_result_sequence(const plb_comm_result_t *results, uint8_t count, uint8_t delay_ticks)
{
    plb_comm_mock_backend_set_result_sequence(results, count, delay_ticks);
}

const char *plb_comm_last_message(void)
{
    return s_last_msg;
}

uint8_t plb_comm_send_count(void)
{
    return s_send_count;
}
