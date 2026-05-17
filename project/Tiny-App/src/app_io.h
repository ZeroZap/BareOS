#ifndef APP_IO_H
#define APP_IO_H

#include "process.h"
#include "tiny_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

PROCESS_NAME(io_process);   /* LED heartbeat blink via ctimer */

void app_io_register_cmds(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_IO_H */
