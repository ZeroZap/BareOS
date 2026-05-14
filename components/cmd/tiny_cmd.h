/**
 * @file tiny_cmd.h
 * @brief Tiny bare-metal command-line shell.
 *
 * Designed for bare-metal MCUs: no heap, no OS, no printf dependency.
 * Input is fed one byte at a time (ISR or polling); parsing and execution
 * happen in the main loop.
 *
 * ── Registration ─────────────────────────────────────────────────────
 *
 * Two registration methods, both work together:
 *
 * 1. Static table (portable, no linker changes):
 *
 *      static const tiny_cmd_entry_t g_cmds[] = {
 *          { "led",    "led <on|off>",  cmd_led    },
 *          { "reboot", "System reboot", cmd_reboot },
 *      };
 *      tiny_cmd_register(g_cmds, 2);
 *
 * 2. Section-based auto-register (GCC/Keil/IAR — requires linker config):
 *
 *      // In your_cmd.c:
 *      TINY_CMD_EXPORT(led, "led <on|off>", cmd_led)
 *
 *      // In your linker script (.ld):
 *      .tiny_cmd : {
 *          __start_tiny_cmd = .;
 *          KEEP(*(.tiny_cmd))
 *          __stop_tiny_cmd = .;
 *      } > FLASH
 *
 *      Enable at compile time: #define TINY_CMD_USE_SECTION 1
 *
 * ── Usage ─────────────────────────────────────────────────────────────
 *
 *   static tiny_cmd_t g_shell;
 *
 *   // Init (output goes to xy_printf → your UART)
 *   tiny_cmd_init(&g_shell, "cmd> ", true, my_write_str);
 *   tiny_cmd_register(g_cmds, ARRAY_SIZE(g_cmds));
 *
 *   // UART ISR / DMA callback:
 *   void uart_rx_isr(char ch) { tiny_cmd_feed_byte(&g_shell, ch); }
 *
 *   // Main loop:
 *   while (1) { tiny_cmd_process(&g_shell); }
 *
 * ── Command handler ────────────────────────────────────────────────────
 *
 *   static int cmd_led(tiny_cmd_t *sh, int argc, char *argv[]) {
 *       if (argc < 2) { tiny_cmd_printf(sh, "Usage: led <on|off>\r\n"); return -1; }
 *       tiny_cmd_printf(sh, "LED %s\r\n", argv[1]);
 *       return 0;
 *   }
 */

#ifndef TINY_CMD_H
#define TINY_CMD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

#ifndef TINY_CMD_LINE_MAX
#define TINY_CMD_LINE_MAX   128u   /* max input line length (bytes)      */
#endif

#ifndef TINY_CMD_ARGS_MAX
#define TINY_CMD_ARGS_MAX   10u    /* max argc per command               */
#endif

#ifndef TINY_CMD_TABLE_MAX
#define TINY_CMD_TABLE_MAX  4u     /* max number of registered tables    */
#endif

#ifndef TINY_CMD_PRINTF_BUF
#define TINY_CMD_PRINTF_BUF 128u   /* tiny_cmd_printf scratch buffer     */
#endif

/* ── Command entry ──────────────────────────────────────────────────── */

typedef struct tiny_cmd_s tiny_cmd_t;

typedef int (*tiny_cmd_fn)(tiny_cmd_t *sh, int argc, char *argv[]);

typedef struct {
    const char   *name;     /* command keyword (case-insensitive match) */
    const char   *desc;     /* one-line description shown in help       */
    tiny_cmd_fn   handler;
} tiny_cmd_entry_t;

/* ── Shell object ───────────────────────────────────────────────────── */

struct tiny_cmd_s {
    /* Input buffers — rx_buf written by feed_byte (ISR-safe),
     * exec_buf is the line snapshot used during tiny_cmd_process.      */
    char     rx_buf[TINY_CMD_LINE_MAX + 1u];
    uint8_t  rx_len;
    char     exec_buf[TINY_CMD_LINE_MAX + 1u];
    volatile uint8_t line_ready;  /* set by feed_byte, cleared by process */

    /* Config */
    bool        echo;
    const char *prompt;
    void      (*write_str)(const char *str);  /* output sink             */
};

/* ── Section-based auto-register (optional) ─────────────────────────── */

#if defined(TINY_CMD_USE_SECTION)

#  if defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
#    define TINY_CMD_SECTION  __attribute__((used, section(".tiny_cmd")))
     extern const tiny_cmd_entry_t __start_tiny_cmd;
     extern const tiny_cmd_entry_t __stop_tiny_cmd;
#    define TINY_CMD_BEGIN  (&__start_tiny_cmd)
#    define TINY_CMD_END    (&__stop_tiny_cmd)

#  elif defined(__CC_ARM)
#    define TINY_CMD_SECTION  __attribute__((used, section("tiny_cmd")))
     extern const tiny_cmd_entry_t tiny_cmd$$Base;
     extern const tiny_cmd_entry_t tiny_cmd$$Limit;
#    define TINY_CMD_BEGIN  (&tiny_cmd$$Base)
#    define TINY_CMD_END    (&tiny_cmd$$Limit)

#  elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000u)
#    define TINY_CMD_SECTION  __attribute__((used, section(".tiny_cmd")))
     extern const tiny_cmd_entry_t __tiny_cmd_start;
     extern const tiny_cmd_entry_t __tiny_cmd_end;
#    define TINY_CMD_BEGIN  (&__tiny_cmd_start)
#    define TINY_CMD_END    (&__tiny_cmd_end)

#  elif defined(__ICCARM__)
#    define TINY_CMD_SECTION  __root
#    pragma section = ".tiny_cmd"
#    define TINY_CMD_BEGIN  ((const tiny_cmd_entry_t *)__section_begin(".tiny_cmd"))
#    define TINY_CMD_END    ((const tiny_cmd_entry_t *)__section_end(".tiny_cmd"))
#  endif

/**
 * Register a command via linker section (auto-collected at link time).
 * Each translation unit can export any number of commands; no central
 * table needed.
 */
#  define TINY_CMD_EXPORT(_name, _desc, _handler)                        \
     TINY_CMD_SECTION static const tiny_cmd_entry_t                      \
         _tiny_cmd_##_handler = { #_name, (_desc), (_handler) }

#else  /* !TINY_CMD_USE_SECTION — define as no-op so user code compiles  */
#  define TINY_CMD_EXPORT(_name, _desc, _handler)  \
     extern int _handler  /* keeps the symbol visible, zero storage */
#endif

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Initialise a shell object.
 * @param sh         Shell instance.
 * @param prompt     Prompt string (e.g. "cmd> "), or NULL for no prompt.
 * @param echo       true = echo typed characters back.
 * @param write_str  Output sink — called with NUL-terminated strings.
 *                   Pass NULL to silence all output.
 */
void tiny_cmd_init(tiny_cmd_t *sh, const char *prompt, bool echo,
                   void (*write_str)(const char *str));

/**
 * Register a static command table.
 * Up to TINY_CMD_TABLE_MAX tables can be registered across all calls.
 * @param tbl    Pointer to array of tiny_cmd_entry_t.
 * @param count  Number of entries.
 */
void tiny_cmd_register(const tiny_cmd_entry_t *tbl, uint8_t count);

/**
 * Feed one byte into the shell (ISR-safe).
 * Handles backspace (\b / 0x7F), echo, and CR/LF line termination.
 * Sets sh->line_ready when a complete line is available.
 * Does NOT execute — call tiny_cmd_process() from the main loop.
 */
void tiny_cmd_feed_byte(tiny_cmd_t *sh, char ch);

/**
 * Parse and execute any pending line.  Call each main-loop iteration.
 * @return 1 if a command was executed, 0 if idle.
 */
int tiny_cmd_process(tiny_cmd_t *sh);

/**
 * Inject a command string directly (bypasses the input buffer).
 * Useful for scripting or self-test.
 */
void tiny_cmd_exec(tiny_cmd_t *sh, const char *cmdline);

/**
 * printf-style output helper for use inside command handlers.
 * Routes through sh->write_str.
 */
void tiny_cmd_printf(tiny_cmd_t *sh, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* TINY_CMD_H */
