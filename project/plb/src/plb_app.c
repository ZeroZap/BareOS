#include "plb_app.h"
#include "plb_comm.h"
#include "plb_pc_bsp.h"

#include "xy_evtlog.h"
#include "xy_gnss.h"
#include "xy_log.h"

extern volatile unsigned int g_sys_tick_ms;

#define PLB_TX_MAX_ATTEMPTS  3u

typedef enum {
    PLB_STATE_IDLE = 0,
    PLB_STATE_ACQUIRE_FIX,
    PLB_STATE_TRANSMIT_START,
    PLB_STATE_TRANSMIT_WAIT,
    PLB_STATE_DONE,
} plb_state_t;

static struct {
    plb_state_t state;
    bool sos_requested;
    xy_gnss_pos_t pos;
    uint8_t tx_attempts;
} s_plb;

static uint32_t now_s(void)
{
    return (uint32_t)(g_sys_tick_ms / 1000u);
}

static bool acquire_fix(xy_gnss_pos_t *pos)
{
    const char *resp = "+QGPSLOC: 085959.0,3114.2506N,12131.4531E,1.1,50.0,2";
    return xy_gnss_parse_at_response(resp, pos);
}

void plb_app_init(void)
{
    s_plb.state = PLB_STATE_IDLE;
    s_plb.sos_requested = false;
    s_plb.tx_attempts = 0;
    plb_comm_init();
    xy_gnss_init();
    xy_evtlog_init(PLB_PC_FLASH_BASE,
                   PLB_PC_FLASH_SIZE,
                   PLB_PC_FLASH_PAGE_SIZE,
                   plb_pc_flash_erase,
                   plb_pc_flash_write,
                   plb_pc_flash_read);
    xy_evtlog_write(XY_EVT_POWER_ON, 0, 0, 0, now_s());
}

void plb_app_request_sos(void)
{
    s_plb.sos_requested = true;
}

bool plb_app_step(void)
{
    switch (s_plb.state) {
    case PLB_STATE_IDLE:
        if (s_plb.sos_requested) {
            s_plb.sos_requested = false;
            xy_log_w("SOS requested");
            xy_evtlog_write(XY_EVT_SOS_TRIGGERED, 0, 0, 0, now_s());
            s_plb.state = PLB_STATE_ACQUIRE_FIX;
        }
        break;

    case PLB_STATE_ACQUIRE_FIX:
        if (acquire_fix(&s_plb.pos)) {
            xy_log_i("GNSS fix acquired lat=%d lon=%d",
                     (int)s_plb.pos.lat_1e7,
                     (int)s_plb.pos.lon_1e7);
            xy_evtlog_write(XY_EVT_GNSS_FIX, 0,
                            s_plb.pos.lat_1e7,
                            s_plb.pos.lon_1e7,
                            now_s());
            s_plb.state = PLB_STATE_TRANSMIT_START;
        } else {
            xy_log_e("GNSS fix failed");
            xy_evtlog_write(XY_EVT_GNSS_LOST, 1, 0, 0, now_s());
            s_plb.state = PLB_STATE_DONE;
        }
        break;

    case PLB_STATE_TRANSMIT_START:
        s_plb.tx_attempts++;
        if (plb_comm_start_distress_send(&s_plb.pos, s_plb.tx_attempts)) {
            s_plb.state = PLB_STATE_TRANSMIT_WAIT;
        } else {
            xy_evtlog_write(XY_EVT_POS_TX_FAIL, s_plb.tx_attempts,
                            s_plb.pos.lat_1e7,
                            s_plb.pos.lon_1e7,
                            now_s());
            s_plb.state = PLB_STATE_DONE;
        }
        break;

    case PLB_STATE_TRANSMIT_WAIT:
        plb_comm_process();
        if (!plb_comm_done()) {
            break;
        }
        if (plb_comm_ok()) {
            xy_evtlog_write(XY_EVT_POS_TX_OK, s_plb.tx_attempts,
                            s_plb.pos.lat_1e7,
                            s_plb.pos.lon_1e7,
                            now_s());
        } else {
            xy_evtlog_write(XY_EVT_POS_TX_FAIL, s_plb.tx_attempts,
                            s_plb.pos.lat_1e7,
                            s_plb.pos.lon_1e7,
                            now_s());
            if (s_plb.tx_attempts < PLB_TX_MAX_ATTEMPTS) {
                s_plb.state = PLB_STATE_TRANSMIT_START;
                break;
            }
        }
        s_plb.state = PLB_STATE_DONE;
        break;

    case PLB_STATE_DONE:
        return false;
    }

    return true;
}

int plb_app_dump_evtlog(void)
{
    xy_evtlog_entry_t entries[8];
    int count = xy_evtlog_read_last(entries, 8);

    xy_log_i("evtlog count=%d", count);
    for (int i = 0; i < count; i++) {
        xy_log_i("evt[%d] t=%u type=0x%x status=%u lat=%d lon=%d",
                 i,
                 (unsigned int)entries[i].timestamp_s,
                 (unsigned int)entries[i].event_type,
                 (unsigned int)entries[i].status,
                 (int)entries[i].lat_1e7,
                 (int)entries[i].lon_1e7);
    }
    return count;
}
