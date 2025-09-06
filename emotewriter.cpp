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

#include "emotewriter.h"

#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QDebug>

#include "settings_defaults.h"

EmoteWriter::EmoteWriter(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
    connect(networkManager, &QNetworkAccessManager::finished, this, &EmoteWriter::handleNetworkReply);
}

void EmoteWriter::saveEmote(const QString &id)
{
    if (m_textures.contains(id)) {
        emit emoteReady(id, m_textures.value(id), m_pixmaps.value(id));
        return;
    }

    QUrl url(QString("https://static-cdn.jtvnw.net/emoticons/v2/%1/static/dark/3.0").arg(id));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("emoteId", id);
    reply->setProperty("isBig", false);
}

void EmoteWriter::saveBigEmote(const QString &id)
{
    QString key = QStringLiteral("big_") + id;
    if (m_textures.contains(key)) {
        emit bigEmoteReady(id, m_textures.value(key), m_pixmaps.value(key));
        return;
    }

    QUrl url(QString("https://static-cdn.jtvnw.net/emoticons/v2/%1/static/dark/3.0").arg(id));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    reply->setProperty("emoteId", id);
    reply->setProperty("isBig", true);
}

void EmoteWriter::saveEmoji(const QString &slug, const QString &emojiData, const QString &fontName)
{
    if (m_textures.contains(slug)) {
        emit emoteReady(slug, m_textures.value(slug), m_pixmaps.value(slug));
        return;
    }

    int imageSize = 112;
    QPixmap pixmap(imageSize, imageSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QString family = fontName.isEmpty() ? QStringLiteral(DEFAULT_EMOJI_FONT) : fontName;
    QFont font(family, 72);
    painter.setFont(font);

    QFontMetrics metrics(font);
    int textWidth = metrics.horizontalAdvance(emojiData);
    int x = (imageSize - textWidth) / 2;
    int y = (imageSize + metrics.ascent() - metrics.descent()) / 2;
    painter.drawText(x, y, emojiData);
    painter.end();

    QImage image = pixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
    auto *texData = new QQuick3DTextureData();
    texData->setParent(this);
    texData->setSize(image.size());
    texData->setFormat(QQuick3DTextureData::RGBA8);
    texData->setTextureData(QByteArray(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes()));

    m_textures.insert(slug, texData);
    m_pixmaps.insert(slug, pixmap);
    emit emoteReady(slug, texData, pixmap);
}


void EmoteWriter::handleNetworkReply(QNetworkReply *reply)
{
    if (!reply || reply->error() != QNetworkReply::NoError) {
        if (reply)
            reply->deleteLater();
        return;
    }

    QString id = reply->property("emoteId").toString();
    bool isBig = reply->property("isBig").toBool();

    QByteArray data = reply->readAll();
    QImage image;
    if (!image.loadFromData(data)) {
        qWarning() << "Failed to load emote image for id" << id;
        reply->deleteLater();
        return;
    }
    QPixmap pixmap = QPixmap::fromImage(image);

    QImage img = pixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
    auto *texData = new QQuick3DTextureData();
    texData->setParent(this);
    texData->setSize(img.size());
    texData->setFormat(QQuick3DTextureData::RGBA8);
    texData->setTextureData(QByteArray(reinterpret_cast<const char*>(img.constBits()), img.sizeInBytes()));

    QString key = isBig ? QStringLiteral("big_") + id : id;
    m_textures.insert(key, texData);
    m_pixmaps.insert(key, pixmap);

    if (isBig)
        emit bigEmoteReady(id, texData, pixmap);
    else
        emit emoteReady(id, texData, pixmap);

    reply->deleteLater();
}

QPixmap EmoteWriter::pixmapFor(const QString &id) const
{
    return m_pixmaps.value(id);
}
