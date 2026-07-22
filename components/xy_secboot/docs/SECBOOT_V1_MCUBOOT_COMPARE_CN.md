# SecBoot V1 与 MCUboot 对比及改进建议

本文记录当前 SecBoot V1 设计与 MCUboot 的差异、适用边界，以及从 MCUboot 成熟设计中可吸收的改进项。

## 1. 当前 SecBoot V1 定位

SecBoot V1 是面向资源受限 MCU 的单槽串口恢复 bootloader。

当前核心目标：

```text
Bootloader 常驻并可恢复
UART V1 接收 signed package
MANIFEST/DATA/END 流程可靠
END 校验通过后才提交 manifest
复位后验证 manifest 和 App image 后跳 App
App 通过 SRAM mailbox 请求 confirmed
Bootloader 写 confirmed state
rollback counter 防旧包回退
```

当前 V1 不是完整 MCUboot 替代品，也不是完整 A/B OTA 框架。它更接近一个轻量 single-slot secure recovery bootloader。

## 2. 当前 V1 分区模型

当前 V1 使用独立 Flash 区域保存不同生命周期的数据：

| 区域 | 作用 | 是否运行时写入 |
|---|---|---:|
| Bootloader | recovery 和验签启动入口 | 否，外部烧录器写入 |
| Boot config A/B | bootloader 配置冗余副本 | 可选，更新 inactive 副本 |
| Boot state | pending/confirmed/attempts | 是 |
| Rollback | 已接受最高 security counter | 是 |
| App manifest | 当前可启动 App 的 manifest | END 校验通过后写 |
| App image | 当前 App 镜像 | DATA 阶段写 |
| Reserved tail | App 参数、FEE、EEPROM 等 | SecBoot 不写 |
| SRAM mailbox | App 请求 confirmed | App 写，Bootloader 清 |

## 3. MCUboot 的典型模型

MCUboot 的核心是 image slot 和 slot trailer。

典型布局：

```text
primary slot
  image header
  image payload
  protected TLV
  regular TLV
  trailer

secondary slot
  image header
  image payload
  protected TLV
  regular TLV
  trailer

scratch slot 可选
```

MCUboot 通过 slot trailer 管理升级状态：

| Trailer 字段 | 作用 |
|---|---|
| `magic` | 标记 slot 中存在可处理镜像 |
| `image_ok` | App 已确认，可永久运行 |
| `copy_done` | copy/swap 已完成 |
| `swap_type` | test/permanent/revert 等动作 |
| `status` | sector-level swap 进度 |

MCUboot 的强项是多 slot、swap、revert 和升级中断恢复。

## 4. 对比分析

| 对比项 | SecBoot V1 | MCUboot |
|---|---|---|
| 主要目标 | 单槽串口恢复和轻量 secure boot | 通用安全升级 bootloader |
| App slot | 当前单槽 | primary/secondary 常见双槽 |
| 升级方式 | UART streaming 直接写 App 区 | secondary 下载后 swap/copy/overwrite |
| 元数据位置 | 独立 manifest/state/rollback/config 页 | image header/TLV/trailer |
| confirmed 方式 | App 写 SRAM mailbox，Bootloader 写 state | App 写 `image_ok` trailer |
| rollback | 独立 rollback page | security counter，存储依平台 |
| 掉电恢复 | manifest 最后提交，避免半包启动 | trailer/status 支持 swap 过程恢复 |
| Bootloader 自升级 | V1 未做 | 可通过多镜像/特定集成实现 |
| 配置冗余 | 可用 Boot config A/B | MCUboot 本身不以 boot config A/B 为核心 |
| 复杂度 | 低 | 中到高 |

## 5. Boot Config A/B 与 MCUboot Trailer 的关系

Boot config A/B 不是 App A/B slot，也不是 MCUboot trailer 的直接替代。

Boot config A/B 适合保存：

```text
product_id
layout version
UART 参数
recovery timeout
security suite
feature flags
factory/production policy
WRP/RDP 计划状态
```

MCUboot trailer 适合保存：

```text
image_ok
copy_done
swap_type
swap status
是否需要 revert
```

两者可以共存：

```text
Boot config A/B 管 bootloader 策略配置
Manifest/state/rollback 管当前镜像安全状态
如果未来做 App A/B，再引入 slot trailer 或类似 trailer 的 slot metadata
```

## 6. Boot Config A/B 是否成熟

成熟。它属于嵌入式常见的 dual-copy metadata / ping-pong page 方案。

推荐规则：

```text
两份配置放不同 Flash erase page
启动时同时读取 A/B
校验 magic、format version、seq inverse、CRC
两份都有效时选择 seq 最新的一份
更新时只擦写 inactive 副本
写完读回校验成功后，新副本成为 active
绝不先擦 active 副本
```

推荐 header：

```c
typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t seq;
    uint32_t seq_inv;
    uint32_t payload_len;
    uint32_t payload_crc32;
    uint32_t header_crc32;
} xy_secboot_cfg_hdr_t;
```

payload 建议使用 TLV 或固定 struct。若配置会影响安全策略，应增加 HMAC/signature 或放入硬件保护区。

## 7. 当前 V1 可改进项

### 7.1 增加 Boot Config A/B 组件

当前 portgen 已能生成 Boot config A/B layout，`xy_secboot_bootcfg` 已提供通用 A/B 配置读写组件。

建议新增组件：

```text
components/xy_secboot/inc/xy_secboot_bootcfg.h
components/xy_secboot/src/xy_secboot_bootcfg.c
```

能力：

```text
读 A/B 两份配置
验证 magic/version/seq/inverse/CRC
选择最新有效配置
写 inactive 副本
payload 写完后最后写 header
掉电后恢复到旧有效副本
两份都坏时由上层回退编译期默认配置或进入 recovery
```

### 7.2 把 Boot State 也升级为 A/B 或 record page

当前 boot state 已是 record/page 方式，能处理 seq rollover。后续可以统一成 bootcfg 风格：

```text
header + seq + inverse + payload CRC
active/inactive page
明确状态机字段
```

目标是让 state、rollback、bootcfg 的元数据格式一致，降低维护成本。

### 7.3 Rollback Counter 增强

当前 rollback page 能记录 counter，但生产安全仍依赖 Flash 保护。

建议：

```text
优先使用 OTP/eFuse/hardware monotonic counter
如果只能用 Flash，必须放 bootloader-private 区
记录增加 inverse 和 CRC
禁止 App 擦写 rollback 区
```

### 7.4 Manifest 与 State 关系明确化

当前 manifest 最后提交是正确的。后续建议更明确地区分：

```text
manifest = 当前镜像身份
state = 当前镜像生命周期状态
rollback = 安全单调计数
bootcfg = bootloader 策略配置
```

避免把这些数据混在一个 Flash page 里。

### 7.5 UART V1 稳定性优化

测试中出现过 `BAD_HEADER_CRC`、`BAD_PAYLOAD_CRC` 和 `drop`。

建议：

```text
默认 payload 调整到 128
增大 UART RX ring buffer
Flash 写入期间主动 drain/poll UART
错帧后增加 resync 逻辑
host 端增加 adaptive backoff
保留 retries=20 timeout=2000ms 的稳妥配置
```

### 7.6 明确 Factory Lock 流程

WRP/RDP 抽象已完成，但是真实硬件保护，开发阶段应跳过 apply。

建议新增独立 factory-lock 文档和工具入口：

```text
读取当前 option bytes
确认 bootloader/state/rollback/bootcfg 保护范围
确认 RDP level
二次确认不可逆风险
执行 WRP/RDP
复位后只做只读验证
```

普通 bootloader 启动路径不得自动执行 WRP/RDP apply。

### 7.7 生产签名方案替换 Dev HMAC

当前 dev HMAC 只适合 bring-up。

生产前必须确定：

```text
公钥签名或安全芯片验签
root key/public key 存储位置
key rotation 策略
manifest signed fields 固化
算法裁剪和合规要求
```

### 7.8 如未来做 App A/B，再参考 MCUboot Trailer

当前不做 App A/B。若后续做真正 A/B OTA，建议吸收 MCUboot 的 slot trailer 思路：

```text
primary/secondary slot
slot trailer magic
image_ok
copy_done
swap_type
sector status
revert policy
```

不要用 Boot config A/B 替代 App A/B trailer。两者职责不同。

## 8. 建议的 V1 后续路线

推荐按风险和收益排序：

1. 实现 `xy_secboot_bootcfg`，支持 Boot config A/B 读写和掉电恢复。
2. 更新 portgen 中文指导书，把 bootcfg 运行时代码接入方式写清楚。
3. 优化 UART V1 默认参数和 RX buffer，降低 NACK/drop。
4. 统一 state/rollback/config record header 风格。
5. 编写 factory-lock 文档，保留真实 WRP/RDP apply 跳过策略。
6. 替换 dev HMAC，确定生产签名/验签方案。
7. 如产品需要完整 OTA A/B，再设计 App A/B slot 和 trailer。

## 9. 当前结论

SecBoot V1 当前架构适合继续走轻量 single-slot recovery bootloader 路线。

从 MCUboot 可以吸收的不是立即引入 App A/B，而是：

```text
明确元数据职责
所有关键状态可恢复
写入状态分阶段提交
confirmed/revert 语义清晰
security counter 不可回退
```

Boot config A/B 是成熟且适合 V1 的改进点；App A/B slot 则应作为后续 OTA 架构升级单独设计。
