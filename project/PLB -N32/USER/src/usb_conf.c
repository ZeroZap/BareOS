/**
 * @file usb_conf.c
 * @author N32cube
 */

#include "usb_conf.h"
#include "usb_pwr.h"

TxRxDataProp usbTxRxDataProp[2];
/*******************************************************************************
 * Function Name  : USBdevice_Tramsmit
 * Description    : tramsmit data
 * Input          : USBdevice,
                    TxBuff,
                    Length.
                    buffNum,0/1,Single buffers can only be 0, and double buffers can be 0/1
 * Output         : None.
 * Return         : address of the protocol value.
 *******************************************************************************/
void USBdevice_Tramsmit(USBdeviceType USBdevice,uint8_t * TxBuff, uint16_t Length,uint8_t buffNum)
{
    if(USBdevice>=USB_INVALID) return;
    if(buffNum>=2) return;
    usbTxRxDataProp[USBdevice].IsNeedTx[buffNum]=true;
    usbTxRxDataProp[USBdevice].TxBuf[buffNum]=TxBuff;
    usbTxRxDataProp[USBdevice].WantTxLen[buffNum]=Length;
    usbTxRxDataProp[USBdevice].TxLen[buffNum]=0;
    switch (USBdevice)
    {
        case USB_VCP:
            USB_SetEpTxCnt(VCP_ENDP, Length);
            USB_SetEpTxValid(VCP_ENDP); 
            break;
        
        default:
          break;
    }
    
}

/*******************************************************************************
 * Function Name  : USBdevice_PrepareReceive
 * Description    : prepare receive data
 * Input          : USBdevice
                    TxBuff,
                    Length.
 * Output         : None.
 * Return         : address of the protocol value.
 *******************************************************************************/
void USBdevice_PrepareReceive(USBdeviceType USBdevice,uint8_t * RxBuff, uint16_t Length)
{
    usbTxRxDataProp[USBdevice].RxBuf=RxBuff;
    usbTxRxDataProp[USBdevice].WantRxLen=Length;
    usbTxRxDataProp[USBdevice].RxLen=0;
    usbTxRxDataProp[USBdevice].IsRxComplet=false;
    
}

/*******************************************************************************
 * Function Name  : USBdevice_GetReceiveDataLen
 * Description    : get receive data len
 * Input          : USBdevice,
 * Output         : Length.
 * Return         : address of the protocol value.
 *******************************************************************************/
uint32_t USBdevice_GetReceiveDataLen(USBdeviceType USBdevice )
{
    return usbTxRxDataProp[USBdevice].RxLen;
}
/*******************************************************************************
 * Function Name  : Enter_LowPowerMode.
 * Description    : Power-off system clocks and power while entering suspend mode.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Enter_LowPowerMode(void)
{
    /* Set the device state to suspend */
    bDeviceState = SUSPENDED;
}

/*******************************************************************************
 * Function Name  : Leave_LowPowerMode.
 * Description    : Restores system clocks and power while exiting suspend mode.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Leave_LowPowerMode(void)
{
    USB_DeviceMess* pInfo = &Device_Info;

    /* Set the device state to the correct state */
    if (pInfo->CurrentConfiguration != 0)
    {
        /* Device configured */
        bDeviceState = CONFIGURED;
    }
    else
    {
        bDeviceState = ATTACHED;
    }
}

