CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

TARGET = cachetest

BUILD_DIR = build

SRCS = $(wildcard *.c)

OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean

