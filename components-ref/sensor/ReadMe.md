# Sensor Framework

一个轻量级、可配置的传感器框架，支持OSAL和裸机环境。

## 特性

### 核心功能
- ✅ 统一的传感器接口
- ✅ 设备注册和管理
- ✅ 回调机制
- ✅ 多传感器支持

### 可选高级功能（通过宏配置）
- 🔧 **FIFO缓冲** - 批量数据采集
- ⚡ **中断支持** - 事件驱动模式
- 📐 **校准系统** - 偏移、比例校准
- 🔍 **数字滤波** - 移动平均、低通、中值滤波
- 🚀 **DMA传输** - 高效数据传输
- 🔋 **功耗管理** - 多级功耗模式
- 🎯 **阈值检测** - 上下限报警
- 🏃 **运动检测** - 运动、自由落体、敲击检测
- 🧭 **传感器融合** - 6轴/9轴姿态融合
- 🔬 **自测试** - 设备验证

## 快速开始

### 1. 配置框架

编辑 `sensor_config.h` 或使用预设配置：

```c
/* 启用需要的功能 */
#define SENSOR_ENABLE_FIFO          1
#define SENSOR_ENABLE_FILTER        1
#define SENSOR_USE_FLOAT            1


2. 编译
- 默认配置 make
- 最小配置 make minimal
- 完整配置 make full
- 自定义配置 make ENABLE_FIFO=1 ENABLE_FILTER=1 USE_FLOAT=0

3. 基础使用
#include "sensor_core.h"
#include "sensor_mpu6050.h"

int main(void)
{
    /* 创建传感器 */
    sensor_device_t *accel = mpu6050_create_accel("accel0", i2c_bus);

    /* 注册并初始化 */
    sensor_register(accel);
    sensor_init(accel);

    /* 读取数据 */
    sensor_data_t data;
    sensor_read(accel, &data);

    printf("Accel: X=%d, Y=%d, Z=%d mg\n",
           data.value.val_3axis.x,
           data.value.val_3axis.y,
           data.value.val_3axis.z);

    return 0;
}

配置选项
宏定义	默认值	说明
SENSOR_ENABLE_FIFO	1	FIFO缓冲支持
SENSOR_ENABLE_INTERRUPT	1	中断支持
SENSOR_ENABLE_CALIBRATION	1	校准功能
SENSOR_ENABLE_FILTER	1	数字滤波器
SENSOR_ENABLE_DMA	0	DMA传输
SENSOR_ENABLE_FUSION	1	传感器融合
SENSOR_ENABLE_POWER_MGMT	1	功耗管理
SENSOR_ENABLE_SELF_TEST	1	自测试
SENSOR_USE_FLOAT	1	浮点运算
SENSOR_USE_MALLOC	1	动态内存
SENSOR_USE_OSAL	0	OSAL支持
文件结构
sensor_framework/
├── sensor_config.h         # 框架配置
├── sensor_type.h           # 类型定义
├── sensor_core.h/c         # 核心实现
├── sensor_fifo.c           # FIFO实现
├── sensor_interrupt.c      # 中断实现
├── sensor_calibration.c    # 校准实现
├── sensor_filter.c         # 滤波器实现
├── sensor_dma.c            # DMA实现
├── sensor_power.c          # 功耗管理
├── sensor_threshold.c      # 阈值检测
├── sensor_motion.c         # 运动检测
├── sensor_fusion.c         # 传感器融合
├── sensor_self_test.c      # 自测试
├── sensor_mem_pool.c       # 静态内存池
├── sensor_mpu6050.h/c      # MPU6050驱动示例
├── example_basic.c         # 基础示例
├── example_advanced.c      # 高级示例
├── Makefile               # 构建脚本
└── README.md              # 本文档

内存占用
最小配置 (仅核心功能)
代码: ~5KB
RAM: ~200 bytes/sensor
完整配置 (所有功能)
代码: ~25KB
RAM: ~2KB/sensor (含FIFO)
支持的传感器
✅ MPU6050 (加速度计/陀螺仪)
🔄 其他传感器可通过实现驱动接口添加
许可证
MIT License

贡献
欢迎提交Issue和Pull Request！


## 总结

这个完整的 Sensor 框架具有以下特点：

### ✅ 核心优势

1. **高度可配置** - 通过宏实现功能的条件编译
2. **零依赖** - 可完全运行在裸机环境
3. **灵活适配** - 支持OSAL或裸机，动态或静态内存
4. **模块化设计** - 每个功能独立，便于裁剪
5. **资源可控** - 根据配置可从5KB到25KB代码大小

### 📦 完整组件

- ✅ 核心框架 (必需)
- ✅ FIFO缓冲 (可选)
- ✅ 中断机制 (可选)
- ✅ 校准系统 (可选)
- ✅ 数字滤波 (可选)
- ✅ DMA传输 (可选)
- ✅ 功耗管理 (可选)
- ✅ 阈值检测 (可选)
- ✅ 运动检测 (可选)
- ✅ 传感器融合 (可选)
- ✅ 自测试 (可选)
- ✅ 静态内存池 (可选)

### 🎯 使用场景

- 嵌入式系统
- RTOS应用
- 裸机程序
- 资源受限设备
- 高性能应用

通过简单的宏配置就能实现从最小化到全功能的灵活部署！