#include "twitchlogmodel.h"

#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QPainter>

#include "settings_defaults.h"
#include "logsettings.h"
#include "logcommandcolors.h"

namespace {
QString platformName(LogPlatform platform)
{
    switch (platform) {
    case LogPlatform::Twitch: return QStringLiteral("Twitch");
    case LogPlatform::Gamerfy: return QStringLiteral("Gamerfy");
    }
    return QString();
}

QPixmap normalizedPlatformLogo(const QString &path)
{
    const QPixmap source(path);
    if (source.isNull())
        return QPixmap();
    const QPixmap scaled = source.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap canvas(32, 32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.drawPixmap((32 - scaled.width()) / 2, (32 - scaled.height()) / 2, scaled);
    return canvas;
}

QPixmap platformLogo(LogPlatform platform)
{
    static const QPixmap twitch = normalizedPlatformLogo(QStringLiteral(":/platformicons/twitch.png"));
    static const QPixmap gamerfy = normalizedPlatformLogo(QStringLiteral(":/platformicons/gamerfy.png"));
    switch (platform) {
    case LogPlatform::Twitch: return twitch;
    case LogPlatform::Gamerfy: return gamerfy;
    }
    return QPixmap();
}
}

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
    , m_connectionStartedAt(QDateTime::currentDateTime())
{
    QSettings settings;
    migrateLogColumns(settings);
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

    if (index.column() == Platform) {
        if (role == Qt::DecorationRole)
            return platformLogo(e.platform);
        if (role == Qt::ToolTipRole || role == Qt::AccessibleTextRole)
            return platformName(e.platform);
        if (role == Qt::SizeHintRole)
            return QSize(36, 36);
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Direction:
            return e.direction == Sent ? QStringLiteral("➡️") : QStringLiteral("⬅️");
        case Source:
            if (e.fromIrc && e.fromEventSub)
                return QStringLiteral("📝🧩");
            if (e.fromEventSub)
                return QStringLiteral("🧩");
            return e.fromIrc ? QStringLiteral("📝") : QString();
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
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == Message) {
            return e.message;
        } else if (index.column() == Source) {
            if (e.fromIrc && e.fromEventSub)
                return tr("Origin: IRC and EventSub");
            if (e.fromEventSub)
                return tr("Origin: EventSub");
            return e.fromIrc ? tr("Origin: IRC") : QString();
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
        case Platform: return tr("Platform");
        case Direction: return tr("Direction");
        case Source: return tr("Source");
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

void TwitchLogModel::addEntry(LogPlatform platform,
                              MsgDirection direction,
                              const QString &command,
                              const QString &sender,
                              const QString &message,
                              const QString &tags,
                              bool fromIrc,
                              bool fromEventSub,
                              const QList<QPixmap> &emotes,
                              const QStringList &pendingEmotes)
{
    QSettings settings;
    QStringList hidden = settings.value(CFG_LOG_HIDE_CMDS, DEFAULT_LOG_HIDE_CMDS).toStringList();
    if (hidden.contains(command))
        return;
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    Entry e;
    e.platform = platform;
    e.direction = direction;
    e.timestamp = QDateTime::currentDateTime();
    e.command = command;
    e.sender = sender;
    e.message = message;
    e.tags = tags;
    e.fromIrc = fromIrc;
    e.fromEventSub = fromEventSub;
    e.emotes = emotes;
    e.pendingEmotes = pendingEmotes;
    m_entries.append(e);
    endInsertRows();

    maybeAutoSave();
}

bool TwitchLogModel::exportToFile(const QString &fileName) const
{
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    for (const Entry &e : m_entries) {
        ts << platformName(e.platform) << '\t'
           << (e.direction == Sent ? QStringLiteral("➡️") : QStringLiteral("⬅️")) << '\t'
           << (e.fromIrc && e.fromEventSub ? QStringLiteral("📝🧩") : (e.fromEventSub ? QStringLiteral("🧩") : (e.fromIrc ? QStringLiteral("📝") : QString()))) << '\t'
           << e.timestamp.toString(Qt::ISODate) << '\t'
           << e.command << '\t'
           << e.sender << '\t'
           << e.message << '\t'
           << e.tags << '\n';
    }
    return true;
}

bool TwitchLogModel::autoSaveEnabled() const
{
    QSettings settings;
    return settings.value(CFG_LOG_AUTOSAVE_ENABLED, DEFAULT_LOG_AUTOSAVE_ENABLED).toBool();
}

bool TwitchLogModel::flushAutoSave() const
{
    if (!autoSaveEnabled())
        return false;

    QSettings settings;
    const QString directory = settings.value(CFG_LOG_AUTOSAVE_DIRECTORY, defaultLogAutosaveDirectory()).toString();
    const QString pattern = settings.value(CFG_LOG_AUTOSAVE_NAME_PATTERN, DEFAULT_LOG_AUTOSAVE_NAME_PATTERN).toString();
    const QString filePath = buildAutoSaveFilePath(directory, pattern);
    if (filePath.isEmpty())
        return false;

    m_autoSaveFilePath = filePath;

    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return false;

    return exportToFile(m_autoSaveFilePath);
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

void TwitchLogModel::loadEmote(const QString &id, const QPixmap &pix)
{
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

QString TwitchLogModel::buildAutoSaveFilePath(const QString &directory, const QString &pattern) const
{
    QString fileNamePattern = pattern.trimmed();
    if (fileNamePattern.isEmpty())
        fileNamePattern = QStringLiteral(DEFAULT_LOG_AUTOSAVE_NAME_PATTERN);

    const QString token = connectionTimestampToken();
    fileNamePattern.replace(QStringLiteral("${timestamp}"), token);

    const QString baseDirectory = directory.trimmed();
    if (baseDirectory.isEmpty())
        return QString();

    return QDir(baseDirectory).filePath(fileNamePattern);
}

void TwitchLogModel::maybeAutoSave() const
{
    flushAutoSave();
}

QString TwitchLogModel::connectionTimestampToken() const
{
    return m_connectionStartedAt.toString(QStringLiteral("yyyyMMddHHmmss"));
}

void TwitchLogModel::setConnectionStartedAt(const QDateTime &timestamp)
{
    m_connectionStartedAt = timestamp.isValid() ? timestamp : QDateTime::currentDateTime();
    m_autoSaveFilePath.clear();
}
