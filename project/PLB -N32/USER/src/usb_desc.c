/**
 * @file usb_desc.c
 * @author N32cube
 */
#include "usb_lib.h"
#include "usb_desc.h"
/* NTFx CODE START */
/* USB Standard Device Descriptor */
const uint8_t USBdevice_DeviceDescriptor[USB_DEIVCE_SIZ_DEVICE_DESC] = {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0xEF, /*bDeviceClass*/
    0x02, /*bDeviceSubClass*/
    0x01, /*bDeviceProtocol*/
    0x40, /*bMaxPacketSize40*/
    0x00, /*idVendor*/
    0x00,
    0x00, /*idProduct*/
    0x00,
    0x00, /*bcdDevice rel. 2.00*/
    0x02,
    1,   /*Index of string descriptor describing
                       manufacturer */
    2,   /*Index of string descriptor describing
                      product*/
    3,   /*Index of string descriptor describing the
                      device serial number */
    0x01 /*bNumConfigurations*/
};       /* USBdevice_DeviceDescriptor */

/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t USBdevice_ConfigDescriptor[USB_DEIVCE_SIZ_CONFIG_DESC] = {
    0x09,                              /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    USB_DEIVCE_SIZ_CONFIG_DESC,
    /* wTotalLength: Bytes returned */
    0x00,
    0x03, /* bNumInterfaces: 3 interface */
    0x01, /* bConfigurationValue: Configuration value */
    0x00, /* iConfiguration: Index of string descriptor describing
                         the configuration*/
    0xC0, /* bmAttributes: Self powered */
    0x32, /* MaxPower 100 mA: this current is used for detecting Vbus */
    /* IAD */
    /* Interface Association Descriptor */
    USBD_IAD_DESC_SIZE,               // bLength
    USBD_IAD_DESCRIPTOR_TYPE,         // bDescriptorType
    0x00,                             // bFirstInterface 接口描述符是在总的配置描述符中的第几个从0开始数 1 
    0x02,                             // bInterfaceCount 接口描述符数量 2
    USB_DEVICE_CLASS_CDC,             // bFunctionClass    
    0x02,             // bFunctionSubClass  Abstract Control Model
    0x00,                             // bInterfaceProtocol  AT Commands: V.250 etc
    0x00,                             // iFunction
    
    /*USB CDC  interface descriptor*/
    /*Interface Descriptor*/
    USB_DEIVCE_NTERFACE_DESC_SIZE,   /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x01,   /* bNumEndpoints: One endpoints used */
    USB_DEVICE_CLASS_CDC,   /* bInterfaceClass: Communication Interface Class */
    0x02,   /* bInterfaceSubClass: Abstract Control Model */
    0x01,   /* bInterfaceProtocol: Common AT commands */
    0x00,   /* iInterface: */
    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,
    /*Call Management Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x01,   /* bDataInterface: 1 */
    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */
        
    /*Endpoint 2 Descriptor*/
    STANDARD_ENDPOINT_DESC_SIZE,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x81,   /* bEndpointAddress: */
    USB_ENDPOINT_TYPE_INTERRUPT,   /* bmAttributes: Interrupt */
    0x40,      /* wMaxPacketSize: */
    0x00,
    0xFF,   /* bInterval: */
        
    /*Data class interface descriptor*/
    USB_DEIVCE_NTERFACE_DESC_SIZE,   /* bLength: Endpoint Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0x0A,   /* bInterfaceClass: CDC */
    0x00,   /* bInterfaceSubClass: */
    0x00,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */
        
    /*Endpoint 3 Descriptor*/
    STANDARD_ENDPOINT_DESC_SIZE,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x02,   /* bEndpointAddress:  */
    USB_ENDPOINT_TYPE_BULK,   /* bmAttributes: Bulk */
    0x40,             /* wMaxPacketSize: */
    0x00,
    0x00,   /* bInterval: ignore for Bulk transfer */
        
    /*Endpoint 1 Descriptor*/
    STANDARD_ENDPOINT_DESC_SIZE,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    0x83,   /* bEndpointAddress: */
    USB_ENDPOINT_TYPE_BULK,   /* bmAttributes: Bulk */
    0x40,             /* wMaxPacketSize: */
    0x00,
    0x00,    /* bInterval */
             
    /*USB MASS storage  interface descriptor*/
    USB_DEIVCE_NTERFACE_DESC_SIZE,   //bLength
    USB_INTERFACE_DESCRIPTOR_TYPE,   //bDescriptorType
    0x02, //bInterfaceNumber
    0x00, //bAlternateSetting
    0x02, //bNumEndpoints
    USB_DEVICE_CLASS_MASS_STORAGE, //bInterfaceClass
    0x06, //bInterfaceSubClass
    0x50, //bInterfaceProtocol
    4, //iConfiguration

/* iInterface: */
    /* 18 */
    STANDARD_ENDPOINT_DESC_SIZE,   /*Endpoint descriptor length = 7*/
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /*Endpoint descriptor type */
    0x84,   /*Endpoint address  */
    USB_ENDPOINT_TYPE_BULK,   /*Bulk endpoint type */
    0x40,   /*Maximum packet size ( 64bytes) */
    0x00,
    0x00,   /*Polling interval in milliseconds */

    /* 25 */
    STANDARD_ENDPOINT_DESC_SIZE,   /*Endpoint descriptor length = 7 */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /*Endpoint descriptor type */
    0x05,   /*Endpoint address  */
    USB_ENDPOINT_TYPE_BULK,   /*Bulk endpoint type */
    0x40,   /*Maximum packet size (64 bytes) */
    0x00,
    0x00,     /*Polling interval in milliseconds*/
        };        /* USBdevice_ConfigDescriptor */

/* USB String Descriptors (optional) */
const uint8_t USBdevice_StringLangID[USB_DEIVCE_SIZ_STRING_LANGID] = {
    USB_DEIVCE_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x04,
    0x09}; /* English */

const uint8_t USBdevice_StringVendor[USB_DEIVCE_SIZ_STRING_VENDOR] = {
    USB_DEIVCE_SIZ_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,  /* bDescriptorType*/
        'N',0, 
        'A',0, 
        'T',0, 
        'I',0, 
        'O',0, 
        'N',0, 
        'S',0, 
        };

const uint8_t USBdevice_StringProduct[USB_DEIVCE_SIZ_STRING_PRODUCT] = 
{
    USB_DEIVCE_SIZ_STRING_PRODUCT, /* bLength */
    USB_STRING_DESCRIPTOR_TYPE, /* bDescriptorType */
        'N',0, 
        '3',0, 
        '2',0, 
        'L',0, 
        '4',0, 
        '0',0, 
        '6',0, 
        'C',0, 
        'B',0, 
        'L',0, 
        '7',0, 
        };
uint8_t USBdevice_StringSerial[USB_DEIVCE_SIZ_STRING_SERIAL]         = 
{
    USB_DEIVCE_SIZ_STRING_SERIAL, /* bLength */
    USB_STRING_DESCRIPTOR_TYPE, /* bDescriptorType */
        'N',0, 
        '3',0, 
        '2',0, 
        'L',0, 
        '4',0, 
        '0',0, 
        '6',0, 
        'C',0, 
        'B',0, 
        'L',0, 
        '7',0, 
        };
/* NTFx CODE END */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
