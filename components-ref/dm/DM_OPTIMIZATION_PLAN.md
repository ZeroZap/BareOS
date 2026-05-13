# DM 组件优化计划

**日期**: 2026-03-05  
**目标**: 全面优化 DM 组件，消除 TODO，提升代码质量

---

## 📊 当前 DM 组件状态

### 已完成组件 (✅)

| 组件 | 状态 | 说明 |
|------|------|------|
| **fee/** | ✅ 完成 | FEE 核心 (完整 + Nano) |
| **eeprom/** | ✅ 完成 | EEPROM 接口层 |
| **xy_tlv/** | ✅ 完成 | TLV 编码/解码 |
| **xy_base64/** | ✅ 完成 | Base64 编解码 |
| **xy_mem/** | ✅ 完成 | 内存管理 |
| **coreJSON/** | ✅ 完成 | JSON 解析器 |
| **cjson/** | ✅ 完成 | cJSON 库 |
| **libyaml/** | ✅ 完成 | YAML 解析器 |
| **micro_ecc/** | ✅ 完成 | ECC 加密 |

### 待优化组件 (⚠️)

| 组件 | 优先级 | TODO 数 | 说明 |
|------|--------|--------|------|
| **xy_json.c** | 🔴 高 | 5 | JSON 解析不完整 |
| **xy_fs.c** | 🔴 高 | 3 | 文件系统接口未实现 |
| **xy_nvm/** | 🟡 中 | 2 | NVM 管理待完善 |
| **xy_flash/sd/** | 🟡 中 | 5 | SD 卡驱动待优化 |
| **xy_norflash/** | 🟡 中 | 1 | NOR Flash 待配置 |
| **xy_rbf/** | 🟢 低 | 0 | 位流处理，待文档 |
| **xy_rblk/** | 🟢 低 | 0 | 块设备，待文档 |

### 待实现组件 (❌)

| 组件 | 优先级 | 预计工时 | 说明 |
|------|--------|---------|------|
| **xy_kv/** | 🔴 高 | 4 小时 | 键值对存储 |
| **xy_wear_level/** | 🔴 高 | 4 小时 | 磨损均衡层 |
| **xy_fcb/** | 🟡 中 | 3 小时 | 循环缓冲区 |
| **xy_lfs/** | 🟡 中 | 6 小时 | 轻量文件系统 |
| **xy_fatfs/** | 🟡 中 | 4 小时 | FatFS 适配层 |

---

## 📋 详细优化计划

### 阶段 1: 修复现有 TODO (8 小时) 🔴

#### 1.1 xy_json.c 完善 (3 小时)

**当前问题**:
```c
/* TODO: 完整实现对象解析 */
/* TODO: 释放数组元素 */
/* TODO: 释放对象成员 */
/* TODO: 实现对象查找 */
/* TODO: 实现数组索引 */
```

**优化方案**:
```c
// 1. 完善对象解析
xy_json_status_t xy_json_parse_object(xy_json_t *json, const char *key);

// 2. 实现内存管理
void xy_json_free(xy_json_t *json);
void xy_json_array_free(xy_json_array_t *arr);

// 3. 实现查找 API
xy_json_t* xy_json_object_find(xy_json_t *obj, const char *key);
xy_json_t* xy_json_array_get(xy_json_array_t *arr, uint16_t index);
```

**文件结构**:
```
dm/
├── inc/
│   └── xy_json.h
├── src/
│   └── xy_json.c
└── tests/
    └── test_json.c
```

---

#### 1.2 xy_fs.c 完善 (2 小时)

**当前问题**:
```c
/* TODO: 调用底层 close */
/* TODO: 调用底层 read */
/* TODO: 调用底层 write */
```

**优化方案**:
```c
// 统一文件系统接口
typedef struct {
    int (*open)(const char *path, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void *buf, size_t len);
    int (*write)(int fd, const void *buf, size_t len);
    int (*seek)(int fd, long offset, int whence);
} xy_fs_ops_t;

// 注册底层驱动
xy_fs_register("flash", &flash_fs_ops);
xy_fs_register("sd", &sd_fs_ops);
```

---

#### 1.3 xy_nvm 完善 (3 小时)

**当前问题**:
- 数据指针转换为实体返回
- RAM 数据不连续，需要分段写

**优化方案**:
```c
// 1. 统一数据格式
typedef struct {
    uint32_t key;
    uint16_t len;
    uint8_t *data;
} xy_nvm_entry_t;

// 2. 优化写入流程
xy_nvm_status_t xy_nvm_write(uint32_t key, const void *data, uint16_t len);
xy_nvm_status_t xy_nvm_read(uint32_t key, void *data, uint16_t *len);
xy_nvm_status_t xy_nvm_delete(uint32_t key);
```

---

### 阶段 2: 实现新组件 (17 小时) 🔴

#### 2.1 xy_kv - 键值对存储 (4 小时)

**文件结构**:
```
xy_kv/
├── inc/
│   └── xy_kv.h
├── src/
│   └── xy_kv.c
├── backends/
│   ├── kv_flash.c    # Flash 后端
│   └── kv_file.c     # 文件后端
└── tests/
    └── test_kv.c
```

**API 设计**:
```c
// 初始化
xy_kv_status_t xy_kv_init(xy_kv_t *kv, xy_kv_config_t *cfg);

// 基本操作
xy_kv_status_t xy_kv_set(xy_kv_t *kv, const char *key, const void *value, uint16_t len);
xy_kv_status_t xy_kv_get(xy_kv_t *kv, const char *key, void *value, uint16_t *len);
xy_kv_status_t xy_kv_delete(xy_kv_t *kv, const char *key);

// 遍历
xy_kv_status_t xy_kv_foreach(xy_kv_t *kv, xy_kv_callback_t cb, void *user_data);
```

---

#### 2.2 xy_wear_level - 磨损均衡层 (4 小时)

**文件结构**:
```
xy_wear_level/
├── inc/
│   └── xy_wear_level.h
├── src/
│   └── xy_wear_level.c
└── tests/
    └── test_wear_level.c
```

**API 设计**:
```c
// 初始化
xy_wl_status_t xy_wl_init(xy_wl_t *wl, xy_wl_config_t *cfg);

// 读写接口
xy_wl_status_t xy_wl_read(xy_wl_t *wl, uint32_t addr, void *data, uint16_t len);
xy_wl_status_t xy_wl_write(xy_wl_t *wl, uint32_t addr, const void *data, uint16_t len);

// 磨损信息
uint16_t xy_wl_get_erase_count(xy_wl_t *wl, uint16_t block);
uint16_t xy_wl_get_avg_erase_count(xy_wl_t *wl);
```

**算法**:
- 动态磨损均衡
- 静态磨损均衡
- 坏块管理

---

#### 2.3 xy_fcb - 循环缓冲区 (3 小时)

**文件结构**:
```
xy_fcb/
├── inc/
│   └── xy_fcb.h
├── src/
│   └── xy_fcb.c
└── tests/
    └── test_fcb.c
```

**API 设计**:
```c
// 初始化
xy_fcb_status_t xy_fcb_init(xy_fcb_t *fcb, xy_fcb_config_t *cfg);

// 追加数据
xy_fcb_status_t xy_fcb_append(xy_fcb_t *fcb, const void *data, uint16_t len);

// 遍历
xy_fcb_status_t xy_fcb_iterate(xy_fcb_t *fcb, xy_fcb_callback_t cb, void *user_data);

// 清理旧数据
xy_fcb_status_t xy_fcb_truncate(xy_fcb_t *fcb, uint16_t keep_count);
```

---

#### 2.4 xy_lfs - 轻量文件系统 (6 小时)

**文件结构**:
```
xy_lfs/
├── inc/
│   └── xy_lfs.h
├── src/
│   └── xy_lfs.c
├── block_device/
│   ├── lfs_flash.c
│   └── lfs_sd.c
└── tests/
    └── test_lfs.c
```

**API 设计**:
```c
// 文件系统操作
xy_lfs_status_t xy_lfs_mount(xy_lfs_t *lfs, xy_lfs_config_t *cfg);
xy_lfs_status_t xy_lfs_unmount(xy_lfs_t *lfs);
xy_lfs_status_t xy_lfs_format(xy_lfs_t *lfs);

// 文件操作
xy_lfs_file_t* xy_lfs_open(xy_lfs_t *lfs, const char *path, int flags);
int xy_lfs_close(xy_lfs_file_t *file);
int xy_lfs_read(xy_lfs_file_t *file, void *buf, size_t len);
int xy_lfs_write(xy_lfs_file_t *file, const void *buf, size_t len);

// 目录操作
xy_lfs_dir_t* xy_lfs_opendir(xy_lfs_t *lfs, const char *path);
struct xy_lfs_dirent* xy_lfs_readdir(xy_lfs_dir_t *dir);
```

---

### 阶段 3: 文档与测试 (5 小时) 🟢

#### 3.1 完善文档

- [ ] xy_rbf/README.md
- [ ] xy_rblk/README.md
- [ ] xy_nvm/README.md
- [ ] xy_flash/README.md

#### 3.2 添加测试用例

- [ ] test_json.c
- [ ] test_fs.c
- [ ] test_kv.c
- [ ] test_wear_level.c
- [ ] test_fcb.c

---

## 📊 总计划

| 阶段 | 任务数 | 预计工时 | 优先级 |
|------|--------|---------|--------|
| **阶段 1** | 3 | 8 小时 | 🔴 高 |
| **阶段 2** | 4 | 17 小时 | 🔴 高 |
| **阶段 3** | 2 | 5 小时 | 🟢 低 |
| **总计** | **9** | **30 小时** | - |

---

## 🎯 实施策略

### 优先级排序

1. **阶段 1 (修复 TODO)** - 消除技术债务
2. **阶段 2 (新组件)** - 增强功能
3. **阶段 3 (文档测试)** - 提升质量

### 实施原则

1. **统一接口** - 所有组件使用相同 API 风格
2. **向后兼容** - 不破坏现有代码
3. **测试先行** - 每个组件配测试用例
4. **文档完善** - 每个组件配 README.md

---

## 📚 相关文档

- [DM_INTEGRATION_COMPLETE.md](DM_INTEGRATION_COMPLETE.md) - DM 整合报告
- [DM_INTEGRATION_PLAN.md](DM_INTEGRATION_PLAN.md) - DM 整合计划

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
