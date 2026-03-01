# Blackhole Singularity Native (KWin C++ Effect)

This is a native (C++) KWin effect plugin for blackhole-themed open/close window animations.

> **Disclaimer:** This project is entirely vibe-coded.

## Why this exists

This plugin follows the same open strategy as KWin's built-in Scale effect and is designed to work perfectly alongside [kwin-effects-glass](https://github.com/4v3ngR/kwin-effects-glass).

- Start open animation immediately on `windowAdded`
- Use explicit `from` values for shader progress, scale, and opacity
- Avoid delayed damage/warmup/fallback state machines
- Support for preserving background blur/glass effects during animation

## Build

```bash
cmake -S . -B /tmp/blackhole-native-build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build /tmp/blackhole-native-build -j
```

## One-command install via Makefile

```bash
# system-wide install (copies both .so files + reloads KWin)
make install

# system-wide hot swap for development:
# alternates between blackholesingularity_a.so and blackholesingularity_b.so
# to avoid stale in-memory plugin code
make install-hot

# user-local install to ~/.local (also reloads KWin)
make install-user

# remove stale duplicate ~/.local plugin copies that can hide the settings gear
make prune-local
```

## Install (system-wide)

```bash
sudo cmake --install /tmp/blackhole-native-build
# or copy modules directly if your distro is using this path:
sudo install -Dm755 /tmp/blackhole-native-build/bin/kwin/effects/plugins/blackholesingularity.so \
  /usr/lib/qt6/plugins/kwin/effects/plugins/blackholesingularity.so
sudo install -Dm755 /tmp/blackhole-native-build/bin/kwin/effects/configs/kwin_blackholesingularity_config.so \
  /usr/lib/qt6/plugins/kwin/effects/configs/kwin_blackholesingularity_config.so
qdbus6 org.kde.KWin /KWin reconfigure
```

## Install (user-local, no root)

```bash
cmake -S . -B /tmp/blackhole-native-build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
  -DKDE_INSTALL_PLUGINDIR=lib/qt6/plugins
cmake --build /tmp/blackhole-native-build -j
cmake --install /tmp/blackhole-native-build
qdbus6 org.kde.KWin /KWin reconfigure
```

The plugin library installs to:

- `$HOME/.local/lib/qt6/plugins/kwin/effects/plugins/blackholesingularity.so` (user prefix)
- `$HOME/.local/lib/qt6/plugins/kwin/effects/configs/kwin_blackholesingularity_config.so` (user prefix)
- or system plugin path when installed with root prefix

Effect id: `blackholesingularity`.

Config module id: `kwin_blackholesingularity_config`.

Some distro/session setups only scan the system Qt plugin path for KWin effects. If user-local install does not show up, install system-wide.
