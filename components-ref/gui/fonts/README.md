# Fonts Component

This directory contains bitmap fonts for the XinYi GUI system.

## Font Types

### ASCII Fonts

| Font | Size | Characters | File |
|------|------|------------|------|
| 8x16 | 8x16 pixels | ASCII 0x20-0x7E (94 chars) | `xy_font_8x16.c/.h` |
| 16x24 | 16x24 pixels | ASCII 0x20-0x7E (94 chars) | `xy_font_16x24.c/.h` |

### Chinese Fonts

| Font | Size | Characters | File |
|------|------|------------|------|
| 16x16 Chinese | 16x16 pixels | 150 common characters | `xy_font_chinese_16x16.c/.h` |

## Font Format

### Bitmap Format

Each font uses a simple bitmap format:
- **8x16 font**: 16 bytes per character (16 rows × 8 bits)
- **16x24 font**: 48 bytes per character (24 rows × 2 bytes)
- **16x16 Chinese**: 32 bytes per character (16 rows × 2 bytes)

### Data Layout

```path:/dev/null/format.md#L1-10
Row data format: MSB first, each row is stored as consecutive bytes.

For 8x16 font:
  Byte 0: Row 0, bits 7-0
  Byte 1: Row 1, bits 7-0
  ...
  Byte 15: Row 15, bits 7-0

For 16x16 Chinese font (2 bytes per row):
  Byte 0: Row 0, bits 15-8 (high byte)
  Byte 1: Row 0, bits 7-0 (low byte)
  Byte 2: Row 1, bits 15-8
  Byte 3: Row 1, bits 7-0
  ...
```

## API Usage

### Getting a Font Handle

```path:/dev/null/example.c#L1-10
#include "xy_font_8x16.h"

// Get ASCII 8x16 font handle
const xy_font_8x16_t* font = xy_font_8x16_get();

// Get Chinese 16x16 font handle
const xy_font_chinese_t* cn_font = xy_font_chinese_16x16_get();
```

### Getting Character Bitmap

```path:/dev/null/example.c#L1-10
// Get ASCII character bitmap
const uint8_t* bitmap = xy_font_8x16_get_char('A');

// Get Chinese character bitmap (by Unicode)
const uint8_t* cn_bitmap = xy_font_chinese_16x16_get_char(0x4E0A); // 上
```

### Measuring String Width

```path:/dev/null/example.c#L1-10
// Measure ASCII string
uint16_t width = xy_font_8x16_measure("Hello");

// Measure mixed string (ASCII + Chinese)
uint16_t cn_width = xy_font_chinese_16x16_measure("Hello中文");
```

## Character Ranges

### ASCII (0x20-0x7E)
- Space, digits (0-9), uppercase letters (A-Z), lowercase letters (a-z)
- Punctuation: `!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~`

### Chinese Characters Included
- Common UI characters: 上、下、左、右、确认、取消、设置、返回、菜单
- Numbers: 一、二、三、四、五、六、七、八、九、十
- Additional common characters for general UI use

## Adding New Fonts

1. Create new `.c/.h` pair following the existing pattern
2. Define font data as arrays of bytes
3. Implement the getter and measurement functions
4. Update this README with the new font information

## Dependencies

- None (standalone font module)

## File Structure

```path:/dev/null/structure.txt#L1-10
fonts/
├── CMakeLists.txt
├── README.md
├── xy_font_8x16.c          # 8x16 ASCII font implementation
├── xy_font_8x16.h          # 8x16 ASCII font header
├── xy_font_16x24.c         # 16x24 ASCII font implementation
├── xy_font_16x24.h         # 16x24 ASCII font header
├── xy_font_chinese_16x16.c # 16x16 Chinese font implementation
├── xy_font_chinese_16x16.h # 16x16 Chinese font header
└── font_data_8x16.inc      # 8x16 font data include file
```
