# NVM 组件优化方案

**日期**: 2026-03-05

---

## 📊 当前问题分析

### xy_nvm/sf_kv.cc (2 个 TODO)

**问题 1**: 数据指针转换为实体返回
```c
// 当前返回的是指针
static void* find_kv_addr(uint8_t key_id);

// 应该返回实体数据
typedef struct {
    int status;
    uint8_t data[64];
    uint16_t len;
} kv_result_t;

kv_result_t kv_get(uint8_t key_id);
```

**问题 2**: RAM 数据不连续，需要分段写
```c
// 当前假设数据连续
memcpy(kv, (kv_sys_m_t*)(addr), sizeof(kv_sys_m_t) - 4);

// 应该分段读取
kv_result_t result;
flash_read_header(addr, &result.header, sizeof(header_t));
flash_read_data(addr + header_size, result.data, result.len);
```

---

## ✅ 修复方案

### 1. 定义返回结构

```c
typedef struct {
    int status;           // 状态码
    uint8_t data[64];     // 数据缓冲区
    uint16_t len;         // 数据长度
} kv_result_t;

#define KV_OK         0
#define KV_NOT_FOUND  -1
#define KV_ERROR      -2
```

### 2. 修改 API

```c
// 旧 API
void* kv_get(uint8_t key_id);

// 新 API
kv_result_t kv_get(uint8_t key_id);
```

### 3. 分段读取

```c
kv_result_t kv_get(uint8_t key_id) {
    kv_result_t result = {0};
    
    // 1. 查找地址
    uint32_t addr = find_kv_addr(key_id);
    if (addr == 0) {
        result.status = KV_NOT_FOUND;
        return result;
    }
    
    // 2. 读取头
    kv_sys_m_t header;
    flash_read(addr, &header, sizeof(header));
    
    // 3. 读取数据 (分段)
    uint32_t data_addr = addr + KV_PACK_HEAD_BYTE + KV_PACK_INFO_BYTE;
    flash_read(data_addr, result.data, header.len - KV_PACK_INFO_BYTE);
    
    result.len = header.len - KV_PACK_INFO_BYTE;
    result.status = KV_OK;
    
    return result;
}
```

---

## 📋 xy_norflash 优化 (1 个 TODO)

### 问题

```c
// TODO: 根据电气参数配置 IO 驱动强度和摆率
// 这部分需要根据具体芯片的寄存器进行配置
```

### 修复方案

```c
/**
 * @brief 硬件初始化
 */
void *xy_nor_hw_init(const xy_nor_config_t *config)
{
    nor_hw_handle_t *hw = (nor_hw_handle_t *)malloc(sizeof(nor_hw_handle_t));
    if (!hw) return NULL;
    
    // 配置 SPI
    hw->spi_handle = spi_init(config->spi_port);
    hw->clock_freq = config->clock_freq;
    
    // 配置 GPIO
    gpio_config_t gpio_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = GPIO_PULL_UP,
        // 根据电气参数配置
        .drive_strength = config->drive_strength,  // 驱动强度
        .slew_rate = config->slew_rate            // 摆率
    };
    
    gpio_init(config->cs_pin, &gpio_cfg);
    gpio_set(config->cs_pin, 1);  // 片选拉高
    
    hw->cs_pin = config->cs_pin;
    
    return hw;
}
```

---

## 🎯 实施计划

| 任务 | 工时 | 状态 |
|------|------|------|
| NVM 返回结构定义 | 1h | ⏳ |
| NVM API 修改 | 1h | ⏳ |
| NVM 分段读取 | 1h | ⏳ |
| NOR Flash 配置完善 | 1h | ⏳ |
| 测试验证 | 1h | ⏳ |
| **总计** | **5h** | - |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
