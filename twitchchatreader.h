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
#include <QDateTime>
#include <QHash>
#include <QMap>
#include <QNetworkAccessManager>
#include <QSet>
#include <QTimer>
#include <QtCore/QUrl>
#include <QtWebSockets/QWebSocket>

#include "emojimapper.h"

class EmoteWriter;

class TwitchChatReader : public QObject {
    Q_OBJECT
public:
    TwitchChatReader(const QString& ircUrl,
                     const QString& token,
                     const QString& channel,
                     const QString& broadcasterId,
                     const QString& userId,
                     EmoteWriter* emoteWriter,
                     QObject* parent = nullptr);
    ~TwitchChatReader();

signals:
    void connected();
    void emoteSent(QString emoteId, QString emoteName);
    void bigEmoteSent(QString emoteId, QString emoteName);
    void emojiSent(QString slug, QString emojiData);

public:
    void setAccessToken(const QString &token);

private:
    struct ChatEvent {
        QString messageId;
        QString sender;
        QString trailing;
        QString metadata;
        bool fromIrc = false;
        bool fromEventSub = false;
        bool processed = false;
        bool gigantified = false;
        QMap<QString, QString> ircEmotes;
        QMap<QString, QString> eventSubEmotes;
        QMap<QString, QString> emojis;
        QMap<QString, QString> cheerEmotes;
    };

    struct CheermoteTier {
        int minBits = 0;
        QString darkStatic3x;
    };

    void onIrcConnected();
    void onIrcTextMessageReceived(const QString& allMsgs);
    void startPingTimer();

    void setupEventSub();
    void onEventSubConnected();
    void onEventSubTextMessageReceived(const QString& payload);
    void createChannelChatMessageSubscription();
    void fetchCheermotes();
    QString cheerEmoteUrlFor(const QString &prefix, int bits) const;

    void mergeIrcPrivmsg(const QString& sender,
                         const QString& tags,
                         const QString& trailing,
                         const QString& metadata);
    void mergeEventSubMessage(const QString& messageId,
                              const QString& sender,
                              const QString& trailing,
                              const QMap<QString, QString>& emotes,
                              const QMap<QString, QString>& cheermotes);
    void scheduleFinalize(const QString& messageId);
    void finalizePending(const QString& messageId);
    void processEvent(ChatEvent& event);
    bool shouldSkipSender(const QString& sender) const;

    QWebSocket* m_webSocket;
    QWebSocket* m_eventSubSocket;
    QString m_token;
    QString m_channel;
    QString m_broadcasterId;
    QString m_userId;
    QString m_eventSubSessionId;
    QString m_eventSubSubscriptionId;
    QTimer* m_pingTimer = nullptr;
    int m_reconnectAttempts = 0;
    bool m_waitingForTokenRefresh = false;
    bool m_eventSubReconnectScheduled = false;
    bool m_eventSubSubscriptionInFlight = false;

    EmojiMapper m_emojiMapper;
    EmoteWriter* m_emoteWriter;
    QNetworkAccessManager m_network;

    QHash<QString, ChatEvent> m_pendingEvents;
    QHash<QString, QTimer*> m_finalizeTimers;
    int m_fallbackCounter = 0;

    QHash<QString, QList<CheermoteTier>> m_cheermotes;
    QSet<QString> m_requestedCheerKeys;
    QHash<QString, QPair<QString, int>> m_pendingCheerLookups;
};

#endif // TWITCHCHATREADER_H
