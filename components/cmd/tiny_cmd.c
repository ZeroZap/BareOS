/**
 * @file tiny_cmd.c
 * @brief Tiny bare-metal command-line shell implementation.
 */

#include "tiny_cmd.h"
#include "xy_stdio.h"
#include "xy_string.h"
#include <stdarg.h>
#include <stddef.h>

/* ── Module-level state ──────────────────────────────────────────────── */

typedef struct {
    const tiny_cmd_entry_t *tbl;
    uint8_t                  count;
} _tbl_slot_t;

static _tbl_slot_t s_tables[TINY_CMD_TABLE_MAX];
static uint8_t     s_table_count;

/* ── Built-in help ───────────────────────────────────────────────────── */

static int _cmd_help(tiny_cmd_t *sh, int argc, char *argv[])
{
    (void)argc; (void)argv;

    for (uint8_t t = 0u; t < s_table_count; t++) {
        for (uint8_t i = 0u; i < s_tables[t].count; i++) {
            const tiny_cmd_entry_t *e = &s_tables[t].tbl[i];
            tiny_cmd_printf(sh, "  %-12s  %s\r\n", e->name, e->desc);
        }
    }

#if defined(TINY_CMD_USE_SECTION)
    for (const tiny_cmd_entry_t *e = TINY_CMD_BEGIN; e < TINY_CMD_END; e++)
        tiny_cmd_printf(sh, "  %-12s  %s\r\n", e->name, e->desc);
#endif

    return 0;
}

static const tiny_cmd_entry_t _help_entry = { "help", "List commands", _cmd_help };
static const tiny_cmd_entry_t *const _builtin_tbl = &_help_entry;

/* ── Init ────────────────────────────────────────────────────────────── */

void tiny_cmd_init(tiny_cmd_t *sh, const char *prompt, bool echo,
                   void (*write_str)(const char *str))
{
    sh->rx_len     = 0u;
    sh->line_ready = 0u;
    sh->echo       = echo;
    sh->prompt     = prompt;
    sh->write_str  = write_str;
    sh->rx_buf[0]  = '\0';

    /* Register built-ins as table 0 the first time any shell is inited */
    if (s_table_count == 0u) {
        s_tables[0].tbl   = _builtin_tbl;
        s_tables[0].count = 1u;
        s_table_count     = 1u;
    }

    if (sh->write_str && sh->prompt)
        sh->write_str(sh->prompt);
}

/* ── Registration ────────────────────────────────────────────────────── */

void tiny_cmd_register(const tiny_cmd_entry_t *tbl, uint8_t count)
{
    if (!tbl || count == 0u || s_table_count >= TINY_CMD_TABLE_MAX) return;
    s_tables[s_table_count].tbl   = tbl;
    s_tables[s_table_count].count = count;
    s_table_count++;
}

/* ── Feed byte (ISR-safe) ────────────────────────────────────────────── */

void tiny_cmd_feed_byte(tiny_cmd_t *sh, char ch)
{
    if (sh->line_ready) return;   /* previous line not yet consumed */

    if (ch == '\r' || ch == '\n') {
        if (sh->rx_len == 0u) {
            /* Empty line: just re-print the prompt */
            if (sh->echo && sh->write_str) sh->write_str("\r\n");
            if (sh->write_str && sh->prompt) sh->write_str(sh->prompt);
            return;
        }
        sh->rx_buf[sh->rx_len] = '\0';
        if (sh->echo && sh->write_str) sh->write_str("\r\n");
        sh->line_ready = 1u;
        return;
    }

    /* Backspace: DEL (0x7F) or BS (0x08) */
    if (ch == '\b' || ch == (char)0x7Fu) {
        if (sh->rx_len > 0u) {
            sh->rx_len--;
            if (sh->echo && sh->write_str) sh->write_str("\b \b");
        }
        return;
    }

    /* Printable characters only */
    if ((uint8_t)ch < 0x20u) return;

    if (sh->rx_len < TINY_CMD_LINE_MAX) {
        sh->rx_buf[sh->rx_len++] = ch;
        if (sh->echo && sh->write_str) {
            char tmp[2] = { ch, '\0' };
            sh->write_str(tmp);
        }
    }
}

/* ── Tokeniser ───────────────────────────────────────────────────────── */

static int _tokenise(char *line, char *argv[], int max_argc)
{
    int argc = 0;
    char *p  = line;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        if (argc >= max_argc) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

/* ── Lookup and dispatch ─────────────────────────────────────────────── */

static int _dispatch(tiny_cmd_t *sh, int argc, char *argv[])
{
    if (argc == 0) return 0;

    /* Search registered tables */
    for (uint8_t t = 0u; t < s_table_count; t++) {
        for (uint8_t i = 0u; i < s_tables[t].count; i++) {
            const tiny_cmd_entry_t *e = &s_tables[t].tbl[i];
            if (xy_strcasecmp(argv[0], e->name) == 0)
                return e->handler(sh, argc, argv);
        }
    }

#if defined(TINY_CMD_USE_SECTION)
    for (const tiny_cmd_entry_t *e = TINY_CMD_BEGIN; e < TINY_CMD_END; e++) {
        if (xy_strcasecmp(argv[0], e->name) == 0)
            return e->handler(sh, argc, argv);
    }
#endif

    tiny_cmd_printf(sh, "Unknown command: %s\r\n", argv[0]);
    return -1;
}

/* ── Process (main loop) ─────────────────────────────────────────────── */

int tiny_cmd_process(tiny_cmd_t *sh)
{
    if (!sh->line_ready) return 0;

    /* Snapshot: copy rx_buf to exec_buf, then release rx_buf immediately */
    uint8_t len = sh->rx_len;
    for (uint8_t i = 0u; i <= len; i++)
        sh->exec_buf[i] = sh->rx_buf[i];
    sh->rx_len     = 0u;
    sh->line_ready = 0u;   /* release ISR side */

    char *argv[TINY_CMD_ARGS_MAX];
    int   argc = _tokenise(sh->exec_buf, argv, TINY_CMD_ARGS_MAX);
    _dispatch(sh, argc, argv);

    if (sh->write_str && sh->prompt)
        sh->write_str(sh->prompt);

    return 1;
}

/* ── Direct injection ────────────────────────────────────────────────── */

void tiny_cmd_exec(tiny_cmd_t *sh, const char *cmdline)
{
    if (!cmdline) return;

    uint8_t i = 0u;
    while (cmdline[i] && i < TINY_CMD_LINE_MAX) {
        sh->exec_buf[i] = cmdline[i];
        i++;
    }
    sh->exec_buf[i] = '\0';

    char *argv[TINY_CMD_ARGS_MAX];
    int   argc = _tokenise(sh->exec_buf, argv, TINY_CMD_ARGS_MAX);
    _dispatch(sh, argc, argv);
}

/* ── Printf ──────────────────────────────────────────────────────────── */

void tiny_cmd_printf(tiny_cmd_t *sh, const char *fmt, ...)
{
    if (!sh->write_str || !fmt) return;

    static char s_buf[TINY_CMD_PRINTF_BUF];
    va_list ap;
    va_start(ap, fmt);
    xy_vsnprintf(s_buf, sizeof(s_buf), fmt, ap);
    va_end(ap);
    sh->write_str(s_buf);
}
