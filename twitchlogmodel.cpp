#include "twitchlogmodel.h"

#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

#include "settings_defaults.h"
#include "logcommandcolors.h"

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
                              const QList<QPixmap> &emotes,
                              const QStringList &pendingEmotes)
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
    e.emotes = emotes;
    e.pendingEmotes = pendingEmotes;
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
        const auto defaults = defaultCommandColors(cmd);
        QColor fg = settings.value("fg", defaults.first).value<QColor>();
        QColor bg = settings.value("bg", defaults.second).value<QColor>();
        m_fgColors.insert(cmd, fg);
        m_bgColors.insert(cmd, bg);
        settings.endGroup();
    }
    settings.endGroup();

    // Defaults for common commands when no colors are configured
    const QStringList defaultCmds{QStringLiteral("PRIVMSG"), QStringLiteral("JOIN"), QStringLiteral("PART")};
    for (const QString &cmd : defaultCmds) {
        if (!m_fgColors.contains(cmd) || !m_bgColors.contains(cmd)) {
            const auto colors = defaultCommandColors(cmd);
            if (!m_fgColors.contains(cmd))
                m_fgColors.insert(cmd, colors.first);
            if (!m_bgColors.contains(cmd))
                m_bgColors.insert(cmd, colors.second);
        }
    }
}

void TwitchLogModel::loadEmote(const QString &path)
{
    QString id = QFileInfo(path).baseName();
    QPixmap pix(path);
    if (pix.isNull())
        return;
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry &e = m_entries[i];
        if (e.pendingEmotes.contains(id)) {
            e.pendingEmotes.removeAll(id);
            e.emotes.append(pix);
            QModelIndex idx = index(i, Emotes);
            emit dataChanged(idx, idx, {Qt::DecorationRole});
        }
    }
}
