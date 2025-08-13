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

#include "emojimapper.h"

EmojiMapper::EmojiMapper(const QString& emojiFilePath)
    : maxLen(0)
{
    loadEmojis(emojiFilePath);
}

void EmojiMapper::loadEmojis(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Cannot open emoji file.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray emojiArray = doc.array();

    for (const QJsonValue &value : emojiArray) {
        QJsonObject obj = value.toObject();
        addEmoji(obj);

        QJsonArray variants = obj["variants"].toArray();
        for (const QJsonValue &variant : variants) {
            addEmoji(variant.toObject());
        }
    }
}

QPair<QString, QString> EmojiMapper::findBestMatch(const QString &text) const
{
    QString bestMatchStr = "";
    QString bestMatchSlug = "";

    for (int i = maxLen ; i > 0 ; --i) {
        if (i > text.length()) {
            continue;
        }

        if (emojiMap.contains(text.left(i))) {
            bestMatchStr = text.left(i);
            bestMatchSlug = emojiMap[bestMatchStr];
            break;
        }
    }

    return qMakePair(bestMatchStr, bestMatchSlug);
}

void EmojiMapper::addEmoji(const QJsonObject &obj)
{
    QString character = obj["character"].toString();
    QString slug = obj["slug"].toString();
    emojiMap[character] = slug;

    maxLen = (maxLen > character.length()) ? maxLen : character.length();
}
