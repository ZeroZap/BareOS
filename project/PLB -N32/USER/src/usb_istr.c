
/**
 * @file usb_istr.c
 * @author N32cube
 */
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"

__IO uint16_t wIstr;            /* STS register last read value */
__IO uint8_t bIntPackSOF   = 0; /* SOFs received between 2 consecutive packets */
__IO uint32_t esof_counter = 0; /* expected SOF counter */
__IO uint32_t wCNTR        = 0;

/* function pointers to non-control endpoints service routines */
void (*pEpInt_IN[7])(void) = {
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
};

void (*pEpInt_OUT[7])(void) = {
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
};

/*******************************************************************************
 * Function Name  : USB_Istr
 * Description    : STS events interrupt service routine
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void USB_Istr(void)
{
    __IO uint32_t EP[8];

    wIstr = _GetISTR();

#if (IMR_MSK & STS_CTRS)
    if (wIstr & STS_CTRS & wInterrupt_Mask)
    {
        /* servicing of the endpoint correct transfer interrupt */
        /* clear of the CTR flag into the sub */
        USB_CorrectTransferLp();
#ifdef CTR_CALLBACK
        CTR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_RST)
    if (wIstr & STS_RST & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_RST);
        Device_Property.Init();
        Device_Property.Reset();
#ifdef RESET_CALLBACK
        RESET_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_DOVR)
    if (wIstr & STS_DOVR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_DOVR);
#ifdef DOVR_CALLBACK
        DOVR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_ERROR)
    if (wIstr & STS_ERROR & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_ERROR);
#ifdef ERR_CALLBACK
        ERR_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_WKUP)
    if (wIstr & STS_WKUP & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_WKUP);
        Resume(RESUME_EXTERNAL);
#ifdef WKUP_CALLBACK
        WKUP_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_SUSPD)
    if (wIstr & STS_SUSPD & wInterrupt_Mask)
    {
        /* check if SUSPEND is possible */
        if (fSuspendEnabled)
        {
            Suspend();
        }
        else
        {
            /* if not possible then resume after xx ms */
            Resume(RESUME_LATER);
        }
        /* clear of the STS bit must be done after setting of CTRL_FSUSPD */
        _SetISTR((uint16_t)CLR_SUSPD);
#ifdef SUSP_CALLBACK
        SUSP_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_SOF)
    if (wIstr & STS_SOF & wInterrupt_Mask)
    {
        _SetISTR((uint16_t)CLR_SOF);
        bIntPackSOF++;

#ifdef SOF_CALLBACK
        SOF_Callback();
#endif
    }
#endif
    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & STS_ESOF)
    uint32_t i=0;
    if (wIstr & STS_ESOF & wInterrupt_Mask)
    {
        /* clear ESOF flag in STS */
        _SetISTR((uint16_t)CLR_ESOF);

        if ((_GetFNR() & FN_RXDP) != 0)
        {
            /* increment ESOF counter */
            esof_counter++;

            /* test if we enter in ESOF more than 3 times with FSUSP =0 and RXDP =1=>> possible missing SUSP flag*/
            if ((esof_counter > 3) && ((_GetCNTR() & CTRL_FSUSPD) == 0))
            {
                /* this a sequence to apply a force RESET*/

                /*Store CTRL value */
                wCNTR = _GetCNTR();

                /*Store endpoints registers status */
                for (i = 0; i < 8; i++)
                    EP[i] = _GetENDPOINT(i);

                /*apply FRES */
                wCNTR |= CTRL_FRST;
                _SetCNTR(wCNTR);

                /*clear FRES*/
                wCNTR &= ~CTRL_FRST;
                _SetCNTR(wCNTR);

                /*poll for RESET flag in STS*/
                while ((_GetISTR() & STS_RST) == 0)
                    ;

                /* clear RESET flag in STS */
                _SetISTR((uint16_t)CLR_RST);

                /*restore Enpoints*/
                for (i = 0; i < 8; i++)
                    _SetENDPOINT(i, EP[i]);

                esof_counter = 0;
            }
        }
        else
        {
            esof_counter = 0;
        }

        /* resume handling timing is made with ESOFs */
        Resume(RESUME_ESOF); /* request without change of the machine state */

#ifdef ESOF_CALLBACK
        ESOF_Callback();
#endif
    }
#endif
} /* USB_Istr */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
