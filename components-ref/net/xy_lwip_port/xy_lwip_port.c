/**
 * @file xy_lwip_port.c
 * @brief lwIP BareOS port — sys_now, memory, critical section, netif glue.
 */

#include "xy_lwip_port.h"
#include "lwipopts.h"

#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#include "kernel/osal/inc/xy_os_tick.h"  /* xy_os_get_tick_ms()   */
#include "kernel/osal/inc/xy_os_sys.h"   /* xy_os_enter_critical() */

/* ── Module-private state ────────────────────────────────────────────── */

static struct netif s_netif;
static volatile int s_rx_pending = 0;  /* set by ISR, cleared by poll */

/* ────────────────────────────────────────────────────────────────────
 * sys_now() — required by lwIP (used for TCP retransmit, ARP aging …)
 * ──────────────────────────────────────────────────────────────────── */

u32_t sys_now(void)
{
    return (u32_t)xy_os_get_tick_ms();
}

/* ────────────────────────────────────────────────────────────────────
 * Critical section — required by lwIP (SYS_ARCH_PROTECT/UNPROTECT)
 * With NO_SYS=1 and SYS_LIGHTWEIGHT_PROT=0 these are not called by
 * lwIP internally, but user code may use them.
 * ──────────────────────────────────────────────────────────────────── */

sys_prot_t sys_arch_protect(void)
{
    return (sys_prot_t)xy_os_enter_critical();
}

void sys_arch_unprotect(sys_prot_t val)
{
    xy_os_exit_critical((uint32_t)val);
}

/* ────────────────────────────────────────────────────────────────────
 * netif init callback — called by netif_add()
 * ──────────────────────────────────────────────────────────────────── */

static err_t netif_init_cb(struct netif *netif)
{
    netif->name[0] = 'e';
    netif->name[1] = '0';
    netif->mtu     = 1500;
    netif->flags   = NETIF_FLAG_BROADCAST |
                     NETIF_FLAG_ETHARP    |
                     NETIF_FLAG_LINK_UP;

    netif->output     = etharp_output;     /* lwIP ARP layer */
    netif->linkoutput = xy_lwip_hw_output; /* platform TX    */

    /* Delegate hardware init to the project-provided function. */
    xy_lwip_hw_init(netif);

    return ERR_OK;
}

/* ────────────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────────────── */

void xy_lwip_init(const ip4_addr_t *ip,
                  const ip4_addr_t *mask,
                  const ip4_addr_t *gw,
                  int               use_dhcp)
{
    ip4_addr_t zero;
    IP4_ADDR(&zero, 0, 0, 0, 0);

    /* Initialize lwIP core. */
    lwip_init();

    /* Register network interface. */
    netif_add(&s_netif,
              ip   ? ip   : &zero,
              mask ? mask : &zero,
              gw   ? gw   : &zero,
              NULL,
              netif_init_cb,
              ethernet_input);

    netif_set_default(&s_netif);
    netif_set_up(&s_netif);

    if (use_dhcp) {
        dhcp_start(&s_netif);
    }
}

void xy_lwip_poll(void)
{
    /* Drive hardware RX — only when ISR has signalled a frame is ready. */
    if (s_rx_pending) {
        s_rx_pending = 0;
        xy_lwip_hw_input(&s_netif);
    }

    /* Drive lwIP internal timers (TCP retransmit, ARP aging, DHCP…). */
    sys_check_timeouts();
}

struct netif *xy_lwip_netif(void)
{
    return &s_netif;
}

int xy_lwip_is_ready(void)
{
#if LWIP_DHCP
    return dhcp_supplied_address(&s_netif);
#else
    return netif_is_up(&s_netif);
#endif
}

/* ────────────────────────────────────────────────────────────────────
 * ISR helper
 * ──────────────────────────────────────────────────────────────────── */

void xy_lwip_isr_notify_rx(void)
{
    s_rx_pending = 1;  /* atomic word write on all supported Cortex-M */
}
