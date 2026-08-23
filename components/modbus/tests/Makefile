CC := gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1
SANITIZERS ?=

TARGET := test_mb_tiny.exe
SOURCES := ../mb_tiny.c xy_string_stubs.c test_mb_tiny.c
INCLUDES := -I.. -I../../clib

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SANITIZERS) $(INCLUDES) $(SOURCES) -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
