#include "blackholesingularityeffect.h"

#include <effect/effecthandler.h>

#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>

#include <KConfigGroup>

#include <QColor>
#include <QRandomGenerator>
#include <QResource>
#include <QSet>

#include <algorithm>
#include <chrono>

int qInitResources_blackhole_shaders();

namespace KWin
{

using namespace std::chrono_literals;

namespace
{

QVector3D toVector3(const QColor &color)
{
    return QVector3D(color.redF(), color.greenF(), color.blueF());
}

QByteArray readShaderResource(const QStringList &paths, QString *selectedPath = nullptr)
{
    for (const QString &path : paths) {
        QResource resource(path);
        if (!resource.isValid()) {
            continue;
        }
        if (selectedPath) {
            *selectedPath = path;
        }
        if (resource.compressionAlgorithm() != QResource::NoCompression) {
            return resource.uncompressedData();
        }
        return QByteArray(reinterpret_cast<const char *>(resource.data()), resource.size());
    }
    return QByteArray();
}

const QSet<QString> s_blacklist = {
    QStringLiteral("ksmserver ksmserver"),
    QStringLiteral("ksmserver-logout-greeter ksmserver-logout-greeter"),
    QStringLiteral("ksplashqml ksplashqml"),
};

} // namespace

BlackholeSingularityEffect::BlackholeSingularityEffect()
{
    // Ensure embedded shader resources are registered when loaded as a plugin module.
    ::qInitResources_blackhole_shaders();

    connect(effects, &EffectsHandler::windowAdded, this, &BlackholeSingularityEffect::onWindowAdded);
    connect(effects, &EffectsHandler::windowClosed, this, &BlackholeSingularityEffect::onWindowClosed);
    connect(effects, &EffectsHandler::windowDeleted, this, &BlackholeSingularityEffect::onWindowDeleted);
    connect(effects, &EffectsHandler::windowDataChanged, this, &BlackholeSingularityEffect::onWindowDataChanged);

    loadShader();
    reconfigure(ReconfigureAll);
}

BlackholeSingularityEffect::~BlackholeSingularityEffect()
{
    for (auto &[window, state] : m_state) {
        unredirect(window);
        restoreGlassRoles(window, state);
    }
    m_state.clear();
}

bool BlackholeSingularityEffect::supported()
{
    return effects->isOpenGLCompositing() && effects->animationsSupported();
}

bool BlackholeSingularityEffect::enabledByDefault()
{
    return false;
}

bool BlackholeSingularityEffect::isActive() const
{
    return !m_state.empty();
}

void BlackholeSingularityEffect::reconfigure(ReconfigureFlags flags)
{
    OffscreenEffect::reconfigure(flags);

    KConfigGroup group(effects->config(), QStringLiteral("Effect-blackholesingularity"));

    m_durationMs = std::max(1, group.readEntry(QStringLiteral("Duration"), 700));
    m_openDeferMs = std::max(0, group.readEntry(QStringLiteral("OpenDeferMs"), 220));
    m_suppressGlass = group.readEntry(QStringLiteral("SuppressGlass"), true);

    m_warp = group.readEntry(QStringLiteral("Warp"), 1.35f);
    m_glow = group.readEntry(QStringLiteral("Glow"), 2.45f);
    m_aberration = group.readEntry(QStringLiteral("Aberration"), 1.45f);
    m_spin = group.readEntry(QStringLiteral("Spin"), 4.2f);
    m_shrink = group.readEntry(QStringLiteral("Shrink"), 1.12f);
    m_outline = group.readEntry(QStringLiteral("Outline"), 1.85f);

    m_closeScaleTo = group.readEntry(QStringLiteral("CloseScaleTo"), 0.64f);
    m_closeOpacityTo = group.readEntry(QStringLiteral("CloseOpacityTo"), 0.0f);

    const QColor defaultAccretion = QColor::fromRgbF(0.38, 0.62, 1.0, 1.0);
    const QColor defaultRing = QColor::fromRgbF(0.92, 0.48, 1.0, 1.0);
    m_accretionColor = toVector3(group.readEntry(QStringLiteral("AccretionColor"), defaultAccretion));
    m_ringColor = toVector3(group.readEntry(QStringLiteral("RingColor"), defaultRing));

    if (!m_shader) {
        loadShader();
    }
    applyStaticShaderUniforms();
}

void BlackholeSingularityEffect::prePaintWindow(RenderView *view,
                                                EffectWindow *w,
                                                WindowPrePaintData &data,
                                                std::chrono::milliseconds presentTime)
{
    auto it = m_state.find(w);
    if (it != m_state.end()) {
        WindowAnimation &state = it->second;

        if (state.waitingForStart) {
            if (state.lastPresentTime) {
                const auto delta = presentTime - *state.lastPresentTime;
                if (delta.count() > 0) {
                    state.waited += delta;
                }
            }
            state.lastPresentTime = presentTime;

            if (state.waited >= state.startDelay) {
                state.waitingForStart = false;
                state.lastPresentTime.reset();
            }
        }

        if (!state.waitingForStart) {
            state.timeline.advance(presentTime);
        }

        data.setTransformed();
    }

    effects->prePaintWindow(view, w, data, presentTime);
}

void BlackholeSingularityEffect::apply(EffectWindow *window, int mask, WindowPaintData &data, WindowQuadList &quads)
{
    Q_UNUSED(mask)
    Q_UNUSED(quads)

    auto it = m_state.find(window);
    if (it == m_state.end()) {
        return;
    }

    const WindowAnimation &state = it->second;
    const float progress = state.waitingForStart ? 0.0f : static_cast<float>(state.timeline.value());

    if (m_shader && m_shader->isValid()) {
        applyWindowShaderUniforms(window, state, progress);
    }

    const float openScaleFrom = std::clamp(m_closeScaleTo, 0.05f, 1.20f);
    const float openOpacityFrom = std::clamp(m_closeOpacityTo, 0.0f, 1.0f);

    const float scale = state.opening
        ? interpolate(openScaleFrom, 1.0f, progress)
        : interpolate(1.0f, m_closeScaleTo, progress);
    const float opacity = state.opening
        ? interpolate(openOpacityFrom, 1.0f, progress)
        : interpolate(1.0f, m_closeOpacityTo, progress);

    data *= scale;
    data.multiplyOpacity(opacity);
}

void BlackholeSingularityEffect::postPaintScreen()
{
    for (auto it = m_state.begin(); it != m_state.end();) {
        EffectWindow *w = it->first;
        WindowAnimation &state = it->second;

        w->addRepaintFull();

        if (!state.waitingForStart && state.timeline.done()) {
            unredirect(w);
            restoreGlassRoles(w, state);
            it = m_state.erase(it);
        } else {
            ++it;
        }
    }

    effects->postPaintScreen();
}

bool BlackholeSingularityEffect::blocksDirectScanout() const
{
    return false;
}

void BlackholeSingularityEffect::onWindowAdded(EffectWindow *w)
{
    if (effects->activeFullScreenEffect()) {
        return;
    }
    if (!shouldAnimate(w)) {
        return;
    }
    if (!w->isVisible()) {
        return;
    }
    if (isGrabbed(w, WindowAddedGrabRole)) {
        return;
    }

    startOpen(w);
}

void BlackholeSingularityEffect::onWindowClosed(EffectWindow *w)
{
    if (isGrabbed(w, WindowClosedGrabRole)) {
        return;
    }

    if (effects->activeFullScreenEffect() || !shouldAnimate(w) || !w->isVisible() || w->skipsCloseAnimation()) {
        clearWindowState(w);
        return;
    }

    startClose(w);
}

void BlackholeSingularityEffect::onWindowDeleted(EffectWindow *w)
{
    clearWindowState(w);
}

void BlackholeSingularityEffect::onWindowDataChanged(EffectWindow *w, int role)
{
    if (role != WindowAddedGrabRole && role != WindowClosedGrabRole) {
        return;
    }

    clearWindowState(w);
}

bool BlackholeSingularityEffect::shouldAnimate(const EffectWindow *w) const
{
    if (!w) {
        return false;
    }

    const QString windowClass = w->windowClass();
    if (windowClass == QLatin1String("plasmashell plasmashell") ||
        windowClass == QLatin1String("plasmashell org.kde.plasmashell")) {
        return w->hasDecoration();
    }

    if (windowClass == QLatin1String("spectacle org.kde.spectacle") &&
        w->tag() == QLatin1String("region-editor")) {
        return false;
    }

    if (s_blacklist.contains(windowClass)) {
        return false;
    }

    if (w->hasDecoration()) {
        return true;
    }

    if (w->isPopupWindow()) {
        return false;
    }

    if (w->isLockScreen() || w->isOutline()) {
        return false;
    }

    if (w->isX11Client() && !w->isManaged()) {
        return false;
    }

    return w->isNormalWindow() || w->isDialog();
}

bool BlackholeSingularityEffect::isGrabbed(const EffectWindow *w, int role) const
{
    if (!w) {
        return false;
    }
    const QVariant value = w->data(role);
    return value.isValid() && !value.isNull();
}

BlackholeSingularityEffect::WindowAnimation &BlackholeSingularityEffect::stateFor(EffectWindow *w)
{
    return m_state[w];
}

void BlackholeSingularityEffect::suppressGlassRoles(EffectWindow *w, WindowAnimation &state)
{
    if (state.blurSuppressed || !m_suppressGlass) {
        return;
    }

    state.previousForceBlur = w->data(WindowForceBlurRole);
    state.previousForceContrast = w->data(WindowForceBackgroundContrastRole);

    w->setData(WindowForceBlurRole, false);
    w->setData(WindowForceBackgroundContrastRole, false);
    state.blurSuppressed = true;
}

void BlackholeSingularityEffect::restoreGlassRoles(EffectWindow *w, WindowAnimation &state)
{
    if (!state.blurSuppressed) {
        return;
    }

    w->setData(WindowForceBlurRole, state.previousForceBlur);
    w->setData(WindowForceBackgroundContrastRole, state.previousForceContrast);
    state.previousForceBlur = QVariant();
    state.previousForceContrast = QVariant();
    state.blurSuppressed = false;
}

void BlackholeSingularityEffect::clearWindowState(EffectWindow *w)
{
    auto it = m_state.find(w);
    if (it == m_state.end()) {
        return;
    }

    unredirect(w);
    restoreGlassRoles(w, it->second);
    m_state.erase(it);
}

void BlackholeSingularityEffect::startOpen(EffectWindow *w)
{
    clearWindowState(w);

    WindowAnimation &state = stateFor(w);
    suppressGlassRoles(w, state);

    state.opening = true;
    state.seed = static_cast<float>(QRandomGenerator::global()->generateDouble());
    state.deletedRef = EffectWindowDeletedRef();

    const int durationMs = std::max(1, static_cast<int>(animationTime(std::chrono::milliseconds(m_durationMs))));
    state.durationMs = durationMs;

    state.timeline.reset();
    state.timeline.setDirection(TimeLine::Forward);
    state.timeline.setDuration(std::chrono::milliseconds(durationMs));
    state.timeline.setEasingCurve(QEasingCurve::OutCubic);

    state.waitingForStart = m_openDeferMs > 0;
    state.startDelay = std::chrono::milliseconds(std::max(0, m_openDeferMs));
    state.waited = std::chrono::milliseconds::zero();
    state.lastPresentTime.reset();

    redirect(w);
    if (m_shader && m_shader->isValid()) {
        setShader(w, m_shader.get());
    }

    effects->addRepaintFull();
}

void BlackholeSingularityEffect::startClose(EffectWindow *w)
{
    clearWindowState(w);

    WindowAnimation &state = stateFor(w);
    suppressGlassRoles(w, state);

    state.opening = false;
    state.seed = static_cast<float>(QRandomGenerator::global()->generateDouble());
    state.deletedRef = EffectWindowDeletedRef(w);

    const int durationMs = std::max(1, static_cast<int>(animationTime(std::chrono::milliseconds(m_durationMs))));
    state.durationMs = durationMs;

    state.timeline.reset();
    state.timeline.setDirection(TimeLine::Forward);
    state.timeline.setDuration(std::chrono::milliseconds(durationMs));
    state.timeline.setEasingCurve(QEasingCurve::InCubic);

    state.waitingForStart = false;
    state.startDelay = std::chrono::milliseconds::zero();
    state.waited = std::chrono::milliseconds::zero();
    state.lastPresentTime.reset();

    redirect(w);
    if (m_shader && m_shader->isValid()) {
        setShader(w, m_shader.get());
    }

    effects->addRepaintFull();
}

void BlackholeSingularityEffect::loadShader()
{
    m_shader.reset();
    m_uProgressLocation = -1;
    m_uForOpeningLocation = -1;
    m_uIsFullscreenLocation = -1;
    m_uDurationLocation = -1;
    m_uSeedLocation = -1;
    m_uWarpLocation = -1;
    m_uGlowLocation = -1;
    m_uAberrationLocation = -1;
    m_uSpinLocation = -1;
    m_uShrinkLocation = -1;
    m_uOutlineLocation = -1;
    m_uAccretionColorLocation = -1;
    m_uRingColorLocation = -1;

    QString selectedShaderPath;
    const QByteArray fragmentSource = readShaderResource(
        {
            QStringLiteral(":/effects/blackhole_singularity/shaders/blackhole_core.frag"),
            QStringLiteral(":/effects/blackhole_singularity/shaders/blackhole.frag"),
            // Legacy resource aliases from earlier builds:
            QStringLiteral(":/effects/blackhole_singularity/src/shaders/blackhole_core.frag"),
            QStringLiteral(":/effects/blackhole_singularity/src/shaders/blackhole.frag"),
        },
        &selectedShaderPath);

    if (fragmentSource.isEmpty()) {
        m_shader.reset();
        return;
    }

    m_shader = ShaderManager::instance()->generateCustomShader(
        ShaderTrait::MapTexture,
        QByteArray(),
        fragmentSource);

    if (!m_shader || !m_shader->isValid()) {
        m_shader.reset();
        return;
    }

    m_uProgressLocation = m_shader->uniformLocation("uProgress");
    m_uForOpeningLocation = m_shader->uniformLocation("uForOpening");
    m_uIsFullscreenLocation = m_shader->uniformLocation("uIsFullscreen");
    m_uDurationLocation = m_shader->uniformLocation("uDuration");
    m_uSeedLocation = m_shader->uniformLocation("uSeed");

    m_uWarpLocation = m_shader->uniformLocation("uWarp");
    m_uGlowLocation = m_shader->uniformLocation("uGlow");
    m_uAberrationLocation = m_shader->uniformLocation("uAberration");
    m_uSpinLocation = m_shader->uniformLocation("uSpin");
    m_uShrinkLocation = m_shader->uniformLocation("uShrink");
    m_uOutlineLocation = m_shader->uniformLocation("uOutline");
    m_uAccretionColorLocation = m_shader->uniformLocation("uAccretionColor");
    m_uRingColorLocation = m_shader->uniformLocation("uRingColor");

    applyStaticShaderUniforms();
}

void BlackholeSingularityEffect::applyStaticShaderUniforms()
{
    if (!m_shader || !m_shader->isValid()) {
        return;
    }

    ShaderBinder binder(m_shader.get());

    if (m_uWarpLocation >= 0) {
        m_shader->setUniform(m_uWarpLocation, m_warp);
    }
    if (m_uGlowLocation >= 0) {
        m_shader->setUniform(m_uGlowLocation, m_glow);
    }
    if (m_uAberrationLocation >= 0) {
        m_shader->setUniform(m_uAberrationLocation, m_aberration);
    }
    if (m_uSpinLocation >= 0) {
        m_shader->setUniform(m_uSpinLocation, m_spin);
    }
    if (m_uShrinkLocation >= 0) {
        m_shader->setUniform(m_uShrinkLocation, m_shrink);
    }
    if (m_uOutlineLocation >= 0) {
        m_shader->setUniform(m_uOutlineLocation, m_outline);
    }
    if (m_uAccretionColorLocation >= 0) {
        m_shader->setUniform(m_uAccretionColorLocation, m_accretionColor);
    }
    if (m_uRingColorLocation >= 0) {
        m_shader->setUniform(m_uRingColorLocation, m_ringColor);
    }
}

void BlackholeSingularityEffect::applyWindowShaderUniforms(EffectWindow *w,
                                                            const WindowAnimation &state,
                                                            float progress)
{
    if (!m_shader || !m_shader->isValid()) {
        return;
    }

    ShaderBinder binder(m_shader.get());

    if (m_uProgressLocation >= 0) {
        m_shader->setUniform(m_uProgressLocation, std::clamp(progress, 0.0f, 1.0f));
    }
    if (m_uForOpeningLocation >= 0) {
        m_shader->setUniform(m_uForOpeningLocation, state.opening ? 1 : 0);
    }
    if (m_uIsFullscreenLocation >= 0) {
        m_shader->setUniform(m_uIsFullscreenLocation, w->isFullScreen() ? 1 : 0);
    }
    if (m_uDurationLocation >= 0) {
        m_shader->setUniform(m_uDurationLocation, static_cast<float>(state.durationMs) / 1000.0f);
    }
    if (m_uSeedLocation >= 0) {
        m_shader->setUniform(m_uSeedLocation, state.seed);
    }

    if (m_uWarpLocation >= 0) {
        m_shader->setUniform(m_uWarpLocation, m_warp);
    }
    if (m_uGlowLocation >= 0) {
        m_shader->setUniform(m_uGlowLocation, m_glow);
    }
    if (m_uAberrationLocation >= 0) {
        m_shader->setUniform(m_uAberrationLocation, m_aberration);
    }
    if (m_uSpinLocation >= 0) {
        m_shader->setUniform(m_uSpinLocation, m_spin);
    }
    if (m_uShrinkLocation >= 0) {
        m_shader->setUniform(m_uShrinkLocation, m_shrink);
    }
    if (m_uOutlineLocation >= 0) {
        m_shader->setUniform(m_uOutlineLocation, m_outline);
    }
    if (m_uAccretionColorLocation >= 0) {
        m_shader->setUniform(m_uAccretionColorLocation, m_accretionColor);
    }
    if (m_uRingColorLocation >= 0) {
        m_shader->setUniform(m_uRingColorLocation, m_ringColor);
    }
}

} // namespace KWin
