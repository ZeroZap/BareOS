#include "sf.h"

#define LED P1

// // all basic pwm
// #define STATUS_0FF 0
// #define STATUS_ON 1
// #define STATUS_ON_HALF 2
// #define STATUS_BREATH 2
// #define STATUS_BLINK 3

#define TICK_500MS
#define TICK_400MS
#define TICK_300MS
#define TICK_200MS
#define TICK_20MS

enum LED_CHANNEL {
    LED_0 = 0x00,
};

enum EFFECT{
    STATUS_0FF = 0x00,
    STATUS_ON,
    STATUS_ON_HALF,
    STATUS_ON_QUARTER,
    STATUS_ON_THREE_QUARTER,
    STATUS_BREATH,
    STATUS_BREATH_500MS,
    STATUS_BLINK,
    STATUS_BLINK_100MS,
    STATUS_MAX
};

#define LED_MAX 10

/**note: sf_pwm_size_t only can be uint8_t or uint16_t*/
typedef sf_uint8_t pwm_size_t;
#define PWM_MAX (sizeof(pwm_size_t)<<3)

struct led{
    pwm_size_t pwm;
    pwm_size_t duty;
    sf_uint8_t step;
    // sf_uint8_t channel;
};

struct led g_led[LED_MAX];

void led_set_status(sf_uint8_t channel, sf_uint8_t status)
{
    case STATUS_0FF:
        g_led[channel].pwm=0;
        g_led[channel].duty=0;
        g_led[channel].step=0;
        break;
    case STATUS_ON:
        g_led[channel].pwm = PWM_MAX;
        g_led[channel].duty = PWM_MAX;
        g_led[channel].step = 0;
        break;
    case STATUS_ON_HALF:
        g_led[channel].pwm = (PWM_MAX >> 1);
        g_led[channel].duty = PWM_MAX;
        g_led[channel].step = 0;
        break;
    case STATUS_ON_THREE_QUARTER:
        g_led[channel].pwm = ((PWM_MAX >> 2) + (PWM_MAX >> 1));
        g_led[channel].duty = ((PWM_MAX >> 2) + (PWM_MAX >> 1));
        g_led[channel].step = 0;
        break;
    case STATUS_ON_QUARTER:
        g_led[channel].pwm = (PWM_MAX >> 2);
        g_led[channel].duty = (PWM_MAX >> 2);
        g_led[channel].step = 0;
        break;
    case STATUS_BREATH:
        g_led[channel].step = 1;
        g_led[channel].pwm  = 0;
        break;
    case STATUS_BLINK:
        led_pwm_set(channel, PWM_MAX);
        break;
    default:
        break;
}

void led_set_pwm(sf_uint8_t channel, pwm_size_t pwm)
{
    return g_led[channel].pwm = pwm;
}

pwm_size_t led_pwm_get(sf_uint8_t channel)
{
    return g_led[channel].pwm;
}


void led_task(void)
{
    pwm_size_t pwm=0;
    sf_uint8_t i;
    for (i=0; i<LED_MAX; i++) {
        if(g_led[i].duty == 0) {
            led_set_status(i, 0);
        } else if(g_led[i].pwm <= g_led[i].duty) {
            led_set_status(i, 1);
        }else {
            led_set_status(i, 0);
        }
        g_led[i].pwm += g_led[i].step;
    }
    pwm++;
}

void led_set_effect(sf_uint8_t channel, sf_uint8_t effect)
{
    switch (effect)
    {
    case STATUS_0FF:
        g_led[channel].pwm=0;
        g_led[channel].duty=0;
        g_led[channel].step=0;
        break;
    case STATUS_ON:
        g_led[channel].pwm = PWM_MAX;
        g_led[channel].duty = PWM_MAX;
        g_led[channel].step = 0;
        break;
    case STATUS_ON_HALF:
        g_led[channel].pwm = (PWM_MAX >> 1);
        g_led[channel].duty = PWM_MAX;
        g_led[channel].step = 0;
        break;
    case STATUS_ON_THREE_QUARTER:
        g_led[channel].pwm = ((PWM_MAX >> 2) + (PWM_MAX >> 1));
        g_led[channel].duty = ((PWM_MAX >> 2) + (PWM_MAX >> 1));
        g_led[channel].step = 0;
        break;
    case STATUS_ON_QUARTER:
        g_led[channel].pwm = (PWM_MAX >> 2);
        g_led[channel].duty = (PWM_MAX >> 2);
        g_led[channel].step = 0;
        break;
    case STATUS_BREATH:
        g_led[channel].step = 1;
        g_led[channel].pwm  = 0;
        break;
    case STATUS_BLINK:
        led_pwm_set(channel, PWM_MAX);
        break;
    default:
        break;
    }
}

// step is interval?? 20ms, 40ms, step default = 1;
void led_effect(sf_uint8_t channel, sf_uint8_t effect, sf_uint8_t step) {
    static pwm_size_t duty = PWM_MAX;
    switch (effect)
    {
    case STATUS_0FF:
        led_pwm_set(channel, 0);
        break;
    case STATUS_ON:
        led_pwm_set(channel, PWM_MAX);
        break;
    case STATUS_ON_HALF:
        led_pwm_set(channel, (PWM_MAX >> 1));
        break;
    case STATUS_ON_THREE_QUARTER:
        led_pwm_set(channel, ((PWM_MAX >> 2) + (PWM_MAX >> 1)));
        break;
    case STATUS_ON_QUARTER:
        led_pwm_set(channel, (PWM_MAX >> 2));
        break;
    case STATUS_BREATH:
        led_pwm_set(channel, (pwm_size_t)(PWM_MAX - duty));
        break;
    case STATUS_BLINK:
        led_pwm_set(channel, PWM_MAX);
        break;
    default:
        break;
    }
    duty -= step;
}


## effect 切换要有个exit处理