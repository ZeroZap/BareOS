
/**
 * @file usb_conf.h
 * @author N32cube
 */
#ifndef __USB_CONF_H__
#define __USB_CONF_H__
#include <stdint.h>
#include <stdbool.h>
#include "usb_lib.h"

typedef  struct TxRxDataProp
{
    uint8_t *RxBuf;
    uint32_t WantRxLen;
    uint32_t RxLen;
    bool  IsRxComplet;
    
    uint8_t *TxBuf[2];//单缓冲用TxBuff[0],双缓冲用2个
    uint32_t TxLen[2];
    uint32_t WantTxLen[2];
    bool  IsNeedTx[2];

}TxRxDataProp;
typedef enum USBdeviceType_t
{
    USB_VCP,
   USB_INVALID,
}USBdeviceType;

extern TxRxDataProp usbTxRxDataProp[2];

#define  PACK_MAX_SIZE  0x40
/*-------------------------------------------------------------*/
/* EP_NUM */
/* defines how many endpoints are used by the device */
/*-------------------------------------------------------------*/
#define EP_NUM (4)


#define VCP_ENDP  ENDP3

/*-------------------------------------------------------------*/
/* --------------   Buffer Description Table  -----------------*/
/*-------------------------------------------------------------*/
/* buffer table base address */
/* buffer table base address */
#define BTABLE_ADDRESS (0x00)

/* EP0  */
/* rx/tx buffer base address */
#define ENDP0_RXADDR (0x40)
#define ENDP0_TXADDR (0x80)
/* EP1buffer base address*/
#define ENDP1_TX_BUF0Addr      (0xC0)
/* EP2buffer base address*/
#define ENDP2_RX_BUF0Addr      (0x100)
/* EP3buffer base address*/
#define ENDP3_TX_BUF0Addr      (0x140)
/*-------------------------------------------------------------*/
/* -------------------   STS events  -------------------------*/
/*-------------------------------------------------------------*/
/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */

#define IMR_MSK (CTRL_CTRSM | CTRL_RSTM)

/* CTR service routines */
/* associated to defined endpoints */
#define EP1_IN_Callback USB_ProcessNop
#define EP2_IN_Callback USB_ProcessNop
#define EP4_IN_Callback USB_ProcessNop
#define EP5_IN_Callback USB_ProcessNop
#define EP6_IN_Callback USB_ProcessNop
#define EP7_IN_Callback USB_ProcessNop
#define EP1_OUT_Callback USB_ProcessNop
#define EP3_OUT_Callback USB_ProcessNop
#define EP4_OUT_Callback USB_ProcessNop
#define EP5_OUT_Callback USB_ProcessNop
#define EP6_OUT_Callback USB_ProcessNop
#define EP7_OUT_Callback USB_ProcessNop

extern void USBdevice_Tramsmit(USBdeviceType USBdevice,uint8_t * TxBuff, uint16_t Length,uint8_t buffNum);
extern void USBdevice_PrepareReceive(USBdeviceType USBdevice,uint8_t * RxBuff, uint16_t Length);
extern uint32_t USBdevice_GetReceiveDataLen(USBdeviceType USBdevice);
extern void Enter_LowPowerMode(void);
extern void Leave_LowPowerMode(void);

#endif /*__USB_CONF_H__*/

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
