/**
 * @file n32l40x_cfg.c
 * @author N32cube
 */

 #include "n32l40x_cfg.h"
#include <stdio.h>
/* NTFx CODE START */
__IO uint32_t mwTick;
volatile uint32_t g_n32_debug_log_tx_count;
volatile uint8_t g_n32_debug_log_last_char;
void SysTick_Delayms(uint32_t Delayms)
{
    uint32_t tickstart = mwTick;
    uint32_t last_tick = tickstart;
    uint32_t stalled_loops = 0;
    uint32_t wait=Delayms;
    /* Add 1 to guarantee minimum wait */
    if (wait < 0xFFFFFFFFU)
    {
        wait +=1;
    }
    while ((mwTick - tickstart) < wait)
    {
        IWDG_ReloadKey();
        if (mwTick != last_tick) {
            last_tick = mwTick;
            stalled_loops = 0;
        } else if (++stalled_loops >= 2000000U) {
            break;
        }
    }
}
 /**
 *@name  DMA_SetPerMemAddr.
 *@brief Set peripher address and memory address of DMA
 *@param DMAChx (The input parameters must be the following values):
 *          - DMA_CH1
 *          - DMA_CH2
 *          - DMA_CH3
 *          - DMA_CH4
 *          - DMA_CH5
 *          - DMA_CH6
 *          - DMA_CH7
 *          - DMA_CH8
 *@param periphAddr   peripher address
 *@param memAddr   memory address
 *@param bufSize   buff size
 *@return status
 */
 void DMA_SetPerMemAddr(DMA_ChannelType* DMAChx, uint32_t periphAddr,uint32_t memAddr,uint32_t bufSize )
 {
     /* DMAy Channelx TXNUM Configuration */
    /* Write to DMAy Channelx TXNUM */
    DMAChx->TXNUM = bufSize;

    /* DMAy Channelx PADDR Configuration */
    /* Write to DMAy Channelx PADDR */
    DMAChx->PADDR = periphAddr;

    /* DMAy Channelx MADDR Configuration */
    /* Write to DMAy Channelx MADDR */
    DMAChx->MADDR = memAddr;
 }
/* NTFx CODE END */

void n32_debug_log_char(char ch)
{
    if (ch == '\n') {
        n32_debug_log_char('\r');
    }

    while (USART_GetFlagStatus(UART4, USART_FLAG_TXDE) == RESET) {
    }
    USART_SendData(UART4, (uint16_t)ch);
    g_n32_debug_log_last_char = (uint8_t)ch;
    g_n32_debug_log_tx_count++;
}

void n32_debug_log_write(const char *str)
{
    while (*str != '\0') {
        n32_debug_log_char(*str++);
    }
}

void xy_log_char(char ch)
{
    n32_debug_log_char(ch);
}

int fputc(int ch, FILE *f)
{
    (void)f;
    n32_debug_log_char((char)ch);
    return ch;
}

int _write(int file, char *ptr, int len)
{
    int i;

    (void)file;
    for (i = 0; i < len; i++) {
        n32_debug_log_char(ptr[i]);
    }
    return len;
}

/* NTFx CODE START */
/**
 *@brief Initializes the clock tree
 *@param null
 *@return status
 */
bool RCC_Configuration(void)
{
    ErrorStatus ClockStatus;
    uint32_t timeout;
    RCC_DeInit();
    RCC_ConfigHclk(RCC_SYSCLK_DIV1);
    RCC_ConfigPclk2(RCC_HCLK_DIV4);
    RCC_ConfigPclk1(RCC_HCLK_DIV4);
     
    RCC_EnableHsi(ENABLE);
    /* Wait till HSI is ready */
    ClockStatus = RCC_WaitHsiStable();
    if (ClockStatus != SUCCESS) return false;
     
    /* PLB-N32 uses HSI PLL: 16MHz / 2 * 12 / 2 = 48MHz SYSCLK.
     * HSE is not used, so do not wait for HSERDF after disabling it. */
    RCC_ConfigHse(RCC_HSE_DISABLE);
     
    /* Enable PWR peripheral clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    /* Enable access to backup registers */
    PWR_BackupAccessEnable(ENABLE);
    
    /* Enable LSI */
    RCC_EnableLsi(ENABLE);
    /* Wait till LSI is ready */
    while (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_LSIRD) == RESET);
     
    RCC_ConfigLse(RCC_LSE_ENABLE,0x1FF);
    /* Do not block UART bring-up forever if the 32.768 kHz crystal is absent or slow. */
    timeout = 0x20000U;
    while ((RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) != SET) && (timeout-- > 0U));
     
    RCC_ConfigPll(RCC_PLL_HSI_PRE_DIV2,RCC_PLL_MUL_12,RCC_PLLDIVCLK_ENABLE);
    /* Enable PLL */
    RCC_EnablePll(ENABLE);
    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) != SET);
     
    /* Disable Prefetch Buffer */
    FLASH_PrefetchBufSet(FLASH_PrefetchBuf_DIS);
    /* Enable iCache */
    FLASH_iCacheCmd(FLASH_iCache_EN);
    /* Flash wait state */
    FLASH_SetLatency(FLASH_LATENCY_1);
     
    /*selsect HSI as RCC ADC1M CLK Source*/
    RCC_ConfigAdc1mClk(RCC_ADC1MCLK_SRC_HSI, RCC_ADC1MCLK_DIV16);  
    RCC_ConfigAdcPllClk(RCC_ADCPLLCLK_DIV2,ENABLE);
    RCC_ConfigAdcHclk(RCC_ADCHCLK_DIV1);
     
    /*config RNG clock*/
    RCC_ConfigTrng1mClk(RCC_TRNG1MCLK_SRC_HSI, RCC_TRNG1MCLK_DIV16);
    RCC_EnableTrng1mClk(ENABLE);
    RCC_ConfigRngcClk(RCC_RNGCCLK_SYSCLK_DIV1);
     
    /*config LPTIM clock*/
    RCC_ConfigLPTIMClk(RCC_LPTIMCLK_SRC_APB1);
    RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM,ENABLE);
     
    /*config RTC clock*/
    if (RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) == SET) {
        RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSE);
    } else {
        RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSI);
    }
    RCC_EnableRtcClk(ENABLE);
     
    /*config USB clock*/
    RCC_ConfigUsbClk(RCC_USBCLK_SRC_PLLCLK_DIV1);
     
    /* Select PLLCLK as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);
    /* Wait till PLLCLK is used as system clock source */
    while (RCC_GetSysclkSrc() != 0x0c) ;
    /*  Configure the SysTick to have interrupt in 1ms time basis*/
    SysTick_Config(48000);
    NVIC_SetPriority(SysTick_IRQn, 0);
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the NVIC
 *@param null
 *@return status
 */
bool NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;
    /*Configure the preemption priority and subpriority:
    - 4 bits for pre-emption priority: possible value are 0..15
    - 0 bits for subpriority: possible value are 0
    - Lower values gives higher priority
    */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    /*Set EXTI1  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set DMA_Channel1  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =DMA_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set USB_LP  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =USB_LP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set USART1  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set USART2  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set USBWakeUp  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =USBWakeUp_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /*Set UART4  interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   =UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        =0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the DMA
 *@param null
 *@return status
 */
bool DMA_Configuration(void)
{
    DMA_InitType DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);
     
    /* Enable DMA clock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA,ENABLE);
    /*USART2.RX---DMA1.CH1 DMA configuration*/
    DMA_DeInit(DMA_CH1);
    //DMA_InitStructure.PeriphAddr     = (uint32_t)&USART2->DAT;
    //DMA_InitStructure.MemAddr        = (uint32_t)USART2_RX_Mem;
    //DMA_InitStructure.BufSize        = DMA1_CH1_BufSize;//set it by DMA_SetPerMemAddr(DMA_CH1,&USART2->DAT,USART2_RX_Mem,DMA_CH1_BufSize)
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL;
    DMA_InitStructure.Priority       = DMA_PRIORITY_LOW;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH1, &DMA_InitStructure);
    
    DMA_RequestRemap(DMA_REMAP_USART2_RX,DMA,DMA_CH1,ENABLE);
     
    /*USART1.RX---DMA1.CH2 DMA configuration*/
    DMA_DeInit(DMA_CH2);
    //DMA_InitStructure.PeriphAddr     = (uint32_t)&USART1->DAT;
    //DMA_InitStructure.MemAddr        = (uint32_t)USART1_RX_Mem;
    //DMA_InitStructure.BufSize        = DMA1_CH2_BufSize;//set it by DMA_SetPerMemAddr(DMA_CH2,&USART1->DAT,USART1_RX_Mem,DMA_CH2_BufSize)
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_Init(DMA_CH2, &DMA_InitStructure);
    
    DMA_RequestRemap(DMA_REMAP_USART1_RX,DMA,DMA_CH2,ENABLE);
     
    /*USART3.RX---DMA1.CH3 DMA configuration*/
    DMA_DeInit(DMA_CH3);
    //DMA_InitStructure.PeriphAddr     = (uint32_t)&USART3->DAT;
    //DMA_InitStructure.MemAddr        = (uint32_t)USART3_RX_Mem;
    //DMA_InitStructure.BufSize        = DMA1_CH3_BufSize;//set it by DMA_SetPerMemAddr(DMA_CH3,&USART3->DAT,USART3_RX_Mem,DMA_CH3_BufSize)
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_Init(DMA_CH3, &DMA_InitStructure);
    
    DMA_RequestRemap(DMA_REMAP_USART3_RX,DMA,DMA_CH3,ENABLE);
     
    /*UART5.RX---DMA1.CH4 DMA configuration*/
    DMA_DeInit(DMA_CH4);
    //DMA_InitStructure.PeriphAddr     = (uint32_t)&UART5->DAT;
    //DMA_InitStructure.MemAddr        = (uint32_t)UART5_RX_Mem;
    //DMA_InitStructure.BufSize        = DMA1_CH4_BufSize;//set it by DMA_SetPerMemAddr(DMA_CH4,&UART5->DAT,UART5_RX_Mem,DMA_CH4_BufSize)
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_Init(DMA_CH4, &DMA_InitStructure);
    
    DMA_RequestRemap(DMA_REMAP_UART5_RX,DMA,DMA_CH4,ENABLE);
     
    /* Clear DMA Channel1 interrupt flag*/
    DMA_ClearFlag(DMA_FLAG_TC1|DMA_FLAG_HT1|DMA_FLAG_TE1, DMA);
    /* Enable DMA Channel1TXC|HTX|ERR interrupt */
    DMA_ConfigInt(DMA_CH1,DMA_INT_TXC|DMA_INT_HTX|DMA_INT_ERR, ENABLE);
     
    /* Use reference:  need to  set Address and enable DMA after at appropriate place 
     * DMA_SetPerMemAddr(......);
     * DMA_EnableChannel(DMA_CH1, ENABLE);
     * DMA_EnableChannel(DMA_CH2, ENABLE);
     * DMA_EnableChannel(DMA_CH3, ENABLE);
     * DMA_EnableChannel(DMA_CH4, ENABLE);
     * */
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the GPIO
 *@param null
 *@return status
 */
bool GPIO_Configuration(void)
{
    GPIO_InitType GPIO_InitStructure;
    EXTI_InitType EXTI_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    EXTI_InitStruct(&EXTI_InitStructure);
     
    /* Enable the GPIO clock*/
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOD | RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_AFIO, ENABLE);
    
     
    GPIO_SetBits(GPIOA,GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_8 | GPIO_PIN_15);
    GPIO_SetBits(GPIOB,GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_15);
    GPIO_SetBits(GPIOD,GPIO_PIN_14 | GPIO_PIN_15);
    GPIO_SetBits(GPIOC,GPIO_PIN_13);
     
    /*Initialize AF_PP GPIO */
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_NO_AF;
    GPIO_InitStructure.Pin            = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_SPI1;
    GPIO_InitStructure.Pin            = GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART2;
    GPIO_InitStructure.Pin            = GPIO_PIN_2;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART1;
    GPIO_InitStructure.Pin            = GPIO_PIN_9;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitStructure.Pin            = GPIO_PIN_0;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
    GPIO_InitStructure.Pin            = GPIO_PIN_8;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF0_USART3;
    GPIO_InitStructure.Pin            = GPIO_PIN_10;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    /*Initialize Out_PP GPIO */
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_NO_AF;
    GPIO_InitStructure.Pin            = GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_8 | GPIO_PIN_15;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.Pin            = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_15;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.Pin            = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitPeripheral(GPIOD,&GPIO_InitStructure);
     
    GPIO_InitStructure.Pin            = GPIO_PIN_13;
    GPIO_InitPeripheral(GPIOC,&GPIO_InitStructure);
     
    /*Initialize input GPIO */
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART2;
    GPIO_InitStructure.Pin            = GPIO_PIN_3;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_USART1;
    GPIO_InitStructure.Pin            = GPIO_PIN_10;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART4;
    GPIO_InitStructure.Pin            = GPIO_PIN_1;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF6_UART5;
    GPIO_InitStructure.Pin            = GPIO_PIN_9;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF5_USART3;
    GPIO_InitStructure.Pin            = GPIO_PIN_11;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    /*Initialize AF_OD GPIO */
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF1_I2C1;
    GPIO_InitStructure.Pin            = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF5_I2C2;
    GPIO_InitStructure.Pin            = GPIO_PIN_13 | GPIO_PIN_14;
    GPIO_InitPeripheral(GPIOB,&GPIO_InitStructure);
     
    /*config exti gpio */
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_High;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_2mA;
    GPIO_InitStructure.GPIO_Alternate = GPIO_NO_AF;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Down;
    GPIO_InitStructure.Pin            = GPIO_PIN_1;
    GPIO_InitPeripheral(GPIOA,&GPIO_InitStructure);
     
     
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE1;
    EXTI_InitPeripheral(&EXTI_InitStructure);
    /*Initialize  GPIO EXTI function PA1.EXTI1*/
    GPIO_ConfigEXTILine(GPIOA_PORT_SOURCE, GPIO_PIN_SOURCE1);
     
    /*config the EXTI17 ,EXTI17 connect to USBFS weakup*/
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE17;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
     
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the USART
 *@param null
 *@return status
 */
bool USART_Configuration(void)
{
    USART_InitType USART_InitStructure;
    USART_StructInit(&USART_InitStructure);
    /* Enable UART4|UART5|USART1 clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4|RCC_APB2_PERIPH_UART5|RCC_APB2_PERIPH_USART1,ENABLE);
    /* Enable USART2|USART3 clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USART2|RCC_APB1_PERIPH_USART3,ENABLE);
     
     
    /*********initialize the USART1************/
    USART_InitStructure.BaudRate            = 115200;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO; 
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE; 
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX; 
    /* Configure USART1 */
    USART_Init(USART1, &USART_InitStructure);
     
     
    /* Enable USART1 IDLEF interrupt*/
    USART_ConfigInt(USART1,USART_INT_IDLEF,ENABLE);
    /* Enable USART1RX DMA */
    USART_EnableDMA(USART1,USART_DMAREQ_RX, ENABLE);
    /* Enable the USART1 */
    USART_Enable(USART1, ENABLE);
     
    /*********initialize the USART2************/
    /* Configure USART2 */
    USART_Init(USART2, &USART_InitStructure);
     
     
    /* Enable USART2 IDLEF interrupt*/
    USART_ConfigInt(USART2,USART_INT_IDLEF,ENABLE);
     
    /* Enable USART2 ERRF interrupt*/
    USART_ConfigInt(USART2,USART_INT_ERRF,ENABLE);
    /* Enable USART2RX DMA */
    USART_EnableDMA(USART2,USART_DMAREQ_RX, ENABLE);
    /* Enable the USART2 */
    USART_Enable(USART2, ENABLE);
     
    /*********initialize the USART3************/
    /* Configure USART3 */
    USART_Init(USART3, &USART_InitStructure);
     
    /* Enable USART3RX DMA */
    USART_EnableDMA(USART3,USART_DMAREQ_RX, ENABLE);
    /* Enable the USART3 */
    USART_Enable(USART3, ENABLE);
     
    /*********initialize the UART4************/
    /* Configure UART4 */
    USART_Init(UART4, &USART_InitStructure);
     
     
    /* UART4 is reserved for blocking debug log output on PB0/PB1. */
    USART_ConfigInt(UART4,USART_INT_IDLEF,DISABLE);
    /* Enable the UART4 */
    USART_Enable(UART4, ENABLE);
     
    /*********initialize the UART5************/
    /* Configure UART5 */
    USART_Init(UART5, &USART_InitStructure);
     
    /* Enable UART5RX DMA */
    USART_EnableDMA(UART5,USART_DMAREQ_RX, ENABLE);
    /* Enable the UART5 */
    USART_Enable(UART5, ENABLE);
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the LPTIM
 *@param null
 *@return status
 */
bool LPTIM_Configuration(void)
{
    
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the RTC
 *@param null
 *@return status
 */
bool RTC_Configuration(void)
{
    RTC_InitType  RTC_InitStructure;
    /* Enable the RTC clock */
    
    
    RCC_EnableRtcClk(ENABLE);
    RTC_WaitForSynchro();
    /* Config RTC Peripheral */
    RTC_StructInit(&RTC_InitStructure);
    /* Configure the RTC data register and RTC prescaler */
    RTC_InitStructure.RTC_AsynchPrediv = 127;
    RTC_InitStructure.RTC_SynchPrediv  = 255;
    RTC_InitStructure.RTC_HourFormat   = RTC_24HOUR_FORMAT;
    if(RTC_Init(&RTC_InitStructure) == ERROR)
    {
        return false;
    }
     
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the IWDG
 *@param null
 *@return status
 */
bool IWDG_Configuration(void)
{
    IWDG_WriteConfig(IWDG_WRITE_ENABLE);
    IWDG_SetPrescalerDiv(IWDG_PRESCALER_DIV256);
    IWDG_CntReload(0x0fff);
    while (IWDG_GetStatus(IWDG_PVU_FLAG) == SET || IWDG_GetStatus(IWDG_CRVU_FLAG) == SET) {
    }
    IWDG_ReloadKey();
    IWDG_WriteConfig(IWDG_WRITE_DISABLE);
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the ADC
 *@param null
 *@return status
 */
bool ADC_Configuration(void)
{
     
    /* EnableADCclock */
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC,ENABLE);
    /***************** ADC configuration ***************/
    
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the USB
 *@param null
 *@return status
 */
bool USBFS_Configuration(void)
{
    /* Enable the USB clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USB, ENABLE);
     
    /***************** USBFS configuration ***************/
    USB_Init();
    //Use reference:
    //USBdevice_Tramsmit(USBdeviceType USBdevice,uint8_t * TxBuff, uint16_t Length,uint8_t buffNum);to tramsmit data
    //USBdevice_PrepareReceive(USBdeviceType USBdevice,uint8_t * RxBuff, uint16_t Length);to receive data and Set the length and buffer you want to receive
    //Mass Storage Device: need perfect the read and write function in usbfsd_storage_flash_driver.c file
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the SPI
 *@param null
 *@return status
 */
bool SPI_Configuration(void)
{
    SPI_InitType SPI_InitStructure;
    SPI_InitStruct(&SPI_InitStructure);
    /* Enable SPI1 clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_SPI1,ENABLE);
    /***************** SPI1 configuration ***************/
    SPI_InitStructure.DataDirection = SPI_DIR_DOUBLELINE_FULLDUPLEX;
    SPI_InitStructure.SpiMode       = SPI_MODE_MASTER;
    SPI_InitStructure.DataLen       = SPI_DATA_SIZE_8BITS;
    SPI_InitStructure.CLKPOL        = SPI_CLKPOL_LOW;
    SPI_InitStructure.CLKPHA        = SPI_CLKPHA_FIRST_EDGE;
    SPI_InitStructure.NSS           = SPI_NSS_SOFT;
    SPI_InitStructure.BaudRatePres  = SPI_BR_PRESCALER_256;
    SPI_InitStructure.FirstBit      = SPI_FB_MSB;
    SPI_InitStructure.CRCPoly       = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
     
    /* config DISABLE CRC*/
    SPI_EnableCalculateCrc( SPI1,DISABLE);
     
     
    /* Enable SPI1*/
    SPI_Enable(SPI1,ENABLE);
/* NTFx CODE END */
    return true;
}
/* NTFx CODE START */
/**
 *@brief Initializes the I2C
 *@param null
 *@return status
 */
bool I2C_Configuration(void)
{
    I2C_InitType I2C_InitStructure;
    I2C_InitStruct(&I2C_InitStructure);
     
    /* Enable I2C1|I2C2 clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1|RCC_APB1_PERIPH_I2C2,ENABLE);
    
     
    /*********************** I2C1 configuration *********************/
     
    I2C_DeInit(I2C1);
    /*init I2C1 */
    I2C_InitStructure.BusMode     = I2C_BUSMODE_I2C;
    I2C_InitStructure.OwnAddr1     = 0x0;
    I2C_InitStructure.AckEnable   = I2C_ACKEN;
    I2C_InitStructure.AddrMode    = I2C_ADDR_MODE_7BIT;
    I2C_InitStructure.ClkSpeed    = 100000;
    I2C_Init(I2C1,&I2C_InitStructure);
     
    /*Enables  the specified I2C Clock stretching*/
    I2C_EnableExtendClk(I2C1,ENABLE);
     
    I2C_Enable(I2C1,ENABLE);
     
    /*********************** I2C2 configuration *********************/
     
    I2C_DeInit(I2C2);
    /*init I2C2 */
    I2C_Init(I2C2,&I2C_InitStructure);
     
    /*Enables  the specified I2C Clock stretching*/
    I2C_EnableExtendClk(I2C2,ENABLE);
     
    I2C_Enable(I2C2,ENABLE);
    //Use reference:
    //I2C_GenerateStart(I2C1, ENABLE);
    //I2C_SendAddr7bit(I2C1, I2C_SLAVE_ADDR, I2C_DIRECTION_SEND); 
    //I2C_SendData(I2C1, *sendBufferPtr++);
    //I2C_GenerateStop(I2C1, ENABLE);
     
/* NTFx CODE END */
    return true;
}
