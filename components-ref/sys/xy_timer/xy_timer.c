#include "xy_timer.h"
#include "xy_tick.h"

volatile xy_uint32_t g_xy_tick  = 0;
volatile xy_uint32_t g_tick_pre = 0;

typedef struct _xy_timer {
    xy_uint32_t cnt;
    xy_uint32_t reload; // when cnt = 0; reload=reload
    struct _xy_timer *pre;
    struct _xy_timer *next;
    timer_proc func;
    void *parameter;
    xy_uint8_t flag; // 0=空闲, 1=正在回调中执行, 2=待删除(回调结束后由ticks释放)
} xy_timer_t;

// 即将到时的计数器
volatile struct _xy_timer *g_xy_timer = NULL;

// 标记是否有待释放的定时器（避免在回调中频繁释放）
static volatile xy_uint8_t g_timer_pending_kill = 0;

void xy_timer_init(void)
{
    g_xy_timer = NULL;
    g_timer_pending_kill = 0;
}

xy_uint32_t xy_timer_get_tick(void)
{
    xy_uint32_t ticks;
    xy_enter_critical();
    ticks = g_xy_tick;
    xy_exit_critical();
    return ticks;
}

xy_uint32_t xy_timer_get_tick_from_isr(void)
{
    xy_uint32_t ticks;
    ticks = g_xy_tick;
    return ticks;
}

void xy_timer_set_tick(xy_uint32_t tick)
{
    xy_enter_critical();
    g_xy_tick  = tick;
    g_tick_pre = tick;
    xy_exit_critical();
}

xy_uint16_t xy_timer_get_nexttick(void)
{
    if (g_xy_timer)
        return g_xy_timer->cnt;
    return 0;
}

static void xy_timer_insert(xy_timer_t *timer)
{
    struct _xy_timer *p;
    timer->pre = NULL;
    for (p = g_xy_timer; p; p = p->next) {
        if (timer->cnt < p->cnt) {
            p->cnt -= timer->cnt;
            break;
        }
        timer->cnt -= p->cnt;
        timer->pre = p;
    }

    timer->next = p;
    if (timer->next) {
        timer->next->pre = timer;
    }

    if (timer->pre) {
        timer->pre->next = timer;
    } else {
        g_xy_timer = timer;
    }
}

static void xy_timer_remove(xy_timer_t *timer)
{
    // 前节点
    if (timer->pre) {
        timer->pre->next = timer->next;
    } else {
        g_xy_timer = timer->next;
    }

    // 后节点
    if (timer->next) {
        timer->next->pre = timer->pre;
        timer->next->cnt += timer->cnt;
    }
}


xy_timer_ref xy_timer_create(xy_uint32_t cnt, xy_uint32_t reload,
                             timer_proc pfunc, void *params)
{
    xy_timer_t *p;

    // 参数校验
    if (pfunc == NULL) {
        return NULL;
    }

    p = (xy_timer_t *)xy_mem_malloc(sizeof(xy_timer_t));

    if (p) {
        p->cnt       = cnt + xy_timer_get_tick() - g_tick_pre;
        p->reload    = reload;
        p->func      = pfunc;
        p->parameter = params;
        p->flag      = 0;
        xy_timer_insert(p);
    }
    return p;
}

void xy_timer_ticks(void)
{
    xy_timer_t *p;
    xy_uint32_t ticks;
    xy_uint16_t i;

    ticks      = xy_timer_get_tick();
    i          = ticks - g_tick_pre;
    g_tick_pre = ticks;

    // 如果有timer且时间进行着
    while (g_xy_timer && i) {
        p = g_xy_timer;
        // 时间未到，则减掉i
        if (i < p->cnt) {
            p->cnt -= i;
            i = 0;
        } else {
            // 时间已到，i减去最近一个到来的计数
            i -= p->cnt;
            p->cnt = 0;

            if (p->func) {
                p->flag = 1;
#if (PLATFORM == PLATFORM_C51)
                (*p->func)(p, (void *)p->parameter);
#else
                (*p->func)(p, (void *)p->parameter);
#endif

                // 检测是否被标记为待删除（回调中调用了xy_timer_kill）
                if (p->reload == 0 && p->flag == 1) {
                    p->flag = 2;  // 标记为待释放状态
                    g_timer_pending_kill = 1;
                } else {
                    p->flag = 0;
                }
            } else {
                p->flag = 0;
            }

            // 如果有重载，则不会kill掉
            if ((p->cnt = p->reload) == 0) {
                // 状态为2表示在回调中被标记待删除，此处真正删除
                if (p->flag == 2) {
                    xy_timer_remove(p);
                    xy_mem_free(p);
                }
                // 否则保持现状（单次定时器自然结束）
            } else {
                // 获取下一个时钟
                g_xy_timer = g_xy_timer->next;
                // 如果下一个时钟为空，则 pre也要NULL了
                if (g_xy_timer) {
                    g_xy_timer->pre = NULL;
                }
                // 重新插入时钟
                xy_timer_insert(p);
            }
        }
    }

    // 处理回调中产生的待释放定时器（遍历链表清理flag=2的节点）
    if (g_timer_pending_kill) {
        g_timer_pending_kill = 0;
        // 重新遍历，处理可能积压的待删除定时器
        p = g_xy_timer;
        while (p) {
            if (p->flag == 2 && p->reload == 0) {
                xy_timer_remove(p);
                xy_mem_free(p);
            }
            p = p->next;
        }
    }
}

void xy_timer_kill(xy_timer_ref timer_handler)
{
    xy_timer_t *p;

    // 空指针检查
    if (timer_handler == NULL) {
        return;
    }

    p = (xy_timer_t *)timer_handler;

    // 正在回调中执行，不允许同步删除
    if (p->flag == 1) {
        // 标记为待删除，回调结束后再删除
        p->reload = 0;
        p->flag = 2;
        g_timer_pending_kill = 1;
        return;
    }

    // 已标记待删除或已空闲，无需重复处理
    if (p->flag == 2) {
        return;
    }

    xy_timer_remove(p);
    p->reload = 0;
    xy_mem_free(p);
}

void xy_timer_change_cnt(xy_timer_ref timer_handler, xy_uint32_t cnt)
{
    xy_timer_t *p;

    // 空指针检查
    if (timer_handler == NULL) {
        return;
    }

    p = (xy_timer_t *)timer_handler;

    // 正在回调中执行，不允许修改
    if (p->flag == 1) {
        return;
    }

    xy_enter_critical();
    xy_timer_remove(p);
    p->cnt = cnt + xy_timer_get_tick() - g_tick_pre;
    xy_timer_insert(p);
    xy_exit_critical();
}

void xy_timer_change_reload(xy_timer_ref timer_handler, xy_uint32_t reload)
{
    xy_timer_t *p;

    // 空指针检查
    if (timer_handler == NULL) {
        return;
    }

    p = (xy_timer_t *)timer_handler;
    xy_enter_critical();
    p->reload = reload;
    xy_exit_critical();
}

void xy_timer_change_func(xy_timer_ref timer_handler, timer_proc pfunc)
{
    xy_timer_t *p;

    // 空指针检查
    if (timer_handler == NULL || pfunc == NULL) {
        return;
    }

    p = (xy_timer_t *)timer_handler;
    xy_enter_critical();
    p->func = pfunc;
    xy_exit_critical();
}

timer_proc xy_timer_get_func(xy_timer_ref timer_handler)
{
    if (!timer_handler)
        return NULL;
    return ((xy_timer_t *)timer_handler)->func;
}
