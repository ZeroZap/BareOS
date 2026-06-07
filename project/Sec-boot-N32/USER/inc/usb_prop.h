
/**
 * @file usb_prop.h
 * @author N32cube
 */
#ifndef __USB_PROP_H__
#define __USB_PROP_H__
#include "stdint.h"
#include "usb_core.h"

typedef struct
{
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
}LINE_CODING;

#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

void USBdevice_init(void);
void USBdevice_Reset(void);
void USBdevice_SetConfiguration(void);
void USBdevice_SetDeviceAddress(void);
void USBdevice_Status_In(void);
void USBdevice_Status_Out(void);
USB_Result USBdevice_Data_Setup(uint8_t);
USB_Result USBdevice_NoData_Setup(uint8_t);
USB_Result USBdevice_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t* USBdevice_GetDeviceDescriptor(uint16_t);
uint8_t* USBdevice_GetConfigDescriptor(uint16_t);
uint8_t* USBdevice_GetStringDescriptor(uint16_t);
USB_Result USBdevice_SetProtocol(void);
uint8_t* USBdevice_GetProtocolValue(uint16_t Length);
USB_Result USBdevice_SetProtocol(void);
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length);
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length);
#define SOF_CALLBACK
/* Exported define -----------------------------------------------------------*/
#define USBdevice_GetConfiguration USB_ProcessNop
//#define USBdevice_SetConfiguration          USB_ProcessNop
#define USBdevice_GetInterface       USB_ProcessNop
#define USBdevice_SetInterface       USB_ProcessNop
#define USBdevice_GetStatus          USB_ProcessNop
#define USBdevice_ClearFeature       USB_ProcessNop
#define USBdevice_SetEndPointFeature USB_ProcessNop
#define USBdevice_SetDeviceFeature   USB_ProcessNop
//#define USBdevice_SetDeviceAddress          USB_ProcessNop

#define REPORT_DESCRIPTOR 0x22

#endif /* __USB_PROP_H__ */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
