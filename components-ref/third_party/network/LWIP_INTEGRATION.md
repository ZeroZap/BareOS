# LwIP TCP/IP 协议栈集成

**状态**: ⏳ 待集成  
**优先级**: 🔴 高

---

## 📦 库信息

- **名称**: LwIP (Lightweight IP)
- **版本**: 2.1.3
- **许可证**: BSD-3-Clause
- **官网**: https://savannah.nongnu.org/projects/lwip/
- **GitHub**: https://github.com/lwip-tcpip/lwip

---

## 🔧 集成步骤

### 1. 添加为 Git Submodule

```bash
cd components/third_party/network
git submodule add https://github.com/lwip-tcpip/lwip.git
cd ../../../
```

### 2. 配置 CMake

```cmake
# components/third_party/network/CMakeLists.txt

# LwIP 配置
set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lwip)
set(LWIP_CONTRIB_DIR ${LWIP_DIR}/contrib)

# LwIP 选项
set(LWIP_TCP 1)
set(LWIP_UDP 1)
set(LWIP_ICMP 1)
set(LWIP_DHCP 1)
set(LWIP_DNS 1)

# 添加 LwIP
add_subdirectory(${LWIP_DIR} lwip)

# LwIP 移植层
add_library(xy_lwip_port STATIC
    port/xy_lwip_arch.c
    port/xy_lwip_sys_arch.c
    port/xy_lwip_netif.c
)

target_include_directories(xy_lwip_port PUBLIC
    ${LWIP_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

target_link_libraries(xy_lwip_port PUBLIC lwip)
```

### 3. 创建移植层

```c
/* components/third_party/network/port/xy_lwip_arch.c */

#include "lwip/arch.h"
#include "xy_os.h"

/* 系统初始化 */
void xy_lwip_init(void)
{
    lwip_init();
}

/* 获取系统时间 (ms) */
u32_t sys_now(void)
{
    return xy_os_tick_get();
}

/* 临界区保护 */
sys_prot_t sys_arch_protect(void)
{
    xy_os_enter_critical();
    return 1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    xy_os_exit_critical();
}
```

### 4. 网络接口层

```c
/* components/third_party/network/port/xy_lwip_netif.c */

#include "lwip/netif.h"
#include "lwip/ethip6.h"
#include "netif/ethernet.h"
#include "xy_hal.h"

static err_t xy_netif_init(struct netif *netif)
{
    /* 初始化 MAC */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x11;
    netif->hwaddr[2] = 0x22;
    netif->hwaddr[3] = 0x33;
    netif->hwaddr[4] = 0x44;
    netif->hwaddr[5] = 0x55;
    
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    
    netif->set_linkoutput = xy_netif_set_linkoutput;
    netif->output = etharp_output;
    
    return ERR_OK;
}

err_t xy_netif_set_linkoutput(struct netif *netif, struct pbuf *p)
{
    /* 调用底层驱动发送数据包 */
    xy_eth_send(p->payload, p->len);
    return ERR_OK;
}

/* 初始化网络接口 */
struct netif* xy_netif_add(void)
{
    static struct netif netif;
    
    IP4_ADDR(&netif.ip_addr, 192, 168, 1, 100);
    IP4_ADDR(&netif.netmask, 255, 255, 255, 0);
    IP4_ADDR(&netif.gw, 192, 168, 1, 1);
    
    netif_add(&netif, NULL, NULL, NULL, NULL, xy_netif_init, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);
    
    return &netif;
}
```

### 5. 使用示例

```c
#include "xy_lwip.h"

int main(void)
{
    /* 初始化系统 */
    xy_os_kernel_init();
    
    /* 初始化 LwIP */
    xy_lwip_init();
    
    /* 添加网络接口 */
    xy_netif_add();
    
    /* 启动网络任务 */
    xy_lwip_start_task();
    
    /* 使用 Socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, ...);
    send(sock, "Hello", 5, 0);
    
    xy_os_kernel_start();
}
```

---

## 📊 集成检查清单

- [ ] 添加 Git Submodule
- [ ] 配置 CMake
- [ ] 创建移植层
- [ ] 实现网络接口
- [ ] 测试 TCP 连接
- [ ] 测试 UDP 通信
- [ ] 测试 DHCP
- [ ] 测试 DNS
- [ ] 添加文档

---

## 🎯 集成时间表

| 阶段 | 任务 | 工时 |
|------|------|------|
| **阶段 1** | 添加 Submodule + 配置 | 2h |
| **阶段 2** | 移植层实现 | 4h |
| **阶段 3** | 网络接口实现 | 4h |
| **阶段 4** | 测试 + 文档 | 2h |
| **总计** | - | **12h** |

---

## 📚 相关文档

- [LwIP 官方文档](https://savannah.nongnu.org/projects/lwip/)
- [LwIP API 参考](https://www.nongnu.org/lwip/2_1_x/)
- [LwIP 移植指南](https://www.nongnu.org/lwip/2_1_x/group__lwip__os.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0 (XY) + BSD-3 (LwIP)
