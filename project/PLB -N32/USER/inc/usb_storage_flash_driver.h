
/**
 * @file usb_storage_flash_driver.h
 * @author N32cube
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_storage_FLASH_DRIVER_H
#define __USB_storage_FLASH_DRIVER_H

#include "stdint.h"

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define MAL_OK   0
#define MAL_FAIL 1
#define MAX_LUN  1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

uint16_t MAL_Init (uint8_t lun);
uint16_t MAL_GetStatus (uint8_t lun);
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length);
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length);
#endif /* __MASS_MAL_H */

