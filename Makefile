# ─── BusyBOF — Busybox-style BOFs for *nix ───────────────────
# Based on https://github.com/outflanknl/nix_bof_template
#
# Usage:
#   make                — build all BOFs for both architectures
#   make all-x64        — build all BOFs for x86_64
#   make all-aarch64    — build all BOFs for aarch64
#   make bf-cat         — build single BOF (both arches)
#   make bf-cat-x64     — build single BOF (x86_64 only)
#   make bf-cat-aarch64 — build single BOF (aarch64 only)
#   make install        — copy to ~/.skovenet/tooling/busybof/
#   make clean          — remove build artifacts
#
# Requirements:
#   x86_64 : x86_64-linux-gnu-gcc  (or gcc on x86_64 host)
#   aarch64 : aarch64-linux-gnu-gcc
# ──────────────────────────────────────────────────────────────

SHELL := bash

CC_X64     ?= $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null || echo gcc)
CC_AARCH64 ?= $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null || echo aarch64-linux-gnu-gcc)

CFLAGS := -c -fPIC -Os -Wall -Wextra -Werror -Wno-unused-parameter \
           -Wformat-overflow=2 -Wformat-security -Wshadow \
           -Wconversion -Wsign-conversion -Wstrict-prototypes \
           -I include -fno-stack-protector -fno-asynchronous-unwind-tables \
           -std=gnu11

OUTDIR  := build
INSTALL := $(HOME)/.skovenet/tooling/busybof

BF_DIRS := $(sort $(wildcard bf-*/))
TOOLS   := $(patsubst %/,%,$(BF_DIRS))

base_name = $(patsubst bf-%,%,$(1))

# ── Per-arch output helpers ────────────────────────────────────
# obj_x64    bf-foo → build/bf-foo/foo.x64.obj
# obj_aarch64 bf-foo → build/bf-foo/foo.aarch64.obj
obj_x64     = $(OUTDIR)/$(1)/$(call base_name,$(1)).x64.obj
obj_aarch64 = $(OUTDIR)/$(1)/$(call base_name,$(1)).aarch64.obj

.PHONY: all all-x64 all-aarch64 clean list install \
        $(TOOLS) $(addsuffix -x64,$(TOOLS)) $(addsuffix -aarch64,$(TOOLS))

all: all-x64 all-aarch64

all-x64: $(foreach t,$(TOOLS),$(call obj_x64,$(t)) $(OUTDIR)/$(t)/extension.json)
	@echo ""
	@echo "  Built $(words $(TOOLS)) BOFs [x86_64] -> $(OUTDIR)/"
	@echo ""

all-aarch64: $(foreach t,$(TOOLS),$(call obj_aarch64,$(t)) $(OUTDIR)/$(t)/extension.json)
	@echo ""
	@echo "  Built $(words $(TOOLS)) BOFs [aarch64] -> $(OUTDIR)/"
	@echo ""

# ── Per-tool rules (both arches + extension.json) ─────────────
define TOOL_RULE
# x86_64
$(call obj_x64,$(1)): $(1)/$(call base_name,$(1)).c include/beacon.h include/bofdefs.h
	@mkdir -p $(OUTDIR)/$(1)
	@echo "  CC [x64]    $$<"
	@$$(CC_X64) $$(CFLAGS) -o $$@ $$<

# aarch64
$(call obj_aarch64,$(1)): $(1)/$(call base_name,$(1)).c include/beacon.h include/bofdefs.h
	@mkdir -p $(OUTDIR)/$(1)
	@echo "  CC [aarch64] $$<"
	@$$(CC_AARCH64) $$(CFLAGS) -o $$@ $$<

# extension.json
$(OUTDIR)/$(1)/extension.json: $(1)/extension.json
	@mkdir -p $(OUTDIR)/$(1)
	@cp -p $$< $$@

# Convenience targets
$(1):         $(call obj_x64,$(1)) $(call obj_aarch64,$(1)) $(OUTDIR)/$(1)/extension.json
$(1)-x64:     $(call obj_x64,$(1))     $(OUTDIR)/$(1)/extension.json
$(1)-aarch64: $(call obj_aarch64,$(1)) $(OUTDIR)/$(1)/extension.json
endef

$(foreach t,$(TOOLS),$(eval $(call TOOL_RULE,$(t))))

# ── Install ───────────────────────────────────────────────────
install: all
	@echo "[*] Installing BOFs to $(INSTALL)/"
	@for t in $(TOOLS); do \
		base=$${t#bf-}; \
		mkdir -p "$(INSTALL)/$$t"; \
		cp "$(OUTDIR)/$$t/$$base.x64.obj"     "$(INSTALL)/$$t/" 2>/dev/null || true; \
		cp "$(OUTDIR)/$$t/$$base.aarch64.obj" "$(INSTALL)/$$t/" 2>/dev/null || true; \
		cp "$(OUTDIR)/$$t/extension.json"     "$(INSTALL)/$$t/"; \
	done
	@echo "[✓] Installed $(words $(TOOLS)) BOFs"

clean:
	rm -rf $(OUTDIR)

list:
	@echo "Available BOFs ($(words $(TOOLS))): $(TOOLS)"
