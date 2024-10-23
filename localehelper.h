/*
 * Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
 * Copyright (C) 2024 - Andrius da Costa Ribas <andriusmao@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOCALEHELPER_H
#define LOCALEHELPER_H

#include <QLocale>
#include <QTranslator>

class LocaleHelper {
public:
    static void loadBestTranslation(const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static void loadTranslation(const QLocale &locale, const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static QLocale findBestLocale(const QLocale &locale = QLocale::system(), const QString &baseName = "atsumari", const QString &directory = ":/i18n");
    static QList<QLocale> availableLocales(const QString &baseName = "atsumari", const QString &directory = ":/i18n");

private:
    static QTranslator translator;
};

#endif // LOCALEHELPER_H
