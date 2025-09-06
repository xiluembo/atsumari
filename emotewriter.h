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

#ifndef EMOTEWRITER_H
#define EMOTEWRITER_H

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QUrl>
#include <QQuick3DTextureData>

class EmoteWriter : public QObject {
    Q_OBJECT

public:
    explicit EmoteWriter(QObject *parent = nullptr);
    void saveEmote(const QString &id);
    void saveBigEmote(const QString &id);
    void saveEmoji(const QString &slug, const QString &emojiData, const QString &fontName);
    QPixmap pixmapFor(const QString &id) const;

signals:
    void emoteReady(const QString &id, QQuick3DTextureData *textureData, const QPixmap &pixmap);
    void bigEmoteReady(const QString &id, QQuick3DTextureData *textureData, const QPixmap &pixmap);

private:
    void handleNetworkReply(QNetworkReply *reply);

    QNetworkAccessManager *networkManager;
    QMap<QString, QQuick3DTextureData*> m_textures;
    QMap<QString, QPixmap> m_pixmaps;
};


#endif // EMOTEWRITER_H
