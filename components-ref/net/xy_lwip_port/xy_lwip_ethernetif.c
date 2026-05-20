/**
 * @file xy_lwip_ethernetif.c
 * @brief Platform-specific Ethernet driver hooks for the lwIP port.
 *
 * This file is a TEMPLATE.  Copy it into your project directory and
 * replace the stub bodies with your actual MAC/PHY driver calls.
 *
 * The three functions here form the only hardware-specific interface
 * that the BareOS lwIP port layer needs:
 *
 *   xy_lwip_hw_init()    — one-time MAC/PHY hardware initialization
 *   xy_lwip_hw_input()   — drain RX FIFO/DMA into lwIP (called from poll)
 *   xy_lwip_hw_output()  — copy pbuf chain into TX FIFO/DMA
 */

#include "xy_lwip_port.h"
#include "lwipopts.h"

#include "lwip/pbuf.h"
#include "lwip/netif.h"

/* ────────────────────────────────────────────────────────────────────
 * ① Hardware initialization
 *    Called once by netif_init_cb() during xy_lwip_init().
 * ──────────────────────────────────────────────────────────────────── */

void xy_lwip_hw_init(struct netif *netif)
{
    /* Set MAC address — replace with your device's unique address. */
    netif->hwaddr_len = 6;
    netif->hwaddr[0]  = 0x02;
    netif->hwaddr[1]  = 0x00;
    netif->hwaddr[2]  = 0x00;
    netif->hwaddr[3]  = 0x00;
    netif->hwaddr[4]  = 0x00;
    netif->hwaddr[5]  = 0x01;

    /*
     * TODO: Initialize your MAC/PHY here, e.g.:
     *   ETH_Init();
     *   PHY_Init();
     *   ETH_Start();
     *
     * Enable the ETH RX interrupt and call xy_lwip_isr_notify_rx()
     * from inside the ISR:
     *   void ETH_IRQHandler(void) {
     *       if (ETH_GetRxFlag()) {
     *           xy_lwip_isr_notify_rx();
     *           ETH_ClearRxFlag();
     *       }
     *   }
     */
    (void)netif;
}

/* ────────────────────────────────────────────────────────────────────
 * ② Receive — drain hardware RX into lwIP
 *    Called by xy_lwip_poll() after ISR set the rx-pending flag.
 * ──────────────────────────────────────────────────────────────────── */

void xy_lwip_hw_input(struct netif *netif)
{
    /*
     * Read all available frames from the hardware RX FIFO / DMA.
     *
     * Pattern:
     *   while (frame available) {
     *       len   = ETH_GetRxFrameSize();
     *       pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
     *       if (!p) { ETH_DropRxFrame(); continue; }
     *
     *       // Copy frame bytes into pbuf chain
     *       for (struct pbuf *q = p; q; q = q->next) {
     *           ETH_ReadRxFIFO(q->payload, q->len);
     *       }
     *
     *       // Hand to lwIP — it will call ethernet_input() → ip4_input()
     *       if (netif->input(p, netif) != ERR_OK) {
     *           pbuf_free(p);
     *       }
     *   }
     *
     * DMA zero-copy optimisation:
     *   Map each DMA RX descriptor directly to a PBUF_POOL pbuf so no
     *   memcpy is needed.  Re-supply the descriptor with a fresh pbuf
     *   after handing the old one to lwIP.
     */
    (void)netif;
}

/* ────────────────────────────────────────────────────────────────────
 * ③ Transmit — copy pbuf chain into hardware TX
 *    Called by lwIP (via netif->linkoutput) when it wants to send.
 * ──────────────────────────────────────────────────────────────────── */

err_t xy_lwip_hw_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    /*
     * Iterate the pbuf chain; each node may hold a partial frame.
     *
     *   for (struct pbuf *q = p; q; q = q->next) {
     *       ETH_WriteTxFIFO(q->payload, q->len);
     *   }
     *   ETH_TriggerTx();
     *
     * DMA zero-copy optimisation:
     *   Map each pbuf node to a DMA TX descriptor (chained scatter-gather).
     *   Call pbuf_ref(p) to keep the pbuf alive until the DMA completes,
     *   then pbuf_free(p) in the TX-done interrupt.
     */
    (void)p;
    return ERR_OK;
}
