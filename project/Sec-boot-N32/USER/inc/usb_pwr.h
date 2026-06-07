
/**
 * @file usb_pwr.h
 * @author N32cube
 */
#ifndef __USB_PWR_H__
#define __USB_PWR_H__
#include "usb_core.h"
#include "usb_type.h"

typedef enum _RESUME_STATE
{
    RESUME_EXTERNAL,
    RESUME_INTERNAL,
    RESUME_LATER,
    RESUME_WAIT,
    RESUME_START,
    RESUME_ON,
    RESUME_OFF,
    RESUME_ESOF
} RESUME_STATE;

typedef enum _DEVICE_STATE
{
    UNCONNECTED,
    ATTACHED,
    POWERED,
    SUSPENDED,
    ADDRESSED,
    CONFIGURED
} DEVICE_STATE;

extern void Suspend(void);
extern void Resume_Init(void);
extern void Resume(RESUME_STATE eResumeSetVal);
extern USB_Result PowerOn(void);
extern USB_Result PowerOff(void);

extern __IO uint32_t bDeviceState; /* USB device status */
extern __IO bool fSuspendEnabled;  /* true when suspend is possible */

#endif /*__USB_PWR_H__*/

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
