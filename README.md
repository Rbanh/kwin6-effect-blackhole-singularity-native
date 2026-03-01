# Blackhole Singularity Native (KWin C++ Effect)

This is a native (C++) KWin effect plugin for blackhole-themed open/close window animations.

> **Note:** This project is vibe-coded—developed via AI-assisted rapid iteration. It prioritizes high-performance visual results over traditional architectural conventions.

## Why this exists

This plugin follows the same open strategy as KWin's built-in Scale effect and is designed to work perfectly alongside [kwin-effects-glass](https://github.com/4v3ngR/kwin-effects-glass).

- Start open animation immediately on `windowAdded`
- Use explicit `from` values for shader progress, scale, and opacity
- Avoid delayed damage/warmup/fallback state machines
- Support for preserving background blur/glass effects during animation

## Installation

### Dependencies
- CMake
- Extra CMake Modules
- Plasma 6
- Qt 6
- KF6
- KWin development packages

### Building
```sh
git clone https://github.com/Rbanh/kwin6-effect-blackhole-singularity-native
cd kwin6-effect-blackhole-singularity-native
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

**Remove the *build* directory when rebuilding the effect.**

## Usage

1. Install the plugin.
2. Open the *Desktop Effects* page in *System Settings*.
3. Enable the *Blackhole Singularity Native* effect.

Effect id: `blackholesingularity`.
Config module id: `kwin_blackholesingularity_config`.
