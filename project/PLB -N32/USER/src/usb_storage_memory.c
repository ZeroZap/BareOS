/**
 * @file usb_storage_memory.c
 * @author N32cube
 */

/* Includes ------------------------------------------------------------------*/

#include "usb_storage_memory.h"
#include "usb_storage_scsi_init.h"
#include "usb_storage_bot.h"
#include "usb_regs.h"
#include "usb_mem.h"
#include "usb_conf.h"
#include "usb_storage_flash_driver.h"
#include "usb_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t Block_Read_count = 0;
__IO uint32_t Block_offset;
__IO uint32_t Counter = 0;
uint32_t  Idx;
uint32_t Data_Buffer[PACK_MAX_SIZE]; /* one 2 KB flash page */
uint8_t TransferState = TXFR_IDLE;
/* Extern variables ----------------------------------------------------------*/
extern uint8_t Bulk_Data_Buff[PACK_MAX_SIZE];  /* data buffer*/
extern uint16_t Data_Len;
extern uint8_t Bot_State;
extern Bulk_Only_CBW CBW;
extern Bulk_Only_CSW CSW;
extern uint32_t Mass_Memory_Size[2];
extern uint32_t Mass_Block_Size[2];

/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : Read_Memory
* Description    : Handle the Read operation from the microSD card.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Read_Memory(uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length)
{
    static uint32_t Offset, Length;

    if (TransferState == TXFR_IDLE )
    {
        Offset = Memory_Offset * Mass_Block_Size[lun];
        Length = Transfer_Length * Mass_Block_Size[lun];
        TransferState = TXFR_ONGOING;
    }

    if (TransferState == TXFR_ONGOING )
    {
        if (!Block_Read_count)
        {
            MAL_Read(lun ,
                   Offset ,
                   Data_Buffer,
                   Mass_Block_Size[lun]);

            USB_SilWrite(EP4_IN, (uint8_t *)Data_Buffer, PACK_MAX_SIZE);

            Block_Read_count = Mass_Block_Size[lun] - PACK_MAX_SIZE;
            Block_offset = PACK_MAX_SIZE;
        }
        else
        {
            USB_SilWrite(EP4_IN, (uint8_t *)Data_Buffer + Block_offset, PACK_MAX_SIZE);

            Block_Read_count -= PACK_MAX_SIZE;
            Block_offset += PACK_MAX_SIZE;
        }

        USB_SetEpTxCnt(ENDP4, PACK_MAX_SIZE);
        SetEPTxStatus(ENDP4, EP_TX_VALID);  
        Offset += PACK_MAX_SIZE;
        Length -= PACK_MAX_SIZE;

        CSW.dDataResidue -= PACK_MAX_SIZE;

    }
    if (Length == 0)
    {
        Block_Read_count = 0;
        Block_offset = 0;
        Offset = 0;
        Bot_State = BOT_DATA_IN_LAST;
        TransferState = TXFR_IDLE;
    }
}

/*******************************************************************************
* Function Name  : Write_Memory
* Description    : Handle the Write operation to the microSD card.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Write_Memory (uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length)
{
    static uint32_t W_Offset, W_Length;

    uint32_t temp =  Counter + 64;

    if (TransferState == TXFR_IDLE )
    {
        W_Offset = Memory_Offset * Mass_Block_Size[lun];
        W_Length = Transfer_Length * Mass_Block_Size[lun];
        TransferState = TXFR_ONGOING;
    }

    if (TransferState == TXFR_ONGOING )
    {

        for (Idx = 0 ; Counter < temp; Counter++)
        {
            *((uint8_t *)Data_Buffer + Counter) = Bulk_Data_Buff[Idx++];
        }

        W_Offset += Data_Len;
        W_Length -= Data_Len;

        if (!(W_Length % Mass_Block_Size[lun]))
        {
            Counter = 0;
            MAL_Write(lun ,
                    W_Offset - Mass_Block_Size[lun],
                    Data_Buffer,
                    Mass_Block_Size[lun]);
        }

        CSW.dDataResidue -= Data_Len;
        SetEPRxStatus(ENDP5, EP_RX_VALID); /* enable the next transaction*/   
    }

    if ((W_Length == 0) || (Bot_State == BOT_CSW_Send))
    {
        Counter = 0;
        Set_CSW (CSW_CMD_PASSED, SEND_CSW_ENABLE);
        TransferState = TXFR_IDLE;
    }
}


