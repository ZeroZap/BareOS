# XY Broker Makefile

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I.
LDFLAGS =

# Source files
SRCS = xy_broker.c example.c
OBJS = $(SRCS:.c=.o)
TARGET = xy_broker_example

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c xy_broker.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean run test
