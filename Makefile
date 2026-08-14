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

.PHONY: all release debug run run-debug test test-harness test-scenario state roundtrip screenshot lint memmap verify-oam verify-font verify-vram gfx gfx-selftest clean

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
	@python3 tools/png2gb.py assets/world_tiles.png --name world_tiles \
		-o $(GFX_OUT_DIR)/world_tiles.h
	@python3 tools/png2gb.py assets/battle_bg.png --name battle_bg --tilemap --global \
		-o $(GFX_OUT_DIR)/battle_bg.h
	@python3 tools/png2gb.py assets/ui_frame.png --name ui_frame --tilemap --global \
		-o $(GFX_OUT_DIR)/ui_frame.h
	@python3 tools/png2gb.py assets/enemy_slime.png --name enemy_slime --global \
		-o $(GFX_OUT_DIR)/enemy_slime.h
	@python3 tools/png2gb.py assets/enemy_bat.png --name enemy_bat --global \
		-o $(GFX_OUT_DIR)/enemy_bat.h
	@python3 tools/png2gb.py assets/enemy_boss.png --name enemy_boss --global \
		-o $(GFX_OUT_DIR)/enemy_boss.h
	@python3 tools/png2gb.py assets/npc_mayor.png --name npc_mayor --global \
		-o $(GFX_OUT_DIR)/npc_mayor.h
	@python3 tools/png2gb.py assets/npc_guard.png --name npc_guard --global \
		-o $(GFX_OUT_DIR)/npc_guard.h
	@python3 tools/png2gb.py assets/npc_shopkeeper.png --name npc_shopkeeper --global \
		-o $(GFX_OUT_DIR)/npc_shopkeeper.h
	@python3 tools/png2gb.py assets/npc_merchant.png --name npc_merchant --global \
		-o $(GFX_OUT_DIR)/npc_merchant.h
	@python3 tools/png2gb.py assets/npc_amulet.png --name npc_amulet --global \
		-o $(GFX_OUT_DIR)/npc_amulet.h

# Host-side regression tests for the PNG -> GB converter (duplicate-tile
# detection, dedup/tilemap, sprite/OAM, validation rules).  Runs from the
# Nix dev shell; deterministic and byte-exact.
gfx-selftest:
	@python3 tools/png2gb.py --selftest

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
# 0xC900-0xC93F ISR region.  The memmap target rejects any overlap.
LDFLAGS = -Wl-b_DATA=0xC940

$(TARGET): gfx $(OBJS) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -o $@ build/crt0.o $(OBJS) $(GB_LITE) $(SM83_LITE)
	@$(RGBFIX) -C -m 0x1b -r 2 -t "GBCARDRPG" $@ 2>/dev/null || true

$(TARGET_DEBUG): gfx $(OBJS_DEBUG) build/crt0.o $(GB_LITE) $(SM83_LITE) | $(BUILD_DIR)
	$(CC) -no-crt -Wm-yc -Wl-yt0x19 -Wl-yo8 $(LDFLAGS) -Wl-m -Wl-j -Wl-y -o $@ build/crt0.o $(OBJS_DEBUG) $(GB_LITE) $(SM83_LITE)
	@python3 tools/make_sym.py $(BUILD_DIR)/rpg_card_proto_debug.noi $(BUILD_DIR)/rpg_card_proto_debug.sym
	@$(RGBFIX) -C -m 0x1b -r 2 -t "GBCARDRPG" $@ 2>/dev/null || true

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

# Verify the console-font -> VRAM tile mapping (HUD) via the DEBUG-only WRAM
# mirror (see tools/verify_font.py).  Guards against the -' ' offset dropping.
verify-font: debug
	@python3 tools/verify_font.py

# Verify the real-boot (non-harness) main loop writes the HUD tilemap during
# VBlank with no dropped cells, comparing real VRAM to the DEBUG mirror (see
# tools/verify_vram.py).  Guards the vsync-before-render and LCD-off redraw.
verify-vram: debug
	@python3 tools/verify_vram.py

# Print a reproducible memory budget (code/WRAM usage, _HOME headroom vs the
# 0x8000 ceiling).  Exits non-zero if a documented invariant is violated.
memmap: debug
	@python3 tools/memmap.py $(BUILD_DIR)/rpg_card_proto_debug.map

clean:
	rm -rf $(BUILD_DIR)
