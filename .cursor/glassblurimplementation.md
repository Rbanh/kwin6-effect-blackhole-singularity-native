# KWin 6 Glass Blur Integration: Implementation Plan

## Goals
The objective is to achieve "Perfect Integration" between the **Blackhole Singularity Native** effect and the **[kwin-effects-glass](https://github.com/4v3ngR/kwin-effects-glass)** plugin. 

Specifically:
1. When a window starts the Blackhole animation (opening/closing), any background blur (Glass) applied to that window should be warped, twisted, and sucked into the singularity alongside the window contents.
2. The transition should be seamless, with no "ghosting" of a static blur block behind the animated vortex.

## The Problem: Static Blur Artifacts
Currently, even with background capturing implemented, the "Glass" blur remains a static rectangle behind the animated window.

### Root Causes
1. **The Chain Position Conflict:** `kwin-effects-glass` runs at position 20. Our effect runs at 51. While we are "later" in the chain, `OffscreenEffect` redirects the window rendering. If the Glass effect has already drawn the blur into the framebuffer *behind* our redirection, our `blitFromRenderTarget` might be capturing the already-finalized background, or worse, the Glass effect might not be applying its blur to the redirected texture we use.
2. **Double Blurring / Redirection:** KWin's `Blur` effect (which Glass forks) typically renders *behind* the window during the window's own paint pass. Because we use `OffscreenEffect`, the window is rendered to a private texture. The background blur pass for that window is likely happening on the main screen framebuffer, which our shader does not automatically "suck in."
3. **Alpha Replacement Logic:** Our current shader attempts to "replace" the background by outputting alpha 1.0. However, if the static blur is already in the framebuffer from a previous pass, we are simply drawing our warped version *on top* of it, but any transparency in our vortex (which there is) allows the static blur to peak through.

## Reference: kwin-effects-glass
The `glass` plugin (forked from the built-in KWin Blur) works by:
- Intercepting `drawWindow`.
- Using a "Dual Kawase" multi-pass shader to blur the content *behind* the window.
- It relies on `WindowForceBlurRole` to identify windows that need this treatment.

## Proposed Strategy: "The Vortex Replacement"

To fix the static blur, we must ensure that the area occupied by the window is "cleared" or entirely overwritten by the shader's output, and that the "Glass" look is reconstructed entirely within our shader.

### Phase 1: Perfect Capture
We must ensure `drawWindow` captures the background *after* the Glass effect has processed it but *before* we draw the window. 
- **Action:** Verify `requestedEffectChainPosition` is high enough (currently 51) to ensure Glass (20) has completed its background pass for the current window.

### Phase 2: Total Framebuffer Takeover
The shader must not merely blend; it must **overwrite**.
- **Issue:** The static blur is already "under" us in the framebuffer.
- **Solution:** We need to find a way to tell KWin to not draw the background for this window, OR our shader must output a composite that is so opaque it covers the static artifacts, OR we must use a blend mode that replaces the destination.

### Phase 3: Synchronized Warping
The `uBackgroundSampler` must be warped using the exact same `shrunk` and `warped` UV coordinates as the window `sampler`.
- **Logic:** 
  ```glsl
  vec4 windowContent = sampleStraight(sampleUv);
  vec4 backgroundContent = sampleBackground(sampleUv); // Warped background
  vec3 composite = mix(backgroundContent.rgb, windowContent.rgb, windowContent.a);
  ```

## Next Steps for Investigation
1. **Check Stacking:** Investigate if we can force the window to be "transparent" to KWin's core but "opaque" in our effect to prevent the system blur from firing, allowing us to do the blur ourselves (too complex) or capturing the "clean" background and applying the glass vibes in our own shader.
2. **Debug Capture:** Add a debug mode to the shader that *only* outputs the captured `uBackgroundSampler` to see if it actually contains the blurred pixels or just the raw wallpaper.
3. **Region Clipping:** Ensure the `Region` passed to `drawWindow` isn't clipping our "Vortex" area, which is often larger than the window itself due to the accretion disk and glow.

---
*This document is vibe-coded but technically grounded.*
