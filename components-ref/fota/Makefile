# XY Component Makefile Template

# Compiler and flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2 -Iinc -I.
LDFLAGS ?=

# Windows system needs additional libraries
ifeq ($(OS),Windows_NT)
    LDFLAGS += -ladvapi32
endif

# Source and object directories
SRCDIR = .
TESTDIR = test
OBJDIR = build
INCDIR = inc

# Find all source files
SOURCES = $(wildcard *.c)
SUBDIRS = $(filter-out $(TESTDIR) $(OBJDIR) $(INCDIR), $(wildcard */))
SUBDIR_SOURCES = $(foreach dir,$(SUBDIRS),$(wildcard $(dir)*.c))

# Combine all sources
ALL_SOURCES = $(SOURCES) $(SUBDIR_SOURCES)
OBJECTS = $(ALL_SOURCES:%.c=$(OBJDIR)/%.o)

# Test sources
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(OBJDIR)/%.o)

# Library and executable names
LIBRARY = libxy_fota.a
TEST_EXECUTABLE = test_fota

.PHONY: all clean library test run_test help install

all: library test

# Create build directory
$(OBJDIR):
	mkdir -p $(OBJDIR)
	@for dir in $(SUBDIRS); do \
		mkdir -p $(OBJDIR)/$$dir; \
	done

# Compile source files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
library: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "Library $(LIBRARY) created successfully"

# Create test executable
test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Test executable $(TEST_EXECUTABLE) created successfully"

# Run tests
run_test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

# Clean
clean:
	rm -rf $(OBJDIR) $(LIBRARY) $(TEST_EXECUTABLE)

# Install headers and library (optional)
install: library
	@echo "Installing library to system directories"
	# cp $(LIBRARY) /usr/local/lib/
	# cp $(INCDIR)/*.h /usr/local/include/

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Compile library and test program"
	@echo "  library   - Compile library file"
	@echo "  test      - Compile test program"
	@echo "  run_test  - Run test program"
	@echo "  clean     - Clean generated files"
	@echo "  install   - Install library and headers"
	@echo "  help      - Show this help information"
