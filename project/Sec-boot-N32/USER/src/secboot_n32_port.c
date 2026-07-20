#include "secboot_n32_port.h"

#include "n32l40x_cfg.h"
#include "n32l40x_flash.h"
#include "secboot_n32_layout.h"

#include <string.h>

static uint32_t le32_read(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int flash_range_ok(uint32_t address, size_t len)
{
    uint32_t end = address + (uint32_t)len;

    if (end < address) {
        return 0;
    }
    if (address < SECBOOT_N32_FLASH_BASE_ADDR) {
        return 0;
    }
    if (end > (SECBOOT_N32_FLASH_BASE_ADDR + SECBOOT_N32_FLASH_TOTAL_SIZE)) {
        return 0;
    }
    return 1;
}

int secboot_n32_port_flash_read(uint32_t address, uint8_t *data, size_t len)
{
    if ((data == NULL && len != 0u) || !flash_range_ok(address, len)) {
        return -1;
    }
    memcpy(data, (const void *)address, len);
    return 0;
}

int secboot_n32_port_flash_erase(uint32_t address, size_t len)
{
    uint32_t end;

    if ((address & (SECBOOT_N32_FLASH_PAGE_SIZE - 1u)) != 0u ||
        !flash_range_ok(address, len)) {
        return -1;
    }

    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return -1;
    }

    end = address + (uint32_t)len;
    FLASH_Unlock();
    while (address < end) {
        if (FLASH_EraseOnePage(address) != FLASH_COMPL) {
            FLASH_Lock();
            return -1;
        }
        secboot_n32_port_watchdog_kick();
        address += SECBOOT_N32_FLASH_PAGE_SIZE;
    }
    FLASH_Lock();
    return 0;
}

int secboot_n32_port_flash_write(uint32_t address, const uint8_t *data, size_t len)
{
    size_t i;

    if ((data == NULL && len != 0u) || !flash_range_ok(address, len) ||
        ((address | len) & 0x3u) != 0u) {
        return -1;
    }

    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return -1;
    }

    FLASH_Unlock();
    for (i = 0u; i < len; i += 4u) {
        uint32_t word;
        memcpy(&word, data + i, sizeof(word));
        if (FLASH_ProgramWord(address + (uint32_t)i, word) != FLASH_COMPL) {
            FLASH_Lock();
            return -1;
        }
        secboot_n32_port_watchdog_kick();
    }
    FLASH_Lock();
    return 0;
}

void secboot_n32_port_uart_init(void)
{
    n32_uart5_secboot_init();
}

void secboot_n32_port_uart_poll(void)
{
    n32_uart5_secboot_poll();
}

int secboot_n32_port_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
    return n32_uart5_secboot_read(data, len, timeout_ms);
}

int secboot_n32_port_uart_write(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    return n32_uart5_secboot_write(data, len, timeout_ms);
}

int secboot_n32_port_uart_wait_tx_done(uint32_t timeout_ms)
{
    return n32_uart5_secboot_wait_tx_done(timeout_ms);
}

uint32_t secboot_n32_port_uart_pending(void)
{
    return g_n32_uart5_rb_pending;
}

void secboot_n32_port_watchdog_kick(void)
{
    IWDG_ReloadKey();
}

void secboot_n32_port_soft_reset(void)
{
    NVIC_SystemReset();
}

int secboot_n32_port_app_vector_check(uint32_t app_addr, uint32_t image_size)
{
    uint32_t sp = le32_read((const uint8_t *)app_addr);
    uint32_t reset = le32_read((const uint8_t *)(app_addr + 4u));

    if (sp < 0x20000000u || sp > 0x20004000u || (sp & 0x7u) != 0u) {
        return -1;
    }
    if (reset < app_addr || reset >= (app_addr + image_size) || (reset & 0x1u) == 0u) {
        return -1;
    }
    return 0;
}

void secboot_n32_port_jump_app(uint32_t app_addr)
{
    uint32_t sp = le32_read((const uint8_t *)app_addr);
    uint32_t reset = le32_read((const uint8_t *)(app_addr + 4u));
    void (*entry)(void) = (void (*)(void))reset;
    uint32_t i;

    __disable_irq();
    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;

    for (i = 0u; i < 8u; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    SCB->VTOR = app_addr;
    __set_MSP(sp);
    __DSB();
    __ISB();
    entry();
}
