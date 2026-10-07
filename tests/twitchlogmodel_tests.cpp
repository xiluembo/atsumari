#include <QtTest>
#include <QFile>
#include <QImage>
#include <QSettings>
#include <QTemporaryDir>
#include "../twitchlogmodel.h"
#include "../logsettings.h"

class TwitchLogModelTests : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_directory;

private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        QCoreApplication::setOrganizationName("AtsumariTests");
        QCoreApplication::setApplicationName("LogPlatform");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
    }

    void init() { QSettings().clear(); }

    void previousSettingsEnablePlatformOnce()
    {
        QSettings settings;
        settings.setValue(CFG_VERSION, 3);
        settings.setValue(CFG_LOG_COLUMNS, QStringList{"Timestamp", "Message"});
        settings.setValue(CFG_LOG_SOURCE_MIGRATED, true);
        settings.setValue(CFG_TOKEN, "existing-token");
        const auto migrated = migrateLogColumns(settings);
        QCOMPARE(migrated, (QStringList{"Platform", "Timestamp", "Message"}));
        QCOMPARE(settings.value(CFG_TOKEN).toString(), QString("existing-token"));
        QCOMPARE(settings.value(CFG_LOG_COLUMNS_VERSION).toInt(), CURRENT_LOG_COLUMNS_VERSION);
        // Turning the column off is a lasting preference, even if the app's
        // global version has not yet been saved by the setup dialog.
        settings.setValue(CFG_LOG_COLUMNS, QStringList{"Message"});
        QCOMPARE(migrateLogColumns(settings), QStringList{"Message"});
    }

    void newSettingsAndLegacySource()
    {
        QSettings settings;
        QCOMPARE(migrateLogColumns(settings), DEFAULT_LOG_COLUMNS);
        settings.clear();
        settings.setValue(CFG_LOG_COLUMNS, QStringList{"Sender"});
        QCOMPARE(migrateLogColumns(settings), (QStringList{"Platform", "Sender", "Source"}));
        QCOMPARE(migrateLogColumns(settings).count("Platform"), 1);
        settings.setValue(CFG_LOG_COLUMNS, QStringList());
        QVERIFY(migrateLogColumns(settings).isEmpty());
    }

    void platformIconsAndExport()
    {
        auto *model = TwitchLogModel::instance();
        const int first = model->rowCount();
        model->addEntry(LogPlatform::Twitch, TwitchLogModel::Received,
                        "PRIVMSG", "viewer", "twitch-message", "");
        model->addEntry(LogPlatform::Gamerfy, TwitchLogModel::Received,
                        "PRIVMSG", "viewer", "gamerfy-message", "", false, false);
        model->addEntry(LogPlatform::Gamerfy, TwitchLogModel::Received,
                        "GAMERFY", "", "connection-error", "", false, false);
        QCOMPARE(model->rowCount(), first + 3);
        const QStringList names{"Twitch", "Gamerfy", "Gamerfy"};
        for (int i = 0; i < names.size(); ++i) {
            const QModelIndex platform = model->index(first + i, TwitchLogModel::Platform);
            QVERIFY(!model->data(platform, Qt::DisplayRole).isValid());
            QCOMPARE(model->data(platform, Qt::ToolTipRole).toString(), names.at(i));
            QCOMPARE(model->data(platform, Qt::AccessibleTextRole).toString(), names.at(i));
            const QPixmap icon = qvariant_cast<QPixmap>(model->data(platform, Qt::DecorationRole));
            QVERIFY(!icon.isNull());
            QCOMPARE(icon.size(), QSize(32, 32));
            QVERIFY(icon.hasAlphaChannel());
            QCOMPARE(icon.toImage().pixelColor(31, 31).alpha(), 0);
        }
        QVERIFY(model->data(model->index(first + 1, TwitchLogModel::Source), Qt::DisplayRole).toString().isEmpty());
        // Export remains textual even when the platform column is hidden.
        QSettings settings;
        settings.setValue(CFG_LOG_COLUMNS, QStringList{"Message"});
        const QString path = m_directory.filePath("export.txt");
        QVERIFY(model->exportToFile(path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
        QCOMPARE(lines.size(), first + 3);
        for (int i = 0; i < names.size(); ++i)
            QCOMPARE(lines.at(first + i).section('\t', 0, 0), names.at(i));

        settings.setValue(CFG_LOG_AUTOSAVE_ENABLED, true);
        settings.setValue(CFG_LOG_AUTOSAVE_DIRECTORY, m_directory.path());
        settings.setValue(CFG_LOG_AUTOSAVE_NAME_PATTERN, "autosave.txt");
        QVERIFY(model->flushAutoSave());
        QFile automatic(m_directory.filePath("autosave.txt"));
        QVERIFY(automatic.open(QIODevice::ReadOnly));
        QVERIFY(file.seek(0));
        QCOMPARE(automatic.readAll(), file.readAll());
    }
};

QTEST_MAIN(TwitchLogModelTests)
#include "twitchlogmodel_tests.moc"
