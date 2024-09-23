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

#ifndef EMOJIMAPPER_H
#define EMOJIMAPPER_H

#include <QMap>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QDebug>

class EmojiMapper {
public:
    EmojiMapper();

    void loadEmojis(const QString& filePath);

    QPair<QString, QString> findBestMatch(const QString& text) const;

private:
    QMap<QString, QString> emojiMap;
    int maxLen;

    void addEmoji(const QJsonObject& obj);
};


#endif // EMOJIMAPPER_H
