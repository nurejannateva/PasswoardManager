CC ?= gcc
PKG_CONFIG ?= pkg-config

APP := password_manager
BUILD_DIR := build
SRC_DIR := src
INCLUDE_DIR := include

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

CFLAGS += -Wall -Wextra -g -I$(INCLUDE_DIR) $(shell $(PKG_CONFIG) --cflags gtk4 libadwaita-1)
LDLIBS += $(shell $(PKG_CONFIG) --libs gtk4 libadwaita-1)

.PHONY: all clean run

all: $(BUILD_DIR)/$(APP)

$(BUILD_DIR)/$(APP): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/$(APP)

clean:
	rm -rf $(BUILD_DIR)
