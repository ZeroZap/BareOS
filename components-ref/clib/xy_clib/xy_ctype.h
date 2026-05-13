#ifndef _XY_CTYPE_H_
#define _XY_CTYPE_H_

#include <stdint.h>

uint8_t xy_islower(int8_t c);
uint8_t xy_isupper(int8_t c);
uint8_t xy_isdigit(int8_t c);
uint8_t xy_isalnum(int8_t c);
uint8_t xy_isalpha(int8_t c);
uint8_t xy_isblank(int8_t c);
uint8_t xy_iscntrl(int8_t c);
uint8_t xy_isgraph(int8_t c);
uint8_t xy_isprint(int8_t c);
uint8_t xy_ispunct(int8_t c);
uint8_t xy_isspace(int8_t c);
uint8_t xy_isxdigit(int8_t c);
uint8_t xy_tolower(int8_t c);
uint8_t xy_toupper(int8_t c);
uint8_t xy_isascii(int8_t c);

#endif
