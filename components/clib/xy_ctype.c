#include "xy_ctype.h"
#include <stdint.h>
/**
 * @brief 检查给定字符是否按照当前 C 本地环境分类为小写字符。默认 "C"
 * 本地环境中， islower 仅对小写字母（ abcdefghijklmnopqrstuvwxyz ）返回非零值。
 */
uint8_t xy_islower(int8_t c)
{
    return (('a' <= (c)) && ('z' >= (c)));
}

/**
 * @brief 根据当前C本地环境检查给定字符是否大写字母。在默认 "C" 本地环境中
 * supper 仅对大写拉丁字母（ ABCDEFGHIJKLMNOPQRSTUVWXYZ ）返回非零值。
 */
uint8_t xy_isupper(int8_t c)
{
    return (('A' <= (c)) && ('Z' >= (c)));
}

/**
 * @brief 检查给定字符是否字母字符，即是大写字母（ ABCDEFGHIJKLMNOPQRSTUVWXYZ
 * ）或小写字母（ abcdefghijklmnopqrstuvwxyz ）。
 *
 */
uint8_t xy_isalpha(int8_t c)
{
    return (uint8_t)((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')));
}

uint8_t xy_isdigit(int8_t c)
{
    return (('0' <= (c)) && ('9' >= (c)));
}

/**
 * @brief 检查给定的字符是否为当前 C 本地环境所分类的字母数字字符。
 * 在默认本地环境中，下列字符为字母数字：
 * 数字（ 0123456789 ）
 * 大写字母（ ABCDEFGHIJKLMNOPQRSTUVWXYZ ）
 * 小写字母（ abcdefghijklmnopqrstuvwxyz ）
 */
uint8_t xy_isalnum(int8_t c)
{
    return (uint8_t)((('a' <= c) && (c <= 'z'))
                  || (('A' <= c) && (c <= 'Z'))
                  || (('0' <= c) && (c <= '9')));
}

/**
 * @brief 检查给定的字符是否拥有图形表示，
 * 即它是数字（ 0123456789 ）、大写字母（ ABCDEFGHIJKLMNOPQRSTUVWXYZ
 * ）、小写字母（ abcdefghijklmnopqrstuvwxyz ） 或标点字符（
 * !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ ），或任何指定于当前 C
 * 本地环境的图形字符之一。 从ASCII表看就是出
 */
uint8_t xy_isgraph(int8_t c)
{
    /* int8_t range is -128..127; treat input as unsigned to handle 127 case
     * (warning -Wtype-limits about "127 >= c" always true). */
    uint8_t u = (uint8_t)c;
    return (uint8_t)((u >= 33u) && (u <= 126u));
}

/**
 * @brief 检查给定的字符在当前的 C 本地环境中是否空格符。
 * 在默认 C 本地环境中，只有空格（ 0x20 ）与水平制表符（ 0x09 ）被分类为空格符。

 */
uint8_t xy_isblank(int8_t c)
{

    return ((0x20 == (c)) || (0x09 == (c)));
}

/**
 * @brief 检查给定的字符是否空白符，即空格（ 0x20 ）、换行（ 0x0a ）、回车（
 * 0x0d ）、 水平制表符（ 0x09 ）或垂直制表符（ 0x0b ）之一。
 * @param c
 * @return uint8_t
 */
uint8_t xy_isspace(int8_t c)
{
    return ((0x20 == (c)) || (0x0a == (c)) || (0x0d == (c)) || (0x09 == (c))
            || (0x0b == (c)));
}


/**
 * @brief 检查给定字符是否为控制字符，即编码 0x00-0x1F 及 0x7F
 *
 * @param c
 * @return uint8_t
 */
uint8_t xy_iscntrl(int8_t c)
{
    return (((0x0 <= (c)) && (0x1F >= (c))) || (0x7F == (c)));
}

/**
 * 从 iscntrl 和 isgraph 可以看出，除了 0 NULL 留给编辑器处理，都不会作处理了
 *
 */
uint8_t xy_isnull(int8_t c)
{
    return (c);
}

uint8_t xy_isxdigit(int8_t c)
{
    return (uint8_t)((('0' <= c) && (c <= '9'))
                  || (('A' <= c) && (c <= 'F'))
                  || (('a' <= c) && (c <= 'f')));
}

/**
 * @brief 将 hex 字符转为 0..15 的 nibble 值；非 hex 字符返回 0xFF。
 */
uint8_t xy_xdigit_val(int8_t c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFFu;
}

uint8_t xy_tolower(int8_t c)
{
    return (uint8_t)(xy_isupper(c) ? (c + ('a' - 'A')) : c);
}

uint8_t xy_toupper(int8_t c)
{
    return (uint8_t)(xy_islower(c) ? (c - ('a' - 'A')) : c);
}
