
/**
 * @file usb_storage_memory.h
 * @author N32cube
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _USB_storage_MEMEORY_H
#define _USB_storage_MEMEORY_H

/* Includes ------------------------------------------------------------------*/
#include "usb_conf.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define TXFR_IDLE     0
#define TXFR_ONGOING  1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Write_Memory (uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);
void Read_Memory (uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);
#endif /* __memory_H */
