# XY ISO7816 Makefile
# Build ISO7816 protocol implementation

# Project name
PROJECT = xy_iso7816

# Compiler
CC = gcc
AR = ar

# Directories
SRC_DIR = .
INC_DIR = .
CLIB_DIR = ../../xy_clib
HAL_DIR = ../../../bsp/xy_hal

# Include paths
INCLUDES = -I$(INC_DIR) \
           -I$(CLIB_DIR) \
           -I$(HAL_DIR)/inc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -O2 $(INCLUDES)

# Debug flags
DEBUG_FLAGS = -g -DDEBUG -DXY_ISO7816_CFG_DEBUG_LOG=1

# Source files
SRCS = xy_iso7816.c

# Example source
EXAMPLE_SRCS = xy_iso7816_example.c

# Object files
OBJS = $(SRCS:.c=.o)
EXAMPLE_OBJS = $(EXAMPLE_SRCS:.c=.o)

# Output library
LIB = lib$(PROJECT).a

# Output example binary
EXAMPLE_BIN = $(PROJECT)_example

# Targets
.PHONY: all clean lib example debug

all: lib

lib: $(LIB)

example: $(EXAMPLE_BIN)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Build library
$(LIB): $(OBJS)
	@echo "Creating library $@..."
	$(AR) rcs $@ $^
	@echo "Library created successfully!"

# Build example
$(EXAMPLE_BIN): $(OBJS) $(EXAMPLE_OBJS)
	@echo "Building example..."
	$(CC) $(CFLAGS) -DISO7816_EXAMPLE_MAIN $^ -o $@
	@echo "Example built successfully!"

# Compile source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	@echo "Cleaning..."
	rm -f $(OBJS) $(EXAMPLE_OBJS) $(LIB) $(EXAMPLE_BIN)
	@echo "Clean complete!"

# Install (copy to lib directory)
install: lib
	@echo "Installing library..."
	@mkdir -p ../../../lib
	cp $(LIB) ../../../lib/
	@echo "Library installed to ../../../lib/"

# Help
help:
	@echo "XY ISO7816 Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build library (default)"
	@echo "  lib      - Build library only"
	@echo "  example  - Build example application"
	@echo "  debug    - Build with debug symbols"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install library to lib directory"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  Edit xy_iso7816_cfg.h for compile-time options"
