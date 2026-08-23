#ifndef PLB_N32_AT_H
#define PLB_N32_AT_H

#include <stdbool.h>
#include "at_chat.h"

#ifdef __cplusplus
extern "C" {
#endif

bool plb_n32_at_init(void);
bool plb_n32_at_selftest_start(void);
void plb_n32_at_process(void);
at_obj_t *plb_n32_at_obj(void);
bool plb_n32_at_pm_can_sleep(void *arg);

#ifdef __cplusplus
}
#endif

#endif
