#include "gamerfychatreader.h"
#include "settings_defaults.h"

#include <QJsonDocument>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>

GamerfyChatReader::GamerfyChatReader(const QString &token, QObject *parent,
                                   const QString &emojiFile)
    : QObject(parent), m_emojiMapper(emojiFile), m_token(token.trimmed())
{
    m_reconnect.setSingleShot(true);
    m_handshake.setSingleShot(true);
    connect(&m_reconnect, &QTimer::timeout, this, &GamerfyChatReader::start);
    connect(&m_socket, &QWebSocket::connected, this, [this] {
        m_handshake.start(15000);
    });
    connect(&m_handshake, &QTimer::timeout, this, [this] { m_socket.abort(); });
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &GamerfyChatReader::receiveMessage);
    connect(&m_socket, &QWebSocket::disconnected, this, &GamerfyChatReader::disconnected);
    connect(&m_socket, &QWebSocket::errorOccurred, this, [this] {
        // Do not log server payloads or tokens.
        emit errorOccurred(tr("Could not connect to Gamerfy. Retrying."));
        if (m_socket.state() == QAbstractSocket::UnconnectedState && !m_reconnect.isActive())
            disconnected();
    });
    connect(&m_heartbeat, &QTimer::timeout, this, [this] {
        if (m_waitingForAck) {
            m_socket.abort();
            return;
        }
        m_waitingForAck = true;
        send(QStringLiteral("heartbeat"));
    });
}

GamerfyChatReader::~GamerfyChatReader()
{
    m_stopped = true;
    m_heartbeat.stop();
    m_reconnect.stop();
    m_handshake.stop();
    m_socket.disconnect(this);
    m_socket.abort();
}

bool GamerfyChatReader::isValidOverlayToken(const QString &token)
{
    // The gateway documents the gfo_ prefix; the key's length is opaque.
    static const QRegularExpression pattern(QStringLiteral("^gfo_[A-Za-z0-9_-]+$"));
    return pattern.match(token.trimmed()).hasMatch();
}

void GamerfyChatReader::start()
{
    if (m_stopped || m_socket.state() != QAbstractSocket::UnconnectedState)
        return;
    if (!isValidOverlayToken(m_token)) {
        m_stopped = true;
        emit errorOccurred(tr("Enter a valid Gamerfy overlay key (gfo_…)."));
        return;
    }
    m_reconnect.stop();
    m_identified = false;
    m_ready = false;
    m_waitingForAck = false;
    m_socket.open(QUrl(QStringLiteral("wss://api.gamerfy.gg/v1/events")));
}

void GamerfyChatReader::send(const QString &op, const QJsonObject &data)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;
    QJsonObject frame{{"op", op}};
    if (!data.isEmpty())
        frame.insert("d", data);
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(frame).toJson(QJsonDocument::Compact)));
}

void GamerfyChatReader::clearSession()
{
    m_session.clear();
    m_sequence.clear();
    // Keep the bounded deduplication cache across a fresh identify, too.
}

void GamerfyChatReader::disconnected()
{
    m_heartbeat.stop();
    m_handshake.stop();
    m_ready = false;
    const int code = static_cast<int>(m_socket.closeCode());
    if (code == 4000 || code == 4002 || code == 4003 || code == 4010) {
        m_stopped = true;
        m_reconnect.stop();
        emit errorOccurred(tr("Gamerfy disconnected (code %1). Check your overlay key and active connections, then restart Atsumari.").arg(code));
        return;
    }
    if (code == 4006)
        clearSession();
    if (!m_stopped && !m_reconnect.isActive()) {
        const int delay = qMin(30000, m_retrySeconds * 1000
                              + QRandomGenerator::global()->bounded(501));
        m_reconnect.start(delay);
        m_retrySeconds = qMin(30, m_retrySeconds * 2);
    }
}

void GamerfyChatReader::receiveMessage(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;
    const QJsonObject frame = document.object();
    const QString op = frame.value("op").toString();
    const QJsonObject data = frame.value("d").toObject();
    if (op == QStringLiteral("hello")) {
        if (m_identified)
            return;
        const int interval = data.value("heartbeat_interval_ms").toInt();
        if (data.value("version").toInt() != 1 || interval < 1000 || interval > 60000) {
            m_stopped = true;
            m_socket.abort();
            emit errorOccurred(tr("Unsupported Gamerfy gateway handshake."));
            return;
        }
        m_identified = true;
        if (!m_session.isEmpty() && !m_sequence.isEmpty()) {
            send(QStringLiteral("resume"), {{"token", m_token}, {"session_id", m_session}, {"seq", m_sequence}});
        } else {
            send(QStringLiteral("identify"), {{"token", m_token}});
        }
        m_heartbeat.start(interval);
        return;
    }
    if (op == QStringLiteral("heartbeat_ack")) {
        m_waitingForAck = false;
        return;
    }
    if (op != QStringLiteral("dispatch") || !m_identified)
        return;
    const QString type = frame.value("t").toString();
    if (type == QStringLiteral("ready")) {
        m_session = data.value("session_id").toString();
        m_ready = true;
        m_retrySeconds = 1;
        m_handshake.stop();
        emit connected();
    } else if (type == QStringLiteral("resumed")) {
        m_ready = true;
        m_retrySeconds = 1;
        m_handshake.stop();
        emit connected();
    }
    // Replayed dispatches arrive before 'resumed'.
    if (!m_ready && m_session.isEmpty())
        return;
    const QString sequence = frame.value("seq").toString();
    if (!sequence.isEmpty())
        m_sequence = sequence;
    const QString id = frame.value("id").toString();
    if (!id.isEmpty()) {
        if (m_seen.contains(id))
            return;
        m_seen.insert(id);
        m_seenOrder.enqueue(id);
        if (m_seenOrder.size() > 512)
            m_seen.remove(m_seenOrder.dequeue());
    }
    if (type != QStringLiteral("channel.chat.message") || id.isEmpty()
        || data.value("kind").toString() != QStringLiteral("default"))
        return;

    const QString sender = data.value("user_login").toString();
    const QString content = data.value("text").toString();
    if (sender.isEmpty() || content.isEmpty())
        return;
    const QStringList excluded = QSettings().value(CFG_EXCLUDE_CHAT, DEFAULT_EXCLUDE_CHAT).toStringList();
    if (excluded.contains(sender, Qt::CaseInsensitive))
        return;
    emit chatMessage(sender, content);
    QString remaining = content;
    QSet<QString> emitted;
    while (!remaining.isEmpty()) {
        const auto match = m_emojiMapper.findBestMatch(remaining);
        if (match.first.isEmpty()) {
            remaining.remove(0, 1);
        } else {
            if (!emitted.contains(match.second)) {
                emitted.insert(match.second);
                emit emojiSent(match.second, match.first);
            }
            remaining.remove(0, match.first.size());
        }
    }
}
