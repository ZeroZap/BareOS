#include "io_pulse.h"


void pin_pulse_process(pin_pulse_t  *pins, unsigned char pulse_num)
{
//     uint8_t i = 0;
//     for(i=0; i<pulse_num; i++)
//     {
//         if(NULL != pins->pin_set_func)
//         {
//             switch (pins->cfg.setting.mode)
//             {
//             case OFF:
//                 pins->pin_set_func(0);
//                 break;
//             case ON:
//                 pins->pin_set_func(1);
//                 break;
//             case PWM_MODE:
//             case BREATH_MODE:
//             case USER_MODE:
//             default:
//                 break;
//             }
//         }

//     }
// }



void pp_pwm_process(pin_pulse_t pins)
{
    // static pwm_cnt = 100;
}
