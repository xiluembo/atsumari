#include "twitchlogmodel.h"

#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QPainter>

#include "settings_defaults.h"

static TwitchLogModel* s_instance = nullptr;

TwitchLogModel* TwitchLogModel::instance()
{
    if (!s_instance) {
        s_instance = new TwitchLogModel();
    }
    return s_instance;
}

TwitchLogModel::TwitchLogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadColors();
}

int TwitchLogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_entries.size();
}

int TwitchLogModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant TwitchLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const Entry &e = m_entries.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Direction:
            return e.direction == Sent ? QStringLiteral("➡️") : QStringLiteral("⬅️");
        case Timestamp:
            return e.timestamp.toString(Qt::ISODate);
        case Command:
            return e.command;
        case Sender:
            return e.sender;
        case Message:
            return e.message;
        case Tags:
            return e.tags;
        default:
            return QVariant();
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == Badges && !e.badges.isEmpty())
            return e.badges.first();
        if (index.column() == Emotes && !e.emotes.isEmpty()) {
            if (e.emotes.size() == 1)
                return e.emotes.first();

            int totalWidth = 0;
            int maxHeight = 0;
            for (const QPixmap &p : e.emotes) {
                totalWidth += p.width();
                maxHeight = qMax(maxHeight, p.height());
            }

            QPixmap combined(totalWidth, maxHeight);
            combined.fill(Qt::transparent);
            QPainter painter(&combined);
            int x = 0;
            for (const QPixmap &p : e.emotes) {
                painter.drawPixmap(x, 0, p);
                x += p.width();
            }
            return combined;
        }
    } else if (role == Qt::ForegroundRole) {
        auto it = m_fgColors.find(e.command);
        if (it != m_fgColors.end())
            return it.value();
    } else if (role == Qt::BackgroundRole) {
        auto it = m_bgColors.find(e.command);
        if (it != m_bgColors.end())
            return it.value();
    }
    return QVariant();
}

QVariant TwitchLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Direction: return tr("Direction");
        case Timestamp: return tr("Timestamp");
        case Command: return tr("Command");
        case Badges: return tr("Badges");
        case Sender: return tr("Sender");
        case Message: return tr("Message");
        case Tags: return tr("Tags");
        case Emotes: return tr("Emotes");
        }
    }
    return QVariant();
}

void TwitchLogModel::addEntry(MsgDirection direction,
                              const QString &command,
                              const QString &sender,
                              const QString &message,
                              const QString &tags,
                              const QList<QPixmap> &badges,
                              const QList<QPixmap> &emotes)
{
    QSettings settings;
    QStringList hidden = settings.value(CFG_LOG_HIDE_CMDS, DEFAULT_LOG_HIDE_CMDS).toStringList();
    if (hidden.contains(command))
        return;
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    Entry e;
    e.direction = direction;
    e.timestamp = QDateTime::currentDateTime();
    e.command = command;
    e.sender = sender;
    e.message = message;
    e.tags = tags;
    e.badges = badges;
    e.emotes = emotes;
    m_entries.append(e);
    endInsertRows();
}

bool TwitchLogModel::exportToFile(const QString &fileName) const
{
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    for (const Entry &e : m_entries) {
        ts << (e.direction == Sent ? QStringLiteral("➡️") : QStringLiteral("⬅️")) << '\t'
           << e.timestamp.toString(Qt::ISODate) << '\t'
           << e.command << '\t'
           << e.sender << '\t'
           << e.message << '\t'
           << e.tags << '\n';
    }
    return true;
}

QList<QPixmap> TwitchLogModel::emotesForRow(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return QList<QPixmap>();
    return m_entries.at(row).emotes;
}

void TwitchLogModel::loadColors()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("log/colors"));
    QStringList cmds = settings.childGroups();
    for (const QString &cmd : cmds) {
        settings.beginGroup(cmd);
        QColor fg = settings.value("fg", QColor(Qt::black)).value<QColor>();
        QColor bg = settings.value("bg", QColor(Qt::white)).value<QColor>();
        m_fgColors.insert(cmd, fg);
        m_bgColors.insert(cmd, bg);
        settings.endGroup();
    }
    settings.endGroup();
}
