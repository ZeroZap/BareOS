#include "pc_uart.h"

#include <stdio.h>
#include <string.h>

/* ── Platform non-blocking stdin ─────────────────────────────────────── */

#ifdef _WIN32
#  include <conio.h>
#  include <windows.h>

/* ── Pipe reader thread (used when stdin is not a real console) ─────────
 * Git Bash / MSYS2 stdin is an MSYS2 private pipe; PeekNamedPipe only
 * returns the first byte reliably.  A dedicated thread blocks on ReadFile
 * and pushes bytes into a small lock-protected ring buffer.
 */
#define _PIPE_BUF 256
static uint8_t           s_pb[_PIPE_BUF];
static volatile int      s_ph, s_pt;  /* head (writer), tail (reader) */
static CRITICAL_SECTION  s_pcs;
static HANDLE            s_stdin_h;
static BOOL              s_is_console;

static DWORD WINAPI _stdin_reader(LPVOID p)
{
    (void)p;
    for (;;) {
        /* fgetc blocks until a byte is available, works with MSYS2 pipes */
        int c = fgetc(stdin);
        if (c == EOF) break;
        EnterCriticalSection(&s_pcs);
        int next = (s_ph + 1) % _PIPE_BUF;
        if (next != s_pt) { s_pb[s_ph] = (uint8_t)c; s_ph = next; }
        LeaveCriticalSection(&s_pcs);
    }
    return 0;
}

static void _stdin_nonblock_init(void)
{
    s_stdin_h = GetStdHandle(STD_INPUT_HANDLE);
    /* FILE_TYPE_CHAR = real keyboard (cmd.exe / PowerShell / mintty interactive).
     * FILE_TYPE_PIPE / FILE_TYPE_DISK = piped or redirected — GetConsoleMode
     * may still return TRUE under mintty ConPTY even for pipes, so we use
     * GetFileType which is unambiguous. */
    s_is_console = (GetFileType(s_stdin_h) == FILE_TYPE_CHAR);
    if (!s_is_console) {
        InitializeCriticalSection(&s_pcs);
        CreateThread(NULL, 0, _stdin_reader, NULL, 0, NULL);
    }
}

static int _stdin_read_byte(uint8_t *ch)
{
    if (s_is_console) {
        if (_kbhit()) { *ch = (uint8_t)_getch(); return 1; }
        return 0;
    }
    int has = 0;
    EnterCriticalSection(&s_pcs);
    if (s_ph != s_pt) {
        *ch = s_pb[s_pt];
        s_pt = (s_pt + 1) % _PIPE_BUF;
        has = 1;
    }
    LeaveCriticalSection(&s_pcs);
    return has;
}

#else  /* POSIX */
#  include <unistd.h>
#  include <fcntl.h>

static void _stdin_nonblock_init(void)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

static int _stdin_read_byte(uint8_t *ch)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        *ch = c;
        return 1;
    }
    return 0;
}
#endif

/* ── State ───────────────────────────────────────────────────────────── */

static pc_uart_rx_fn s_rx_cb;
static void         *s_rx_arg;

/* ── API ─────────────────────────────────────────────────────────────── */

void pc_uart_init(pc_uart_rx_fn rx_cb, void *rx_arg)
{
    s_rx_cb  = rx_cb;
    s_rx_arg = rx_arg;
    _stdin_nonblock_init();
}

void pc_uart_poll(void)
{
    if (!s_rx_cb) return;

    /* Read ONE byte per call — mirrors real UART ISR firing per byte.
     * The caller (main loop) must invoke us every iteration to drain input. */
    uint8_t ch;
    if (_stdin_read_byte(&ch))
        s_rx_cb(ch, s_rx_arg);
}

void pc_uart_write_str(const char *s)
{
    if (!s) return;
    fputs(s, stdout);
    fflush(stdout);
}

void pc_uart_write(const uint8_t *buf, uint16_t len)
{
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
}
