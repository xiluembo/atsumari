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

#include "twitchchatreader.h"

#include <QRegularExpression>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QUrl>
#include <QStringList>

#include "settings_defaults.h"
#include "twitchlogmodel.h"

TwitchChatReader::TwitchChatReader(const QString &url, const QString &token, const QString &channel, QObject *parent)
    : QObject(parent), m_webSocket(new QWebSocket), m_channel(channel)
{

    // Ensure the websocket is deleted with this reader to avoid leaks
    m_webSocket->setParent(this);

    connect(m_webSocket, &QWebSocket::connected, this, &TwitchChatReader::onConnected);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, [=](QAbstractSocket::SocketError error) { qDebug() << "error:: " << error;});
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TwitchChatReader::onTextMessageReceived);

    connect(m_webSocket, &QWebSocket::disconnected, this, [=] {
        m_webSocket->open(QUrl(url + "?oauth_token=" + token));
        qDebug() << QUrl(url + "?oauth_token=" + token);
    });

    m_token = token;

    m_webSocket->open(QUrl(url + "?oauth_token=" + token));

    startPingTimer();
}

TwitchChatReader::~TwitchChatReader()
{
    m_webSocket->close();
}

void TwitchChatReader::onConnected()
{
    qDebug() << "Sending Commands";
    // Send required IRC commands
    m_webSocket->sendTextMessage(QStringLiteral("PASS oauth:") + m_token);
    TwitchLogModel::instance()->addEntry("PASS", m_channel, QStringLiteral("oauth:***"), QString());
    m_webSocket->sendTextMessage(QStringLiteral("CAP REQ :twitch.tv/commands twitch.tv/tags twitch.tv/membership"));
    TwitchLogModel::instance()->addEntry("CAP", m_channel, QStringLiteral("twitch.tv/commands twitch.tv/tags twitch.tv/membership"), QString());
    m_webSocket->sendTextMessage(QStringLiteral("NICK ") + m_channel);
    TwitchLogModel::instance()->addEntry("NICK", m_channel, m_channel, QString());
    m_webSocket->sendTextMessage(QStringLiteral("JOIN #") + m_channel);
    TwitchLogModel::instance()->addEntry("JOIN", m_channel, QStringLiteral("#") + m_channel, QString());
}

void TwitchChatReader::onTextMessageReceived(const QString &allMsgs)
{
    for (const QString& message: allMsgs.split("\n")) {
        if (message.trimmed().isEmpty())
            continue;

        QString msg = message;
        QString tags;
        QString prefix;
        if (msg.startsWith("@")) {
            int sp = msg.indexOf(' ');
            tags = msg.left(sp);
            msg = msg.mid(sp + 1);
        }
        if (msg.startsWith(":")) {
            int sp = msg.indexOf(' ');
            prefix = msg.mid(1, sp - 1);
            msg = msg.mid(sp + 1);
        }
        int sp = msg.indexOf(' ');
        QString command = sp == -1 ? msg : msg.left(sp);
        QString params = sp == -1 ? QString() : msg.mid(sp + 1);
        QString trailing;
        int colon = params.indexOf(" :");
        if (colon != -1) {
            trailing = params.mid(colon + 2);
            params = params.left(colon);
        }

        QString sender = prefix.section('!', 0, 0);

        if (command == "PING") {
            QString response = message;
            response.replace("PING", "PONG");
            m_webSocket->sendTextMessage(response);
            TwitchLogModel::instance()->addEntry("PING", sender, trailing, tags);
            TwitchLogModel::instance()->addEntry("PONG", m_channel, trailing, QString());
            continue;
        }

        if (command != "PRIVMSG") {
            TwitchLogModel::instance()->addEntry(command, sender, trailing, tags);
            continue;
        }

        QString metadata = tags;
        QString content = trailing;

        static QRegularExpression displayNameRegex("display.name=([^;\\s]+)");
        QRegularExpressionMatch nameMatch = displayNameRegex.match(metadata);
        if (nameMatch.hasMatch()) {
            QSettings settings;
            QStringList exceptNames = settings.value(CFG_EXCLUDE_CHAT).toStringList();
            QString nameData = nameMatch.captured(1).toLower();
            if (exceptNames.contains(nameData)) {
                continue;
            }
            sender = nameMatch.captured(1);
        }

        bool isGigantifiedEmoteMessage = metadata.contains("msg-id=gigantified-emote-message");

        static QRegularExpression emoteRegex("emotes=([^;\\s]+)");
        QRegularExpressionMatch match = emoteRegex.match(metadata);

        QList<QPixmap> emotePixmaps;
        QStringList missingEmotes;
        if (match.hasMatch()) {
            QString emotesData = match.captured(1);
            QStringList emoteList = emotesData.split('/');

            QString lastEmoteId;
            QString lastEmoteName;
            int lastEmoteEndPos = -1;

            QMap<QString, QString> processedEmotes;
            for (const QString& emote : emoteList) {
                QStringList parts = emote.split(':');
                if (parts.length() < 2) continue;

                QString emoteId = parts[0];
                QString position = parts[1].split(',')[parts[1].split(',').length() - 1];

                QStringList range = position.split('-');
                if (range.length() != 2) continue;

                int start = range[0].toInt();
                int end = range[1].toInt() + 1;
                QString emoteName = content.mid(start, end - start);

                if (isGigantifiedEmoteMessage && end > lastEmoteEndPos) {
                    lastEmoteId = emoteId;
                    lastEmoteName = emoteName;
                    lastEmoteEndPos = end;
                }

                processedEmotes[emoteId] = emoteName.trimmed();

                QSettings settings;
                QString dir = settings.value(CFG_EMOTE_DIR, DEFAULT_EMOTE_DIR).toString();
                QString path = QString("%1/%2.png").arg(dir, emoteId);
                if (QFile::exists(path)) {
                    emotePixmaps.append(QPixmap(path));
                } else {
                    missingEmotes.append(emoteId);
                }
            }

            if (isGigantifiedEmoteMessage && !lastEmoteId.isEmpty()) {
                emit bigEmoteSent(lastEmoteId, lastEmoteName.trimmed());
                processedEmotes.remove(lastEmoteId);
            }

            for (const QString& emoteId: processedEmotes.keys()) {
                emit emoteSent(emoteId, processedEmotes[emoteId]);
            }
        }

        QMap<QString, QString> emojisFound;
        while (!content.isEmpty()) {
            QPair<QString, QString> emojidata = m_emojiMapper.findBestMatch(content);
            QString emojiStr = emojidata.first;
            QString emojiSlug = emojidata.second;
            if (!emojiStr.isEmpty()) {
                emojisFound[emojiSlug] = emojiStr;
                content.remove(0, emojiStr.length());
            } else {
                content.remove(0, 1);
            }
        }

        for (const QString& slug: emojisFound.keys()) {
            emit emojiSent(slug, emojisFound[slug]);

            QSettings settings;
            QString dir = settings.value(CFG_EMOJI_DIR, DEFAULT_EMOJI_DIR).toString();
            QString path = QString("%1/%2.png").arg(dir, slug);
            if (QFile::exists(path)) {
                emotePixmaps.append(QPixmap(path));
            } else {
                missingEmotes.append(slug);
            }
        }

        TwitchLogModel::instance()->addEntry(command, sender, trailing, metadata, QList<QPixmap>(), emotePixmaps, missingEmotes);
    }
}

void TwitchChatReader::startPingTimer()
{
    QTimer* pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, [=]() {
        m_webSocket->sendTextMessage("PING");
    });
    pingTimer->start(180000); // Send a PING every 3 minutes
}
