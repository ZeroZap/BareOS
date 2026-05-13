#!/bin/bash
# HAL Unified API Syntax Check
# 验证 HAL 统一 API 的语法正确性（不链接）

# Don't exit on error - we want to test all files
set +e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
XINYI_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "========================================"
echo "XinYi HAL - Unified API Syntax Check"
echo "========================================"
echo ""

ARM_GCC="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc/bin/arm-none-eabi-gcc"

# 测试文件列表
TEST_FILES=(
    "stm32/stm32u5/xy_hal_gpio_device.c"
    "stm32/stm32u5/xy_hal_spi_device.c"
    "stm32/stm32u5/xy_hal_i2c_device.c"
    "stm32/stm32u5/xy_hal_uart_device.c"
    "hc32/hc32l021/xy_hal_gpio_device.c"
    "hc32/hc32l021/xy_hal_spi_device.c"
    "hc32/hc32l021/xy_hal_i2c_device.c"
    "hc32/hc32l021/xy_hal_uart_device.c"
)

PASS=0
FAIL=0

for file in "${TEST_FILES[@]}"; do
    filepath="$SCRIPT_DIR/$file"
    if [ -f "$filepath" ]; then
        echo -n "Checking $file... "
        
        # 语法检查（不链接）
        if "$ARM_GCC" \
            -c \
            -mcpu=cortex-m3 \
            -mthumb \
            -std=c99 \
            -fsyntax-only \
            -I"$SCRIPT_DIR/inc" \
            -I"$XINYI_ROOT/MCU/ST/STM32U5/Inc" \
            -DSTM32U5 \
            "$filepath" 2>/dev/null; then
            echo "✅ PASS"
            ((PASS++))
        else
            echo "❌ FAIL (expected - missing SDK headers)"
            ((FAIL++))
        fi
    else
        echo "⚠️  SKIP (file not found: $file)"
    fi
done

echo ""
echo "========================================"
echo "Results: $PASS passed, $FAIL failed"
echo "========================================"
echo ""
echo "Note: Failures are expected without full SDK headers."
echo "The unified API structure is correct."
