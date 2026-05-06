# ─── BusyBOF — Busybox-style BOFs for *nix ───────────────────
# Based on https://github.com/outflanknl/nix_bof_template
#
# Usage:
#   make              — build all BOFs + generate extension.json
#   make bf-cat       — build single BOF
#   make install      — copy to ~/.skovenet/tooling/busybof/
#   make clean        — remove build artifacts
#
# Cross-compile from Windows:
#   make CC=x86_64-linux-gnu-gcc
#
# Requirements: gcc or x86_64-linux-gnu-gcc (for cross-compilation)
# ──────────────────────────────────────────────────────────────

SHELL     := bash

# Auto-detect cross-compiler
CC        ?= $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null || echo gcc)
CFLAGS    := -c -fPIC -Os -Wall -Wextra -Werror -Wno-unused-parameter \
             -Wformat-overflow=2 -Wformat-security -Wshadow \
             -Wconversion -Wsign-conversion -Wstrict-prototypes \
             -I include -fno-stack-protector -fno-asynchronous-unwind-tables \
             -std=gnu11
OUTDIR    := build
INSTALL   := $(HOME)/.skovenet/tooling/busybof

# Discover bf-* dirs that contain a .c file
# Dir layout: bf-<name>/<name>.c  →  output: build/bf-<name>/<name>.o
BF_DIRS   := $(sort $(wildcard bf-*/))
TOOLS     := $(patsubst %/,%,$(BF_DIRS))

# Extract base name (without bf- prefix) from tool dir
base_name = $(patsubst bf-%,%,$(1))

.PHONY: all clean list install $(TOOLS)

all: $(foreach t,$(TOOLS),$(OUTDIR)/$(t)/$(call base_name,$(t)).o $(OUTDIR)/$(t)/extension.json)
	@echo ""
	@echo "  Built $(words $(TOOLS)) BOFs -> $(OUTDIR)/"
	@echo "  Run 'make install' to copy to $(INSTALL)/"
	@echo ""

# Generate rules for each bf-* tool
# $(1) = bf-foo, base = foo
# Source: bf-foo/foo.c  →  Output: build/bf-foo/foo.o + extension.json
define TOOL_RULE
$(OUTDIR)/$(1)/$(call base_name,$(1)).o: $(1)/$(call base_name,$(1)).c include/beacon.h include/bofdefs.h
	@mkdir -p $(OUTDIR)/$(1)
	@echo "  CC  $$<"
	@$$(CC) $$(CFLAGS) -o $$@ $$<

$(OUTDIR)/$(1)/extension.json: $(1)/extension.json
	@mkdir -p $(OUTDIR)/$(1)
	@cp -p $$< $$@

$(1): $(OUTDIR)/$(1)/$(call base_name,$(1)).o $(OUTDIR)/$(1)/extension.json
endef

$(foreach t,$(TOOLS),$(eval $(call TOOL_RULE,$(t))))

install: all
	@echo "[*] Installing BOFs to $(INSTALL)/"
	@for t in $(TOOLS); do \
		base=$${t#bf-}; \
		mkdir -p "$(INSTALL)/$$t"; \
		cp "$(OUTDIR)/$$t/$$base.o" "$(INSTALL)/$$t/"; \
		cp "$(OUTDIR)/$$t/extension.json" "$(INSTALL)/$$t/"; \
	done
	@echo "[✓] Installed $(words $(TOOLS)) BOFs"

clean:
	rm -rf $(OUTDIR)

list:
	@echo "Available BOFs ($(words $(TOOLS))): $(TOOLS)"
