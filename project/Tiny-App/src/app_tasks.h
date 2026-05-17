/**
 * @file app_tasks.h
 * @brief Application protothread task declarations for the PC test build.
 */

#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "process.h"
#include "tiny_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Process declarations ─────────────────────────────────────────────
 * Declare all application processes so main.c can start them.
 */
PROCESS_NAME(heartbeat_process);   /* 1-second alive log */
PROCESS_NAME(cmd_process);         /* tiny_cmd shell over stdin */
PROCESS_NAME(timer_process);       /* etimer/ctimer/rtimer/stimer demo */
PROCESS_NAME(io_process);          /* LED blink via ctimer */

/* Global shell instance — extern so app_tasks.c and main.c can share it. */
extern tiny_cmd_t g_shell;

/* ── Application event IDs (extend PROCESS_EVENT_USER) ─────────────── */
#define APP_EVT_UART_BYTE  (PROCESS_EVENT_USER + 0x00u)

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
