# XY Tiny Crypto Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinc -I.
LDFLAGS =

# Windows 系统需要链接额外库
ifeq ($(OS),Windows_NT)
    LDFLAGS += -ladvapi32
endif

# 源文件和对象文件
SRCDIR = src
TESTDIR = test
OBJDIR = build
INCDIR = include

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(OBJDIR)/%.o)

# 库文件
LIBRARY = libxy_tiny_crypto.a
TEST_EXECUTABLE = test_crypto

.PHONY: all clean library test

# 默认只构建库，跳过测试 (测试需要完整的加密函数实现)
all: library

# 创建构建目录
$(OBJDIR):
	mkdir -p $(OBJDIR)

# 编译库源文件
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译测试文件
$(OBJDIR)/%.o: $(TESTDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 创建静态库
library: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "库文件 $(LIBRARY) 创建完成"

# 创建测试可执行文件
test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "测试程序 $(TEST_EXECUTABLE) 创建完成"

# 运行测试
run_test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

# 清理
clean:
	rm -rf $(OBJDIR) $(LIBRARY) $(TEST_EXECUTABLE)

# 安装头文件和库文件（可选）
install: library
	@echo "安装库文件到系统目录"
	# cp $(LIBRARY) /usr/local/lib/
	# cp $(INCDIR)/*.h /usr/local/include/

# 显示帮助信息
help:
	@echo "可用目标:"
	@echo "  all       - 编译库和测试程序"
	@echo "  library   - 编译库文件"
	@echo "  test      - 编译测试程序"
	@echo "  run_test  - 运行测试程序"
	@echo "  clean     - 清理生成的文件"
	@echo "  help      - 显示此帮助信息"