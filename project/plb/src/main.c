#include "plb_app.h"
#include "plb_pc_bsp.h"

#include "xy_log.h"

int main(void)
{
    plb_pc_bsp_init();
    xy_log_init();

    xy_log_i("=== BareOS PLB minimal simulation ===");
    plb_app_init();
    plb_app_request_sos();

    for (int i = 0; i < 16 && plb_app_step(); i++) {
        plb_pc_tick_advance(1000u);
    }

    plb_app_dump_evtlog();
    return 0;
}
