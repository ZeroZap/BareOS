#ifndef PLB_COMM_H
#define PLB_COMM_H

#include <stdbool.h>
#include "plb_comm_backend.h"
#include "xy_gnss.h"

void plb_comm_init(void);
bool plb_comm_start_distress_send(const xy_gnss_pos_t *pos, uint8_t attempt);
void plb_comm_process(void);
bool plb_comm_done(void);
bool plb_comm_ok(void);
plb_comm_result_t plb_comm_result(void);
void plb_comm_set_backend(const plb_comm_backend_t *backend);
void plb_comm_mock_set_result(plb_comm_result_t result, uint8_t delay_ticks);
void plb_comm_mock_set_result_sequence(const plb_comm_result_t *results, uint8_t count, uint8_t delay_ticks);
const char *plb_comm_last_message(void);
uint8_t plb_comm_send_count(void);

#endif /* PLB_COMM_H */
