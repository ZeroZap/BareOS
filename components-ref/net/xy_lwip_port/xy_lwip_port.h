/**
 * @file xy_lwip_port.h
 * @brief lwIP BareOS port — sys_now, critical section, and netif helpers.
 *
 * Covers the four minimum requirements to port lwIP to a bare-metal MCU:
 *   ① sys_now()           — millisecond timestamp from xy_os
 *   ② SYS_ARCH_PROTECT    — critical section via HAL interrupt control
 *   ③ mem_malloc / free   — routed to xy_mem pool
 *   ④ netif registration  — xy_lwip_netif_add() wraps netif_add()
 *
 * ─── Bare-metal main loop integration ───────────────────────────────
 *
 *   void main(void) {
 *       xy_lwip_init();                      // lwip_init() + netif_add()
 *       while (1) {
 *           task_comm_poll();
 *           xy_lwip_poll();                  // ethernetif_input + sys_check_timeouts
 *           ...
 *       }
 *   }
 */

#ifndef XY_LWIP_PORT_H
#define XY_LWIP_PORT_H

#include <stdint.h>
#include "lwip/netif.h"
#include "lwip/ip_addr.h"

/* ────────────────────────────────────────────────────────────────────
 * Port initialization
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Initialize lwIP core and register the default network interface.
 *
 * @param ip       Static IP address.  Pass NULL to use DHCP.
 * @param mask     Subnet mask.        Ignored when ip is NULL.
 * @param gw       Default gateway.   Ignored when ip is NULL.
 * @param use_dhcp If true, start DHCP after netif registration.
 */
void xy_lwip_init(const ip4_addr_t *ip,
                  const ip4_addr_t *mask,
                  const ip4_addr_t *gw,
                  int               use_dhcp);

/**
 * Must be called once per main-loop iteration.
 * Drives:
 *   - ethernetif_input()    — pull received frames into the stack
 *   - sys_check_timeouts()  — drive TCP/ARP/DHCP internal timers
 */
void xy_lwip_poll(void);

/** Return the registered netif (useful for status checks). */
struct netif *xy_lwip_netif(void);

/** Returns non-zero when an IP address has been assigned (static or DHCP). */
int xy_lwip_is_ready(void);

/* ────────────────────────────────────────────────────────────────────
 * Platform hooks — implement in xy_lwip_ethernetif.c
 * ──────────────────────────────────────────────────────────────────── */

/**
 * Called by xy_lwip_port when it is time to initialize the MAC/PHY.
 * Implement this in the project's xy_lwip_ethernetif.c.
 */
void xy_lwip_hw_init(struct netif *netif);

/**
 * Called by xy_lwip_poll() to drive hardware RX.
 * Implement this to read frames from your MAC RX FIFO / DMA and
 * call netif->input() for each complete frame.
 */
void xy_lwip_hw_input(struct netif *netif);

/**
 * Called by lwIP when it needs to transmit a frame.
 * Implement this to copy pbuf chain data into your MAC TX FIFO / DMA.
 */
struct pbuf;
err_t xy_lwip_hw_output(struct netif *netif, struct pbuf *p);

/* ────────────────────────────────────────────────────────────────────
 * ISR helper — call from ETH RX interrupt
 * ──────────────────────────────────────────────────────────────────── */

/** Set a flag consumed by xy_lwip_poll() to drive ethernetif_input(). */
void xy_lwip_isr_notify_rx(void);

#endif /* XY_LWIP_PORT_H */
