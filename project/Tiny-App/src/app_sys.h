#ifndef APP_SYS_H
#define APP_SYS_H

#include "tiny_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Log reset cause + version strings at startup. */
void app_sys_init(void);

/** Register sysinfo / reset shell commands. */
void app_sys_register_cmds(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SYS_H */
