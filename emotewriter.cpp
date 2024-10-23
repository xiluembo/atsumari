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
#include <QDir>
#include <QApplication>
#include <QFontDatabase>
#include <QSettings>

#include "settings_defaults.h"

EmoteWriter::EmoteWriter(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this))
{
    QSettings settings;

    QDir dir;
    dir.mkpath(settings.value(CFG_EMOTE_DIR, DEFAULT_EMOTE_DIR).toString());
    dir.mkpath(settings.value(CFG_EMOJI_DIR, DEFAULT_EMOJI_DIR).toString());

    connect(networkManager, &QNetworkAccessManager::finished, this, &EmoteWriter::handleNetworkReply);
}

void EmoteWriter::saveEmote(const QString &id)
{
    QSettings settings;

    if (m_emotes.contains(id)) {
        emit emoteWritten(m_emotes[id]);
    } else {
        QUrl url(QString("https://static-cdn.jtvnw.net/emoticons/v2/%1/static/dark/3.0").arg(id));
        QNetworkRequest request(url);
        networkManager->get(request);

        QDir emotePath(QString("%1/%2.png").arg(settings.value(CFG_EMOTE_DIR, DEFAULT_EMOTE_DIR).toString(), id));

        pendingEmotes.insert(id, emotePath.absolutePath());
    }
}

void EmoteWriter::saveBigEmote(const QString &id)
{
    QSettings settings;

    if (m_emotes.contains(id)) {
        emit bigEmoteWritten(m_emotes[id]);
    } else {
        QUrl url(QString("https://static-cdn.jtvnw.net/emoticons/v2/%1/static/dark/3.0").arg(id));
        QNetworkRequest request(url);
        networkManager->get(request);

        QDir emotePath(QString("%1/big_Emote_%2.png").arg(settings.value(CFG_EMOTE_DIR, DEFAULT_EMOTE_DIR).toString(), id));

        pendingEmotes.insert(id, emotePath.absolutePath());
    }
}

void EmoteWriter::saveEmoji(const QString &slug, const QString& emojiData)
{
    QSettings settings;
    // Check if the emoji is already saved
    QString filePath = QString("%1/%2.png").arg(settings.value(CFG_EMOJI_DIR, DEFAULT_EMOJI_DIR).toString(), slug);
    if (m_emotes.contains(slug)) {
        emit emoteWritten(m_emotes[slug]);
        return;
    }

    // Setting up the pixmap where the emoji is going to be rendered
    int imageSize = 112;
    QPixmap pixmap(imageSize, imageSize);
    pixmap.fill(Qt::transparent); // Transparent background

    // Setting up the QPainter
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font(settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString(), 72); // Set up a big font for the emoji
    painter.setFont(font);

    // Center the emoji into the image
    QFontMetrics metrics(font);
    int textWidth = metrics.horizontalAdvance(emojiData);
    int x = (imageSize - textWidth) / 2;
    int y = (imageSize + metrics.ascent() - metrics.descent()) / 2;
    painter.drawText(x, y, emojiData);

    // Save the image file
    if (pixmap.save(filePath, "PNG")) {
        m_emotes[slug] = filePath; // Update emotes map
        emit emoteWritten(filePath); // Emit signal
    }

    painter.end();
}


void EmoteWriter::handleNetworkReply(QNetworkReply *reply)
{
    if (reply && reply->error() == QNetworkReply::NoError) {
        QString id = reply->url().toString().split('/').at(5); // Extracting the ID from URL
        QString filePath = pendingEmotes.value(id);
        if (filePath.isEmpty()) {
            reply->deleteLater();
            return;
        }

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            m_emotes.insert(id, filePath);
            if (filePath.contains("/big_Emote_")) {
                emit bigEmoteWritten(filePath);
            } else {
                emit emoteWritten(filePath);
            }
            pendingEmotes.remove(id);
        }
        reply->deleteLater();
    }
}
