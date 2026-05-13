# Makefile for XY ChaCha20-Poly1305 Library

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -I. -I.. -I../inc -I../../xy_clib
LDFLAGS =

# Source files
SOURCES = xy_chacha20_poly1305.c
OBJECTS = $(SOURCES:.c=.o)

# Library name
LIBRARY = libxy_chacha20_poly1305.a

.PHONY: all clean library help

all: library

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
library: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "Library $(LIBRARY) created successfully"

# Clean
clean:
	rm -f $(OBJECTS) $(LIBRARY)

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Compile library"
	@echo "  library   - Compile library file only"
	@echo "  clean     - Clean generated files"
	@echo "  help      - Show this help information"
