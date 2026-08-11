# Makefile for Game Boy ROM development with GBDK-4 and RGBDS

CC = lcc
RGBFIX = rgbfix

BUILD_DIR = build
SRC_DIR = src

TARGET = $(BUILD_DIR)/rpg_card_proto.gb
TARGET_DEBUG = $(BUILD_DIR)/rpg_card_proto_debug.gb

# sdldgb links entire archives; trimmed closures keep the non-bankable
# _HOME area below 0x8000 on MBC5 (see docs/roadmap.md state foundation).
GB_LITE = $(BUILD_DIR)/gb_lite.lib
SM83_LITE = $(BUILD_DIR)/sm83_lite.lib

INCLUDES = -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/world -I$(SRC_DIR)/battle -I$(SRC_DIR)/input -I$(SRC_DIR)/audio -I$(SRC_DIR)/ui -I$(SRC_DIR)/debug -I$(SRC_DIR)/screens

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)

# Debug-harness-only sources excluded from the release ROM.
# telemetry.c IS needed by gameplay (game.c/world.c emit events).
DEBUG_ONLY_SRCS = $(SRC_DIR)/debug/scenarios.c $(SRC_DIR)/debug/assertions.c $(SRC_DIR)/debug/rng.c
RELEASE_SRCS = $(filter-out $(DEBUG_ONLY_SRCS),$(SRCS))

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(RELEASE_SRCS))
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(SRCS))

# Emulator detection
EMULATOR ?= $(shell command -v sameboy 2>/dev/null || command -v mgba-sdl 2>/dev/null || command -v mgba-qt 2>/dev/null || command -v mgba 2>/dev/null || echo "")

.PHONY: all release debug run run-debug test test-harness test-scenario state roundtrip screenshot clean

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

$(GB_LITE) $(SM83_LITE): $(OBJS) $(OBJS_DEBUG) | $(BUILD_DIR)
	python3 tools/make_lite_libs.py $(BUILD_DIR)

$(TARGET): $(OBJS) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 -o $@ build/crt0.o $(OBJS) $(GB_LITE) $(SM83_LITE)

$(TARGET_DEBUG): $(OBJS_DEBUG) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 -Wl-m -Wl-j -Wl-y -o $@ build/crt0.o $(OBJS_DEBUG) $(GB_LITE) $(SM83_LITE)
	@python3 tools/make_sym.py $(BUILD_DIR)/rpg_card_proto_debug.noi $(BUILD_DIR)/rpg_card_proto_debug.sym

build/crt0.o: src/crt0.s | $(BUILD_DIR)
	sdasgb -o $@ $<

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
	python3 tools/dev.py test

test-scenario: debug
	@python3 tools/dev.py scenario $(SCENARIO)

state: debug
	@python3 tools/dev.py state $(SCENARIO)

roundtrip: debug
	@python3 tools/dev.py roundtrip $(SCENARIO)

screenshot: $(TARGET)
	@bash tools/screenshot.sh $(BUILD_DIR)/screenshot.png $(TARGET)

clean:
	rm -rf $(BUILD_DIR)
