/**
 * @file usb_storage_flash_driver.c
 * @author N32cube
 */
/* Includes ------------------------------------------------------------------*/
/* NTFx CODE START */
#include "usb_storage_flash_driver.h"
/* NTFx CODE END */
/* Private define ------------------------------------------------------------*/

#define FLASH_START_ADDR    0x08010000  // Flash start address
#define FLASH_SIZE          0x70000
#define FLASH_PAGE_SIZE     0x800       // 2K per page
/* NTFx CODE START */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];
/* logic unit count; the first is 0 */
uint32_t Max_Lun = 0;
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
/* NTFx CODE END */


/* NTFx CODE START */
    return MAL_OK;

}
/* NTFx CODE END */


/* NTFx CODE START */
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
/* NTFx CODE END */


/* NTFx CODE START */
    return MAL_OK;
}
/* NTFx CODE END */


/* NTFx CODE START */
/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
/* NTFx CODE END */


/* NTFx CODE START */
    return MAL_OK;
}
/* NTFx CODE END */


/* NTFx CODE START */
/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
    if (lun == 0)
    {
        Mass_Block_Count[0] = FLASH_SIZE/FLASH_PAGE_SIZE; 
        Mass_Block_Size[0] =  FLASH_PAGE_SIZE; 
        Mass_Memory_Size[0] = FLASH_SIZE; 
        return MAL_OK;
    }
    return MAL_FAIL;
}
/* NTFx CODE END */

