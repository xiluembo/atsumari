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

#include <algorithm>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>
#include <QUrlQuery>

#include "emotewriter.h"
#include "settings_defaults.h"
#include "twitchlogmodel.h"

namespace {

void prepareTwitchRequest(QNetworkRequest &request)
{
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
}

}

TwitchChatReader::TwitchChatReader(const QString &ircUrl,
                                   const QString &token,
                                   const QString &channel,
                                   const QString &broadcasterId,
                                   const QString &userId,
                                   EmoteWriter *emoteWriter,
                                   QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket)
    , m_eventSubSocket(new QWebSocket)
    , m_token(token)
    , m_channel(channel)
    , m_broadcasterId(broadcasterId)
    , m_userId(userId)
    , m_emoteWriter(emoteWriter)
{
    m_webSocket->setParent(this);
    m_eventSubSocket->setParent(this);
    m_ircReconnectTimer = new QTimer(this);
    m_ircReconnectTimer->setSingleShot(true);
    connect(m_ircReconnectTimer, &QTimer::timeout, this, [this, ircUrl]() {
        if (m_webSocket->state() == QAbstractSocket::UnconnectedState)
            m_webSocket->open(QUrl(ircUrl + "?oauth_token=" + m_token));
    });

    connect(m_webSocket, &QWebSocket::connected, this, &TwitchChatReader::onIrcConnected);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, [=](QAbstractSocket::SocketError error) { qDebug() << "IRC error:" << error; });
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TwitchChatReader::onIrcTextMessageReceived);

    connect(m_webSocket, &QWebSocket::disconnected, this, [=] {
        ++m_reconnectAttempts;
        int delay = qMin(30000, 1000 * (1 << (m_reconnectAttempts - 1)));
        m_ircReconnectTimer->start(delay);
    });

    connect(m_eventSubSocket, &QWebSocket::connected, this, &TwitchChatReader::onEventSubConnected);
    connect(m_eventSubSocket, &QWebSocket::textMessageReceived, this, &TwitchChatReader::onEventSubTextMessageReceived);
    connect(m_eventSubSocket, &QWebSocket::errorOccurred, this, [=](QAbstractSocket::SocketError error) { qDebug() << "EventSub error:" << error; });
    connect(m_eventSubSocket, &QWebSocket::disconnected, this, [=]() {
        m_eventSubSessionId.clear();
        m_eventSubSubscriptionId.clear();
        m_eventSubSubscriptionInFlight = false;
        if (m_waitingForTokenRefresh || m_eventSubReconnectScheduled)
            return;
        m_eventSubReconnectScheduled = true;
        QTimer::singleShot(2000, this, [this]() {
            m_eventSubReconnectScheduled = false;
            if (!m_waitingForTokenRefresh && m_eventSubSocket->state() == QAbstractSocket::UnconnectedState)
                setupEventSub();
        });
    });

    m_webSocket->open(QUrl(ircUrl + "?oauth_token=" + m_token));
    setupEventSub();
    startPingTimer();
}


void TwitchChatReader::setAccessToken(const QString &token)
{
    if (token.isEmpty() || token == m_token)
        return;

    m_token = token;
    m_waitingForTokenRefresh = false;
    m_eventSubSubscriptionInFlight = false;

    if (m_eventSubSocket->state() != QAbstractSocket::ConnectedState)
        setupEventSub();
    else if (!m_eventSubSessionId.isEmpty())
        createChannelChatMessageSubscription();
}

TwitchChatReader::~TwitchChatReader()
{
    m_webSocket->close();
    m_eventSubSocket->close();

    if (m_pingTimer) {
        m_pingTimer->stop();
        m_pingTimer->deleteLater();
    }

    for (QTimer *timer : m_finalizeTimers)
        timer->deleteLater();
}

void TwitchChatReader::onIrcConnected()
{
    m_ircReconnectTimer->stop();
    m_reconnectAttempts = 0;
    emit connected();

    m_webSocket->sendTextMessage(QStringLiteral("PASS oauth:") + m_token);
    TwitchLogModel::instance()->addEntry(TwitchLogModel::Sent, "PASS", m_channel, QStringLiteral("oauth:***"), QString());
    m_webSocket->sendTextMessage(QStringLiteral("CAP REQ :twitch.tv/commands twitch.tv/tags twitch.tv/membership"));
    TwitchLogModel::instance()->addEntry(TwitchLogModel::Sent, "CAP", m_channel, QStringLiteral("twitch.tv/commands twitch.tv/tags twitch.tv/membership"), QString());
    m_webSocket->sendTextMessage(QStringLiteral("NICK ") + m_channel);
    TwitchLogModel::instance()->addEntry(TwitchLogModel::Sent, "NICK", m_channel, m_channel, QString());
    m_webSocket->sendTextMessage(QStringLiteral("JOIN #") + m_channel);
    TwitchLogModel::instance()->addEntry(TwitchLogModel::Sent, "JOIN", m_channel, QStringLiteral("#") + m_channel, QString());
}

void TwitchChatReader::onIrcTextMessageReceived(const QString &allMsgs)
{
    for (const QString& message: allMsgs.split("\n")) {
        if (message.trimmed().isEmpty())
            continue;

        QString msg = message.trimmed();
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
            TwitchLogModel::instance()->addEntry(TwitchLogModel::Received, "PING", sender, trailing, tags);
            TwitchLogModel::instance()->addEntry(TwitchLogModel::Sent, "PONG", m_channel, trailing, QString());
            continue;
        }

        if (command != "PRIVMSG") {
            TwitchLogModel::instance()->addEntry(TwitchLogModel::Received, command, sender, trailing, tags);
            continue;
        }

        QString metadata = tags;
        static QRegularExpression displayNameRegex("display-name=([^;\\s]+)");
        QRegularExpressionMatch nameMatch = displayNameRegex.match(metadata);
        if (nameMatch.hasMatch())
            sender = nameMatch.captured(1);

        mergeIrcPrivmsg(sender, tags, trailing, metadata);
    }
}

void TwitchChatReader::setupEventSub()
{
    if (m_waitingForTokenRefresh || m_token.isEmpty())
        return;

    if (m_eventSubSocket->state() != QAbstractSocket::UnconnectedState)
        return;

    m_eventSubSessionId.clear();
    m_eventSubSubscriptionId.clear();
    m_eventSubSubscriptionInFlight = false;
    m_eventSubSocket->open(QUrl(QStringLiteral("wss://eventsub.wss.twitch.tv/ws")));
}

void TwitchChatReader::onEventSubConnected()
{
    qDebug() << "EventSub websocket connected";
}

void TwitchChatReader::onEventSubTextMessageReceived(const QString &payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    QString type = root.value("metadata").toObject().value("message_type").toString();

    if (type == QStringLiteral("session_welcome")) {
        m_eventSubSessionId = root.value("payload").toObject().value("session").toObject().value("id").toString();
        m_eventSubSubscriptionId.clear();
        m_eventSubSubscriptionInFlight = false;
        if (!m_eventSubSessionId.isEmpty())
            createChannelChatMessageSubscription();
        return;
    }

    if (type != QStringLiteral("notification"))
        return;

    QJsonObject eventObj = root.value("payload").toObject().value("event").toObject();
    QString messageId = eventObj.value("message_id").toString();
    QString sender = eventObj.value("chatter_user_name").toString();
    QString text = eventObj.value("message").toObject().value("text").toString();

    QMap<QString, QString> emotes;
    QMap<QString, QString> cheermotes;

    QJsonArray fragments = eventObj.value("message").toObject().value("fragments").toArray();
    for (const QJsonValue &fragmentVal : fragments) {
        QJsonObject fragment = fragmentVal.toObject();
        QString fragmentType = fragment.value("type").toString();
        if (fragmentType == QStringLiteral("emote")) {
            QJsonObject emote = fragment.value("emote").toObject();
            QString emoteId = emote.value("id").toString();
            QString emoteText = fragment.value("text").toString();
            if (!emoteId.isEmpty())
                emotes[emoteId] = emoteText;
        } else if (fragmentType == QStringLiteral("cheermote")) {
            QJsonObject cheer = fragment.value("cheermote").toObject();
            QString prefix = cheer.value("prefix").toString().toLower();
            int bits = cheer.value("bits").toInt();
            if (!prefix.isEmpty() && bits > 0) {
                const QString cheerId = QStringLiteral("cheer_%1_%2").arg(prefix).arg(bits);
                cheermotes[cheerId] = fragment.value("text").toString();
                if (!m_requestedCheerKeys.contains(cheerId)) {
                    const QString cheerUrl = cheerEmoteUrlFor(prefix, bits);
                    if (!cheerUrl.isEmpty()) {
                        m_requestedCheerKeys.insert(cheerId);
                        m_emoteWriter->saveEmoteFromUrl(cheerId, cheerUrl);
                    } else {
                        m_pendingCheerLookups.insert(cheerId, qMakePair(prefix, bits));
                        fetchCheermotes();
                    }
                }
            }
        }
    }

    if (!eventObj.value("cheer").isNull() && m_cheermotes.isEmpty())
        fetchCheermotes();

    mergeEventSubMessage(messageId, sender, text, emotes, cheermotes);
}

void TwitchChatReader::createChannelChatMessageSubscription()
{
    if (m_eventSubSessionId.isEmpty() || m_broadcasterId.isEmpty() || m_userId.isEmpty())
        return;
    if (m_eventSubSubscriptionInFlight || !m_eventSubSubscriptionId.isEmpty())
        return;

    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();

    QUrl requestUrl(QStringLiteral("https://api.twitch.tv/helix/eventsub/subscriptions"));
    QNetworkRequest request(requestUrl);
    prepareTwitchRequest(request);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_token).toUtf8());
    request.setRawHeader("Client-Id", clientId.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject payload {
        {"type", "channel.chat.message"},
        {"version", "1"},
        {"condition", QJsonObject{{"broadcaster_user_id", m_broadcasterId}, {"user_id", m_userId}}},
        {"transport", QJsonObject{{"method", "websocket"}, {"session_id", m_eventSubSessionId}}}
    };

    m_eventSubSubscriptionInFlight = true;
    QNetworkReply *reply = m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_eventSubSubscriptionInFlight = false;
        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qWarning() << "Failed to subscribe to EventSub:" << reply->readAll();
            if (status == 401) {
                m_waitingForTokenRefresh = true;
                m_eventSubSocket->close();
            }
            if (status == 429) {
                m_eventSubSubscriptionId = QStringLiteral("rate-limited");
            }
            reply->deleteLater();
            return;
        }

        const QJsonDocument responseJson = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray data = responseJson.object().value("data").toArray();
        if (!data.isEmpty()) {
            const QString id = data.first().toObject().value("id").toString();
            if (!id.isEmpty())
                m_eventSubSubscriptionId = id;
        }
        reply->deleteLater();
    });

    fetchCheermotes();
}

void TwitchChatReader::fetchCheermotes()
{
    if (m_broadcasterId.isEmpty())
        return;

    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();

    QUrl requestUrl(QStringLiteral("https://api.twitch.tv/helix/bits/cheermotes"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("broadcaster_id"), m_broadcasterId);
    requestUrl.setQuery(query);

    QNetworkRequest request(requestUrl);
    prepareTwitchRequest(request);
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_token).toUtf8());
    request.setRawHeader("Client-Id", clientId.toUtf8());

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 401)
                m_waitingForTokenRefresh = true;
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray data = doc.object().value("data").toArray();
        m_cheermotes.clear();

        for (const QJsonValue &entryValue : data) {
            QJsonObject entry = entryValue.toObject();
            QString prefix = entry.value("prefix").toString().toLower();
            QList<CheermoteTier> tiers;

            QJsonArray tierArray = entry.value("tiers").toArray();
            for (const QJsonValue &tierValue : tierArray) {
                QJsonObject tierObj = tierValue.toObject();
                QJsonObject images = tierObj.value("images").toObject();
                QJsonObject dark = images.value("dark").toObject();
                QJsonObject animated = dark.value("animated").toObject();
                QJsonObject statik = dark.value("static").toObject();

                QString url = animated.value("3").toString();
                if (url.isEmpty())
                    url = statik.value("3").toString();
                if (url.isEmpty())
                    continue;

                CheermoteTier tier;
                tier.minBits = tierObj.value("min_bits").toInt();
                tier.darkStatic3x = url;
                tiers.append(tier);
            }

            std::sort(tiers.begin(), tiers.end(), [](const CheermoteTier &a, const CheermoteTier &b) {
                return a.minBits < b.minBits;
            });
            m_cheermotes.insert(prefix, tiers);
        }

        for (auto it = m_pendingCheerLookups.begin(); it != m_pendingCheerLookups.end();) {
            const QString cheerId = it.key();
            const QString cheerUrl = cheerEmoteUrlFor(it.value().first, it.value().second);
            if (!cheerUrl.isEmpty() && !m_requestedCheerKeys.contains(cheerId)) {
                m_requestedCheerKeys.insert(cheerId);
                m_emoteWriter->saveEmoteFromUrl(cheerId, cheerUrl);
                it = m_pendingCheerLookups.erase(it);
                continue;
            }
            ++it;
        }

        reply->deleteLater();
    });
}

QString TwitchChatReader::cheerEmoteUrlFor(const QString &prefix, int bits) const
{
    const QList<CheermoteTier> tiers = m_cheermotes.value(prefix.toLower());
    QString candidate;
    for (const CheermoteTier &tier : tiers) {
        if (bits >= tier.minBits)
            candidate = tier.darkStatic3x;
        else
            break;
    }
    return candidate;
}

void TwitchChatReader::mergeIrcPrivmsg(const QString &sender,
                                       const QString &tags,
                                       const QString &trailing,
                                       const QString &metadata)
{
    static QRegularExpression idRegex("(?:^|;)id=([^;\\s]+)");
    static QRegularExpression emoteRegex("emotes=([^;\\s]+)");

    QString messageId;
    const QRegularExpressionMatch idMatch = idRegex.match(tags);
    if (idMatch.hasMatch())
        messageId = idMatch.captured(1);
    if (messageId.isEmpty())
        messageId = QStringLiteral("irc-fallback-%1").arg(++m_fallbackCounter);

    ChatEvent &event = m_pendingEvents[messageId];
    event.messageId = messageId;
    event.sender = sender;
    event.trailing = trailing;
    event.metadata = metadata;
    event.fromIrc = true;
    event.gigantified = metadata.contains("msg-id=gigantified-emote-message");

    const QRegularExpressionMatch emoteMatch = emoteRegex.match(tags);
    if (emoteMatch.hasMatch()) {
        QStringList emoteList = emoteMatch.captured(1).split('/');
        for (const QString &emote : emoteList) {
            QStringList parts = emote.split(':');
            if (parts.size() < 2)
                continue;

            QString emoteId = parts[0];
            QStringList ranges = parts[1].split(',');
            QString lastRange = ranges.last();
            const QStringList positions = lastRange.split('-');
            if (positions.size() != 2)
                continue;

            int start = positions[0].toInt();
            int end = positions[1].toInt() + 1;
            if (start >= 0 && end <= trailing.length() && end > start)
                event.ircEmotes[emoteId] = trailing.mid(start, end - start).trimmed();
        }
    }

    QString content = trailing;
    while (!content.isEmpty()) {
        QPair<QString, QString> emojidata = m_emojiMapper.findBestMatch(content);
        if (!emojidata.first.isEmpty()) {
            event.emojis[emojidata.second] = emojidata.first;
            content.remove(0, emojidata.first.length());
        } else {
            content.remove(0, 1);
        }
    }

    scheduleFinalize(messageId);
}

void TwitchChatReader::mergeEventSubMessage(const QString &messageId,
                                            const QString &sender,
                                            const QString &trailing,
                                            const QMap<QString, QString> &emotes,
                                            const QMap<QString, QString> &cheermotes)
{
    if (messageId.isEmpty())
        return;

    ChatEvent &event = m_pendingEvents[messageId];
    event.messageId = messageId;
    if (event.sender.isEmpty())
        event.sender = sender;
    if (event.trailing.isEmpty())
        event.trailing = trailing;
    event.fromEventSub = true;

    for (auto it = emotes.constBegin(); it != emotes.constEnd(); ++it)
        event.eventSubEmotes[it.key()] = it.value();
    for (auto it = cheermotes.constBegin(); it != cheermotes.constEnd(); ++it)
        event.cheerEmotes[it.key()] = it.value();

    scheduleFinalize(messageId);
}

void TwitchChatReader::scheduleFinalize(const QString &messageId)
{
    QTimer *timer = m_finalizeTimers.value(messageId, nullptr);
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, messageId]() {
            finalizePending(messageId);
        });
        m_finalizeTimers.insert(messageId, timer);
    }

    timer->start(400);
}

void TwitchChatReader::finalizePending(const QString &messageId)
{
    if (!m_pendingEvents.contains(messageId))
        return;

    ChatEvent event = m_pendingEvents.take(messageId);
    QTimer *timer = m_finalizeTimers.take(messageId);
    if (timer)
        timer->deleteLater();

    processEvent(event);
}

bool TwitchChatReader::shouldSkipSender(const QString &sender) const
{
    QSettings settings;
    QStringList exceptNames = settings.value(CFG_EXCLUDE_CHAT).toStringList();
    for (QString &exceptName : exceptNames)
        exceptName = exceptName.toLower();

    return exceptNames.contains(sender.toLower());
}

void TwitchChatReader::processEvent(ChatEvent &event)
{
    if (event.processed || shouldSkipSender(event.sender))
        return;
    event.processed = true;

    QList<QPixmap> emotePixmaps;
    QStringList missingEmotes;

    QMap<QString, QString> mergedEmotes = event.ircEmotes;
    for (auto it = event.eventSubEmotes.constBegin(); it != event.eventSubEmotes.constEnd(); ++it)
        mergedEmotes[it.key()] = it.value();

    QString gigantifiedId;
    QString gigantifiedName;
    if (event.gigantified && !mergedEmotes.isEmpty()) {
        gigantifiedId = mergedEmotes.lastKey();
        gigantifiedName = mergedEmotes.value(gigantifiedId);
        emit bigEmoteSent(gigantifiedId, gigantifiedName);
        mergedEmotes.remove(gigantifiedId);
    }

    for (auto it = mergedEmotes.constBegin(); it != mergedEmotes.constEnd(); ++it) {
        emit emoteSent(it.key(), it.value());
        QPixmap pix = m_emoteWriter ? m_emoteWriter->pixmapFor(it.key()) : QPixmap();
        if (!pix.isNull())
            emotePixmaps.append(pix);
        else
            missingEmotes.append(it.key());
    }

    for (auto it = event.cheerEmotes.constBegin(); it != event.cheerEmotes.constEnd(); ++it) {
        emit emoteSent(it.key(), it.value());
        QPixmap pix = m_emoteWriter ? m_emoteWriter->pixmapFor(it.key()) : QPixmap();
        if (!pix.isNull())
            emotePixmaps.append(pix);
        else
            missingEmotes.append(it.key());
    }

    for (auto it = event.emojis.constBegin(); it != event.emojis.constEnd(); ++it) {
        emit emojiSent(it.key(), it.value());
        QPixmap pix = m_emoteWriter ? m_emoteWriter->pixmapFor(it.key()) : QPixmap();
        if (!pix.isNull())
            emotePixmaps.append(pix);
        else
            missingEmotes.append(it.key());
    }

    TwitchLogModel::instance()->addEntry(TwitchLogModel::Received,
                                         QStringLiteral("PRIVMSG"),
                                         event.sender,
                                         event.trailing,
                                         event.metadata,
                                         event.fromIrc,
                                         event.fromEventSub,
                                         emotePixmaps,
                                         missingEmotes);
}

void TwitchChatReader::startPingTimer()
{
    if (!m_pingTimer) {
        m_pingTimer = new QTimer(this);
        connect(m_pingTimer, &QTimer::timeout, this, [=]() {
            m_webSocket->sendTextMessage("PING");
        });
    }
    m_pingTimer->start(180000);
}
