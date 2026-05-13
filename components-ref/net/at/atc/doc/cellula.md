我们将增加4G模块常用的接口解析，包括信号强度查询、网络注册状态、IP地址获取、短信处理、TCP/IP通信等。我们将基于之前的框架，添加这些特定的功能接口。

注意：由于AT命令集因模块而异，这里以常见的SIMCOM系列（如SIM7600）为例，其他模块可能需要调整。

我们将添加以下功能：

1. 信号强度查询（CSQ）
2. 网络注册状态（CREG）
3. 获取模块IP地址（CGPADDR）
4. 短信发送和读取（CMGF, CMGS, CMGR）
5. TCP/IP连接（CIPSTART, CIPCLOSE, CIPSEND）
6. 获取网络时间（CCLK）
7. 获取模块IMEI（CGSN）

我们将在at_client.h和at_client.c的基础上，增加一个at_4g.h和at_4g.c文件，专门用于4G模块的常用命令封装。

注意：我们不会改变原有框架，而是以扩展的方式提供这些功能，用户可以选择是否使用这些特定功能。

我来为AT Client框架增加4G模块常用的接口解析。这里以SIM7600、EC200等常见4G模块为例，添加完整的4G通信接口。



## 主要特性

### 1. **完整的4G模块功能覆盖**

- **基础功能**：模块信息、SIM卡状态、网络注册
- **通信功能**：TCP/UDP、HTTP、MQTT、短信
- **增值功能**：GPS定位、数据统计、低功耗模式

### 2. **模块兼容性**

- 支持SIM7600、EC200、BG96等主流模块
- 自动检测模块类型，适配不同AT命令集
- 统一的API接口，模块间可无缝切换

### 3. **协议支持**

- **TCP/UDP Socket**：原生Socket接口
- **HTTP客户端**：GET/POST请求
- **MQTT客户端**：发布/订阅模式
- **短信服务**：收发短信、编码转换

### 4. **事件驱动架构**

- 网络状态变化URC
- 短信到达通知URC
- Socket数据接收URC
- 连接状态变化URC

### 5. **错误处理与恢复**

- 命令执行状态返回
- 网络重连机制
- 数据完整性校验
- 超时重试机制

## 使用建议

### 1. **初始化流程**

c

```
1. at_device_register() 注册设备
2. at_4g_init() 初始化4G模块
3. at_4g_check_sim() 检查SIM卡
4. at_4g_get_network_status() 等待网络注册
5. at_4g_activate_pdp() 激活数据业务
6. at_4g_get_ip_address() 获取IP地址
```



### 2. **URC处理策略**

- 网络状态URC：更新连接状态，触发重连
- 短信URC：异步读取，避免阻塞主线程
- Socket数据URC：使用缓冲区，批量处理

### 3. **性能优化**

- 批量发送数据，减少AT命令数量
- 使用连接池管理Socket连接
- 合理设置超时时间，平衡响应速度与可靠性

### 4. **资源管理**

- 及时关闭不需要的连接
- 监控数据使用量，避免超额
- 合理设置心跳间隔，平衡功耗

### 5. **错误恢复**

- 网络断开时自动重连
- SIM卡错误时提示用户
- 模块异常时软重启


## 蜂窝网支持的协议和典型应用场景

**支持的协议列表：**
1. HTTP/HTTPS - 通用Web请求，API调用，数据上报
2. MQTT - 物联网实时消息推送，设备控制
3. FTP - 文件传输，固件升级，大数据上传
4. NTP - 时间同步，日志时间戳
5. SSL/TLS - 安全通信，证书验证
6. DNS - 域名解析，网络诊断
7. PING - 网络连通性测试
8. SMTP - 邮件发送，报警通知
9. WebSocket - 实时双向通信（需要自己实现协议）

**典型应用场景：**
1. 智能家居 - MQTT控制 + HTTPS备份 + SMTP报警
2. 车联网 - GPS + MQTT实时位置 + FTP日志 + HTTPS上报
3. 工业物联网 - MQTT实时数据 + FTP历史数据 + SMTP报警
4. 远程医疗 - HTTPS安全上报 + MQTT指令 + SMS紧急通知
5. 农业物联网 - MQTT环境数据 + FTP图片上传 + 自动控制
6. 安防监控 - FTP视频上传 + MQTT报警 + HTTPS云存储

**电源管理集成：**
所有协议都支持在低功耗模式下工作，可以根据：
- 信号强度调整传输功率
- 数据量选择传输协议
- 电池电量调整工作模式
- 时间计划调整睡眠周期

# 协议选择建议

### 1. **按数据类型选择协议**

``` c
/* 小数据、高频次：MQTT (QoS 0) */
if (data_size < 1024 && update_freq > 1/60) {
    use_mqtt();
}

/* 中等数据、中频次：HTTPS */
else if (data_size < 10*1024 && update_freq > 1/3600) {
    use_https();
}

/* 大数据、低频次：FTP */
else if (data_size > 10*1024) {
    use_ftp();
}

/* 实时双向通信：WebSocket */
else if (need_bidirectional && realtime) {
    use_websocket();
}
```



### 2. **按网络条件选择协议**

``` c
/* 信号好：使用原始协议 */
if (rssi > 20) {
    // 使用协议本身
}

/* 信号中等：使用压缩和重试 */
else if (rssi > 10) {
    compress_data();
    enable_retry();
}

/* 信号差：使用精简协议或存储转发 */
else {
    if (data_size > 512) {
        store_and_forward();  /* 存储转发 */
    } else {
        use_udp_with_retry();  /* UDP+重试 */
    }
}
```



### 3. **按安全要求选择协议**

``` c
/* 高安全性：HTTPS + 双向认证 */
if (security_level == HIGH) {
    use_https_with_mutual_auth();
}

/* 中等安全：HTTPS + 服务器验证 */
else if (security_level == MEDIUM) {
    use_https_server_auth();
}

/* 低安全：HTTP或MQTT */
else {
    use_http_or_mqtt();
}
```
### 4. **完整的多协议应用架构**

这个框架提供了完整的4G模块多协议支持，从基础的TCP/UDP到高级的HTTPS、MQTT、FTP、NTP等协议，可以满足各种物联网应用的需求。关键特性包括：

1. **协议抽象层**：统一接口，简化使用
2. **智能协议选择**：根据网络条件和数据类型自动选择最佳协议
3. **错误恢复机制**：自动重连、协议降级、存储转发
4. **安全通信**：SSL/TLS支持，证书管理
5. **功耗优化**：与电源管理框架深度集成
6. **扩展性强**：易于添加新的协议支持

每个协议都经过优化，考虑了4G模块的特性（如有限的内存、AT命令的延迟等），可以在资源受限的环境中高效运行。


## 主要增强特性

1. **动态URC注入**
   - 运行时动态注册/注销URC处理器
   - 支持多种匹配模式（前缀、后缀、包含、精确、正则）
   - 每个URC处理器可独立启用/禁用
2. **多设备支持**
   - 每个设备独立的URC管理器
   - 处理器链表结构，动态扩展
   - 独立的统计信息
3. **平台适配**
   - RTOS环境：独立的URC处理任务
   - 裸机环境：轮询处理机制
   - 统一的接口抽象
4. **高级功能**
   - URC自动应答
   - 条件URC处理
   - 批量URC注册
   - URC到事件转换
5. **错误处理**
   - URC处理器异常保护
   - 处理状态跟踪
   - 统计信息记录

## 使用建议

1. **URC设计原则**
   - 保持URC处理器简洁，避免长时间阻塞
   - 处理器中不要发送AT命令，防止死锁
   - 复杂处理应该发送事件，由主任务处理
2. **性能优化**
   - 按使用频率排序URC处理器
   - 禁用不常用的URC处理器
   - 使用精确匹配减少字符串比较
3. **内存管理**
   - 根据实际需要调整URC链表大小
   - 及时注销不再需要的URC处理器
   - 考虑使用内存池分配
4. **扩展性**
   - 可扩展新的匹配算法
   - 支持优先级处理
   - 支持URC过滤器链

这个框架提供了强大的URC处理能力，可以方便地集成到各种物联网设备、通信模块等场景中，实现灵活的事件驱动架构。

## 电源控制架构设计

### 1. **三层电源管理架构**



``` text
应用层 (Application)
    ├── 场景模式（性能/均衡/省电/超级省电）
    ├── 功耗分析
    └── 故障恢复

业务层 (Service)
    ├── 4G模块专用电源管理
    ├── 智能功率调整
    ├── 睡眠/唤醒调度
    └── 紧急处理

硬件层 (Hardware)
    ├── GPIO控制（PWR_KEY, RESET, WAKEUP）
    ├── 电压/温度监控
    └── 充电管理
```



### 2. **主要电源模式**

| 模式         | 描述                 | 适用场景             |
| :----------- | :------------------- | :------------------- |
| **性能模式** | 全功率运行，最快响应 | 实时视频、大数据传输 |
| **均衡模式** | 智能功耗管理         | 常规物联网应用       |
| **省电模式** | PSM + eDRX           | 电池供电设备         |
| **超级省电** | 深度睡眠 + 定时唤醒  | 太阳能/NB-IoT        |

### 3. **关键电源控制点**

c

```
/* 典型的4G模块电源控制流程 */
1. 开机: PWR_KEY拉低2秒 → 等待5-10秒启动
2. 关机: PWR_KEY拉低2秒 → 等待关机完成
3. 复位: RESET拉低1秒 → 等待重启完成
4. 唤醒: WAKEUP脉冲100ms → 发送AT命令
5. 睡眠: 发送AT+CSCLK=2 → 硬件进入低功耗
```



### 4. **功耗优化策略**

c

```
/* 智能功耗优化 */
1. 根据信号强度调整发射功率
2. 根据数据量调整DRX周期
3. 根据电池电量调整工作模式
4. 根据环境温度调整最大功率
5. 根据网络负载调整连接参数
```



## 使用建议

### 1. **电源管理初始化顺序**

c

```
1. 注册AT设备
2. 初始化电源管理（配置GPIO和回调）
3. 开机模块（等待启动完成）
4. 初始化4G功能
5. 配置省电参数
6. 启动自动电源管理
```



### 2. **回调函数实现示例**



``` c
/* 实际硬件控制实现 */
int real_hardware_pwr_ctrl(at_device_t *dev, pwr_cmd_t cmd, void *param) {
    switch (cmd) {
        case PWR_CMD_POWER_ON:
            HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_RESET);
            HAL_Delay(2000);
            HAL_GPIO_WritePin(PWR_KEY_GPIO_Port, PWR_KEY_Pin, GPIO_PIN_SET);

            /* 可以添加状态检测 */
            while (!HAL_GPIO_ReadPin(STATUS_GPIO_Port, STATUS_Pin)) {
                HAL_Delay(100);
            }
            break;

        case PWR_CMD_SLEEP:
            /* 进入睡眠前保存状态 */
            save_current_state();

            /* 发送睡眠命令 */
            uart_send("AT+CSCLK=2\r\n");

            /* 关闭不必要的电源域 */
            disable_peripheral_power();
            break;
    }
    return 0;
}
```



### 3. **异常处理**



``` c
/* 电源异常处理流程 */
void power_fault_handler(at_device_t *dev, int error_code) {
    switch (error_code) {
        case PWR_FAULT_OVERVOLTAGE:
            printf("Overvoltage detected!\n");
            at_power_control(dev, PWR_CMD_POWER_OFF, NULL);
            break;

        case PWR_FAULT_OVERTEMP:
            printf("Over temperature: %d°C\n", read_temperature());
            at_4g_set_tx_power(dev, 1);  /* 降低发射功率 */
            break;

        case PWR_FAULT_BATTERY_LOW:
            printf("Battery low, entering emergency mode\n");
            emergency_power_management(dev);
            break;
    }
}
```



### 4. **功耗分析工具**


``` c
/* 功耗分析和优化建议 */
void power_analysis_report(at_device_t *dev) {
    float avg_current, battery_life;

    at_power_analyze(dev, &avg_current, &battery_life);

    printf("Power Analysis Report:\n");
    printf("  Average Current: %.2f mA\n", avg_current);
    printf("  Estimated Battery Life: %.1f hours\n", battery_life);
    printf("  Power Mode: %d\n", get_power_mode(dev));
    printf("  Optimization Suggestions:\n");

    if (avg_current > 100) {
        printf("    - Consider enabling PSM\n");
        printf("    - Reduce data transmission frequency\n");
    }

    if (battery_life < 24) {
        printf("    - Consider larger battery capacity\n");
        printf("    - Add solar charging capability\n");
    }
}
```

## 电源控制使用指南
1. **开关机控制**：许多4G模块需要通过拉低某个GPIO一段时间来实现开关机。
2. **休眠唤醒**：为了省电，模块可能支持休眠模式，需要通过GPIO或AT命令进行唤醒。
3. **复位控制**：异常情况下可能需要硬件复位。

因此，我们可以在AT设备的结构体中增加电源控制回调函数，并在适当的时候调用它。

我们设计一个电源控制回调函数，可以用于控制设备的电源、复位、休眠等。回调函数可以接收一个参数，表示具体的电源操作（如开机、关机、休眠、唤醒等）。

让我们修改AT设备结构体，增加电源控制回调和相关参数。同时，为了兼容性，我们也可以考虑在设备私有数据中存储电源控制相关的GPIO等信息。

具体修改如下：

1. 在AT设备结构体中增加电源控制回调函数指针。
2. 增加一个电源控制参数，用于传递电源操作类型。
3. 在AT设备注册函数中，允许用户传入电源控制回调函数。

但是，考虑到电源控制可能涉及硬件操作，且不同模块的电源控制方式不同（有的通过GPIO，有的通过AT命令），我们可以将电源控制设计为一个回调函数，由用户实现具体的硬件操作。

同时，我们也可以将电源控制集成到4G模块的专用接口中，因为4G模块对电源控制的需求更常见。

我们先修改通用的AT设备结构体，然后为4G模块增加电源控制接口。

注意：由于电源控制可能不是所有AT设备都需要，我们可以将其作为可选功能，回调函数可以为NULL。

修改at_client.h中的at_device结构体，增加电源控制回调：

c

``` c
/* 电源控制操作类型 */
typedef enum {
    AT_PWR_OFF = 0,     /* 关机 */
    AT_PWR_ON,          /* 开机 */
    AT_PWR_RESET,       /* 复位 */
    AT_PWR_SLEEP,       /* 进入休眠 */
    AT_PWR_WAKEUP,      /* 唤醒 */
    AT_PWR_LOW_POWER    /* 进入低功耗模式 */
} at_pwr_cmd_t;

/* 电源控制回调函数类型 */
typedef int (*at_pwr_ctrl_t)(at_device_t *dev, at_pwr_cmd_t cmd);

/* 在at_device结构体中增加电源控制回调 */
struct at_device {
    /* ... 原有成员 ... */

    /* 电源控制 */
    at_pwr_ctrl_t power_ctrl;   /* 电源控制回调函数 */

    /* ... 其他成员 ... */
};
```



然后，在at_device_register函数中增加电源控制回调参数。但是，为了保持向后兼容，我们可以先不修改注册函数，而是通过一个单独的设置函数来设置电源控制回调。

我们添加一个函数：

c

``` c
/**
 * @brief 设置AT设备的电源控制回调
 */
int at_device_set_power_ctrl(at_device_t *dev, at_pwr_ctrl_t pwr_ctrl);
```



另外，在4G模块的初始化函数中，我们可以使用这个电源控制回调来实现模块的开关机等操作。

对于4G模块，我们还可以在at_4g_init函数中增加一个步骤：如果提供了电源控制回调，则先开机模块。因为有些模块可能默认是关机状态。

下面，我们修改代码，添加电源控制相关的内容。

由于修改较多，我将主要修改部分列出：

1. 在at_client.h中添加电源控制相关的定义和函数声明。
2. 在at_client.c中实现at_device_set_power_ctrl函数。
3. 在at_4g.c中，在at_4g_init函数中使用电源控制回调（如果存在）来确保模块开机。

此外，我们还可以为4G模块提供专用的电源控制函数，例如：

c

``` c
/**
 * @brief 4G模块电源控制
 */
int at_4g_power_ctrl(at_device_t *dev, at_pwr_cmd_t cmd);
```



这个函数内部会调用设备注册的电源控制回调，如果不存在，则尝试使用默认的AT命令方式（例如，对于关机，发送AT+CPOWD=1）。

但是，注意：有些电源操作可能无法通过AT命令实现，比如开机，因为模块已经关机，无法响应AT命令。所以硬件电源控制回调是必要的。

我们优先使用硬件电源控制回调，如果没有提供，则尝试使用AT命令（仅对可以通过AT命令实现的操作，如关机、休眠、唤醒等）。

下面，我们开始修改代码。

您提出了一个非常重要的功能需求！电源控制回调对于4G/LTE模块是必需的，因为：

1. **硬件开关机控制**：模块需要外置电源控制引脚（如PWR_KEY）
2. **多种睡眠模式**：深度睡眠、轻度睡眠等
3. **复位控制**：硬件复位、软件复位
4. **低功耗管理**：根据应用场景切换功耗模式

让我来完善这个框架，添加完整的电源管理功能：