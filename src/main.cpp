#include "blackholesingularityeffect.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED(BlackholeSingularityEffect,
                                      "plugin.json",
                                      return BlackholeSingularityEffect::supported();,
                                      return BlackholeSingularityEffect::enabledByDefault();)

} // namespace KWin

#include "main.moc"
