#pragma once

#include <QColor>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>
#include <QString>
#include <utility>

inline std::pair<QColor, QColor> defaultCommandColors(const QString &cmd)
{
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark) {
        if (cmd == QLatin1String("PRIVMSG"))
            return {QColor("#c0c0ff"), QColor("#000070")};
        if (cmd == QLatin1String("JOIN"))
            return {QColor("#ffffc0"), QColor("#707000")};
        if (cmd == QLatin1String("PART"))
            return {QColor("#dbc0ff"), QColor("#300070")};
    } else {
        if (cmd == QLatin1String("PRIVMSG"))
            return {QColor("#000070"), QColor("#c0c0ff")};
        if (cmd == QLatin1String("JOIN"))
            return {QColor("#707000"), QColor("#ffffc0")};
        if (cmd == QLatin1String("PART"))
            return {QColor("#300070"), QColor("#dbc0ff")};
    }
    const auto pal = QGuiApplication::palette();
    return {pal.color(QPalette::Text), pal.color(QPalette::Base)};
}

