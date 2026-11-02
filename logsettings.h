#ifndef LOGSETTINGS_H
#define LOGSETTINGS_H

#include <QSettings>
#include "settings_defaults.h"

inline QStringList migrateLogColumns(QSettings &settings)
{
    QStringList columns = settings.value(CFG_LOG_COLUMNS, DEFAULT_LOG_COLUMNS).toStringList();
    if (!settings.value(CFG_LOG_SOURCE_MIGRATED, false).toBool()) {
        if (!columns.contains(QStringLiteral("Source")))
            columns.append(QStringLiteral("Source"));
        settings.setValue(CFG_LOG_SOURCE_MIGRATED, true);
        settings.setValue(CFG_LOG_COLUMNS, columns);
    }
    if (settings.value(CFG_LOG_COLUMNS_VERSION, 0).toInt() < CURRENT_LOG_COLUMNS_VERSION) {
        if (!columns.contains(QStringLiteral("Platform")))
            columns.prepend(QStringLiteral("Platform"));
        settings.setValue(CFG_LOG_COLUMNS, columns);
        settings.setValue(CFG_LOG_COLUMNS_VERSION, CURRENT_LOG_COLUMNS_VERSION);
    }
    return columns;
}

#endif
