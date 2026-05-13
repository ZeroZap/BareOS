UNITY_PATH   ?= ./unity
CC           = gcc
CFLAGS       = -std=c99 -I.. -I. -I$(UNITY_PATH) -I./test_xy_common -I./test_xy_stdlib -DUNITY_INCLUDE_DOUBLE -DUNITY_SUPPORT_64

# 源文件路径
SRCS         = $(wildcard ../*.c) \
               $(wildcard *.c) \
               $(wildcard test_xy_common/*.c) \
               $(wildcard test_xy_stdlib/*.c) \
			   $(wildcard test_xy_stdio/*.c) \
               $(UNITY_PATH)/unity.c

# 定义 build 目录
BUILD_DIR    = build

# 将 .o 文件路径转换为 build 目录下保持相同相对路径
OBJS         = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET       = $(BUILD_DIR)/test_runner

.PHONY: all clean mkbuilddir

all: mkbuilddir $(TARGET)

# 创建 build 目录结构
mkbuilddir:
	@mkdir -p $(dir $(OBJS))

# 链接目标
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# 编译规则，确保生成目录存在
$(BUILD_DIR)/%.o: %.c | mkbuilddir
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)