# OSAL RTOS 内核位置说明

## RTOS 内核已移至 third_party

为了保持代码结构清晰，所有 RTOS 内核源码已移至 `third_party/` 目录：

```
third_party/
├── freertos/
│   └── FreeRTOS/          # FreeRTOS 内核源码
├── CMSIS-RTX/
│   └── ...                # CMSIS-RTX 内核源码
└── rt-thread/
    └── ...                # RT-Thread 内核源码
```

## OSAL 目录结构

OSAL 目录现在仅包含适配层代码：

```
components/kernel/osal/
├── include/               # OSAL 公共头文件
├── src/                   # OSAL 通用实现
├── baremetal/             # Bare-metal 后端
├── freertos/              # FreeRTOS 后端适配
│   └── xy_os_freertos.c   # FreeRTOS 适配层
└── tests/                 # 单元测试
```

## 获取 RTOS 内核

### FreeRTOS

```bash
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel third_party/freertos/FreeRTOS
```

### CMSIS-RTX

```bash
git clone https://github.com/ARM-software/CMSIS-RTX third_party/CMSIS-RTX
```

### RT-Thread

```bash
git clone https://github.com/RT-Thread/rt-thread third_party/rt-thread
```

## 构建配置

在 CMakeLists.txt 中指定 RTOS 路径：

```cmake
# FreeRTOS
set(FREERTOS_ROOT ${CMAKE_SOURCE_DIR}/third_party/freertos/FreeRTOS)

# CMSIS-RTX
set(CMSIS_RTX_ROOT ${CMAKE_SOURCE_DIR}/third_party/CMSIS-RTX)

# RT-Thread
set(RTTHREAD_ROOT ${CMAKE_SOURCE_DIR}/third_party/rt-thread)
```

## 许可证

- **FreeRTOS**: MIT License
- **CMSIS-RTX**: Apache-2.0 License
- **RT-Thread**: Apache-2.0 License

---

**更新日期**: 2026-02-28  
**维护者**: XinYi Team
