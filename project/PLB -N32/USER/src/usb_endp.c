
/**
 * @file usb_endp.c
 * @author N32cube
 */
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_conf.h"
#include "usb_pwr.h"
/*******************************************************************************
 * Function Name  : EP2_OUT_Callback.
 * Description    : EP2 OUT Callback Routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void EP2_OUT_Callback(void)
{
    uint16_t Data_Len;       /* data length*/
    Data_Len = USB_GetEpRxCnt(ENDP2);
    USB_CopyPMAToUserBuf(usbTxRxDataProp[USB_VCP].RxBuf+usbTxRxDataProp[USB_VCP].RxLen, ENDP2_RX_BUF0Addr, Data_Len);
    SetEPRxStatus(ENDP2, EP_RX_VALID);
    usbTxRxDataProp[USB_VCP].RxLen += Data_Len;
    if(usbTxRxDataProp[USB_VCP].RxLen>=usbTxRxDataProp[USB_VCP].WantRxLen)
    {
        usbTxRxDataProp[USB_VCP].IsRxComplet=true;
    }
}


/*******************************************************************************
 * Function Name  : EP3_IN_Callback.
 * Description    : EP3 IN Callback Routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void EP3_IN_Callback(void)
{
    uint32_t txLen=0;
    if(usbTxRxDataProp[USB_VCP].IsNeedTx[0])
    {
        if(usbTxRxDataProp[USB_VCP].WantTxLen[0]-usbTxRxDataProp[USB_VCP].TxLen[0]>=PACK_MAX_SIZE)
        {
            txLen=PACK_MAX_SIZE;
                        
        }
        else txLen=usbTxRxDataProp[USB_VCP].WantTxLen[0]-usbTxRxDataProp[USB_VCP].TxLen[0];
        _SetEPTxStatus(ENDP3, EP_TX_NAK);
        USB_SilWrite(EP3_IN, usbTxRxDataProp[USB_VCP].TxBuf[0]+usbTxRxDataProp[USB_VCP].TxLen[0], txLen);
        USB_SetEpTxCnt(ENDP3, txLen);
        _SetEPTxStatus(ENDP3, EP_TX_VALID);
        usbTxRxDataProp[USB_VCP].TxLen[0]+=txLen;
        if(usbTxRxDataProp[USB_VCP].WantTxLen[0]<=usbTxRxDataProp[USB_VCP].TxLen[0])
            usbTxRxDataProp[USB_VCP].IsNeedTx[0]=false;
    }
    else USB_SetEpTxCnt(ENDP3, 0);

}
/*******************************************************************************
* Function Name  : SOF_Callback / INTR_SOFINTR_Callback
* Description    :
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void SOF_Callback(void)
{
    static uint32_t FrameCount = 0;

    if(bDeviceState == CONFIGURED)
    {
        if (FrameCount++ == 5)
        {
            /* Reset the frame counter */
            FrameCount = 0;

            /* Check the data to be sent through IN pipe */
            //Handle_USBAsynchXfer();
        }
    } 
}


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
