/*
 * Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
 * Copyright (C) 2026 - Andrius da Costa Ribas <andriusmao@gmail.com>
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

#include "communityshadersloader.h"

#include <QtCore/private/qzipreader_p.h>

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>

namespace {
QNetworkAccessManager& communityShadersNetworkAccessManager()
{
    static QNetworkAccessManager manager;
    return manager;
}

QString normalizeReleaseTag(const QString& tag)
{
    if (tag.startsWith('v')) {
        return tag.mid(1);
    }

    return tag;
}

QString localizedMetadataValue(const QJsonObject& metadata,
                               const QLocale& locale,
                               const QString& key)
{
    const QString localeName = locale.name();
    const QJsonArray translations = metadata.value(QStringLiteral("translations")).toArray();

    for (const QJsonValue& translationValue : translations) {
        const QJsonObject translationObject = translationValue.toObject();
        const QJsonObject localizedObject = translationObject.value(localeName).toObject();
        const QString localizedValue = localizedObject.value(key).toString().trimmed();
        if (!localizedValue.isEmpty()) {
            return localizedValue;
        }
    }

    return metadata.value(key).toString().trimmed();
}

QString packSearchableText(const CommunityShaderPack& pack)
{
    return QStringLiteral("%1 %2 %3 %4 %5")
        .arg(pack.name, pack.description, pack.authorName, pack.authorEmail, pack.authorWebsite)
        .toCaseFolded();
}

QUrl websiteUrlFromString(const QString& website)
{
    if (website.trimmed().isEmpty()) {
        return {};
    }

    return QUrl::fromUserInput(website.trimmed());
}

bool isPathInsideDirectory(const QString& rootPath, const QString& candidatePath)
{
    const QString normalizedRoot = QDir::cleanPath(QDir(rootPath).absolutePath());
    const QString normalizedCandidate = QDir::cleanPath(QFileInfo(candidatePath).absoluteFilePath());
    const QString normalizedRootWithSlash = normalizedRoot + QLatin1Char('/');

    return normalizedCandidate == normalizedRoot
        || normalizedCandidate.startsWith(normalizedRootWithSlash);
}

bool writeZipEntry(QZipReader& zipReader,
                   const QZipReader::FileInfo& entry,
                   const QString& outputRoot,
                   QString* errorMessage)
{
    const QString relativePath = QDir::cleanPath(entry.filePath);
    if (relativePath.isEmpty() || relativePath == QStringLiteral(".")) {
        return true;
    }

    if (relativePath.startsWith(QStringLiteral("..")) || QDir::isAbsolutePath(relativePath)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("The community shader archive contains an unsafe path.");
        }
        return false;
    }

    const QString outputPath = QDir(outputRoot).absoluteFilePath(relativePath);
    if (!isPathInsideDirectory(outputRoot, outputPath)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("The community shader archive contains an invalid path.");
        }
        return false;
    }

    if (entry.isDir) {
        if (!QDir().mkpath(outputPath)) {
            if (errorMessage) {
                *errorMessage = QObject::tr("Unable to create the extraction directory for community shaders.");
            }
            return false;
        }
        return true;
    }

    if (entry.isSymLink) {
        return true;
    }

    if (!QDir().mkpath(QFileInfo(outputPath).absolutePath())) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Unable to create the extraction directory for community shaders.");
        }
        return false;
    }

    const QByteArray fileData = zipReader.fileData(entry.filePath);
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Unable to write extracted community shader files.");
        }
        return false;
    }

    if (outputFile.write(fileData) != fileData.size()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Unable to write extracted community shader files.");
        }
        return false;
    }

    return true;
}

QString findArchiveRootPath(const QString& extractedPath)
{
    QDir extractedDir(extractedPath);
    const QFileInfoList candidateDirs = extractedDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                                   QDir::Name | QDir::IgnoreCase);

    if (candidateDirs.size() == 1) {
        return candidateDirs.first().absoluteFilePath();
    }

    return extractedDir.absolutePath();
}

bool loadPackFromDirectory(const QFileInfo& dirInfo,
                           const QLocale& locale,
                           const QString& releaseTag,
                           CommunityShaderPack* pack)
{
    const QDir packDir(dirInfo.absoluteFilePath());
    const QString metadataPath = packDir.absoluteFilePath(QStringLiteral("metadata.json"));
    if (!QFileInfo::exists(metadataPath)) {
        return false;
    }

    const QStringList vertFiles = packDir.entryList({QStringLiteral("*.vert")}, QDir::Files, QDir::Name);
    const QStringList fragFiles = packDir.entryList({QStringLiteral("*.frag")}, QDir::Files, QDir::Name);
    if (vertFiles.isEmpty() || fragFiles.isEmpty()) {
        return false;
    }

    QFile metadataFile(metadataPath);
    if (!metadataFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument metadataDoc = QJsonDocument::fromJson(metadataFile.readAll());
    if (!metadataDoc.isObject()) {
        return false;
    }

    QFile vertexFile(packDir.absoluteFilePath(vertFiles.first()));
    QFile fragmentFile(packDir.absoluteFilePath(fragFiles.first()));
    if (!vertexFile.open(QIODevice::ReadOnly) || !fragmentFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonObject metadata = metadataDoc.object();
    const QJsonObject author = metadata.value(QStringLiteral("author")).toObject();

    pack->id = dirInfo.fileName();
    pack->releaseTag = releaseTag;
    pack->name = localizedMetadataValue(metadata, locale, QStringLiteral("name"));
    pack->description = localizedMetadataValue(metadata, locale, QStringLiteral("description"));
    pack->authorName = author.value(QStringLiteral("name")).toString().trimmed();
    pack->authorEmail = author.value(QStringLiteral("email")).toString().trimmed();
    pack->authorWebsite = websiteUrlFromString(author.value(QStringLiteral("website")).toString()).toString();
    pack->vertexShaderPath = vertexFile.fileName();
    pack->fragmentShaderPath = fragmentFile.fileName();
    pack->vertexShaderSource = QString::fromUtf8(vertexFile.readAll());
    pack->fragmentShaderSource = QString::fromUtf8(fragmentFile.readAll());
    pack->searchableText = packSearchableText(*pack);

    return !pack->name.isEmpty()
        && !pack->description.isEmpty()
        && !pack->authorName.isEmpty()
        && !pack->vertexShaderSource.isEmpty()
        && !pack->fragmentShaderSource.isEmpty();
}
}

CommunityShadersLoader::CommunityShadersLoader(const QLocale& locale, QObject *parent)
    : QObject(parent)
    , m_locale(locale)
    , m_temporaryDir()
{
    m_temporaryDir.setAutoRemove(true);
}

void CommunityShadersLoader::setLocale(const QLocale& locale)
{
    m_locale = locale;
}

void CommunityShadersLoader::fetchLatestRelease()
{
    clearLoadedState();
    requestLatestRelease(false);
}

void CommunityShadersLoader::loadRelease(const CommunityShaderReleaseInfo& release)
{
    clearLoadedState();

    if (release.isValid()) {
        m_releaseInfo = release;
        emit latestReleaseChanged();
        beginArchiveDownload(m_releaseInfo);
        return;
    }

    requestLatestRelease(true);
}

const CommunityShaderReleaseInfo& CommunityShadersLoader::releaseInfo() const
{
    return m_releaseInfo;
}

const QList<CommunityShaderPack>& CommunityShadersLoader::packs() const
{
    return m_packs;
}

QString CommunityShadersLoader::errorString() const
{
    return m_errorString;
}

void CommunityShadersLoader::cancelCurrentReply()
{
    if (!m_currentReply) {
        return;
    }

    disconnect(m_currentReply, nullptr, this, nullptr);
    m_currentReply->abort();
    m_currentReply->deleteLater();
    m_currentReply.clear();
}

void CommunityShadersLoader::clearLoadedState()
{
    cancelCurrentReply();
    m_packs.clear();
    m_errorString.clear();
    if (!m_temporaryDir.path().isEmpty()) {
        m_temporaryDir.remove();
        QDir().mkpath(m_temporaryDir.path());
    }
}

void CommunityShadersLoader::requestLatestRelease(bool continueLoadingAfterReply)
{
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/xiluembo/atsumari-communityshaders/releases/latest")));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Atsumari"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = communityShadersNetworkAccessManager().get(request);
    m_currentReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, continueLoadingAfterReply]() {
        handleLatestReleaseReply(reply, continueLoadingAfterReply);
    });
}

void CommunityShadersLoader::handleLatestReleaseReply(QNetworkReply* reply, bool continueLoadingAfterReply)
{
    if (!reply) {
        return;
    }

    const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);
    if (m_currentReply == reply) {
        m_currentReply.clear();
    }

    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Unable to check the latest community shader release: %1").arg(reply->errorString()));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QJsonObject root = doc.object();
    const QString tag = normalizeReleaseTag(root.value(QStringLiteral("tag_name")).toString().trimmed());
    const QUrl zipballUrl = QUrl(root.value(QStringLiteral("zipball_url")).toString());

    if (tag.isEmpty() || !zipballUrl.isValid()) {
        fail(tr("The latest community shader release does not contain a valid zip archive."));
        return;
    }

    m_releaseInfo = {tag, zipballUrl};
    emit latestReleaseChanged();

    if (continueLoadingAfterReply) {
        beginArchiveDownload(m_releaseInfo);
    }
}

void CommunityShadersLoader::beginArchiveDownload(const CommunityShaderReleaseInfo& release)
{
    emit statusChanged(tr("Downloading community shader packs..."));
    emit progressRangeChanged(0, 0);
    emit progressValueChanged(0);

    QNetworkRequest request(release.zipballUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Atsumari"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = communityShadersNetworkAccessManager().get(request);
    m_currentReply = reply;

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            emit progressRangeChanged(0, 100);
            emit progressValueChanged(static_cast<int>((bytesReceived * 100) / bytesTotal));
        } else {
            emit progressRangeChanged(0, 0);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleArchiveReply(reply);
    });
}

void CommunityShadersLoader::handleArchiveReply(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);
    if (m_currentReply == reply) {
        m_currentReply.clear();
    }

    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Unable to download community shader packs: %1").arg(reply->errorString()));
        return;
    }

    if (m_temporaryDir.path().isEmpty()) {
        fail(tr("Unable to create a temporary directory for community shader packs."));
        return;
    }

    const QString archivePath = m_temporaryDir.filePath(QStringLiteral("communityshaders.zip"));
    QFile archiveFile(archivePath);
    if (!archiveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(tr("Unable to save the downloaded community shader archive."));
        return;
    }

    const QByteArray archiveData = reply->readAll();
    if (archiveFile.write(archiveData) != archiveData.size()) {
        fail(tr("Unable to save the downloaded community shader archive."));
        return;
    }

    archiveFile.close();
    extractArchive();
}

void CommunityShadersLoader::extractArchive()
{
    emit statusChanged(tr("Extracting community shader packs..."));

    const QString archivePath = m_temporaryDir.filePath(QStringLiteral("communityshaders.zip"));
    const QString extractPath = m_temporaryDir.filePath(QStringLiteral("extracted"));
    if (!QDir().mkpath(extractPath)) {
        fail(tr("Unable to prepare the extraction directory for community shader packs."));
        return;
    }

    QZipReader zipReader(archivePath);
    if (!zipReader.exists() || !zipReader.isReadable() || zipReader.status() != QZipReader::NoError) {
        fail(tr("Unable to read the downloaded community shader archive."));
        return;
    }

    const QList<QZipReader::FileInfo> entries = zipReader.fileInfoList();
    emit progressRangeChanged(0, qMax(1, entries.size()));
    emit progressValueChanged(0);

    int currentEntry = 0;
    for (const QZipReader::FileInfo& entry : entries) {
        QString errorMessage;
        if (!writeZipEntry(zipReader, entry, extractPath, &errorMessage)) {
            fail(errorMessage);
            return;
        }

        ++currentEntry;
        emit progressValueChanged(currentEntry);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    zipReader.close();
    parseExtractedPacks();
}

void CommunityShadersLoader::parseExtractedPacks()
{
    emit statusChanged(tr("Loading community shader packs..."));

    const QString rootPath = findArchiveRootPath(m_temporaryDir.filePath(QStringLiteral("extracted")));
    QDir rootDir(rootPath);
    const QFileInfoList packDirs = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                         QDir::Name | QDir::IgnoreCase);

    emit progressRangeChanged(0, qMax(1, packDirs.size()));
    emit progressValueChanged(0);

    QList<CommunityShaderPack> packs;
    int currentPack = 0;
    for (const QFileInfo& dirInfo : packDirs) {
        CommunityShaderPack pack;
        if (loadPackFromDirectory(dirInfo, m_locale, m_releaseInfo.tag, &pack)) {
            packs.append(pack);
        }

        ++currentPack;
        emit progressValueChanged(currentPack);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    if (packs.isEmpty()) {
        fail(tr("No valid community shader packs were found in the downloaded release."));
        return;
    }

    m_packs = packs;
    emit statusChanged(tr("Loaded %1 community shader packs from %2.").arg(m_packs.size()).arg(m_releaseInfo.tag));
    emit progressRangeChanged(0, qMax(1, m_packs.size()));
    emit progressValueChanged(qMax(1, m_packs.size()));
    emit loadingFinished();
}

void CommunityShadersLoader::fail(const QString& message)
{
    m_errorString = message;
    emit errorOccurred(message);
}
