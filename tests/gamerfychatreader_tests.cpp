#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QSettings>
#include "../gamerfychatreader.h"
#include "../settings_defaults.h"

class GamerfyChatReaderTests : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_directory;
    QString m_emojiFile;

    void dispatch(GamerfyChatReader &reader, const QString &type, const QString &id,
                  const QJsonObject &data, const QString &sequence = "1727640423458-0")
    {
        reader.receiveMessage(QString::fromUtf8(QJsonDocument(QJsonObject{
            {"op", "dispatch"}, {"t", type}, {"id", id}, {"seq", sequence}, {"d", data}
        }).toJson()));
    }

    void ready(GamerfyChatReader &reader)
    {
        reader.receiveMessage(R"({"op":"hello","d":{"version":1,"heartbeat_interval_ms":30000}})");
        dispatch(reader, "ready", "ready-id", {{"session_id", "session-1"}});
    }

    QJsonObject message(const QString &sender = "viewer") const
    {
        return {{"user_login", sender}, {"text", QString::fromUtf8("Hi 😀😀 👨‍👩‍👧‍👦")},
                {"kind", "default"}, {"message_id", "9007199254740993"}};
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        QCoreApplication::setOrganizationName("AtsumariTests");
        QCoreApplication::setApplicationName("GamerfyReader");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_emojiFile = m_directory.filePath("emoji.json");
        QFile file(m_emojiFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"([{"character":"😀","slug":"grinning"},{"character":"👨‍👩‍👧‍👦","slug":"family"}])");
    }

    void init() { QSettings().clear(); }

    void overlayKeys()
    {
        QVERIFY(GamerfyChatReader::isValidOverlayToken(" gfo_example-key "));
        QVERIFY(!GamerfyChatReader::isValidOverlayToken("gfa_app-token"));
        QVERIFY(!GamerfyChatReader::isValidOverlayToken("gfb_bot-token"));
        QVERIFY(!GamerfyChatReader::isValidOverlayToken("gfo_"));
        QVERIFY(!GamerfyChatReader::isValidOverlayToken("https://gamerfy.gg/overlay#gfo_key"));
    }

    void emojisAndReplayDeduplication()
    {
        GamerfyChatReader reader("gfo_test", nullptr, m_emojiFile);
        QSignalSpy emojis(&reader, &GamerfyChatReader::emojiSent);
        QSignalSpy chat(&reader, &GamerfyChatReader::chatMessage);
        ready(reader);
        dispatch(reader, "channel.chat.message", "9007199254740993", message());
        QCOMPARE(emojis.size(), 2);
        QCOMPARE(emojis.at(1).at(0).toString(), QString("family"));
        QCOMPARE(chat.size(), 1);
        dispatch(reader, "channel.chat.message", "9007199254740993", message());
        QCOMPARE(emojis.size(), 2);
        // Adjacent 64-bit IDs must stay distinct strings.
        dispatch(reader, "channel.chat.message", "9007199254740994", message());
        QCOMPARE(emojis.size(), 4);
    }

    void ignoredEventsAndSenders()
    {
        GamerfyChatReader reader("gfo_test", nullptr, m_emojiFile);
        QSignalSpy chat(&reader, &GamerfyChatReader::chatMessage);
        ready(reader);
        QSettings().setValue(CFG_EXCLUDE_CHAT, QStringList{"Viewer"});
        dispatch(reader, "channel.chat.message", "excluded", message());
        dispatch(reader, "channel.chat.message_update", "edit", message("other"));
        auto system = message("system");
        system.insert("kind", "system");
        dispatch(reader, "channel.chat.message", "system", system);
        reader.receiveMessage("not json");
        reader.receiveMessage("[]");
        QCOMPARE(chat.size(), 0);
        dispatch(reader, "channel.chat.message", "allowed", message("other"));
        QCOMPARE(chat.size(), 1);
    }

    void resumeAndBoundedCache()
    {
        GamerfyChatReader reader("gfo_test", nullptr, m_emojiFile);
        QSignalSpy chat(&reader, &GamerfyChatReader::chatMessage);
        ready(reader);
        reader.m_ready = false; // replay is sent before the resumed marker
        dispatch(reader, "channel.chat.message", "replayed", message(), "opaque-cursor");
        QCOMPARE(chat.size(), 1);
        QCOMPARE(reader.m_sequence, QString("opaque-cursor"));
        for (int i = 0; i < 600; ++i)
            dispatch(reader, "channel.follow", QString::number(i), {});
        QCOMPARE(reader.m_seen.size(), 512);
        reader.clearSession();
        QVERIFY(reader.m_session.isEmpty());
        QVERIFY(reader.m_sequence.isEmpty());
    }

    void handshakeAndHeartbeat()
    {
        GamerfyChatReader reader("gfo_test", nullptr, m_emojiFile);
        QSignalSpy connected(&reader, &GamerfyChatReader::connected);
        QSignalSpy errors(&reader, &GamerfyChatReader::errorOccurred);
        ready(reader);
        QCOMPARE(connected.size(), 1);
        QCOMPARE(reader.m_heartbeat.interval(), 30000);
        reader.m_waitingForAck = true;
        reader.receiveMessage(R"({"op":"heartbeat_ack"})");
        QVERIFY(!reader.m_waitingForAck);
        GamerfyChatReader invalid("gfo_test", nullptr, m_emojiFile);
        QSignalSpy invalidErrors(&invalid, &GamerfyChatReader::errorOccurred);
        invalid.receiveMessage(R"({"op":"hello","d":{"version":2,"heartbeat_interval_ms":0}})");
        QCOMPARE(invalidErrors.size(), 1);
        QVERIFY(invalid.m_stopped);
        QCOMPARE(errors.size(), 0);
    }
};

QTEST_GUILESS_MAIN(GamerfyChatReaderTests)
#include "gamerfychatreader_tests.moc"
