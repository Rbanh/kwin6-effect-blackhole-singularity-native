SHELL := /bin/bash

BUILD_TYPE ?= RelWithDebInfo
BUILD_DIR ?= /tmp/blackhole-native-build-$(notdir $(CURDIR))
BUILD_DIR_USER ?= /tmp/blackhole-native-build-user-local-$(notdir $(CURDIR))

EFFECT_ID := blackholesingularity
EFFECT_SO := $(BUILD_DIR)/bin/kwin/effects/plugins/blackholesingularity.so
CONFIG_SO := $(BUILD_DIR)/bin/kwin/effects/configs/kwin_blackholesingularity_config.so

SYSTEM_PLUGIN_DIR ?= /usr/lib/qt6/plugins
SYSTEM_EFFECT_DEST := $(SYSTEM_PLUGIN_DIR)/kwin/effects/plugins/blackholesingularity.so
SYSTEM_CONFIG_DEST := $(SYSTEM_PLUGIN_DIR)/kwin/effects/configs/kwin_blackholesingularity_config.so
HOT_A_ID ?= blackholesingularity_a
HOT_B_ID ?= blackholesingularity_b
HOT_A_DEST := $(SYSTEM_PLUGIN_DIR)/kwin/effects/plugins/$(HOT_A_ID).so
HOT_B_DEST := $(SYSTEM_PLUGIN_DIR)/kwin/effects/plugins/$(HOT_B_ID).so

LOCAL_PREFIX ?= $(HOME)/.local

INSTALL_CMD ?= install
ifeq ($(shell id -u),0)
  SUDO :=
else
  SUDO ?= sudo
endif

.PHONY: help configure build install install-hot install-user copy-system reload status prune-local clean

help:
	@echo "Targets:"
	@echo "  make build         - Configure and build (system build dir)"
	@echo "  make install       - Build + install both plugins system-wide + reload KWin"
	@echo "  make install-hot   - Build + hot-swap effect filename (_a/_b) + reload KWin"
	@echo "  make install-user  - Build + install to $$HOME/.local + reload KWin"
	@echo "  make reload        - Reload the effect in KWin"
	@echo "  make status        - Show built and installed plugin hashes"
	@echo "  make prune-local   - Remove stale ~/.local blackhole effect duplicates"
	@echo "  make clean         - Remove build directories"

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j$$(nproc)

copy-system: build
	$(SUDO) $(INSTALL_CMD) -Dm755 $(EFFECT_SO) $(SYSTEM_EFFECT_DEST)
	$(SUDO) $(INSTALL_CMD) -Dm755 $(CONFIG_SO) $(SYSTEM_CONFIG_DEST)

install: copy-system prune-local reload
	@echo "Installed system-wide:"
	@echo "  $(SYSTEM_EFFECT_DEST)"
	@echo "  $(SYSTEM_CONFIG_DEST)"

install-hot: build prune-local
	@set -e; \
	if [ -f "$(HOT_A_DEST)" ] && [ ! -f "$(HOT_B_DEST)" ]; then \
	  NEXT_ID="$(HOT_B_ID)"; OTHER_ID="$(HOT_A_ID)"; \
	elif [ -f "$(HOT_B_DEST)" ] && [ ! -f "$(HOT_A_DEST)" ]; then \
	  NEXT_ID="$(HOT_A_ID)"; OTHER_ID="$(HOT_B_ID)"; \
	elif [ -f "$(HOT_A_DEST)" ] && [ -f "$(HOT_B_DEST)" ]; then \
	  if [ "$(HOT_A_DEST)" -nt "$(HOT_B_DEST)" ]; then \
	    NEXT_ID="$(HOT_B_ID)"; OTHER_ID="$(HOT_A_ID)"; \
	  else \
	    NEXT_ID="$(HOT_A_ID)"; OTHER_ID="$(HOT_B_ID)"; \
	  fi; \
	else \
	  NEXT_ID="$(HOT_A_ID)"; OTHER_ID="$(HOT_B_ID)"; \
	fi; \
	NEXT_DEST="$(SYSTEM_PLUGIN_DIR)/kwin/effects/plugins/$${NEXT_ID}.so"; \
	$(SUDO) $(INSTALL_CMD) -Dm755 "$(EFFECT_SO)" "$${NEXT_DEST}"; \
	$(SUDO) $(INSTALL_CMD) -Dm755 "$(CONFIG_SO)" "$(SYSTEM_CONFIG_DEST)"; \
	kwriteconfig6 --file kwinrc --group Plugins --key "$${OTHER_ID}Enabled" false; \
	kwriteconfig6 --file kwinrc --group Plugins --key "$(EFFECT_ID)Enabled" false; \
	kwriteconfig6 --file kwinrc --group Plugins --key "$${NEXT_ID}Enabled" true; \
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect "$${OTHER_ID}" || true; \
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect "$(EFFECT_ID)" || true; \
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect "$${NEXT_ID}" || true; \
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect "$${NEXT_ID}" || true; \
	qdbus6 org.kde.KWin /KWin reconfigure || true; \
	echo "Hot-swapped active effect id: $${NEXT_ID}"; \
	echo "Installed: $${NEXT_DEST}"

install-user:
	cmake -S . -B $(BUILD_DIR_USER) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	  -DCMAKE_INSTALL_PREFIX="$(LOCAL_PREFIX)" \
	  -DKDE_INSTALL_PLUGINDIR=lib/qt6/plugins
	cmake --build $(BUILD_DIR_USER) -j$$(nproc)
	cmake --install $(BUILD_DIR_USER)
	$(MAKE) reload
	@echo "Installed user-local:"
	@echo "  $(LOCAL_PREFIX)/lib/qt6/plugins/kwin/effects/plugins/blackholesingularity.so"
	@echo "  $(LOCAL_PREFIX)/lib/qt6/plugins/kwin/effects/configs/kwin_blackholesingularity_config.so"

reload:
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect $(EFFECT_ID) || true
	qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect $(EFFECT_ID) || true
	qdbus6 org.kde.KWin /KWin reconfigure || true

prune-local:
	@set -e; \
	for d in "$(HOME)/.local/lib/qt6/plugins/kwin/effects/plugins" "$(HOME)/.local/lib/plugins/kwin/effects/plugins"; do \
	  if [ -d "$$d" ]; then \
	    for f in "$$d"/blackholesingularity*.so; do \
	      [ -e "$$f" ] || continue; \
	      echo "Removing stale local plugin: $$f"; \
	      rm -f "$$f" 2>/dev/null || $(SUDO) rm -f "$$f"; \
	    done; \
	  fi; \
	done

status:
	@echo "Build output hashes:"
	@if [ -f "$(EFFECT_SO)" ]; then sha256sum "$(EFFECT_SO)"; else echo "  missing: $(EFFECT_SO)"; fi
	@if [ -f "$(CONFIG_SO)" ]; then sha256sum "$(CONFIG_SO)"; else echo "  missing: $(CONFIG_SO)"; fi
	@echo
	@echo "Installed system hashes:"
	@if [ -f "$(SYSTEM_EFFECT_DEST)" ]; then sha256sum "$(SYSTEM_EFFECT_DEST)"; else echo "  missing: $(SYSTEM_EFFECT_DEST)"; fi
	@if [ -f "$(SYSTEM_CONFIG_DEST)" ]; then sha256sum "$(SYSTEM_CONFIG_DEST)"; else echo "  missing: $(SYSTEM_CONFIG_DEST)"; fi

clean:
	rm -rf "$(BUILD_DIR)" "$(BUILD_DIR_USER)"
