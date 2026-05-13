/**
 * @file xy_net.c
 * @brief XinYi Network Component Implementation
 */

#include "xy_net.h"
#include "xy_log.h"

int xy_net_init(void)
{
    XY_LOG_I("Network component initialized\n");
    return 0;
}

int xy_net_deinit(void)
{
    XY_LOG_I("Network component deinitialized\n");
    return 0;
}