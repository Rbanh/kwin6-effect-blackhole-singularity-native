#pragma once

#include <effect/effectwindow.h>
#include <effect/offscreeneffect.h>
#include <effect/timeline.h>

#include <QVariant>
#include <QVector3D>

#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>

namespace KWin
{

class GLShader;

class BlackholeSingularityEffect : public OffscreenEffect
{
    Q_OBJECT

public:
    BlackholeSingularityEffect();
    ~BlackholeSingularityEffect() override;

    static bool supported();
    static bool enabledByDefault();

    bool isActive() const override;
    void reconfigure(ReconfigureFlags flags) override;
    void prePaintWindow(RenderView *view,
                        EffectWindow *w,
                        WindowPrePaintData &data,
                        std::chrono::milliseconds presentTime) override;
    void postPaintScreen() override;
    bool blocksDirectScanout() const override;

    int requestedEffectChainPosition() const override
    {
        return 50;
    }

protected:
    void apply(EffectWindow *window, int mask, WindowPaintData &data, WindowQuadList &quads) override;

private Q_SLOTS:
    void onWindowAdded(KWin::EffectWindow *w);
    void onWindowClosed(KWin::EffectWindow *w);
    void onWindowDeleted(KWin::EffectWindow *w);
    void onWindowDataChanged(KWin::EffectWindow *w, int role);

private:
    struct WindowAnimation
    {
        EffectWindowDeletedRef deletedRef;
        TimeLine timeline;

        bool opening = false;
        float seed = 0.0f;
        int durationMs = 700;

        bool waitingForStart = false;
        std::chrono::milliseconds startDelay{0};
        std::chrono::milliseconds waited{0};
        std::optional<std::chrono::milliseconds> lastPresentTime;

        bool blurSuppressed = false;
        QVariant previousForceBlur;
        QVariant previousForceContrast;
    };

    bool shouldAnimate(const EffectWindow *w) const;
    bool isGrabbed(const EffectWindow *w, int role) const;

    WindowAnimation &stateFor(EffectWindow *w);

    void suppressGlassRoles(EffectWindow *w, WindowAnimation &state);
    void restoreGlassRoles(EffectWindow *w, WindowAnimation &state);

    void clearWindowState(EffectWindow *w);
    void startOpen(EffectWindow *w);
    void startClose(EffectWindow *w);

    void loadShader();
    void applyStaticShaderUniforms();
    void applyWindowShaderUniforms(EffectWindow *w, const WindowAnimation &state, float progress);

    std::unique_ptr<GLShader> m_shader;
    int m_uProgressLocation = -1;
    int m_uForOpeningLocation = -1;
    int m_uIsFullscreenLocation = -1;
    int m_uDurationLocation = -1;
    int m_uSeedLocation = -1;
    int m_uWarpLocation = -1;
    int m_uGlowLocation = -1;
    int m_uAberrationLocation = -1;
    int m_uSpinLocation = -1;
    int m_uShrinkLocation = -1;
    int m_uOutlineLocation = -1;
    int m_uAccretionColorLocation = -1;
    int m_uRingColorLocation = -1;

    int m_durationMs = 700;
    int m_openDeferMs = 220;

    float m_warp = 1.35f;
    float m_glow = 2.45f;
    float m_aberration = 1.45f;
    float m_spin = 4.2f;
    float m_shrink = 1.12f;
    float m_outline = 1.85f;

    QVector3D m_accretionColor = QVector3D(0.38f, 0.62f, 1.0f);
    QVector3D m_ringColor = QVector3D(0.92f, 0.48f, 1.0f);

    float m_closeScaleTo = 0.64f;
    float m_closeOpacityTo = 0.0f;

    std::unordered_map<EffectWindow *, WindowAnimation> m_state;
};

} // namespace KWin
