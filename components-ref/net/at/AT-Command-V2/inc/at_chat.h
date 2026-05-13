/**
 * @file at_chat.h
 * @brief AT Command Chat Interface
 */

#ifndef AT_CHAT_H
#define AT_CHAT_H

#include <stdint.h>

typedef struct {
    void *uart_handle;
    uint32_t timeout;
} at_chat_t;

int at_chat_init(at_chat_t *chat, void *uart);
int at_chat_send(at_chat_t *chat, const char *cmd);
int at_chat_recv(at_chat_t *chat, char *resp, uint32_t max_len);

#endif /* AT_CHAT_H */
