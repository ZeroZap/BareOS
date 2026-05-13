# XY TLV Architecture

## System Architecture

```mermaid
graph TB
    User[User Application]
    Encoder[Encoder API]
    Decoder[Decoder API]
    Buffer[Buffer Management]
    Iterator[Iterator]
    Validator[Validator]

    User --> Encoder
    User --> Decoder
    Encoder --> Buffer
    Decoder --> Iterator
    Iterator --> Validator
    Buffer --> Memory[Memory Buffer]

    style User fill:#e1f5ff
    style Memory fill:#ffe1e1
```

## Component Flow

### Encoding Flow
```mermaid
graph LR
    A[Initialize Buffer] --> B[Encode TLV #1]
    B --> C[Encode TLV #2]
    C --> D[Encode TLV #N]
    D --> E[Get Size]
    E --> F[Send/Store Buffer]

    style A fill:#c8e6c9
    style F fill:#ffccbc
```

### Decoding Flow
```mermaid
graph LR
    A[Receive Buffer] --> B[Validate]
    B --> C[Initialize Iterator]
    C --> D{Has Next?}
    D -->|Yes| E[Get Next TLV]
    E --> F[Decode Value]
    F --> D
    D -->|No| G[Complete]

    style A fill:#c8e6c9
    style G fill:#ffccbc
```

## TLV Binary Format

```
┌─────────────────────────────────────────────────┐
│           Single TLV Element                    │
├─────────┬─────────┬─────────┬─────────┬────────┤
│ Type    │ Type    │ Length  │ Length  │ Value  │
│ (MSB)   │ (LSB)   │ (MSB)   │ (LSB)   │        │
│ 1 byte  │ 1 byte  │ 1 byte  │ 1 byte  │ N bytes│
└─────────┴─────────┴─────────┴─────────┴────────┘
   Byte 0    Byte 1    Byte 2    Byte 3    4 ... N

Example: Encode uint32 value 0x12345678 with type 0x1001

┌────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┐
│  0x10  │  0x01  │  0x00  │  0x04  │  0x12  │  0x34  │  0x56  │  0x78  │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘
  Type=0x1001      Length=4         Value=0x12345678

Total size: 8 bytes (4 header + 4 value)
```

## Buffer Structure

```
┌──────────────────────────────────────────────────────────────┐
│                    TLV Buffer                                │
├──────────────────────────────────────────────────────────────┤
│ TLV #1 Header (4) │ TLV #1 Value (N₁)                       │
├───────────────────┴──────────────────────────────────────────┤
│ TLV #2 Header (4) │ TLV #2 Value (N₂)                       │
├───────────────────┴──────────────────────────────────────────┤
│ TLV #3 Header (4) │ TLV #3 Value (N₃)                       │
├───────────────────┴──────────────────────────────────────────┤
│                        ...                                    │
├───────────────────────────────────────────────────────────────┤
│ TLV #N Header (4) │ TLV #N Value (Nₙ)                       │
└───────────────────┴───────────────────────────────────────────┘

Total Size = Σ(4 + Value_Length_i) for i=1 to N
```

## Data Type Hierarchy

```mermaid
graph TD
    Root[TLV Types]
    Basic[Basic Types 0x0001-0x00FF]
    String[String/Binary 0x0100-0x01FF]
    Container[Containers 0x0200-0x02FF]
    Special[Special 0x0300-0x03FF]
    User[User Defined 0x1000-0xFFFF]

    Root --> Basic
    Root --> String
    Root --> Container
    Root --> Special
    Root --> User

    Basic --> INT[Integers]
    Basic --> FLOAT[Float/Double]
    Basic --> BOOL[Boolean]

    String --> STR[String]
    String --> BIN[Bytes/Blob]

    Container --> CONT[Container]
    Container --> ARR[Array]
    Container --> LIST[List]

    Special --> TIME[Timestamp]
    Special --> NET[Network Types]
    Special --> CHECK[Checksum]
```

## API Layer Structure

```
┌──────────────────────────────────────────────────────────┐
│                   User Application                       │
└───────────────────┬──────────────────────────────────────┘
                    │
    ┌───────────────┴───────────────┐
    │                               │
┌───▼────────────┐        ┌────────▼───────┐
│  Encode APIs   │        │  Decode APIs   │
├────────────────┤        ├────────────────┤
│ - encode_uint8 │        │ - decode_uint8 │
│ - encode_uint16│        │ - decode_uint16│
│ - encode_uint32│        │ - decode_uint32│
│ - encode_string│        │ - decode_string│
│ - encode_bytes │        │ - decode_bytes │
│      ...       │        │      ...       │
└────────┬───────┘        └────────┬───────┘
         │                         │
         └──────────┬──────────────┘
                    │
         ┌──────────▼──────────┐
         │   Core Functions    │
         ├─────────────────────┤
         │ - Buffer Init       │
         │ - Iterator Init     │
         │ - Validate          │
         │ - Checksum          │
         └──────────┬──────────┘
                    │
         ┌──────────▼──────────┐
         │  Helper Functions   │
         ├─────────────────────┤
         │ - read_uint16       │
         │ - write_uint16      │
         │ - read_uint32       │
         │ - write_uint32      │
         └──────────┬──────────┘
                    │
         ┌──────────▼──────────┐
         │   Memory Buffer     │
         └─────────────────────┘
```

## State Machine - Iterator

```mermaid
stateDiagram-v2
    [*] --> Initialized: iterator_init()
    Initialized --> HasData: has_next()
    HasData --> Reading: iterator_next()
    Reading --> Decoded: decode_xxx()
    Decoded --> HasData: has_next()
    HasData --> Complete: No more data
    Reading --> Error: Invalid data
    Complete --> [*]
    Error --> [*]
    Initialized --> Reset: iterator_reset()
    Reset --> Initialized
```

## Memory Layout - Encoding Buffer

```
xy_tlv_buffer_t structure:
┌─────────────────────────────────────┐
│ buffer (pointer)                    │ ───┐
├─────────────────────────────────────┤    │
│ capacity (uint16_t)                 │    │
├─────────────────────────────────────┤    │
│ offset (uint16_t)                   │    │
├─────────────────────────────────────┤    │
│ nesting (uint8_t)                   │    │
└─────────────────────────────────────┘    │
                                            │
                                            │
              ┌─────────────────────────────┘
              │
              ▼
┌──────────────────────────────────────────────────────┐
│                 User-provided buffer                  │
├──────────────────────────────────────────────────────┤
│  Used space (offset bytes)  │  Free space            │
│  ◄─────────────────────────►│◄──────────────────────►│
└──────────────────────────────────────────────────────┘
0                           offset                  capacity
```

## Nested Container Example

```
┌────────────────────────────────────────────────────────┐
│ Container TLV                                          │
├──────────┬──────────┬──────────────────────────────────┤
│Type:CONT │Length: N │ Value (nested TLVs)             │
└──────────┴──────────┴──────────────────────────────────┘
                       │
                       └─┬────────────────────────┬───────
                         │                        │
              ┌──────────▼──────┐      ┌─────────▼────────┐
              │ TLV #1          │      │ TLV #2           │
              ├─────┬─────┬─────┤      ├─────┬─────┬──────┤
              │Type │Len  │Value│      │Type │Len  │Value │
              └─────┴─────┴─────┘      └─────┴─────┴──────┘

Nesting level tracking prevents overflow (max: XY_TLV_MAX_NESTING_LEVEL)
```

## Error Handling Flow

```mermaid
graph TD
    A[Function Call] --> B{Valid Parameters?}
    B -->|No| E1[Return INVALID_PARAM]
    B -->|Yes| C{Buffer Check}
    C -->|Overflow| E2[Return BUFFER_OVERFLOW]
    C -->|Underflow| E3[Return BUFFER_UNDERFLOW]
    C -->|OK| D{Operation}
    D -->|Success| OK[Return OK]
    D -->|Fail| E4[Return ERROR]

    style E1 fill:#ffcccc
    style E2 fill:#ffcccc
    style E3 fill:#ffcccc
    style E4 fill:#ffcccc
    style OK fill:#ccffcc
```

## Performance Characteristics

### Time Complexity

| Operation | Best | Average | Worst | Notes |
|-----------|------|---------|-------|-------|
| Encode | O(1) | O(1) | O(1) | Direct write |
| Decode | O(1) | O(1) | O(1) | Zero-copy |
| Find | O(n) | O(n) | O(n) | Linear search |
| Validate | O(n) | O(n) | O(n) | Single pass |
| Checksum | O(n) | O(n) | O(n) | Full scan |

### Space Complexity

| Component | Size | Notes |
|-----------|------|-------|
| TLV Header | 4 bytes | Fixed |
| Buffer Context | 12 bytes | Per encoder |
| Iterator | 16 bytes | Per decoder |
| Statistics | 24 bytes | Global |
| User Buffer | Variable | Provided by user |

## Thread Safety Model

```
Thread 1                    Thread 2
┌────────────┐             ┌────────────┐
│  Buffer A  │             │  Buffer B  │
│  Iterator A│             │  Iterator B│
└─────┬──────┘             └─────┬──────┘
      │                          │
      └──────────┬───────────────┘
                 │
                 ▼
         ┌───────────────┐
         │ Global Stats  │ ◄── Needs mutex if shared
         └───────────────┘

Recommendation: Separate buffers per thread, no shared state.
```

## Integration Points

```mermaid
graph LR
    Flash[Flash Storage] --> TLV
    EEPROM[EEPROM] --> TLV
    Network[Network Layer] --> TLV
    IPC[IPC Messages] --> TLV

    TLV[XY TLV System]

    TLV --> Config[Configuration]
    TLV --> Sensors[Sensor Data]
    TLV --> Commands[Commands]
    TLV --> State[State Storage]
```

## Design Principles

1. **Zero Allocation**
   - User provides all buffers
   - No malloc/free calls
   - Predictable memory usage

2. **Type Safety**
   - Dedicated encode/decode per type
   - Compile-time type checking
   - Automatic endianness handling

3. **Error Resilience**
   - Comprehensive validation
   - Clear error codes
   - Graceful degradation

4. **Performance**
   - Zero-copy parsing
   - Single-pass operations
   - Minimal overhead

5. **Portability**
   - C99 standard only
   - No platform dependencies
   - Network byte order

6. **Extensibility**
   - Custom type support
   - Nested containers
   - User-defined ranges
