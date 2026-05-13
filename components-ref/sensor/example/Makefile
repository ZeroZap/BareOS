# Sensor Framework Makefile

# 工具链配置
CC = gcc
AR = ar
RM = rm -f

# 目标平台 (可选: x86, arm-none-eabi, etc.)
PLATFORM ?= x86

# 编译选项
CFLAGS = -Wall -Wextra -O2 -std=c99
CFLAGS += -I.

# 调试选项
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -g -DSENSOR_DEBUG=1
else
    CFLAGS += -DSENSOR_DEBUG=0
endif

# 功能配置宏 (可以通过命令行覆盖)
CFLAGS += -DSENSOR_ENABLE_FIFO=$(ENABLE_FIFO)
CFLAGS += -DSENSOR_ENABLE_INTERRUPT=$(ENABLE_INTERRUPT)
CFLAGS += -DSENSOR_ENABLE_CALIBRATION=$(ENABLE_CALIBRATION)
CFLAGS += -DSENSOR_ENABLE_FILTER=$(ENABLE_FILTER)
CFLAGS += -DSENSOR_ENABLE_DMA=$(ENABLE_DMA)
CFLAGS += -DSENSOR_ENABLE_FUSION=$(ENABLE_FUSION)
CFLAGS += -DSENSOR_ENABLE_POWER_MGMT=$(ENABLE_POWER_MGMT)
CFLAGS += -DSENSOR_ENABLE_SELF_TEST=$(ENABLE_SELF_TEST)
CFLAGS += -DSENSOR_ENABLE_THRESHOLD=$(ENABLE_THRESHOLD)
CFLAGS += -DSENSOR_ENABLE_MOTION_DETECT=$(ENABLE_MOTION_DETECT)
CFLAGS += -DSENSOR_USE_FLOAT=$(USE_FLOAT)
CFLAGS += -DSENSOR_USE_MALLOC=$(USE_MALLOC)
CFLAGS += -DSENSOR_USE_OSAL=$(USE_OSAL)

# 默认配置
ENABLE_FIFO ?= 1
ENABLE_INTERRUPT ?= 1
ENABLE_CALIBRATION ?= 1
ENABLE_FILTER ?= 1
ENABLE_DMA ?= 0
ENABLE_FUSION ?= 1
ENABLE_POWER_MGMT ?= 1
ENABLE_SELF_TEST ?= 1
ENABLE_THRESHOLD ?= 1
ENABLE_MOTION_DETECT ?= 1
USE_FLOAT ?= 1
USE_MALLOC ?= 1
USE_OSAL ?= 0

# 链接库
LDFLAGS = -lm

# 源文件
CORE_SRC = sensor_core.c

# 条件编译源文件
SRC = $(CORE_SRC)

ifeq ($(ENABLE_FIFO), 1)
    SRC += sensor_fifo.c
endif

ifeq ($(ENABLE_INTERRUPT), 1)
    SRC += sensor_interrupt.c
endif

ifeq ($(ENABLE_CALIBRATION), 1)
    SRC += sensor_calibration.c
endif

ifeq ($(ENABLE_FILTER), 1)
    SRC += sensor_filter.c
endif

ifeq ($(ENABLE_DMA), 1)
    SRC += sensor_dma.c
endif

ifeq ($(ENABLE_POWER_MGMT), 1)
    SRC += sensor_power.c
endif

ifeq ($(ENABLE_THRESHOLD), 1)
    SRC += sensor_threshold.c
endif

ifeq ($(ENABLE_MOTION_DETECT), 1)
    SRC += sensor_motion.c
endif

ifeq ($(ENABLE_FUSION), 1)
    SRC += sensor_fusion.c
endif

ifeq ($(ENABLE_SELF_TEST), 1)
    SRC += sensor_self_test.c
endif

ifeq ($(USE_MALLOC), 0)
    SRC += sensor_mem_pool.c
endif

# 驱动源文件
DRIVER_SRC = sensor_mpu6050.c \
             sensor_sc7a20.c \
             sensor_aht20.c \
             sensor_bmp280.c \
             sensor_ap3216c.c \
             sensor_icm20608.c \
             sensor_qmc5883l.c

# 示例源文件
EXAMPLE_BASIC_SRC = example_basic.c
EXAMPLE_ADVANCED_SRC = example_advanced.c
# 多传感器示例
EXAMPLE_MULTI_SRC = example_multi_sensors.c

# 目标文件
OBJ = $(SRC:.c=.o)
DRIVER_OBJ = $(DRIVER_SRC:.c=.o)

# 输出
LIB = libsensor.a
EXAMPLE_BASIC = sensor_basic_demo
EXAMPLE_ADVANCED = sensor_advanced_demo

# 默认目标
all: $(LIB) examples

# 生成静态库
$(LIB): $(OBJ) $(DRIVER_OBJ)
	@echo "Creating library: $@"
	$(AR) rcs $@ $^
	@echo "Library created successfully"

# 生成基础示例
$(EXAMPLE_BASIC): $(EXAMPLE_BASIC_SRC) $(LIB)
	@echo "Building basic example: $@"
	$(CC) $(CFLAGS) -o $@ $< -L. -lsensor $(LDFLAGS)

# 生成高级示例
$(EXAMPLE_ADVANCED): $(EXAMPLE_ADVANCED_SRC) $(LIB)
	@echo "Building advanced example: $@"
	$(CC) $(CFLAGS) -o $@ $< -L. -lsensor $(LDFLAGS)

# 生成多传感器示例
$(EXAMPLE_MULTI): $(EXAMPLE_MULTI_SRC) $(LIB)
	@echo "Building multi-sensor example: $@"
	$(CC) $(CFLAGS) -o $@ $< -L. -lsensor $(LDFLAGS)

# 新增目标
EXAMPLE_MULTI = sensor_multi_demo

examples: $(EXAMPLE_BASIC) $(EXAMPLE_ADVANCED) $(EXAMPLE_MULTI)

# 编译示例
examples: $(EXAMPLE_BASIC) $(EXAMPLE_ADVANCED)

# 编译规则
%.o: %.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	@echo "Cleaning..."
	$(RM) $(OBJ) $(DRIVER_OBJ) $(LIB) $(EXAMPLE_BASIC) $(EXAMPLE_ADVANCED)
	@echo "Clean complete"

# 安装
install: $(LIB)
	@echo "Installing..."
	install -d $(DESTDIR)/usr/local/lib
	install -d $(DESTDIR)/usr/local/include/sensor
	install -m 644 $(LIB) $(DESTDIR)/usr/local/lib/
	install -m 644 sensor_config.h $(DESTDIR)/usr/local/include/sensor/
	install -m 644 sensor_type.h $(DESTDIR)/usr/local/include/sensor/
	install -m 644 sensor_core.h $(DESTDIR)/usr/local/include/sensor/
	@echo "Installation complete"

# 卸载
uninstall:
	@echo "Uninstalling..."
	$(RM) $(DESTDIR)/usr/local/lib/$(LIB)
	$(RM) -r $(DESTDIR)/usr/local/include/sensor
	@echo "Uninstall complete"

# 显示配置
config:
	@echo "=== Sensor Framework Configuration ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Debug: $(DEBUG)"
	@echo "FIFO: $(ENABLE_FIFO)"
	@echo "Interrupt: $(ENABLE_INTERRUPT)"
	@echo "Calibration: $(ENABLE_CALIBRATION)"
	@echo "Filter: $(ENABLE_FILTER)"
	@echo "DMA: $(ENABLE_DMA)"
	@echo "Fusion: $(ENABLE_FUSION)"
	@echo "Power Management: $(ENABLE_POWER_MGMT)"
	@echo "Self Test: $(ENABLE_SELF_TEST)"
	@echo "Threshold: $(ENABLE_THRESHOLD)"
	@echo "Motion Detect: $(ENABLE_MOTION_DETECT)"
	@echo "Float Support: $(USE_FLOAT)"
	@echo "Dynamic Memory: $(USE_MALLOC)"
	@echo "OSAL: $(USE_OSAL)"
	@echo "======================================"

# 帮助
help:
	@echo "Sensor Framework Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build library and examples (default)"
	@echo "  $(LIB)     - Build sensor library only"
	@echo "  examples   - Build example programs"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install library and headers"
	@echo "  uninstall  - Remove installed files"
	@echo "  config     - Show current configuration"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1              - Enable debug build"
	@echo "  ENABLE_FIFO=0/1      - Enable/disable FIFO support"
	@echo "  ENABLE_INTERRUPT=0/1 - Enable/disable interrupt support"
	@echo "  ENABLE_CALIBRATION=0/1 - Enable/disable calibration"
	@echo "  ENABLE_FILTER=0/1    - Enable/disable filter"
	@echo "  ENABLE_DMA=0/1       - Enable/disable DMA"
	@echo "  ENABLE_FUSION=0/1    - Enable/disable sensor fusion"
	@echo "  ENABLE_POWER_MGMT=0/1 - Enable/disable power management"
	@echo "  ENABLE_SELF_TEST=0/1 - Enable/disable self test"
	@echo "  USE_FLOAT=0/1        - Enable/disable floating point"
	@echo "  USE_MALLOC=0/1       - Use dynamic/static memory"
	@echo "  USE_OSAL=0/1         - Use OSAL or bare metal"
	@echo ""
	@echo "Examples:"
	@echo "  make                           # Build with default config"
	@echo "  make DEBUG=1                   # Build debug version"
	@echo "  make ENABLE_FIFO=0             # Build without FIFO"
	@echo "  make USE_MALLOC=0 USE_FLOAT=0  # Build for minimal system"

# 预设配置
minimal:
	$(MAKE) clean
	$(MAKE) ENABLE_FIFO=0 ENABLE_INTERRUPT=0 ENABLE_CALIBRATION=0 \
	        ENABLE_FILTER=0 ENABLE_DMA=0 ENABLE_FUSION=0 \
	        ENABLE_POWER_MGMT=0 ENABLE_SELF_TEST=0 \
	        USE_FLOAT=0 USE_MALLOC=0

full:
	$(MAKE) clean
	$(MAKE) ENABLE_FIFO=1 ENABLE_INTERRUPT=1 ENABLE_CALIBRATION=1 \
	        ENABLE_FILTER=1 ENABLE_DMA=1 ENABLE_FUSION=1 \
	        ENABLE_POWER_MGMT=1 ENABLE_SELF_TEST=1 \
	        USE_FLOAT=1 USE_MALLOC=1

.PHONY: all clean install uninstall examples config help minimal full