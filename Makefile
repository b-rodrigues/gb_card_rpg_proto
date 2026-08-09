# Makefile for Game Boy ROM development with GBDK-4 and RGBDS

CC = lcc
RGBFIX = rgbfix

BUILD_DIR = build
SRC_DIR = src

TARGET = $(BUILD_DIR)/rpg_card_proto.gb
TARGET_DEBUG = $(BUILD_DIR)/rpg_card_proto_debug.gb

INCLUDES = -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/world -I$(SRC_DIR)/battle -I$(SRC_DIR)/input -I$(SRC_DIR)/audio -I$(SRC_DIR)/ui -I$(SRC_DIR)/debug

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(SRCS))

# Emulator detection
EMULATOR ?= $(shell command -v sameboy 2>/dev/null || command -v mgba-sdl 2>/dev/null || command -v mgba-qt 2>/dev/null || command -v mgba 2>/dev/null || echo "")

.PHONY: all release debug run run-debug test test-harness test-scenario screenshot clean

all: $(TARGET)

release: $(TARGET)

debug: $(TARGET_DEBUG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDES) -o $@ $<

$(BUILD_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) -c -DDEBUG_BUILD $(INCLUDES) -o $@ $<

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -o $@ $(GBDKDIR)lib/gb/crt0.o $(OBJS) $(GBDKDIR)lib/gb/gb.lib $(GBDKDIR)lib/sm83/sm83.lib

$(TARGET_DEBUG): $(OBJS_DEBUG) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -o $@ $(GBDKDIR)lib/gb/crt0.o $(OBJS_DEBUG) $(GBDKDIR)lib/gb/gb.lib $(GBDKDIR)lib/sm83/sm83.lib

run: $(TARGET)
	@if [ -z "$(EMULATOR)" ]; then \
		echo "Error: No suitable Game Boy emulator found in PATH." >&2; \
		exit 1; \
	fi; \
	echo "Launching ROM in emulator ($(EMULATOR))..."; \
	$(EMULATOR) $(TARGET)

run-debug: $(TARGET_DEBUG)
	@if [ -z "$(EMULATOR)" ]; then \
		echo "Error: No suitable Game Boy emulator found in PATH." >&2; \
		exit 1; \
	fi; \
	echo "Launching Debug ROM in emulator ($(EMULATOR))..."; \
	$(EMULATOR) $(TARGET_DEBUG)

test: $(TARGET)
	@echo "Validating Game Boy ROM header..."
	@if command -v $(RGBFIX) >/dev/null 2>&1; then \
		$(RGBFIX) -v -C -t "GBCARDRPG" $(TARGET); \
	else \
		test -s $(TARGET); \
	fi
	@echo "ROM validation successful: $(TARGET)"

test-harness: debug
	@python3 tools/dev.py test

test-scenario: debug
	@python3 tools/dev.py scenario $(SCENARIO)

screenshot: $(TARGET)
	@bash tools/screenshot.sh $(BUILD_DIR)/screenshot.png $(TARGET)

clean:
	rm -rf $(BUILD_DIR)
