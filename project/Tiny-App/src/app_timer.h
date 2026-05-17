#ifndef APP_TIMER_H
#define APP_TIMER_H

#include "process.h"
#include "tiny_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

PROCESS_NAME(timer_process);   /* etimer 1s + stimer 10s demo */

void app_timer_register_cmds(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_TIMER_H */
