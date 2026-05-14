#include "xy_common.h"

uint64_t xy_u64_div10(uint64_t u64val)
{
#ifndef XY_USE_SOFT_DIV
    return u64val / 10;
#else
    uint64_t q64, r64;
    uint32_t q32, r32, u32val;

    if (u64val >> 32) {
        q64 = (u64val >> 1) + (u64val >> 2);
        q64 += (q64 >> 4);
        q64 += (q64 >> 8);
        q64 += (q64 >> 16);
        q64 += (q64 >> 32);
        q64 >>= 3;
        r64 = u64val - q64 * 10;
        return q64 + ((r64 + 6) >> 4);
    } else {
        u32val = (uint32_t)(u64val & 0xffffffff);
        q32    = (u32val >> 1) + (u32val >> 2);
        q32 += (q32 >> 4);
        q32 += (q32 >> 8);
        q32 += (q32 >> 16);
        q32 >>= 3;
        r32 = u32val - q32 * 10;
        return (uint64_t)(q32 + ((r32 + 6) >> 4));
    }
#endif
}

uint32_t xy_u32_div10(uint32_t u32val)
{
#ifndef XY_USE_SOFT_DIV
    return u32val / 10;
#else
    uint32_t q32, r32;
    q32 = (u32val >> 1) + (u32val >> 2);
    q32 += (q32 >> 4);
    q32 += (q32 >> 8);
    q32 += (q32 >> 16);
    q32 >>= 3;
    r32 = u32val - q32 * 10;
    return q32 + ((r32 + 6) >> 4);
#endif
}

uint8_t xy_u8_mod10(uint8_t val)
{
#ifndef XY_USE_SOFT_DIV
    return val % 10;
#else
    uint8_t quotient = (uint8_t)(((uint16_t)val * 205) >> 11);
    return val - 10 * quotient;
#endif
}

uint16_t xy_u16_mod10(uint16_t val)
{
#ifndef XY_USE_SOFT_DIV
    return val % 10;
#else
    // 使用更精确的乘数 52429 (0xCCCD) 和 19位位移
    uint32_t product  = (uint32_t)val * 0xCCCD;
    uint16_t quotient = (product >> 19);
    return val - 10 * quotient;
#endif
}

uint32_t xy_u32_mod10(uint32_t val)
{
#ifndef XY_USE_SOFT_DIV
    return val % 10;
#else
    uint32_t quotient = (uint32_t)(((uint64_t)val * 0xCCCCCCCD) >> 35);
    return val - 10 * quotient;
#endif
}

uint32_t xy_hex2bcd(uint32_t hex)
{
    uint32_t bcd  = 0;
    uint8_t shift = 0;

    while (hex > 0) {
        uint8_t digit = hex & 0xF;
        if (digit > 9) {
            return 0;
        }
        bcd |= (digit << shift);
        shift += 4;
        hex >>= 4;
    }

    return bcd;
}

uint32_t xy_bcd2hex(uint32_t bcd)
{
    uint32_t result = 0, multiplier = 1;

    while (bcd > 0) {
        uint8_t digit = bcd & 0x0F;
        if (digit > 9)
            return 0;
        result += digit * multiplier;
        multiplier *= 10;
        bcd >>= 4;
    }
    return result;
}

uint32_t xy_dec2bcd(uint32_t dec)
{
    uint32_t bcd  = 0;
    uint8_t shift = 0;
#ifdef XY_USE_SOFT_DIV
    while (dec > 0) {
        uint8_t digit = xy_u32_mod10(dec);
        bcd |= (digit << shift);
        shift += 4;
        dec = xy_u32_div10(dec);
    }

#else
    while (dec > 0) {
        uint8_t digit = dec % 10;
        bcd |= (digit << shift);
        shift += 4;
        dec /= 10;
    }
#endif

    return bcd;
}
uint32_t xy_bcd2dec(uint32_t bcd)
{
    uint32_t dec        = 0;
    uint32_t multiplier = 1;

    while (bcd > 0) {
        uint8_t digit = bcd & 0xF;
        if (digit > 9) {
            return 0;
        }
        dec += digit * multiplier;
        multiplier *= 10;
        bcd >>= 4;
    }

    return dec;
}
