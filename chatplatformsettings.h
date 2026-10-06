#ifndef CHATPLATFORMSETTINGS_H
#define CHATPLATFORMSETTINGS_H

#include <QSettings>
#include <QStringList>
#include "settings_defaults.h"

inline QStringList enabledChatPlatforms(const QSettings &settings)
{
    // An explicitly empty list means that all platforms are disabled.
    if (settings.contains(CFG_CHAT_PLATFORMS))
        return settings.value(CFG_CHAT_PLATFORMS).toStringList();

    // Compatibility with the initial Gamerfy branch's single-choice setting.
    const QString legacy = settings.value(CFG_CHAT_PROVIDER, "twitch").toString();
    if (legacy == "both")
        return {QStringLiteral("twitch"), QStringLiteral("gamerfy")};
    if (legacy == "gamerfy")
        return {QStringLiteral("gamerfy")};
    return {QStringLiteral("twitch")};
}

#endif
