#ifndef GAMERFYCHATREADER_H
#define GAMERFYCHATREADER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QQueue>
#include <QSet>
#include "emojimapper.h"

// Gamerfy's documented read-only overlay gateway. No client secret is needed.
class GamerfyChatReader : public QObject
{
    Q_OBJECT
public:
    explicit GamerfyChatReader(const QString &token, QObject *parent = nullptr,
                               const QString &emojiFile = ":/emoji/emojis.json");
    ~GamerfyChatReader() override;
    void start();
    static bool isValidOverlayToken(const QString &token);

signals:
    void connected();
    void errorOccurred(const QString &message);
    void emojiSent(const QString &slug, const QString &emoji);
    void chatMessage(const QString &sender, const QString &text);

private:
    friend class GamerfyChatReaderTests;
    void receiveMessage(const QString &message);
    void disconnected();
    void send(const QString &op, const QJsonObject &data = {});
    void clearSession();

    QWebSocket m_socket;
    QTimer m_heartbeat;
    QTimer m_reconnect;
    QTimer m_handshake;
    EmojiMapper m_emojiMapper;
    QString m_token;
    QString m_session;
    QString m_sequence;
    QSet<QString> m_seen;
    QQueue<QString> m_seenOrder;
    int m_retrySeconds = 1;
    bool m_stopped = false;
    bool m_identified = false;
    bool m_ready = false;
    bool m_waitingForAck = false;
};

#endif
