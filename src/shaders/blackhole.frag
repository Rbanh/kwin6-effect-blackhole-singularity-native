#version 140

uniform sampler2D sampler;
uniform sampler2D uBackgroundSampler;
uniform int textureWidth;
uniform int textureHeight;

in vec2 texcoord0;
out vec4 fragColor;

uniform int uForOpening;
uniform int uIsFullscreen;
uniform float uProgress;
uniform float uDuration;

uniform vec3 uAccretionColor;
uniform vec3 uRingColor;
uniform float uWarp;
uniform float uGlow;
uniform float uAberration;
uniform float uSpin;
uniform float uShrink;
uniform float uOutline;
uniform float uSeed;

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 rotate2d(vec2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec4 sampleStraight(vec2 uv)
{
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec4(0.0);
    }

    vec4 c = texture(sampler, vec2(uv.x, 1.0 - uv.y));
    if (c.a > 0.0) {
        c.rgb /= c.a;
    }
    return c;
}

vec4 sampleBackground(vec2 uv)
{
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    return texture(uBackgroundSampler, uv);
}

float edgeMask(vec2 uv, float widthPx)
{
    if (uIsFullscreen != 0) {
        return 1.0;
    }

    vec2 px = vec2(max(1.0, float(textureWidth)), max(1.0, float(textureHeight)));
    float wx = widthPx / px.x;
    float wy = widthPx / px.y;

    float mx = smoothstep(0.0, wx, uv.x) * smoothstep(0.0, wx, 1.0 - uv.x);
    float my = smoothstep(0.0, wy, uv.y) * smoothstep(0.0, wy, 1.0 - uv.y);
    return mx * my;
}

vec4 alphaOver(vec4 under, vec4 over)
{
    if (under.a <= 0.0) {
        return over;
    }
    if (over.a <= 0.0) {
        return under;
    }

    float alpha = mix(under.a, 1.0, over.a);
    vec3 rgb = mix(under.rgb * under.a, over.rgb, over.a) / max(alpha, 1e-6);
    return vec4(rgb, alpha);
}

void main()
{
    float t = saturate(uProgress);
    bool opening = (uForOpening != 0);

    float collapse = opening ? (1.0 - t) : t;
    float collapseShape = opening ? pow(collapse, 1.12) : pow(collapse, 0.96);

    float reveal = opening ? smoothstep(0.02, 0.88, t) : (1.0 - smoothstep(0.60, 1.0, t));
    float tailFade = opening ? smoothstep(0.0, 0.10, t) : (1.0 - smoothstep(0.78, 1.0, t));
    reveal *= tailFade;

    float auraFadeIn = opening ? smoothstep(0.0, 0.06, t) : 1.0;

    vec2 uv = vec2(texcoord0.x, 1.0 - texcoord0.y);
    float aspect = max(0.001, float(textureWidth) / max(1.0, float(textureHeight)));
    vec2 centered = uv * 2.0 - 1.0;
    centered.x *= aspect;

    float r = length(centered);
    vec2 dir = normalize(centered + vec2(1e-4, 0.0));
    vec2 ortho = vec2(-dir.y, dir.x);

    float theta = atan(centered.y, centered.x);
    float phase = collapse * 11.0 + uSeed * 37.0;
    float radialWave = sin(r * 34.0 - phase * 3.1);
    float noiseA = hash12(vec2(theta * 5.3 + phase * 0.6, r * 16.5 + phase * 0.2));
    float noiseB = hash12(vec2(theta * 12.7 - phase * 0.5, r * 8.9 + phase * 0.7));
    float turbulence = ((noiseA * 2.0 - 1.0) * 0.6) + ((noiseB * 2.0 - 1.0) * 0.4);

    float lensPull = uWarp * collapseShape * exp(-r * 2.15);
    lensPull *= (1.0 + 0.72 * radialWave * collapseShape + 0.36 * turbulence * collapseShape);

    float twist = uSpin * collapseShape * (0.25 + 2.35 * collapseShape * collapseShape) /
                  (r + 0.08 + 0.22 * (1.0 - collapseShape));
    vec2 swirled = rotate2d(centered - dir * lensPull * 0.62, twist);

    float shearAmt = 0.045 * collapseShape * collapseShape * (0.5 + 0.5 * turbulence);
    vec2 warped = swirled + ortho * shearAmt;

    float minShrink = max(0.004, 1.0 - 0.988 * clamp(uShrink, 0.0, 1.5));
    float shrink = mix(1.0, minShrink, smoothstep(0.0, 1.0, collapseShape));
    shrink *= (1.0 - 0.18 * collapseShape * exp(-r * 2.5));
    vec2 shrunk = warped / max(shrink, 1e-4);

    vec2 sampleUv = shrunk;
    sampleUv.x /= aspect;
    sampleUv = sampleUv * 0.5 + 0.5;

    float fringeEnvelope = smoothstep(0.0, 0.14, t) * (1.0 - smoothstep(0.82, 1.0, t));
    float splitAmount = (0.012 + 0.030 * collapseShape) * uAberration * fringeEnvelope * exp(-r * 1.85) *
                        (1.0 + 0.45 * abs(radialWave));
    vec2 split = dir * splitAmount;
    vec2 splitUv = split;
    splitUv.x /= aspect;

    vec4 base = sampleStraight(sampleUv);
    vec4 sampleR = sampleStraight(sampleUv + splitUv);
    vec4 sampleB = sampleStraight(sampleUv - splitUv);
    vec4 sampleG = sampleStraight(sampleUv + vec2(-splitUv.y, splitUv.x) * 0.25);
    base.rgb = vec3(sampleR.r, sampleG.g, sampleB.b);

    // Sample and warp the background to match refraction/glass look
    vec4 bg = sampleBackground(sampleUv);
    vec4 bgR = sampleBackground(sampleUv + splitUv);
    vec4 bgB = sampleBackground(sampleUv - splitUv);
    bg.rgb = vec3(bgR.r, bg.g, bgB.b);

    // Composite window over background
    vec3 compositeRgb = mix(bg.rgb, base.rgb, base.a);

    vec2 px = vec2(1.0 / max(1.0, float(textureWidth)), 1.0 / max(1.0, float(textureHeight)));
    float aC = base.a;
    float aL = sampleStraight(sampleUv - vec2(px.x, 0.0)).a;
    float aR = sampleStraight(sampleUv + vec2(px.x, 0.0)).a;
    float aU = sampleStraight(sampleUv + vec2(0.0, px.y)).a;
    float aD = sampleStraight(sampleUv - vec2(0.0, px.y)).a;

    float edgeDelta = abs(aC - aL) + abs(aC - aR) + abs(aC - aU) + abs(aC - aD);
    float outline = saturate(edgeDelta * 2.2) * smoothstep(0.0, 0.96, collapse) * clamp(uOutline, 0.0, 8.0);
    outline *= mix(0.8, 1.45, abs(radialWave));

    float diskRadius = mix(0.014, 0.53, pow(collapse, 1.04));
    float diskFeather = mix(0.19, 0.018, collapse) * smoothstep(0.0, 0.23, collapse);
    float diskMask = 1.0 - smoothstep(diskRadius - diskFeather, diskRadius + diskFeather, r);

    float ringWidth = mix(0.24, 0.020, collapse) * smoothstep(0.0, 0.20, collapse);
    float ringDist = abs(r - diskRadius) / max(0.001, ringWidth);
    float ring = exp(-ringDist * ringDist * 3.6);

    float spoke = pow(abs(sin(theta * (34.0 + noiseA * 18.0) + phase * 1.9 + noiseB * 6.2831)), 7.0);
    float filaments = mix(0.70, 2.15, noiseA) * mix(0.80, 1.55, spoke);
    ring *= filaments * collapseShape;

    float outerRingDist = abs(r - (diskRadius + ringWidth * (1.20 + 0.25 * turbulence))) /
                          max(0.001, ringWidth * 2.1);
    float outerRing = exp(-outerRingDist * outerRingDist * 2.2) * collapseShape * (0.45 + 0.35 * noiseB);

    float innerRingDist = abs(r - (diskRadius - ringWidth * 0.72)) / max(0.001, ringWidth * 1.35);
    float innerRing = exp(-innerRingDist * innerRingDist * 2.8) * collapseShape * 0.36;

    float jet = exp(-abs(sin(theta * 2.0 + phase * 0.7)) * 6.5) *
                exp(-abs(r - diskRadius) * 12.0) * collapseShape * (0.2 + 0.35 * noiseB);

    float haze = exp(-r * mix(8.8, 2.25, collapseShape)) * collapseShape * uGlow;
    float ringGlow = exp(-abs(r - diskRadius) * mix(26.0, 7.0, collapseShape)) * collapseShape * (0.55 + 0.45 * uGlow);
    float coreGlow = exp(-r * 28.0) * collapseShape * (0.55 + 0.95 * uGlow);

    vec3 singularityDark = vec3(0.002, 0.003, 0.009);
    float diskLife = smoothstep(0.0, 0.12, collapse);
    float darken = min(0.995, diskMask * (1.05 + 0.45 * collapseShape)) * diskLife;
    compositeRgb = mix(compositeRgb, singularityDark, darken);

    float edgeFade = edgeMask(uv, 2.0);
    float contentVis = edgeFade * reveal;
    float auraVis = opening ? (edgeFade * auraFadeIn) : contentVis;

    float alpha = mix(base.a, 1.0, diskMask * diskLife) * contentVis;
    alpha *= (1.0 - 0.08 * collapseShape * exp(-r * 3.0));

    float openingFlash = opening ? (smoothstep(0.0, 0.12, t) * (1.0 - smoothstep(0.22, 0.48, t))) : 0.0;

    vec3 outlineColor = mix(uAccretionColor, uRingColor, 0.72) * (0.45 + 0.55 * collapse);
    vec3 jetColor = mix(uRingColor, uAccretionColor, 0.35);

    vec3 auraRgb = uAccretionColor * (haze + coreGlow * 1.05) +
                   uRingColor * (ring + outerRing + innerRing * 0.7 + ringGlow * 0.55) +
                   outlineColor * outline * 1.25 +
                   jetColor * jet * 1.35 +
                   mix(uAccretionColor, uRingColor, 0.5) * openingFlash * 0.65;

    float auraAlpha = saturate(max(haze * 0.95 + coreGlow * 0.75,
                                   max(ring + outerRing * 0.9 + innerRing * 0.7, ringGlow * 0.85)) +
                               outline * 0.95 + jet * 0.9 + openingFlash * 0.7) * auraVis;

    vec4 mainLayer = vec4(compositeRgb, alpha);
    vec4 auraLayer = vec4(auraRgb, auraAlpha);
    vec4 outColor = alphaOver(mainLayer, auraLayer);
    outColor.rgb *= outColor.a;
    fragColor = outColor;
}
