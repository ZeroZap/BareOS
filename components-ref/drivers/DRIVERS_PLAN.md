# Drivers 组件实现计划

**日期**: 2026-03-05  
**目标**: 分析现有驱动，制定实现计划

---

## 📊 当前 Drivers 状态

### 已完成驱动

| 驱动 | 状态 | 说明 |
|------|------|------|
| **display/** | ✅ 完成 | LCD/LED 显示驱动 |
| **hal/** | ✅ 完成 | 硬件抽象层 |

### 待实现驱动

| 驱动 | 优先级 | 预计工时 | 说明 |
|------|--------|---------|------|
| **adc/** | 🔴 高 | 4 小时 | ADC 驱动 (内部/外部) |
| **dac/** | 🔴 高 | 2 小时 | DAC 驱动 |
| **pwm/** | 🔴 高 | 2 小时 | PWM 驱动 |
| **timer/** | 🟡 中 | 3 小时 | 定时器驱动 |
| **wdt/** | 🟡 中 | 1 小时 | 看门狗驱动 |
| **rtc/** | 🟡 中 | 2 小时 | RTC 驱动 |
| **can/** | 🟡 中 | 4 小时 | CAN 总线驱动 |
| **i2s/** | 🟢 低 | 3 小时 | I2S 音频驱动 |
| **sdio/** | 🟢 低 | 4 小时 | SDIO 驱动 |
| **ethernet/** | 🟢 低 | 6 小时 | 以太网驱动 |

---

## 📋 详细实现计划

### 阶段 1: 基础模拟驱动 (8 小时) ⭐

#### 1.1 ADC 驱动 (4 小时)

**文件结构**:
```
drivers/adc/
├── inc/
│   ├── xy_adc.h          # 统一 ADC 接口
│   ├── xy_adc_internal.h # 内部 ADC
│   └── xy_adc_external.h # 外部 ADC (MCP3008 等)
├── src/
│   ├── xy_adc.c
│   ├── xy_adc_internal.c
│   └── xy_adc_external.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_adc_status_t xy_adc_init(xy_adc_t *adc, xy_adc_config_t *cfg);

// 单次转换
xy_adc_result_t xy_adc_convert(xy_adc_t *adc, uint8_t channel);

// 连续转换
xy_adc_result_t xy_adc_convert_continuous(xy_adc_t *adc, 
                                           uint8_t channel,
                                           uint16_t *buffer,
                                           uint16_t count);

// 获取电压 (mV)
uint16_t xy_adc_get_voltage_mv(xy_adc_t *adc, uint8_t channel);
```

**支持芯片**:
- ✅ STM32 (内部 ADC)
- ✅ WCH (内部 ADC)
- ✅ MCP3008/3208 (外部 SPI ADC)
- ✅ ADS1115 (外部 I2C ADC)

---

#### 1.2 DAC 驱动 (2 小时)

**文件结构**:
```
drivers/dac/
├── inc/
│   └── xy_dac.h
├── src/
│   └── xy_dac.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_dac_status_t xy_dac_init(xy_dac_t *dac, xy_dac_config_t *cfg);

// 设置电压 (mV)
xy_dac_result_t xy_dac_set_voltage_mv(xy_dac_t *dac, uint16_t mv);

// 设置原始值
xy_dac_result_t xy_dac_set_value(xy_dac_t *dac, uint16_t value);

// 输出波形
xy_dac_result_t xy_dac_output_waveform(xy_dac_t *dac, 
                                        xy_dac_waveform_t type,
                                        uint16_t freq);
```

---

#### 1.3 PWM 驱动 (2 小时)

**文件结构**:
```
drivers/pwm/
├── inc/
│   └── xy_pwm.h
├── src/
│   └── xy_pwm.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_pwm_status_t xy_pwm_init(xy_pwm_t *pwm, xy_pwm_config_t *cfg);

// 设置占空比 (0-100%)
xy_pwm_result_t xy_pwm_set_duty(xy_pwm_t *pwm, uint8_t duty);

// 设置频率
xy_pwm_result_t xy_pwm_set_frequency(xy_pwm_t *pwm, uint32_t freq);

// 启动/停止
xy_pwm_result_t xy_pwm_start(xy_pwm_t *pwm);
xy_pwm_result_t xy_pwm_stop(xy_pwm_t *pwm);
```

---

### 阶段 2: 定时器与系统驱动 (6 小时)

#### 2.1 定时器驱动 (3 小时)

**文件结构**:
```
drivers/timer/
├── inc/
│   └── xy_timer.h
├── src/
│   └── xy_timer.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_timer_status_t xy_timer_init(xy_timer_t *timer, xy_timer_config_t *cfg);

// 启动定时器
xy_timer_result_t xy_timer_start(xy_timer_t *timer);

// 停止定时器
xy_timer_result_t xy_timer_stop(xy_timer_t *timer);

// 获取计数值
uint32_t xy_timer_get_count(xy_timer_t *timer);

// 设置回调
xy_timer_result_t xy_timer_set_callback(xy_timer_t *timer,
                                         xy_timer_callback_t cb,
                                         void *user_data);
```

---

#### 2.2 看门狗驱动 (1 小时)

**文件结构**:
```
drivers/wdt/
├── inc/
│   └── xy_wdt.h
├── src/
│   └── xy_wdt.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_wdt_status_t xy_wdt_init(xy_wdt_t *wdt, xy_wdt_config_t *cfg);

// 喂狗
xy_wdt_result_t xy_wdt_feed(xy_wdt_t *wdt);

// 启动看门狗
xy_wdt_result_t xy_wdt_start(xy_wdt_t *wdt);
```

---

#### 2.3 RTC 驱动 (2 小时)

**文件结构**:
```
drivers/rtc/
├── inc/
│   └── xy_rtc.h
├── src/
│   └── xy_rtc.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_rtc_status_t xy_rtc_init(xy_rtc_t *rtc, xy_rtc_config_t *cfg);

// 设置时间
xy_rtc_result_t xy_rtc_set_time(xy_rtc_t *rtc, xy_rtc_time_t *time);

// 获取时间
xy_rtc_result_t xy_rtc_get_time(xy_rtc_t *rtc, xy_rtc_time_t *time);

// 设置闹钟
xy_rtc_result_t xy_rtc_set_alarm(xy_rtc_t *rtc, xy_rtc_time_t *alarm);
```

---

### 阶段 3: 通信驱动 (10 小时)

#### 3.1 CAN 驱动 (4 小时)

**文件结构**:
```
drivers/can/
├── inc/
│   └── xy_can_bus.h
├── src/
│   └── xy_can_bus.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_can_status_t xy_can_init(xy_can_t *can, xy_can_config_t *cfg);

// 发送帧
xy_can_result_t xy_can_send(xy_can_t *can, xy_can_frame_t *frame);

// 接收帧
xy_can_result_t xy_can_receive(xy_can_t *can, xy_can_frame_t *frame,
                                uint32_t timeout);

// 设置过滤器
xy_can_result_t xy_can_set_filter(xy_can_t *can, xy_can_filter_t *filter);
```

---

#### 3.2 I2S 驱动 (3 小时)

**文件结构**:
```
drivers/i2s/
├── inc/
│   └── xy_i2s.h
├── src/
│   └── xy_i2s.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_i2s_status_t xy_i2s_init(xy_i2s_t *i2s, xy_i2s_config_t *cfg);

// 发送数据
xy_i2s_result_t xy_i2s_send(xy_i2s_t *i2s, int16_t *buffer, uint16_t len);

// 接收数据
xy_i2s_result_t xy_i2s_receive(xy_i2s_t *i2s, int16_t *buffer, uint16_t len);

// DMA 传输
xy_i2s_result_t xy_i2s_transfer_dma(xy_i2s_t *i2s,
                                     int16_t *tx_buf,
                                     int16_t *rx_buf,
                                     uint16_t len);
```

---

#### 3.3 SDIO 驱动 (4 小时)

**文件结构**:
```
drivers/sdio/
├── inc/
│   └── xy_sdio.h
├── src/
│   └── xy_sdio.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_sdio_status_t xy_sdio_init(xy_sdio_t *sdio, xy_sdio_config_t *cfg);

// 读块
xy_sdio_result_t xy_sdio_read_blocks(xy_sdio_t *sdio, uint32_t block,
                                      uint8_t *buffer, uint16_t count);

// 写块
xy_sdio_result_t xy_sdio_write_blocks(xy_sdio_t *sdio, uint32_t block,
                                       const uint8_t *buffer, uint16_t count);

// 获取卡信息
xy_sdio_result_t xy_sdio_get_card_info(xy_sdio_t *sdio,
                                        xy_sdio_card_info_t *info);
```

---

### 阶段 4: 网络驱动 (6 小时)

#### 4.1 以太网驱动 (6 小时)

**文件结构**:
```
drivers/ethernet/
├── inc/
│   ├── xy_eth.h
│   └── xy_eth_phy.h
├── src/
│   ├── xy_eth.c
│   └── xy_eth_phy.c
└── README.md
```

**API 设计**:
```c
// 初始化
xy_eth_status_t xy_eth_init(xy_eth_t *eth, xy_eth_config_t *cfg);

// 发送数据包
xy_eth_result_t xy_eth_send(xy_eth_t *eth, uint8_t *packet, uint16_t len);

// 接收数据包
xy_eth_result_t xy_eth_receive(xy_eth_t *eth, uint8_t *packet, 
                                uint16_t *len, uint32_t timeout);

// PHY 管理
xy_eth_result_t xy_eth_phy_read(xy_eth_t *eth, uint8_t reg, uint16_t *value);
xy_eth_result_t xy_eth_phy_write(xy_eth_t *eth, uint8_t reg, uint16_t value);
```

---

## 📊 总计划

| 阶段 | 驱动数 | 预计工时 | 优先级 |
|------|--------|---------|--------|
| **阶段 1** | 3 | 8 小时 | 🔴 高 |
| **阶段 2** | 3 | 6 小时 | 🟡 中 |
| **阶段 3** | 3 | 10 小时 | 🟡 中 |
| **阶段 4** | 1 | 6 小时 | 🟢 低 |
| **总计** | **10** | **30 小时** | - |

---

## 🎯 实施策略

### 优先级排序

1. **阶段 1 (ADC/DAC/PWM)** - 最基础模拟驱动
2. **阶段 2 (Timer/WDT/RTC)** - 系统驱动
3. **阶段 3 (CAN/I2S/SDIO)** - 通信驱动
4. **阶段 4 (Ethernet)** - 网络驱动

### 实施原则

1. **统一接口** - 所有驱动使用相同 API 风格
2. **HAL 兼容** - 基于现有 HAL 层实现
3. **文档先行** - 每个驱动配 README.md
4. **测试覆盖** - 每个驱动配测试用例

---

## 📚 相关文档

- [DISPLAY_ARCHITECTURE.md](../DISPLAY_ARCHITECTURE.md) - 显示驱动参考
- [HAL 文档](../hal/README.md) - HAL 接口参考

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
