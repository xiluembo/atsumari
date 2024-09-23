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

#ifndef TWITCHCHATREADER_H
#define TWITCHCHATREADER_H

#include <QCoreApplication>
#include <QtWebSockets/QWebSocket>
#include <QtCore/QUrl>

#include "emojimapper.h"

class TwitchChatReader : public QObject {
    Q_OBJECT
public:
    TwitchChatReader(const QString& url, const QString& token, const QString& channel, QObject* parent = nullptr);

    ~TwitchChatReader();

    void startPingTimer();

public slots:
    void onPongReceived(quint64 elapsedTime, const QByteArray &payload);
    void onErrorOccurred(QAbstractSocket::SocketError error);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& allMsgs);

signals:
    void emoteSent(QString emoteId, QString emoteName);
    void bigEmoteSent(QString emoteId, QString emoteName);
    void emojiSent(QString slug, QString emojiData);

private:
    QWebSocket* m_webSocket;
    QString m_token;
    QString m_channel;

    EmojiMapper m_emojiMapper;
};

#endif // TWITCHCHATREADER_H
