

/**
 * @file usb_desc.h
 * @author N32cube
 */
#ifndef __USB_DESC_H__
#define __USB_DESC_H__
#include "stdint.h"

/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE        0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE 0x02
#define USB_STRING_DESCRIPTOR_TYPE        0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE     0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE      0x05

#define    USBD_IAD_DESCRIPTOR_TYPE       0x0B 

#define USB_DEVICE_CLASS_AUDIO                        0x01
#define USB_DEVICE_CLASS_CDC                          0x02
#define USB_DEVICE_CLASS_HID                          0x03
#define USB_DEVICE_CLASS_IMAGE                        0x06
#define USB_DEVICE_CLASS_PRINTER                      0x07
#define USB_DEVICE_CLASS_MASS_STORAGE                 0x08

#define HID_DESCRIPTOR_TYPE    0x21
#define USB_DEIVCE_SIZ_HID_DESC 0x09
#define USB_DEIVCE_OFF_HID_DESC 0x12

#define USB_DEIVCE_SIZ_DEVICE_DESC    18
#define USB_DEIVCE_SIZ_CONFIG_DESC    0x43
#define USB_DEIVCE_SIZ_REPORT_DESC    41
#define USB_DEIVCE_SIZ_STRING_LANGID  4
#define USB_DEIVCE_SIZ_STRING_VENDOR  0x10
#define USB_DEIVCE_SIZ_STRING_PRODUCT 0x18
#define USB_DEIVCE_SIZ_STRING_SERIAL  0x18
#define USB_DEIVCE_NTERFACE_DESC_SIZE 0x09

#define STANDARD_ENDPOINT_DESC_SIZE 0x07
#define AUDIO_ENDPOINT_DESC_SIZE 0x09
#define USBD_IAD_DESC_SIZE  0x08

#define USB_ENDPOINT_TYPE_CONTROL            0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS        0x01
#define USB_ENDPOINT_TYPE_BULK               0x02
#define USB_ENDPOINT_TYPE_INTERRUPT          0x03




extern const uint8_t USBdevice_DeviceDescriptor[USB_DEIVCE_SIZ_DEVICE_DESC];
extern const uint8_t USBdevice_ConfigDescriptor[USB_DEIVCE_SIZ_CONFIG_DESC];
extern const uint8_t USBdevice_StringLangID[USB_DEIVCE_SIZ_STRING_LANGID];
extern const uint8_t USBdevice_StringVendor[USB_DEIVCE_SIZ_STRING_VENDOR];
extern const uint8_t USBdevice_StringProduct[USB_DEIVCE_SIZ_STRING_PRODUCT];
extern uint8_t USBdevice_StringSerial[USB_DEIVCE_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H__ */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
