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

TwitchChatReader::TwitchChatReader(const QString &url, const QString &token, const QString &channel, QObject *parent)
    : QObject(parent), m_webSocket(new QWebSocket), m_channel(channel) {

    connect(m_webSocket, &QWebSocket::connected, this, &TwitchChatReader::onConnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TwitchChatReader::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::pong, this, &TwitchChatReader::onPongReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &TwitchChatReader::onErrorOccurred);

    qDebug() << "CHATREADER " << url << " // " << token << " // " << channel;
    connect(m_webSocket, &QWebSocket::disconnected, this, [=] {
        qDebug() << "RECONNECTING...";
        m_webSocket->open(QUrl(url + "?oauth_token=" + token));
    });

    m_token = token;

    m_webSocket->open(QUrl(url + "?oauth_token=" + token));

    startPingTimer();
}

TwitchChatReader::~TwitchChatReader() {
    m_webSocket->close();
}

void TwitchChatReader::onConnected() {
    qDebug() << "CONNECTED!!!";
    // Send required IRC commands
    m_webSocket->sendTextMessage(QStringLiteral("PASS oauth:") + m_token);
    m_webSocket->sendTextMessage(QStringLiteral("CAP REQ :twitch.tv/commands twitch.tv/tags twitch.tv/membership"));
    m_webSocket->sendTextMessage(QStringLiteral("NICK ") + m_channel);
    m_webSocket->sendTextMessage(QStringLiteral("JOIN #") + m_channel);
}

void TwitchChatReader::onTextMessageReceived(const QString &allMsgs) {    
    for(const QString& message: allMsgs.split("\n")) {
        qDebug() << "Message received:" << message;

        if (message.startsWith("PING")) {
            // Send an equivalent PONG
            QString response = message;
            response.replace("PING", "PONG");
            qDebug() << "PONG sent!";
            m_webSocket->sendTextMessage(response);
            return;  // Don't further process the message
        }

        // Extract metadata from the message
        int privmsgIndex = message.indexOf(" PRIVMSG #");
        if (privmsgIndex == -1) {
            // It's just their pong, don't process further
            if (message.contains("PONG")) {
                qDebug() << "PONG received";
            }
            return;
        }

        QString metadata = message.left(privmsgIndex);
        QString content = message.mid(message.indexOf(':', privmsgIndex) + 1);

        // Find sender
        static QRegularExpression displayNameRegex("display.name=([^;\\s]+)");
        QRegularExpressionMatch nameMatch = displayNameRegex.match(metadata);
        if (nameMatch.hasMatch()) {
            QStringList exceptNames;
            exceptNames << "araxbird";
            exceptNames << "notrotom";

            QString nameData = nameMatch.captured(1).toLower();
            if(exceptNames.contains(nameData)) {
                return;
            }
        }

        // Check for gigantified emote message
        bool isGigantifiedEmoteMessage = metadata.contains("msg-id=gigantified-emote-message");

        // Find emotes in the metadata
        static QRegularExpression emoteRegex("emotes=([^;\\s]+)");
        QRegularExpressionMatch match = emoteRegex.match(metadata);

        qDebug() << "is gigantic :" << isGigantifiedEmoteMessage;

        if (match.hasMatch()) {
            QString emotesData = match.captured(1);
            QStringList emoteList = emotesData.split('/');

            QString lastEmoteId;
            QString lastEmoteName;
            int lastEmoteEndPos = -1;

            // Processar cada emote
            QMap<QString, QString> processedEmotes;
            for (const QString& emote : emoteList) {
                QStringList parts = emote.split(':');
                if (parts.length() < 2) continue;

                QString emoteId = parts[0];
                QString position = parts[1].split(',')[parts[1].split(',').length() - 1];

                QStringList range = position.split('-');
                if (range.length() != 2) continue;

                int start = range[0].toInt();
                int end = range[1].toInt() + 1;  // Adjust to QString ranges
                QString emoteName = content.mid(start, end - start);

                if (isGigantifiedEmoteMessage && end > lastEmoteEndPos) {
                    lastEmoteId = emoteId;
                    lastEmoteName = emoteName;
                    lastEmoteEndPos = end;
                }

                //emit emoteSent(emoteId, emoteName.trimmed()); // Emit signal with ID and emote name
                processedEmotes[emoteId] = emoteName.trimmed();

                qDebug() << "EMOTEINFO:: " << emoteId << " :: " << emoteName.trimmed();
            }

            // Emit bigEmoteSent for the last emote if it's a gigantified emote message
            if (isGigantifiedEmoteMessage && !lastEmoteId.isEmpty()) {
                emit bigEmoteSent(lastEmoteId, lastEmoteName.trimmed());
                qDebug() << "BIG EMOTE SENT:: " << lastEmoteId << " :: " << lastEmoteName.trimmed();
                processedEmotes.remove(lastEmoteId);
            }

            for(const QString& emoteId: processedEmotes.keys()) {
                emit emoteSent(emoteId, processedEmotes[emoteId]);
            }
        }

        // Identify emoji in the contents
        QMap<QString, QString> emojisFound;
        qDebug() << "content:: " << content;
        while (!content.isEmpty()) {
            QPair<QString, QString> emojidata = m_emojiMapper.findBestMatch(content);
            QString emojiStr = emojidata.first;
            QString emojiSlug = emojidata.second;
            if (!emojiStr.isEmpty()) {
                emojisFound[emojiSlug] = emojiStr;
                qDebug() << "content:: " << content;
                qDebug() << "Emoji found:" << emojiSlug;
                // Remove the matched part from the content
                content.remove(0, emojiStr.length());
            } else {
                // Remove the first character and try again
                content.remove(0, 1);
            }
        }

        for(const QString& slug: emojisFound.keys()) {
            emit emojiSent(slug, emojisFound[slug]);
        }
    }
}

void TwitchChatReader::startPingTimer() {
    QTimer* pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, [=]() {
        m_webSocket->sendTextMessage("PING");
    });
    pingTimer->start(180000); // Send a PING every 3 minutes
}

void TwitchChatReader::onPongReceived(quint64 elapsedTime, const QByteArray& payload) {
    qDebug() << "Pong received, latency:" << elapsedTime << "ms, payload:" << payload;
}

void TwitchChatReader::onErrorOccurred(QAbstractSocket::SocketError error) {
    qDebug() << "WebSocket error:" << m_webSocket->errorString();
}
