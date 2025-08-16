#ifndef TWITCHLOGMODEL_H
#define TWITCHLOGMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QPixmap>
#include <QVector>
#include <QHash>
#include <QList>

class TwitchLogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Columns { Timestamp = 0, Command, Badges, Sender, Message, Tags, Emotes, ColumnCount };

    struct Entry {
        QDateTime timestamp;
        QString command;
        QList<QPixmap> badges;
        QString sender;
        QString message;
        QString tags;
        QList<QPixmap> emotes;
    };

    static TwitchLogModel* instance();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void addEntry(const QString &command,
                  const QString &sender,
                  const QString &message,
                  const QString &tags,
                  const QList<QPixmap> &badges = QList<QPixmap>(),
                  const QList<QPixmap> &emotes = QList<QPixmap>());

    bool exportToFile(const QString &fileName) const;

    QList<QPixmap> emotesForRow(int row) const;

private:
    explicit TwitchLogModel(QObject *parent = nullptr);
    void loadColors();

    QVector<Entry> m_entries;
    QHash<QString, QColor> m_fgColors;
    QHash<QString, QColor> m_bgColors;
};

#endif // TWITCHLOGMODEL_H
