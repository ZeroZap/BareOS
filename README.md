# BareOS

这是一个 IOT 裸机程序框架，涵盖：

- MCU 标准外设驱动
  - GPIO
  - Timer
  - UART
  - SPI
  - I2C
  - Flash
  - Misc
- AT 各自外设通讯模块
  - 4G
  - 以太网
  - 蓝牙
  - 卫星通讯模块
  - GNSS 模块

- 存储模块
  - EEPROM
  - 内部 Flash
  - 外部 Flash
  - 定长日志存储
    - 支持同时存在多个定长日志存放 erro，warning，event，usage

- 系统模块
  - 系统休眠处理
  - 系统复位原因
  - 系统版本
  - 系统重启
  - 系统各自模式定义
- 状态机模块

- tiny clib 自行实现 最小 clib
  - str 处理
  - memory
  - 高效宏

详细设计查看 [[架构设计.md]]
