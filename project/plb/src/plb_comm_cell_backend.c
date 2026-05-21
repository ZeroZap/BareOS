#include "plb_comm_cell_backend.h"

static int s_sock_id;
static plb_comm_result_t s_result;

void plb_comm_cell_backend_init(at_obj_t *at, xy_cell_mdm_t mdm, int sock_id)
{
    s_sock_id = sock_id;
    s_result = PLB_COMM_RESULT_SEND_FAIL;
    xy_cell_init(at, mdm);
}

static bool cell_start_send(const char *payload, uint16_t len)
{
    s_result = PLB_COMM_RESULT_SEND_FAIL;
    return xy_cell_start_sock_send(s_sock_id, payload, len);
}

static void cell_process(void)
{
    /* at_obj_process() is owned by the application main loop. */
}

static bool cell_done(void)
{
    return xy_cell_op_done();
}

static plb_comm_result_t cell_result(void)
{
    if (!xy_cell_op_done()) return PLB_COMM_RESULT_TIMEOUT;
    if (xy_cell_op_ok()) {
        s_result = PLB_COMM_RESULT_OK;
    } else {
        s_result = PLB_COMM_RESULT_SEND_FAIL;
    }
    return s_result;
}

static const plb_comm_backend_t s_backend = {
    cell_start_send,
    cell_process,
    cell_done,
    cell_result,
};

const plb_comm_backend_t *plb_comm_cell_backend(void)
{
    return &s_backend;
}
