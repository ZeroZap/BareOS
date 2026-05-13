# 第三方库许可证清单

**日期**: 2026-03-05  
**政策**: ✅ 仅 MIT/Apache-2.0/BSD-2/3

---

## ✅ 白名单 (可直接使用)

### 网络协议栈

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **LwIP** | 2.1.3 | BSD-3 | ✅ | ✅ | ✅ | ✅ 推荐 |
| **picoHTTP** | 1.0 | MIT | ✅ | ✅ | ✅ | ✅ 推荐 |
| **NanoHTTP** | 2.0 | BSD-2 | ✅ | ✅ | ✅ | ✅ 推荐 |

### 文件系统

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **FatFS** | R0.15 | BSD-2 | ✅ | ✅ | ✅ | ✅ 推荐 |
| **LittleFS** | 2.5 | BSD-3 | ✅ | ✅ | ✅ | ✅ 推荐 |
| **SPIFFS** | 0.4 | MIT | ✅ | ✅ | ✅ | ✅ 推荐 |

### USB 协议栈

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **TinyUSB** | 0.16.0 | MIT | ✅ | ✅ | ✅ | ✅ 推荐 |
| **MUSB** | 1.0 | MIT | ✅ | ✅ | ✅ | ✅ 可选 |

### 图形库

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **LVGL** | 8.3.9 | MIT | ✅ | ✅ | ✅ | ✅ 推荐 |
| **U8g2** | 2.35 | BSD-3 | ✅ | ✅ | ✅ | ✅ 推荐 |

### 加密库

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **MbedTLS** | 3.5.0 | Apache-2.0 | ✅ | ✅ | ✅ | ✅ 推荐 |
| **TinyCrypt** | 0.3 | BSD-3 | ✅ | ✅ | ✅ | ✅ 推荐 |

### 蓝牙协议栈

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **NimBLE** | 1.4 | Apache-2.0 | ✅ | ✅ | ✅ | ✅ 推荐 |

### 无线通信

| 库名 | 版本 | 许可证 | 商用 | 修改 | 分发 | 状态 |
|------|------|--------|------|------|------|------|
| **ESP WiFi** | 4.4 | Apache-2.0 | ✅ | ✅ | ✅ | ✅ 推荐 |
| **LoRaMac** | 4.4.7 | Apache-2.0 | ✅ | ✅ | ✅ | ✅ 推荐 |

---

## ❌ 黑名单 (禁止使用)

### GPL 许可证

| 库名 | 版本 | 许可证 | 原因 | 替代方案 |
|------|------|--------|------|---------|
| **BlueZ** | 5.x | GPL-2.0 | 传染性 | NimBLE |
| **WolfSSL** | 5.x | GPL-2.0 | 传染性 | MbedTLS |
| **GNU TLS** | 3.x | GPL-3.0 | 传染性 | MbedTLS |
| **FFmpeg** | 5.x | GPL-3.0 | 传染性 | 无 |
| **SQLite** | 3.x | Public | 部分条款 | 无 |

### LGPL 许可证

| 库名 | 版本 | 许可证 | 原因 | 替代方案 |
|------|------|--------|------|---------|
| **libusb** | 1.0 | LGPL-2.1 | 动态链接要求 | TinyUSB |
| **Readline** | 8.x | GPL-3.0 | 传染性 | linenoise |

---

## ⚠️ 需审查 (有条件使用)

### EPL-1.0

| 库名 | 版本 | 许可证 | 条件 | 状态 |
|------|------|--------|------|------|
| **Eclipse Paho** | 1.3 | EPL-1.0 | 修改需开源 | ⚠️ 需审查 |

### MPL-2.0

| 库名 | 版本 | 许可证 | 条件 | 状态 |
|------|------|--------|------|------|
| **Mozilla scc** | 1.0 | MPL-2.0 | 修改需开源 | ⚠️ 需审查 |

---

## 📊 许可证统计

### 白名单统计

| 许可证 | 库数量 | 占比 |
|--------|--------|------|
| **MIT** | 6 | 40% |
| **Apache-2.0** | 5 | 33% |
| **BSD-2** | 2 | 13% |
| **BSD-3** | 2 | 13% |
| **总计** | 15 | 100% |

### 兼容性统计

| 类别 | 数量 | 占比 |
|------|------|------|
| **完全兼容** | 15 | 88% |
| **需审查** | 2 | 12% |
| **禁止使用** | 6 | - |

---

## 🔍 许可证检查流程

### 添加新库前

1. **查找 LICENSE 文件**
   ```bash
   find library/ -name "LICENSE*" -o -name "COPYING*"
   ```

2. **检查许可证类型**
   ```bash
   cat library/LICENSE
   ```

3. **确认兼容性**
   - [ ] 是否在白名单？
   - [ ] 是否需要审查？
   - [ ] 是否在黑名单？

4. **添加到清单**
   - [ ] 更新本文件
   - [ ] 更新 CMakeLists.txt
   - [ ] 添加版权声明

### 自动化检查

```bash
#!/bin/bash
# check_license.sh

LICENSE_FILE=$1

if grep -q "MIT License" $LICENSE_FILE; then
    echo "✅ MIT License - 兼容"
elif grep -q "Apache License" $LICENSE_FILE; then
    echo "✅ Apache License - 兼容"
elif grep -q "BSD License" $LICENSE_FILE; then
    echo "✅ BSD License - 兼容"
elif grep -q "GNU GPL" $LICENSE_FILE; then
    echo "❌ GPL License - 不兼容"
    exit 1
elif grep -q "GNU LGPL" $LICENSE_FILE; then
    echo "❌ LGPL License - 不兼容"
    exit 1
else
    echo "⚠️ 未知许可证 - 需人工审查"
    exit 2
fi
```

---

## 📝 许可证声明模板

### 源文件头

```c
/**
 * @file xxx.c
 * @brief XXX 功能实现
 * 
 * @copyright Copyright (c) 2026 XinYi Team
 * @copyright Copyright (c) 2023 Third Party Author
 * 
 * @license MIT License
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
```

### CMakeLists.txt 声明

```cmake
# LwIP (BSD-3-Clause)
# Copyright (c) 2001-2003 Swedish Institute of Computer Science
# All rights reserved.
add_subdirectory(third_party/network/lwip)

# FatFS (BSD-2-Clause)
# Copyright (c) 20xx, ChaN
# All rights reserved.
add_subdirectory(third_party/filesystem/fatfs)

# TinyUSB (MIT)
# Copyright (c) 2019 Ha Thach
# All rights reserved.
add_subdirectory(third_party/usb/tinyusb)

# LVGL (MIT)
# Copyright (c) 2020 LVGL Kft
# All rights reserved.
add_subdirectory(third_party/graphics/lvgl)
```

---

## 📚 相关资源

### 许可证文本

- [MIT License](https://opensource.org/licenses/MIT)
- [Apache-2.0](https://opensource.org/licenses/Apache-2.0)
- [BSD-2-Clause](https://opensource.org/licenses/BSD-2-Clause)
- [BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)

### 许可证检查工具

- [FOSSA](https://fossa.com/) - 开源许可证扫描
- [Black Duck](https://www.synopsys.com/software-integrity/security-testing/software-composition-analysis.html) - 成分分析
- [Licensee](https://github.com/licensee/licensee) - Ruby 许可证检查

---

## 🎯 最佳实践

### 1. 优先使用 MIT/Apache 库

```bash
# ✅ 推荐
TinyUSB (MIT)
LVGL (MIT)
MbedTLS (Apache-2.0)

# ❌ 避免
WolfSSL (GPL-2.0)
BlueZ (GPL-2.0)
```

### 2. 保留许可证文件

```bash
components/third_party/
├── lwip/
│   └── LICENSE          # ✅ 必须保留
├── fatfs/
│   └── LICENSE.txt      # ✅ 必须保留
└── tinyusb/
    └── LICENSE.txt      # ✅ 必须保留
```

### 3. 更新项目 LICENSE

```markdown
# XinYi Project License

This project uses the following third-party libraries:

## LwIP (BSD-3-Clause)
Copyright (c) 2001-2003 Swedish Institute of Computer Science

## FatFS (BSD-2-Clause)
Copyright (c) 20xx, ChaN

## TinyUSB (MIT)
Copyright (c) 2019 Ha Thach

## LVGL (MIT)
Copyright (c) 2020 LVGL Kft
```

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
