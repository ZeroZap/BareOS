/**
 * @file lwipopts.h
 * @brief lwIP compile-time configuration for BareOS (NO_SYS bare-metal default).
 *
 * Copy to your project and override values as needed.
 * Switch to RTOS mode by setting NO_SYS 0 and implementing sys_arch.c.
 */

#ifndef XY_LWIPOPTS_H
#define XY_LWIPOPTS_H

/* ── System mode ────────────────────────────────────────────────────── */

#define NO_SYS                  1   /* 1 = bare-metal polling; 0 = RTOS   */
#define SYS_LIGHTWEIGHT_PROT    0   /* no RTOS critical-section overhead   */

/* ── Memory ─────────────────────────────────────────────────────────── */

#define MEM_LIBC_MALLOC         0           /* use internal static pool    */
#define MEM_SIZE                (8 * 1024)  /* general heap pool           */
#define MEM_ALIGNMENT           4

#define MEMP_NUM_PBUF           16
#define MEMP_NUM_RAW_PCB        4
#define MEMP_NUM_UDP_PCB        4
#define MEMP_NUM_TCP_PCB        4
#define MEMP_NUM_TCP_PCB_LISTEN 4
#define MEMP_NUM_TCP_SEG        16
#define MEMP_NUM_REASSDATA      2
#define MEMP_NUM_NETBUF         0
#define MEMP_NUM_NETCONN        0           /* Netconn API not used in NO_SYS */
#define MEMP_NUM_ARP_QUEUE      5

/* ── pbuf ────────────────────────────────────────────────────────────── */

#define PBUF_POOL_SIZE          8
#define PBUF_POOL_BUFSIZE       512

/* ── Protocols ──────────────────────────────────────────────────────── */

#define LWIP_ARP                1
#define LWIP_IPV4               1
#define LWIP_IPV6               0   /* disable to save code size           */
#define LWIP_ICMP               1
#define LWIP_RAW                0
#define LWIP_UDP                1
#define LWIP_TCP                1
#define LWIP_DHCP               1
#define LWIP_DNS                0
#define LWIP_IGMP               0

/* ── TCP tuning ─────────────────────────────────────────────────────── */

#define TCP_MSS                 1460
#define TCP_WND                 (4 * TCP_MSS)
#define TCP_SND_BUF             (4 * TCP_MSS)
#define TCP_SND_QUEUELEN        (2 * TCP_SND_BUF / TCP_MSS)
#define LWIP_TCP_KEEPALIVE      1

/* ── ARP ────────────────────────────────────────────────────────────── */

#define ARP_TABLE_SIZE          10
#define ARP_MAXAGE              300

/* ── IP reassembly ──────────────────────────────────────────────────── */

#define IP_REASSEMBLY           0   /* disable if all packets < MTU        */
#define IP_FRAG                 0

/* ── Stats (disable in production) ─────────────────────────────────── */

#define LWIP_STATS              0
#define LWIP_STATS_DISPLAY      0

/* ── Debug (enable during development) ─────────────────────────────── */

#define LWIP_DEBUG              0
#if LWIP_DEBUG
#define ETHARP_DEBUG            LWIP_DBG_ON
#define NETIF_DEBUG             LWIP_DBG_ON
#define TCP_DEBUG               LWIP_DBG_ON
#define IP_DEBUG                LWIP_DBG_ON
#define DHCP_DEBUG              LWIP_DBG_ON
#define LWIP_DBG_MIN_LEVEL      LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON       LWIP_DBG_ON
#endif

/* ── Checksum offload (enable if MCU has HW checksum) ───────────────── */

#define CHECKSUM_BY_HARDWARE    0
#if CHECKSUM_BY_HARDWARE
#define CHECKSUM_GEN_IP         0
#define CHECKSUM_GEN_UDP        0
#define CHECKSUM_GEN_TCP        0
#define CHECKSUM_CHECK_IP       0
#define CHECKSUM_CHECK_UDP      0
#define CHECKSUM_CHECK_TCP      0
#endif

/* ── Logging bridge ─────────────────────────────────────────────────── */

#if LWIP_DEBUG
#include "trace/xy_log/inc/xy_log.h"
#define LWIP_PLATFORM_DIAG(msg) do { XY_LOG_D msg; } while (0)
#define LWIP_PLATFORM_ASSERT(x) \
    do { XY_LOG_E("ASSERT %s:%d", __FILE__, __LINE__); while(1){} } while(0)
#else
#define LWIP_PLATFORM_DIAG(msg) do {} while (0)
#define LWIP_PLATFORM_ASSERT(x) do { while(1){} } while (0)
#endif

#endif /* XY_LWIPOPTS_H */
