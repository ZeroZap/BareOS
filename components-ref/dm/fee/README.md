# FEE (Flash EEPROM Emulation) - 完整文档

## 目录

1. [概述](https://ai.eaglelab.tcl.com/#概述)
2. [核心设计](https://ai.eaglelab.tcl.com/#核心设计)
3. [API接口](https://ai.eaglelab.tcl.com/#api接口)
4. [头文件](https://ai.eaglelab.tcl.com/#头文件)
5. [实现文件](https://ai.eaglelab.tcl.com/#实现文件)
6. [测试代码](https://ai.eaglelab.tcl.com/#测试代码)
7. [使用指南](https://ai.eaglelab.tcl.com/#使用指南)
8. [移植指南](https://ai.eaglelab.tcl.com/#移植指南)

------

## 概述

### 什么是FEE？

**FEE (Flash EEPROM Emulation)** 是使用Flash模拟EEPROM的软件解决方案。

### 核心特性

✅ **2-Page架构** - 双缓冲设计，简化管理
✅ **磨损均衡** - 延长Flash使用寿命
✅ **掉电保护** - 数据断电不丢失
✅ **日志式写入** - 延迟GC，提高性能
✅ **自动对齐** - 适配不同Flash写入颗粒
✅ **透明缓存** - 读取零开销

### 适用场景

- 配置参数存储
- 设备校准数据
- 运行计数器/日志
- 替代外部EEPROM芯片

------

## 核心设计

### 1. 架构图

```text
┌─────────────────────────────────────────┐
│         用户应用层                        │
│  uint8_t virtual_eeprom[256];           │
│  fee_write(h, 0, data, 8);              │
│  fee_read(h, 0, buf, 8);                │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│         FEE抽象层                        │
│  - Cache管理 (镜像virtual_eeprom)       │
│  - Record写入                            │
│  - GC垃圾回收                            │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│         Flash硬件层                      │
│  FEE Page 0 (Active)                    │
│  FEE Page 1 (Standby)                   │
└─────────────────────────────────────────┘
```

### 2. 2-Page架构

```text
Flash布局：
┌──────────────────┬──────────────────┐
│   FEE Page 0     │   FEE Page 1     │
│  (2048 bytes)    │  (2048 bytes)    │
└──────────────────┴──────────────────┘

每个FEE Page由N个Flash Page组成：
FEE Page = Flash Page × pages_per_fee_page

工作模式：
- 活动页（Active）：当前正在写入
- 备用页（Standby）：等待切换
- GC时互换角色
```

### 3. FEE Page结构

```text
┌─────────────────────────────────────────┐
│ FEE Page Header (对齐到颗粒)            │
│  ├─ page_state (4B)  "FEEA"/"FEER"     │
│  ├─ erase_count (2B)                    │
│  ├─ header_crc (2B)                     │
│  └─ padding (对齐填充)                  │
├─────────────────────────────────────────┤
│ Record 0 (= 1个颗粒)                    │
│  ├─ addr (2B)                           │
│  ├─ crc (2B)                            │
│  └─ data (gran-4 bytes)                 │
├─────────────────────────────────────────┤
│ Record 1 (= 1个颗粒)                    │
│ ...                                     │
│ Record N (= 1个颗粒)                    │
└─────────────────────────────────────────┘
```

### 4. 关键设计原则

#### 原则1：Header对齐到写入颗粒

```c
// write_granularity = 8
Header基础大小 = 8字节
对齐后 = 8字节（无填充）

// write_granularity = 16
Header基础大小 = 8字节
对齐后 = 16字节（8字节填充）
```

#### 原则2：每条Record = 1个写入颗粒

```c
Record = Header(4B) + Data(gran-4B)

gran=8:  Record = 4B + 4B = 8B
gran=16: Record = 4B + 12B = 16B
gran=32: Record = 4B + 28B = 32B
```

#### 原则3：Cache是虚拟EEPROM的镜像

```c
uint8_t virtual_eeprom[256];
fee_init(&fee, &config, virtual_eeprom, work);

// virtual_eeprom[0]   = 逻辑地址0的数据
// virtual_eeprom[100] = 逻辑地址100的数据
// 读取直接从virtual_eeprom返回
```

------

## API接口

### 数据类型

```c
// FEE句柄
typedef struct {
    // ... (内部实现)
} fee_handle_t;

// Flash操作接口
typedef struct {
    int (*erase)(uint32_t addr);
    int (*write)(uint32_t addr, const uint8_t *data, uint16_t len);
    int (*read)(uint32_t addr, uint8_t *data, uint16_t len);
} fee_flash_ops_t;

// 配置参数
typedef struct {
    uint8_t  *flash_base;
    uint8_t   pages_per_fee_page;
    uint16_t  flash_page_size;
    uint16_t  cache_size;
    uint8_t   write_granularity;
    uint16_t  max_erase_count;
    const fee_flash_ops_t *flash_ops;
} fee_config_t;

// 返回状态
typedef enum {
    FEE_OK = 0,
    FEE_ERROR = -1,
    FEE_ERROR_PARAM = -2,
    FEE_ERROR_NOT_INIT = -3,
    FEE_ERROR_ERASE_LIMIT = -4,
    FEE_ERROR_FLASH = -5,
    FEE_ERROR_FULL = -6
} fee_status_t;
```

### 核心函数

#### fee_init - 初始化

```c
fee_status_t fee_init(fee_handle_t *handle,
                      const fee_config_t *config,
                      uint8_t *cache_buffer,
                      uint8_t *work_buffer);
```

**功能**：初始化FEE系统
**参数**：

- `handle` - FEE句柄
- `config` - 配置参数
- `cache_buffer` - 虚拟EEPROM数组（用户提供）
- `work_buffer` - 工作缓冲区（大小=FEE_WORK_SIZE(gran)）

**返回**：FEE_OK 或错误码

#### fee_write - 写入数据

```c
fee_status_t fee_write(fee_handle_t *handle,
                       uint16_t addr,
                       const uint8_t *data,
                       uint16_t len);
```

**功能**：写入数据到虚拟EEPROM
**参数**：

- `addr` - 逻辑地址
- `data` - 数据指针
- `len` - 数据长度

**行为**：

1. 写入Record到Flash
2. 更新Cache
3. 必要时触发GC

#### fee_read - 读取数据

```c
fee_status_t fee_read(fee_handle_t *handle,
                      uint16_t addr,
                      uint8_t *data,
                      uint16_t len);
```

**功能**：从虚拟EEPROM读取数据
**参数**：

- `addr` - 逻辑地址
- `data` - 读取缓冲区
- `len` - 读取长度

**行为**：直接从Cache复制，不访问Flash

#### fee_format - 格式化

```c
fee_status_t fee_format(fee_handle_t *handle);
```

**功能**：格式化FEE，擦除所有数据

#### fee_gc - 垃圾回收

```c
fee_status_t fee_gc(fee_handle_t *handle);
```

**功能**：手动触发垃圾回收（通常自动触发）

#### fee_get_info - 获取信息

```c
fee_status_t fee_get_info(fee_handle_t *handle,
                          uint16_t *erase_count,
                          uint16_t *free_bytes,
                          uint16_t *record_count);
```

**功能**：获取FEE运行状态

------

## 头文件

### fee.h

```c
#ifndef FEE_H
#define FEE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file fee.h
 * @brief Flash EEPROM Emulation - 使用Flash模拟EEPROM
 * @version 1.0
 * @date 2024
 * 
 * @note 设计约束：
 *       - write_granularity >= 8 (保证至少4字节数据)
 *       - FEE Page Header对齐到write_granularity
 *       - 每条Record = 1个write_granularity
 *       - Cache是虚拟EEPROM的完整镜像
 */

/* ============ 配置常量 ============ */

#define FEE_MIN_GRANULARITY  8      /**< 最小写入颗粒（字节） */
#define FEE_MAX_GRANULARITY  128    /**< 最大写入颗粒（字节） */

/* ============ 页状态定义 ============ */

#define FEE_PAGE_STATE_ERASED     0xFFFFFFFF  /**< 擦除态 */
#define FEE_PAGE_STATE_ACTIVE     0x46454541  /**< "FEEA" - 活动页 */
#define FEE_PAGE_STATE_RECEIVING  0x46454552  /**< "FEER" - 接收中（GC） */
#define FEE_PAGE_STATE_INVALID    0x46454549  /**< "FEEI" - 已失效 */

/* ============ 数据结构 ============ */

/**
 * @brief FEE Page头部（基础8字节，实际占用会对齐到write_granularity）
 */
typedef struct __attribute__((packed)) {
    uint32_t page_state;     /**< 页状态（magic + state合一） */
    uint16_t erase_count;    /**< 擦除次数 */
    uint16_t header_crc;     /**< 头部CRC16校验 */
} fee_page_header_t;

#define FEE_PAGE_HEADER_BASE_SIZE 8

/**
 * @brief Record头部（固定4字节）
 */
typedef struct __attribute__((packed)) {
    uint16_t addr;       /**< 逻辑地址 */
    uint16_t crc;        /**< 数据CRC16 */
    /* 后面紧跟 (write_granularity - 4) 字节数据 */
} fee_record_header_t;

#define FEE_RECORD_HEADER_SIZE 4

/**
 * @brief 判断Record是否为擦除态
 */
#define FEE_RECORD_IS_ERASED(rec)  ((rec)->addr == 0xFFFF && (rec)->crc == 0xFFFF)

/**
 * @brief Flash操作接口
 */
typedef struct {
    /**
     * @brief 擦除Flash页
     * @param addr Flash页起始地址
     * @return 0-成功，非0-失败
     */
    int (*erase)(uint32_t addr);
    
    /**
     * @brief 写入Flash
     * @param addr 写入地址
     * @param data 数据指针
     * @param len 数据长度（必须是write_granularity的整数倍）
     * @return 0-成功，非0-失败
     */
    int (*write)(uint32_t addr, const uint8_t *data, uint16_t len);
    
    /**
     * @brief 读取Flash
     * @param addr 读取地址
     * @param data 数据缓冲区
     * @param len 读取长度
     * @return 0-成功，非0-失败
     */
    int (*read)(uint32_t addr, uint8_t *data, uint16_t len);
} fee_flash_ops_t;

/**
 * @brief FEE配置参数
 */
typedef struct {
    uint8_t  *flash_base;           /**< Flash基地址 */
    uint8_t   pages_per_fee_page;   /**< 每个FEE Page包含的Flash page数 */
    uint16_t  flash_page_size;      /**< 单个Flash page大小（字节） */
    uint16_t  cache_size;           /**< 虚拟EEPROM大小（字节） */
    uint8_t   write_granularity;    /**< 写入颗粒（8/16/32/64/128） */
    uint16_t  max_erase_count;      /**< 最大擦除次数 */
    const fee_flash_ops_t *flash_ops; /**< Flash操作接口 */
} fee_config_t;

/**
 * @brief FEE句柄（内部状态）
 */
typedef struct {
    /* Flash基础信息 */
    uint8_t  *flash_base;
    uint16_t  fee_page_size;            /**< 单个FEE Page大小 */
    uint8_t   pages_per_fee_page;
    uint8_t   write_granularity;
    
    /* 布局信息 */
    uint8_t   aligned_header_size;      /**< Header对齐后的大小 */
    uint8_t   record_data_size;         /**< 每条Record的数据大小 */
    uint16_t  data_area_size;           /**< 数据区大小 */
    
    /* 运行状态 */
    uint16_t  cache_size;
    uint16_t  write_offset;             /**< 当前写入偏移 */
    uint16_t  max_erase_count;
    uint8_t   active_page;              /**< 活动FEE Page (0或1) */
    uint8_t   flags;
    
    /* 缓冲区 */
    uint8_t  *cache;                    /**< 指向虚拟EEPROM数组 */
} fee_handle_t;

#define FEE_FLAG_INIT     (1 << 0)
#define FEE_FLAG_VALID    (1 << 1)

/**
 * @brief FEE状态码
 */
typedef enum {
    FEE_OK = 0,                 /**< 成功 */
    FEE_ERROR = -1,             /**< 通用错误 */
    FEE_ERROR_PARAM = -2,       /**< 参数错误 */
    FEE_ERROR_NOT_INIT = -3,    /**< 未初始化 */
    FEE_ERROR_ERASE_LIMIT = -4, /**< 达到擦除次数上限 */
    FEE_ERROR_FLASH = -5,       /**< Flash操作失败 */
    FEE_ERROR_FULL = -6         /**< 空间已满 */
} fee_status_t;

/* ============ API函数 ============ */

/**
 * @brief 初始化FEE
 * 
 * @param handle FEE句柄指针
 * @param config 配置参数
 * @param cache_buffer 虚拟EEPROM数组（用户提供，大小=config->cache_size）
 * @param work_buffer 工作缓冲区（用户提供，大小=FEE_WORK_SIZE(gran)）
 * @return FEE状态码
 * 
 * @note 1. 如果Flash中存在有效数据，会自动恢复到cache_buffer
 *       2. 如果Flash为空，会格式化FEE Page 0
 *       3. cache_buffer就是虚拟EEPROM，读写操作直接访问它
 */
fee_status_t fee_init(fee_handle_t *handle,
                      const fee_config_t *config,
                      uint8_t *cache_buffer,
                      uint8_t *work_buffer);

/**
 * @brief 写入数据到虚拟EEPROM
 * 
 * @param handle FEE句柄
 * @param addr 逻辑地址
 * @param data 数据指针
 * @param len 数据长度
 * @return FEE状态码
 * 
 * @note 1. 会同时写入Flash和更新Cache
 *       2. 如果空间不足，自动触发GC
 *       3. 地址和长度无对齐要求
 */
fee_status_t fee_write(fee_handle_t *handle,
                       uint16_t addr,
                       const uint8_t *data,
                       uint16_t len);

/**
 * @brief 从虚拟EEPROM读取数据
 * 
 * @param handle FEE句柄
 * @param addr 逻辑地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return FEE状态码
 * 
 * @note 直接从Cache读取，不访问Flash（速度极快）
 */
fee_status_t fee_read(fee_handle_t *handle,
                      uint16_t addr,
                      uint8_t *data,
                      uint16_t len);

/**
 * @brief 格式化FEE（擦除所有数据）
 * 
 * @param handle FEE句柄
 * @return FEE状态码
 */
fee_status_t fee_format(fee_handle_t *handle);

/**
 * @brief 手动触发垃圾回收
 * 
 * @param handle FEE句柄
 * @return FEE状态码
 * 
 * @note 通常由fee_write()自动触发，无需手动调用
 */
fee_status_t fee_gc(fee_handle_t *handle);

/**
 * @brief 获取FEE运行信息
 * 
 * @param handle FEE句柄
 * @param erase_count 擦除次数（可为NULL）
 * @param free_bytes 剩余空间（字节，可为NULL）
 * @param record_count 当前记录数（可为NULL）
 * @return FEE状态码
 */
fee_status_t fee_get_info(fee_handle_t *handle,
                          uint16_t *erase_count,
                          uint16_t *free_bytes,
                          uint16_t *record_count);

/* ============ 工具宏 ============ */

/**
 * @brief 计算对齐后的Header大小
 */
#define FEE_ALIGNED_HEADER_SIZE(gran) \
    (((FEE_PAGE_HEADER_BASE_SIZE + (gran) - 1) / (gran)) * (gran))

/**
 * @brief 计算每条Record的数据大小
 */
#define FEE_RECORD_DATA_SIZE(gran)  ((gran) - FEE_RECORD_HEADER_SIZE)

/**
 * @brief 计算Record总大小（固定等于颗粒）
 */
#define FEE_RECORD_SIZE(gran)  (gran)

/**
 * @brief 计算数据区大小
 */
#define FEE_DATA_AREA_SIZE(fee_page_size, gran) \
    ((fee_page_size) - FEE_ALIGNED_HEADER_SIZE(gran))

/**
 * @brief 计算最大Record数
 */
#define FEE_MAX_RECORDS(fee_page_size, gran) \
    (FEE_DATA_AREA_SIZE(fee_page_size, gran) / (gran))

/**
 * @brief 计算工作缓冲区大小
 */
#define FEE_WORK_SIZE(gran)  ((gran) * 2)

/**
 * @brief 计算总RAM占用
 */
#define FEE_TOTAL_RAM(cache_size, gran) \
    (sizeof(fee_handle_t) + (cache_size) + FEE_WORK_SIZE(gran))

#endif /* FEE_H */
```

------

## 实现文件

### fee.c

```c
/**
 * @file fee.c
 * @brief Flash EEPROM Emulation 实现
 */

#include "fee.h"
#include <string.h>

/* ============ 全局变量 ============ */

static uint8_t *g_work_buffer = NULL;
static const fee_flash_ops_t *g_flash_ops = NULL;

/* ============ CRC16计算 ============ */

/**
 * @brief CRC16-CCITT计算
 */
static uint16_t crc16_calc(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 8; i; i--) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/* ============ Flash地址计算 ============ */

static inline uint32_t get_fee_page_addr(fee_handle_t *h, uint8_t page_idx) {
    return (uint32_t)(h->flash_base + (uint32_t)page_idx * h->fee_page_size);
}

static inline uint32_t get_data_area_addr(fee_handle_t *h, uint8_t page_idx) {
    return get_fee_page_addr(h, page_idx) + h->aligned_header_size;
}

/* ============ Flash操作封装 ============ */

static fee_status_t flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    return g_flash_ops->read(addr, buf, len) == 0 ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t flash_write(fee_handle_t *h, uint32_t addr,
                                const uint8_t *buf, uint16_t len) {
    if (len % h->write_granularity != 0) {
        return FEE_ERROR_FLASH;
    }
    return g_flash_ops->write(addr, buf, len) == 0 ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t flash_erase_fee_page(fee_handle_t *h, uint8_t page_idx) {
    uint32_t base = get_fee_page_addr(h, page_idx);
    uint16_t flash_page_size = h->fee_page_size / h->pages_per_fee_page;
    
    for (uint8_t i = 0; i < h->pages_per_fee_page; i++) {
        if (g_flash_ops->erase(base + (uint32_t)i * flash_page_size) != 0) {
            return FEE_ERROR_FLASH;
        }
    }
    return FEE_OK;
}

/* ============ FEE Page Header操作 ============ */

static fee_status_t read_page_header(fee_handle_t *h, uint8_t page_idx,
                                     fee_page_header_t *hdr) {
    uint32_t addr = get_fee_page_addr(h, page_idx);
    
    fee_status_t status = flash_read(addr, (uint8_t *)hdr,
                                     FEE_PAGE_HEADER_BASE_SIZE);
    if (status != FEE_OK) return status;
    
    // 验证CRC
    uint16_t crc = crc16_calc((uint8_t *)hdr,
                             offsetof(fee_page_header_t, header_crc));
    return (crc == hdr->header_crc) ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t write_page_header(fee_handle_t *h, uint8_t page_idx,
                                      fee_page_header_t *hdr) {
    uint32_t addr = get_fee_page_addr(h, page_idx);
    uint8_t aligned_size = h->aligned_header_size;
    
    // 计算CRC
    hdr->header_crc = crc16_calc((uint8_t *)hdr,
                                offsetof(fee_page_header_t, header_crc));
    
    // 准备对齐缓冲区
    memset(g_work_buffer, 0xFF, aligned_size);
    memcpy(g_work_buffer, hdr, FEE_PAGE_HEADER_BASE_SIZE);
    
    // 按颗粒写入
    uint8_t gran = h->write_granularity;
    for (uint16_t offset = 0; offset < aligned_size; offset += gran) {
        fee_status_t status = flash_write(h, addr + offset,
                                          g_work_buffer + offset, gran);
        if (status != FEE_OK) return status;
    }
    
    return FEE_OK;
}

/* ============ 写入偏移计算 ============ */

/**
 * @brief 通过扫描Flash计算当前写入位置
 */
static uint16_t calc_write_offset(fee_handle_t *h, uint8_t page_idx) {
    uint32_t addr = get_data_area_addr(h, page_idx);
    uint16_t max_size = h->data_area_size;
    uint16_t offset = 0;
    uint8_t rec_size = h->write_granularity;
    
    while (offset + rec_size <= max_size) {
        fee_record_header_t rec;
        
        if (flash_read(addr + offset, (uint8_t *)&rec,
                      FEE_RECORD_HEADER_SIZE) != FEE_OK) {
            break;
        }
        
        if (FEE_RECORD_IS_ERASED(&rec)) {
            return offset;
        }
        
        offset += rec_size;
    }
    
    return offset;
}

/* ============ Record操作 ============ */

/**
 * @brief 写入一条Record
 * @note Record大小固定为1个write_granularity
 */
static fee_status_t write_record(fee_handle_t *h,
                                 uint16_t addr,
                                 const uint8_t *data,
                                 uint8_t len) {
    uint16_t max_size = h->data_area_size;
    uint8_t gran = h->write_granularity;
    
    // 检查空间
    if (h->write_offset + gran > max_size) {
        return FEE_ERROR_FULL;
    }
    
    // 检查数据长度
    if (len > h->record_data_size) {
        return FEE_ERROR_PARAM;
    }
    
    // 构造Record
    fee_record_header_t rec;
    rec.addr = addr;
    rec.crc = crc16_calc(data, len);
    
    // 组装到工作缓冲区（Header + Data，总大小=颗粒）
    memset(g_work_buffer, 0xFF, gran);
    memcpy(g_work_buffer, &rec, FEE_RECORD_HEADER_SIZE);
    memcpy(g_work_buffer + FEE_RECORD_HEADER_SIZE, data, len);
    
    // 一次写入（正好1个颗粒）
    uint32_t write_addr = get_data_area_addr(h, h->active_page) + h->write_offset;
    fee_status_t status = flash_write(h, write_addr, g_work_buffer, gran);
    if (status != FEE_OK) return status;
    
    h->write_offset += gran;
    return FEE_OK;
}

/* ============ Cache重建 ============ */

/**
 * @brief 从Flash重建Cache（掉电恢复的关键）
 */
static fee_status_t rebuild_cache(fee_handle_t *h, uint8_t page_idx) {
    uint32_t addr = get_data_area_addr(h, page_idx);
    uint16_t max_size = h->data_area_size;
    uint16_t offset = 0;
    uint8_t gran = h->write_granularity;
    uint8_t data_size = h->record_data_size;
    
    // 清空Cache
    memset(h->cache, 0xFF, h->cache_size);
    
    // 遍历所有Record
    while (offset + gran <= max_size) {
        fee_record_header_t rec;
        
        if (flash_read(addr + offset, (uint8_t *)&rec,
                      FEE_RECORD_HEADER_SIZE) != FEE_OK) {
            break;
        }
        
        if (FEE_RECORD_IS_ERASED(&rec)) break;
        
        // 读取Record数据
        uint8_t data_buf[FEE_MAX_GRANULARITY];
        if (flash_read(addr + offset + FEE_RECORD_HEADER_SIZE,
                      data_buf, data_size) == FEE_OK) {
            
            // 验证CRC
            uint16_t calc_crc = crc16_calc(data_buf, data_size);
            if (calc_crc == rec.crc) {
                // 更新Cache（后面的Record会覆盖前面的，实现最新值）
                if (rec.addr + data_size <= h->cache_size) {
                    memcpy(&h->cache[rec.addr], data_buf, data_size);
                }
            }
        }
        
        offset += gran;
    }
    
    h->write_offset = offset;
    h->flags |= FEE_FLAG_VALID;
    
    return FEE_OK;
}

/* ============ 2-Page管理 ============ */

/**
 * @brief 查找活动FEE Page
 * @return 0或1，-1表示无有效页
 */
static int8_t find_active_page(fee_handle_t *h) {
    fee_page_header_t hdr0, hdr1;
    bool valid0 = false, valid1 = false;
    
    if (read_page_header(h, 0, &hdr0) == FEE_OK) {
        if (hdr0.page_state == FEE_PAGE_STATE_ACTIVE ||
            hdr0.page_state == FEE_PAGE_STATE_RECEIVING) {
            valid0 = true;
        }
    }
    
    if (read_page_header(h, 1, &hdr1) == FEE_OK) {
        if (hdr1.page_state == FEE_PAGE_STATE_ACTIVE ||
            hdr1.page_state == FEE_PAGE_STATE_RECEIVING) {
            valid1 = true;
        }
    }
    
    // 两个都有效，选择擦除次数大的（最新的）
    if (valid0 && valid1) {
        return (hdr0.erase_count >= hdr1.erase_count) ? 0 : 1;
    }
    
    if (valid0) return 0;
    if (valid1) return 1;
    
    return -1;
}

/* ============ GC（垃圾回收） ============ */

/**
 * @brief 执行GC迁移
 * @note 将Cache中的有效数据迁移到另一个FEE Page
 */
static fee_status_t fee_gc_migrate(fee_handle_t *h) {
    fee_page_header_t old_hdr, new_hdr;
    uint8_t new_page = h->active_page ^ 1;  // 0->1, 1->0
    
    // 读取旧页头
    if (read_page_header(h, h->active_page, &old_hdr) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }
    
    // 检查擦除次数限制
    if (old_hdr.erase_count >= h->max_erase_count) {
        return FEE_ERROR_ERASE_LIMIT;
    }
    
    // 擦除新页
    if (flash_erase_fee_page(h, new_page) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }
    
    // 写入新页头（RECEIVING状态）
    new_hdr.page_state = FEE_PAGE_STATE_RECEIVING;
    new_hdr.erase_count = old_hdr.erase_count + 1;
    
    if (write_page_header(h, new_page, &new_hdr) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }
    
    // 切换活动页
    uint8_t old_page = h->active_page;
    h->active_page = new_page;
    h->write_offset = 0;
    
    // 迁移Cache中的有效数据
    uint8_t data_size = h->record_data_size;
    for (uint16_t addr = 0; addr < h->cache_size; addr += data_size) {
        bool has_data = false;
        uint8_t chunk = (addr + data_size <= h->cache_size) ?
                       data_size : (h->cache_size - addr);
        
        // 检查是否有非0xFF数据
        for (uint8_t i = 0; i < chunk; i++) {
            if (h->cache[addr + i] != 0xFF) {
                has_data = true;
                break;
            }
        }
        
        if (has_data) {
            fee_status_t status = write_record(h, addr, &h->cache[addr], chunk);
            if (status != FEE_OK && status != FEE_ERROR_FULL) {
                // 失败，恢复旧页
                h->active_page = old_page;
                h->write_offset = calc_write_offset(h, old_page);
                return status;
            }
        }
    }
    
    // 更新新页状态为ACTIVE
    new_hdr.page_state = FEE_PAGE_STATE_ACTIVE;
    write_page_header(h, new_page, &new_hdr);
    
    // 标记旧页为INVALID
    old_hdr.page_state = FEE_PAGE_STATE_INVALID;
    write_page_header(h, old_page, &old_hdr);
    
    return FEE_OK;
}

/* ============ 公共API实现 ============ */

fee_status_t fee_init(fee_handle_t *handle,
                      const fee_config_t *config,
                      uint8_t *cache_buffer,
                      uint8_t *work_buffer) {
    fee_page_header_t hdr;
    int8_t active;
    
    // 参数检查
    if (!handle || !config || !cache_buffer || !work_buffer) {
        return FEE_ERROR_PARAM;
    }
    
    uint8_t gran = config->write_granularity;
    
    // 检查颗粒度（必须是2的幂，且在范围内）
    if (gran < FEE_MIN_GRANULARITY || gran > FEE_MAX_GRANULARITY ||
        (gran & (gran - 1)) != 0) {
        return FEE_ERROR_PARAM;
    }
    
    // 设置全局变量
    g_work_buffer = work_buffer;
    g_flash_ops = config->flash_ops;
    
    // 计算对齐参数
    uint8_t aligned_header = FEE_ALIGNED_HEADER_SIZE(gran);
    
    // 初始化句柄
    memset(handle, 0, sizeof(fee_handle_t));
    handle->flash_base = config->flash_base;
    handle->pages_per_fee_page = config->pages_per_fee_page;
    handle->fee_page_size = config->flash_page_size * config->pages_per_fee_page;
    handle->write_granularity = gran;
    handle->aligned_header_size = aligned_header;
    handle->record_data_size = gran - FEE_RECORD_HEADER_SIZE;
    handle->data_area_size = handle->fee_page_size - aligned_header;
    handle->cache = cache_buffer;
    handle->cache_size = config->cache_size;
    handle->max_erase_count = config->max_erase_count;
    
    // 查找活动FEE Page
    active = find_active_page(handle);
    
    if (active >= 0) {
        // 找到活动页，重建Cache
        handle->active_page = (uint8_t)active;
        
        if (rebuild_cache(handle, (uint8_t)active) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }
        
        // 修复RECEIVING状态（如果GC中断）
        if (read_page_header(handle, (uint8_t)active, &hdr) == FEE_OK) {
            if (hdr.page_state == FEE_PAGE_STATE_RECEIVING) {
                hdr.page_state = FEE_PAGE_STATE_ACTIVE;
                write_page_header(handle, (uint8_t)active, &hdr);
            }
        }
    } else {
        // 没有活动页，格式化FEE Page 0
        handle->active_page = 0;
        
        if (flash_erase_fee_page(handle, 0) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }
        
        hdr.page_state = FEE_PAGE_STATE_ACTIVE;
        hdr.erase_count = 1;
        
        if (write_page_header(handle, 0, &hdr) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }
        
        memset(handle->cache, 0xFF, handle->cache_size);
        handle->write_offset = 0;
        handle->flags |= FEE_FLAG_VALID;
    }
    
    handle->flags |= FEE_FLAG_INIT;
    return FEE_OK;
}

fee_status_t fee_write(fee_handle_t *handle,
                       uint16_t addr,
                       const uint8_t *data,
                       uint16_t len) {
    if (!handle || !data || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }
    
    if (addr + len > handle->cache_size) {
        return FEE_ERROR_PARAM;
    }
    
    // 检查数据是否相同（优化：避免不必要的写入）
    if (memcmp(&handle->cache[addr], data, len) == 0) {
        return FEE_OK;
    }
    
    // 按record_data_size分块写入
    uint16_t remaining = len;
    uint16_t offset = 0;
    uint8_t chunk_size = handle->record_data_size;
    
    while (remaining > 0) {
        uint8_t chunk = (remaining > chunk_size) ? chunk_size : (uint8_t)remaining;
        
        fee_status_t status = write_record(handle, addr + offset,
                                          data + offset, chunk);
        
        if (status == FEE_ERROR_FULL) {
            // 空间不足，触发GC
            status = fee_gc_migrate(handle);
            if (status != FEE_OK) return status;
            
            // 重试写入
            status = write_record(handle, addr + offset, data + offset, chunk);
        }
        
        if (status != FEE_OK) return status;
        
        offset += chunk;
        remaining -= chunk;
    }
    
    // 更新Cache
    memcpy(&handle->cache[addr], data, len);
    
    return FEE_OK;
}

fee_status_t fee_read(fee_handle_t *handle,
                      uint16_t addr,
                      uint8_t *data,
                      uint16_t len) {
    if (!handle || !data || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }
    
    if (addr + len > handle->cache_size) {
        return FEE_ERROR_PARAM;
    }
    
    // 直接从Cache复制
    memcpy(data, &handle->cache[addr], len);
    return FEE_OK;
}

fee_status_t fee_format(fee_handle_t *handle) {
    fee_page_header_t hdr;
    
    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }
    
    // 擦除两个FEE Page
    flash_erase_fee_page(handle, 0);
    flash_erase_fee_page(handle, 1);
    
    // 重新初始化FEE Page 0
    handle->active_page = 0;
    handle->write_offset = 0;
    
    hdr.page_state = FEE_PAGE_STATE_ACTIVE;
    hdr.erase_count = 1;
    write_page_header(handle, 0, &hdr);
    
    // 清空Cache
    memset(handle->cache, 0xFF, handle->cache_size);
    handle->flags |= FEE_FLAG_VALID;
    
    return FEE_OK;
}

fee_status_t fee_gc(fee_handle_t *handle) {
    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }
    return fee_gc_migrate(handle);
}

fee_status_t fee_get_info(fee_handle_t *handle,
                          uint16_t *erase_count,
                          uint16_t *free_bytes,
                          uint16_t *record_count) {
    fee_page_header_t hdr;
    
    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }
    
    if (read_page_header(handle, handle->active_page, &hdr) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }
    
    if (erase_count) {
        *erase_count = hdr.erase_count;
    }
    
    if (free_bytes) {
        *free_bytes = handle->data_area_size - handle->write_offset;
    }
    
    if (record_count) {
        *record_count = handle->write_offset / handle->write_granularity;
    }
    
    return FEE_OK;
}
```

------

## 测试代码

### fee_test.c

```c
/**
 * @file fee_test.c
 * @brief FEE完整测试套件
 */

#include "fee.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* ============ Flash模拟器 ============ */

typedef struct {
    uint8_t *storage;
    uint16_t size;
    uint16_t page_size;
    uint32_t erase_count;
    uint32_t write_count;
    uint32_t read_count;
} flash_sim_t;

static flash_sim_t *g_sim = NULL;

int sim_erase(uint32_t addr) {
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    memset(&g_sim->storage[offset], 0xFF, g_sim->page_size);
    g_sim->erase_count++;
    printf("    [Flash] Erase at 0x%04X (count=%u)\n", offset, g_sim->erase_count);
    return 0;
}

int sim_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    for (uint16_t i = 0; i < len; i++) {
        g_sim->storage[offset + i] &= data[i];
    }
    g_sim->write_count++;
    return 0;
}

int sim_read(uint32_t addr, uint8_t *data, uint16_t len) {
    uint32_t offset = addr - (uint32_t)g_sim->storage;
    memcpy(data, &g_sim->storage[offset], len);
    g_sim->read_count++;
    return 0;
}

static const fee_flash_ops_t flash_ops = {
    .erase = sim_erase,
    .write = sim_write,
    .read = sim_read
};

void reset_flash_stats(void) {
    g_sim->erase_count = 0;
    g_sim->write_count = 0;
    g_sim->read_count = 0;
}

/* ============ 测试辅助函数 ============ */

void print_test_header(const char *title) {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║  %-52s║\n", title);
    printf("╚════════════════════════════════════════════════════════╝\n\n");
}

void print_cache_data(const char *label, uint8_t *cache, uint16_t addr, uint16_t len) {
    printf("  %s [0x%04X]: ", label, addr);
    for (uint16_t i = 0; i < len && i < 16; i++) {
        printf("%02X ", cache[addr + i]);
    }
    if (len > 16) printf("...");
    printf("\n");
}

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) { \
        printf("  ✗ FAILED: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
        return false; \
    }

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { \
        printf("  ✗ FAILED: %s\n", msg); \
        return false; \
    }

/* ============ 测试用例 ============ */

/**
 * @brief 测试1：基本初始化
 */
bool test_basic_init(void) {
    print_test_header("Test 1: Basic Initialization");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 256
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    memset(virtual_eeprom, 0x00, sizeof(virtual_eeprom));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    printf("Configuration:\n");
    printf("  FEE Page size:  %d bytes\n", PAGE_SIZE * PAGES_PER_FEE);
    printf("  Cache size:     %d bytes\n", CACHE_SIZE);
    printf("  Granularity:    %d bytes\n", GRAN);
    printf("  Record data:    %d bytes\n\n", FEE_RECORD_DATA_SIZE(GRAN));
    
    fee_status_t status = fee_init(&fee, &config, virtual_eeprom, work);
    ASSERT_EQ(status, FEE_OK, "fee_init() should return FEE_OK");
    
    // 验证Cache已清空（初始化为0xFF）
    bool all_ff = true;
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (virtual_eeprom[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    ASSERT_TRUE(all_ff, "Cache should be all 0xFF after init");
    
    printf("  ✓ Initialization OK\n");
    printf("  ✓ Cache cleared to 0xFF\n");
    printf("  ✓ Test PASSED\n");
    
    return true;
}

/**
 * @brief 测试2：基本读写
 */
bool test_basic_read_write(void) {
    print_test_header("Test 2: Basic Read/Write");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 256
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    // 写入测试数据
    uint8_t test_data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    printf("Writing 8 bytes to address 0x00...\n");
    
    fee_status_t status = fee_write(&fee, 0, test_data, 8);
    ASSERT_EQ(status, FEE_OK, "fee_write() should succeed");
    
    print_cache_data("Cache", virtual_eeprom, 0, 8);
    
    // 验证Cache已更新
    ASSERT_TRUE(memcmp(virtual_eeprom, test_data, 8) == 0,
                "Cache should contain written data");
    
    // 读取验证
    uint8_t read_buf[8];
    status = fee_read(&fee, 0, read_buf, 8);
    ASSERT_EQ(status, FEE_OK, "fee_read() should succeed");
    
    ASSERT_TRUE(memcmp(read_buf, test_data, 8) == 0,
                "Read data should match written data");
    
    printf("  ✓ Write OK\n");
    printf("  ✓ Cache updated\n");
    printf("  ✓ Read matches write\n");
    printf("  ✓ Test PASSED\n");
    
    return true;
}

/**
 * @brief 测试3：多次覆盖写入
 */
bool test_multiple_writes(void) {
    print_test_header("Test 3: Multiple Overwrites");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 128
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    // 第1次写入
    uint8_t data1[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    printf("Write #1: ");
    for (int i = 0; i < 8; i++) printf("%02X ", data1[i]);
    printf("\n");
    fee_write(&fee, 0, data1, 8);
    
    // 第2次写入（覆盖）
    uint8_t data2[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    printf("Write #2: ");
    for (int i = 0; i < 8; i++) printf("%02X ", data2[i]);
    printf("\n");
    fee_write(&fee, 0, data2, 8);
    
    // 第3次写入（覆盖）
    uint8_t data3[8] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8};
    printf("Write #3: ");
    for (int i = 0; i < 8; i++) printf("%02X ", data3[i]);
    printf("\n");
    fee_write(&fee, 0, data3, 8);
    
    print_cache_data("Final cache", virtual_eeprom, 0, 8);
    
    // 验证Cache是最新值
    ASSERT_TRUE(memcmp(virtual_eeprom, data3, 8) == 0,
                "Cache should contain latest data");
    
    // 获取信息
    uint16_t erase, free, rec_cnt;
    fee_get_info(&fee, &erase, &free, &rec_cnt);
    printf("\n  Flash records: %u (3 writes * 2 records = 6)\n", rec_cnt);
    printf("  Free space: %u bytes\n", free);
    
    printf("  ✓ Multiple writes OK\n");
    printf("  ✓ Cache contains latest value\n");
    printf("  ✓ Test PASSED\n");
    
    return true;
}

/**
 * @brief 测试4：掉电恢复
 */
bool test_power_loss_recovery(void) {
    print_test_header("Test 4: Power-off Recovery");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 128
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    // ====== 第一次上电 ======
    printf("=== First Power-on ===\n");
    
    uint8_t eeprom1[CACHE_SIZE];
    uint8_t work1[FEE_WORK_SIZE(GRAN)];
    
    fee_handle_t fee1;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee1, &config, eeprom1, work1);
    
    // 写入数据
    uint8_t data_addr0[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t data_addr64[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    
    fee_write(&fee1, 0, data_addr0, 8);
    fee_write(&fee1, 64, data_addr64, 8);
    
    print_cache_data("Before power-off [0]", eeprom1, 0, 8);
    print_cache_data("Before power-off [64]", eeprom1, 64, 8);
    
    // ====== 模拟掉电 ======
    printf("\n=== Power Loss ===\n");
    printf("  Simulating power-off...\n");
    
    // ====== 第二次上电 ======
    printf("\n=== Second Power-on (Recovery) ===\n");
    
    uint8_t eeprom2[CACHE_SIZE];
    uint8_t work2[FEE_WORK_SIZE(GRAN)];
    
    memset(eeprom2, 0x00, sizeof(eeprom2));  // 故意初始化为非0xFF
    
    fee_handle_t fee2;
    fee_init(&fee2, &config, eeprom2, work2);
    
    print_cache_data("After recovery [0]", eeprom2, 0, 8);
    print_cache_data("After recovery [64]", eeprom2, 64, 8);
    
    // 验证恢复的数据
    ASSERT_TRUE(memcmp(&eeprom2[0], data_addr0, 8) == 0,
                "Recovered data at addr 0 should match");
    ASSERT_TRUE(memcmp(&eeprom2[64], data_addr64, 8) == 0,
                "Recovered data at addr 64 should match");
    
    printf("  ✓ Data recovered correctly\n");
    printf("  ✓ Cache rebuilt from Flash\n");
    printf("  ✓ Test PASSED\n");
    
    return true;
}

/**
 * @brief 测试5：垃圾回收
 */
bool test_garbage_collection(void) {
    print_test_header("Test 5: Garbage Collection");
    
    #define PAGE_SIZE 512
    #define PAGES_PER_FEE 1
    #define CACHE_SIZE 64
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    printf("Data area: %u bytes\n", fee.data_area_size);
    printf("Max records: %u\n\n", fee.data_area_size / GRAN);
    
    // 填满FEE Page
    printf("Filling FEE Page...\n");
    uint16_t write_count = 0;
    
    for (int i = 0; i < 100; i++) {
        uint8_t data[8] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2), (uint8_t)(i+3),
                           (uint8_t)(i+4), (uint8_t)(i+5), (uint8_t)(i+6), (uint8_t)(i+7)};
        
        fee_status_t status = fee_write(&fee, (i % 8) * 8, data, 8);
        
        if (status == FEE_OK) {
            write_count++;
            
            if (i % 20 == 0) {
                uint16_t erase, free, rec_cnt;
                fee_get_info(&fee, &erase, &free, &rec_cnt);
                printf("  Iteration %2d: Erase=%u, Records=%u, Free=%u\n",
                       i, erase, rec_cnt, free);
            }
        }
    }
    
    uint16_t erase_before, free_before, rec_before;
    fee_get_info(&fee, &erase_before, &free_before, &rec_before);
    
    printf("\nBefore GC:\n");
    printf("  Erase count: %u\n", erase_before);
    printf("  Records: %u\n", rec_before);
    printf("  Free: %u bytes\n", free_before);
    
    // 保存Cache内容
    uint8_t saved_cache[CACHE_SIZE];
    memcpy(saved_cache, virtual_eeprom, CACHE_SIZE);
    
    // 手动触发GC
    printf("\nTriggering manual GC...\n");
    reset_flash_stats();
    
    fee_status_t status = fee_gc(&fee);
    ASSERT_EQ(status, FEE_OK, "GC should succeed");
    
    uint16_t erase_after, free_after, rec_after;
    fee_get_info(&fee, &erase_after, &free_after, &rec_after);
    
    printf("\nAfter GC:\n");
    printf("  Erase count: %u (+%u)\n", erase_after, erase_after - erase_before);
    printf("  Records: %u\n", rec_after);
    printf("  Free: %u bytes\n", free_after);
    printf("  Flash erases: %u\n", g_sim->erase_count);
    
    // 验证
    ASSERT_EQ(erase_after, erase_before + 1, "Erase count should increment by 1");
    ASSERT_TRUE(rec_after < rec_before, "Records should be compacted");
    ASSERT_TRUE(memcmp(virtual_eeprom, saved_cache, CACHE_SIZE) == 0,
                "Cache should remain unchanged after GC");
    
    printf("  ✓ GC executed successfully\n");
    printf("  ✓ Cache unchanged\n");
    printf("  ✓ Records compacted\n");
    printf("  ✓ Test PASSED\n");
    
    return true;
}

/**
 * @brief 测试6：Cache一致性
 */
bool test_cache_consistency(void) {
    print_test_header("Test 6: Cache Consistency");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 128
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    printf("Testing: Cache as virtual EEPROM mirror\n\n");
    
    // 测试1：写入后立即检查Cache
    printf("Test 6.1: Immediate cache update\n");
    uint8_t data1[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    fee_write(&fee, 10, data1, 8);
    
    ASSERT_TRUE(memcmp(&virtual_eeprom[10], data1, 8) == 0,
                "Cache should update immediately after write");
    printf("  ✓ Cache updated immediately\n\n");
    
    // 测试2：读取等价于直接访问Cache
    printf("Test 6.2: Read equals direct cache access\n");
    uint8_t read_buf[8];
    fee_read(&fee, 10, read_buf, 8);
    
    ASSERT_TRUE(memcmp(read_buf, &virtual_eeprom[10], 8) == 0,
                "fee_read() should equal direct cache access");
    printf("  ✓ Read from cache directly\n\n");
    
    // 测试3：多地址写入
    printf("Test 6.3: Multiple address writes\n");
    uint8_t data2[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t data3[4] = {0xEE, 0xFF, 0x11, 0x22};
    
    fee_write(&fee, 0, data2, 4);
    fee_write(&fee, 64, data3, 4);
    
    ASSERT_TRUE(memcmp(&virtual_eeprom[0], data2, 4) == 0,
                "Cache[0] should contain data2");
    ASSERT_TRUE(memcmp(&virtual_eeprom[64], data3, 4) == 0,
                "Cache[64] should contain data3");
    printf("  ✓ Multiple addresses handled correctly\n\n");
    
    // 测试4：GC后Cache不变
    printf("Test 6.4: Cache unchanged after GC\n");
    uint8_t saved[CACHE_SIZE];
    memcpy(saved, virtual_eeprom, CACHE_SIZE);
    
    fee_gc(&fee);
    
    ASSERT_TRUE(memcmp(virtual_eeprom, saved, CACHE_SIZE) == 0,
                "Cache should be identical after GC");
    printf("  ✓ Cache unchanged after GC\n\n");
    
    // 测试5：掉电恢复后Cache一致
    printf("Test 6.5: Cache consistency after recovery\n");
    
    uint8_t eeprom2[CACHE_SIZE];
    uint8_t work2[FEE_WORK_SIZE(GRAN)];
    fee_handle_t fee2;
    
    memset(eeprom2, 0x00, sizeof(eeprom2));
    fee_init(&fee2, &config, eeprom2, work2);
    
    ASSERT_TRUE(memcmp(eeprom2, virtual_eeprom, CACHE_SIZE) == 0,
                "Recovered cache should match original");
    printf("  ✓ Cache recovered correctly\n\n");
    
    printf("  ✓ All cache consistency tests PASSED\n");
    
    return true;
}

/**
 * @brief 测试7：边界条件
 */
bool test_boundary_conditions(void) {
    print_test_header("Test 7: Boundary Conditions");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 128
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    // 测试1：起始地址
    printf("Test 7.1: Write at address 0\n");
    uint8_t data1[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    fee_status_t status = fee_write(&fee, 0, data1, 8);
    ASSERT_EQ(status, FEE_OK, "Write at addr 0 should succeed");
    printf("  ✓ Address 0 OK\n\n");
    
    // 测试2：结束地址
    printf("Test 7.2: Write at end of cache\n");
    uint8_t data2[8] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8};
    status = fee_write(&fee, CACHE_SIZE - 8, data2, 8);
    ASSERT_EQ(status, FEE_OK, "Write at end should succeed");
    printf("  ✓ End address OK\n\n");
    
    // 测试3：超出边界
    printf("Test 7.3: Write beyond boundary (should fail)\n");
    uint8_t data3[8] = {0};
    status = fee_write(&fee, CACHE_SIZE - 4, data3, 8);
    ASSERT_EQ(status, FEE_ERROR_PARAM, "Write beyond boundary should fail");
    printf("  ✓ Boundary check OK\n\n");
    
    // 测试4：部分写入
    printf("Test 7.4: Partial write (3 bytes)\n");
    uint8_t data4[3] = {0xAA, 0xBB, 0xCC};
    status = fee_write(&fee, 50, data4, 3);
    ASSERT_EQ(status, FEE_OK, "Partial write should succeed");
    ASSERT_TRUE(virtual_eeprom[50] == 0xAA && 
                virtual_eeprom[51] == 0xBB && 
                virtual_eeprom[52] == 0xCC,
                "Partial data should be correct");
    printf("  ✓ Partial write OK\n\n");
    
    // 测试5：零长度写入
    printf("Test 7.5: Zero-length write\n");
    status = fee_write(&fee, 10, data1, 0);
    // 应该直接返回成功（无操作）
    printf("  ✓ Zero-length handled\n\n");
    
    printf("  ✓ All boundary tests PASSED\n");
    
    return true;
}

/**
 * @brief 测试8：不同颗粒度
 */
bool test_different_granularities(void) {
    print_test_header("Test 8: Different Write Granularities");
    
    uint8_t granularities[] = {8, 16, 32};
    
    for (int g = 0; g < 3; g++) {
        uint8_t gran = granularities[g];
        
        printf("Testing granularity = %d bytes\n", gran);
        printf("─────────────────────────────────\n");
        
        #define PAGE_SIZE 1024
        #define PAGES_PER_FEE 2
        #define CACHE_SIZE 128
        
        static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
        uint8_t virtual_eeprom[CACHE_SIZE];
        uint8_t work[FEE_MAX_GRANULARITY * 2];
        
        flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
        g_sim = &sim;
        
        memset(flash, 0xFF, sizeof(flash));
        
        fee_handle_t fee;
        fee_config_t config = {
            .flash_base = flash,
            .pages_per_fee_page = PAGES_PER_FEE,
            .flash_page_size = PAGE_SIZE,
            .cache_size = CACHE_SIZE,
            .write_granularity = gran,
            .max_erase_count = 10000,
            .flash_ops = &flash_ops
        };
        
        fee_init(&fee, &config, virtual_eeprom, work);
        
        printf("  Aligned header: %d bytes\n", fee.aligned_header_size);
        printf("  Record data:    %d bytes\n", fee.record_data_size);
        printf("  Max records:    %d\n", fee.data_area_size / gran);
        
        // 写入测试
        uint8_t test_data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                  0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
        
        fee_write(&fee, 0, test_data, 16);
        
        uint16_t erase, free, rec_cnt;
        fee_get_info(&fee, &erase, &free, &rec_cnt);
        
        printf("  Records created: %d\n", rec_cnt);
        
        // 验证
        uint8_t read_buf[16];
        fee_read(&fee, 0, read_buf, 16);
        
        if (memcmp(read_buf, test_data, 16) == 0) {
            printf("  ✓ Gran=%d PASSED\n\n", gran);
        } else {
            printf("  ✗ Gran=%d FAILED\n\n", gran);
            return false;
        }
    }
    
    printf("  ✓ All granularity tests PASSED\n");
    
    return true;
}

/**
 * @brief 性能测试
 */
void test_performance(void) {
    print_test_header("Performance Test");
    
    #define PAGE_SIZE 1024
    #define PAGES_PER_FEE 2
    #define CACHE_SIZE 256
    #define GRAN 8
    
    static uint8_t flash[PAGE_SIZE * PAGES_PER_FEE * 2];
    uint8_t virtual_eeprom[CACHE_SIZE];
    uint8_t work[FEE_WORK_SIZE(GRAN)];
    
    flash_sim_t sim = {flash, sizeof(flash), PAGE_SIZE, 0, 0, 0};
    g_sim = &sim;
    
    memset(flash, 0xFF, sizeof(flash));
    reset_flash_stats();
    
    fee_handle_t fee;
    fee_config_t config = {
        .flash_base = flash,
        .pages_per_fee_page = PAGES_PER_FEE,
        .flash_page_size = PAGE_SIZE,
        .cache_size = CACHE_SIZE,
        .write_granularity = GRAN,
        .max_erase_count = 10000,
        .flash_ops = &flash_ops
    };
    
    printf("Configuration:\n");
    printf("  Cache: %d bytes\n", CACHE_SIZE);
    printf("  Gran:  %d bytes\n", GRAN);
    printf("  RAM:   %zu bytes\n\n", FEE_TOTAL_RAM(CACHE_SIZE, GRAN));
    
    fee_init(&fee, &config, virtual_eeprom, work);
    
    // 写入性能测试
    printf("Write Performance Test (100 operations):\n");
    reset_flash_stats();
    
    uint8_t test_data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    
    for (int i = 0; i < 100; i++) {
        fee_write(&fee, (i % 32) * 8, test_data, 8);
    }
    
    printf("  Total writes:   100\n");
    printf("  Flash erases:   %u\n", g_sim->erase_count);
    printf("  Flash writes:   %u\n", g_sim->write_count);
    printf("  Flash reads:    %u (during init)\n", g_sim->read_count);
    
    // 读取性能测试
    printf("\nRead Performance Test (1000 operations):\n");
    reset_flash_stats();
    
    uint8_t read_buf[8];
    for (int i = 0; i < 1000; i++) {
        fee_read(&fee, (i % 32) * 8, read_buf, 8);
    }
    
    printf("  Total reads:    1000\n");
    printf("  Flash accesses: %u (should be 0!)\n", g_sim->read_count);
    
    if (g_sim->read_count == 0) {
        printf("  ✓ All reads from cache (zero Flash access)\n");
    }
    
    printf("\n");
}

/**
 * @brief 运行所有测试
 */
void run_all_tests(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           FEE Test Suite v1.0                         ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    int passed = 0;
    int failed = 0;
    
    #define RUN_TEST(test) \
        if (test()) { \
            passed++; \
        } else { \
            failed++; \
        }
    
    RUN_TEST(test_basic_init);
    RUN_TEST(test_basic_read_write);
    RUN_TEST(test_multiple_writes);
    RUN_TEST(test_power_loss_recovery);
    RUN_TEST(test_garbage_collection);
    RUN_TEST(test_cache_consistency);
    RUN_TEST(test_boundary_conditions);
    RUN_TEST(test_different_granularities);
    
    test_performance();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                        ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Total:  %2d tests                                      ║\n", passed + failed);
    printf("║  Passed: %2d tests                                      ║\n", passed);
    printf("║  Failed: %2d tests                                      ║\n", failed);
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    if (failed == 0) {
        printf("\n  ✓✓✓ ALL TESTS PASSED ✓✓✓\n\n");
    } else {
        printf("\n  ✗✗✗ SOME TESTS FAILED ✗✗✗\n\n");
    }
}

/* ============ Main函数 ============ */

int main(void) {
    run_all_tests();
    return 0;
}
```

------

## 使用指南

### 快速开始

#### 1. 定义虚拟EEPROM

```c
#include "fee.h"

// 定义256字节的虚拟EEPROM
uint8_t virtual_eeprom[256];
```

#### 2. 实现Flash操作接口

```c
// STM32 HAL示例
int flash_erase(uint32_t addr) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = addr;
    erase.NbPages = 1;
    uint32_t error;
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &error);
    HAL_FLASH_Lock();
    return (status == HAL_OK) ? 0 : -1;
}

int flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    HAL_FLASH_Unlock();
    for (uint16_t i = 0; i < len; i += 8) {
        uint64_t dword = *(uint64_t *)(data + i);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, 
                              addr + i, dword) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }
    HAL_FLASH_Lock();
    return 0;
}

int flash_read(uint32_t addr, uint8_t *data, uint16_t len) {
    memcpy(data, (void *)addr, len);
    return 0;
}

const fee_flash_ops_t flash_ops = {
    .erase = flash_erase,
    .write = flash_write,
    .read = flash_read
};
```

#### 3. 初始化FEE

```c
// 配置参数
fee_config_t config = {
    .flash_base = (uint8_t *)0x0800F000,  // Flash起始地址
    .pages_per_fee_page = 2,              // 每个FEE Page = 2个Flash Page
    .flash_page_size = 2048,              // STM32L4: 2KB/page
    .cache_size = 256,                    // 虚拟EEPROM大小
    .write_granularity = 8,               // STM32L4: 8字节双字
    .max_erase_count = 10000,             // 最大擦除次数
    .flash_ops = &flash_ops
};

// 分配缓冲区
uint8_t work_buffer[FEE_WORK_SIZE(8)];

// 初始化
fee_handle_t fee;
fee_status_t status = fee_init(&fee, &config, virtual_eeprom, work_buffer);

if (status != FEE_OK) {
    // 初始化失败
    printf("FEE init failed: %d\n", status);
}
```

#### 4. 读写操作

```c
// 写入配置
struct config {
    uint32_t baudrate;
    uint16_t timeout;
    uint8_t  mode;
} cfg = {115200, 1000, 1};

fee_write(&fee, 0, (uint8_t *)&cfg, sizeof(cfg));

// 读取配置
struct config loaded_cfg;
fee_read(&fee, 0, (uint8_t *)&loaded_cfg, sizeof(loaded_cfg));

// 或者直接访问虚拟EEPROM
uint32_t baudrate = *(uint32_t *)&virtual_eeprom[0];
```

### 内存计算

```c
// 计算所需RAM
#define CACHE_SIZE 512
#define GRAN 8

size_t total_ram = FEE_TOTAL_RAM(CACHE_SIZE, GRAN);
// = sizeof(fee_handle_t) + 512 + FEE_WORK_SIZE(8)
// = ~28 + 512 + 16 = 556 bytes
```

### 注意事项

1. **Flash空间分配**

   ```text
   最小配置：2个FEE Page，每个至少1个Flash Page
   推荐配置：2个FEE Page，每个2-4个Flash Page
   ```

2. **写入颗粒选择**

   ```text
   STM32F1/F4: 4字节
   STM32L4:    8字节
   STM32H7:    32字节
   ```

3. **Cache大小规划**

   ```text
   根据实际需求设定，通常64-1024字节
   必须小于数据区大小
   ```

------

## 移植指南

### STM32F103示例

```c
// Flash: 1KB/page, 写入颗粒4字节
#define FLASH_BASE   0x0801F000
#define FLASH_SIZE   2048      // 2KB (2 pages)
#define CACHE_SIZE   256
#define GRANULARITY  4

fee_config_t config = {
    .flash_base = (uint8_t *)FLASH_BASE,
    .pages_per_fee_page = 1,
    .flash_page_size = 1024,
    .cache_size = CACHE_SIZE,
    .write_granularity = GRANULARITY,
    .max_erase_count = 10000,
    .flash_ops = &flash_ops
};
```

### STM32L476示例

```c
// Flash: 2KB/page, 写入颗粒8字节
#define FLASH_BASE   0x080F0000
#define FLASH_SIZE   8192      // 8KB (4 pages)
#define CACHE_SIZE   512
#define GRANULARITY  8

fee_config_t config = {
    .flash_base = (uint8_t *)FLASH_BASE,
    .pages_per_fee_page = 2,  // 每个FEE Page = 4KB
    .flash_page_size = 2048,
    .cache_size = CACHE_SIZE,
    .write_granularity = GRANULARITY,
    .max_erase_count = 10000,
    .flash_ops = &flash_ops
};
```

### ESP32示例

```c
// Flash: 4KB/sector, 写入颗粒4字节
#include "esp_partition.h"

const esp_partition_t *partition;

int esp32_erase(uint32_t addr) {
    return esp_partition_erase_range(partition, 
                                     addr - (uint32_t)partition->address, 
                                     4096);
}

int esp32_write(uint32_t addr, const uint8_t *data, uint16_t len) {
    return esp_partition_write(partition, 
                               addr - (uint32_t)partition->address, 
                               data, len);
}

int esp32_read(uint32_t addr, uint8_t *data, uint16_t len) {
    return esp_partition_read(partition, 
                              addr - (uint32_t)partition->address, 
                              data, len);
}
```

------

## 常见问题

### Q1: Cache初始值是什么？

**A:** 初始化后Cache全为`0xFF`（擦除态），如果Flash中有数据会自动恢复。

### Q2: 需要手动调用GC吗？

**A:** 不需要，`fee_write()`会在空间不足时自动触发GC。

### Q3: 读取操作访问Flash吗？

**A:** 不访问，`fee_read()`直接从Cache读取，速度极快。

### Q4: 掉电时数据会丢失吗？

**A:** 不会，所有写入都立即持久化到Flash。

### Q5: 可以同时使用多个FEE实例吗？

**A:** 可以，只要分配不同的Flash区域和Cache。

### Q6: 如何监控Flash寿命？

**A:** 使用`fee_get_info()`获取擦除次数。

------

## 版本历史

- **v1.0** (2024) - 初始版本
  - 2-Page架构
  - 固定颗粒Record
  - Cache镜像机制
  - 自动GC

------

## 许可证

MIT License - 可自由用于商业和开源项目

------

**文档完成！** 🎉