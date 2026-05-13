##
## @file Makefile
## @brief Makefile for XY HAL STM32U5
## @version 2.0
## @date 2026-02-28
##

# Toolchain
CC ?= arm-none-eabi-gcc
AR ?= arm-none-eabi-ar

# STM32U5 Configuration
STM32_FAMILY ?= STM32U5
STM32_DEVICE ?= STM32U5xx

# Directories
SRC_DIR := .
INC_DIR := ../inc
BUILD_DIR := build

# Source files
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=$(BUILD_DIR)/.o)

# Include paths
INCLUDES := -I$(SRC_DIR) -I$(INC_DIR)

# Compile flags
CFLAGS := -mcpu=cortex-m33
CFLAGS += -mthumb
CFLAGS += -mfloat-abi=soft
CFLAGS += -std=c99
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic
CFLAGS += -Wno-unused-parameter
CFLAGS += -fdata-sections
CFLAGS += -ffunction-sections
CFLAGS += -ffunction-sections
CFLAGS += -O2

# Definitions
DEFS := -DSTM32U5 -DSTM32U5xx -DSTM32_HAL_ENABLED -DUSE_HAL_DRIVER

# Library name
LIB_NAME := libxy_hal_stm32u5.a

# Default target
all: $(BUILD_DIR)/$(LIB_NAME)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "  CC      $<"
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $< -o $@

# Create library
$(BUILD_DIR)/$(LIB_NAME): $(OBJS)
	@echo "  AR      $@"
	$(AR) rcs $@ $^

# Clean
clean:
	@echo "  CLEAN"
	rm -rf $(BUILD_DIR)

# Distclean
distclean: clean
	@echo "  DISTCLEAN"

# Help
help:
	@echo "XY HAL STM32U5 Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the library (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CC          - C compiler (default: arm-none-eabi-gcc)"
	@echo "  STM32_FAMILY - STM32 family (default: STM32U5)"
	@echo "  STM32_DEVICE - STM32 device (default: STM32U5xx)"
	@echo ""
	@echo "Example:"
	@echo "  make clean && make all"

.PHONY: all clean distclean help
