# mb_tiny

`mb_tiny` is a small Modbus RTU ADU codec with synchronous master wrappers. It
contains no heap allocation or RTOS dependency.

## Current scope

Supported function codes:

| Code   | Operation                        | Master | Slave |
| ------ | -------------------------------- | -----: | ----: |
| `0x01` | Read coils                       |    Yes |   Yes |
| `0x02` | Read discrete inputs             |    Yes |   Yes |
| `0x03` | Read holding registers           |    Yes |   Yes |
| `0x04` | Read input registers             |    Yes |   Yes |
| `0x05` | Write single coil                |    Yes |   Yes |
| `0x06` | Write single holding register    |    Yes |   Yes |
| `0x0F` | Write multiple coils             |    Yes |   Yes |
| `0x10` | Write multiple holding registers |    Yes |   Yes |

The pure `mb_tiny_slave_process()` API converts one complete request ADU into a
caller-owned response buffer without performing IO. `mb_tiny_slave_handle()` is
a compatible wrapper that sends generated responses through the configured
callback.

`mb_tiny_rtu_rx_t` provides poll-driven RTU frame separation. Feed timestamped
UART bytes from the main loop with `mb_tiny_rtu_rx_feed()`, then call
`mb_tiny_rtu_rx_poll()` to retrieve an ADU after the `t3.5` silence interval.
`mb_tiny_rtu_frame_gap_ms()` calculates a conservative whole-millisecond gap
without floating point.

`mb_tiny_rtu_rx_queue_t` is an optional ISR-to-main-loop SPSC adapter. Its
storage is application-owned, so queue RAM can be selected for the UART baud
rate and worst-case main-loop latency. The single UART RX ISR calls
`mb_tiny_rtu_rx_queue_push_isr()` with the timestamp captured when the byte was
received. The main loop calls `mb_tiny_rtu_rx_queue_process()`; queued timestamps
allow it to separate multiple complete frames even when processing is delayed.

The component does **not** control an RS-485 DE pin, validate `t1.5` mid-frame
gaps, or provide a non-blocking master transaction state machine.

The synchronous master receive callback can wait up to `timeout_ms`. Do not call
these wrappers from BareOS's cooperative main loop when blocking would delay AT
processing or power management. A poll-driven RTU transport remains future
work.

Capacity-aware master read APIs use the `_ex` suffix. Register capacities are
specified in registers; coil and discrete-input capacities are specified in
bytes. They reject undersized output buffers before starting UART IO. The legacy
read APIs remain source-compatible, but callers should prefer `_ex` when the
actual destination capacity is known.

## ISR queue integration

```c
static mb_tiny_rtu_rx_slot_t rx_slots[64];
static mb_tiny_rtu_rx_queue_t rx_queue;
static mb_tiny_rtu_rx_t rtu_rx;

void modbus_init(void)
{
    uint16_t gap = mb_tiny_rtu_frame_gap_ms(9600U, 11U);
    mb_tiny_rtu_rx_init(&rtu_rx, gap);
    mb_tiny_rtu_rx_queue_init(&rx_queue, rx_slots, 64U);
}

void modbus_uart_rx_isr(uint8_t byte)
{
    (void)mb_tiny_rtu_rx_queue_push_isr(&rx_queue, byte, g_sys_tick_ms);
}

void modbus_process(void)
{
    uint8_t request[MB_TINY_MAX_ADU_SIZE];
    uint16_t request_len;
    int status;

    status = mb_tiny_rtu_rx_queue_process(
        &rx_queue, &rtu_rx, g_sys_tick_ms,
        request, sizeof(request), &request_len);
    if (status == MB_TINY_FRAME_READY) {
        (void)mb_tiny_slave_handle(&slave, request, request_len);
    }
}
```

The queue reserves one slot, so an array of 64 slots holds at most 63 bytes.
Only one ISR may produce into an instance. If multiple producers are required,
the BSP must serialize them. Queue overflow increments `dropped_bytes`; the
main-loop processor then discards the affected queued data and resets the
partial frame instead of risking execution of a truncated request.

Call `mb_tiny_rtu_rx_queue_is_idle()` from power-management eligibility logic.
Do not enter a sleep mode that stops the UART or shared timestamp source while
it reports busy.

## Addressing and data layout

Addresses are zero-based PDU addresses. Human-readable addresses such as
`40001` must be converted by the application.

Coils and discrete inputs are bit-packed:

```text
data[0] bit 0 = start_addr
data[0] bit 1 = start_addr + 1
...
data[1] bit 0 = start_addr + 8
```

A request may begin at any bit address; it does not need to be byte aligned.

Valid unicast unit IDs are `1..247`. Slave address `0` is accepted only as a
broadcast request. Broadcast writes are executed without a response; broadcast
reads are ignored.

## ADU limits

The default is deliberately bounded for small bare-metal systems:

```c
#define MB_TINY_MAX_ADU_SIZE 128U
```

Derived limits are:

| Operation                | Maximum quantity |
| ------------------------ | ---------------: |
| Read registers           |     61 registers |
| Read bits                |         984 bits |
| Write multiple registers |     59 registers |
| Write multiple bits      |         952 bits |

The current implementation uses these derived values rather than the larger
Modbus protocol maxima, preventing a valid-looking request from overflowing the
configured ADU buffer.

## Callback contract

Send callbacks must consume or copy all bytes before returning. A return value
other than the requested length is treated as `MB_TINY_IO_ERROR`.

Receive callbacks receive the destination capacity and must return:

- a positive complete-ADU length on success;
- `0` on timeout;
- a negative value on driver failure.

Each master and slave instance stores its own callbacks. Instances no longer
share mutable global callback state. The pure slave processing API does not
require a send callback.

## Error handling

Local errors and Modbus exception codes are separate. A master receiving a
valid exception response returns `MB_TINY_EXCEPTION`; the exception byte is
available in `master.last_exception`.

Slave CRC and malformed-frame errors are returned locally and do not generate a
response. Valid requests with unsupported functions, invalid addresses, or
invalid values generate Modbus exception responses.

The pure slave API additionally returns:

- `MB_TINY_IGNORED` for another unit's request or a broadcast read;
- `MB_TINY_NO_RESPONSE` after successfully executing a broadcast write;
- `MB_TINY_BUFFER_TOO_SMALL` before writing outside the response capacity.

For write requests, a too-small unicast response buffer is detected before the
mapped application data is modified.

## Host tests

From `components/modbus/tests`:

```sh
make clean
make test
```

The tests compile as C99 with `-Wall -Wextra -Werror`. On a host toolchain that
provides sanitizer runtimes, enable them explicitly:

```sh
make clean
make test SANITIZERS="-fsanitize=address,undefined -fno-omit-frame-pointer"
```

The current Windows MinGW installation may not include `libasan`/`libubsan`, so
sanitizers are optional rather than enabled by default.

## Remaining work

- Add RS-485 DE control and TX-complete handling.
- Add a non-blocking master transaction state machine driven by the shared
  BareOS time source.
