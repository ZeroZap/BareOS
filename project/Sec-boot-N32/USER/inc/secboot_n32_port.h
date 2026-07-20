#ifndef SECBOOT_N32_PORT_H
#define SECBOOT_N32_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int secboot_n32_port_flash_read(uint32_t address, uint8_t *data, size_t len);
int secboot_n32_port_flash_erase(uint32_t address, size_t len);
int secboot_n32_port_flash_write(uint32_t address, const uint8_t *data, size_t len);

void secboot_n32_port_uart_init(void);
void secboot_n32_port_uart_poll(void);
int secboot_n32_port_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms);
int secboot_n32_port_uart_write(const uint8_t *data, size_t len, uint32_t timeout_ms);
int secboot_n32_port_uart_wait_tx_done(uint32_t timeout_ms);
uint32_t secboot_n32_port_uart_pending(void);

void secboot_n32_port_watchdog_kick(void);
void secboot_n32_port_soft_reset(void);
int secboot_n32_port_app_vector_check(uint32_t app_addr, uint32_t image_size);
void secboot_n32_port_jump_app(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif /* SECBOOT_N32_PORT_H */
