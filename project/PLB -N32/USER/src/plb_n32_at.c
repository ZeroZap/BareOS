#include "plb_n32_at.h"

#include "n32l40x_cfg.h"
#include "xy_log.h"
#include "xy_mem.h"

#ifndef PLB_N32_ENABLE_AT_SELFTEST
#define PLB_N32_ENABLE_AT_SELFTEST 0
#endif

XY_MEM_POOL_DECLARE(s_at_mem, 2048u);

static at_obj_t *s_at;

#if PLB_N32_ENABLE_AT_SELFTEST
static const char *const s_selftest_commands[] = {
    "AT",
    "AT+CSQ",
    "AT+CEREG?",
};
static unsigned int s_selftest_step;
static bool s_selftest_active;
static bool s_selftest_waiting;

static void plb_n32_at_selftest_response(at_response_t *response)
{
    const char *command = s_selftest_commands[s_selftest_step];

    s_selftest_waiting = false;
    xy_log_i("PLB-N32 AT selftest command=%s code=%d len=%u",
             command,
             (int)response->code,
             (unsigned int)response->recvcnt);
    if (response->code != AT_RESP_OK) {
        s_selftest_active = false;
        xy_log_w("PLB-N32 AT selftest FAILED command=%s", command);
        return;
    }

    s_selftest_step++;
    if (s_selftest_step >=
        sizeof(s_selftest_commands) / sizeof(s_selftest_commands[0])) {
        s_selftest_active = false;
        xy_log_i("PLB-N32 AT selftest PASSED");
    }
}
#endif

static unsigned int plb_n32_at_write(const void *buf, unsigned int len)
{
    return n32_uart5_write_nonblock(buf, len);
}

static unsigned int plb_n32_at_read(void *buf, unsigned int len)
{
    int read_len = n32_uart5_secboot_read((uint8_t *)buf, (size_t)len, 0u);

    return read_len > 0 ? (unsigned int)read_len : 0u;
}

static const at_adapter_t s_adapter = {
    .lock = NULL,
    .unlock = NULL,
    .write = plb_n32_at_write,
    .read = plb_n32_at_read,
    .error = NULL,
    .debug = NULL,
#if AT_URC_WARCH_EN
    .urc_bufsize = 256u,
#endif
    .recv_bufsize = 256u,
};

bool plb_n32_at_init(void)
{
    if (s_at != NULL) {
        return true;
    }

    XY_MEM_POOL_INIT(s_at_mem);
    s_at = at_obj_create(&s_adapter);
    return s_at != NULL;
}

bool plb_n32_at_selftest_start(void)
{
#if PLB_N32_ENABLE_AT_SELFTEST
    if (s_at == NULL || s_selftest_active) {
        return false;
    }
    s_selftest_step = 0u;
    s_selftest_waiting = false;
    s_selftest_active = true;
    xy_log_i("PLB-N32 AT selftest start");
    return true;
#else
    return false;
#endif
}

void plb_n32_at_process(void)
{
    if (s_at != NULL) {
        at_obj_process(s_at);
#if PLB_N32_ENABLE_AT_SELFTEST
        if (s_selftest_active && !s_selftest_waiting) {
            at_attr_t attr;

            at_attr_deinit(&attr);
            attr.cb = plb_n32_at_selftest_response;
            attr.timeout = 1000u;
            attr.retry = 2u;
            if (at_exec_cmd(s_at, &attr, "%s",
                            s_selftest_commands[s_selftest_step])) {
                s_selftest_waiting = true;
            }
        }
#endif
    }
}

at_obj_t *plb_n32_at_obj(void)
{
    return s_at;
}
