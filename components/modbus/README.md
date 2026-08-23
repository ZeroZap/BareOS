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

`mb_tiny_rtu_tx_t` provides non-blocking RS-485 transmission. It copies the ADU
into instance-owned storage, asserts DE, starts UART/DMA transmission, and waits
for the UART transmission-complete ISR to set a flag. The main loop then calls
`mb_tiny_rtu_tx_process()` to deassert DE. It also restores receive direction on
start failure, timeout, or explicit abort.

`mb_tiny_rtu_master_t` provides a non-blocking master transaction state machine.
It sends through the same RS-485 TX object, starts the response timeout after
TX-complete restores receive direction, consumes a timestamped response from the
RX queue, and validates CRC, unit ID, function code, and exception shape. Raw ADU
transactions and function-specific start/result APIs are both available.

The component does **not** validate `t1.5` mid-frame gaps.

The synchronous master receive callback can wait up to `timeout_ms`. Do not call
these wrappers from BareOS's cooperative main loop when blocking would delay AT
processing or power management. Prefer the non-blocking RTU master API in the
cooperative main loop.

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

void modbus_rx_process(void)
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

## Non-blocking RS-485 TX

```c
static mb_tiny_rtu_tx_t rtu_tx;

static int uart_tx_start(const uint8_t *data, uint16_t len)
{
    return bsp_uart_tx_dma(data, len); /* return accepted byte count */
}

static int rs485_set_direction(bool transmit)
{
    bsp_gpio_write(RS485_DE_PIN, transmit);
    return 0;
}

void modbus_tx_init(void)
{
    mb_tiny_rtu_tx_init(&rtu_tx, uart_tx_start, rs485_set_direction, 100U);
}

void modbus_uart_tx_complete_isr(void)
{
    mb_tiny_rtu_tx_complete_isr(&rtu_tx);
}

void modbus_process(void)
{
    int status = mb_tiny_rtu_tx_process(&rtu_tx, g_sys_tick_ms);
    if (status == MB_TINY_TIMEOUT) {
        /* Reset or recover the UART as required by the BSP. */
    }
}
```

The TX-complete interrupt must mean that the UART shift register has transmitted
the final stop bit. A DMA-complete or TXE interrupt only means that bytes have
moved into the UART and is too early to deassert DE. The ISR hook only records
completion; GPIO and recovery callbacks run in main-loop context.

The timeout bounds how long DE may remain asserted if the UART completion event
is lost. `mb_tiny_rtu_tx_is_idle()` can participate in power-management sleep
eligibility. Do not modify or reuse the TX instance until it reports idle.

A non-blocking slave can combine receive framing, request processing, and RS-485
transmission with one main-loop call:

```c
static uint8_t request[MB_TINY_MAX_ADU_SIZE];
static uint8_t response[MB_TINY_MAX_ADU_SIZE];

void modbus_process(void)
{
    int status = mb_tiny_rtu_slave_poll(
        &slave, &rx_queue, &rtu_rx, &rtu_tx, g_sys_tick_ms,
        request, sizeof(request), response, sizeof(response));

    if (status == MB_TINY_TIMEOUT || status == MB_TINY_IO_ERROR) {
        /* Recover the UART or record a communication fault. */
    }
}
```

The request and response buffers must not overlap. While TX is active, this
helper only advances TX completion or timeout handling and leaves queued RX
bytes untouched. Broadcast writes execute without starting TX; requests for a
different unit are ignored.

## Non-blocking master transactions

```c
static mb_tiny_rtu_master_t rtu_master;

void modbus_master_init(void)
{
    mb_tiny_rtu_master_init(
        &rtu_master, &rx_queue, &rtu_rx, &rtu_tx, 1000U);
}

int modbus_read_holding_start(uint8_t unit, uint16_t address, uint16_t count)
{
    return mb_tiny_rtu_master_read_holding_start(
        &rtu_master, unit, address, count, g_sys_tick_ms);
}

void modbus_master_process(void)
{
    int status = mb_tiny_rtu_master_process(&rtu_master, g_sys_tick_ms);

    if (status == MB_TINY_OK) {
        uint16_t registers[2];

        if (mb_tiny_rtu_master_read_holding_result(
                &rtu_master, registers, 2U) == MB_TINY_OK) {
            /* Consume the decoded registers. */
        }
        mb_tiny_rtu_master_reset(&rtu_master);
    } else if (status == MB_TINY_EXCEPTION) {
        /* Inspect rtu_master.last_exception before reset. */
        mb_tiny_rtu_master_reset(&rtu_master);
    } else if (status < 0 && status != MB_TINY_BUSY) {
        mb_tiny_rtu_master_reset(&rtu_master);
    }
}
```

Only unicast request ADUs are accepted. The request CRC is checked before TX,
and a new transaction requires idle TX, RX framing, and RX queue objects. A
completed transaction remains in `MB_TINY_RTU_MASTER_DONE` so the response and
`last_exception` remain available. Call `mb_tiny_rtu_master_reset()` after
handling the result. Use `mb_tiny_rtu_master_abort()` for an active transaction;
it restores receive direction and flushes partial or queued response bytes.

The response timeout starts only after the final request stop bit has been sent
and DE has been deasserted. This prevents slow request transmission from using
up the slave response window.

Function-specific starters cover all supported function codes. Their matching
`_result()` functions strictly validate response length, byte count, and write
echo fields before copying data. Register capacities are in registers; coil and
discrete-input capacities are in packed bytes. Results remain available until
`mb_tiny_rtu_master_reset()` is called. Raw requests started with
`mb_tiny_rtu_master_start()` must be decoded with
`mb_tiny_rtu_master_get_response()` rather than a function-specific result API.

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

The current eight-function-code scope is complete. Add more Modbus function
codes only when required by a product integration.
