#include <stdint.h>

#define USE_FIXED_TICK_PER_SEC
#ifdef USE_FIXED_TICK_PER_SEC
#define TICK_P1S 1000
#endif

enum PULSE_MODE
{
    OFF = 0,
    ON = 1,
	PWM_MODE=2,
	BREATH_MODE=3,
    USER_MODE = 4,
    PULSE_MODE_MAX
};

typedef union
{
    union
    {
        struct
        {
            uint32_t mode : 2;
            uint32_t repeat : 8;
            uint32_t value : 16;
        } basic_mode;

        struct
        {
            uint32_t mode : 8;
            uint32_t repeat : 8;
            uint32_t value : 16;
        } pulse_mode;
    } setting;
    union
    {
        uint32_t mode;
    } common;
    uint32_t clean_all;

} PULSE_CFG;

typedef struct IO_PULSE
{
	int (*pin_set_func)(uint8_t);
	PULSE_CFG cfg;
}pin_pulse_t;

void pin_pulse_process(pin_pulse_t *pulses, unsigned pulse_num);

// add SOS Signal
// https://packages.rt-thread.org/detail.html?package=quick_led
