
/**
 * @file usb_prop.c
 * @author N32cube
 */
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
 
#include "usb_storage_bot.h"
#include "usb_storage_flash_driver.h"
extern uint32_t Max_Lun;

uint32_t ProtocolValue;
__IO uint8_t EXTI_Enable;
__IO uint8_t Request = 0;
uint8_t Report_Buf[2];
LINE_CODING linecoding =
{
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* no. of bits 8*/
};
/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */

USB_Device Device_Table = {EP_NUM, 1};

DEVICE_PROP Device_Property = {
    USBdevice_init,
    USBdevice_Reset,
    USBdevice_Status_In,
    USBdevice_Status_Out,
    USBdevice_Data_Setup,
    USBdevice_NoData_Setup,
    USBdevice_Get_Interface_Setting,
    USBdevice_GetDeviceDescriptor,
    USBdevice_GetConfigDescriptor,
    USBdevice_GetStringDescriptor,
    0,
    0x40 /*MAX PACKET SIZE*/
};
USER_STANDARD_REQUESTS User_Standard_Requests = {USBdevice_GetConfiguration,
                                                 USBdevice_SetConfiguration,
                                                 USBdevice_GetInterface,
                                                 USBdevice_SetInterface,
                                                 USBdevice_GetStatus,
                                                 USBdevice_ClearFeature,
                                                 USBdevice_SetEndPointFeature,
                                                 USBdevice_SetDeviceFeature,
                                                 USBdevice_SetDeviceAddress};

USB_OneDescriptor Device_Descriptor = {(uint8_t*)USBdevice_DeviceDescriptor, USB_DEIVCE_SIZ_DEVICE_DESC};

USB_OneDescriptor Config_Descriptor = {(uint8_t*)USBdevice_ConfigDescriptor, USB_DEIVCE_SIZ_CONFIG_DESC};

USB_OneDescriptor String_Descriptor[4] = {{(uint8_t*)USBdevice_StringLangID, USB_DEIVCE_SIZ_STRING_LANGID},
                                          {(uint8_t*)USBdevice_StringVendor, USB_DEIVCE_SIZ_STRING_VENDOR},
                                          {(uint8_t*)USBdevice_StringProduct, USB_DEIVCE_SIZ_STRING_PRODUCT},
                                          {(uint8_t*)USBdevice_StringSerial, USB_DEIVCE_SIZ_STRING_SERIAL}};

/*USBdevice_SetReport_Feature function prototypes*/
uint8_t* USBdevice_SetReport_Feature(uint16_t Length);

/* Extern function prototypes ------------------------------------------------*/

/*******************************************************************************
 * Function Name  : USBdevice_init.
 * Description    : Custom HID init routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_init(void)
{
    /* Update the serial number string descriptor with the data from the unique
    ID*/
    pInformation->CurrentConfiguration = 0;
    /* Connect the device */
    PowerOn();

    /* Perform basic device initialization operations */
    USB_SilInit();

    bDeviceState = UNCONNECTED;
}

/*******************************************************************************
 * Function Name  : USBdevice_Reset.
 * Description    : Custom HID reset routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_Reset(void)
{
    /* Set USBdevice_DEVICE as not configured */
    pInformation->CurrentConfiguration = 0;
    pInformation->CurrentInterface     = 0; /*the default Interface*/

    /* Current Feature initialization */
    pInformation->CurrentFeature = USBdevice_ConfigDescriptor[7];

    USB_SetBuftab(BTABLE_ADDRESS);

    /* Initialize Endpoint 0 */
    USB_SetEpType(ENDP0, EP_CONTROL);
    SetEPTxStatus(ENDP0, EP_TX_STALL);
    USB_SetEpRxAddr(ENDP0, ENDP0_RXADDR);
    USB_SetEpTxAddr(ENDP0, ENDP0_TXADDR);
    USB_ClrStsOut(ENDP0);
    USB_SetEpRxCnt(ENDP0, Device_Property.MaxPacketSize);
    USB_SetEpRxValid(ENDP0);

    /* Initialize Endpoint 1 */
    USB_SetEpType(ENDP1, EP_INTERRUPT);
    USB_SetEpTxAddr(ENDP1, ENDP1_TX_BUF0Addr);
    SetEPTxStatus(ENDP1, EP_TX_VALID);
    SetEPRxStatus(ENDP1, EP_RX_DIS);

    /* Initialize Endpoint 2 */
    USB_SetEpType(ENDP2, EP_BULK);
  
    USB_SetEpRxAddr(ENDP2, ENDP2_RX_BUF0Addr);
    USB_SetEpRxCnt(ENDP2, 0x40);
    SetEPRxStatus(ENDP2, EP_RX_VALID);
    SetEPTxStatus(ENDP2, EP_TX_DIS);

    /* Initialize Endpoint 3 */
    USB_SetEpType(ENDP3, EP_BULK);
    USB_SetEpTxAddr(ENDP3, ENDP3_TX_BUF0Addr);
    SetEPTxStatus(ENDP3, EP_TX_VALID);
    SetEPRxStatus(ENDP3, EP_RX_DIS);

    /* Initialize Endpoint 4 */
    USB_SetEpType(ENDP4, EP_BULK);
    USB_SetEpTxAddr(ENDP4, ENDP4_TX_BUF0Addr);
    SetEPTxStatus(ENDP4, EP_TX_VALID);
    SetEPRxStatus(ENDP4, EP_RX_DIS);

    /* Initialize Endpoint 5 */
    USB_SetEpType(ENDP5, EP_BULK);
  
    USB_SetEpRxAddr(ENDP5, ENDP5_RX_BUF0Addr);
    USB_SetEpRxCnt(ENDP5, 0x40);
    SetEPRxStatus(ENDP5, EP_RX_VALID);
    SetEPTxStatus(ENDP5, EP_TX_DIS);

    /* Set this device to response on default address */
    USB_SetDeviceAddress(0);
    bDeviceState = ATTACHED;
    CBW.dSignature = BOT_CBW_SIGNATURE;
    Bot_State = BOT_IDLE;
    MAL_Init(0);
}
/*******************************************************************************
 * Function Name  : USBdevice_SetConfiguration.
 * Description    : Update the device state to configured and command the ADC
 *                  conversion.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_SetConfiguration(void)
{
    if (pInformation->CurrentConfiguration != 0)
    {
        /* Device configured */
        bDeviceState = CONFIGURED;
        /* Initialize Endpoint 4 */
        USB_ClrDattogTx(ENDP4);

        /* Initialize Endpoint 5 */
        USB_ClrDattogRx(ENDP5);
        Bot_State = BOT_IDLE; /* set the Bot state machine to the IDLE state */
    }
}
/*******************************************************************************
* Function Name  : USBdevice_ClearFeature
* Description    : Handle the ClearFeature request.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USBdevice_ClearFeature(void)
{
    /* when the host send a CBW with invalid signature or invalid length the two
       Endpoints (IN & OUT) shall stall until receiving a Mass Storage Reset     */
    if (CBW.dSignature != BOT_CBW_SIGNATURE)
        Bot_Abort(BOTH_DIR);
}
/*******************************************************************************
 * Function Name  : USBdevice_SetConfiguration.
 * Description    : Update the device state to addressed.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_SetDeviceAddress(void)
{
    bDeviceState = ADDRESSED;
}
/*******************************************************************************
 * Function Name  : USBdevice_Status_In.
 * Description    : Joystick status IN routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_Status_In(void)
{
}

/*******************************************************************************
 * Function Name  : USBdevice_Status_Out
 * Description    : Joystick status OUT routine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USBdevice_Status_Out(void)
{
}

/*******************************************************************************
 * Function Name  : USBdevice_Data_Setup
 * Description    : Handle the data class specific requests.
 * Input          : Request Nb.
 * Output         : None.
 * Return         : UnSupport or Success.
 *******************************************************************************/
USB_Result USBdevice_Data_Setup(uint8_t RequestNo)
{
    uint8_t* (*CopyRoutine)(uint16_t);

//    if (pInformation->USBwIndex != 0)
//        return UnSupport;

    CopyRoutine = NULL;
    if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
            && (RequestNo == GET_MAX_LUN) && (pInformation->USBwValue == 0)
            && (pInformation->USBwLength == 0x01))
    {
        CopyRoutine = Get_Max_Lun;
    }
    if (RequestNo == GET_LINE_CODING)
    {
        if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
        {
            CopyRoutine = Virtual_Com_Port_GetLineCoding;
        }
    }
    else if (RequestNo == SET_LINE_CODING)
    {
        if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
        {
            CopyRoutine = Virtual_Com_Port_SetLineCoding;
        }
        Request = SET_LINE_CODING;
    }
    if (CopyRoutine == NULL)
    {
        return UnSupport;
    }

    pInformation->Ctrl_Info.CopyData    = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return Success;
}

/*******************************************************************************
 * Function Name  : USBdevice_SetReport_Feature
 * Description    : Set Feature request handling
 * Input          : Length.
 * Output         : None.
 * Return         : Buffer
 *******************************************************************************/
uint8_t* USBdevice_SetReport_Feature(uint16_t Length)
{
    if (Length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = 2;
        return NULL;
    }
    else
    {
        return Report_Buf;
    }
}

/*******************************************************************************
 * Function Name  : USBdevice_NoData_Setup
 * Description    : handle the no data class specific requests
 * Input          : Request Nb.
 * Output         : None.
 * Return         : UnSupport or Success.
 *******************************************************************************/
USB_Result USBdevice_NoData_Setup(uint8_t RequestNo)
{
    if ((Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
            && (RequestNo == MASS_STORAGE_RESET) && (pInformation->USBwValue == 0)
            && (pInformation->USBwIndex == 0) && (pInformation->USBwLength == 0x00))
    {
        /* Initialize Endpoint 4 */
        USB_ClrDattogTx(ENDP4);

        /* Initialize Endpoint 5 */
        USB_ClrDattogRx(ENDP5);

        /*intialise the CBW signature to enable the clear feature*/
        CBW.dSignature = BOT_CBW_SIGNATURE;
        Bot_State = BOT_IDLE;

        return Success;
    }

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
        if (RequestNo == SET_COMM_FEATURE)
        {
            return Success;
        }
        else if (RequestNo == SET_CONTROL_LINE_STATE)
        {
            return Success;
        }
    }
    
        return UnSupport;
}

/*******************************************************************************
 * Function Name  : USBdevice_GetDeviceDescriptor.
 * Description    : Gets the device descriptor.
 * Input          : Length
 * Output         : None.
 * Return         : The address of the device descriptor.
 *******************************************************************************/
uint8_t* USBdevice_GetDeviceDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

/*******************************************************************************
 * Function Name  : USBdevice_GetConfigDescriptor.
 * Description    : Gets the configuration descriptor.
 * Input          : Length
 * Output         : None.
 * Return         : The address of the configuration descriptor.
 *******************************************************************************/
uint8_t* USBdevice_GetConfigDescriptor(uint16_t Length)
{
    return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

/*******************************************************************************
 * Function Name  : USBdevice_GetStringDescriptor
 * Description    : Gets the string descriptors according to the needed index
 * Input          : Length
 * Output         : None.
 * Return         : The address of the string descriptors.
 *******************************************************************************/
uint8_t* USBdevice_GetStringDescriptor(uint16_t Length)
{
    uint8_t wValue0 = pInformation->USBwValue0;
    if (wValue0 > 4)
    {
        return NULL;
    }
    else
    {
        return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
    }
}
/*******************************************************************************
 * Function Name  : USBdevice_Get_Interface_Setting.
 * Description    : tests the interface and the alternate setting according to the
 *                  supported one.
 * Input          : - Interface : interface number.
 *                  - AlternateSetting : Alternate Setting number.
 * Output         : None.
 * Return         : Success or UnSupport.
 *******************************************************************************/
USB_Result USBdevice_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
    if (AlternateSetting > 0)
    {
        return UnSupport;
    }
    else if (Interface > 3)
    {
        return UnSupport;
    }
    return Success;
}

/*******************************************************************************
 * Function Name  : USBdevice_SetProtocol
 * Description    : Joystick Set Protocol request routine.
 * Input          : None.
 * Output         : None.
 * Return         : USB SUCCESS.
 *******************************************************************************/
USB_Result USBdevice_SetProtocol(void)
{
    uint8_t wValue0 = pInformation->USBwValue0;
    ProtocolValue   = wValue0;
    return Success;
}

/*******************************************************************************
 * Function Name  : USBdevice_GetProtocolValue
 * Description    : get the protocol value
 * Input          : Length.
 * Output         : None.
 * Return         : address of the protocol value.
 *******************************************************************************/
uint8_t* USBdevice_GetProtocolValue(uint16_t Length)
{
    if (Length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = 1;
        return NULL;
    }
    else
    {
        return (uint8_t*)(&ProtocolValue);
    }
}
/*******************************************************************************
* Function Name  : Get_Max_Lun
* Description    : Handle the Get Max Lun request.
* Input          : uint16_t Length.
* Output         : None.
* Return         : None.
*******************************************************************************/
uint8_t *Get_Max_Lun(uint16_t Length)
{
    if (Length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = LUN_DATA_LENGTH;
        return 0;
    }
    else
    {
        return((uint8_t*)(&Max_Lun));
    }
}
/*******************************************************************************
* Function Name  : Virtual_Com_Port_GetLineCoding.
* Description    : send the linecoding structure to the PC host.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length)
{
    if (Length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
        return NULL;
    }
    return(uint8_t *)&linecoding;
}

/*******************************************************************************
* Function Name  : Virtual_Com_Port_SetLineCoding.
* Description    : Set the linecoding structure fields.
* Input          : Length.
* Output         : None.
* Return         : Linecoding structure base address.
*******************************************************************************/
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length)
{
    if (Length == 0)
    {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
        return NULL;
    }
    return(uint8_t *)&linecoding;
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
