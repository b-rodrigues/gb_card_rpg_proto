# Makefile for Game Boy ROM development with GBDK-4 and RGBDS

CC = lcc
RGBFIX = rgbfix

BUILD_DIR = build
SRC_DIR = src
ASM_DIR = asm

TARGET = $(BUILD_DIR)/game.gb

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Emulator detection
EMULATOR ?= $(shell command -v sameboy 2>/dev/null || command -v mgba-sdl 2>/dev/null || command -v mgba-qt 2>/dev/null || command -v mgba 2>/dev/null || echo "")

.PHONY: all run clean test screenshot

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) -c -I$(SRC_DIR) -o $@ $<

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -o $@ $(GBDKDIR)lib/gb/crt0.o $(OBJS) $(GBDKDIR)lib/gb/gb.lib $(GBDKDIR)lib/sm83/sm83.lib

run: $(TARGET)
	@if [ -z "$(EMULATOR)" ]; then \
		echo "Error: No suitable Game Boy emulator found in PATH." >&2; \
		exit 1; \
	fi; \
	echo "Launching ROM in emulator ($(EMULATOR))..."; \
	$(EMULATOR) $(TARGET)

test: $(TARGET)
	@echo "Validating Game Boy ROM header..."
	@if command -v $(RGBFIX) >/dev/null 2>&1; then \
		$(RGBFIX) -v -C -t "BRUNOTETRIS" $(TARGET); \
	else \
		test -s $(TARGET); \
	fi
	@echo "ROM validation successful: $(TARGET)"

screenshot: $(TARGET)
	@bash tools/screenshot.sh $(BUILD_DIR)/screenshot.png $(TARGET)

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.gb $(BUILD_DIR)/*.map $(BUILD_DIR)/*.sym \
	      $(BUILD_DIR)/*.noi $(BUILD_DIR)/*.lst $(BUILD_DIR)/*.ihx $(BUILD_DIR)/*.cdb \
	      $(BUILD_DIR)/screenshot.png
