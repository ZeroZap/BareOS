#ifndef _XY_BASE64_H_
#define _XY_BASE64_H_

uint32_t xy_base64_encode(const uint8_t *text, uint32_t text_len,
                          uint8_t *encode);

uint32_t xy_base64_decode(const uint8_t *code, uint32_t code_len,
                          uint8_t *plain);


#endif