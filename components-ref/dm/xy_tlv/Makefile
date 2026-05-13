# XY TLV Management System - Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS =

# Directories
SRC_DIR = .
OBJ_DIR = build
BIN_DIR = bin

# Source files
SRCS = xy_tlv.c
EXAMPLE_SRC = example.c

# Object files
OBJS = $(OBJ_DIR)/xy_tlv.o
EXAMPLE_OBJ = $(OBJ_DIR)/example.o

# Targets
LIB_NAME = libxy_tlv.a
EXAMPLE_BIN = $(BIN_DIR)/tlv_example

.PHONY: all clean lib example test install

# Default target
all: lib example

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
lib: $(OBJS)
	ar rcs $(LIB_NAME) $(OBJS)
	@echo "Library $(LIB_NAME) created successfully"

# Build example
example: lib $(EXAMPLE_OBJ) | $(BIN_DIR)
	$(CC) $(EXAMPLE_OBJ) -L. -lxy_tlv $(LDFLAGS) -o $(EXAMPLE_BIN)
	@echo "Example binary $(EXAMPLE_BIN) created successfully"

# Run example
run: example
	@echo "Running TLV example..."
	@$(EXAMPLE_BIN)

# Test target (runs example)
test: run

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_NAME)
	@echo "Cleaned build artifacts"

# Install library (optional)
install: lib
	@echo "Installing xy_tlv library..."
	cp xy_tlv.h /usr/local/include/ || echo "Header install failed (may need sudo)"
	cp $(LIB_NAME) /usr/local/lib/ || echo "Library install failed (may need sudo)"

# Help
help:
	@echo "XY TLV Management System - Build Targets"
	@echo "========================================"
	@echo "make all      - Build library and example (default)"
	@echo "make lib      - Build static library only"
	@echo "make example  - Build example program"
	@echo "make run      - Run example program"
	@echo "make test     - Run tests (same as run)"
	@echo "make clean    - Remove build artifacts"
	@echo "make install  - Install library system-wide"
	@echo "make help     - Show this help message"
