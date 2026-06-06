# AT 组件裸机集成指南

`components/at` 是一个前台轮询式 AT 命令客户端，适合 BareOS 当前的无 RTOS 架构。ISR 或 DMA 回调只负责把 UART RX 字节写入环形缓冲区，主循环周期性调用 `at_obj_process()` 推进命令、响应和 URC。

## 是否会每条命令 malloc/free

当前版本默认不会为每个命令 work item 反复调用 `xy_malloc()` / `xy_free()`。

- `AT_WORK_STATIC_POOL_EN=1` 默认启用。
- `work_item_t` 从静态固定块池分配。
- 命令完成后只归还静态块，不释放堆。
- `at_exec_cmd()` 和 `env->println()` 的格式化临时缓冲区使用栈上 `char[AT_MAX_CMD_LEN]`。

仍然会在 `at_obj_create()` 初始化阶段分配对象级缓冲区：

- `at_info_t`
- `recvbuf`
- `urcbuf`，如果启用 URC 且 `urc_bufsize != 0`

这些对象级分配只发生在创建/销毁 AT 对象时，不发生在每条命令执行时。

如果定义 `AT_WORK_STATIC_POOL_EN=0`，work item 会回退到原来的 `at_core_malloc()` / `at_core_free()` 行为。

## 主循环接入

每个物理 AT 串口创建一个 `at_obj_t`：

```c
static unsigned int modem_write(const void *buf, unsigned int len)
{
    return bsp_uart1_write_nonblock(buf, len);
}

static unsigned int modem_read(void *buf, unsigned int len)
{
    return bsp_uart1_read_ring(buf, len);
}

static at_adapter_t s_at_4g_adap = {
    .lock = NULL,
    .unlock = NULL,
    .write = modem_write,
    .read = modem_read,
    .error = NULL,
    .debug = NULL,
#if AT_URC_WARCH_EN
    .urc_bufsize = 256,
#endif
    .recv_bufsize = 256,
};

at_obj_t *g_at_4g = at_obj_create(&s_at_4g_adap);
```

主循环中固定轮询：

```c
while (1) {
    at_obj_process(g_at_4g);
    at_obj_process(g_at_gnss);
    at_obj_process(g_at_sat);
    PT_SCHEDULE(task_comm(&s_pt_comm));
    PT_SCHEDULE(task_distress(&s_pt_distress));
}
```

## UART 适配要求

`at_adapter_t.read()` 和 `at_adapter_t.write()` 必须是非阻塞接口。

- `read()` 从 UART RX ring buffer 读取当前已有数据；无数据返回 0。
- `write()` 建议写入 UART TX ring buffer 或 DMA 队列后立即返回。
- 不要在 `read()` 或 `write()` 内等待 AT 响应。
- 不要在 ISR 内调用 `at_obj_process()`。

`at_obj_process()` 每次最多读取 64 字节。如果模块可能连续吐出大 URC 或二进制 payload，需要保证主循环调用频率和 UART RX ring buffer 大小足够。

## 配置项

所有配置都可以在编译参数或项目配置头中覆盖。

```c
#define AT_MAX_CMD_LEN           256u
#define AT_LIST_WORK_COUNT       8u
#define AT_WORK_STATIC_POOL_EN   1u
#define AT_WORK_POOL_COUNT       AT_LIST_WORK_COUNT
#define AT_WORK_ITEM_DATA_SIZE   AT_MAX_CMD_LEN
#define AT_MEM_WATCH_EN          1u
#define AT_MEM_LIMIT_SIZE        (3u * 1024u)
```

建议：

- `AT_LIST_WORK_COUNT`：PLB 场景建议 4 到 8。不要使用过大的默认队列掩盖上层状态机问题。
- `AT_WORK_POOL_COUNT`：静态 work item 全局池块数。多 AT 对象可能同时入队时，应按全系统并发 work 数配置。
- `AT_MAX_CMD_LEN`：普通 modem 控制命令 128 通常足够；有长 MQTT topic、URL、证书片段时保留 256 或更大。
- `AT_WORK_ITEM_DATA_SIZE`：限制 `at_send_data()` 可复制进 work item 的最大 payload。静态池启用时，超过该值会入队失败。
- `recv_bufsize`：按最长命令响应配置，例如 128/256/512。
- `urc_bufsize`：按最大 URC 帧配置。socket/MQTT payload URC 必须留足 header + payload + trailing CRLF。

静态 work item RAM 估算：

```text
AT_WORK_POOL_COUNT * (sizeof(work_item_t) + AT_WORK_ITEM_DATA_SIZE)
```

对象级 RAM 估算：

```text
sizeof(at_info_t) + recv_bufsize + urc_bufsize
```

如果创建 4G、GNSS、卫星三个 AT 对象，需要分别计算对象级 buffer；静态 work item 池当前是 AT 组件全局共享池。

## 命令写法

简单命令：

```c
at_exec_cmd(g_at_4g, NULL, "AT+CSQ");
```

需要结果回调：

```c
static void on_csq(at_response_t *r)
{
    if (r->code == AT_RESP_OK) {
        /* parse r->recvbuf */
    }
}

at_attr_t attr;
at_attr_deinit(&attr);
attr.cb = on_csq;
attr.timeout = 3000;
at_exec_cmd(g_at_4g, &attr, "AT+CSQ");
```

多步骤命令建议写成 `at_do_work()` 自定义状态机，避免在上层阻塞等待：

```c
static int work_modem_init(at_env_t *env)
{
    switch (env->state) {
    case 0:
        env->println(env, "ATE0");
        env->reset_timer(env);
        env->state = 1;
        break;
    case 1:
        if (env->contains(env, "OK")) {
            env->finish(env, AT_RESP_OK);
        } else if (env->is_timeout(env, 3000)) {
            env->finish(env, AT_RESP_TIMEOUT);
        }
        break;
    }
    return 0;
}
```

## URC 使用

URC 表应为静态常驻对象：

```c
static const urc_item_t s_urc_tbl[] = {
    { "+QIURC: \"recv\"", '\n', on_recv },
    { "+QIURC: \"closed\"", '\n', on_closed },
};

at_obj_set_urc(g_at_4g, s_urc_tbl, sizeof(s_urc_tbl) / sizeof(s_urc_tbl[0]));
```

二进制 payload URC 建议使用 `at_urc_recv_split()`，避免把 header 和 payload 分裂处理的逻辑散落到各驱动中。

## 不适合的用法

- 在 ISR 中调用 `at_exec_cmd()` 或 `at_obj_process()`。
- 在 `read()` 内忙等串口数据。
- 在 `write()` 内等待低速 UART 完整发完大 payload。
- 用 `at_exec_cmd()` 高频发送日志型数据。
- 让多个模块共享同一个 AT 对象。

## 上线检查清单

- `g_sys_tick_ms` 已由 SysTick 1 ms 递增。
- UART RX ring buffer 在最大 URC burst 下不会溢出。
- `write()` 能接受 AT 命令最大长度，或上层不发送超过 TX ring 可容纳的数据。
- `AT_LIST_WORK_COUNT` 与单个 AT 对象并发入队数量一致。
- `AT_WORK_POOL_COUNT` 与全系统 AT 并发 work 数一致。
- `AT_WORK_ITEM_DATA_SIZE` 覆盖最大 `at_send_data()` payload。
- `recv_bufsize` 覆盖最长命令响应。
- `urc_bufsize` 覆盖最长 URC 帧。
- 低功耗 STOP 前确认 `at_obj_busy()` 为 false，或确认 UART/定时器可唤醒继续处理。
