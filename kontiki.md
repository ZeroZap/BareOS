```
lwtcpip 详细指导，输出要生成各个学习 topic 开始讨论，记录每个 topic 的详细
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

这是一份关于 **lwIP (lightweight IP)** 的系统性学习指南，专为嵌入式系统工程师设计，结合你的 BMS 固件背景，以底层驱动视角切入。

------

# lwIP 系统学习指南

> **定位**：lwIP 是专为资源受限 MCU 设计的轻量级 TCP/IP 协议栈，广泛用于无 OS 或 RTOS 嵌入式系统。学习路径从底层网络接口驱动到上层应用 API 逐层展开。

------

## 📚 Topic 总览

|  #   |            Topic             |                核心内容                 |
| :--: | :--------------------------: | :-------------------------------------: |
|  1   |      **架构与移植基础**      |    lwIP 内部分层、内存模型、移植接口    |
|  2   |     **pbuf 缓冲区管理**      |       pbuf 链、零拷贝机制、内存池       |
|  3   |   **网络接口层（netif）**    |   netif 注册、以太网驱动对接、MAC/PHY   |
|  4   |          **IP 层**           |        IPv4 分片重组、路由、ICMP        |
|  5   |           **UDP**            |     无连接传输、PCB 管理、回调机制      |
|  6   |           **TCP**            |     连接状态机、滑动窗口、重传机制      |
|  7   |        **应用层 API**        | RAW API / Netconn API / Socket API 对比 |
|  8   |     **DHCP / DNS / ARP**     |           地址解析与动态配置            |
|  9   | **无 OS 驱动模式（NO_SYS）** |          轮询调度、定时器管理           |
|  10  |        **RTOS 集成**         |         线程模型、信号量、邮箱          |
|  11  |      **调试与性能调优**      |       统计、日志、lwipopts.h 裁剪       |

------

------

## Topic 1：架构与移植基础

### 1.1 lwIP 整体分层架构

```text
┌─────────────────────────────────────┐
│         Application Layer           │  ← HTTP / MQTT / CoAP
├─────────────────────────────────────┤
│     API Layer (RAW / Netconn / BSD) │  ← 用户选择的编程接口
├─────────────────────────────────────┤
│         TCP / UDP Layer             │  ← 传输层协议
├─────────────────────────────────────┤
│           IP / ICMP Layer           │  ← 网络层
├─────────────────────────────────────┤
│         ARP / Ethernet Layer        │  ← 链路层
├─────────────────────────────────────┤
│     netif (Network Interface)       │  ← 硬件抽象层（你来实现）
├─────────────────────────────────────┤
│       MAC / PHY Driver              │  ← 底层以太网/WiFi驱动
└─────────────────────────────────────┘
```

lwIP 的核心设计哲学：**协议栈本身不直接操作硬件**，所有硬件访问通过 `netif` 回调函数由用户实现。

------

### 1.2 关键源码目录结构

```text
lwip/
├── src/
│   ├── core/           ← 核心协议（IP、TCP、UDP、DHCP...）
│   │   ├── tcp.c
│   │   ├── udp.c
│   │   ├── ip4.c
│   │   ├── pbuf.c      ← 缓冲区管理（极重要）
│   │   └── netif.c
│   ├── netif/
│   │   ├── ethernet.c  ← 以太网帧处理
│   │   └── ppp/        ← PPP 支持（串口拨号）
│   ├── api/            ← Netconn / Socket API
│   └── include/
│       └── lwip/
│           ├── opt.h   ← 所有编译选项默认值
│           └── ...
└── contrib/
    └── ports/          ← 各平台移植参考
```

------

### 1.3 移植必须实现的接口

移植 lwIP 到你的 MCU，**最少需要提供以下 4 类接口**：

#### ① 内存分配接口（`mem_malloc` / `mem_free`）

lwIP 默认使用自己的内存堆管理器（`mem.c`），你也可以对接系统 malloc：

```c
// lwipopts.h 中配置
#define MEM_LIBC_MALLOC     0   // 0=使用lwIP内部堆, 1=使用系统malloc
#define MEM_SIZE            (8 * 1024)  // lwIP内部堆大小（字节）
```

> ⚠️ 对于无动态内存的 BMS 类项目，建议 `MEM_LIBC_MALLOC 0`，完全使用静态内存池，避免堆碎片。

#### ② 时间戳接口（`sys_now()`）

```c
// 返回系统运行毫秒数，lwIP 用于超时计算
u32_t sys_now(void) {
    return HAL_GetTick();  // STM32 示例
}
```

#### ③ 临界区保护（`SYS_ARCH_PROTECT`）

```c
// 无 OS 模式下，通常直接关中断
#define SYS_ARCH_PROTECT(lev)    lev = __get_PRIMASK(); __disable_irq()
#define SYS_ARCH_UNPROTECT(lev)  __set_PRIMASK(lev)
```

#### ④ 网络接口驱动（`netif_add` + 回调）

```c
// 注册网络接口（详见 Topic 3）
netif_add(&my_netif, &ip, &mask, &gw, NULL, ethernetif_init, ethernet_input);
```

------

### 1.4 `lwipopts.h` —— 移植的核心配置文件

这是你**唯一需要创建**的配置文件，覆盖 `opt.h` 中的默认值：

```c
// lwipopts.h 最小化模板

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* ===== 系统模式 ===== */
#define NO_SYS              1   // 1=无OS轮询模式, 0=RTOS模式

/* ===== 内存配置 ===== */
#define MEM_SIZE            (8 * 1024)   // 动态内存堆
#define MEMP_NUM_PBUF       16           // pbuf 池数量
#define MEMP_NUM_TCP_PCB    4            // 最大 TCP 连接数
#define MEMP_NUM_UDP_PCB    4            // 最大 UDP 连接数

/* ===== 协议开关 ===== */
#define LWIP_TCP            1
#define LWIP_UDP            1
#define LWIP_DHCP           1
#define LWIP_DNS            0   // 暂不需要可关闭

/* ===== 调试（开发阶段打开）===== */
#define LWIP_DEBUG          1
#define TCP_DEBUG           LWIP_DBG_ON
#define IP_DEBUG            LWIP_DBG_ON

/* ===== 性能调优 ===== */
#define TCP_MSS             1460
#define TCP_WND             (4 * TCP_MSS)
#define TCP_SND_BUF         (4 * TCP_MSS)

#endif
```

------

### 1.5 移植验证步骤（Checklist）

```text
[ ] 1. sys_now() 返回值单调递增，精度 ≤ 10ms
[ ] 2. 内存池大小足够（用 LWIP_STATS 观察 memp 使用峰值）
[ ] 3. netif 的 output / linkoutput 回调已正确实现
[ ] 4. 能 ping 通本机 IP（ICMP echo 正常）
[ ] 5. ARP 表能正确建立（用 Wireshark 抓包验证）
```

------

## Topic 2：pbuf 缓冲区管理

### 2.1 pbuf 是什么

`pbuf`（packet buffer）是 lwIP 中**所有数据传输的载体**，类似于 Linux 内核的 `sk_buff`。理解 pbuf 是读懂 lwIP 源码的前提。

```c
// lwip/pbuf.h
struct pbuf {
    struct pbuf *next;   // 链表指针（支持链式 pbuf）
    void        *payload;// 指向实际数据
    u16_t        tot_len;// 链中所有 pbuf 的总数据长度
    u16_t        len;    // 本节点数据长度
    u8_t         type_internal; // pbuf 类型
    u8_t         flags;
    LWIP_PBUF_REF_T ref; // 引用计数
};
```

------

### 2.2 pbuf 的四种类型

|    类型     |        内存来源        |              特点              |     适用场景     |
| :---------: | :--------------------: | :----------------------------: | :--------------: |
| `PBUF_RAM`  | 动态堆（`mem_malloc`） |         连续内存，灵活         |  应用层发送数据  |
| `PBUF_ROM`  |       用户静态区       |   零拷贝，payload 指向只读区   |   发送常量数据   |
| `PBUF_REF`  |        用户提供        | 零拷贝，payload 指向用户缓冲区 | 发送用户 buffer  |
| `PBUF_POOL` |       固定内存池       |    固定大小，分配快，无碎片    | 接收路径（推荐） |

```text
PBUF_POOL 内存布局（推荐用于 RX）：

┌──────────────────────────────────────┐
│  struct pbuf  │  header空间  │  data  │  ← 一块连续内存
└──────────────────────────────────────┘
  ↑ MEMP_NUM_PBUF 个这样的块预分配好
```

------

### 2.3 pbuf 链（Chain）

当数据超过单个 pbuf 容量时，lwIP 自动形成链：

```text
pbuf_chain:

[pbuf_A] → [pbuf_B] → [pbuf_C] → NULL
  len=100    len=100    len=56
  tot_len=256 tot_len=156 tot_len=56
```

> **关键规则**：`tot_len` 是从当前节点到链尾的总长度，只有链头的 `tot_len` 等于整个数据包大小。

------

### 2.4 常用 pbuf 操作 API

```c
// 分配 pbuf
struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);

// 释放（引用计数减1，为0时真正释放）
pbuf_free(p);

// 增加引用计数（持有 pbuf 时必须调用）
pbuf_ref(p);

// 拷贝数据到 pbuf
pbuf_take(p, src_data, len);

// 从 pbuf 读取数据
pbuf_copy_partial(p, dst_buf, len, offset);

// 连接两个 pbuf 链
pbuf_chain(head, tail);

// 调整 payload 指针（添加/移除协议头）
pbuf_add_header(p, header_size_to_add);   // 向前扩展（添加头部）
pbuf_remove_header(p, header_size_to_rm); // 向后收缩（移除头部）
```

------

### 2.5 零拷贝发送示例

```c
// 场景：发送一块已有的静态 buffer，不想拷贝
static const uint8_t my_data[64] = { /* ... */ };

// 使用 PBUF_REF，payload 直接指向 my_data，不发生内存拷贝
struct pbuf *p = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
p->payload = (void *)my_data;
p->len = p->tot_len = sizeof(my_data);

udp_sendto(pcb, p, &dest_ip, dest_port);
pbuf_free(p);  // 只释放 pbuf 结构体，不释放 my_data
```

> ⚠️ 使用 `PBUF_REF` 时，**必须保证在 pbuf 被发送完成之前，`my_data` 的内存有效**。在中断驱动的 DMA 发送场景中需特别注意。

------

### 2.6 pbuf 引用计数陷阱

```c
struct pbuf *p = pbuf_alloc(...);  // ref = 1

pbuf_ref(p);   // ref = 2（你额外持有一份）

pbuf_free(p);  // ref = 1（未真正释放）
pbuf_free(p);  // ref = 0（真正释放）

// ❌ 常见错误：忘记 pbuf_ref 就把 p 传给协议栈
// 协议栈内部会 pbuf_free(p)，导致你的指针悬空
```

------

## Topic 3：网络接口层（netif）

### 3.1 netif 的角色

`netif` 是 lwIP 与硬件之间的**唯一桥梁**。你需要实现两个核心回调：

|     回调     |         调用时机          |            你需要做什么            |
| :----------: | :-----------------------: | :--------------------------------: |
|  `init_fn`   | `netif_add()` 时调用一次  |   初始化 MAC/PHY，设置 MAC 地址    |
|   `output`   |    IP 层要发送数据包时    | 调用 ARP 解析，然后调 `linkoutput` |
| `linkoutput` |      ARP 解析完成后       |    把 pbuf 数据写入 MAC TX FIFO    |
|   `input`    | 你从 RX 中断/轮询收到帧时 | 调用 `ethernet_input()` 送入协议栈 |

------

### 3.2 netif 初始化模板

```c
#include "lwip/netif.h"
#include "netif/ethernet.h"

static struct netif my_netif;

err_t ethernetif_init(struct netif *netif) {
    /* 设置接口名（2字节缩写）*/
    netif->name[0] = 'e';
    netif->name[1] = '0';

    /* 设置 MAC 地址 */
    netif->hwaddr_len = ETH_HWADDR_LEN;  // 6
    netif->hwaddr[0] = 0x02;
    netif->hwaddr[1] = 0x00;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x00;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x01;

    /* 设置 MTU */
    netif->mtu = 1500;

    /* 设置能力标志 */
    netif->flags = NETIF_FLAG_BROADCAST |
                   NETIF_FLAG_ETHARP    |
                   NETIF_FLAG_LINK_UP;

    /* 注册发送回调 */
    netif->output     = etharp_output;   // lwIP 提供，处理 ARP
    netif->linkoutput = low_level_output; // 你实现

    /* 初始化 MAC/PHY 硬件 */
    low_level_init(netif);

    return ERR_OK;
}
```

------

### 3.3 发送路径实现（`linkoutput`）

```c
static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    /* 遍历 pbuf 链，将数据写入 DMA 描述符或 TX FIFO */
    for (struct pbuf *q = p; q != NULL; q = q->next) {
        // 将 q->payload 的 q->len 字节写入硬件
        ETH_WriteTxFIFO(q->payload, q->len);
    }
    ETH_TriggerTx();  // 触发发送
    return ERR_OK;
}
```

> 💡 **DMA 零拷贝优化**：如果 MAC 支持链式 DMA 描述符，可以直接将 pbuf 链的每个节点映射为一个 DMA 描述符，避免拷贝到连续 buffer。

------

### 3.4 接收路径实现

```c
// 在 RX 中断或轮询中调用
void ethernetif_input(struct netif *netif) {
    struct pbuf *p;
    u16_t len;

    /* 从硬件读取帧长度 */
    len = ETH_GetRxFrameSize();
    if (len == 0) return;

    /* 从 PBUF_POOL 分配（快速，无碎片）*/
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        ETH_DropRxFrame();  // 内存不足，丢帧
        return;
    }

    /* 将数据从 RX FIFO 拷贝到 pbuf */
    for (struct pbuf *q = p; q != NULL; q = q->next) {
        ETH_ReadRxFIFO(q->payload, q->len);
    }

    /* 送入 lwIP 协议栈（ethernet_input 会处理 ARP/IP 分发）*/
    if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);  // 协议栈拒绝，释放
    }
}
```

------

### 3.5 netif 注册与启动

```c
void network_init(void) {
    ip4_addr_t ip, mask, gw;

    IP4_ADDR(&ip,   192, 168, 1, 100);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    IP4_ADDR(&gw,   192, 168, 1, 1);

    /* 初始化 lwIP 核心 */
    lwip_init();

    /* 注册网络接口 */
    netif_add(&my_netif, &ip, &mask, &gw,
              NULL,              // 用户数据指针
              ethernetif_init,   // init 回调
              ethernet_input);   // input 处理函数

    /* 设为默认接口 */
    netif_set_default(&my_netif);
    netif_set_up(&my_netif);
}
```

------

## Topic 4：IP 层

### 4.1 IP 层在 lwIP 中的职责

```text
接收方向：
ethernet_input()
    → ARP 帧 → etharp_input()
    → IP  帧 → ip4_input()
                  → TCP → tcp_input()
                  → UDP → udp_input()
                  → ICMP → icmp_input()

发送方向：
tcp_output() / udp_sendto()
    → ip4_output()
        → etharp_output()  (查 ARP 表)
            → netif->linkoutput()  (写硬件)
```

------

### 4.2 IP 分片与重组

lwIP 默认**支持接收分片**（重组），但**发送分片默认关闭**：

```c
// lwipopts.h
#define IP_REASSEMBLY   1   // 开启接收重组（默认开）
#define IP_FRAG         1   // 开启发送分片（按需开启）

// 重组相关资源配置
#define MEMP_NUM_REASSDATA  5   // 最多同时重组 5 个分片包
#define IP_REASS_MAX_PBUFS  10  // 重组用的最大 pbuf 数
```

> ⚠️ 在 BMS 类嵌入式场景中，如果数据包都是小包（< MTU），建议**关闭分片重组**以节省内存：`IP_REASSEMBLY 0`

------

### 4.3 ICMP（ping）

lwIP 自动处理 ICMP Echo Request，无需用户代码。只需确保：

```c
#define LWIP_ICMP   1   // 默认开启
```

调试时用 ping 验证 IP 层是否正常是最快的方法。

------

### 4.4 路由与多接口

lwIP 支持多个 `netif`，路由规则：

1. 目标 IP 与某 netif 同子网 → 直接从该 netif 发出
2. 否则 → 从默认 netif 发出，目标 MAC 为网关 MAC

```c
// 手动指定路由（高级用法）
ip4_addr_t dest, gw;
// lwIP 无显式路由表 API，通过 netif 子网掩码隐式路由
```

------

## Topic 5：UDP

### 5.1 UDP PCB（Protocol Control Block）

lwIP 用 PCB 结构体管理每个 UDP 端点：

```c
struct udp_pcb {
    IP_PCB;              // 包含本地/远端 IP
    struct udp_pcb *next;
    u8_t  flags;
    u16_t local_port;
    u16_t remote_port;
    udp_recv_fn recv;    // 接收回调函数
    void *recv_arg;      // 回调用户参数
};
```

------

### 5.2 UDP 完整收发示例（RAW API）

```c
#include "lwip/udp.h"

static struct udp_pcb *udp_pcb;

/* 接收回调 */
static void udp_recv_callback(void *arg, struct udp_pcb *pcb,
                               struct pbuf *p,
                               const ip_addr_t *addr, u16_t port) {
    if (p == NULL) return;

    /* 读取数据 */
    char buf[128];
    u16_t copy_len = LWIP_MIN(p->tot_len, sizeof(buf) - 1);
    pbuf_copy_partial(p, buf, copy_len, 0);
    buf[copy_len] = '\0';

    printf("UDP RX from %s:%d -> %s\n", ipaddr_ntoa(addr), port, buf);

    pbuf_free(p);  // ⚠️ 必须释放！
}

/* 初始化 UDP */
void udp_app_init(void) {
    udp_pcb = udp_new();
    udp_bind(udp_pcb, IP_ADDR_ANY, 5000);   // 绑定本地端口 5000
    udp_recv(udp_pcb, udp_recv_callback, NULL);
}

/* 发送 UDP */
void udp_send_data(const char *data, u16_t len) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) return;

    pbuf_take(p, data, len);

    ip_addr_t dest_ip;
    IP4_ADDR(&dest_ip, 192, 168, 1, 200);
    udp_sendto(udp_pcb, p, &dest_ip, 6000);  // 发往 192.168.1.200:6000

    pbuf_free(p);
}
```

------

### 5.3 UDP 广播

```c
// 发送广播
ip_addr_t broadcast;
IP4_ADDR(&broadcast, 255, 255, 255, 255);
udp_sendto(udp_pcb, p, &broadcast, port);

// lwipopts.h 需要开启
#define IP_SOF_BROADCAST            1
#define IP_SOF_BROADCAST_RECV       1
```

------

## Topic 6：TCP

### 6.1 TCP 状态机

lwIP 完整实现了 TCP 状态机：

```text
CLOSED → LISTEN → SYN_RCVD → ESTABLISHED → FIN_WAIT_1
                                          → CLOSE_WAIT → LAST_ACK → CLOSED
```

每个状态转换都有对应的回调通知用户。

------

### 6.2 TCP Server 完整示例（RAW API）

```c
#include "lwip/tcp.h"

static struct tcp_pcb *server_pcb;

/* 数据接收回调 */
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb,
                                struct pbuf *p, err_t err) {
    if (p == NULL) {
        /* 对端关闭连接 */
        tcp_close(tpcb);
        return ERR_OK;
    }

    /* 通知协议栈已消费多少字节（滑动窗口更新）*/
    tcp_recved(tpcb, p->tot_len);

    /* 处理数据 */
    char buf[256];
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    pbuf_free(p);

    /* 回显数据 */
    tcp_write(tpcb, buf, strlen(buf), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);  // 立即发送（否则等 Nagle 算法积累）

    return ERR_OK;
}

/* 新连接接受回调 */
static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

/* 初始化 TCP Server */
void tcp_server_init(void) {
    server_pcb = tcp_new();
    tcp_bind(server_pcb, IP_ADDR_ANY, 80);
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, tcp_accept_callback);
}
```

------

### 6.3 TCP 关键参数调优

```c
// lwipopts.h
#define TCP_MSS         1460        // 最大报文段大小（以太网标准）
#define TCP_WND         (4*TCP_MSS) // 接收窗口（影响吞吐量）
#define TCP_SND_BUF     (4*TCP_MSS) // 发送缓冲区
#define TCP_SND_QUEUELEN (2*TCP_SND_BUF/TCP_MSS) // 发送队列深度

// Nagle 算法（小包合并）
// 关闭 Nagle（低延迟场景）：
tcp_nagle_disable(tpcb);
```

------

### 6.4 `tcp_recved()` 的重要性

```text
⚠️ 这是 TCP 初学者最常犯的错误！

每次在 recv 回调中消费了数据，必须调用：
    tcp_recved(tpcb, bytes_consumed);

否则：接收窗口不会更新 → 对端认为你的缓冲区满了
     → 对端停止发送 → 连接"假死"
```

------

## Topic 7：应用层 API 对比

### 7.1 三种 API 模式

|    特性     |     RAW API      | Netconn API |  Socket API  |
| :---------: | :--------------: | :---------: | :----------: |
|  编程模型   |     事件回调     |   阻塞式    |   BSD 标准   |
| 是否需要 OS |     ❌ 不需要     | ✅ 需要 RTOS | ✅ 需要 RTOS  |
|  内存开销   |       最小       |    中等     |     最大     |
|    性能     |       最高       |    中等     |     中等     |
|  移植难度   | 高（需理解回调） |     低      |     最低     |
|  适用场景   |    无OS嵌入式    | RTOS嵌入式  | 移植现有代码 |

------

### 7.2 RAW API 核心回调汇总

```c
// TCP 回调注册
tcp_recv(pcb, recv_fn);      // 数据到达
tcp_sent(pcb, sent_fn);      // 数据发送确认
tcp_err(pcb, err_fn);        // 连接错误（RST/abort）
tcp_poll(pcb, poll_fn, interval); // 定期轮询（心跳/超时检测）
tcp_accept(pcb, accept_fn);  // 新连接（Server）
tcp_connected(pcb, connected_fn); // 连接建立（Client）
```

------

### 7.3 Netconn API 示例（RTOS 场景）

```c
// 需要 NO_SYS=0，且有 RTOS
struct netconn *conn = netconn_new(NETCONN_TCP);
netconn_bind(conn, IP_ADDR_ANY, 80);
netconn_listen(conn);

while (1) {
    struct netconn *client;
    netconn_accept(conn, &client);  // 阻塞等待连接

    struct netbuf *buf;
    while (netconn_recv(client, &buf) == ERR_OK) {  // 阻塞等待数据
        void *data;
        u16_t len;
        netbuf_data(buf, &data, &len);
        netconn_write(client, data, len, NETCONN_COPY);
        netbuf_delete(buf);
    }
    netconn_close(client);
    netconn_delete(client);
}
```

------

## Topic 8：DHCP / ARP / DNS

### 8.1 DHCP 客户端

```c
// 启动 DHCP（替代静态 IP）
#define LWIP_DHCP   1   // lwipopts.h

// 代码中
dhcp_start(&my_netif);

// 轮询等待 IP 分配（无OS模式）
while (dhcp_supplied_address(&my_netif) == 0) {
    sys_check_timeouts();  // 驱动 lwIP 定时器
    ethernetif_input(&my_netif);
}
printf("IP: %s\n", ip4addr_ntoa(netif_ip4_addr(&my_netif)));
```

------

### 8.2 ARP 工作机制

```text
发送 IP 包时，目标 MAC 未知：
1. lwIP 查 ARP 缓存表（etharp_table）
2. 未命中 → 发送 ARP Request（广播）
3. 目标回复 ARP Reply → 更新缓存
4. 重发挂起的 IP 包

ARP 缓存配置：
#define ARP_TABLE_SIZE  10   // 最多缓存 10 条 ARP 记录
#define ARP_MAXAGE      300  // ARP 条目老化时间（秒）
```

------

## Topic 9：无 OS 驱动模式（NO_SYS=1）

### 9.1 主循环结构

```c
// 无 OS 模式的标准主循环
int main(void) {
    hardware_init();
    network_init();      // lwip_init() + netif_add()

    while (1) {
        /* 1. 驱动网络接收 */
        ethernetif_input(&my_netif);

        /* 2. 驱动 lwIP 内部定时器（TCP 重传、ARP 老化、DHCP 等）*/
        sys_check_timeouts();

        /* 3. 用户应用逻辑 */
        app_poll();
    }
}
```

------

### 9.2 定时器管理

`sys_check_timeouts()` 内部维护一个软件定时器链表，依赖 `sys_now()` 返回的毫秒时间戳。

```text
lwIP 内部定时器（部分）：
- TCP 重传定时器：250ms
- ARP 老化定时器：5s
- DHCP 定时器：500ms
- TCP 保活定时器：75s

⚠️ 必须在主循环中频繁调用 sys_check_timeouts()
   建议每次循环都调用，不要加延时阻塞
```

------

### 9.3 中断与主循环的数据交换

```c
// 推荐模式：中断只设标志，主循环处理
static volatile uint8_t eth_rx_flag = 0;

void ETH_IRQHandler(void) {
    if (ETH_GetRxFlag()) {
        eth_rx_flag = 1;
        ETH_ClearRxFlag();
    }
}

// 主循环
while (1) {
    if (eth_rx_flag) {
        eth_rx_flag = 0;
        ethernetif_input(&my_netif);  // 在主循环上下文处理
    }
    sys_check_timeouts();
}
```

> 💡 这与你在 BMS 项目中使用的轻量级伪并发状态机思路完全一致——中断只做最小工作，主循环做实际处理。

------

## Topic 10：RTOS 集成（NO_SYS=0）

### 10.1 sys_arch 移植层

RTOS 模式下需要实现 `sys_arch.c`，提供：

```c
// 信号量
sys_sem_t    → 对应 RTOS 信号量
sys_sem_new() / sys_sem_signal() / sys_arch_sem_wait()

// 互斥锁
sys_mutex_t  → 对应 RTOS 互斥锁

// 邮箱（消息队列）
sys_mbox_t   → 对应 RTOS 消息队列
sys_mbox_new() / sys_mbox_post() / sys_arch_mbox_fetch()

// 线程创建
sys_thread_new(name, thread_fn, arg, stacksize, prio)
```

------

### 10.2 FreeRTOS 集成示例

```c
// lwIP 官方提供 FreeRTOS 移植，位于：
// contrib/ports/freertos/sys_arch.c

// 典型线程结构：
void tcpip_thread(void *arg) {
    // lwIP 核心线程，处理所有协议栈操作
    // 通过 tcpip_input() 接收来自其他线程的数据
}

// 从 ISR 或其他线程送入数据：
tcpip_input(p, netif);  // 线程安全，内部通过邮箱传递
```

------

## Topic 11：调试与性能调优

### 11.1 LWIP_STATS 统计

```c
// lwipopts.h
#define LWIP_STATS          1
#define LWIP_STATS_DISPLAY  1

// 代码中打印统计
stats_display();
```

输出示例：

```text
MEMP_PBUF: avail=16 used=3 max=8 err=0
MEMP_TCP_PCB: avail=4 used=1 max=2 err=0
MEM: avail=8192 used=1024 max=2048 err=0
```

------

### 11.2 常见问题排查

|     现象     |             可能原因              |                 排查方法                 |
| :----------: | :-------------------------------: | :--------------------------------------: |
|  ping 不通   |    netif 未 up / MAC 地址冲突     |  `netif_is_up()` 检查；Wireshark 抓 ARP  |
| TCP 连接假死 |       忘记调 `tcp_recved()`       |               检查接收回调               |
|   内存耗尽   |   pbuf 泄漏（忘记 `pbuf_free`）   |      `stats_display()` 看 MEMP 使用      |
|   发送卡住   |         TCP 发送缓冲区满          | 增大 `TCP_SND_BUF`；检查 `tcp_sent` 回调 |
|  DHCP 失败   | `sys_check_timeouts()` 调用不及时 |          确保主循环无长时间阻塞          |

------

### 11.3 Wireshark 过滤器速查

```text
# 只看你的设备
eth.addr == 02:00:00:00:00:01

# 看 ARP
arp

# 看 ICMP（ping）
icmp

# 看特定端口 TCP
tcp.port == 80

# 看 TCP 重传
tcp.analysis.retransmission
```

------

## 🗺️ 学习路径建议

```text
第1周：Topic 1 + 2 + 3
  → 目标：能 ping 通，Wireshark 看到 ARP 和 ICMP

第2周：Topic 4 + 5
  → 目标：UDP 收发正常，能与 PC 端 netcat 互通

第3周：Topic 6 + 7
  → 目标：TCP Server 能接受连接并回显数据

第4周：Topic 8 + 9 + 11
  → 目标：DHCP 自动获取 IP，系统稳定运行 24h 无内存泄漏

按需：Topic 10（如果项目引入 RTOS）
```

------

每个 Topic 都可以继续深入展开——你想从哪个 Topic 开始详细讨论？或者有具体的应用场景（比如 BMS 数据上报、OTA 升级通道）想结合 lwIP 来设计？





```
最新的 pbuf typedef enum {
  /** Includes spare room for transport layer header, e.g. UDP header.
   * Use this if you intend to pass the pbuf to functions like udp_send().
   */
  PBUF_TRANSPORT = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN,
  /** Includes spare room for IP header.
   * Use this if you intend to pass the pbuf to functions like raw_send().
   */
  PBUF_IP = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN,
  /** Includes spare room for link layer header (ethernet header).
   * Use this if you intend to pass the pbuf to functions like ethernet_output().
   * @see PBUF_LINK_HLEN
   */
  PBUF_LINK = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN,
  /** Includes spare room for additional encapsulation header before ethernet
   * headers (e.g. 802.11).
   * Use this if you intend to pass the pbuf to functions like netif->linkoutput().
   * @see PBUF_LINK_ENCAPSULATION_HLEN
   */
  PBUF_RAW_TX = PBUF_LINK_ENCAPSULATION_HLEN,
  /** Use this for input packets in a netif driver when calling netif->input()
   * in the most common case - ethernet-layer netif driver. */
  PBUF_RAW = 0
} pbuf_layer; 
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好问题。这个枚举的设计非常精妙，值得深入理解。

------

## `pbuf_layer` 深度解析

### 核心设计思想：**预留头部空间（headroom）**

`pbuf_layer` 的值**不是类型标识符**，而是一个**偏移量（offset）**，告诉 `pbuf_alloc()` 在数据区前面预留多少字节给协议头部。

```text
pbuf_alloc(layer, length, type) 内部：
    payload 指针 = 原始内存起点 + layer（偏移量）
```

------

### 各层 headroom 数值关系

```text
PBUF_LINK_ENCAPSULATION_HLEN  (通常 = 0 或 14，取决于 802.11 等封装)
PBUF_LINK_HLEN                (以太网帧头 = 14 字节)
PBUF_IP_HLEN                  (IP 头      = 20 字节)
PBUF_TRANSPORT_HLEN           (UDP/TCP 头 = 8/20 字节，通常取最大值)

PBUF_RAW     = 0
PBUF_RAW_TX  = ENCAP
PBUF_LINK    = ENCAP + 14
PBUF_IP      = ENCAP + 14 + 20
PBUF_TRANSPORT = ENCAP + 14 + 20 + TRANSPORT_HLEN
```

------

### 内存布局可视化

以 `PBUF_TRANSPORT` 为例，分配后内存长这样：

```text
pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM)

┌──────────┬──────────┬──────────┬──────────┬──────────────────┐
│  ENCAP   │  ETH 头  │  IP 头   │ UDP/TCP头│   你的 payload   │
│ (预留)   │ (预留)   │ (预留)   │ (预留)   │  ← p->payload    │
└──────────┴──────────┴──────────┴──────────┴──────────────────┘
 ←──────────────── layer offset ────────────→ ↑
                                              p->payload 初始指向这里
```

协议栈在发送时，逐层**向前**移动 `payload` 指针，填充各层头部：

```text
udp_send()  → pbuf_add_header(p, UDP_HLEN)   → 填 UDP 头
ip4_output()→ pbuf_add_header(p, IP_HLEN)    → 填 IP 头
ethernet_output()→ pbuf_add_header(p, ETH_HLEN)→ 填 ETH 头
linkoutput()→ 整个 buffer 写入硬件
```

> 这就是为什么叫"预留"——空间在分配时就留好了，头部填充是**原地操作，零拷贝**。

------

### 每个枚举值的使用场景

|      枚举值      |      你的代码处于哪一层      |         典型调用目标         |
| :--------------: | :--------------------------: | :--------------------------: |
| `PBUF_TRANSPORT` |       应用层，只管数据       | `udp_send()` / `tcp_write()` |
|    `PBUF_IP`     |     需要自己构造传输层头     |         `raw_send()`         |
|   `PBUF_LINK`    |  需要自己构造 IP + 传输层头  |     `ethernet_output()`      |
|  `PBUF_RAW_TX`   |   需要自己构造完整以太网帧   |    `netif->linkoutput()`     |
|    `PBUF_RAW`    | **接收路径**，驱动收到原始帧 |       `netif->input()`       |

------

### `PBUF_RAW` 的特殊性

```c
PBUF_RAW = 0  // 没有任何预留空间
```

`PBUF_RAW` 是**接收专用**的语义：

```text
RX 路径：
硬件 → 你的驱动 → pbuf_alloc(PBUF_RAW, frame_len, PBUF_POOL)
                   ↑ payload 直接指向数据起点，没有 headroom
                   → netif->input(p, netif)
                   → ethernet_input() 解析 ETH 头
                   → ip4_input() 解析 IP 头（通过 pbuf_remove_header 移动指针）
                   → udp_input() 解析 UDP 头
                   → 用户 recv 回调，此时 payload 指向纯数据
```

接收方向是**逐层剥头**（`pbuf_remove_header`），与发送方向的逐层加头完全对称。

------

### `pbuf_add_header` / `pbuf_remove_header` 本质

```c
// 伪代码
pbuf_add_header(p, header_size) {
    p->payload -= header_size;  // 指针前移
    p->len     += header_size;
    p->tot_len += header_size;
}

pbuf_remove_header(p, header_size) {
    p->payload += header_size;  // 指针后移
    p->len     -= header_size;
    p->tot_len -= header_size;
}
```

**整个发送/接收过程，数据本身从未移动，只有指针在移动。** 这是 lwIP 零拷贝设计的核心。

------

### 常见错误：layer 选错

```c
// ❌ 错误：用 PBUF_RAW 分配，然后调 udp_send()
struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
udp_send(pcb, p);
// 结果：udp_send 内部 pbuf_add_header 会让 payload 指针
//       越过 pbuf 内存边界，写入非法区域 → 数据损坏或 assert

// ✅ 正确：应用层发送用 PBUF_TRANSPORT
struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
pbuf_take(p, data, len);
udp_send(pcb, p);
pbuf_free(p);
```

------

### 一句话总结

> `pbuf_layer` 的值就是 **headroom 字节数**。选哪个 layer，取决于你把 pbuf 交给协议栈的**哪一层入口**——交给越底层的函数，需要预留的空间越少（因为上层头部你自己填或不需要）；交给越上层的函数，需要预留越多（让协议栈有空间逐层填充头部）。





```
但是上述 2.2 pbuf的四种类型 是否刚才的冲突
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

非常好的观察，确实存在**概念混淆**，需要澄清。

------

## 两个维度，两套枚举，完全独立

`pbuf_alloc()` 有**两个参数**，分别对应两个完全不同的枚举：

```c
struct pbuf *pbuf_alloc(pbuf_layer layer, u16_t length, pbuf_type type);
//                      ↑                              ↑
//                  第一个维度                     第二个维度
//               "headroom 预留多少"            "内存从哪里来"
```

------

## 第一维度：`pbuf_layer`（刚才讲的）

**决定 payload 指针的起始偏移**，与内存来源无关。

```text
PBUF_TRANSPORT / PBUF_IP / PBUF_LINK / PBUF_RAW_TX / PBUF_RAW
```

------

## 第二维度：`pbuf_type`（Topic 2.2 讲的）

**决定内存从哪里分配**，与 headroom 无关。

```c
typedef enum {
  PBUF_RAM,   // 从动态堆分配，pbuf结构体和数据连续存放
  PBUF_ROM,   // pbuf结构体从内存池，payload指向只读静态区
  PBUF_REF,   // pbuf结构体从内存池，payload指向用户提供的buffer
  PBUF_POOL,  // pbuf结构体和数据都从固定内存池分配
} pbuf_type;
```

------

## 两个维度正交组合

```text
                    ┌─────────────────────────────────────────┐
                    │           pbuf_type（内存来源）          │
                    │  RAM      ROM      REF      POOL        │
┌───────────────────┼──────────┬────────┬────────┬───────────┤
│ p  PBUF_TRANSPORT │  ✅常用  │  ✅   │  ✅   │  ✅       │
│ b  PBUF_IP        │  ✅      │  ✅   │  ✅   │  ✅       │
│ u  PBUF_LINK      │  ✅      │  ✅   │  ✅   │  ✅       │
│ f  PBUF_RAW_TX    │  ✅      │  ✅   │  ✅   │  ✅       │
│ _  PBUF_RAW       │  少用    │  少用  │  少用  │  ✅RX常用 │
│ l                 │          │        │        │           │
│ a                 │          │        │        │           │
│ y                 │          │        │        │           │
│ e                 │          │        │        │           │
│ r                 │          │        │        │           │
└───────────────────┴──────────┴────────┴────────┴───────────┘
```

------

## 内存布局对比（同样是 PBUF_TRANSPORT layer）

### `PBUF_TRANSPORT` + `PBUF_RAM`

```text
堆内存（连续一块）：
┌────────────┬──────────┬──────────┬──────────┬──────────┬──────────────┐
│ struct pbuf│  ENCAP   │  ETH头   │  IP头    │ UDP头    │   data       │
│  (结构体)  │  (预留)  │  (预留)  │  (预留)  │  (预留)  │ ←p->payload  │
└────────────┴──────────┴──────────┴──────────┴──────────┴──────────────┘
 ←────────────────────── mem_malloc 一次分配 ──────────────────────────→
```

### `PBUF_TRANSPORT` + `PBUF_POOL`

```text
内存池（固定块大小，PBUF_POOL_BUFSIZE）：
┌────────────┬──────────┬──────────┬──────────┬──────────┬──────────────┐
│ struct pbuf│  ENCAP   │  ETH头   │  IP头    │ UDP头    │   data       │
│  (结构体)  │  (预留)  │  (预留)  │  (预留)  │  (预留)  │ ←p->payload  │
└────────────┴──────────┴──────────┴──────────┴──────────┴──────────────┘
 ←────────── 从预分配的固定池取出，超长则自动形成 pbuf 链 ─────────────→
```

### `PBUF_TRANSPORT` + `PBUF_REF`

```text
内存池（只存结构体）：        用户提供的 buffer：
┌────────────┐               ┌──────────────────────────────────────────┐
│ struct pbuf│               │  ENCAP  │ ETH头 │ IP头 │ UDP头 │  data   │
│  p->payload│ ─────────────→│                                ↑         │
└────────────┘               └──────────────────────────────────────────┘
  从 MEMP 池分配                用户自己管理这块内存（零拷贝）
```

------

## 修正 Topic 2.2 的表述

原来的表述把两个维度混在一起描述了，正确的理解应该是：

| `pbuf_type` |      结构体内存      |      数据内存      |    payload初始指向     |   典型用途    |
| :---------: | :------------------: | :----------------: | :--------------------: | :-----------: |
| `PBUF_RAM`  |   堆（与数据连续）   | 堆（与结构体连续） | 结构体之后 + layer偏移 | TX 应用层数据 |
| `PBUF_POOL` | 内存池（与数据连续） |  内存池（固定块）  | 结构体之后 + layer偏移 |  RX 接收路径  |
| `PBUF_ROM`  |    内存池（单独）    |   用户静态只读区   |      用户指定地址      | 发送常量数据  |
| `PBUF_REF`  |    内存池（单独）    |   用户提供buffer   |      用户指定地址      |  零拷贝发送   |

> **`pbuf_layer` 对 `PBUF_ROM` / `PBUF_REF` 实际上无效**——因为这两种类型的 payload 是你自己指定的外部地址，分配后你需要手动设置 `p->payload`，layer 偏移没有意义。

------

## 一句话总结

> **`pbuf_layer` 解决"头部空间留多少"，`pbuf_type` 解决"内存从哪来"。** 两者正交，Topic 2.2 描述的是 `pbuf_type`，刚才讲的是 `pbuf_layer`，两套概念互不冲突，只是原文没有明确区分这两个维度。





```
lwip有哪些宏拼接骚操作，比如拼接宏定义，拼接成函数
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好问题，lwIP 的宏系统非常有代表性，值得系统梳理。

------

## lwIP 宏拼接骚操作全景

------

### 1. 函数名拼接（最核心的骚操作）

#### 1.1 协议族函数分发

```c
// src/core/ipv4/ip4.c 和 ipv6 同名函数通过宏统一入口

#define ip_output(pcb, p, src, dst, ttl, tos, proto) \
  (IP_IS_V6_PCB(pcb) ?                               \
    ip6_output_if(...)                                \
  : ip4_output_if(...))
```

#### 1.2 MEMP 内存池函数名拼接

```c
// include/lwip/memp.h
// 内存池类型枚举通过宏生成
#define LWIP_MEMPOOL(name, num, size, desc) MEMP_##name,

typedef enum {
#include "lwip/priv/memp_std.h"   // 展开所有 LWIP_MEMPOOL(...)
  MEMP_MAX
} memp_t;
```

`memp_std.h` 内容：

```c
// lwip/priv/memp_std.h
LWIP_MEMPOOL(PBUF,           MEMP_NUM_PBUF,          sizeof(struct pbuf),          "PBUF")
LWIP_MEMPOOL(RAW_PCB,        MEMP_NUM_RAW_PCB,       sizeof(struct raw_pcb),       "RAW_PCB")
LWIP_MEMPOOL(UDP_PCB,        MEMP_NUM_UDP_PCB,        sizeof(struct udp_pcb),       "UDP_PCB")
LWIP_MEMPOOL(TCP_PCB,        MEMP_NUM_TCP_PCB,        sizeof(struct tcp_pcb),       "TCP_PCB")
// ...
```

**同一份 `memp_std.h` 被多次 include，每次 `LWIP_MEMPOOL` 宏定义不同，产生不同代码：**

```c
// 第一次 include：生成枚举
#define LWIP_MEMPOOL(name, num, size, desc) MEMP_##name,
#include "lwip/priv/memp_std.h"
// 展开为：MEMP_PBUF, MEMP_RAW_PCB, MEMP_UDP_PCB, MEMP_TCP_PCB, ...

// 第二次 include：生成内存池大小数组
#define LWIP_MEMPOOL(name, num, size, desc) size,
#include "lwip/priv/memp_std.h"
// 展开为：sizeof(struct pbuf), sizeof(struct raw_pcb), ...

// 第三次 include：生成数量数组
#define LWIP_MEMPOOL(name, num, size, desc) num,
#include "lwip/priv/memp_std.h"
// 展开为：MEMP_NUM_PBUF, MEMP_NUM_RAW_PCB, ...
```

> 这是 lwIP 最经典的**X-Macro 模式**，一份数据源，多次展开，零冗余。

------

### 2. X-Macro 模式（数据驱动代码生成）

#### 2.1 统计结构体自动生成

```c
// include/lwip/stats.h
#define STAT_COUNTER_DEFINE(name) u32_t name;

struct stats_proto {
#define STAT_COUNTER(name) STAT_COUNTER_DEFINE(name)
  STAT_COUNTER(xmit)
  STAT_COUNTER(recv)
  STAT_COUNTER(fw)
  STAT_COUNTER(drop)
  STAT_COUNTER(err)
#undef STAT_COUNTER
};
// 展开为：
// struct stats_proto {
//   u32_t xmit;
//   u32_t recv;
//   u32_t fw;
//   u32_t drop;
//   u32_t err;
// };
```

#### 2.2 调试模块名称表

```c
// 调试子系统枚举 + 名称字符串，同一宏展开两次

#define LWIP_DBG_TYPES_ON \
  X(ETHARP)  \
  X(NETIF)   \
  X(PBUF)    \
  X(API_LIB) \
  X(TCP)     \
  X(UDP)

// 生成枚举
#define X(name) LWIP_DBG_##name,
typedef enum { LWIP_DBG_TYPES_ON LWIP_DBG_MAX } lwip_dbg_type;
#undef X

// 生成名称字符串数组
#define X(name) #name,
static const char *dbg_names[] = { LWIP_DBG_TYPES_ON };
#undef X
```

------

### 3. 条件编译拼接（功能开关）

#### 3.1 IPv4/IPv6 双栈透明切换

```c
// include/lwip/ip.h

#if LWIP_IPV4 && LWIP_IPV6

#define ip_addr_copy(dest, src)         \
  do {                                  \
    if (IP_IS_V6_VAL(src)) {            \
      ip6_addr_copy(*ip_2_ip6(&(dest)), \
                    *ip_2_ip6(&(src))); \
      IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V6); \
    } else {                            \
      ip4_addr_copy(*ip_2_ip4(&(dest)), \
                    *ip_2_ip4(&(src))); \
      IP_SET_TYPE_VAL(dest, IPADDR_TYPE_V4); \
    }                                   \
  } while(0)

#elif LWIP_IPV4   // 纯 IPv4，直接展开，无运行时判断

#define ip_addr_copy(dest, src) \
  ip4_addr_copy(*ip_2_ip4(&(dest)), *ip_2_ip4(&(src)))

#elif LWIP_IPV6   // 纯 IPv6

#define ip_addr_copy(dest, src) \
  ip6_addr_copy(*ip_2_ip6(&(dest)), *ip_2_ip6(&(src)))

#endif
```

上层代码永远只写 `ip_addr_copy()`，编译期根据配置选择实现，**零运行时开销**。

------

### 4. 统计计数宏（有无统计零开销切换）

```c
// include/lwip/stats.h

#if LWIP_STATS

#define STATS_INC(x)      (lwip_stats.x++)
#define STATS_DEC(x)      (lwip_stats.x--)
#define STATS_INC_USED(x) do { lwip_stats.x.used++; \
                               if (lwip_stats.x.used > lwip_stats.x.max) \
                                 lwip_stats.x.max = lwip_stats.x.used; \
                               } while(0)

#else  // 关闭统计时，完全消除，不产生任何代码

#define STATS_INC(x)
#define STATS_DEC(x)
#define STATS_INC_USED(x)

#endif

// 使用：协议栈内部到处都是
STATS_INC(ip.recv);
STATS_INC(tcp.xmit);
```

------

### 5. 调试打印宏（分级 + 模块过滤）

```c
// include/lwip/debug.h

#define LWIP_DEBUGF(debug, message)                          \
  do {                                                       \
    if (                                                     \
      ((debug) & LWIP_DBG_ON)          &&  /* 该模块已开启 */ \
      ((debug) & LWIP_DBG_TYPES_ON)    &&  /* 级别匹配 */    \
      ((s16_t)((debug) & LWIP_DBG_MASK_LEVEL) >=            \
               LWIP_DBG_MIN_LEVEL)                           \
    ) {                                                      \
      LWIP_PLATFORM_DIAG(message);  /* 实际输出，你来实现 */ \
    }                                                        \
  } while(0)

// 使用：
LWIP_DEBUGF(TCP_DEBUG, ("tcp_recv: window %"U16_F"\n", wnd));
//          ↑模块+级别   ↑注意双括号！message 本身是一个括号表达式
```

**双括号技巧**：`message` 是 `(fmt, ...)` 形式，整体作为一个宏参数传入，内部展开为 `printf(fmt, ...)`，避免可变参数宏的兼容性问题（C89 不支持 `__VA_ARGS__`）。

------

### 6. 类型安全的字节序转换宏

```c
// include/lwip/def.h

// 编译期检测大小端，选择不同实现
#if BYTE_ORDER == BIG_ENDIAN

#define lwip_htons(x) ((u16_t)(x))   // 大端机器，直接返回
#define lwip_htonl(x) ((u32_t)(x))

#else  // 小端机器，需要字节交换

#define lwip_htons(x) ((u16_t)((((x) & 0x00ffU) << 8) | \
                                (((x) & 0xff00U) >> 8)))

#define lwip_htonl(x) ((u32_t)((((x) & 0x000000ffUL) << 24) | \
                                (((x) & 0x0000ff00UL) <<  8) | \
                                (((x) & 0x00ff0000UL) >>  8) | \
                                (((x) & 0xff000000UL) >> 24)))
#endif

// 别名宏
#define lwip_ntohs(x) lwip_htons(x)
#define lwip_ntohl(x) lwip_htonl(x)

// 再套一层，允许平台覆盖
#ifndef htons
#define htons(x) lwip_htons(x)
#endif
```

------

### 7. 结构体字段偏移 + 容器指针还原

```c
// include/lwip/priv/tcp_priv.h
// 从成员指针还原父结构体指针（类似 Linux container_of）

#define TCP_PCB_COMMON(type) \
  type *next;                \
  void *callback_arg;        \
  TCP_PCB_EXTARGS            \
  enum tcp_state state;      \
  u8_t  prio;                \
  ip_addr_t local_ip;        \
  ip_addr_t remote_ip;       \
  u16_t local_port

// tcp_pcb 和 tcp_pcb_listen 共享头部布局
struct tcp_pcb_listen {
  TCP_PCB_COMMON(struct tcp_pcb_listen);  // ← 宏展开公共字段
  // listen 专有字段...
};

struct tcp_pcb {
  TCP_PCB_COMMON(struct tcp_pcb);         // ← 同一宏，不同类型参数
  // 完整 PCB 专有字段...
};
```

**`TCP_PCB_COMMON(type)` 中的 `type` 参数用于 `next` 指针的类型**，同一宏在两个结构体中展开，`next` 指针类型不同，实现了简单的"泛型链表"。

------

### 8. 平台适配宏（编译器无关抽象）

```c
// include/lwip/arch.h

// 内存对齐
#ifndef LWIP_MEM_ALIGN_SIZE
#define LWIP_MEM_ALIGN_SIZE(size) \
  (((size) + MEM_ALIGNMENT - 1U) & ~(MEM_ALIGNMENT - 1U))
#endif

// 对齐指针
#ifndef LWIP_MEM_ALIGN
#define LWIP_MEM_ALIGN(addr) \
  ((void *)(((mem_ptr_t)(addr) + MEM_ALIGNMENT - 1) \
            & ~(mem_ptr_t)(MEM_ALIGNMENT - 1)))
#endif

// 编译器 packed 属性抽象
#ifndef PACK_STRUCT_BEGIN
#define PACK_STRUCT_BEGIN   // GCC 用 __attribute__，MSVC 用 #pragma pack
#endif
#ifndef PACK_STRUCT_STRUCT
#define PACK_STRUCT_STRUCT __attribute__((packed))
#endif

// 使用：
PACK_STRUCT_BEGIN
struct eth_hdr {
  PACK_STRUCT_FLD_8(u8_t dest[ETH_HWADDR_LEN]);
  PACK_STRUCT_FLD_8(u8_t src[ETH_HWADDR_LEN]);
  PACK_STRUCT_FIELD(u16_t type);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
```

------

### 9. 格式化字符串类型宏（跨平台 printf）

```c
// include/lwip/arch.h

// 不同平台 u16_t 可能是 unsigned short 或 unsigned int
// 直接用 %u 可能触发编译器警告

#define U16_F  "hu"   // 或 "u"，取决于平台
#define S16_F  "hd"
#define U32_F  "lu"   // 或 "u"
#define S32_F  "ld"
#define X8_F   "02x"
#define U8_F   "u"    // 或 "hu"

// 使用：
LWIP_DEBUGF(TCP_DEBUG,
  ("port=%"U16_F" seq=%"U32_F"\n", pcb->local_port, pcb->snd_nxt));
// 展开为：
// printf("port=%hu seq=%lu\n", pcb->local_port, pcb->snd_nxt);
```

------

### 总结：lwIP 宏骚操作分类

```text
┌─────────────────────┬──────────────────────────────────────────┐
│ 技术                │ 用途                                      │
├─────────────────────┼──────────────────────────────────────────┤
│ ## 拼接             │ 枚举名、数组名、函数名自动生成            │
│ X-Macro             │ 同一数据源多次展开，枚举/数组/字符串同步  │
│ 多次 #include       │ 配合 X-Macro，用同一头文件生成不同代码    │
│ 条件编译分发        │ IPv4/IPv6 双栈透明切换，零运行时开销      │
│ 空宏开关            │ 统计/调试完全消除，不影响生产代码体积     │
│ 双括号 message      │ 兼容 C89 的可变参数调试打印              │
│ 宏展开公共结构体    │ 类似 C++ 继承的 PCB 公共字段复用         │
│ 平台抽象宏          │ packed、对齐、printf格式跨编译器兼容      │
└─────────────────────┴──────────────────────────────────────────┘
```

最值得深挖的是 **X-Macro + 多次 `#include`** 这套组合——`memp_std.h` 被 include 了至少 5 次，每次 `LWIP_MEMPOOL` 宏定义不同，分别生成枚举、大小数组、数量数组、描述字符串数组、内存池静态变量定义，完全没有冗余。你在 BMS 项目里的宏注册 Shell 命令框架，本质上用的是同一套思路。





```
lwip 还有什么值得学习的麽，好像 和 protothreads 同一作者
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

对，你眼光很准。lwIP 和 Protothreads 都是 **Adam Dunkels** 的作品，他还有一个更大的项目把这些全串起来了。

------

## Adam Dunkels 的技术谱系

```text
Adam Dunkels
├── lwIP          (2001)  轻量级 TCP/IP 协议栈
├── Protothreads  (2005)  无栈协程原语
├── uIP           (2000)  比 lwIP 更小的 TCP/IP（8位MCU级别）
└── Contiki OS    (2002)  ← 把上面全部串起来的嵌入式OS
```

> **Contiki OS** 是真正值得研究的东西——它是一个完整的嵌入式操作系统，lwIP / uIP / Protothreads 都是它的子系统，整个系统的架构设计极其精妙。

------

## Contiki OS 值得学习的核心设计

### 1. 事件驱动内核（无抢占）

```c
// Contiki 的核心调度模型
// 不是 RTOS 的抢占式调度，而是事件驱动

while(1) {
    // 取出一个事件
    event = event_queue_get();

    // 分发给目标进程
    process_run(event->target_process, event);

    // 目标进程运行直到主动让出（PT_YIELD / PT_WAIT）
}
```

**与 lwIP NO_SYS 模式的关系**：lwIP 的 `sys_check_timeouts()` + 主循环轮询，本质上就是 Contiki 事件驱动模型的简化版。

------

### 2. Protothreads 在 Contiki 中的真实用法

```c
// Contiki 进程 = Protothread + 事件响应

PROCESS(tcp_process, "TCP Handler");

PROCESS_THREAD(tcp_process, ev, data) {
    PROCESS_BEGIN();

    // 等待网络事件（挂起，不占栈）
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    // 处理数据
    handle_tcp_data(data);

    PROCESS_END();
}
```

这和你在 BMS 项目里用 Protothread 做状态机协调是完全一样的模式，只是 Contiki 把它系统化了。

------

### 3. Coffee 文件系统（嵌入式 Flash 文件系统）

```text
Coffee FS 设计要点：
├── 专为 Flash 擦写特性设计（顺序写，避免随机写）
├── 无动态内存分配（静态元数据区）
├── 日志结构（Log-structured）写入
└── 与你做的 NVM 模拟机制思路高度相似
```

> 你之前设计的"不依赖动态内存分配的 NVM 模拟机制"，Coffee FS 是一个完整的工业级参考实现，值得对照研究。

------

## lwIP 本身还有哪些值得深挖

### 值得继续学的部分

```text
lwIP 深水区
├── altcp（应用层 TLS 抽象层）    ← 与 mbedTLS 对接
├── PPPoS                         ← 串口拨号，IoT 4G模块常用
├── SNMP Agent                    ← 网络管理，工业设备必备
├── MQTT / HTTP 客户端            ← contrib 目录里有参考实现
└── IPv6 + NDP                    ← 下一代协议栈
```

### altcp 特别值得关注

```c
// altcp = Application Layered TCP
// 在 TCP 之上插入一个透明代理层

应用代码
    ↓ altcp_write() / altcp_recv()
[ altcp 抽象层 ]  ← 可以是 TLS、代理、任何中间层
    ↓
[ 底层 TCP PCB ]

// 你的应用代码完全不感知下面是裸 TCP 还是 TLS
// 与你做的共享加密库二进制跨工程调用架构，思路一致
```

------

## 真正值得花时间的方向建议

结合你的 BMS 背景，优先级排序：

```text
优先级 1：lwIP altcp + mbedTLS
    → BMS 数据上云，安全传输通道

优先级 2：Contiki 事件驱动内核源码
    → 理解你现在用的 Protothread 模式的完整体系

优先级 3：Coffee FS 源码
    → 对照你的 NVM 方案，看工业级实现的差异

优先级 4：uIP 源码（比 lwIP 更小）
    → 极限资源下的协议栈裁剪思路
```

------

## 一句话总结

> lwIP 只是 Adam Dunkels 技术体系的一个切面。**Contiki OS** 才是完整答案——它把事件驱动调度、Protothreads、轻量级网络栈、Flash 文件系统整合成一个自洽的嵌入式系统架构，每一个子系统都值得单独深挖，而且和你现在做的 BMS 固件架构高度相关。

要展开哪个方向？





```
Contiki OS 逐步展开
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

# Contiki OS 系统学习指南

> **定位**：Contiki 是专为资源极度受限的嵌入式系统设计的开源操作系统，核心创新在于用纯 C 实现了事件驱动 + 协作式多任务，不依赖硬件栈切换。整个系统是理解现代嵌入式 OS 设计的极佳教材。

------

## Topic 总览

|  #   |         Topic         |              核心内容               |
| :--: | :-------------------: | :---------------------------------: |
|  1   |     **整体架构**      |    设计哲学、与 RTOS 的本质区别     |
|  2   |     **进程模型**      |  process 结构体、生命周期、调度器   |
|  3   |     **事件系统**      |  事件队列、同步/异步事件、事件分发  |
|  4   | **Protothreads 集成** |     PT 在 Contiki 中的真实角色      |
|  5   |    **定时器体系**     | etimer / ctimer / rtimer 三层定时器 |
|  6   |     **内存模型**      |   memb / list / mmem 三套内存管理   |
|  7   |    **网络子系统**     |        uIP / Rime 协议栈集成        |
|  8   |  **Coffee 文件系统**  |    Flash 感知的日志结构文件系统     |
|  9   |     **平台移植**      |     硬件抽象层、时钟、中断接口      |
|  10  |   **与 RTOS 对比**    |   何时选 Contiki，何时选 FreeRTOS   |

------

------

## Topic 1：整体架构

### 1.1 设计哲学

Contiki 的核心问题意识：

```text
传统 RTOS 的代价：
├── 每个任务需要独立栈（通常 512B ~ 4KB）
├── 上下文切换需要保存/恢复全部寄存器
├── 栈大小必须按最坏情况预分配（浪费）
└── 10 个任务 × 1KB 栈 = 10KB RAM（对 MCU 是灾难）

Contiki 的回答：
├── 所有进程共享同一个栈
├── 进程切换不保存栈（Protothread 原理）
├── 进程状态只保存一个 PC 值（2 字节）
└── 10 个进程的额外开销 ≈ 10 × sizeof(process) ≈ 几十字节
```

### 1.2 与 RTOS 的本质区别

```text
RTOS（抢占式）：                    Contiki（协作式事件驱动）：

Task A ──────┐                     Process A ──────────────────────
             │ 抢占                              ↑ 事件到达才运行
Task B    ───┴──────┐              Process B          ──────
                    │ 抢占                        ↑ 事件到达才运行
Task C          ────┴──────        Process C               ──────

特点：                              特点：
- 任意时刻可被打断                  - 运行到 YIELD/WAIT 才让出
- 需要互斥锁保护共享资源            - 单线程语义，无需互斥锁
- 栈独立，内存开销大                - 共享栈，内存开销极小
```

### 1.3 整体架构图

```text
┌─────────────────────────────────────────────────────┐
│                   Applications                       │
│  (sensors / actuators / network apps / shell ...)   │
├─────────────────────────────────────────────────────┤
│                  Contiki Services                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────────────────┐ │
│  │  Timers  │ │  Memory  │ │   Network (uIP/Rime)  │ │
│  │ etimer   │ │  memb    │ │   Coffee FS           │ │
│  │ ctimer   │ │  list    │ │   Serial / UART       │ │
│  │ rtimer   │ │  mmem    │ │                       │ │
│  └──────────┘ └──────────┘ └──────────────────────┘ │
├─────────────────────────────────────────────────────┤
│              Contiki Kernel                          │
│  ┌─────────────────────┐  ┌────────────────────┐    │
│  │   Process Scheduler │  │   Event System     │    │
│  │   (process.c)       │  │   (event queue)    │    │
│  └─────────────────────┘  └────────────────────┘    │
├─────────────────────────────────────────────────────┤
│              Protothreads (pt.h)                     │
├─────────────────────────────────────────────────────┤
│              Platform HAL                            │
│  (clock / uart / gpio / watchdog / rtc ...)          │
└─────────────────────────────────────────────────────┘
```

### 1.4 源码目录结构

```text
contiki/
├── core/
│   ├── sys/
│   │   ├── process.c/h     ← 进程调度器（内核核心）
│   │   ├── event.c/h       ← 事件系统
│   │   ├── etimer.c/h      ← 事件定时器
│   │   ├── ctimer.c/h      ← 回调定时器
│   │   ├── rtimer.c/h      ← 实时定时器
│   │   └── pt.h            ← Protothreads
│   ├── lib/
│   │   ├── memb.c/h        ← 固定块内存池
│   │   ├── list.c/h        ← 侵入式链表
│   │   └── mmem.c/h        ← 托管内存
│   └── net/
│       ├── uip/            ← TCP/IP 协议栈
│       └── rime/           ← 低功耗无线协议栈
├── platform/               ← 各平台移植
│   ├── avr/
│   ├── cc2538/             ← TI 无线 MCU
│   └── native/             ← Linux 仿真（调试用）
└── apps/                   ← 应用示例
```

------

## Topic 2：进程模型

### 2.1 process 结构体

```c
// core/sys/process.h

struct process {
    struct process *next;          // 进程链表指针
    const char     *name;          // 进程名（调试用）
    PT_THREAD((*thread)            // 进程线程函数指针
              (struct pt *,        //   Protothread 上下文
               process_event_t,    //   当前事件
               process_data_t));   //   事件附带数据
    struct pt       pt;            // Protothread 状态（就是一个 lc_t）
    unsigned char   state;         // NONE / RUNNING / CALLED
    unsigned char   needspoll;     // 是否需要轮询
};
```

> **关键洞察**：整个进程状态只有 `struct pt`（本质是一个 `uint16_t` 的行号），这就是 Contiki 内存极省的根本原因。

### 2.2 进程定义宏

```c
// 定义一个进程（本质是声明 + 定义两个东西）
PROCESS(blink_process, "LED Blink");

// 展开为：
// extern struct process blink_process;          // 声明
// struct process blink_process = {              // 定义
//     NULL,                                     // next
//     "LED Blink",                              // name
//     process_thread_blink_process,             // thread 函数指针
//     {0},                                      // pt（初始状态）
//     PROCESS_STATE_NONE,                       // state
//     0                                         // needspoll
// };

// 进程线程函数实现
PROCESS_THREAD(blink_process, ev, data) {
    PROCESS_BEGIN();

    while(1) {
        leds_toggle(LEDS_RED);
        // 等待 etimer 事件（挂起，不占 CPU）
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    }

    PROCESS_END();
}
```

### 2.3 进程生命周期

```text
process_start()
      ↓
  state = RUNNING
  发送 PROCESS_EVENT_INIT 事件
      ↓
  process_run() 调度
      ↓
  thread() 执行直到 PT_YIELD / PT_WAIT
      ↓
  ┌─────────────────────────────────┐
  │  等待事件（state = RUNNING,     │
  │  pt.lc 保存当前行号）           │
  └─────────────────────────────────┘
      ↓ 事件到达
  thread() 从 pt.lc 恢复执行
      ↓
  PROCESS_END() 或 process_exit()
      ↓
  state = NONE
  发送 PROCESS_EVENT_EXIT 事件
  从进程链表移除
```

### 2.4 调度器核心（process.c）

```c
// 简化版 process_run()
int process_run(void) {
    // 1. 处理需要轮询的进程（needspoll 标志）
    if (poll_requested) {
        poll_requested = 0;
        for (p = process_list; p != NULL; p = p->next) {
            if (p->needspoll) {
                p->needspoll = 0;
                p->state = PROCESS_STATE_RUNNING;
                // 调用进程线程函数，传入 PROCESS_EVENT_POLL
                call_process(p, PROCESS_EVENT_POLL, NULL);
            }
        }
    }

    // 2. 处理事件队列中的一个事件
    if (nevents > 0) {
        event = event_queue[fevent];
        fevent = (fevent + 1) % PROCESS_CONF_NUMEVENTS;
        nevents--;

        // 分发给目标进程
        if (event.p == PROCESS_BROADCAST) {
            // 广播：遍历所有进程
            for (p = process_list; p != NULL; p = p->next) {
                call_process(p, event.ev, event.data);
            }
        } else {
            call_process(event.p, event.ev, event.data);
        }
    }

    return nevents + poll_requested;
}
// call_process：真正调用进程线程函数
static void call_process(struct process *p,
                          process_event_t ev,
                          process_data_t data) {
    if (p->state == PROCESS_STATE_RUNNING && p->thread != NULL) {
        // 调用 Protothread 函数
        // 返回 PT_ENDED 说明进程结束
        if (p->thread(&p->pt, ev, data) == PT_ENDED) {
            process_exit(p);
        }
    }
}
```

### 2.5 主循环结构

```c
// platform/xxx/main.c 典型结构

int main(void) {
    // 硬件初始化
    clock_init();
    uart_init();

    // Contiki 内核初始化
    process_init();
    etimer_init();

    // 启动系统进程
    process_start(&etimer_process, NULL);

    // 启动用户进程
    process_start(&blink_process, NULL);
    process_start(&sensor_process, NULL);

    // 主循环：不断处理事件
    while(1) {
        // 运行一次事件调度
        int pending = process_run();

        // 没有待处理事件，进入低功耗
        if (pending == 0) {
            // MCU 进入 sleep，等待中断唤醒
            ENERGEST_OFF(ENERGEST_TYPE_CPU);
            __WFI();   // 或平台相关的 sleep 调用
            ENERGEST_ON(ENERGEST_TYPE_CPU);
        }
    }
}
```

> 这与你 BMS 项目中极低功耗模式的主循环设计完全同构——有事件处理事件，无事件休眠，中断唤醒。

------

## Topic 3：事件系统

### 3.1 事件的本质

```c
// 事件 = 一个整数 ID + 一个 void* 数据指针
typedef unsigned char process_event_t;   // 事件 ID（0~255）
typedef void*         process_data_t;    // 附带数据

// 事件队列节点
struct event_data {
    process_event_t  ev;    // 事件 ID
    process_data_t   data;  // 附带数据（可以是任何指针）
    struct process  *p;     // 目标进程（NULL = 广播）
};

// 环形事件队列
static struct event_data events[PROCESS_CONF_NUMEVENTS];  // 默认 32 个槽
static process_num_events_t nevents;   // 队列中待处理事件数
static process_num_events_t fevent;    // 队列头索引
```

### 3.2 系统内置事件

```c
// core/sys/process.h

// 进程生命周期事件
#define PROCESS_EVENT_NONE      0x80  // 空事件
#define PROCESS_EVENT_INIT      0x81  // 进程启动时收到
#define PROCESS_EVENT_POLL      0x82  // 轮询请求
#define PROCESS_EVENT_EXIT      0x83  // 进程退出时收到
#define PROCESS_EVENT_CONTINUE  0x86  // 继续执行

// 定时器事件（etimer 到期时发送）
#define PROCESS_EVENT_TIMER     0x85

// 消息传递
#define PROCESS_EVENT_MSG       0x84

// 用户自定义事件从这里开始分配
#define PROCESS_EVENT_MAX       0xFF
```

### 3.3 同步 vs 异步事件

```c
// ① 异步事件（投入队列，下次调度时处理）
process_post(target_process, event_id, data);
//           ↑ 目标进程，PROCESS_BROADCAST = 广播所有

// ② 同步事件（立即调用目标进程，当前调用栈内完成）
process_post_synch(target_process, event_id, data);
// ⚠️ 同步事件会立即执行目标进程的 thread()
//    相当于函数调用，注意重入问题

// ③ 轮询请求（最高优先级，下次 process_run 优先处理）
process_poll(target_process);
// 典型用途：在中断中通知进程有数据到达
```

### 3.4 中断与进程通信模式

```c
// ⚠️ 中断中不能调用 process_post()（非线程安全）
// 正确模式：中断只设标志 + process_poll()

static volatile uint8_t uart_rx_flag = 0;
static volatile uint8_t uart_rx_byte = 0;

// UART 中断
void UART_IRQHandler(void) {
    uart_rx_byte = UART_ReadByte();
    uart_rx_flag = 1;
    // 通知进程有数据，poll 是中断安全的
    process_poll(&uart_process);
}

// uart_process 线程
PROCESS_THREAD(uart_process, ev, data) {
    PROCESS_BEGIN();
    while(1) {
        // 等待 POLL 事件（由中断触发）
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
        if (uart_rx_flag) {
            uart_rx_flag = 0;
            handle_byte(uart_rx_byte);
        }
    }
    PROCESS_END();
}
```

### 3.5 用户自定义事件

```c
// 分配自定义事件 ID（运行时分配，全局唯一）
process_event_t my_data_ready_event;

PROCESS_THREAD(init_process, ev, data) {
    PROCESS_BEGIN();

    // 分配事件 ID
    my_data_ready_event = process_alloc_event();

    // 发送给另一个进程
    process_post(&consumer_process,
                 my_data_ready_event,
                 &my_data_struct);   // 传递数据指针

    PROCESS_END();
}

// 消费者进程
PROCESS_THREAD(consumer_process, ev, data) {
    PROCESS_BEGIN();
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == my_data_ready_event);
        struct my_data *d = (struct my_data *)data;
        // 处理数据...
    }
    PROCESS_END();
}
```

------

## Topic 4：Protothreads 集成

### 4.1 PT 在 Contiki 中的真实角色

```text
Contiki 进程 ≠ Protothread
Contiki 进程 = 事件响应框架 + 内嵌一个 Protothread

每次事件到达 → 调用 thread() → PT 从上次 yield 点恢复
                                → 执行直到下一个 yield 点
                                → 返回（让出 CPU）
```

### 4.2 PROCESS_WAIT 系列宏展开

```c
// 等待任意事件
#define PROCESS_WAIT_EVENT() \
    PT_YIELD(&process_pt)

// 等待满足条件的事件
#define PROCESS_WAIT_EVENT_UNTIL(c) \
    PT_YIELD_UNTIL(&process_pt, c)
// 展开为：
// do {
//     LC_SET(pt->lc);          // 保存当前行号
//     if (!(c)) return PT_YIELDED;  // 条件不满足，挂起
// } while(0)
// 下次调用时从 LC_SET 处恢复，重新检查条件

// 暂停当前进程，让其他进程运行
#define PROCESS_PAUSE() \
    do {                                              \
        process_post(PROCESS_CURRENT(),               \
                     PROCESS_EVENT_CONTINUE, NULL);   \
        PROCESS_WAIT_EVENT_UNTIL(                     \
            ev == PROCESS_EVENT_CONTINUE);            \
    } while(0)
// 原理：给自己发一个异步事件，然后挂起等它
//       其他进程有机会在这期间运行
```

### 4.3 进程内嵌子 Protothread

```c
// Contiki 进程可以在内部使用额外的 PT
// 实现更细粒度的状态机

static PT_THREAD(send_packet(struct pt *pt, uint8_t *buf, uint16_t len)) {
    PT_BEGIN(pt);

    // 等待发送缓冲区就绪
    PT_WAIT_UNTIL(pt, tx_buffer_ready());
    memcpy(tx_buf, buf, len);
    trigger_tx();

    // 等待发送完成
    PT_WAIT_UNTIL(pt, tx_done_flag);
    tx_done_flag = 0;

    PT_END(pt);
}

PROCESS_THREAD(network_process, ev, data) {
    static struct pt send_pt;  // ⚠️ 必须 static！PT_YIELD 后局部变量消失

    PROCESS_BEGIN();
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == data_ready_event);

        // 调用子 PT（会多次 yield 直到完成）
        PT_INIT(&send_pt);
        PROCESS_PT_SPAWN(&send_pt,
                         send_packet(&send_pt, payload, len));
    }
    PROCESS_END();
}
```

------

## Topic 5：定时器体系

Contiki 有三层定时器，分工明确：

```text
rtimer  ← 硬件定时器，精度最高，中断上下文，不能用 PT
ctimer  ← 回调定时器，基于 etimer，回调在进程上下文
etimer  ← 事件定时器，到期发送 PROCESS_EVENT_TIMER 事件
```

### 5.1 etimer（事件定时器）

```c
// 最常用，与进程事件系统深度集成

PROCESS_THREAD(blink_process, ev, data) {
    static struct etimer et;   // ⚠️ 必须 static

    PROCESS_BEGIN();

    while(1) {
        // 设置 1 秒定时器
        etimer_set(&et, CLOCK_SECOND);

        // 挂起，等待定时器事件
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        leds_toggle(LEDS_RED);
    }

    PROCESS_END();
}

// etimer API
etimer_set(&et, interval);      // 设置并启动
etimer_reset(&et);              // 从上次到期时间重置（精确周期）
etimer_restart(&et);            // 从现在重置（可能有漂移）
etimer_stop(&et);               // 停止
etimer_expired(&et);            // 是否已到期
etimer_expiration_time(&et);    // 到期时间戳
```

### 5.2 ctimer（回调定时器）

```c
// 不需要进程等待，到期直接调用回调函数
// 适合不需要挂起进程的定时任务

static struct ctimer ct;

void timeout_callback(void *data) {
    struct sensor_ctx *ctx = (struct sensor_ctx *)data;
    // 在进程上下文中执行（安全）
    trigger_measurement(ctx);

    // 可以在回调中重新设置定时器（实现周期回调）
    ctimer_reset(&ct);
}

// 启动 500ms 后调用 callback
ctimer_set(&ct, CLOCK_SECOND / 2, timeout_callback, &sensor_ctx);

// ctimer API
ctimer_set(&ct, interval, callback, data);
ctimer_reset(&ct);    // 重置为原始间隔
ctimer_restart(&ct);  // 从现在重新计时
ctimer_stop(&ct);
ctimer_expired(&ct);
```

### 5.3 rtimer（实时定时器）

```c
// 直接基于硬件定时器，在中断上下文执行
// 精度最高，但不能使用任何 Contiki 进程/事件 API

static struct rtimer rt;

void rtimer_callback(struct rtimer *t, void *ptr) {
    // ⚠️ 中断上下文！
    // 只能做最简单的操作
    gpio_toggle(LED_PIN);

    // 重新调度（实现精确周期）
    rtimer_set(t, RTIMER_NOW() + RTIMER_SECOND / 100,
               rtimer_callback, ptr);
}

// 10ms 后触发
rtimer_set(&rt, RTIMER_NOW() + RTIMER_SECOND / 100,
           rtimer_callback, NULL);
```

### 5.4 三层定时器对比

```text
┌─────────┬──────────┬──────────┬──────────┬──────────────────────┐
│         │ 精度     │ 执行上下文│ 可用API  │ 典型用途             │
├─────────┼──────────┼──────────┼──────────┼──────────────────────┤
│ etimer  │ 低(ms级) │ 进程上下文│ 全部     │ 应用层周期任务       │
│ ctimer  │ 低(ms级) │ 进程上下文│ 全部     │ 超时回调、延迟执行   │
│ rtimer  │ 高(us级) │ 中断上下文│ 极少     │ 精确时序、MAC层调度  │
└─────────┴──────────┴──────────┴──────────┴──────────────────────┘
```

------

## Topic 6：内存模型

Contiki 提供三套内存管理，对应不同场景：

### 6.1 memb（固定块内存池）

```c
// 最常用，零碎片，分配 O(1)
// 类似 lwIP 的 MEMP

#include "lib/memb.h"

// 声明一个存放 MAX_SENSORS 个 sensor_t 的内存池
MEMB(sensor_pool, struct sensor_t, MAX_SENSORS);

void init(void) {
    memb_init(&sensor_pool);
}

struct sensor_t *alloc_sensor(void) {
    return memb_alloc(&sensor_pool);  // 返回 NULL 表示池满
}

void free_sensor(struct sensor_t *s) {
    memb_free(&sensor_pool, s);
}
```

**内部实现**：

```c
// MEMB 宏展开
#define MEMB(name, structure, num) \
    static char name##_memb_count[num];           \
    static structure name##_memb_mem[num];        \
    static struct memb name = {sizeof(structure), \
                                num,              \
                                name##_memb_count,\
                                (void *)name##_memb_mem}

// count 数组：0=空闲，1=已分配
// 分配时线性扫描找第一个 count[i]==0 的槽
```

### 6.2 list（侵入式链表）

```c
// Contiki 的链表要求节点第一个字段是 next 指针
// 类似 Linux 内核链表，但更简单

#include "lib/list.h"

struct packet {
    struct packet *next;   // ⚠️ 必须是第一个字段
    uint8_t  data[64];
    uint16_t len;
};

// 声明链表头
LIST(packet_queue);

void init(void) {
    list_init(packet_queue);
}

void enqueue(struct packet *p) {
    list_add(packet_queue, p);      // 添加到尾部
}

struct packet *dequeue(void) {
    return list_pop(packet_queue);  // 从头部取出
}

// 遍历
for (struct packet *p = list_head(packet_queue);
     p != NULL;
     p = list_item_next(p)) {
    process_packet(p);
}
```

### 6.3 memb + list 组合（标准模式）

```c
// Contiki 内部到处都是这个组合
// memb 管内存，list 管顺序

MEMB(timer_pool, struct ctimer, MAX_CTIMERS);
LIST(timer_list);

struct ctimer *ctimer_alloc(void) {
    struct ctimer *t = memb_alloc(&timer_pool);
    if (t != NULL) {
        list_add(timer_list, t);
    }
    return t;
}

void ctimer_free(struct ctimer *t) {
    list_remove(timer_list, t);
    memb_free(&timer_pool, t);
}
```

### 6.4 mmem（托管内存，Managed Memory）

```c
// 用于需要动态大小但生命周期明确的场景
// 内部是紧凑堆，支持压缩（移动已分配块）

#include "lib/mmem.h"

struct mmem mymem;

// 分配 100 字节
if (mmem_alloc(&mymem, 100)) {
    uint8_t *ptr = MMEM_PTR(&mymem);  // 获取指针
    // 使用 ptr...
    mmem_free(&mymem);
}

// ⚠️ mmem 可能移动内存块（压缩碎片）
//    不能长期持有裸指针，每次使用前通过 MMEM_PTR 获取
```

------

## Topic 7：网络子系统

### 7.1 两套协议栈

```text
Contiki 网络
├── uIP    ← 有线/WiFi，TCP/IP 协议栈（Adam 的第一个作品）
└── Rime   ← 无线传感器网络，比 TCP/IP 更轻量的无线协议栈
```

### 7.2 uIP vs lwIP

```text
┌──────────────┬────────────────┬────────────────────┐
│              │ uIP            │ lwIP               │
├──────────────┼────────────────┼────────────────────┤
│ RAM 占用     │ ~2KB           │ ~10KB+             │
│ 目标平台     │ 8位 MCU        │ 32位 MCU           │
│ TCP 窗口     │ 单包（无滑窗） │ 完整滑动窗口        │
│ 并发连接     │ 极少           │ 多连接             │
│ API          │ 事件回调       │ RAW/Netconn/Socket │
│ 与Contiki    │ 深度集成       │ 可集成但独立        │
└──────────────┴────────────────┴────────────────────┘
```

### 7.3 Rime 协议栈

```text
Rime 是专为低功耗无线（802.15.4）设计的协议栈：

应用层
├── collect    ← 数据汇聚（多跳收集到 sink）
├── broadcast  ← 广播
├── unicast    ← 单播（有确认）
├── multihop   ← 多跳路由
└── runicast   ← 可靠单播（重传机制）
        ↓
    stunicast  ← 有时间戳的单播
        ↓
    framer     ← 帧格式化
        ↓
    MAC layer  ← CSMA / TDMA
        ↓
    Radio driver（CC2420 / CC2538 等）
```

------

## Topic 8：Coffee 文件系统

### 8.1 设计目标

```text
Flash 的物理特性约束：
├── 写之前必须先擦除（擦除单位：sector，通常 4KB~64KB）
├── 擦除次数有限（NOR Flash ~10万次，NAND ~10万次/块）
├── 可以按字节/页读，但擦除是大块操作
└── 随机写 = 读-改-擦-写，代价极高

Coffee FS 的应对策略：
├── 日志结构（Log-structured）：只追加写，不随机写
├── 垃圾回收：定期整理，合并碎片
├── 磨损均衡：均匀分布擦除操作
└── 无动态内存：所有元数据静态分配
```

### 8.2 文件布局

```text
Flash 物理布局：

┌──────────┬──────────┬──────────┬──────────┬──────────┐
│  Sector0 │  Sector1 │  Sector2 │  Sector3 │  Sector4 │
│ [Header] │ [Data  ] │ [Data  ] │ [Log   ] │ [Header] │
│ file_A   │ file_A   │ file_A   │ file_A   │ file_B   │
└──────────┴──────────┴──────────┴──────────┴──────────┘

每个文件由：
├── Header sector：文件名、大小、标志
├── Data sectors：原始数据
└── Log sector：修改记录（追加写，不改原始数据）
```

### 8.3 Coffee FS API

```c
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

// 写文件
int fd = cfs_open("sensor.log", CFS_WRITE);
if (fd >= 0) {
    cfs_write(fd, &sensor_data, sizeof(sensor_data));
    cfs_close(fd);
}

// 读文件
fd = cfs_open("sensor.log", CFS_READ);
if (fd >= 0) {
    cfs_read(fd, &buf, sizeof(buf));
    cfs_close(fd);
}

// 追加写（日志场景）
fd = cfs_open("sensor.log", CFS_WRITE | CFS_APPEND);
cfs_write(fd, &new_entry, sizeof(new_entry));
cfs_close(fd);

// 文件信息
struct cfs_dirent dirent;
struct cfs_dir dir;
cfs_opendir(&dir, "/");
while(cfs_readdir(&dir, &dirent) == 0) {
    printf("file: %s\n", dirent.name);
}
cfs_closedir(&dir);
```

> 与你之前设计的 NVM 模拟机制对比：Coffee FS 多了文件名索引和垃圾回收，你的方案更轻量但功能更单一——两者的 tradeoff 取决于存储空间和访问模式。

------

## Topic 9：平台移植

### 9.1 必须实现的 HAL 接口

```c
// 1. 时钟（最关键）
// platform/xxx/clock.c

void clock_init(void);
clock_time_t clock_time(void);      // 返回当前 tick 数
unsigned long clock_seconds(void);  // 返回秒数

#define CLOCK_SECOND  (某个平台相关的 tick 频率)
// 例如：定时器 100Hz → CLOCK_SECOND = 100

// 2. rtimer（如果需要高精度定时）
void rtimer_arch_init(void);
void rtimer_arch_schedule(rtimer_clock_t t);  // 在 t 时刻触发中断
rtimer_clock_t rtimer_arch_now(void);

// 3. 看门狗
void watchdog_init(void);
void watchdog_start(void);
void watchdog_periodic(void);   // 喂狗
void watchdog_stop(void);

// 4. 低功耗
// 在 main 循环的 process_run() 返回 0 时调用
void LPM_enter(void);   // 平台相关的休眠
```

### 9.2 移植最小系统

```c
// 最小移植：只需要 clock + process

// main.c
int main(void) {
    // 1. 硬件初始化
    SystemInit();
    clock_init();       // 你实现

    // 2. Contiki 内核初始化
    process_init();
    etimer_init();

    // 3. 启动 etimer 系统进程（必须）
    process_start(&etimer_process, NULL);

    // 4. 启动你的应用进程
    process_start(&my_app_process, NULL);

    // 5. 主循环
    while(1) {
        if (process_run() == 0) {
            // 无待处理事件，休眠
            __WFI();
        }
    }
}

// clock.c（STM32 示例）
static volatile clock_time_t current_tick = 0;

void SysTick_Handler(void) {
    current_tick++;
    // 通知 etimer 系统检查到期定时器
    if (etimer_pending()) {
        etimer_request_poll();  // 设置 poll 标志，主循环处理
    }
}

clock_time_t clock_time(void) {
    return current_tick;
}
```

------

## Topic 10：与 RTOS 对比

### 10.1 选型决策树

```text
你的系统需要什么？
│
├─ 需要硬实时响应（us 级）？
│   └─ YES → RTOS（FreeRTOS + 高优先级任务）
│              或 Contiki rtimer（中断级）
│
├─ RAM < 4KB？
│   └─ YES → Contiki（或裸机状态机）
│
├─ 需要移植现有 POSIX 代码？
│   └─ YES → RTOS + lwIP Socket API
│
├─ 主要是事件驱动（传感器采集、网络收发）？
│   └─ YES → Contiki 天然适合
│
└─ 团队熟悉 RTOS？
    └─ YES → FreeRTOS（学习成本更低）
```

### 10.2 核心 tradeoff 总结

```text
┌──────────────────┬─────────────────────┬─────────────────────┐
│                  │ Contiki             │ FreeRTOS            │
├──────────────────┼─────────────────────┼─────────────────────┤
│ 调度模型         │ 协作式事件驱动      │ 抢占式优先级        │
│ 最小 RAM         │ ~2KB                │ ~5KB+               │
│ 每任务额外开销   │ ~20 字节            │ ~500B（栈）         │
│ 共享资源保护     │ 无需锁（单线程语义）│ 需要互斥锁/信号量   │
│ 实时性           │ 软实时              │ 硬实时              │
│ 调试难度         │ 高（PT 栈帧问题）   │ 中（标准调试工具）  │
│ 生态             │ 小众                │ 主流                │
└──────────────────┴─────────────────────┴─────────────────────┘
```

------

## 学习路径建议

```text
第 1 步：运行 native 平台（Linux 上仿真）
  → 不需要硬件，直接 make TARGET=native
  → 先跑通 hello-world 和 blink 示例

第 2 步：深读 process.c + etimer.c
  → 理解事件队列和调度器的完整实现
  → 约 500 行代码，是整个系统的核心

第 3 步：移植到你的 MCU
  → 只需实现 clock.c（SysTick）
  → 验证 etimer 精度

第 4 步：结合 BMS 场景实践
  → 用 Contiki 进程重构你的电池加热使能逻辑
  → 用 ctimer 替代手写的软件定时器
  → 用 Coffee FS 替代你的 NVM 模拟机制（对比评估）
```

------

想从哪个 Topic 继续深挖？或者直接看 **process.c 源码逐行解析**？





```
关键洞察：整个进程状态只有 struct pt（本质是一个 uint16_t 的行号），这就是 Contiki 内存极省的根本原因。只能限制 uint16_t 麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好问题，这触及了 Protothread 底层实现的核心。

------

## `lc`（Local Continuation）的类型不是固定的

`struct pt` 里存的不是直接的 `uint16_t`，而是一个 `lc_t`，它的具体类型取决于你选择哪种 LC 实现。

------

## Protothread 的两套 LC 实现

### 实现一：`lc-switch.h`（默认，用 `__LINE__`）

```c
// include/pt/lc-switch.h

typedef unsigned short lc_t;   // ← 这里才是 uint16_t

#define LC_INIT(s)   s = 0
#define LC_RESUME(s) switch(s) { case 0:
#define LC_SET(s)    s = __LINE__; case __LINE__:
#define LC_END(s)    }
// struct pt 就是包了一层
struct pt {
    lc_t lc;   // unsigned short，存 __LINE__ 行号
};
```

**限制来源**：`__LINE__` 是编译器内置宏，值是当前行号，类型是 `int`，但存入 `unsigned short` 时：

```text
unsigned short 最大值 = 65535
→ 单个 .c 文件不能超过 65535 行
→ 实际上完全够用
```

------

### 实现二：`lc-addrlabels.h`（GCC 扩展，用 `&&label`）

```c
// include/pt/lc-addrlabels.h

typedef void * lc_t;   // ← 这里是指针，不是 uint16_t！

#define LC_INIT(s)   s = NULL
#define LC_RESUME(s) do { if(s != NULL) goto *s; } while(0)
#define LC_SET(s)    do { __label__ r;            \
                          s = &&r;                \
                          r:; } while(0)
#define LC_END(s)
struct pt {
    lc_t lc;   // void*，存 GCC label 地址
};
```

**原理**：GCC 的 `&&label` 扩展可以取标签的地址，`goto *ptr` 可以跳转到任意地址。

```c
// 展开示意
PT_THREAD(my_thread(struct pt *pt)) {
    PT_BEGIN(pt);
    // LC_RESUME 展开：
    // if (pt->lc != NULL) goto *(pt->lc);

    do_something();

    // LC_SET 展开：
    // __label__ r;
    // pt->lc = &&r;   ← 存的是 r 标签的实际内存地址
    // r:;             ← 下次直接 goto 到这里
    PT_YIELD(pt);

    do_something_else();

    PT_END(pt);
}
```

------

## 两种实现的对比

```text
┌─────────────────┬──────────────────────┬──────────────────────┐
│                 │ lc-switch.h          │ lc-addrlabels.h      │
├─────────────────┼──────────────────────┼──────────────────────┤
│ lc_t 类型       │ unsigned short       │ void *               │
│ 大小            │ 2 字节               │ 4 字节（32位MCU）    │
│                 │                      │ 8 字节（64位）       │
│ 编译器依赖      │ 标准 C（可移植）     │ GCC 专有扩展         │
│ 跳转机制        │ switch-case          │ goto *ptr            │
│ 嵌套 switch     │ ⚠️ 有限制            │ ✅ 无限制            │
│ 性能            │ 略低（switch 跳转表）│ 略高（直接跳转）     │
│ 调试友好度      │ 高（行号可读）       │ 低（地址不直观）     │
└─────────────────┴──────────────────────┴──────────────────────┘
```

------

## `lc-switch.h` 的嵌套 switch 限制

这是 switch 实现最著名的坑：

```c
PT_THREAD(bad_thread(struct pt *pt)) {
    PT_BEGIN(pt);
    // PT_BEGIN 展开了一个 switch(pt->lc) {

    switch (some_value) {   // ← ⚠️ 嵌套 switch！
        case 1:
            PT_YIELD(pt);   // ← LC_SET 展开 case __LINE__:
                            //   这个 case 属于内层 switch
                            //   而不是外层 PT 的 switch！
            break;
    }

    PT_END(pt);
    // } ← 外层 switch 结束
}
```

**结果**：`PT_YIELD` 生成的 `case __LINE__:` 被内层 switch 捕获，PT 恢复时跳转错误，行为未定义。

**规避方法**：

```c
// 方法一：用 if-else 替代 switch
// 方法二：把含 switch 的逻辑提取到子函数
// 方法三：换用 lc-addrlabels.h（goto * 无此限制）
```

------

## 能不能自定义 `lc_t` 类型

完全可以，自己写第三种实现：

```c
// 自定义 lc-uint8.h（极限省内存，文件 < 255 行）
typedef uint8_t lc_t;

#define LC_INIT(s)   s = 0
#define LC_RESUME(s) switch(s) { case 0:
#define LC_SET(s)    s = (uint8_t)__LINE__; case __LINE__:
#define LC_END(s)    }

// struct pt 只占 1 字节！
// 代价：单文件不能超过 255 行（PT 函数内）
// 自定义 lc-uint32.h（超大文件）
typedef uint32_t lc_t;
// struct pt 占 4 字节，但支持 40 亿行（没有任何实际意义）
```

------

## 一句话总结

```text
lc_t 的类型完全由你选择的 LC 实现决定：

lc-switch.h      → unsigned short（2字节，标准C，有嵌套switch限制）
lc-addrlabels.h  → void*（4/8字节，GCC专有，无限制）
自定义            → 任意整数类型，按需裁剪

"uint16_t 行号" 只是默认实现的特例，
本质是：用最小的状态量保存"下次从哪里继续执行"。
```





```
可以直接在 PC 上执行麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

可以，而且非常方便。Contiki-NG（现代维护版本）原生支持在 PC 上直接运行。

------

## 两个版本的区分

```text
Contiki（原版，Adam Dunkels）
    → 已停止维护，最后 release 2017 年
    → 仍可运行，但依赖较老

Contiki-NG（Next Generation，社区维护）
    → 2017 年从原版 fork，持续活跃维护
    → 推荐使用这个
    → GitHub: contiki-ng/contiki-ng
```

------

## 在 PC 上运行的两种方式

### 方式一：native 平台（最简单）

直接编译成 Linux 可执行文件，在本机运行：

```bash
# 1. 克隆仓库
git clone https://github.com/contiki-ng/contiki-ng.git
cd contiki-ng
git submodule update --init --recursive

# 2. 安装依赖（Ubuntu/Debian）
sudo apt-get install build-essential python3 python3-pip

# 3. 编译 hello-world（native 平台）
cd examples/hello-world
make TARGET=native

# 4. 直接运行
./hello-world.native
```

输出：

```text
Contiki-NG starting
Hello, world!
```

------

### 方式二：Docker 容器（推荐，环境隔离）

Contiki-NG 官方提供完整的 Docker 镜像，包含所有工具链：

```bash
# 拉取官方镜像
docker pull contiker/contiki-ng

# 进入容器，挂载源码目录
docker run --privileged -it \
    -v $(pwd)/contiki-ng:/home/user/contiki-ng \
    contiker/contiki-ng

# 容器内编译运行
cd examples/hello-world
make TARGET=native
./hello-world.native
```

------

## native 平台的工作原理

这是最值得理解的部分：

```text
native 平台 = Contiki 内核 + Linux 进程模拟

┌─────────────────────────────────────────┐
│           你的 Contiki 应用代码          │
├─────────────────────────────────────────┤
│           Contiki-NG 内核               │
│   process.c / etimer.c / ...            │
├─────────────────────────────────────────┤
│         native platform HAL             │
│  ┌──────────────┬───────────────────┐   │
│  │ clock → 用   │ UART → 用         │   │
│  │ gettimeofday │ printf/stdin      │   │
│  │              │                   │   │
│  │ 网络 → 用    │ 低功耗 → 用       │   │
│  │ Linux TUN/   │ select() 阻塞     │   │
│  │ TAP 虚拟网卡 │ 替代 WFI          │   │
│  └──────────────┴───────────────────┘   │
├─────────────────────────────────────────┤
│              Linux OS                   │
└─────────────────────────────────────────┘
```

关键实现细节：

```c
// platform/native/clock.c
// clock_time() 直接用 Linux gettimeofday
clock_time_t clock_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (clock_time_t)(tv.tv_sec * CLOCK_SECOND
                        + tv.tv_usec * CLOCK_SECOND / 1000000);
}

// platform/native/main.c
// 主循环里用 select() 替代 WFI
while(1) {
    int pending = process_run();
    if (pending == 0) {
        // 不是 __WFI()，而是带超时的 select()
        // 等待下一个 etimer 到期或 fd 可读
        struct timeval timeout = next_etimer_time();
        select(max_fd + 1, &fds, NULL, NULL, &timeout);
    }
}
```

------

## native 平台能做什么

### ① 网络功能（TUN/TAP 虚拟网卡）

```bash
# 创建虚拟网络接口
sudo ip tuntap add dev tun0 mode tun
sudo ip addr add 192.168.1.1/24 dev tun0
sudo ip link set tun0 up

# 运行带网络的 Contiki 示例
cd examples/coap/coap-example-server
make TARGET=native
sudo ./coap-example-server.native

# 另一个终端，用 coap-client 访问
coap-client -m get coap://192.168.1.2/
```

Contiki 进程里的 `uip_send()` 最终写入 TUN 设备，Linux 网络栈收到真实数据包。

### ② 多个 native 实例组网

```bash
# 终端 1：运行节点 A
./node.native --id=1

# 终端 2：运行节点 B
./node.native --id=2

# 两个进程通过 Unix socket 或 TUN 互相通信
# 模拟真实的多节点无线传感器网络
```

### ③ GDB 调试

```bash
# native 就是普通 Linux 可执行文件，直接 GDB
gdb ./hello-world.native

(gdb) break process_run
(gdb) run
(gdb) bt        # 查看调用栈
(gdb) p process_list  # 查看进程链表
```

------

## 实际上手步骤（最快路径）

```bash
# Step 1：克隆
git clone https://github.com/contiki-ng/contiki-ng.git
cd contiki-ng
git submodule update --init --recursive

# Step 2：跑最简单的例子
cd examples/hello-world
make TARGET=native && ./hello-world.native

# Step 3：跑定时器例子（验证 etimer）
cd ../timers
make TARGET=native && ./timers.native

# Step 4：改代码，加自己的进程
# 直接编辑 hello-world.c，重新 make，秒级反馈

# Step 5：用 GDB 单步跟踪 process_run()
gdb ./hello-world.native
(gdb) break process_run
(gdb) run
```

------

## 一句话总结

> native 平台把所有硬件依赖（时钟、网络、串口、低功耗）替换成等价的 Linux 系统调用，Contiki 内核代码**一行不改**直接在 PC 上跑。这是学习 Contiki 内核机制最高效的方式——改代码、加断点、看 process 链表，完全不需要硬件。

要不要直接从 `process_run()` 源码开始单步跟踪，把调度器走一遍？

参考资料





```
Contiki 与 Contiki-NG 区别
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

## Contiki vs Contiki-NG 全面对比

------

### 历史脉络

```text
2000  Adam Dunkels 开始开发 uIP
2002  Contiki OS 诞生（瑞典计算机科学研究所 SICS）
2005  Protothreads 发布，集成进 Contiki
2012  Contiki 2.x → 3.x，加入 6LoWPAN / RPL
2017  Contiki 最后一个 release（3.0），原版停止维护
      ↓
      社区 fork → Contiki-NG（Next Generation）
      主导机构：Inria（法国）+ RISE（瑞典）
2017~今  Contiki-NG 持续活跃维护
```

------

### 核心差异总览

```text
┌─────────────────────┬──────────────────────┬──────────────────────┐
│                     │ Contiki（原版）       │ Contiki-NG           │
├─────────────────────┼──────────────────────┼──────────────────────┤
│ 维护状态            │ ❌ 停止维护（2017）   │ ✅ 活跃维护          │
│ 最新版本            │ 3.0（2017）          │ 4.9+（2024持续更新） │
│ 代码质量            │ 历史包袱重           │ 大规模重构清理       │
│ 网络协议栈          │ uIP（IPv4/IPv6混合） │ 纯 IPv6 为核心       │
│ MAC 层              │ ContikiMAC           │ TSCH（主推）         │
│ 路由协议            │ RPL（基础）          │ RPL（增强）          │
│ 安全机制            │ 基本无               │ DTLS / LLSEC 完整    │
│ 代码规范            │ 宽松                 │ 强制 CI 检查         │
│ 测试体系            │ 几乎没有             │ 完整自动化测试       │
│ 支持平台            │ 多但质量参差         │ 精简但质量高         │
│ Coffee FS           │ ✅ 有                │ ❌ 移除              │
│ Rime 协议栈         │ ✅ 有                │ ❌ 移除              │
│ 动态模块加载        │ ✅ 有（ELF loader）  │ ❌ 移除              │
└─────────────────────┴──────────────────────┴──────────────────────┘
```

------

### 1. 内核层：基本一致，但 NG 做了清理

**进程模型、Protothreads、事件系统**——这部分两者几乎相同，NG 主要是代码规范化：

```c
// Contiki 原版：命名混乱，风格不统一
void tcpip_input(void) { ... }
void UIP_APPCALL(void) { ... }   // 全大写，风格混乱

// Contiki-NG：统一命名规范，全部小写+下划线
void tcpip_input(void) { ... }
void coap_engine_process(...) { ... }
```

NG 强制执行：

```text
- 所有代码通过 uncrustify 格式化检查
- 每个 PR 必须通过 CI（编译 + 单元测试 + 模拟测试）
- 禁止使用 goto（除 PT 内部）
- 头文件必须有 include guard
```

------

### 2. 网络层：最大的分歧点

#### Contiki 原版：uIP 双栈

```text
Contiki 网络架构：

应用层
├── HTTP server
├── CoAP
└── ...
        ↓
uIP（同时支持 IPv4 + IPv6，但实现较简陋）
        ↓
6LoWPAN（可选）
        ↓
MAC：ContikiMAC / nullmac / CSMA
        ↓
Rime（自研无线协议栈，非标准）
        ↓
Radio driver
```

#### Contiki-NG：纯 IPv6 + 标准协议栈

```text
Contiki-NG 网络架构：

应用层
├── CoAP（Erbium → 重写为 Contiki-NG CoAP）
├── LWM2M
├── MQTT（新增）
└── ...
        ↓
uIP（仅保留 IPv6，IPv4 大幅削减）
        ↓
6LoWPAN（增强，符合 RFC 4944 / RFC 6282）
        ↓
6TiSCH（新增，IEEE 802.15.4e TSCH 调度）
        ↓
MAC：TSCH（主推）/ CSMA
        ↓
Radio driver（统一抽象接口）

❌ Rime 完全移除（非标准，阻碍互操作性）
```

**为什么移除 Rime？**

```text
Rime 是 Adam Dunkels 自研的无线协议栈：
- 不基于任何标准（非 IEEE / IETF）
- 无法与其他厂商设备互操作
- 6LoWPAN + IPv6 已经能覆盖 Rime 的所有场景
- NG 的目标是标准化，Rime 与此冲突
```

------

### 3. MAC 层：ContikiMAC → TSCH

这是技术上最重要的升级：

#### ContikiMAC（原版主推）

```text
ContikiMAC = 异步低功耗侦听（Asynchronous LPL）

工作原理：
发送方：连续发送多个相同数据包（strobing）
接收方：周期性短暂唤醒侦听信道

┌──┐    ┌──┐    ┌──┐    ┌──┐
│TX│    │TX│    │TX│    │TX│   发送方连续发包
└──┘    └──┘    └──┘    └──┘
              ↑
         ┌────┐                接收方恰好在这次唤醒时收到
         │ RX │
         └────┘

问题：
- 时序不确定（接收方唤醒时机随机）
- 多跳延迟累积严重
- 信道利用率低
- 不适合工业级确定性需求
```

#### TSCH（Contiki-NG 主推）

```text
TSCH = Time Slotted Channel Hopping（IEEE 802.15.4e）

工作原理：
- 所有节点时间同步（精度 ~30us）
- 通信按时隙（slot）调度，每个时隙固定分配给特定链路
- 每个时隙在不同信道上跳频（抗干扰）

时间轴：
Slot 0    Slot 1    Slot 2    Slot 3    Slot 4
Ch 11     Ch 15     Ch 20     Ch 25     Ch 11
[A→B]     [B→C]     [C→D]     [空]      [A→B]

优势：
✅ 确定性延迟（工业级）
✅ 极低功耗（精确唤醒，无需侦听）
✅ 高可靠性（跳频抗干扰）
✅ IEEE 标准，可互操作
```

------

### 4. 安全机制：从无到有

```text
Contiki 原版：
└── 几乎没有安全机制
    └── 链路层加密：可选，实现简陋

Contiki-NG：
├── 链路层安全（LLSEC）
│   └── IEEE 802.15.4 AES-128 CCM* 加密认证
│       └── 与 TSCH 深度集成
│
└── 应用层安全
    ├── DTLS（基于 mbedTLS）
    │   └── CoAP over DTLS = CoAPs
    └── LWM2M 安全对象
        └── PSK / RPK 模式
```

------

### 5. 被 NG 移除的功能

#### ① Coffee 文件系统

```text
移除原因：
- 维护成本高，使用场景窄
- 现代 IoT 设备倾向于用外部 Flash + 专用驱动
- 与 NG 的"聚焦核心、精简边缘"策略冲突

替代方案：
- 用户自行集成 LittleFS / SPIFFS
- 或直接操作 NVM（你在 BMS 项目中的做法）
```

#### ② 动态模块加载（ELF Loader）

```text
Contiki 原版有一个极其罕见的特性：
在运行时动态加载 ELF 格式的模块（类似 Linux 的 .so）

移除原因：
- 实现复杂，安全风险大
- 现代 IoT 更倾向于 OTA 整包升级
- 维护成本远超使用价值
```

#### ③ Rime 协议栈

```text
移除原因（上文已述）：
非标准、无法互操作、IPv6 已覆盖其功能
```

------

### 6. 平台支持变化

```text
Contiki 原版支持平台（多而杂）：
AVR / MSP430 / ARM / x86 / ...
包含大量年久失修的平台代码

Contiki-NG 精简后的平台（少而精）：
├── native（Linux/macOS PC 仿真）✅
├── cc26x0-cc13x0（TI 无线 MCU）✅
├── cc2538（TI 旧款无线 MCU）✅
├── nrf52840（Nordic）✅
├── gecko（Silicon Labs）✅
├── simplelink（TI SimpleLink）✅
└── zoul（Zolertia）✅

移除了：
❌ AVR（Arduino 系列）
❌ 大量年久失修的 MSP430 变体
❌ x86
```

------

### 7. 一句话总结两者定位

```text
Contiki（原版）：
  学术研究原型，功能大而全，代码质量参差，
  已停止维护，适合研究历史设计思想。

Contiki-NG：
  工程化演进版，聚焦 IPv6 IoT 标准协议栈，
  代码质量高，有完整 CI/测试体系，
  适合实际项目使用。

内核思想（进程/事件/PT/定时器）：两者完全一致。
分歧点：网络协议栈的标准化程度 + 工程质量。
```

------

### 选哪个？

```text
学习内核设计思想（进程/事件/PT）
    → 两者都可以，原版代码更简洁，NG 更规范

实际项目开发
    → Contiki-NG，有维护、有安全、有标准协议

研究 Coffee FS / Rime / ELF 动态加载
    → 只能看 Contiki 原版

目标硬件是 TI CC26xx / Nordic nRF52
    → Contiki-NG，官方支持最好
```

参考资料





```
uIP（仅保留 IPv6，IPv4 大幅削减）为什么，现在的通讯模块不都还是 IPv4 的麽 ？│ MAC 层              │ ContikiMAC           │ TSCH（主推）         │ 是免费的麽？  动态模块加载需要 elf 加载麽，其他不能？DTLS（基于 mbedTLS） 一定得 mbedTLS 麽如果这样是不是得耗资源呢
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

四个很好的问题，逐一深挖。

------

## 问题一：为什么 NG 削减 IPv4，现在不都还是 IPv4？

### 现实情况确实如此

```text
现网设备：
├── 家用路由器      → IPv4（NAT）
├── 4G/NB-IoT 模块 → 运营商分配 IPv4（私网地址）
├── WiFi 模块       → IPv4 DHCP
└── 以太网          → IPv4 为主

IoT 设备直接暴露公网 IPv6 的比例：极低
```

### 但 Contiki-NG 的目标场景不是这些

```text
Contiki-NG 的核心场景：
低功耗无线传感器网络（IEEE 802.15.4）

┌─────────────────────────────────────────────┐
│  传感器节点（Contiki-NG）                    │
│  CC2538 / nRF52840                          │
│  802.15.4 无线                              │
└──────────────┬──────────────────────────────┘
               │ 6LoWPAN（IPv6 over 802.15.4）
               ↓
┌─────────────────────────────────────────────┐
│  边界路由器（Border Router）                 │
│  一个节点，连接传感器网络和外部网络          │
└──────────────┬──────────────────────────────┘
               │ 以太网 / WiFi（可以是 IPv4）
               ↓
          互联网 / 云平台
```

**关键点**：

```text
6LoWPAN 协议本身就是"IPv6 over 低功耗无线"
它的存在意义就是让每个传感器节点有全球唯一 IPv6 地址

如果用 IPv4：
- 802.15.4 帧最大 127 字节
- IPv4 头 20 字节 + UDP 8 字节 = 28 字节开销
- IPv4 没有 6LoWPAN 的头部压缩标准（RFC 6282 只定义了 IPv6 压缩）
- 压缩后 IPv6 头可以缩减到 2~3 字节（链路本地地址可完全省略）

所以在 802.15.4 这个场景，IPv6 + 6LoWPAN 比 IPv4 更省空间
```

### 实际工程中怎么处理 IPv4 互通

```text
传感器节点（IPv6）
    ↓ 6LoWPAN
边界路由器
    ↓ NAT64 / 代理
IPv4 服务器

NAT64：把 IPv6 地址映射到 IPv4，节点不感知
这是标准方案，不需要节点自己支持 IPv4
```

### 结论

```text
削减 IPv4 不是因为 IPv4 不重要
而是因为：
1. Contiki-NG 的目标硬件（802.15.4）天然适合 IPv6
2. 6LoWPAN 压缩只对 IPv6 有标准定义
3. IPv4 互通由边界路由器的 NAT64 解决
4. 维护两套协议栈代价太高，选择聚焦

如果你的场景是 4G 模块 + TCP/IP，
Contiki-NG 本来就不是目标选型，lwIP 更合适
```

------

## 问题二：TSCH 是免费的吗？

### 完全免费，是 IEEE 标准

```text
TSCH = Time Slotted Channel Hopping
定义在：IEEE 802.15.4e-2012（后合并进 IEEE 802.15.4-2015）

IEEE 标准本身：
├── 标准文档需要付费购买（IEEE 官网约 $100）
├── 但实现和使用完全免费，无专利壁垒
└── Contiki-NG 的 TSCH 实现是 BSD 许可证

类比：
TCP/IP 是 IETF 标准，免费使用
802.11 WiFi 是 IEEE 标准，免费使用
TSCH 同理
```

### 需要付费的是什么

```text
付费项目（与 TSCH 本身无关）：
├── 芯片：CC2538 / nRF52840 等支持 802.15.4 的芯片
│         → 正常购买硬件，无额外授权费
├── 认证：如果产品需要 Zigbee / Thread 认证
│         → Zigbee Alliance / Thread Group 会员费
│         → 但这是生态认证，不是 TSCH 本身
└── 专利：某些 TSCH 调度算法（如 6TiSCH Orchestra）
          → 学术论文实现，通常免费
          → 商业实现需确认具体专利情况
```

### ContikiMAC 也是免费的

```text
ContikiMAC：
- Adam Dunkels 设计，发表在学术论文中
- Contiki 项目内实现，BSD 许可证
- 完全免费

两者都免费，区别只在技术特性上
```

------

## 问题三：动态模块加载必须用 ELF 吗？

### 不是必须，ELF 只是 Contiki 的选择

动态加载的本质需求是：

```text
运行时把一段新代码加载到内存并执行
需要解决三个问题：
1. 代码格式：怎么描述这段代码（函数、数据、依赖）
2. 重定位：代码加载到哪个地址，内部引用怎么修正
3. 符号解析：新代码调用已有函数，怎么找到地址
```

### 各种方案对比

#### ① ELF（Contiki 原版的选择）

```c
// ELF = Executable and Linkable Format
// Linux 标准可执行文件格式

优点：
- 标准格式，工具链完整（objcopy / readelf / nm）
- 支持完整的符号表和重定位信息
- 可以直接用 GCC 编译生成

缺点：
- ELF 解析器本身需要 ~5KB ROM
- ELF 文件包含大量元数据，文件较大
- 实现复杂，安全风险（解析恶意 ELF）
```

#### ② 位置无关代码（PIC / PIE）

```c
// 编译时加 -fPIC，生成位置无关代码
// 加载到任意地址都能运行，无需重定位

// 编译
arm-none-eabi-gcc -fPIC -shared module.c -o module.bin

// 加载
void *addr = load_to_ram(module.bin);
typedef void (*init_fn)(void);
init_fn init = (init_fn)((uint8_t*)addr + entry_offset);
init();

优点：简单，不需要复杂的重定位逻辑
缺点：
- PIC 需要 GOT（全局偏移表），增加间接寻址开销
- ARM Cortex-M 的 PIC 支持有限
- 调用已有系统函数仍需符号解析
```

#### ③ 固定地址 + 函数指针表（嵌入式最常用）

```c
// 最简单的"动态加载"：约定好接口，直接跳转

// 系统侧：导出函数表
typedef struct {
    void (*uart_send)(uint8_t);
    int  (*timer_set)(uint32_t ms);
    void *(*malloc)(size_t);
} system_api_t;

const system_api_t SYSTEM_API = {
    .uart_send = hal_uart_send,
    .timer_set = sw_timer_set,
    .malloc    = NULL,  // 不提供
};

// 模块侧：约定入口和 API 表指针
typedef struct {
    uint32_t magic;          // 0xDEADBEEF，校验用
    uint32_t version;
    void (*module_init)(const system_api_t *api);
    void (*module_run)(void);
} module_header_t;

// 加载器
void load_module(uint8_t *bin) {
    module_header_t *hdr = (module_header_t *)bin;
    if (hdr->magic != 0xDEADBEEF) return;
    hdr->module_init(&SYSTEM_API);  // 注入 API 表
    hdr->module_run();
}
这本质上就是你的 BMS 项目里
bootloader 和 application 共享加密库的架构：
- 约定好函数表地址（链接脚本固定）
- 双方通过函数指针互相调用
- 不需要任何 ELF 解析
```

#### ④ 自定义二进制格式（最轻量）

```c
// 完全自定义，只包含必要信息

// 模块二进制布局：
// [4B magic] [4B size] [4B entry_offset]
// [重定位表: N × {offset, type}]
// [代码段]
// [数据段]

// 重定位表只记录需要修正的位置
// 加载时逐条处理，比 ELF 解析简单 10 倍

struct reloc_entry {
    uint16_t offset;   // 需要修正的位置（相对代码段起始）
    uint8_t  type;     // 重定位类型（绝对地址 / 相对跳转）
};
```

### 各方案资源对比

```text
┌──────────────────┬──────────┬──────────┬──────────────────────┐
│ 方案             │ ROM开销  │ 复杂度   │ 灵活性               │
├──────────────────┼──────────┼──────────┼──────────────────────┤
│ ELF loader       │ ~5KB     │ 高       │ 最高（完整符号解析） │
│ PIC + 简单加载   │ ~1KB     │ 中       │ 高                   │
│ 函数指针表       │ ~100B    │ 低       │ 低（接口固定）       │
│ 自定义格式       │ ~500B    │ 中       │ 中                   │
└──────────────────┴──────────┴──────────┴──────────────────────┘
```

### 结论

```text
ELF 不是必须的，它只是功能最完整的选择
嵌入式场景更常用：
- 函数指针表（你的 BMS bootloader 架构）
- 自定义二进制格式 + 简单重定位
- OTA 整包升级（直接替换整个 firmware，最简单）

Contiki-NG 移除 ELF loader 的原因：
复杂度和安全风险远超实际收益，
OTA 整包升级已经能满足 99% 的需求
```

------

## 问题四：DTLS 一定要 mbedTLS 吗？会不会太耗资源？

### 不是必须，但 mbedTLS 是最合适的选择

#### DTLS 的本质

```text
DTLS = Datagram TLS
TLS 运行在 TCP 上，DTLS 运行在 UDP 上
（CoAP 用 UDP，所以需要 DTLS 而不是 TLS）

DTLS 需要实现：
├── 握手协议（证书 / PSK 协商）
├── 记录层（加密 / 解密 / MAC）
├── 重传机制（UDP 不可靠，DTLS 自己处理）
└── 密码算法（AES / ECC / SHA）
```

#### 可选的 TLS/DTLS 库对比

```text
┌──────────────┬────────────┬────────────┬──────────────────────┐
│ 库           │ ROM（最小）│ RAM（最小）│ 特点                 │
├──────────────┼────────────┼────────────┼──────────────────────┤
│ mbedTLS      │ ~60KB      │ ~20KB      │ 模块化，可裁剪       │
│ wolfSSL      │ ~20KB      │ ~1KB       │ 专为嵌入式设计       │
│ TinyDTLS     │ ~10KB      │ ~5KB       │ 专为 CoAP/IoT 设计   │
│ RIOT-OS DTLS │ ~15KB      │ ~4KB       │ RIOT 生态            │
│ s2n-tls      │ ~50KB      │ 较大       │ AWS 出品，非嵌入式   │
└──────────────┴────────────┴────────────┴──────────────────────┘
```

#### Contiki-NG 实际使用的是 TinyDTLS

```text
⚠️ 纠正一个细节：
Contiki-NG 默认集成的不是完整 mbedTLS
而是 TinyDTLS（专为受限设备设计）

mbedTLS 是可选集成，用于需要完整 TLS 功能的场景
```

#### TinyDTLS 的资源占用

```c
// TinyDTLS 支持的密码套件（极简）：
TLS_PSK_WITH_AES_128_CCM_8        // PSK 预共享密钥
TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 // 证书模式

// 资源占用（Cortex-M 平台）：
ROM：~10KB（PSK 模式）/ ~25KB（证书模式）
RAM：~4KB（握手期间峰值）/ ~1KB（会话维持）

// 对比你的 BMS MCU：
// 如果是 STM32F4 系列（512KB Flash / 128KB RAM）
// TinyDTLS PSK 模式完全可以接受
```

#### mbedTLS 的裁剪能力

```c
// mbedTLS 通过 config.h 精细裁剪
// 关闭不需要的模块

// 只保留 AES-128-CCM + ECDHE + SHA256
#define MBEDTLS_AES_C
#define MBEDTLS_CCM_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_SHA256_C

// 关闭所有不需要的
#undef MBEDTLS_RSA_C          // 不用 RSA（太大）
#undef MBEDTLS_X509_CRT_PARSE_C  // 不用证书解析
#undef MBEDTLS_SSL_TLS_C      // 不用 TLS（只用 DTLS）
#undef MBEDTLS_BIGNUM_C       // 如果不用 RSA 可以关

// 裁剪后 mbedTLS 最小可以到 ~20KB ROM
// 代价：只支持 PSK 模式，无证书
```

#### 实际资源消耗分析

```text
场景：BMS 数据上云，CoAP over DTLS（PSK 模式）

使用 TinyDTLS：
├── ROM：+10KB
├── RAM：+4KB（握手峰值）/ +1KB（稳态）
└── 结论：Cortex-M3/M4 完全可接受

使用 mbedTLS（裁剪后 PSK 模式）：
├── ROM：+20~30KB
├── RAM：+8~16KB
└── 结论：128KB RAM 的 MCU 需要仔细规划

使用 wolfSSL（嵌入式优化）：
├── ROM：+20KB
├── RAM：+1~2KB（静态分配模式）
└── 结论：资源最省，但配置复杂
```

------

## 四个问题的一句话总结

```text
① IPv4 削减：
   不是 IPv4 不重要，而是 6LoWPAN 场景天然用 IPv6，
   IPv4 互通由边界路由器的 NAT64 解决。

② TSCH 免费：
   IEEE 标准，实现和使用完全免费，
   付费的是芯片和可选的生态认证。

③ 动态加载不必须 ELF：
   ELF 只是最完整的方案，
   函数指针表（你的 bootloader 架构）是嵌入式最常用的轻量替代。

④ DTLS 不必须 mbedTLS：
   Contiki-NG 默认用 TinyDTLS（~10KB ROM），
   mbedTLS 可裁剪，wolfSSL 更省 RAM，
   PSK 模式是嵌入式 DTLS 的标准选择。
```





```
6LoWPAN 中国普及麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

直接给你一个诚实的答案。

------

## 6LoWPAN 在中国的真实处境

### 一句话结论

```text
6LoWPAN 作为独立技术名词在中国几乎没有普及
但它作为底层技术，藏在 Thread / Matter 里正在快速增长
```

------

### 中国 IoT 无线协议真实市场格局

```text
┌─────────────────┬──────────┬────────────────────────────┐
│ 协议            │ 市场份额 │ 主要场景                   │
├─────────────────┼──────────┼────────────────────────────┤
│ WiFi            │ 极高     │ 智能家居、消费电子          │
│ 蓝牙 / BLE      │ 极高     │ 穿戴、音频、近场配网        │
│ Zigbee          │ 高       │ 智能家居（小米/涂鸦生态）   │
│ NB-IoT          │ 高       │ 水表/电表/停车（运营商推）  │
│ LoRa / LoRaWAN  │ 中       │ 工业、农业、城市感知        │
│ Thread / Matter │ 快速增长 │ 智能家居新品（苹果/谷歌推） │
│ 6LoWPAN（裸用） │ 极低     │ 学术研究、工业小众场景      │
└─────────────────┴──────────┴────────────────────────────┘
```

------

### 为什么 6LoWPAN 没有单独普及

#### 原因一：Zigbee 先入为主，生态太强

```text
Zigbee 和 6LoWPAN 都基于 IEEE 802.15.4 物理层
但 Zigbee 更早成熟，国内生态已经固化：

小米：Zigbee 网关 + 大量 Zigbee 子设备
涂鸦：Zigbee 方案大量出货给中小厂商
海尔/美的：私有 Zigbee 方案

这些厂商没有动力换 6LoWPAN
换了还要重新认证、重新开发 App
```

#### 原因二：NB-IoT 抢占了广域 IoT 场景

```text
6LoWPAN 的优势是低功耗 + IPv6 直连
但中国运营商强推 NB-IoT：

- 中国移动 / 电信 / 联通 大力补贴 NB-IoT 模块
- 水表、气表、停车、路灯全部上 NB-IoT
- 政府项目要求用运营商网络（可管可控）

6LoWPAN 是自组网，不经过运营商
→ 在政府主导的场景里天然处于劣势
```

#### 原因三：没有一个强势的中国本土推手

```text
Thread / 6LoWPAN 的主要推手：
Apple（HomeKit）/ Google（Nest）/ Amazon（Echo）

这些公司在中国影响力有限
国内对应的是：
小米（Zigbee）/ 华为（HiLink，私有）/ 阿里（飞燕，WiFi为主）

没有巨头推，生态起不来
```

------

### 但是：Thread = 6LoWPAN 的工程化封装

```text
Thread 协议栈：

应用层：CoAP / Matter
        ↓
传输层：UDP
        ↓
网络层：IPv6
        ↓
适配层：6LoWPAN  ← 就是这个
        ↓
MAC层：IEEE 802.15.4 TSCH / CSMA
        ↓
物理层：IEEE 802.15.4 2.4GHz

用了 Thread = 用了 6LoWPAN
只是用户和开发者感知到的是 Thread，不是 6LoWPAN
```

### Thread / Matter 在中国的增长

```text
2022：Matter 1.0 发布，Apple / Google / Amazon 联合推动
2023：小米宣布支持 Matter
2024：涂鸦推出 Matter 模组，大量中国厂商跟进
2025：国内智能家居新品大量标注 Matter 认证

→ 6LoWPAN 正在通过 Thread/Matter 渗透中国市场
  只是消费者和大多数开发者不知道底层是 6LoWPAN
```

------

### 对你的 BMS 项目有没有参考价值

```text
BMS 的通信场景：

当前主流方案：
├── CAN / SMBus（车载/工业，有线）
├── UART + 私有协议（设备内部）
├── WiFi / 4G（远程监控上云）
└── BLE（手机近场调试）

6LoWPAN / Thread 的潜在价值：
├── 电池柜 / 储能系统：多个 BMS 节点组网上报
│   → 有线布线成本高，6LoWPAN 自组网有优势
├── 工厂产线：多节点低功耗数据采集
│   → TSCH 的确定性调度适合工业场景
└── 当前实际采用率：极低

结论：
技术上有价值，但中国工业 BMS 场景
目前还是 CAN / RS485 / 4G 的天下
6LoWPAN 进入这个场景至少还需要 3~5 年
```

------

### 总结

```text
问题：6LoWPAN 在中国普及吗？

直接回答：
作为独立技术 → 没有普及，市场极小众
作为 Thread 底层 → 随 Matter 生态正在增长，但慢

根本原因：
├── Zigbee 生态先入为主（小米/涂鸦）
├── NB-IoT 被运营商强推占据广域场景
├── 国内没有强势推手（Apple/Google 影响力有限）
└── 中国厂商更倾向私有协议或成熟生态

对嵌入式工程师的意义：
学 6LoWPAN / Thread 的底层原理有价值
但短期内在中国做产品，Zigbee / NB-IoT / WiFi
仍然是更务实的选择
```

参考资料





```
当前常用的是 4G,WIFI,BLE 配合 MCU的 IOT，怎么选择呢
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好问题，直接从工程实践角度切入。

------

## IoT 通信方案选型框架

### 先问四个决定性问题

```text
1. 设备在哪里？          → 室内 / 室外 / 移动
2. 数据量多大？          → 字节级 / KB级 / MB级
3. 实时性要求？          → 秒级 / 分钟级 / 小时级
4. 功耗预算？            → 电池供电 / 持续供电
```

------

## 三种方案的本质定位

```text
┌─────────┬──────────────────────────────────────────────────┐
│ 4G      │ 随时随地，不依赖任何基础设施，数据直达云端        │
│ WiFi    │ 高带宽，依赖路由器，适合固定场所                  │
│ BLE     │ 极低功耗，近场，手机是天然网关                    │
└─────────┴──────────────────────────────────────────────────┘
```

------

## 详细对比

```text
┌──────────────┬──────────────┬──────────────┬──────────────┐
│              │ 4G（含NB）   │ WiFi         │ BLE          │
├──────────────┼──────────────┼──────────────┼──────────────┤
│ 覆盖范围     │ 全国         │ ~50m室内     │ ~10m         │
│ 数据速率     │ 1Mbps~100M   │ 10M~1Gbps    │ 125K~2Mbps   │
│ 典型延迟     │ 50~200ms     │ 1~50ms       │ 3~30ms       │
│ 待机功耗     │ 高(~10mA)    │ 中(~1mA)     │ 极低(~2uA)   │
│ 发送功耗     │ 峰值~500mA   │ 峰值~300mA   │ 峰值~15mA    │
│ 月通信成本   │ 流量费~5~30元│ 宽带分摊     │ 免费         │
│ 需要基础设施 │ 运营商网络   │ 路由器       │ 手机/网关    │
│ 直连云端     │ ✅ 原生支持  │ ✅ 需路由器  │ ❌ 需手机/GW │
│ 模块成本     │ ~30~80元     │ ~5~20元      │ ~2~10元      │
│ SIM卡/认证   │ 需要         │ 不需要       │ 不需要       │
└──────────────┴──────────────┴──────────────┴──────────────┘
```

------

## 场景决策树

```text
设备需要独立联网（无路由器/手机）？
│
├─ YES → 4G / NB-IoT
│         │
│         ├─ 数据量大 / 实时视频 → 4G Cat-1 / Cat-4
│         ├─ 数据量小 / 低频上报 → NB-IoT（更省电省钱）
│         └─ 移动设备（车载/物流）→ 4G Cat-1
│
└─ NO → 有固定WiFi环境？
          │
          ├─ YES → 数据量大 / 需要OTA升级 / 固定插电
          │         → WiFi（ESP32 / W5500 + MCU）
          │
          └─ NO → 手机是主要交互入口？
                    │
                    ├─ YES → BLE
                    │         ├─ 只需近场配置 → BLE 单独
                    │         └─ 需要远程 → BLE + 手机转发
                    │
                    └─ 混合场景 → 组合方案（见下）
```

------

## 组合方案（实际项目最常见）

### 组合一：BLE + WiFi（消费电子标配）

```text
典型产品：智能音箱、扫地机器人、智能插座

┌─────────────────────────────────────┐
│  MCU（ESP32，内置双模）              │
│  ├─ BLE：手机配网（首次连接）        │
│  │       手机直接控制（近场）        │
│  └─ WiFi：连接路由器后上云           │
│            OTA 固件升级              │
│            远程控制（云→设备）       │
└─────────────────────────────────────┘

配网流程：
手机 BLE → 把 WiFi 密码发给设备 → 设备连 WiFi → 上云
之后 BLE 可以关掉省电，或保持用于近场快速响应
```

### 组合二：BLE + 4G（户外 + 手机交互）

```text
典型产品：共享单车锁、资产追踪器、工业手持设备

┌─────────────────────────────────────┐
│  主 MCU                              │
│  ├─ BLE 模块：手机近场解锁/配置      │
│  └─ 4G 模块：GPS位置 + 状态上云      │
│              远程指令下发            │
└─────────────────────────────────────┘

功耗策略：
4G 模块平时关闭，定时唤醒上报（类似你的无线模块唤醒场景）
BLE 常开低功耗广播，等待手机连接
```

### 组合三：BLE + NB-IoT（电池供电 + 长期部署）

```text
典型产品：智能水表、环境传感器、停车地磁

┌─────────────────────────────────────┐
│  超低功耗 MCU（STM32L4 / nRF9160）  │
│  ├─ BLE：现场运维人员配置参数        │
│  │       不需要时完全关闭            │
│  └─ NB-IoT：每小时上报一次数据      │
│              PSM 模式待机电流 ~3uA   │
└─────────────────────────────────────┘

电池寿命：
纽扣电池 / 2节AA → 3~5年不换电池
这是 NB-IoT 的核心价值
```

### 组合四：WiFi + BLE + 本地有线（网关类产品）

```text
典型产品：智能家居网关、工业边缘网关

┌─────────────────────────────────────┐
│  网关 MCU / Linux SBC               │
│  ├─ WiFi：上行连云                  │
│  ├─ BLE：汇聚周边 BLE 传感器        │
│  ├─ Zigbee（可选）：汇聚 Zigbee 设备│
│  └─ RS485 / CAN：连接工业设备       │
└─────────────────────────────────────┘
```

------

## 针对 BMS 场景的具体建议

结合你的工作场景：

### 场景 A：储能柜 / 工业 BMS（固定安装，有网络）

```text
推荐：WiFi 或 4G Cat-1

MCU（你的 BMS 主控）
    ↓ UART / SPI
WiFi 模块（ESP8266 / ESP32-S3）
    ↓ MQTT over TLS
云平台（阿里云 IoT / 华为云）

优势：
- 带宽足够传输完整 BMS 数据（SOC/SOH/温度/电流曲线）
- OTA 升级方便
- 调试时可以开 WebSocket 实时看波形

注意：
- 瞬态大电流问题你已经遇到过（无线模块唤醒）
- WiFi 模块发包瞬间电流峰值 ~300mA
- 需要在电源路径上加足够的去耦电容（470uF~1000uF）
- 或者让 BMS 主控提前预判，在限流阈值宽松时才允许发包
```

### 场景 B：便携式 BMS / 户外设备

```text
推荐：4G Cat-1 + BLE

4G：远程监控、告警上报
BLE：手机 App 近场调试、参数配置

模块推荐：
- 移远 EC600N（4G Cat-1，国内主流）
- 移远 BC660K（NB-IoT，超低功耗）
- Nordic nRF52840（BLE 5.0，你熟悉的平台）
```

### 场景 C：电池测试设备（实验室）

```text
推荐：WiFi（直连局域网）

原因：
- 实验室有 WiFi
- 数据量大（充放电曲线、阻抗谱）
- 需要与 PC 上位机实时交互
- 不需要考虑功耗

方案：
MCU → ESP32（AT 指令 或 直接用 ESP32 做主控）
    → TCP/UDP → PC 上位机
```

------

## 模块选型速查

```text
4G 模块（国内主流）：
├── 移远 EC600N    Cat-1，性价比高，国内出货量最大
├── 移远 EC800E    Cat-1，封装更小
├── 移远 BC660K    NB-IoT，PSM 超低功耗
└── 芯讯通 SIM7600 Cat-4，速率更高

WiFi 模块：
├── ESP8266        最便宜，只有 WiFi，AT 指令
├── ESP32          WiFi + BLE，可直接跑应用
├── ESP32-S3       更强 MCU，支持 AI 推理
└── W5500 + MCU    硬件 TCP/IP 栈，稳定性最好（工业场景）

BLE 模块：
├── nRF52840       功耗最低，BLE 5.0，你熟悉
├── nRF52832       更便宜，BLE 5.0
├── ESP32          BLE + WiFi 二合一
└── DA14531        超小封装，纽扣电池场景
```

------

## 一句话选型口诀

```text
要独立联网    → 4G
要高带宽      → WiFi
要省电        → BLE 或 NB-IoT
要手机交互    → BLE 必有
要远程 + 省电 → NB-IoT
要全能        → ESP32（WiFi + BLE 二合一）

实际项目 90% 是组合方案：
消费电子  → ESP32（WiFi + BLE）
工业设备  → 4G Cat-1 + BLE
户外抄表  → NB-IoT + BLE（运维用）
```

你们 BMS 产品目前用的是哪种通信方案？是需要远程监控还是只做本地调试接口？





```
上述的4G,WIFI,BLE 的 MCU 采用 contikit 或者 contikit-NG 开发可以麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

直接给你一个诚实的工程判断。

------

## 结论先说

```text
Contiki / Contiki-NG 用于 4G / WiFi / BLE 的 MCU 开发：

4G   → ❌ 几乎不用，没有生态
WiFi → ❌ 几乎不用，ESP32 有自己的框架
BLE  → ❌ 几乎不用，nRF SDK 更成熟

Contiki-NG 的真实定位：
✅ IEEE 802.15.4 无线（Zigbee物理层 / Thread）
✅ 学术研究 / 传感器网络
✅ 学习嵌入式OS设计思想
```

------

## 为什么不适合

### 根本原因：Contiki-NG 的网络栈是为 802.15.4 设计的

```text
Contiki-NG 网络架构：

应用层 CoAP / LWM2M
    ↓
uIP（IPv6）
    ↓
6LoWPAN          ← 专为 802.15.4 小帧设计的压缩层
    ↓
IEEE 802.15.4    ← 硬件依赖

4G / WiFi / BLE 根本不走这条路：

4G  → PPP / 网络层由模块内部处理，MCU 只发 AT 指令
WiFi→ 802.11，完全不同的 MAC 层
BLE → 完全独立的协议栈（GATT / ATT / L2CAP）
```

### 具体问题

#### 4G 场景

```text
实际开发模式：

MCU ──UART──► 4G模块（EC600N）
                │
                │ 模块内部已经有完整 TCP/IP 栈
                │ MCU 只需要发 AT 指令
                ▼
              云平台

MCU 侧需要的：
├── UART 驱动
├── AT 指令解析状态机
├── MQTT / HTTP 客户端（可选，也可以让模块处理）
└── 重连逻辑

Contiki-NG 能提供什么：
├── 进程模型 / 事件系统  → 有用，但 FreeRTOS 也能做
├── uIP 网络栈          → 完全用不上（模块内部有）
├── 6LoWPAN            → 完全用不上
└── TSCH MAC           → 完全用不上

结论：Contiki-NG 80% 的代码对 4G 场景无意义
```

#### WiFi 场景

```text
ESP32 的现实：

ESP32 = Xtensa LX6 双核 + WiFi + BLE
官方提供 ESP-IDF（基于 FreeRTOS）

ESP-IDF 已经包含：
├── FreeRTOS 内核
├── lwIP（完整 TCP/IP）
├── WiFi 驱动（闭源，乐鑫私有）
├── BLE 协议栈（NimBLE / Bluedroid）
├── MQTT / HTTP / WebSocket 客户端
├── OTA 升级框架
└── 完整的 Flash 分区管理

Contiki-NG 移植到 ESP32：
├── WiFi 驱动？→ 乐鑫不开放，无法对接 Contiki 网络层
├── BLE 栈？  → 完全独立，Contiki 没有 BLE 支持
└── 结论：移植成本极高，收益为零
```

#### BLE 场景

```text
nRF52840 的现实：

Nordic 提供两套 SDK：
├── nRF5 SDK（传统，稳定）
└── nRF Connect SDK（基于 Zephyr RTOS，新一代）

两套 SDK 都包含：
├── SoftDevice（BLE 协议栈，闭源二进制）
│   └── 占用固定 RAM / Flash 区域
├── GATT / GAP / ATT 完整实现
├── BLE Mesh 支持
└── Thread（OpenThread，基于 6LoWPAN）← 这里才有交集

Contiki-NG 在 nRF52840 上：
├── 有社区移植，但不是官方支持
├── BLE SoftDevice 和 Contiki 进程模型冲突
│   SoftDevice 有自己的事件循环，优先级最高
│   Contiki 的协作式调度会被打断
└── 结论：能跑，但不如直接用 nRF Connect SDK
```

------

## 那 Contiki-NG 在哪里真正有价值

```text
✅ 真实适用场景：

场景                    硬件                    为什么用 Contiki-NG
─────────────────────────────────────────────────────────────────
智能电表 / 气表         CC2538 / CC1310         802.15.4 + 6LoWPAN
工业传感器网络          CC2650                  TSCH 确定性调度
智慧城市路灯            nRF52840（Thread模式）  Thread = Contiki-NG 的主场
学术研究节点            TelosB / Zolertia       论文复现
Matter 设备底层         nRF52840                Thread over Matter
```

------

## 4G / WiFi / BLE 真实技术栈选型

### 4G + MCU

```text
推荐技术栈：

STM32 / GD32（主控 MCU）
├── 操作系统：FreeRTOS 或 裸机状态机
├── AT 指令层：自己写 或 用 esp-at / 移远 SDK
├── 网络层：MQTT（coreMQTT）/ HTTP（自己实现）
└── 应用层：你的 BMS 业务逻辑

为什么不用 Contiki：
FreeRTOS 生态更大，AT 指令库更多，
ST 官方支持，调试工具完善
```

### WiFi + MCU

```text
方案 A：ESP32 做主控（最省事）
├── ESP-IDF + FreeRTOS
├── WiFi / BLE 原生支持
└── 适合：数据量大，需要云连接

方案 B：STM32 + ESP8266（AT 指令）
├── STM32 跑业务逻辑（你熟悉）
├── ESP8266 纯做 WiFi 透传
└── 适合：已有 STM32 方案，加 WiFi 功能

方案 C：STM32 + W5500（硬件 TCP/IP）
├── SPI 接口，稳定性最好
├── lwIP 对接 W5500 驱动
└── 适合：工业场景，稳定性优先
```

### BLE + MCU

```text
方案 A：nRF52840 做主控（最专业）
├── nRF Connect SDK（Zephyr RTOS）
├── SoftDevice BLE 协议栈
└── 适合：BLE 是核心功能

方案 B：STM32 + HM-10 / DA14531（AT 指令）
├── STM32 跑业务，BLE 模块做透传
└── 适合：BLE 只是配置接口，不是核心

方案 C：ESP32（WiFi + BLE 二合一）
├── NimBLE 协议栈（开源，资源占用小）
└── 适合：同时需要 WiFi 和 BLE
```

------

## Contiki-NG 的思想可以借鉴，但不要直接用

```text
值得从 Contiki-NG 学习并移植到你的项目的：

① 进程 + 事件模型
  → 用在 STM32 裸机上替代 FreeRTOS
  → 你已经在用 Protothread，这是同一套思想

② etimer / ctimer 三层定时器体系
  → 比 FreeRTOS 的 xTimerCreate 更轻量
  → 适合你的 BMS 多定时器场景

③ memb + list 内存管理
  → 零碎片，适合资源受限 MCU
  → 比 FreeRTOS heap 更可预测

④ 事件驱动主循环 + WFI 低功耗
  → 和你现在做的极低功耗模式完全一致

这些组件完全可以剥离出来单独使用
不需要带上整个 Contiki-NG
```

------

## 一句话总结

```text
Contiki-NG 是为 IEEE 802.15.4 无线传感器网络设计的
用在 4G / WiFi / BLE 场景 = 用锤子拧螺丝，能用但不合适

正确姿势：
├── 4G / WiFi / BLE 项目  → FreeRTOS / Zephyr / 裸机
├── 802.15.4 / Thread 项目 → Contiki-NG 是首选
└── 学习 OS 设计思想       → Contiki-NG 源码是极佳教材
    把 PT / 事件模型 / 定时器体系移植到你的 BMS 项目
    这才是对你最有价值的部分
```





```
就是想借鉴 Contiki-NG 实现低功耗框架去控制 4G,WIFI,BLE 
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

这个思路非常清晰，而且完全可行。本质上是把 Contiki-NG 的**调度内核**剥离出来，网络层完全换掉。

------

## 架构定位

```text
你要做的事：

Contiki-NG 原版：
┌─────────────────────────────────┐
│  CoAP / LWM2M 应用              │
├─────────────────────────────────┤
│  uIP + 6LoWPAN + TSCH           │  ← 不要这些
├─────────────────────────────────┤
│  进程 / 事件 / PT / 定时器       │  ← 只要这些
├─────────────────────────────────┤
│  Platform HAL                   │
└─────────────────────────────────┘

你的目标框架：
┌─────────────────────────────────┐
│  BMS 业务逻辑                   │
├──────────┬──────────┬───────────┤
│ 4G 驱动  │ WiFi 驱动│ BLE 驱动  │  ← 你自己写
├──────────┴──────────┴───────────┤
│  进程 / 事件 / PT / 定时器       │  ← 从 Contiki-NG 剥离
├─────────────────────────────────┤
│  STM32 / nRF HAL                │
└─────────────────────────────────┘
```

------

## 第一步：剥离最小内核

需要的文件只有这几个：

```text
从 Contiki-NG 源码取出：

core/sys/
├── process.c / process.h     ← 进程调度器（核心）
├── etimer.c  / etimer.h      ← 事件定时器
├── ctimer.c  / ctimer.h      ← 回调定时器
├── rtimer.c  / rtimer.h      ← 实时定时器（可选）
├── pt.h                      ← Protothread
├── lc.h / lc-switch.h        ← PT 底层
└── clock.h                   ← 时钟抽象接口（只有头文件）

core/lib/
├── list.c / list.h           ← 侵入式链表
└── memb.c / memb.h           ← 内存池

需要你自己实现：
platform/your-mcu/
└── clock.c                   ← 对接 SysTick，唯一必须实现的 HAL
```

总计：**约 1500 行 C 代码**，没有任何外部依赖。

------

## 第二步：实现 clock.c（唯一必须的 HAL）

```c
// platform/stm32/clock.c

#include "sys/clock.h"
#include "sys/etimer.h"

static volatile clock_time_t tick_count = 0;

// SysTick 中断，100Hz（CLOCK_SECOND = 100）
void SysTick_Handler(void) {
    tick_count++;

    // 通知 etimer 系统检查到期定时器
    if (etimer_pending()) {
        etimer_request_poll();  // 设置 poll 标志
    }
}

clock_time_t clock_time(void) {
    return tick_count;
}

unsigned long clock_seconds(void) {
    return tick_count / CLOCK_SECOND;
}

void clock_init(void) {
    // STM32 SysTick 配置，10ms 一次
    SysTick_Config(SystemCoreClock / CLOCK_SECOND);
}
// platform/stm32/clock.h（或放在 contiki-conf.h）

#define CLOCK_SECOND    100U          // 100 tick/s = 10ms 精度
typedef uint32_t        clock_time_t;
typedef uint32_t        rtimer_clock_t;
```

------

## 第三步：设计通信模块驱动接口

三种通信模块统一抽象：

```c
// comm/comm_driver.h

typedef enum {
    COMM_EVT_CONNECTED,      // 连接建立
    COMM_EVT_DISCONNECTED,   // 连接断开
    COMM_EVT_DATA_RECV,      // 收到数据
    COMM_EVT_SEND_DONE,      // 发送完成
    COMM_EVT_ERROR,          // 错误
} comm_event_t;

typedef struct {
    comm_event_t  event;
    uint8_t      *data;
    uint16_t      len;
    int           error_code;
} comm_evt_data_t;

// 统一驱动接口
typedef struct comm_driver {
    const char *name;
    int  (*init)   (void);
    int  (*connect)(const char *target);   // IP:port 或 BLE addr
    int  (*send)   (const uint8_t *buf, uint16_t len);
    void (*disconnect)(void);
    void (*power_on) (void);
    void (*power_off)(void);
    int  (*get_signal_strength)(void);     // RSSI
} comm_driver_t;

// 三个驱动实例
extern const comm_driver_t comm_4g_driver;
extern const comm_driver_t comm_wifi_driver;
extern const comm_driver_t comm_ble_driver;
```

------

## 第四步：4G 驱动进程实现

```c
// comm/comm_4g.c
// 用 Protothread 实现 AT 指令状态机，彻底消灭嵌套回调

#include "sys/process.h"
#include "sys/etimer.h"
#include "sys/pt.h"

// ── 内部事件 ──────────────────────────────────────────
static process_event_t evt_at_resp;    // AT 响应到达
static process_event_t evt_4g_req;    // 上层请求

// AT 响应缓冲（中断填充）
static volatile char   at_resp_buf[256];
static volatile bool   at_resp_ready = false;

// ── UART 中断（只做最简单的事）────────────────────────
void USART1_IRQHandler(void) {
    static uint16_t idx = 0;
    char c = USART1->DR;

    at_resp_buf[idx++] = c;

    // 检测到行结束
    if (c == '\n' || idx >= sizeof(at_resp_buf) - 1) {
        at_resp_buf[idx] = '\0';
        idx = 0;
        at_resp_ready = true;
        process_poll(&comm_4g_process);  // 通知进程，中断安全
    }
}

// ── AT 指令子 PT：发送并等待响应 ──────────────────────
static PT_THREAD(at_send_wait(struct pt *pt,
                               const char *cmd,
                               const char *expect,
                               uint32_t    timeout_ms,
                               bool       *ok))
{
    static struct etimer at_timer;

    PT_BEGIN(pt);

    // 发送 AT 指令
    at_resp_ready = false;
    uart_send_string(cmd);
    uart_send_string("\r\n");

    // 启动超时定时器
    etimer_set(&at_timer, timeout_ms * CLOCK_SECOND / 1000);

    // 等待响应或超时
    PT_WAIT_UNTIL(pt, at_resp_ready || etimer_expired(&at_timer));

    if (etimer_expired(&at_timer)) {
        *ok = false;
    } else {
        *ok = (strstr((char*)at_resp_buf, expect) != NULL);
    }

    PT_END(pt);
}

// ── 4G 主进程 ─────────────────────────────────────────
PROCESS(comm_4g_process, "4G Driver");

PROCESS_THREAD(comm_4g_process, ev, data) {
    static struct pt     at_pt;
    static struct etimer retry_timer;
    static bool          at_ok;
    static uint8_t       retry_count;

    PROCESS_BEGIN();

    evt_at_resp = process_alloc_event();
    evt_4g_req  = process_alloc_event();

    while (1) {

        /* ── 阶段1：模块上电 ── */
        ldo_4g_enable(true);
        etimer_set(&retry_timer, 3 * CLOCK_SECOND);  // 等待模块启动
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));

        /* ── 阶段2：AT 握手 ── */
        retry_count = 0;
        do {
            PT_INIT(&at_pt);
            PROCESS_PT_SPAWN(&at_pt,
                at_send_wait(&at_pt, "AT", "OK", 1000, &at_ok));
            retry_count++;
        } while (!at_ok && retry_count < 5);

        if (!at_ok) {
            // 模块无响应，断电重试
            ldo_4g_enable(false);
            etimer_set(&retry_timer, 10 * CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
            continue;
        }

        /* ── 阶段3：配置 APN ── */
        PT_INIT(&at_pt);
        PROCESS_PT_SPAWN(&at_pt,
            at_send_wait(&at_pt,
                         "AT+CGDCONT=1,\"IP\",\"cmnet\"",
                         "OK", 3000, &at_ok));

        /* ── 阶段4：等待注网 ── */
        retry_count = 0;
        do {
            PT_INIT(&at_pt);
            PROCESS_PT_SPAWN(&at_pt,
                at_send_wait(&at_pt, "AT+CREG?", "+CREG: 0,1",
                             2000, &at_ok));
            if (!at_ok) {
                etimer_set(&retry_timer, 2 * CLOCK_SECOND);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
            }
            retry_count++;
        } while (!at_ok && retry_count < 30);  // 最多等 60 秒

        if (!at_ok) {
            // 注网失败，通知上层
            process_post(PROCESS_BROADCAST,
                         comm_event_disconnected, NULL);
            continue;
        }

        /* ── 阶段5：建立 MQTT 连接 ── */
        PT_INIT(&at_pt);
        PROCESS_PT_SPAWN(&at_pt,
            at_send_wait(&at_pt,
                         "AT+QMTOPEN=0,\"mqtt.example.com\",1883",
                         "+QMTOPEN: 0,0", 10000, &at_ok));

        // 通知上层连接成功
        process_post(PROCESS_BROADCAST,
                     comm_event_connected, NULL);

        /* ── 阶段6：正常运行，等待发送请求或断线 ── */
        while (1) {
            PROCESS_WAIT_EVENT();

            if (ev == comm_send_request_event) {
                // 上层请求发数据
                comm_send_req_t *req = (comm_send_req_t *)data;
                // 构造 AT+QMTPUB 指令发送
                at_4g_publish(req->topic, req->payload, req->len);

            } else if (ev == PROCESS_EVENT_POLL) {
                // UART 中断触发，检查是否有断线 URC
                if (strstr((char*)at_resp_buf, "+QMTSTAT")) {
                    // MQTT 断线，跳出内层循环重连
                    break;
                }
            }
        }

        // 重连前等待
        etimer_set(&retry_timer, 5 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }

    PROCESS_END();
}
```

------

## 第五步：低功耗控制进程

这是整个框架的核心价值所在：

```c
// power/power_manager.c

typedef enum {
    PWR_MODE_ACTIVE,      // 全速运行
    PWR_MODE_IDLE,        // CPU 停，外设开
    PWR_MODE_SLEEP,       // 关闭非必要外设
    PWR_MODE_DEEP_SLEEP,  // 只保留 RTC + 唤醒源
} power_mode_t;

// 唤醒源注册表
typedef struct wakeup_source {
    struct wakeup_source *next;       // list 侵入式链表
    const char           *name;
    bool                (*is_active)(void);  // 是否需要保持唤醒
    uint32_t              min_active_ms;     // 最短活跃时间
} wakeup_source_t;

LIST(wakeup_sources);

// 注册唤醒源
void power_register_wakeup_source(wakeup_source_t *src) {
    list_add(wakeup_sources, src);
}

// ── 低功耗管理进程 ────────────────────────────────────
PROCESS(power_manager_process, "Power Manager");

PROCESS_THREAD(power_manager_process, ev, data) {
    static struct etimer eval_timer;
    static power_mode_t  current_mode;
    static wakeup_source_t *src;

    PROCESS_BEGIN();

    current_mode = PWR_MODE_ACTIVE;

    while (1) {
        // 每 500ms 评估一次功耗状态
        etimer_set(&eval_timer, CLOCK_SECOND / 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&eval_timer));

        // 检查所有唤醒源
        bool any_active = false;
        for (src = list_head(wakeup_sources);
             src != NULL;
             src = list_item_next(src)) {
            if (src->is_active()) {
                any_active = true;
                break;
            }
        }

        if (any_active) {
            if (current_mode != PWR_MODE_ACTIVE) {
                power_set_mode(PWR_MODE_ACTIVE);
                current_mode = PWR_MODE_ACTIVE;
            }
        } else {
            // 没有活跃唤醒源，进入低功耗
            power_mode_t target = evaluate_target_mode();
            if (target != current_mode) {
                power_set_mode(target);
                current_mode = target;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第六步：通信模块唤醒源注册

```c
// 4G 模块的唤醒源
static bool is_4g_active(void) {
    return (comm_4g_state == COMM_STATE_CONNECTING ||
            comm_4g_state == COMM_STATE_SENDING   ||
            bms_has_pending_upload());
}

static wakeup_source_t wakeup_4g = {
    .name          = "4G",
    .is_active     = is_4g_active,
    .min_active_ms = 500,
};

// BLE 模块的唤醒源
static bool is_ble_active(void) {
    return ble_is_connected() || ble_adv_requested();
}

static wakeup_source_t wakeup_ble = {
    .name          = "BLE",
    .is_active     = is_ble_active,
    .min_active_ms = 100,
};

// 在 main 里注册
void comm_init(void) {
    power_register_wakeup_source(&wakeup_4g);
    power_register_wakeup_source(&wakeup_ble);
    power_register_wakeup_source(&wakeup_wifi);
}
```

------

## 第七步：主循环（解决你之前遇到的瞬态大电流问题）

```c
// main.c

int main(void) {
    clock_init();
    process_init();
    etimer_init();

    // 启动系统进程
    process_start(&etimer_process,       NULL);
    process_start(&power_manager_process,NULL);

    // 启动通信进程
    process_start(&comm_4g_process,      NULL);
    process_start(&comm_ble_process,     NULL);
    process_start(&comm_wifi_process,    NULL);

    // 启动 BMS 业务进程
    process_start(&bms_monitor_process,  NULL);
    process_start(&bms_charge_process,   NULL);

    while (1) {
        int pending = process_run();

        if (pending == 0) {
            // ── 关键：进入低功耗前的检查 ──────────────
            // 解决你遇到的"无线模块唤醒瞬态大电流"问题：
            // 在 WFI 前确认输入限流余量足够

            if (power_input_current_headroom() > COMM_WAKEUP_CURRENT_MA) {
                // 允许通信模块唤醒
                comm_allow_wakeup(true);
            } else {
                // 输入限流紧张，推迟通信模块唤醒
                comm_allow_wakeup(false);
            }

            // 进入低功耗，等待中断
            __WFI();
        }
    }
}
```

------

## 整体框架结构

```text
your_project/
├── contiki-core/          ← 从 Contiki-NG 剥离，不改一行
│   ├── sys/
│   │   ├── process.c/h
│   │   ├── etimer.c/h
│   │   ├── ctimer.c/h
│   │   ├── pt.h
│   │   └── clock.h
│   └── lib/
│       ├── list.c/h
│       └── memb.c/h
│
├── platform/              ← 你实现，只有 clock.c
│   └── stm32/clock.c
│
├── comm/                  ← 通信驱动，用 PT 写状态机
│   ├── comm_driver.h      ← 统一接口
│   ├── comm_4g.c          ← AT 指令状态机
│   ├── comm_wifi.c        ← ESP8266 AT 或 ESP-IDF
│   └── comm_ble.c         ← HM-10 AT 或 nRF UART
│
├── power/                 ← 低功耗管理
│   ├── power_manager.c    ← 唤醒源评估进程
│   └── power_hal.c        ← STM32 Stop/Standby 模式
│
└── app/                   ← BMS 业务逻辑
    ├── bms_monitor.c      ← 电池监控进程
    ├── bms_charge.c       ← 充电管理进程
    └── bms_upload.c       ← 数据上云进程
```

------

## 这套框架解决你现有的哪些问题

```text
你之前遇到的问题          框架如何解决
──────────────────────────────────────────────────────────
无线模块唤醒瞬态大电流    power_manager 在 WFI 前检查限流余量
  → 输入限流故障          comm_allow_wakeup() 推迟唤醒

AT 指令嵌套回调难维护     PT_SPAWN 子 Protothread
  → 状态机爆炸            线性代码写异步流程

多模块并发（4G+BLE同时）  独立进程，事件驱动
  → 互相干扰              单线程语义，无需互斥锁

充电状态机与通信状态机    进程间事件通信
  → 耦合严重              process_post() 解耦

低功耗模式下外设唤醒      etimer_request_poll() 中断安全
  → 时序问题              主循环统一处理
```

------

## 移植工作量评估

```text
工作项                        工作量
──────────────────────────────────────
剥离 contiki-core             半天（复制文件，调 include 路径）
实现 clock.c                  2小时（SysTick 对接）
编译验证（native 先跑通）      1天
4G AT 指令进程                 3~5天（含调试）
BLE AT 指令进程                2~3天
WiFi AT 指令进程               2~3天
power_manager 进程             2天
BMS 业务进程迁移               视现有代码量

总计：约 2~3 周
收益：AT 状态机可维护性大幅提升 + 低功耗框架统一管理
```

想先从哪个模块开始？建议从 `clock.c` + `hello-world` 进程在你的 STM32 上跑通，再逐步加通信驱动。





```
因为 4G,WIFI,BLE 都是可以通过 UART 的 AT，所以需要一个 AT 的框架，同时支持多个 AT Client 像  RTTHREAD 的 AT 那样
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好，这个需求非常清晰。先看 RT-Thread AT 组件的设计，再基于 Contiki-NG 的进程模型重新实现一套。

------

## 先分析 RT-Thread AT 组件的核心设计

```text
RT-Thread AT 组件结构：

┌─────────────────────────────────────────┐
│  at_client_exec_cmd()                   │  用户 API
│  at_client_send() / at_client_recv()    │
├─────────────────────────────────────────┤
│  AT Client                              │
│  ├── 发送队列                           │
│  ├── 响应解析（OK / ERROR / URC）       │
│  ├── URC 回调表                         │
│  └── 超时管理                           │
├─────────────────────────────────────────┤
│  UART 抽象层（rt_device）               │
└─────────────────────────────────────────┘

支持多实例：
at_client_t client_4g  = at_client_init("uart1", 512);
at_client_t client_ble = at_client_init("uart2", 256);
at_client_t client_wifi= at_client_init("uart3", 256);
```

RT-Thread 的实现依赖：

- 线程（每个 client 一个接收线程）
- 信号量（等待响应）
- 互斥锁（保护发送）
- 消息队列（URC 分发）

**我们要做的：把这些全部换成 Contiki-NG 的进程 + 事件 + PT。**

------

## 整体架构设计

```text
┌──────────────────────────────────────────────────────┐
│                  用户进程                             │
│  comm_4g_process / comm_ble_process / comm_wifi_proc │
│                                                      │
│  PROCESS_PT_SPAWN(&pt,                               │
│      at_exec(&at_pt, client, "AT+CSQ", "OK", 1000)) │
└──────────────────┬───────────────────────────────────┘
                   │ PT_SPAWN（阻塞式调用，无回调）
┌──────────────────▼───────────────────────────────────┐
│              AT Framework                             │
│                                                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │  AT Client  │  │  AT Client  │  │  AT Client  │  │
│  │   (4G)      │  │   (WiFi)    │  │   (BLE)     │  │
│  │  uart1      │  │  uart2      │  │  uart3      │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         │ 共享同一套框架代码，多实例        │         │
└─────────┼────────────────┼────────────────┼─────────┘
          │                │                │
       UART1            UART2            UART3
       (4G模块)         (WiFi模块)       (BLE模块)
```

------

## 核心数据结构

```c
// at/at_client.h

#ifndef AT_CLIENT_H
#define AT_CLIENT_H

#include "sys/process.h"
#include "sys/etimer.h"
#include "sys/pt.h"
#include "lib/list.h"
#include "lib/memb.h"

// ── 配置 ──────────────────────────────────────────────
#define AT_MAX_CLIENTS        4      // 最多同时几个 client
#define AT_RESP_BUF_SIZE      256    // 每个 client 的接收缓冲
#define AT_URC_TABLE_SIZE     16     // 每个 client 最多注册几个 URC
#define AT_LINE_MAX_LEN       128    // 单行最大长度

// ── AT 响应结果 ────────────────────────────────────────
typedef enum {
    AT_RESULT_OK = 0,
    AT_RESULT_ERROR,
    AT_RESULT_TIMEOUT,
    AT_RESULT_BUSY,
} at_result_t;

// ── URC（主动上报）回调 ────────────────────────────────
typedef void (*at_urc_cb_t)(const char *urc_line, void *arg);

typedef struct {
    const char  *prefix;      // URC 前缀，如 "+QMTSTAT"
    at_urc_cb_t  callback;
    void        *arg;
} at_urc_entry_t;

// ── AT Client 实例 ─────────────────────────────────────
typedef struct {
    // 标识
    const char     *name;              // "4G" / "WiFi" / "BLE"

    // UART 接口（平台相关，函数指针注入）
    void  (*uart_send)(const uint8_t *buf, uint16_t len);
    void  (*uart_recv_start)(void);    // 使能接收中断

    // 接收缓冲（由中断填充）
    volatile char    rx_buf[AT_RESP_BUF_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile bool     line_ready;      // 有完整行待处理

    // 当前响应上下文（exec 期间有效）
    const char      *expect_ok;        // 期望的成功响应，如 "OK"
    const char      *expect_err;       // 期望的错误响应，如 "ERROR"
    char             last_line[AT_LINE_MAX_LEN]; // 最后收到的行
    at_result_t      last_result;

    // URC 表
    at_urc_entry_t   urc_table[AT_URC_TABLE_SIZE];
    uint8_t          urc_count;

    // 状态
    bool             busy;             // 是否正在执行命令
    struct process  *owner_process;    // 当前占用的进程

} at_client_t;

// ── 全局事件（框架内部使用）──────────────────────────
extern process_event_t at_line_received_event;  // 收到完整行

#endif
```

------

## AT 框架核心实现

```c
// at/at_client.c

#include "at_client.h"
#include <string.h>
#include <stdio.h>

process_event_t at_line_received_event;

// ── 初始化 ─────────────────────────────────────────────
void at_client_init(at_client_t *client) {
    memset(client->rx_buf,   0, sizeof(client->rx_buf));
    memset(client->urc_table,0, sizeof(client->urc_table));
    client->rx_head    = 0;
    client->rx_tail    = 0;
    client->line_ready = false;
    client->busy       = false;
    client->urc_count  = 0;
    client->owner_process = NULL;

    // 分配全局事件（只需一次）
    static bool event_allocated = false;
    if (!event_allocated) {
        at_line_received_event = process_alloc_event();
        event_allocated = true;
    }

    client->uart_recv_start();
}

// ── 注册 URC ───────────────────────────────────────────
void at_client_register_urc(at_client_t    *client,
                             const char     *prefix,
                             at_urc_cb_t     callback,
                             void           *arg)
{
    if (client->urc_count >= AT_URC_TABLE_SIZE) return;
    at_urc_entry_t *e = &client->urc_table[client->urc_count++];
    e->prefix   = prefix;
    e->callback = callback;
    e->arg      = arg;
}

// ── 中断调用：接收一个字节 ────────────────────────────
// 在 UART ISR 中调用此函数
void at_client_isr_recv_byte(at_client_t *client, char c) {
    uint16_t next = (client->rx_head + 1) % AT_RESP_BUF_SIZE;
    if (next == client->rx_tail) return;  // 缓冲满，丢弃

    client->rx_buf[client->rx_head] = c;
    client->rx_head = next;

    // 检测到行结束
    if (c == '\n') {
        client->line_ready = true;
        // 通知 AT 进程（中断安全）
        process_poll(&at_dispatcher_process);
    }
}

// ── 从环形缓冲取一行 ───────────────────────────────────
static bool at_client_get_line(at_client_t *client, char *out, uint16_t max) {
    uint16_t i = 0;

    while (client->rx_tail != client->rx_head && i < max - 1) {
        char c = client->rx_buf[client->rx_tail];
        client->rx_tail = (client->rx_tail + 1) % AT_RESP_BUF_SIZE;

        if (c == '\r') continue;   // 跳过 \r
        out[i++] = c;
        if (c == '\n') break;      // 行结束
    }

    out[i] = '\0';

    // 去掉末尾 \n
    if (i > 0 && out[i-1] == '\n') out[--i] = '\0';

    // 检查是否还有更多行
    client->line_ready = (client->rx_tail != client->rx_head);

    return i > 0;
}

// ── URC 匹配与分发 ─────────────────────────────────────
static bool at_client_dispatch_urc(at_client_t *client,
                                    const char  *line)
{
    for (uint8_t i = 0; i < client->urc_count; i++) {
        if (strncmp(line,
                    client->urc_table[i].prefix,
                    strlen(client->urc_table[i].prefix)) == 0)
        {
            client->urc_table[i].callback(line,
                                          client->urc_table[i].arg);
            return true;
        }
    }
    return false;
}
```

------

## AT 调度进程（框架心脏）

```c
// at/at_dispatcher.c

// 所有 AT client 注册到这里
static at_client_t *client_registry[AT_MAX_CLIENTS];
static uint8_t      client_count = 0;

void at_register_client(at_client_t *client) {
    if (client_count < AT_MAX_CLIENTS) {
        client_registry[client_count++] = client;
    }
}

// ── AT 调度进程：统一处理所有 client 的接收 ───────────
PROCESS(at_dispatcher_process, "AT Dispatcher");

PROCESS_THREAD(at_dispatcher_process, ev, data) {
    static char line_buf[AT_LINE_MAX_LEN];

    PROCESS_BEGIN();

    while (1) {
        // 等待任意 client 有数据（由 ISR 通过 process_poll 触发）
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        // 遍历所有 client，处理有数据的
        for (uint8_t i = 0; i < client_count; i++) {
            at_client_t *c = client_registry[i];

            while (c->line_ready) {
                if (!at_client_get_line(c, line_buf, sizeof(line_buf))) {
                    break;
                }

                // 空行跳过
                if (strlen(line_buf) == 0) continue;

                // 保存最后一行（exec 用来判断结果）
                strncpy(c->last_line, line_buf, AT_LINE_MAX_LEN - 1);

                // ① 先尝试 URC 匹配（无论是否 busy 都要处理）
                bool is_urc = at_client_dispatch_urc(c, line_buf);

                // ② 如果当前有命令在等待响应
                if (c->busy && !is_urc) {
                    bool matched_ok  = c->expect_ok
                        && strstr(line_buf, c->expect_ok);
                    bool matched_err = c->expect_err
                        && strstr(line_buf, c->expect_err);

                    if (matched_ok) {
                        c->last_result = AT_RESULT_OK;
                        // 通知等待的进程
                        process_post(c->owner_process,
                                     at_line_received_event,
                                     c);
                    } else if (matched_err) {
                        c->last_result = AT_RESULT_ERROR;
                        process_post(c->owner_process,
                                     at_line_received_event,
                                     c);
                    }
                    // 中间行（如 +CSQ: 12,0）：
                    // 保存在 last_line，等 OK 后上层读取
                }
            }
        }
    }

    PROCESS_END();
}
```

------

## 用户 API：PT 子线程（最关键的部分）

```c
// at/at_client.h 追加

// ── 核心 API：在 PT 上下文中执行一条 AT 指令 ──────────
//
// 用法：
//   static struct at_req req;
//   PROCESS_PT_SPAWN(&sub_pt,
//       at_exec(&sub_pt, &client_4g, &req,
//               "AT+CSQ", "+CSQ", "OK", "ERROR", 2000));
//   if (req.result == AT_RESULT_OK) { ... }

typedef struct {
    at_result_t  result;
    char         response[AT_LINE_MAX_LEN]; // 中间行内容
} at_req_t;

PT_THREAD(at_exec(struct pt     *pt,
                  at_client_t   *client,
                  at_req_t      *req,
                  const char    *cmd,        // AT 指令
                  const char    *data_prefix,// 中间行前缀，NULL=不关心
                  const char    *expect_ok,  // 成功标志
                  const char    *expect_err, // 失败标志
                  uint32_t       timeout_ms));

// ── 便捷宏 ────────────────────────────────────────────
// 只关心 OK/ERROR，不需要中间行
#define AT_EXEC_SIMPLE(pt, client, req, cmd, timeout_ms) \
    at_exec(pt, client, req, cmd, NULL, "OK", "ERROR", timeout_ms)
// at/at_client.c 追加

PT_THREAD(at_exec(struct pt     *pt,
                  at_client_t   *client,
                  at_req_t      *req,
                  const char    *cmd,
                  const char    *data_prefix,
                  const char    *expect_ok,
                  const char    *expect_err,
                  uint32_t       timeout_ms))
{
    static struct etimer timeout_timer;

    PT_BEGIN(pt);

    // 等待 client 空闲
    PT_WAIT_UNTIL(pt, !client->busy);

    // 占用 client
    client->busy          = true;
    client->expect_ok     = expect_ok;
    client->expect_err    = expect_err;
    client->owner_process = PROCESS_CURRENT();
    req->response[0]      = '\0';
    req->result           = AT_RESULT_TIMEOUT;

    // 发送指令
    client->uart_send((uint8_t *)cmd, strlen(cmd));
    client->uart_send((uint8_t *)"\r\n", 2);

    // 启动超时定时器
    etimer_set(&timeout_timer, timeout_ms * CLOCK_SECOND / 1000);

    // 等待响应或超时
    PT_WAIT_UNTIL(pt,
        (process_get_last_event() == at_line_received_event &&
         process_get_last_event_data() == client)
        || etimer_expired(&timeout_timer));

    if (!etimer_expired(&timeout_timer)) {
        // 有响应，结果已由 dispatcher 写入 client->last_result
        req->result = client->last_result;

        // 如果需要中间行数据，从 last_line 提取
        if (data_prefix && req->result == AT_RESULT_OK) {
            // last_line 可能是中间行，也可能是 OK
            // 实际实现中需要一个小缓冲保存中间行
            strncpy(req->response,
                    client->last_line,
                    AT_LINE_MAX_LEN - 1);
        }
    }

    // 释放 client
    client->busy          = false;
    client->expect_ok     = NULL;
    client->expect_err    = NULL;
    client->owner_process = NULL;
    etimer_stop(&timeout_timer);

    PT_END(pt);
}
```

------

## 实际使用示例

### 4G 进程使用 AT 框架

```c
// comm/comm_4g.c

// 声明 client 实例
at_client_t client_4g;

// UART1 发送函数（平台相关）
static void uart1_send(const uint8_t *buf, uint16_t len) {
    HAL_UART_Transmit(&huart1, buf, len, 100);
}
static void uart1_recv_start(void) {
    HAL_UART_Receive_IT(&huart1, ...);
}

// UART1 中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1) {
        at_client_isr_recv_byte(&client_4g, rx_byte);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

// URC 回调：MQTT 断线
static void urc_mqtt_stat(const char *line, void *arg) {
    // 在进程上下文中执行（由 dispatcher 调用）
    process_post(&comm_4g_process,
                 evt_4g_disconnected, NULL);
}

PROCESS(comm_4g_process, "4G");

PROCESS_THREAD(comm_4g_process, ev, data) {
    static struct pt     sub_pt;
    static at_req_t      req;
    static struct etimer retry_timer;
    static char          cmd_buf[64];
    static int           signal_strength;

    PROCESS_BEGIN();

    // 初始化 client
    client_4g.name            = "4G";
    client_4g.uart_send       = uart1_send;
    client_4g.uart_recv_start = uart1_recv_start;
    at_client_init(&client_4g);
    at_register_client(&client_4g);

    // 注册 URC
    at_client_register_urc(&client_4g,
                            "+QMTSTAT",
                            urc_mqtt_stat, NULL);
    at_client_register_urc(&client_4g,
                            "+QMTRECV",
                            urc_mqtt_recv, NULL);

    while (1) {

        /* ── Step 1: AT 握手 ── */
        PT_INIT(&sub_pt);
        PROCESS_PT_SPAWN(&sub_pt,
            AT_EXEC_SIMPLE(&sub_pt, &client_4g, &req,
                           "AT", 1000));

        if (req.result != AT_RESULT_OK) {
            LOG_ERR("4G: no response\n");
            etimer_set(&retry_timer, 5 * CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
            continue;
        }

        /* ── Step 2: 查询信号强度 ── */
        PT_INIT(&sub_pt);
        PROCESS_PT_SPAWN(&sub_pt,
            at_exec(&sub_pt, &client_4g, &req,
                    "AT+CSQ", "+CSQ", "OK", "ERROR", 2000));

        if (req.result == AT_RESULT_OK) {
            // req.response = "+CSQ: 18,0"
            sscanf(req.response, "+CSQ: %d", &signal_strength);
            LOG_INF("4G signal: %d\n", signal_strength);
        }

        /* ── Step 3: 配置 APN ── */
        PT_INIT(&sub_pt);
        PROCESS_PT_SPAWN(&sub_pt,
            AT_EXEC_SIMPLE(&sub_pt, &client_4g, &req,
                           "AT+CGDCONT=1,\"IP\",\"cmnet\"", 3000));

        /* ── Step 4: 等待注网（循环查询）── */
        uint8_t retry = 0;
        do {
            PT_INIT(&sub_pt);
            PROCESS_PT_SPAWN(&sub_pt,
                at_exec(&sub_pt, &client_4g, &req,
                        "AT+CREG?", "+CREG",
                        "OK", "ERROR", 2000));

            if (req.result == AT_RESULT_OK &&
                strstr(req.response, "0,1")) {
                break;  // 注网成功
            }

            etimer_set(&retry_timer, 2 * CLOCK_SECOND);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
        } while (++retry < 30);

        /* ── Step 5: 建立 MQTT 连接 ── */
        snprintf(cmd_buf, sizeof(cmd_buf),
                 "AT+QMTOPEN=0,\"%s\",%d",
                 MQTT_HOST, MQTT_PORT);
        PT_INIT(&sub_pt);
        PROCESS_PT_SPAWN(&sub_pt,
            at_exec(&sub_pt, &client_4g, &req,
                    cmd_buf, "+QMTOPEN",
                    "+QMTOPEN: 0,0", "ERROR", 10000));

        if (req.result != AT_RESULT_OK) continue;

        // 通知上层连接成功
        process_post(PROCESS_BROADCAST,
                     comm_event_connected, &client_4g);

        /* ── Step 6: 运行态，等待发送请求或断线 ── */
        while (1) {
            PROCESS_WAIT_EVENT();

            if (ev == comm_send_request_event) {
                comm_send_req_t *r = data;
                snprintf(cmd_buf, sizeof(cmd_buf),
                         "AT+QMTPUB=0,0,0,0,\"%s\"", r->topic);
                PT_INIT(&sub_pt);
                PROCESS_PT_SPAWN(&sub_pt,
                    at_exec(&sub_pt, &client_4g, &req,
                            cmd_buf, NULL,
                            ">", "ERROR", 3000));
                if (req.result == AT_RESULT_OK) {
                    // 收到 > 提示符，发送 payload
                    client_4g.uart_send(r->payload, r->len);
                    uint8_t ctrl_z = 0x1A;
                    client_4g.uart_send(&ctrl_z, 1);
                }

            } else if (ev == evt_4g_disconnected) {
                // URC 触发的断线事件
                break;
            }
        }

        // 重连等待
        etimer_set(&retry_timer, 5 * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }

    PROCESS_END();
}
```

### BLE 进程（复用同一套框架）

```c
// comm/comm_ble.c

at_client_t client_ble;

static void uart2_send(const uint8_t *buf, uint16_t len) {
    HAL_UART_Transmit(&huart2, buf, len, 100);
}

// URC：BLE 连接状态变化
static void urc_ble_conn(const char *line, void *arg) {
    if (strstr(line, "+CONNECTED")) {
        process_post(&comm_ble_process, evt_ble_connected, NULL);
    } else if (strstr(line, "+DISCONNECTED")) {
        process_post(&comm_ble_process, evt_ble_disconnected, NULL);
    }
}

PROCESS(comm_ble_process, "BLE");

PROCESS_THREAD(comm_ble_process, ev, data) {
    static struct pt  sub_pt;
    static at_req_t   req;

    PROCESS_BEGIN();

    client_ble.name            = "BLE";
    client_ble.uart_send       = uart2_send;
    client_ble.uart_recv_start = uart2_recv_start;
    at_client_init(&client_ble);
    at_register_client(&client_ble);

    at_client_register_urc(&client_ble, "+CONNECTED",
                            urc_ble_conn, NULL);
    at_client_register_urc(&client_ble, "+DISCONNECTED",
                            urc_ble_conn, NULL);

    /* 设置广播名 */
    PT_INIT(&sub_pt);
    PROCESS_PT_SPAWN(&sub_pt,
        AT_EXEC_SIMPLE(&sub_pt, &client_ble, &req,
                       "AT+NAME=BMS_DEVICE", 1000));

    /* 开启广播 */
    PT_INIT(&sub_pt);
    PROCESS_PT_SPAWN(&sub_pt,
        AT_EXEC_SIMPLE(&sub_pt, &client_ble, &req,
                       "AT+ADV=1", 1000));

    while (1) {
        PROCESS_WAIT_EVENT();

        if (ev == evt_ble_connected) {
            LOG_INF("BLE connected\n");
            process_post(&bms_monitor_process,
                         evt_debug_client_connected, NULL);

        } else if (ev == comm_send_request_event) {
            comm_send_req_t *r = data;
            // BLE 透传发送
            PT_INIT(&sub_pt);
            PROCESS_PT_SPAWN(&sub_pt,
                at_exec(&sub_pt, &client_ble, &req,
                        (char*)r->payload, NULL,
                        "OK", "ERROR", 1000));
        }
    }

    PROCESS_END();
}
```

------

## 框架文件结构

```text
at_framework/
├── at_client.h          ← 数据结构 + API 声明
├── at_client.c          ← init / isr_recv / get_line / dispatch_urc
├── at_dispatcher.c      ← PROCESS(at_dispatcher_process)
│
platform/stm32/
├── clock.c              ← SysTick
├── uart1.c              ← 4G UART，调用 at_client_isr_recv_byte
├── uart2.c              ← BLE UART
└── uart3.c              ← WiFi UART

comm/
├── comm_4g.c            ← PROCESS(comm_4g_process)
├── comm_ble.c           ← PROCESS(comm_ble_process)
└── comm_wifi.c          ← PROCESS(comm_wifi_process)
```

------

## 与 RT-Thread AT 组件的对比

```text
┌─────────────────┬──────────────────────┬──────────────────────┐
│                 │ RT-Thread AT         │ 本框架               │
├─────────────────┼──────────────────────┼──────────────────────┤
│ 多实例          │ ✅ 支持              │ ✅ 支持              │
│ 等待响应机制    │ 信号量阻塞线程       │ PT_WAIT 挂起进程     │
│ URC 处理        │ 独立线程轮询         │ dispatcher 进程统一  │
│ 超时            │ rt_sem_take timeout  │ etimer               │
│ 内存占用        │ 每client一个线程栈   │ 共享栈，极省内存     │
│ 线程安全        │ 互斥锁              │ 单线程语义，无需锁   │
│ 依赖            │ RT-Thread 内核       │ 仅 Contiki-NG 内核   │
│ 代码可读性      │ 回调 / 信号量        │ 线性 PT 代码         │
└─────────────────┴──────────────────────┴──────────────────────┘
```

------

## 关键设计决策说明

```text
① 为什么用 process_poll 而不是 process_post 在中断里？
  → process_poll 是中断安全的，process_post 不是
  → poll 只设一个标志位，dispatcher 进程统一处理所有 client

② 为什么 dispatcher 是单独进程而不是每个 client 一个进程？
  → 节省进程数量
  → 所有 UART 中断都 poll 同一个进程，统一调度
  → 遍历 client_registry 找有数据的，O(n) 但 n 很小

③ at_exec 为什么用 PT_SPAWN 而不是普通函数？
  → 需要在等待响应期间挂起，让其他进程运行
  → 普通函数做不到非阻塞等待
  → PT_SPAWN 让调用方进程挂起，dispatcher 继续运行

④ busy 标志如何保证互斥？
  → 单线程语义：PT_WAIT_UNTIL(!client->busy) 保证
  → 不需要互斥锁，因为进程切换只发生在 PT_WAIT 点
  → 两个进程不会同时执行到 busy=true 这一行
```

想先把 `at_dispatcher` + `at_client` 在 native 平台上跑通验证，还是直接上 STM32 硬件调试？





```
同时也想搬运 rtthread 的 device 那一套，还有 除了 AT，还有 GNSS的原生 UART 接口，航海呼救的模块等
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

这个需求本质上是在 Contiki-NG 进程模型上构建一套**轻量级设备框架**。先把整体架构想清楚再动手。

------

## 整体架构规划

```text
┌─────────────────────────────────────────────────────────────┐
│                      应用层进程                              │
│  bms_process / gnss_process / distress_process / ...        │
├─────────────────────────────────────────────────────────────┤
│                    设备服务层                                │
│  ┌─────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │  AT 框架    │ │  GNSS 解析器 │ │  航海呼救协议解析器   │ │
│  │ (上节设计)  │ │  NMEA/UBX    │ │  DSC / AIS / EPIRB    │ │
│  └─────────────┘ └──────────────┘ └───────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    设备抽象层（Device Framework）            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │  UART    │ │   SPI    │ │   I2C    │ │    GPIO       │  │
│  │  device  │ │  device  │ │  device  │ │    device     │  │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                 Contiki-NG 内核                              │
│         process / event / PT / etimer / memb / list         │
├─────────────────────────────────────────────────────────────┤
│                    Platform HAL                              │
│              STM32 / nRF / GD32 寄存器级驱动                │
└─────────────────────────────────────────────────────────────┘
```

------

## 第一步：先对齐需求边界

在动手之前，把所有设备列清楚：

```text
你提到的设备类型：

① AT 指令类（上节已设计）
   ├── 4G 模块      (EC600N / SIM7600)   UART + AT
   ├── WiFi 模块    (ESP8266 / ESP32)     UART + AT
   └── BLE 模块     (HM-10 / DA14531)    UART + AT

② 原生 UART 流类（不是 AT，是连续数据流）
   ├── GNSS 模块    (u-blox / 中科微)    UART，输出 NMEA / UBX 帧
   └── 航海呼救模块  (DSC / AIS)         UART，输出专有协议帧

③ 可能还有（你没提但 BMS 场景常见）
   ├── 电量计       (BQ27xxx)            I2C / HDQ
   ├── 充电管理     (BQ25xxx)            I2C
   └── 传感器       (温度/压力)          I2C / SPI
```

**关键区别**：

```text
AT 类：
  MCU 主动发命令 → 模块响应
  请求-响应模型
  需要 AT 框架

原生 UART 流类：
  模块主动持续输出数据帧
  MCU 被动接收、解析
  需要流式帧解析器

两者都需要：
  底层 UART 设备抽象（共用）
```

------

## 第二步：Device Framework 设计

参考 RT-Thread device，但去掉不需要的部分：

### 2.1 设备基类

```c
// device/device.h

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "lib/list.h"

// ── 设备类型 ──────────────────────────────────────────
typedef enum {
    DEV_TYPE_UART    = 0x01,
    DEV_TYPE_SPI     = 0x02,
    DEV_TYPE_I2C     = 0x03,
    DEV_TYPE_GPIO    = 0x04,
    DEV_TYPE_VIRTUAL = 0x10,   // 虚拟设备（AT client / GNSS 等）
} dev_type_t;

// ── 设备标志 ──────────────────────────────────────────
#define DEV_FLAG_RDONLY    0x01
#define DEV_FLAG_WRONLY    0x02
#define DEV_FLAG_RDWR      0x03
#define DEV_FLAG_NONBLOCK  0x10   // 非阻塞（PT 语义下默认）

// ── 设备操作接口（虚函数表）──────────────────────────
typedef struct device dev_t;

typedef struct {
    int  (*open)    (dev_t *dev, uint8_t flags);
    int  (*close)   (dev_t *dev);
    int  (*read)    (dev_t *dev, uint32_t pos,
                        void *buf, uint32_t len);
    int  (*write)   (dev_t *dev, uint32_t pos,
                        const void *buf, uint32_t len);
    int  (*control) (dev_t *dev, int cmd, void *arg);
} dev_ops_t;

// ── 设备基类 ──────────────────────────────────────────
struct device {
    struct device   *next;          // list 侵入式链表
    const char      *name;          // 设备名 "uart1"/"spi1"
    dev_type_t       type;
    uint8_t          flags;
    bool             opened;
    const dev_ops_t *ops;           // 操作接口
    void            *user_data;     // 子类私有数据

    // 回调（数据到达通知，替代 RT-Thread 的 rx_indicate）
    void (*rx_indicate)(dev_t *dev, uint32_t len);
    void (*tx_complete)(dev_t *dev);
};

// ── 设备注册表（全局链表）────────────────────────────
void  device_register  (dev_t *dev);
dev_t *device_find     (const char *name);

// ── 统一操作 API ──────────────────────────────────────
int   device_open      (dev_t *dev, uint8_t flags);
int   device_close     (dev_t *dev);
int   device_read      (dev_t *dev, uint32_t pos,
                            void *buf, uint32_t len);
int   device_write     (dev_t *dev, uint32_t pos,
                            const void *buf, uint32_t len);
int   device_control   (dev_t *dev, int cmd, void *arg);

// ── 控制命令（cmd）────────────────────────────────────
#define DEV_CTRL_SET_RX_CB      0x01  // 设置接收回调
#define DEV_CTRL_SET_TX_CB      0x02  // 设置发送完成回调
#define DEV_CTRL_SET_BAUDRATE   0x10  // UART 波特率
#define DEV_CTRL_SET_POWER      0x20  // 模块电源控制
#define DEV_CTRL_GET_STATUS     0x30  // 获取设备状态

#endif
```

### 2.2 设备注册表实现

```c
// device/device.c

#include "device.h"
#include "lib/list.h"
#include <string.h>

LIST(device_list);

void device_register(dev_t *dev) {
    // 检查重名
    if (device_find(dev->name)) return;
    list_add(device_list, dev);
}

dev_t *device_find(const char *name) {
    dev_t *d;
    for (d = list_head(device_list); d; d = list_item_next(d)) {
        if (strcmp(d->name, name) == 0) return d;
    }
    return NULL;
}

int device_open(dev_t *dev, uint8_t flags) {
    if (!dev || !dev->ops->open) return -1;
    dev->flags  = flags;
    dev->opened = true;
    return dev->ops->open(dev, flags);
}

int device_close(dev_t *dev) {
    if (!dev || !dev->opened) return -1;
    dev->opened = false;
    return dev->ops->close ? dev->ops->close(dev) : 0;
}

int device_read(dev_t *dev, uint32_t pos, void *buf, uint32_t len) {
    if (!dev || !dev->opened || !dev->ops->read) return -1;
    return dev->ops->read(dev, pos, buf, len);
}

int device_write(dev_t *dev, uint32_t pos,
                 const void *buf, uint32_t len) {
    if (!dev || !dev->opened || !dev->ops->write) return -1;
    return dev->ops->write(dev, pos, buf, len);
}

int device_control(dev_t *dev, int cmd, void *arg) {
    if (!dev || !dev->ops->control) return -1;
    return dev->ops->control(dev, cmd, arg);
}
```

------

## 第三步：UART 设备实现

### 3.1 UART 设备结构（带环形缓冲）

```c
// device/uart_device.h

#include "device.h"

#define UART_RX_BUF_SIZE   256
#define UART_TX_BUF_SIZE   256

typedef struct {
    // 继承 dev_t（必须是第一个字段）
    dev_t base;

    // 平台相关句柄（STM32 是 UART_HandleTypeDef*）
    void *hal_handle;

    // 接收环形缓冲
    uint8_t  rx_buf[UART_RX_BUF_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;

    // 发送缓冲
    uint8_t  tx_buf[UART_TX_BUF_SIZE];
    uint16_t tx_len;
    bool     tx_busy;

    // 配置
    uint32_t baudrate;

} uart_dev_t;

// 中断调用入口（在 ISR 中调用）
void uart_dev_isr_rx(uart_dev_t *uart, uint8_t byte);
void uart_dev_isr_tx_done(uart_dev_t *uart);

// 注册一个 UART 设备
void uart_dev_register(uart_dev_t *uart,
                       const char *name,
                       void       *hal_handle,
                       uint32_t    baudrate);
```

### 3.2 UART 设备实现

```c
// device/uart_device.c

#include "uart_device.h"
#include "sys/process.h"
#include <string.h>

// ── ISR 接收（中断上下文）────────────────────────────
void uart_dev_isr_rx(uart_dev_t *uart, uint8_t byte) {
    uint16_t next = (uart->rx_head + 1) % UART_RX_BUF_SIZE;
    if (next == uart->rx_tail) return;  // 满了丢弃

    uart->rx_buf[uart->rx_head] = byte;
    uart->rx_head = next;

    // 通知上层（rx_indicate 由上层注册）
    if (uart->base.rx_indicate) {
        uart->base.rx_indicate(&uart->base, 1);
    }
}

void uart_dev_isr_tx_done(uart_dev_t *uart) {
    uart->tx_busy = false;
    if (uart->base.tx_complete) {
        uart->base.tx_complete(&uart->base);
    }
}

// ── ops 实现 ──────────────────────────────────────────
static int uart_open(dev_t *dev, uint8_t flags) {
    uart_dev_t *uart = (uart_dev_t *)dev;
    // 平台相关：使能 UART 中断
    platform_uart_enable_irq(uart->hal_handle);
    return 0;
}

static int uart_close(dev_t *dev) {
    uart_dev_t *uart = (uart_dev_t *)dev;
    platform_uart_disable_irq(uart->hal_handle);
    return 0;
}

static int uart_read(dev_t *dev, uint32_t pos,
                     void *buf, uint32_t len) {
    uart_dev_t *uart = (uart_dev_t *)dev;
    uint8_t    *dst  = (uint8_t *)buf;
    uint32_t    cnt  = 0;

    while (cnt < len && uart->rx_tail != uart->rx_head) {
        dst[cnt++] = uart->rx_buf[uart->rx_tail];
        uart->rx_tail = (uart->rx_tail + 1) % UART_RX_BUF_SIZE;
    }
    return cnt;
}

static int uart_write(dev_t *dev, uint32_t pos,
                      const void *buf, uint32_t len) {
    uart_dev_t *uart = (uart_dev_t *)dev;
    // 平台相关发送（DMA 或中断）
    platform_uart_send(uart->hal_handle, buf, len);
    return len;
}

static int uart_control(dev_t *dev, int cmd, void *arg) {
    uart_dev_t *uart = (uart_dev_t *)dev;
    switch (cmd) {
    case DEV_CTRL_SET_RX_CB:
        dev->rx_indicate = (void(*)(dev_t*, uint32_t))arg;
        break;
    case DEV_CTRL_SET_BAUDRATE:
        uart->baudrate = *(uint32_t *)arg;
        platform_uart_set_baudrate(uart->hal_handle, uart->baudrate);
        break;
    }
    return 0;
}

static const dev_ops_t uart_ops = {
    .open    = uart_open,
    .close   = uart_close,
    .read    = uart_read,
    .write   = uart_write,
    .control = uart_control,
};

void uart_dev_register(uart_dev_t *uart,
                       const char *name,
                       void       *hal_handle,
                       uint32_t    baudrate) {
    memset(uart, 0, sizeof(*uart));
    uart->base.name     = name;
    uart->base.type     = DEV_TYPE_UART;
    uart->base.ops      = &uart_ops;
    uart->hal_handle    = hal_handle;
    uart->baudrate      = baudrate;
    device_register(&uart->base);
}
```

------

## 第四步：GNSS 解析器（原生 UART 流）

### 4.1 架构思路

```text
GNSS 模块持续输出 NMEA 语句：

$GNGGA,123519.00,3157.0000,N,11706.0000,E,1,08,0.9,545.4,M,...*47\r\n
$GNRMC,123519.00,A,3157.0000,N,11706.0000,E,0.0,0.0,140126,...*6A\r\n
$GNGSA,...\r\n

解析流程：
UART ISR → 环形缓冲 → GNSS进程 → 帧提取 → NMEA解析 → 数据发布
```

### 4.2 GNSS 进程实现

```c
// service/gnss.h

typedef struct {
    // 位置
    double   latitude;     // 度，正北
    double   longitude;    // 度，正东
    float    altitude;     // 米
    // 速度
    float    speed_knots;
    float    course_deg;
    // 精度
    uint8_t  satellites;
    float    hdop;
    // 时间（UTC）
    uint8_t  hour, minute, second;
    uint8_t  day, month;
    uint16_t year;
    // 状态
    bool     valid;        // 定位是否有效
    bool     fresh;        // 是否有新数据（读取后清零）
} gnss_data_t;

extern gnss_data_t gnss_data;
extern process_event_t gnss_updated_event;  // 有新定位数据

PROCESS_NAME(gnss_process);
// service/gnss.c

#include "gnss.h"
#include "device/device.h"
#include "sys/process.h"
#include "sys/etimer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

gnss_data_t     gnss_data;
process_event_t gnss_updated_event;

// ── NMEA 帧缓冲 ───────────────────────────────────────
#define NMEA_MAX_LEN   100
static char nmea_frame[NMEA_MAX_LEN];
static uint8_t nmea_idx = 0;
static bool    nmea_ready = false;

// ── UART rx_indicate 回调（中断上下文）───────────────
static void gnss_uart_rx_cb(dev_t *dev, uint32_t len) {
    process_poll(&gnss_process);  // 通知 GNSS 进程
}

// ── NMEA 校验和验证 ───────────────────────────────────
static bool nmea_checksum_ok(const char *sentence) {
    // $GPGGA,...*XX  校验和在 * 后面
    const char *star = strchr(sentence, '*');
    if (!star) return false;

    uint8_t calc = 0;
    for (const char *p = sentence + 1; p < star; p++) {
        calc ^= (uint8_t)*p;
    }
    uint8_t recv = (uint8_t)strtol(star + 1, NULL, 16);
    return calc == recv;
}

// ── 解析 GGA 语句 ─────────────────────────────────────
// $GNGGA,123519.00,3157.0000,N,11706.0000,E,1,08,0.9,545.4,M,,,,*47
static void parse_gga(const char *sentence) {
    char buf[NMEA_MAX_LEN];
    strncpy(buf, sentence, sizeof(buf) - 1);

    char *tok = strtok(buf, ",");   // $GNGGA
    tok = strtok(NULL, ",");        // 时间 HHMMSS.ss

    if (tok && strlen(tok) >= 6) {
        gnss_data.hour   = (tok[0]-'0')*10 + (tok[1]-'0');
        gnss_data.minute = (tok[2]-'0')*10 + (tok[3]-'0');
        gnss_data.second = (tok[4]-'0')*10 + (tok[5]-'0');
    }

    tok = strtok(NULL, ",");        // 纬度 DDMM.MMMM
    double raw_lat = tok ? atof(tok) : 0;
    tok = strtok(NULL, ",");        // N/S
    double lat = (int)(raw_lat / 100) + fmod(raw_lat, 100) / 60.0;
    if (tok && tok[0] == 'S') lat = -lat;
    gnss_data.latitude = lat;

    tok = strtok(NULL, ",");        // 经度 DDDMM.MMMM
    double raw_lon = tok ? atof(tok) : 0;
    tok = strtok(NULL, ",");        // E/W
    double lon = (int)(raw_lon / 100) + fmod(raw_lon, 100) / 60.0;
    if (tok && tok[0] == 'W') lon = -lon;
    gnss_data.longitude = lon;

    tok = strtok(NULL, ",");        // 定位质量
    int fix = tok ? atoi(tok) : 0;
    gnss_data.valid = (fix > 0);

    tok = strtok(NULL, ",");        // 卫星数
    gnss_data.satellites = tok ? atoi(tok) : 0;

    tok = strtok(NULL, ",");        // HDOP
    gnss_data.hdop = tok ? atof(tok) : 99.9f;

    tok = strtok(NULL, ",");        // 海拔
    gnss_data.altitude = tok ? atof(tok) : 0;
}

// ── 解析 RMC 语句（速度、航向、日期）────────────────
// $GNRMC,123519.00,A,3157.0000,N,11706.0000,E,0.0,0.0,140126,...
static void parse_rmc(const char *sentence) {
    char buf[NMEA_MAX_LEN];
    strncpy(buf, sentence, sizeof(buf) - 1);

    char *tok = strtok(buf, ",");   // $GNRMC
    tok = strtok(NULL, ",");        // 时间
    tok = strtok(NULL, ",");        // 状态 A=有效
    bool valid = tok && tok[0] == 'A';

    // 跳过纬度、N/S、经度、E/W
    for (int i = 0; i < 4; i++) tok = strtok(NULL, ",");

    tok = strtok(NULL, ",");        // 速度（节）
    gnss_data.speed_knots = tok ? atof(tok) : 0;

    tok = strtok(NULL, ",");        // 航向（度）
    gnss_data.course_deg = tok ? atof(tok) : 0;

    tok = strtok(NULL, ",");        // 日期 DDMMYY
    if (tok && strlen(tok) >= 6) {
        gnss_data.day   = (tok[0]-'0')*10 + (tok[1]-'0');
        gnss_data.month = (tok[2]-'0')*10 + (tok[3]-'0');
        gnss_data.year  = 2000 + (tok[4]-'0')*10 + (tok[5]-'0');
    }
}

// ── NMEA 语句分发 ─────────────────────────────────────
static void nmea_dispatch(const char *sentence) {
    if (!nmea_checksum_ok(sentence)) return;

    // 跳过 $ 和前缀（GP/GN/GL 等）
    const char *type = sentence + 3;  // 跳过 $GN / $GP

    if (strncmp(type, "GGA", 3) == 0) {
        parse_gga(sentence);
        gnss_data.fresh = true;
        // 通知所有关心 GNSS 数据的进程
        process_post(PROCESS_BROADCAST,
                     gnss_updated_event, &gnss_data);

    } else if (strncmp(type, "RMC", 3) == 0) {
        parse_rmc(sentence);
    }
    // 可扩展：GSA（精度）、GSV（卫星视图）、VTG（速度）
}

// ── GNSS 主进程 ───────────────────────────────────────
PROCESS(gnss_process, "GNSS");

PROCESS_THREAD(gnss_process, ev, data) {
    static dev_t    *gnss_uart;
    static uint8_t   byte;
    static struct etimer watchdog_timer;

    PROCESS_BEGIN();

    gnss_updated_event = process_alloc_event();

    // 找到 GNSS 对应的 UART 设备
    gnss_uart = device_find("uart3");
    device_control(gnss_uart, DEV_CTRL_SET_RX_CB,
                   gnss_uart_rx_cb);
    device_open(gnss_uart, DEV_FLAG_RDONLY);

    // 看门狗：超过 5 秒没有定位数据，标记无效
    etimer_set(&watchdog_timer, 5 * CLOCK_SECOND);

    while (1) {
        PROCESS_YIELD_UNTIL(
            ev == PROCESS_EVENT_POLL ||
            etimer_expired(&watchdog_timer));

        if (etimer_expired(&watchdog_timer)) {
            gnss_data.valid = false;
            etimer_reset(&watchdog_timer);
            continue;
        }

        // 从 UART 缓冲读字节，逐字节组帧
        while (device_read(gnss_uart, 0, &byte, 1) == 1) {
            if (byte == '$') {
                // 新帧开始
                nmea_idx = 0;
                nmea_frame[nmea_idx++] = byte;

            } else if (byte == '\n' && nmea_idx > 0) {
                // 帧结束
                nmea_frame[nmea_idx] = '\0';
                nmea_dispatch(nmea_frame);
                nmea_idx = 0;
                // 重置看门狗
                etimer_restart(&watchdog_timer);

            } else if (nmea_idx > 0 && nmea_idx < NMEA_MAX_LEN - 1) {
                nmea_frame[nmea_idx++] = byte;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第五步：航海呼救模块

### 5.1 先明确协议类型

```text
航海呼救相关协议：

┌──────────────┬────────────────────────────────────────┐
│ 协议         │ 说明                                   │
├──────────────┼────────────────────────────────────────┤
│ DSC          │ 数字选择呼叫，VHF 无线电，NMEA 0183    │
│              │ $CDDSC / $CDDSE 语句                   │
├──────────────┼────────────────────────────────────────┤
│ AIS          │ 船舶自动识别，VHF，NMEA 0183           │
│              │ !AIVDM / !AIVDO 语句（二进制编码）     │
├──────────────┼────────────────────────────────────────┤
│ EPIRB        │ 紧急示位无线电信标，406MHz             │
│              │ 通常是独立设备，MCU 只控制触发 GPIO    │
├──────────────┼────────────────────────────────────────┤
│ SART         │ 搜救雷达应答器，9GHz                   │
│              │ 同上，GPIO 控制                        │
└──────────────┴────────────────────────────────────────┘

你的模块是哪种？
→ 如果输出 NMEA 语句（DSC/AIS）：走流式解析，和 GNSS 类似
→ 如果是 AT 指令控制：走 AT 框架
→ 如果是 GPIO 触发：最简单，直接 device_control
```

### 5.2 DSC 解析器示例

```c
// service/dsc.h

typedef enum {
    DSC_CALL_DISTRESS       = 0,   // 遇难呼叫
    DSC_CALL_URGENCY        = 1,   // 紧急呼叫
    DSC_CALL_SAFETY         = 2,   // 安全呼叫
    DSC_CALL_ROUTINE        = 3,   // 常规呼叫
} dsc_call_type_t;

typedef struct {
    dsc_call_type_t  call_type;
    char             mmsi[10];      // 船舶识别码
    char             nature[32];    // 遇难性质
    double           latitude;
    double           longitude;
    bool             has_position;
    uint8_t          hour, minute;
} dsc_message_t;

extern process_event_t dsc_distress_event;  // 收到遇难呼叫

PROCESS_NAME(dsc_process);
// service/dsc.c
// $CDDSC,20,123456789,00,00,26,1234.5,N,05612.3,E,0959,,,S*52

process_event_t dsc_distress_event;

static void parse_dsc(const char *sentence) {
    // 解析 $CDDSC 语句
    static dsc_message_t msg;
    char buf[128];
    strncpy(buf, sentence, sizeof(buf) - 1);

    char *tok = strtok(buf, ",");   // $CDDSC
    tok = strtok(NULL, ",");        // 格式说明符
    tok = strtok(NULL, ",");        // MMSI

    if (tok) strncpy(msg.mmsi, tok, sizeof(msg.mmsi) - 1);

    tok = strtok(NULL, ",");        // 呼叫类型
    int type = tok ? atoi(tok) : 99;

    if (type == 0) {
        msg.call_type = DSC_CALL_DISTRESS;
        // 解析位置...
        // 发布遇难事件（高优先级）
        process_post(PROCESS_BROADCAST,
                     dsc_distress_event, &msg);
    }
}

PROCESS(dsc_process, "DSC");

PROCESS_THREAD(dsc_process, ev, data) {
    static dev_t   *dsc_uart;
    static uint8_t  byte;
    static char     frame[128];
    static uint8_t  idx;

    PROCESS_BEGIN();

    dsc_distress_event = process_alloc_event();

    dsc_uart = device_find("uart4");
    device_control(dsc_uart, DEV_CTRL_SET_RX_CB,
                   (void*)dsc_uart_rx_cb);
    device_open(dsc_uart, DEV_FLAG_RDONLY);

    while (1) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        while (device_read(dsc_uart, 0, &byte, 1) == 1) {
            if (byte == '$') {
                idx = 0;
                frame[idx++] = byte;
            } else if (byte == '\n' && idx > 0) {
                frame[idx] = '\0';
                if (strncmp(frame, "$CDDSC", 6) == 0) {
                    parse_dsc(frame);
                }
                idx = 0;
            } else if (idx > 0 && idx < sizeof(frame) - 1) {
                frame[idx++] = byte;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第六步：完整框架文件结构

```text
project/
│
├── contiki-core/               ← 原封不动从 Contiki-NG 剥离
│   ├── sys/
│   │   ├── process.c/h
│   │   ├── etimer.c/h
│   │   ├── ctimer.c/h
│   │   ├── pt.h / lc.h
│   │   └── clock.h
│   └── lib/
│       ├── list.c/h
│       └── memb.c/h
│
├── device/                     ← 设备抽象层
│   ├── device.c/h              ← 基类 + 注册表
│   ├── uart_device.c/h         ← UART 设备
│   ├── spi_device.c/h          ← SPI 设备（后续扩展）
│   └── i2c_device.c/h          ← I2C 设备（BQ27xxx 用）
│
├── at/                         ← AT 框架
│   ├── at_client.c/h           ← AT client 多实例
│   └── at_dispatcher.c/h       ← 统一调度进程
│
├── service/                    ← 协议解析服务
│   ├── gnss.c/h                ← NMEA 解析 + gnss_process
│   ├── dsc.c/h                 ← DSC 解析 + dsc_process
│   ├── ais.c/h                 ← AIS 解析 + ais_process
│   └── nmea_common.c/h         ← 公共 NMEA 工具（校验和等）
│
├── comm/                       ← 通信模块驱动
│   ├── comm_4g.c/h             ← 4G 进程（用 AT 框架）
│   ├── comm_wifi.c/h           ← WiFi 进程
│   └── comm_ble.c/h            ← BLE 进程
│
├── power/                      ← 低功耗管理
│   └── power_manager.c/h
│
├── platform/                   ← 平台相关（唯一需要改的地方）
│   └── stm32/
│       ├── clock.c             ← SysTick → clock_time()
│       ├── uart_hal.c          ← HAL_UART → platform_uart_*
│       └── board.c             ← 设备注册入口
│
└── app/                        ← 业务逻辑
    ├── main.c
    ├── bms_monitor.c
    └── bms_upload.c
```

------

## 第七步：main.c 把所有东西串起来

```c
// app/main.c

int main(void) {
    // ── 1. 硬件初始化 ──────────────────────────────
    SystemInit();
    clock_init();           // SysTick

    // ── 2. Contiki 内核 ────────────────────────────
    process_init();
    etimer_init();

    // ── 3. 注册设备（平台相关）────────────────────
    board_devices_init();
    // 内部：
    // uart_dev_register(&uart1, "uart1", &huart1, 115200); // 4G
    // uart_dev_register(&uart2, "uart2", &huart2, 9600);   // BLE
    // uart_dev_register(&uart3, "uart3", &huart3, 9600);   // GNSS
    // uart_dev_register(&uart4, "uart4", &huart4, 38400);  // DSC

    // ── 4. 启动系统进程 ────────────────────────────
    process_start(&etimer_process,        NULL);
    process_start(&at_dispatcher_process, NULL);

    // ── 5. 启动服务进程 ────────────────────────────
    process_start(&gnss_process,          NULL);
    process_start(&dsc_process,           NULL);
    process_start(&power_manager_process, NULL);

    // ── 6. 启动通信进程 ────────────────────────────
    process_start(&comm_4g_process,       NULL);
    process_start(&comm_ble_process,      NULL);
    process_start(&comm_wifi_process,     NULL);

    // ── 7. 启动业务进程 ────────────────────────────
    process_start(&bms_monitor_process,   NULL);
    process_start(&bms_upload_process,    NULL);

    // ── 8. 主循环 ──────────────────────────────────
    while (1) {
        int pending = process_run();
        if (pending == 0) {
            __WFI();    // 无事件，休眠等中断
        }
    }
}
```

------

## 层次关系总结

```text
调用关系（从上到下）：

bms_upload_process
    │ process_post(comm_send_request_event)
    ▼
comm_4g_process
    │ PROCESS_PT_SPAWN(at_exec)
    ▼
at_exec (PT子线程)
    │ device_write(uart1_dev)
    ▼
uart_device → platform_uart_send → HAL_UART_Transmit

数据上行（从下到上）：

HAL_UART_RxCpltCallback
    │ uart_dev_isr_rx → rx_indicate → process_poll
    ▼
at_dispatcher_process（POLL唤醒）
    │ at_client_get_line → 匹配 OK/URC
    │ process_post(at_line_received_event)
    ▼
comm_4g_process（PT_WAIT 解除）
    │ 解析结果，process_post(data_ready_event)
    ▼
bms_upload_process
```

------

你的航海呼救模块具体是哪种接口？确认之后 DSC/AIS 解析器可以继续细化。





```
设计这框架比原始的 rthread-nano 有什么优势呢
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

好问题，必须诚实回答，不能只说优点。

------

## 先说结论

```text
不是"更好"，而是"不同的取舍"

RT-Thread Nano 在大多数工程场景下是更务实的选择
这套框架的价值在于特定约束条件下的极致优化
```

------

## 客观对比

```text
┌─────────────────────┬──────────────────────┬──────────────────────┐
│                     │ RT-Thread Nano       │ 本框架               │
│                     │                      │(Contiki-NG内核)      │
├─────────────────────┼──────────────────────┼──────────────────────┤
│ 内核模型            │ 抢占式多线程          │ 协作式进程+PT        │
│ 最小 ROM            │ ~8KB                 │ ~3KB                 │
│ 最小 RAM            │ ~2KB（含线程栈）      │ ~512B                │
│ 每个任务额外 RAM    │ 栈空间（通常512B起）  │ 几乎为零（PT状态）   │
│ 实时性              │ ✅ 有优先级抢占       │ ❌ 无抢占            │
│ 调试工具            │ ✅ SystemView/Keil    │ ❌ 几乎没有          │
│ 生态组件            │ ✅ 丰富（AT/FS/USB）  │ ❌ 需自己搬运        │
│ 学习曲线            │ 低（概念通用）        │ 高（PT思维转换）     │
│ 社区支持            │ ✅ 活跃，中文文档全   │ ❌ 几乎没有中文资料  │
│ 商业支持            │ ✅ RT-Thread官方      │ ❌ 无                │
│ 线程间同步          │ 信号量/互斥锁/邮箱   │ 事件（功能较弱）     │
│ 死锁风险            │ 有（优先级反转）      │ 无（单线程语义）     │
│ 栈溢出风险          │ 有                   │ 无（无独立栈）       │
│ 中断延迟            │ 确定性好             │ 取决于最长PT段       │
└─────────────────────┴──────────────────────┴──────────────────────┘
```

------

## RT-Thread Nano 真正的优势

### ① 抢占式调度：高优先级任务不被阻塞

```c
// RT-Thread：遇难呼叫可以立即抢占低优先级任务
void dsc_distress_thread(void *arg) {
    while (1) {
        rt_sem_take(&distress_sem, RT_WAITING_FOREVER);
        // 收到遇难信号，立即执行
        // 不管其他线程在干什么，直接抢占
        trigger_emergency_beacon();
    }
}
// 优先级设最高，保证实时响应

// ────────────────────────────────────────────
// 本框架：协作式，必须等当前 PT 主动让出
PROCESS_THREAD(dsc_process, ev, data) {
    // 如果此时 gnss_process 正在执行一段长计算
    // dsc 的遇难事件必须等 gnss_process 执行到
    // PROCESS_YIELD 才能被处理
    // → 响应延迟不确定
}
```

### ② 成熟的组件生态，开箱即用

```text
RT-Thread Nano 可以直接用：
├── AT 组件        → 你现在要自己实现的
├── NMEA 解析库    → 社区有现成的
├── FinSH Shell   → 调试神器
├── DFS 文件系统  → 日志存储
├── ULOG 日志     → 分级日志
└── CRC / 加密库  → 直接用

本框架：
└── 全部需要自己搬运或实现
```

### ③ 线程栈隔离，故障不蔓延

```c
// RT-Thread：每个线程有独立栈
// 一个线程栈溢出，只影响自己（通常）
// 有栈检测机制（canary）

// 本框架：所有进程共享调用栈
// 一个进程里的局部变量写越界
// 可能破坏其他进程的 static 变量
// 更难排查
```

------

## 本框架真正有优势的场景

### 优势一：RAM 极度受限（< 8KB SRAM）

```text
典型芯片：STM32F030（4KB RAM）/ STM32G030（8KB RAM）

RT-Thread Nano 最小配置：
├── 内核：~2KB RAM
├── 3个线程 × 512B栈 = 1.5KB
├── AT组件缓冲：512B
└── 合计：~4KB → 4KB RAM 的芯片装不下

本框架：
├── 内核：~256B RAM
├── 3个进程：几乎 0 额外 RAM（PT 只有 uint16_t）
├── AT缓冲：512B（可配置）
└── 合计：~1KB → 4KB RAM 完全够用
```

### 优势二：任务本身就是 IO 等待密集型

```text
你的场景分析：

comm_4g_process：
  90% 时间在等 AT 响应（PT_WAIT）
  10% 时间在处理数据

gnss_process：
  99% 时间在等 UART 数据（PROCESS_YIELD）
  1% 时间在解析 NMEA

bms_monitor_process：
  95% 时间在等定时器（etimer）
  5% 时间在读寄存器

→ 这种场景协作式调度完全够用
  没有任何任务需要"抢占"别人
  因为大家都在等 IO，不会长时间占用 CPU
```

### 优势三：无栈切换开销，功耗更低

```text
RT-Thread 线程切换：
├── 保存当前线程寄存器（Cortex-M：17个寄存器）
├── 切换 PSP 指针
├── 恢复新线程寄存器
└── 约 100~200 ns / 次切换

本框架进程切换：
├── process_run() 直接调用下一个进程函数
├── PT 恢复：switch(pt->lc) 跳转
└── 约 10~20 ns / 次

在高频事件场景（如 GNSS 9600bps 每字节中断）
切换开销差异会积累成可测量的功耗差异
```

### 优势四：无优先级反转，无死锁

```c
// RT-Thread 经典陷阱：
rt_mutex_t uart_mutex;

// 低优先级线程持有锁
void low_prio_thread(void) {
    rt_mutex_take(&uart_mutex, RT_WAITING_FOREVER);
    // 此时被中优先级线程抢占
    // 高优先级线程也在等这个锁
    // → 优先级反转，需要优先级继承协议
}

// 本框架：
// 根本不存在这个问题
// 单线程语义，at_exec 里的 busy 标志
// 不需要任何锁
```

------

## 诚实的建议

```text
选 RT-Thread Nano 的条件（满足任意一条）：
├── RAM > 8KB
├── 有实时响应需求（遇难呼叫必须在 Xms 内响应）
├── 团队成员熟悉 RTOS 概念
├── 需要用 RT-Thread 生态组件
└── 项目工期紧，不能自己造轮子

选本框架的条件（需要同时满足）：
├── RAM < 8KB，每个字节都要抠
├── 所有任务都是 IO 等待密集型
├── 团队能接受 PT 编程模型
└── 有时间自己实现和调试框架
```

------

## 对你的 BMS 项目的具体判断

```text
你的 MCU 是什么型号？

如果是 STM32F4 / STM32G4（128KB+ RAM）：
→ 直接用 RT-Thread Nano
  生态成熟，AT组件现成，节省 2~3 周开发时间
  本框架的 RAM 优势在这里毫无意义

如果是 STM32G0 / STM32L0（8~16KB RAM）：
→ 本框架有价值
  RT-Thread Nano 装进去很勉强
  PT 模型在 IO 密集场景完全够用

如果是 STM32L4 + 低功耗是核心指标：
→ 两者都可以
  但本框架的 WFI 集成更自然
  RT-Thread 的 tickless 模式配置更复杂

真正的建议：
如果你的目标是学习 OS 设计思想
→ 本框架是极佳的学习路径

如果你的目标是快速交付产品
→ RT-Thread Nano 是更务实的选择

两者不矛盾：
先用 RT-Thread Nano 交付产品
同时研究 Contiki-NG 内核理解底层原理
把学到的 PT / 事件模型思想反哺到 RT-Thread 的设计中
```





```
你的航海呼救模块具体是哪种接口？确认之后 DSC/AIS 解析器可以继续细化。 - 可以都支持麽
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-Sonnet-4.6

可以，但要先把每种协议的接口现实搞清楚，再决定框架怎么设计。

------

## 先把每种协议的硬件接口现实说清楚

```text
┌──────────┬──────────────┬────────────────┬──────────────────────┐
│ 协议     │ 硬件接口     │ 数据方向       │ 备注                 │
├──────────┼──────────────┼────────────────┼──────────────────────┤
│ DSC      │ UART(NMEA)   │ 双向           │ 收 $CDDSC            │
│          │              │ 模块→MCU       │ 发 $CDDSC 触发呼叫   │
├──────────┼──────────────┼────────────────┼──────────────────────┤
│ AIS      │ UART(NMEA)   │ 单向           │ 只收 !AIVDM          │
│          │              │ 模块→MCU       │ 发送需要专用 VHF TX  │
├──────────┼──────────────┼────────────────┼──────────────────────┤
│ EPIRB    │ GPIO         │ MCU→模块       │ 触发信标激活         │
│          │ + 状态GPIO   │ 模块→MCU       │ 读取激活状态         │
├──────────┼──────────────┼────────────────┼──────────────────────┤
│ SART     │ GPIO         │ MCU→模块       │ 激活/静默控制        │
├──────────┼──────────────┼────────────────┼──────────────────────┤
│ PLB      │ GPIO         │ MCU→模块       │ 个人定位信标，同EPIRB│
└──────────┴──────────────┴────────────────┴──────────────────────┘

关键发现：
DSC + AIS → UART 流式解析（和 GNSS 同类）
EPIRB / SART / PLB → GPIO 控制（最简单）

DSC 和 AIS 通常共用同一个 VHF 模块的同一个 UART
```

------

## AIS 协议的特殊性：需要单独说清楚

```text
AIS 报文不是普通文本，是 6-bit ASCII 编码的二进制：

原始 NMEA 语句：
!AIVDM,1,1,,B,15M67N0Oh;wp<H4WD>Mk0h0p0000,0*6B

字段分解：
!AIVDM          → 句子标识（他船数据）
1,1             → 总片数,当前片（多片拼接用）
                → 顺序号（空）
B               → 信道（A或B）
15M67N0Oh;wp... → 有效载荷（6-bit编码）
0               → 填充位数
*6B             → 校验和

有效载荷解码后得到：
├── 消息类型（1-27种）
├── MMSI（船舶识别码）
├── 航速、航向、位置
├── 船名、呼号
└── 船舶类型、尺寸
```

------

## 整体框架设计

```text
┌─────────────────────────────────────────────────────────┐
│                    应用层                                │
│  distress_manager_process（统一呼救决策）                │
├───────────────┬─────────────────┬───────────────────────┤
│  DSC 服务     │   AIS 服务      │  信标控制服务          │
│  dsc_process  │   ais_process   │  beacon_process        │
├───────────────┴─────────────────┴───────────────────────┤
│              NMEA 公共层                                 │
│   帧提取 / 校验和 / 多片拼接                             │
├─────────────────────────────────────────────────────────┤
│              设备层                                      │
│   uart_device（VHF模块）  gpio_device（EPIRB/SART）     │
└─────────────────────────────────────────────────────────┘
```

------

## 第一步：NMEA 公共层（DSC 和 AIS 共用）

```c
// service/nmea_common.h

#ifndef NMEA_COMMON_H
#define NMEA_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define NMEA_MAX_SENTENCE_LEN   128
#define NMEA_MAX_FIELDS         20

// ── 校验和验证 ────────────────────────────────────────
bool nmea_checksum_ok(const char *sentence);

// ── 字段分割（不破坏原字符串）────────────────────────
// 返回字段数量，fields[] 指向各字段起始位置
typedef struct {
    char   buf[NMEA_MAX_SENTENCE_LEN];  // 内部拷贝
    char  *fields[NMEA_MAX_FIELDS];
    uint8_t count;
} nmea_parsed_t;

bool nmea_parse(const char *sentence, nmea_parsed_t *out);

// ── 多片拼接（AIS 用）────────────────────────────────
// AIS 大报文会分成多片：
// !AIVDM,2,1,3,B,payload1,0
// !AIVDM,2,2,3,B,payload2,0
typedef struct {
    char     payload[256];   // 拼接后的完整载荷
    uint8_t  total;          // 总片数
    uint8_t  received;       // 已收到片数
    uint8_t  seq_id;         // 顺序号
    bool     complete;       // 是否拼接完成
} nmea_fragment_t;

void nmea_fragment_init(nmea_fragment_t *frag);
bool nmea_fragment_feed(nmea_fragment_t *frag,
                        const nmea_parsed_t *parsed);

#endif
// service/nmea_common.c

#include "nmea_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool nmea_checksum_ok(const char *sentence) {
    const char *start = sentence;

    // 找到 $ 或 !
    if (*start == '$' || *start == '!') start++;
    else return false;

    const char *star = strchr(start, '*');
    if (!star || strlen(star) < 3) return false;

    uint8_t calc = 0;
    for (const char *p = start; p < star; p++) {
        calc ^= (uint8_t)*p;
    }

    uint8_t recv = (uint8_t)strtol(star + 1, NULL, 16);
    return calc == recv;
}

bool nmea_parse(const char *sentence, nmea_parsed_t *out) {
    if (!nmea_checksum_ok(sentence)) return false;

    strncpy(out->buf, sentence, sizeof(out->buf) - 1);
    out->count = 0;

    // 去掉校验和部分（*XX）
    char *star = strchr(out->buf, '*');
    if (star) *star = '\0';

    // 分割字段
    char *p = out->buf;
    while (p && out->count < NMEA_MAX_FIELDS) {
        out->fields[out->count++] = p;
        p = strchr(p, ',');
        if (p) *p++ = '\0';
    }

    return out->count > 0;
}

void nmea_fragment_init(nmea_fragment_t *frag) {
    memset(frag, 0, sizeof(*frag));
}

bool nmea_fragment_feed(nmea_fragment_t *frag,
                        const nmea_parsed_t *parsed)
{
    // fields: [0]句子头 [1]总片 [2]当前片 [3]顺序号
    //         [4]信道  [5]载荷 [6]填充位
    if (parsed->count < 7) return false;

    uint8_t total   = atoi(parsed->fields[1]);
    uint8_t current = atoi(parsed->fields[2]);
    uint8_t seq     = atoi(parsed->fields[3]);

    // 新序列开始
    if (current == 1) {
        nmea_fragment_init(frag);
        frag->total  = total;
        frag->seq_id = seq;
    }

    // 校验顺序号一致
    if (frag->seq_id != seq && total > 1) return false;

    // 拼接载荷
    strncat(frag->payload, parsed->fields[5],
            sizeof(frag->payload) - strlen(frag->payload) - 1);
    frag->received++;

    if (frag->received >= frag->total) {
        frag->complete = true;
        return true;
    }
    return false;
}
```

------

## 第二步：AIS 解码器

### AIS 6-bit 解码是核心难点

```c
// service/ais_decoder.h

#ifndef AIS_DECODER_H
#define AIS_DECODER_H

#include <stdint.h>
#include <stdbool.h>

// ── AIS 消息类型（只实现常用的）─────────────────────
typedef enum {
    AIS_MSG_POS_CLASS_A     = 1,   // A类位置报告（商船）
    AIS_MSG_POS_CLASS_A_2   = 2,
    AIS_MSG_POS_CLASS_A_3   = 3,
    AIS_MSG_BASE_STATION    = 4,   // 基站报告
    AIS_MSG_STATIC_VOYAGE   = 5,   // 静态和航次数据
    AIS_MSG_POS_CLASS_B     = 18,  // B类位置报告（小船）
    AIS_MSG_STATIC_CLASS_B  = 24,  // B类静态数据
} ais_msg_type_t;

// ── 位置报告（消息1/2/3/18）─────────────────────────
typedef struct {
    uint8_t  msg_type;
    uint32_t mmsi;
    uint8_t  nav_status;      // 0=航行,1=锚泊,5=停车...
    float    speed;           // 节，0.1精度
    float    longitude;       // 度，东正西负
    float    latitude;        // 度，北正南负
    float    course;          // 对地航向，度
    uint16_t heading;         // 真航向，度
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    bool     valid_position;
} ais_position_t;

// ── 静态数据（消息5/24）─────────────────────────────
typedef struct {
    uint8_t  msg_type;
    uint32_t mmsi;
    char     vessel_name[21];  // 船名，20字符+\0
    char     call_sign[8];     // 呼号
    uint8_t  ship_type;        // 船舶类型
    uint16_t length;           // 船长，米
    uint16_t beam;             // 船宽，米
} ais_static_t;

// ── 解码结果联合体 ────────────────────────────────────
typedef struct {
    ais_msg_type_t type;
    union {
        ais_position_t position;
        ais_static_t   static_data;
    };
} ais_message_t;

// ── 解码 API ──────────────────────────────────────────
// payload: 6-bit 编码字符串
// pad:     填充位数（最后一个字符的无效位数）
bool ais_decode(const char *payload, uint8_t pad,
                ais_message_t *out);

#endif
// service/ais_decoder.c

#include "ais_decoder.h"
#include <string.h>
#include <math.h>

// ── 6-bit ASCII 解码表 ────────────────────────────────
// ASCII码 - 48，超过 40 再减 8
static uint8_t ais_char_to_6bit(char c) {
    uint8_t val = (uint8_t)c - 48;
    if (val > 40) val -= 8;
    return val & 0x3F;
}

// ── 从 6-bit 流中提取任意位宽的整数 ──────────────────
// payload: 6-bit 编码字符串
// start:   起始位（从0计）
// len:     位数
static uint32_t ais_get_uint(const char *payload,
                              uint16_t start, uint8_t len)
{
    uint32_t result = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint16_t bit_pos  = start + i;
        uint16_t char_idx = bit_pos / 6;
        uint8_t  bit_idx  = 5 - (bit_pos % 6);  // MSB first

        uint8_t byte = ais_char_to_6bit(payload[char_idx]);
        if (byte & (1 << bit_idx)) {
            result |= (1u << (len - 1 - i));
        }
    }
    return result;
}

// ── 有符号整数（经纬度用）────────────────────────────
static int32_t ais_get_int(const char *payload,
                            uint16_t start, uint8_t len)
{
    uint32_t val = ais_get_uint(payload, start, len);
    // 符号扩展
    if (val & (1u << (len - 1))) {
        val |= ~((1u << len) - 1);
    }
    return (int32_t)val;
}

// ── 提取文本字段（6-bit ASCII）───────────────────────
static void ais_get_text(const char *payload,
                         uint16_t start, uint8_t char_count,
                         char *out)
{
    // AIS 文本用 6-bit ASCII：0=@, 1=A, ..., 26=Z, 32=空格
    static const char ais_ascii[] =
        "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";

    for (uint8_t i = 0; i < char_count; i++) {
        uint8_t code = ais_get_uint(payload, start + i * 6, 6);
        out[i] = (code < sizeof(ais_ascii)) ? ais_ascii[code] : ' ';
    }
    // 去掉末尾 @ 和空格
    int8_t end = char_count - 1;
    while (end >= 0 && (out[end] == '@' || out[end] == ' ')) {
        end--;
    }
    out[end + 1] = '\0';
}

// ── 解码位置报告（消息1/2/3）─────────────────────────
static bool decode_position_class_a(const char *payload,
                                     ais_position_t *pos)
{
    pos->msg_type   = ais_get_uint(payload, 0,  6);
    pos->mmsi       = ais_get_uint(payload, 8,  30);
    pos->nav_status = ais_get_uint(payload, 38, 4);

    // 速度：0.1节精度，1023=不可用
    uint16_t spd    = ais_get_uint(payload, 50, 10);
    pos->speed      = (spd < 1023) ? spd / 10.0f : -1;

    // 经度：1/10000分精度，181000=不可用
    int32_t lon_raw = ais_get_int(payload, 61, 28);
    pos->longitude  = (lon_raw != 0x6791AC0) ?
                      lon_raw / 600000.0f : 181.0f;

    // 纬度：1/10000分精度，91000=不可用
    int32_t lat_raw = ais_get_int(payload, 89, 27);
    pos->latitude   = (lat_raw != 0x3412140) ?
                      lat_raw / 600000.0f : 91.0f;

    // 对地航向：0.1度精度，3600=不可用
    uint16_t cog    = ais_get_uint(payload, 116, 12);
    pos->course     = (cog < 3600) ? cog / 10.0f : -1;

    // 真航向：511=不可用
    pos->heading    = ais_get_uint(payload, 128, 9);

    // 时间戳（秒）
    pos->second     = ais_get_uint(payload, 137, 6);

    pos->valid_position = (pos->longitude < 181.0f &&
                           pos->latitude  <  91.0f);
    return true;
}

// ── 解码 B 类位置报告（消息18）───────────────────────
static bool decode_position_class_b(const char *payload,
                                     ais_position_t *pos)
{
    pos->msg_type  = ais_get_uint(payload, 0,  6);
    pos->mmsi      = ais_get_uint(payload, 8,  30);

    uint16_t spd   = ais_get_uint(payload, 46, 10);
    pos->speed     = (spd < 1023) ? spd / 10.0f : -1;

    int32_t lon_raw = ais_get_int(payload, 57, 28);
    pos->longitude  = (lon_raw != 0x6791AC0) ?
                      lon_raw / 600000.0f : 181.0f;

    int32_t lat_raw = ais_get_int(payload, 85, 27);
    pos->latitude   = (lat_raw != 0x3412140) ?
                      lat_raw / 600000.0f : 91.0f;

    uint16_t cog   = ais_get_uint(payload, 112, 12);
    pos->course    = (cog < 3600) ? cog / 10.0f : -1;

    pos->heading   = ais_get_uint(payload, 124, 9);
    pos->second    = ais_get_uint(payload, 133, 6);

    pos->valid_position = (pos->longitude < 181.0f &&
                           pos->latitude  <  91.0f);
    return true;
}

// ── 解码静态数据（消息5）─────────────────────────────
static bool decode_static_voyage(const char *payload,
                                  ais_static_t *st)
{
    st->msg_type = ais_get_uint(payload, 0,  6);
    st->mmsi     = ais_get_uint(payload, 8,  30);

    ais_get_text(payload, 112, 20, st->vessel_name);
    ais_get_text(payload, 70,  7,  st->call_sign);

    st->ship_type = ais_get_uint(payload, 232, 8);

    // 船尺寸：前后左右距离参考点的距离
    uint16_t bow   = ais_get_uint(payload, 240, 9);
    uint16_t stern = ais_get_uint(payload, 249, 9);
    uint16_t port  = ais_get_uint(payload, 258, 6);
    uint16_t star  = ais_get_uint(payload, 264, 6);
    st->length = bow + stern;
    st->beam   = port + star;

    return true;
}

// ── 解码 B 类静态数据（消息24）───────────────────────
static bool decode_static_class_b(const char *payload,
                                   ais_static_t *st)
{
    st->msg_type  = ais_get_uint(payload, 0, 6);
    st->mmsi      = ais_get_uint(payload, 8, 30);
    uint8_t part  = ais_get_uint(payload, 38, 2);

    if (part == 0) {
        // Part A：船名
        ais_get_text(payload, 40, 20, st->vessel_name);
    } else {
        // Part B：呼号、船型、尺寸
        st->ship_type = ais_get_uint(payload, 40, 8);
        ais_get_text(payload, 90, 7, st->call_sign);
    }
    return true;
}

// ── 主解码入口 ────────────────────────────────────────
bool ais_decode(const char *payload, uint8_t pad,
                ais_message_t *out)
{
    if (!payload || strlen(payload) < 2) return false;

    uint8_t msg_type = ais_get_uint(payload, 0, 6);
    out->type = (ais_msg_type_t)msg_type;

    switch (msg_type) {
    case 1: case 2: case 3:
        return decode_position_class_a(payload,
                                       &out->position);
    case 18:
        return decode_position_class_b(payload,
                                       &out->position);
    case 5:
        return decode_static_voyage(payload,
                                    &out->static_data);
    case 24:
        return decode_static_class_b(payload,
                                     &out->static_data);
    default:
        return false;  // 暂不支持的消息类型
    }
}
```

------

## 第三步：DSC 解析器

```c
// service/dsc.h

#ifndef DSC_H
#define DSC_H

#include <stdint.h>
#include <stdbool.h>
#include "sys/process.h"

// ── DSC 呼叫类型 ──────────────────────────────────────
typedef enum {
    DSC_FORMAT_DISTRESS     = 112,  // 遇难
    DSC_FORMAT_URGENCY      = 116,  // 紧急
    DSC_FORMAT_SAFETY       = 114,  // 安全
    DSC_FORMAT_ALL_SHIPS    = 116,  // 全体呼叫
    DSC_FORMAT_GROUP        = 120,  // 群呼
    DSC_FORMAT_INDIVIDUAL   = 114,  // 个别呼叫
} dsc_format_t;

// ── 遇难性质 ──────────────────────────────────────────
typedef enum {
    DSC_NATURE_UNDESIGNATED = 100,
    DSC_NATURE_FIRE         = 101,
    DSC_NATURE_FLOODING     = 102,
    DSC_NATURE_COLLISION    = 103,
    DSC_NATURE_GROUNDING    = 104,
    DSC_NATURE_LISTING      = 105,
    DSC_NATURE_SINKING      = 106,
    DSC_NATURE_DISABLED     = 107,
    DSC_NATURE_ABANDONING   = 112,
    DSC_NATURE_PIRACY       = 113,
    DSC_NATURE_MOB          = 114,  // 落水
    DSC_NATURE_EPIRB        = 102,
} dsc_nature_t;

// ── DSC 消息 ──────────────────────────────────────────
typedef struct {
    dsc_format_t  format;
    char          mmsi[10];         // 呼叫方 MMSI
    dsc_nature_t  nature;           // 遇难性质
    double        latitude;
    double        longitude;
    bool          has_position;
    uint8_t       hour;
    uint8_t       minute;
    char          subsequent[4];    // 后续通信信道
} dsc_message_t;

extern process_event_t dsc_distress_event;
extern process_event_t dsc_urgency_event;
extern process_event_t dsc_safety_event;
extern process_event_t dsc_call_event;

PROCESS_NAME(dsc_process);

// ── 发送 DSC 遇难呼叫 API ─────────────────────────────
// 供 distress_manager 调用
int dsc_send_distress(double lat, double lon,
                      dsc_nature_t nature);

#endif
// service/dsc.c

#include "dsc.h"
#include "nmea_common.h"
#include "device/device.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

process_event_t dsc_distress_event;
process_event_t dsc_urgency_event;
process_event_t dsc_safety_event;
process_event_t dsc_call_event;

static dsc_message_t last_dsc_msg;

// ── 解析 NMEA 坐标字段 ────────────────────────────────
// DSC 坐标格式：DDMM.MMMM 或 DDDMM.MMMM
static double parse_dsc_coord(const char *val,
                               const char *dir)
{
    if (!val || strlen(val) == 0) return 0.0;
    double raw = atof(val);
    int    deg = (int)(raw / 100);
    double min = raw - deg * 100;
    double result = deg + min / 60.0;
    if (dir && (dir[0] == 'S' || dir[0] == 'W')) {
        result = -result;
    }
    return result;
}

// ── 解析 $CDDSC 语句 ──────────────────────────────────
// $CDDSC,20,123456789,00,00,26,1234.5,N,05612.3,E,0959,,,S*52
// 字段：[0]$CDDSC [1]格式说明符 [2]MMSI [3]类别
//       [4]性质   [5]后续通信   [6]纬度 [7]N/S
//       [8]经度   [9]E/W        [10]时间 ...
static void parse_cddsc(const nmea_parsed_t *p) {
    if (p->count < 5) return;

    memset(&last_dsc_msg, 0, sizeof(last_dsc_msg));

    // 格式说明符（呼叫类型）
    last_dsc_msg.format = (dsc_format_t)atoi(p->fields[1]);

    // MMSI
    strncpy(last_dsc_msg.mmsi, p->fields[2],
            sizeof(last_dsc_msg.mmsi) - 1);

    // 遇难性质
    last_dsc_msg.nature = (dsc_nature_t)atoi(p->fields[4]);

    // 位置（可选字段）
    if (p->count >= 10 && strlen(p->fields[6]) > 0) {
        last_dsc_msg.latitude  = parse_dsc_coord(
            p->fields[6], p->fields[7]);
        last_dsc_msg.longitude = parse_dsc_coord(
            p->fields[8], p->fields[9]);
        last_dsc_msg.has_position = true;
    }

    // 时间
    if (p->count >= 11 && strlen(p->fields[10]) >= 4) {
        const char *t = p->fields[10];
        last_dsc_msg.hour   = (t[0]-'0')*10 + (t[1]-'0');
        last_dsc_msg.minute = (t[2]-'0')*10 + (t[3]-'0');
    }

    // 后续通信信道
    if (p->count >= 6) {
        strncpy(last_dsc_msg.subsequent, p->fields[5],
                sizeof(last_dsc_msg.subsequent) - 1);
    }

    // 根据格式分发事件
    switch (last_dsc_msg.format) {
    case DSC_FORMAT_DISTRESS:
        process_post(PROCESS_BROADCAST,
                     dsc_distress_event, &last_dsc_msg);
        break;
    case DSC_FORMAT_URGENCY:
        process_post(PROCESS_BROADCAST,
                     dsc_urgency_event, &last_dsc_msg);
        break;
    case DSC_FORMAT_SAFETY:
        process_post(PROCESS_BROADCAST,
                     dsc_safety_event, &last_dsc_msg);
        break;
    default:
        process_post(PROCESS_BROADCAST,
                     dsc_call_event, &last_dsc_msg);
        break;
    }
}

// ── 发送 DSC 遇难呼叫 ─────────────────────────────────
int dsc_send_distress(double lat, double lon,
                      dsc_nature_t nature)
{
    dev_t *vhf_uart = device_find("uart_vhf");
    if (!vhf_uart) return -1;

    // 构造 $CDDSC 遇难呼叫语句
    // 格式：$CDDSC,112,999999999,00,<nature>,06,
    //              <lat>,N,<lon>,E,<time>,,,S
    char lat_str[12], lon_str[12];
    int  lat_deg = (int)fabs(lat);
    int  lon_deg = (int)fabs(lon);
    double lat_min = (fabs(lat) - lat_deg) * 60.0;
    double lon_min = (fabs(lon) - lon_deg) * 60.0;

    snprintf(lat_str, sizeof(lat_str),
             "%02d%05.2f", lat_deg, lat_min);
    snprintf(lon_str, sizeof(lon_str),
             "%03d%05.2f", lon_deg, lon_min);

    char sentence[128];
    snprintf(sentence, sizeof(sentence),
             "CDDSC,112,999999999,00,%d,06,%s,%c,%s,%c,9999,,,S",
             nature,
             lat_str, lat >= 0 ? 'N' : 'S',
             lon_str, lon >= 0 ? 'E' : 'W');

    // 计算校验和
    uint8_t cs = 0;
    for (const char *p = sentence; *p; p++) cs ^= *p;

    char full[140];
    snprintf(full, sizeof(full), "$%s*%02X\r\n", sentence, cs);

    device_write(vhf_uart, 0, full, strlen(full));
    return 0;
}

// ── DSC 进程 ──────────────────────────────────────────
static void dsc_uart_rx_cb(dev_t *dev, uint32_t len) {
    process_poll(&dsc_process);
}

PROCESS(dsc_process, "DSC");

PROCESS_THREAD(dsc_process, ev, data) {
    static dev_t        *vhf_uart;
    static uint8_t       byte;
    static char          frame[NMEA_MAX_SENTENCE_LEN];
    static uint8_t       idx;
    static nmea_parsed_t parsed;

    PROCESS_BEGIN();

    dsc_distress_event = process_alloc_event();
    dsc_urgency_event  = process_alloc_event();
    dsc_safety_event   = process_alloc_event();
    dsc_call_event     = process_alloc_event();

    vhf_uart = device_find("uart_vhf");
    device_control(vhf_uart, DEV_CTRL_SET_RX_CB,
                   dsc_uart_rx_cb);
    device_open(vhf_uart, DEV_FLAG_RDWR);

    idx = 0;

    while (1) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        while (device_read(vhf_uart, 0, &byte, 1) == 1) {
            if (byte == '$') {
                idx = 0;
                frame[idx++] = byte;
            } else if (byte == '\n' && idx > 0) {
                frame[idx] = '\0';
                if (strncmp(frame, "$CDDSC", 6) == 0 ||
                    strncmp(frame, "$CDDSE", 6) == 0) {
                    if (nmea_parse(frame, &parsed)) {
                        parse_cddsc(&parsed);
                    }
                }
                idx = 0;
            } else if (idx > 0 &&
                       idx < NMEA_MAX_SENTENCE_LEN - 1) {
                frame[idx++] = byte;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第四步：AIS 进程

```c
// service/ais.c

#include "ais_decoder.h"
#include "nmea_common.h"
#include "device/device.h"
#include "lib/memb.h"
#include "lib/list.h"

// ── AIS 目标跟踪表 ────────────────────────────────────
#define AIS_MAX_TARGETS   32   // 最多跟踪 32 个目标

typedef struct ais_target {
    struct ais_target *next;
    uint32_t           mmsi;
    ais_position_t     position;
    ais_static_t       static_data;
    bool               has_static;
    uint32_t           last_seen;   // clock_seconds()
} ais_target_t;

MEMB(ais_target_memb, ais_target_t, AIS_MAX_TARGETS);
LIST(ais_target_list);

process_event_t ais_position_event;   // 新位置数据
process_event_t ais_static_event;     // 新静态数据
process_event_t ais_new_target_event; // 发现新目标

// ── 目标管理 ──────────────────────────────────────────
static ais_target_t *ais_find_target(uint32_t mmsi) {
    ais_target_t *t;
    for (t = list_head(ais_target_list);
         t; t = list_item_next(t)) {
        if (t->mmsi == mmsi) return t;
    }
    return NULL;
}

static ais_target_t *ais_get_or_create(uint32_t mmsi) {
    ais_target_t *t = ais_find_target(mmsi);
    if (t) return t;

    t = memb_alloc(&ais_target_memb);
    if (!t) {
        // 内存满，淘汰最旧的目标
        t = list_tail(ais_target_list);
        list_remove(ais_target_list, t);
        memset(t, 0, sizeof(*t));
    }
    t->mmsi = mmsi;
    list_push(ais_target_list, t);
    return t;
}

// ── 处理解码后的 AIS 消息 ─────────────────────────────
static void ais_handle_message(const ais_message_t *msg) {
    ais_target_t *target;

    switch (msg->type) {
    case AIS_MSG_POS_CLASS_A:
    case AIS_MSG_POS_CLASS_A_2:
    case AIS_MSG_POS_CLASS_A_3:
    case AIS_MSG_POS_CLASS_B: {
        const ais_position_t *pos = &msg->position;
        bool is_new = (ais_find_target(pos->mmsi) == NULL);

        target = ais_get_or_create(pos->mmsi);
        target->position  = *pos;
        target->last_seen = clock_seconds();

        if (is_new) {
            process_post(PROCESS_BROADCAST,
                         ais_new_target_event, target);
        }
        process_post(PROCESS_BROADCAST,
                     ais_position_event, target);
        break;
    }

    case AIS_MSG_STATIC_VOYAGE:
    case AIS_MSG_STATIC_CLASS_B: {
        const ais_static_t *st = &msg->static_data;
        target = ais_get_or_create(st->mmsi);
        target->static_data = *st;
        target->has_static  = true;
        target->last_seen   = clock_seconds();

        process_post(PROCESS_BROADCAST,
                     ais_static_event, target);
        break;
    }

    default:
        break;
    }
}

// ── AIS 进程 ──────────────────────────────────────────
static void ais_uart_rx_cb(dev_t *dev, uint32_t len) {
    process_poll(&ais_process);
}

PROCESS(ais_process, "AIS");

PROCESS_THREAD(ais_process, ev, data) {
    static dev_t           *vhf_uart;
    static uint8_t          byte;
    static char             frame[NMEA_MAX_SENTENCE_LEN];
    static uint8_t          idx;
    static nmea_parsed_t    parsed;
    static nmea_fragment_t  fragment;
    static ais_message_t    msg;
    static struct etimer    gc_timer;

    PROCESS_BEGIN();

    ais_position_event   = process_alloc_event();
    ais_static_event     = process_alloc_event();
    ais_new_target_event = process_alloc_event();

    memb_init(&ais_target_memb);
    list_init(ais_target_list);
    nmea_fragment_init(&fragment);

    // DSC 和 AIS 共用同一个 VHF UART
    // 注意：dsc_process 已经 open 了，这里只注册回调
    vhf_uart = device_find("uart_vhf");
    // 多进程共用同一 UART：rx_indicate 只能有一个
    // 解决方案：用 at_dispatcher 同样的思路
    // 让 vhf_dispatcher_process 统一接收，再分发
    // （见下节）

    // 垃圾回收：每 5 分钟清理超时目标
    etimer_set(&gc_timer, 5 * 60 * CLOCK_SECOND);

    while (1) {
        PROCESS_YIELD_UNTIL(
            ev == PROCESS_EVENT_POLL ||
            etimer_expired(&gc_timer));

        if (etimer_expired(&gc_timer)) {
            // 清理 10 分钟未更新的目标
            uint32_t now = clock_seconds();
            ais_target_t *t = list_head(ais_target_list);
            while (t) {
                ais_target_t *next = list_item_next(t);
                if (now - t->last_seen > 600) {
                    list_remove(ais_target_list, t);
                    memb_free(&ais_target_memb, t);
                }
                t = next;
            }
            etimer_reset(&gc_timer);
            continue;
        }

        while (device_read(vhf_uart, 0, &byte, 1) == 1) {
            if (byte == '!') {
                idx = 0;
                frame[idx++] = byte;
            } else if (byte == '\n' && idx > 0) {
                frame[idx] = '\0';

                if (strncmp(frame, "!AIVDM", 6) == 0 ||
                    strncmp(frame, "!AIVDO", 6) == 0) {

                    if (nmea_parse(frame, &parsed)) {
                        // 多片拼接
                        if (nmea_fragment_feed(&fragment,
                                               &parsed)) {
                            // 拼接完成，解码
                            uint8_t pad = atoi(
                                parsed.fields[6]);
                            if (ais_decode(fragment.payload,
                                           pad, &msg)) {
                                ais_handle_message(&msg);
                            }
                            nmea_fragment_init(&fragment);
                        }
                    }
                }
                idx = 0;
            } else if (idx > 0 &&
                       idx < NMEA_MAX_SENTENCE_LEN - 1) {
                frame[idx++] = byte;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第五步：VHF UART 共享问题解决

DSC 和 AIS 共用同一个 UART，需要一个分发器：

```c
// service/vhf_dispatcher.c
// 类似 at_dispatcher，统一接收 VHF UART 数据
// 根据句子头分发给 DSC 或 AIS 进程

static void vhf_uart_rx_cb(dev_t *dev, uint32_t len) {
    process_poll(&vhf_dispatcher_process);
}

PROCESS(vhf_dispatcher_process, "VHF Dispatcher");

PROCESS_THREAD(vhf_dispatcher_process, ev, data) {
    static dev_t   *vhf_uart;
    static uint8_t  byte;
    static char     frame[NMEA_MAX_SENTENCE_LEN];
    static uint8_t  idx;
    static bool     is_ais_frame;

    PROCESS_BEGIN();

    vhf_uart = device_find("uart_vhf");
    device_control(vhf_uart, DEV_CTRL_SET_RX_CB,
                   vhf_uart_rx_cb);
    device_open(vhf_uart, DEV_FLAG_RDWR);

    idx = 0;

    while (1) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

        while (device_read(vhf_uart, 0, &byte, 1) == 1) {

            // 帧头检测
            if (byte == '$' || byte == '!') {
                idx = 0;
                is_ais_frame = (byte == '!');
                frame[idx++] = byte;

            } else if (byte == '\n' && idx > 0) {
                frame[idx] = '\0';

                // 分发给对应进程
                if (is_ais_frame) {
                    process_post(&ais_process,
                                 PROCESS_EVENT_POLL, NULL);
                } else if (strncmp(frame, "$CD", 3) == 0) {
                    process_post(&dsc_process,
                                 PROCESS_EVENT_POLL, NULL);
                }
                // 把帧内容放入对应进程的缓冲
                // 实际实现：用 memb 分配帧缓冲，传指针
                idx = 0;

            } else if (idx > 0 &&
                       idx < NMEA_MAX_SENTENCE_LEN - 1) {
                frame[idx++] = byte;
            }
        }
    }

    PROCESS_END();
}
```

------

## 第六步：EPIRB / SART GPIO 控制

```c
// service/beacon.c

#include "device/device.h"
#include "sys/process.h"
#include "sys/etimer.h"

// ── GPIO 控制命令 ─────────────────────────────────────
#define GPIO_CTRL_SET_HIGH   0x01
#define GPIO_CTRL_SET_LOW    0x02
#define GPIO_CTRL_GET_LEVEL  0x03

process_event_t beacon_activated_event;
process_event_t beacon_deactivated_event;

// ── 信标状态 ──────────────────────────────────────────
typedef struct {
    bool epirb_active;
    bool sart_active;
    bool plb_active;
} beacon_status_t;

static beacon_status_t beacon_status;

// ── 公共 API ──────────────────────────────────────────
void beacon_activate_epirb(void) {
    dev_t *gpio = device_find("gpio_epirb");
    if (gpio) device_control(gpio, GPIO_CTRL_SET_HIGH, NULL);
    beacon_status.epirb_active = true;
    process_post(PROCESS_BROADCAST,
                 beacon_activated_event, &beacon_status);
}

void beacon_activate_sart(void) {
    dev_t *gpio = device_find("gpio_sart");
    if (gpio) device_control(gpio, GPIO_CTRL_SET_HIGH, NULL);
    beacon_status.sart_active = true;
    process_post(PROCESS_BROADCAST,
                 beacon_activated_event, &beacon_status);
}

void beacon_deactivate_all(void) {
    dev_t *gpio;
    gpio = device_find("gpio_epirb");
    if (gpio) device_control(gpio, GPIO_CTRL_SET_LOW, NULL);
    gpio = device_find("gpio_sart");
    if (gpio) device_control(gpio, GPIO_CTRL_SET_LOW, NULL);

    memset(&beacon_status, 0, sizeof(beacon_status));
    process_post(PROCESS_BROADCAST,
                 beacon_deactivated_event, &beacon_status);
}

// ── 信标监控进程 ──────────────────────────────────────
PROCESS(beacon_process, "Beacon");

PROCESS_THREAD(beacon_process, ev, data) {
    static struct etimer poll_timer;
    static dev_t        *epirb_status_gpio;

    PROCESS_BEGIN();

    beacon_activated_event   = process_alloc_event();
    beacon_deactivated_event = process_alloc_event();

    // 监控 EPIRB 硬件状态引脚（有些 EPIRB 有状态输出）
    epirb_status_gpio = device_find("gpio_epirb_status");

    etimer_set(&poll_timer, CLOCK_SECOND);

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&poll_timer));
        etimer_reset(&poll_timer);

        if (epirb_status_gpio) {
            int level = 0;
            device_control(epirb_status_gpio,
                           GPIO_CTRL_GET_LEVEL, &level);
            if (level && !beacon_status.epirb_active) {
                // 硬件检测到 EPIRB 被手动激活
                beacon_status.epirb_active = true;
                process_post(PROCESS_BROADCAST,
                             beacon_activated_event,
                             &beacon_status);
            }
        }
    }

    PROCESS_END();
}
```

------

## 第七步：呼救决策进程（把所有东西串起来）

```c
// app/distress_manager.c

PROCESS(distress_manager_process, "Distress Manager");

PROCESS_THREAD(distress_manager_process, ev, data) {
    PROCESS_BEGIN();

    while (1) {
        PROCESS_WAIT_EVENT();

        // ── 收到 DSC 遇难呼叫 ──────────────────────
        if (ev == dsc_distress_event) {
            dsc_message_t *msg = (dsc_message_t *)data;

            // 记录日志
            LOG_WARN("DSC DISTRESS from MMSI:%s nature:%d\n",
                     msg->mmsi, msg->nature);

            // 如果有位置，检查是否在附近
            if (msg->has_position && gnss_data.valid) {
                float dist = calc_distance(
                    gnss_data.latitude,  gnss_data.longitude,
                    msg->latitude,       msg->longitude);

                if (dist < 50.0f) {  // 50海里内
                    // 激活 SART 协助搜救
                    beacon_activate_sart();
                    // 通过 4G 上报云端
                    comm_upload_distress(msg);
                }
            }
        }

        // ── 收到 AIS 新目标 ────────────────────────
        else if (ev == ais_new_target_event) {
            ais_target_t *target = (ais_target_t *)data;
            LOG_INF("AIS new: MMSI:%u\n", target->mmsi);
            // 可以做避碰预警
        }

        // ── 信标被激活 ─────────────────────────────
        else if (ev == beacon_activated_event) {
            beacon_status_t *st = (beacon_status_t *)data;
            LOG_WARN("Beacon activated! EPIRB:%d SART:%d\n",
                     st->epirb_active, st->sart_active);
            // 同步发送 DSC 遇难呼叫
            if (gnss_data.valid) {
                dsc_send_distress(
                    gnss_data.latitude,
                    gnss_data.longitude,
                    DSC_NATURE_UNDESIGNATED);
            }
            // 通过 4G 上报
            comm_upload_beacon_status(st);
        }
    }

    PROCESS_END();
}
```

------

## 完整文件结构

```text
service/
├── nmea_common.c/h        ← 校验和 / 字段分割 / 多片拼接
├── gnss.c/h               ← NMEA GGA/RMC 解析
├── ais_decoder.c/h        ← 6-bit 解码 + 消息解析
├── ais.c/h                ← AIS 进程 + 目标跟踪表
├── dsc.c/h                ← DSC 解析 + 发送
├── vhf_dispatcher.c/h     ← VHF UART 共享分发
└── beacon.c/h             ← EPIRB/SART GPIO 控制
```

你的 VHF 模块型号是什么？确认后可以把 `dsc_send_distress` 的指令格式对齐到具体模块。