#include <KCModule>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVariantList>
#include <QVBoxLayout>

#include <limits>

namespace
{

QColor defaultAccretionColor()
{
    return QColor::fromRgbF(0.38, 0.62, 1.0, 1.0);
}

QColor defaultRingColor()
{
    return QColor::fromRgbF(0.92, 0.48, 1.0, 1.0);
}

} // namespace

class BlackholeSingularityConfig : public KCModule
{
    Q_OBJECT

public:
    explicit BlackholeSingularityConfig(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
        : KCModule(parent, metaData)
    {
        Q_UNUSED(args)

        auto *rootLayout = new QVBoxLayout(widget());

        auto *timingGroup = new QGroupBox(tr("Animation Timing"), widget());
        auto *timingLayout = new QFormLayout(timingGroup);
        m_durationMs = new QSpinBox(timingGroup);
        m_durationMs->setRange(120, 4000);
        m_durationMs->setSuffix(tr(" ms"));
        timingLayout->addRow(tr("Duration"), m_durationMs);

        m_openDeferMs = new QSpinBox(timingGroup);
        m_openDeferMs->setRange(0, std::numeric_limits<int>::max());
        m_openDeferMs->setSuffix(tr(" ms"));
        timingLayout->addRow(tr("Open defer"), m_openDeferMs);

        m_suppressGlass = new QCheckBox(timingGroup);
        m_suppressGlass->setText(tr("Suppress Glass Effect during animation"));
        timingLayout->addRow(tr("Glass Effect"), m_suppressGlass);

        auto *shaderGroup = new QGroupBox(tr("Shader and Transform"), widget());
        auto *shaderLayout = new QFormLayout(shaderGroup);

        m_warp = makeDoubleSpin(shaderGroup, 0.0, 4.0, 0.01, 2);
        shaderLayout->addRow(tr("Warp"), m_warp);

        m_glow = makeDoubleSpin(shaderGroup, 0.0, 8.0, 0.01, 2);
        shaderLayout->addRow(tr("Glow"), m_glow);

        m_aberration = makeDoubleSpin(shaderGroup, 0.0, 5.0, 0.01, 2);
        shaderLayout->addRow(tr("Aberration"), m_aberration);

        m_spin = makeDoubleSpin(shaderGroup, -20.0, 20.0, 0.05, 2);
        shaderLayout->addRow(tr("Spin"), m_spin);

        m_shrink = makeDoubleSpin(shaderGroup, 0.0, 3.0, 0.01, 2);
        shaderLayout->addRow(tr("Shrink"), m_shrink);

        m_outline = makeDoubleSpin(shaderGroup, 0.0, 6.0, 0.01, 2);
        shaderLayout->addRow(tr("Outline"), m_outline);

        m_closeScaleTo = makeDoubleSpin(shaderGroup, 0.05, 1.20, 0.01, 2);
        shaderLayout->addRow(tr("Close scale"), m_closeScaleTo);

        m_closeOpacityTo = makeDoubleSpin(shaderGroup, 0.0, 1.0, 0.01, 2);
        shaderLayout->addRow(tr("Close opacity"), m_closeOpacityTo);

        m_accretionColorButton = new QPushButton(shaderGroup);
        shaderLayout->addRow(tr("Accretion color"), m_accretionColorButton);

        m_ringColorButton = new QPushButton(shaderGroup);
        shaderLayout->addRow(tr("Ring color"), m_ringColorButton);

        rootLayout->addWidget(timingGroup);
        rootLayout->addWidget(shaderGroup);
        rootLayout->addStretch();

        connect(m_durationMs, qOverload<int>(&QSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_openDeferMs, qOverload<int>(&QSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_suppressGlass, &QCheckBox::stateChanged, this, [this]() {
            onUserChanged();
        });
        connect(m_warp, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_glow, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_aberration, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_spin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_shrink, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_outline, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_closeScaleTo, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });
        connect(m_closeOpacityTo, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() {
            onUserChanged();
        });

        connect(m_accretionColorButton, &QPushButton::clicked, this, [this]() {
            chooseColor(m_accretionColor, m_accretionColorButton, tr("Select accretion color"));
        });
        connect(m_ringColorButton, &QPushButton::clicked, this, [this]() {
            chooseColor(m_ringColor, m_ringColorButton, tr("Select ring color"));
        });

        load();
    }

    void load() override
    {
        const Settings settings = readSettings();
        applySettings(settings);
        setNeedsSave(false);
    }

    void save() override
    {
        writeSettings(currentSettings());
        setNeedsSave(false);
    }

    void defaults() override
    {
        applySettings(defaultSettings());
        setNeedsSave(true);
    }

private:
    struct Settings
    {
        int durationMs = 700;
        int openDeferMs = 220;
        bool suppressGlass = true;

        double warp = 1.35;
        double glow = 2.45;
        double aberration = 1.45;
        double spin = 4.2;
        double shrink = 1.12;
        double outline = 1.85;

        double closeScaleTo = 0.64;
        double closeOpacityTo = 0.0;

        QColor accretionColor = defaultAccretionColor();
        QColor ringColor = defaultRingColor();
    };

    static Settings defaultSettings()
    {
        return Settings();
    }

    QDoubleSpinBox *makeDoubleSpin(QWidget *parent, double minimum, double maximum, double step, int decimals) const
    {
        auto *spin = new QDoubleSpinBox(parent);
        spin->setDecimals(decimals);
        spin->setRange(minimum, maximum);
        spin->setSingleStep(step);
        return spin;
    }

    Settings readSettings() const
    {
        Settings settings = defaultSettings();
        const KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        const KConfigGroup group(config, QStringLiteral("Effect-blackholesingularity"));

        settings.durationMs = group.readEntry(QStringLiteral("Duration"), settings.durationMs);
        settings.openDeferMs = group.readEntry(QStringLiteral("OpenDeferMs"), settings.openDeferMs);
        settings.suppressGlass = group.readEntry(QStringLiteral("SuppressGlass"), settings.suppressGlass);

        settings.warp = group.readEntry(QStringLiteral("Warp"), settings.warp);
        settings.glow = group.readEntry(QStringLiteral("Glow"), settings.glow);
        settings.aberration = group.readEntry(QStringLiteral("Aberration"), settings.aberration);
        settings.spin = group.readEntry(QStringLiteral("Spin"), settings.spin);
        settings.shrink = group.readEntry(QStringLiteral("Shrink"), settings.shrink);
        settings.outline = group.readEntry(QStringLiteral("Outline"), settings.outline);

        settings.closeScaleTo = group.readEntry(QStringLiteral("CloseScaleTo"), settings.closeScaleTo);
        settings.closeOpacityTo = group.readEntry(QStringLiteral("CloseOpacityTo"), settings.closeOpacityTo);

        settings.accretionColor = group.readEntry(QStringLiteral("AccretionColor"), settings.accretionColor);
        settings.ringColor = group.readEntry(QStringLiteral("RingColor"), settings.ringColor);
        return settings;
    }

    void writeSettings(const Settings &settings)
    {
        const KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
        KConfigGroup group(config, QStringLiteral("Effect-blackholesingularity"));

        group.writeEntry(QStringLiteral("Duration"), settings.durationMs);
        group.writeEntry(QStringLiteral("OpenDeferMs"), settings.openDeferMs);
        group.writeEntry(QStringLiteral("SuppressGlass"), settings.suppressGlass);

        group.writeEntry(QStringLiteral("Warp"), settings.warp);
        group.writeEntry(QStringLiteral("Glow"), settings.glow);
        group.writeEntry(QStringLiteral("Aberration"), settings.aberration);
        group.writeEntry(QStringLiteral("Spin"), settings.spin);
        group.writeEntry(QStringLiteral("Shrink"), settings.shrink);
        group.writeEntry(QStringLiteral("Outline"), settings.outline);

        group.writeEntry(QStringLiteral("CloseScaleTo"), settings.closeScaleTo);
        group.writeEntry(QStringLiteral("CloseOpacityTo"), settings.closeOpacityTo);

        group.writeEntry(QStringLiteral("AccretionColor"), settings.accretionColor);
        group.writeEntry(QStringLiteral("RingColor"), settings.ringColor);
        config->sync();
    }

    Settings currentSettings() const
    {
        Settings settings;
        settings.durationMs = m_durationMs->value();
        settings.openDeferMs = m_openDeferMs->value();
        settings.suppressGlass = m_suppressGlass->isChecked();

        settings.warp = m_warp->value();
        settings.glow = m_glow->value();
        settings.aberration = m_aberration->value();
        settings.spin = m_spin->value();
        settings.shrink = m_shrink->value();
        settings.outline = m_outline->value();

        settings.closeScaleTo = m_closeScaleTo->value();
        settings.closeOpacityTo = m_closeOpacityTo->value();

        settings.accretionColor = m_accretionColor;
        settings.ringColor = m_ringColor;
        return settings;
    }

    void applySettings(const Settings &settings)
    {
        m_syncing = true;

        m_durationMs->setValue(settings.durationMs);
        m_openDeferMs->setValue(settings.openDeferMs);
        m_suppressGlass->setChecked(settings.suppressGlass);

        m_warp->setValue(settings.warp);
        m_glow->setValue(settings.glow);
        m_aberration->setValue(settings.aberration);
        m_spin->setValue(settings.spin);
        m_shrink->setValue(settings.shrink);
        m_outline->setValue(settings.outline);

        m_closeScaleTo->setValue(settings.closeScaleTo);
        m_closeOpacityTo->setValue(settings.closeOpacityTo);

        m_accretionColor = settings.accretionColor;
        m_ringColor = settings.ringColor;
        setColorButton(m_accretionColorButton, m_accretionColor);
        setColorButton(m_ringColorButton, m_ringColor);

        m_syncing = false;
    }

    void chooseColor(QColor &target, QPushButton *button, const QString &title)
    {
        const QColor color = QColorDialog::getColor(target, widget(), title, QColorDialog::ShowAlphaChannel);
        if (!color.isValid() || color == target) {
            return;
        }
        target = color;
        setColorButton(button, target);
        onUserChanged();
    }

    void setColorButton(QPushButton *button, const QColor &color)
    {
        button->setText(color.name(QColor::HexArgb));
        button->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; }").arg(color.name(QColor::HexArgb)));
    }

    void onUserChanged()
    {
        if (!m_syncing) {
            markAsChanged();
        }
    }

    bool m_syncing = false;

    QSpinBox *m_durationMs = nullptr;
    QSpinBox *m_openDeferMs = nullptr;
    QCheckBox *m_suppressGlass = nullptr;

    QDoubleSpinBox *m_warp = nullptr;
    QDoubleSpinBox *m_glow = nullptr;
    QDoubleSpinBox *m_aberration = nullptr;
    QDoubleSpinBox *m_spin = nullptr;
    QDoubleSpinBox *m_shrink = nullptr;
    QDoubleSpinBox *m_outline = nullptr;
    QDoubleSpinBox *m_closeScaleTo = nullptr;
    QDoubleSpinBox *m_closeOpacityTo = nullptr;

    QPushButton *m_accretionColorButton = nullptr;
    QPushButton *m_ringColorButton = nullptr;
    QColor m_accretionColor = defaultAccretionColor();
    QColor m_ringColor = defaultRingColor();
};

K_PLUGIN_CLASS(BlackholeSingularityConfig)

#include "blackholesingularityconfig.moc"
