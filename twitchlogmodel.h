#ifndef TWITCHLOGMODEL_H
#define TWITCHLOGMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QPixmap>
#include <QVector>
#include <QHash>
#include <QList>
#include <QStringList>

class TwitchLogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Columns { Direction = 0, Source, Timestamp, Command, Sender, Message, Tags, Emotes, ColumnCount };
    enum MsgDirection { Sent = 0, Received };

    struct Entry {
        MsgDirection direction;
        QDateTime timestamp;
        QString command;
        QString sender;
        QString message;
        QString tags;
        bool fromIrc = true;
        bool fromEventSub = false;
        QList<QPixmap> emotes;
        QStringList pendingEmotes;
    };

    static TwitchLogModel* instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void addEntry(MsgDirection direction,
                  const QString &command,
                  const QString &sender,
                  const QString &message,
                  const QString &tags,
                  bool fromIrc = true,
                  bool fromEventSub = false,
                  const QList<QPixmap> &emotes = QList<QPixmap>(),
                  const QStringList &pendingEmotes = QStringList());

    bool exportToFile(const QString &fileName) const;

    QList<QPixmap> emotesForRow(int row) const;

public slots:
    void loadEmote(const QString &id, const QPixmap &pixmap);
    void setConnectionStartedAt(const QDateTime &timestamp);

private:
    explicit TwitchLogModel(QObject *parent = nullptr);
    void loadColors();
    QString buildAutoSaveFilePath(const QString &directory, const QString &pattern) const;
    void maybeAutoSave() const;
    QString connectionTimestampToken() const;


    QVector<Entry> m_entries;
    QHash<QString, QColor> m_fgColors;
    QHash<QString, QColor> m_bgColors;
    QDateTime m_connectionStartedAt;
    mutable QString m_autoSaveFilePath;
};

#endif // TWITCHLOGMODEL_H
