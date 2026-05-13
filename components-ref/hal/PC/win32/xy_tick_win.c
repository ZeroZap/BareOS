#include "xy_timer_win.h"
#include <windows.h>


static void CALLBACK TimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser,
                                   DWORD_PTR dw1, DWORD_PTR dw2)
{
    (*(uint32_t *)dwUser)++;
}

int32_t xy_tick_win_init(uint32_t *tick_counter)
{
    if (!tick_counter)
        return -1;

    // 设置最小定时器分辨率
    timeBeginPeriod(1);

    // 创建1ms周期定时器
    g_timer_handle =
        (HANDLE)timeSetEvent(1,                       // 1ms周期
                             0,                       // 最高精度
                             TimerCallback,           // 回调函数
                             (DWORD_PTR)tick_counter, // 传递计数值指针
                             TIME_PERIODIC            // 周期模式
        );

    return g_timer_handle ? 0 : -2;
}
