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

INCLUDES = -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/world -I$(SRC_DIR)/battle -I$(SRC_DIR)/input -I$(SRC_DIR)/audio -I$(SRC_DIR)/ui -I$(SRC_DIR)/debug -I$(SRC_DIR)/screens -I$(SRC_DIR)/game

SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*/*.c)

# Debug-harness-only sources excluded from the release ROM.
# telemetry.c IS needed by gameplay (game.c/world.c emit events).
DEBUG_ONLY_SRCS = $(SRC_DIR)/debug/scenarios.c $(SRC_DIR)/debug/assertions.c $(SRC_DIR)/debug/rng.c
RELEASE_SRCS = $(filter-out $(DEBUG_ONLY_SRCS),$(SRCS))

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(RELEASE_SRCS))
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/debug/%.o,$(SRCS))

# Emulator detection
EMULATOR ?= $(shell command -v sameboy 2>/dev/null || command -v mgba-sdl 2>/dev/null || command -v mgba-qt 2>/dev/null || command -v mgba 2>/dev/null || echo "")

.PHONY: all release debug run run-debug test test-harness test-scenario state roundtrip screenshot lint memmap verify-oam verify-vram vram-check vram-text vram-dialogue gfx clean

all: $(TARGET)

release: $(TARGET)

debug: $(TARGET_DEBUG)

# Compile-to-assembly warning pass.  -Wall cannot be part of the normal
# build: sdcc's --use-stdout pipeline corrupts the .asm stream when warnings
# are enabled (they leak into stdout).  Compiling with -S surfaces the same
# warnings without invoking the assembler.
lint: gfx $(SRCS)
	@ok=1; \
	for f in $(SRCS); do \
		out=$$($(CC) -S -Wf-Wall $(INCLUDES) -o /dev/null "$$f" 2>&1 || true); \
		if echo "$$out" | grep -q "warning"; then \
			echo "=== $$f ==="; echo "$$out" | grep "warning"; ok=0; \
		fi; \
	done; \
	if [ "$$ok" = "1" ]; then echo "lint: no warnings"; else exit 1; fi

# Regenerate GB tile data headers from PNG assets (docs/graphics.md pipeline).
# Deterministic: rerunning produces byte-identical output.  Requires Pillow,
# which the Nix dev shell provides.
GFX_OUT_DIR = $(SRC_DIR)/gfx

gfx:
	@mkdir -p $(GFX_OUT_DIR)
	@python3 tools/png2gb.py assets/player_demo.png --name player_sprite_tile \
		-o $(GFX_OUT_DIR)/player_sprite_tile.h

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

# The VBlank ISR is copied to WRAM 0xC900 by crt0.s.  sdldgb auto-places
# _DATA at 0xC0A0 (after shadow OAM) and ignores ABS .org reservations, so
# _DATA is pinned at 0xC940 to keep every C symbol above the reserved
# 0xC900-0xC93F ISR region.  Without this, g_boot_phase/g_harness_mode land
# at 0xC89A-C89B and get corrupted by the fixed-layout WRAM (blank screen).
LDFLAGS = -Wl-b_DATA=0xC940

$(TARGET): gfx $(OBJS) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -o $@ build/crt0.o $(OBJS) $(GB_LITE) $(SM83_LITE)
	@$(RGBFIX) -v -C -m 0x1b -r 2 -t "GBCARDRPG" $@

$(TARGET_DEBUG): gfx $(OBJS_DEBUG) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -Wl-m -Wl-j -Wl-y -o $@ build/crt0.o $(OBJS_DEBUG) $(GB_LITE) $(SM83_LITE)
	@python3 tools/make_sym.py $(BUILD_DIR)/rpg_card_proto_debug.noi $(BUILD_DIR)/rpg_card_proto_debug.sym
	@$(RGBFIX) -v -C -m 0x1b -r 2 -t "GBCARDRPG" $@

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

# Verify the player sprite's real-OAM transition-hide across screen changes
# and scene (map) changes via the mGBA debugger (see tools/verify_oam.py).
verify-oam: debug
	@python3 tools/verify_oam.py

# Verify real-boot VRAM writes land (vsync-before-render + LCD-off boot
# redraw): boots the debug ROM WITHOUT harness mode and compares the real
# background ring against the WRAM mirror (see tools/verify_vram.py).
verify-vram: debug
	@python3 tools/verify_vram.py

# Font/VRAM pixel ground truth via PyBoy (see tools/vram_check.py).  Boots the
# real release ROM headlessly and reads VRAM directly -- the one place a
# pixel-level check is the correct tool (the char->tile mapping has no
# semantic representation).  Manual only; not part of the CI chain.
vram-check: release
	@python3 tools/vram_check.py

# Text-layer ground truth via PyBoy (see tools/vram_text_check.py): asserts
# generic text lands on the always-displayed BACKGROUND (0x9800), not the
# overworld-only WINDOW (0x9C00), by opening the ITEM menu with START and
# reading real VRAM.  Manual only; not part of the CI chain.
vram-text: release
	@python3 tools/vram_text_check.py

# Dialogue-box placement ground truth via PyBoy (see
# tools/vram_dialogue_check.py): asserts the dialogue box renders in the
# WINDOW tilemap (0x9C00) at fixed screen rows 12-17, not the scrolled
# background ring, so its text cannot shift with the camera or pollute the
# map ("text from other scenes").  Manual only; not part of the CI chain.
vram-dialogue: release
	@python3 tools/vram_dialogue_check.py

# Print a reproducible memory budget (code/WRAM usage, _HOME headroom vs the
# 0x8000 ceiling).  Exits non-zero if a documented invariant is violated.
memmap: debug
	@python3 tools/memmap.py $(BUILD_DIR)/rpg_card_proto_debug.map

clean:
	rm -rf $(BUILD_DIR)
