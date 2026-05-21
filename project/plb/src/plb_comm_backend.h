#ifndef PLB_COMM_BACKEND_H
#define PLB_COMM_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PLB_COMM_RESULT_OK = 0,
    PLB_COMM_RESULT_SEND_FAIL,
    PLB_COMM_RESULT_TIMEOUT,
} plb_comm_result_t;

typedef struct {
    bool (*start_send)(const char *payload, uint16_t len);
    void (*process)(void);
    bool (*done)(void);
    plb_comm_result_t (*result)(void);
} plb_comm_backend_t;

const plb_comm_backend_t *plb_comm_mock_backend(void);
void plb_comm_mock_backend_init(void);
void plb_comm_mock_backend_set_result(plb_comm_result_t result, uint8_t delay_ticks);
void plb_comm_mock_backend_set_result_sequence(const plb_comm_result_t *results,
                                               uint8_t count,
                                               uint8_t delay_ticks);

#endif /* PLB_COMM_BACKEND_H */
