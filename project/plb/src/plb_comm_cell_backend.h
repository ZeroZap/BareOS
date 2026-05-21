#ifndef PLB_COMM_CELL_BACKEND_H
#define PLB_COMM_CELL_BACKEND_H

#include "at_chat.h"
#include "plb_comm_backend.h"
#include "xy_cell.h"

void plb_comm_cell_backend_init(at_obj_t *at, xy_cell_mdm_t mdm, int sock_id);
const plb_comm_backend_t *plb_comm_cell_backend(void);

#endif /* PLB_COMM_CELL_BACKEND_H */
