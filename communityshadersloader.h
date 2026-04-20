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

#ifndef COMMUNITYSHADERSLOADER_H
#define COMMUNITYSHADERSLOADER_H

#include <QObject>
#include <QList>
#include <QLocale>
#include <QPointer>
#include <QTemporaryDir>
#include <QUrl>

class QNetworkReply;

struct CommunityShaderReleaseInfo {
    QString tag;
    QUrl zipballUrl;

    bool isValid() const
    {
        return !tag.isEmpty() && zipballUrl.isValid();
    }
};

struct CommunityShaderPack {
    QString id;
    QString releaseTag;
    QString name;
    QString description;
    QString authorName;
    QString authorEmail;
    QString authorWebsite;
    QString licenseSpdx;
    QString vertexShaderPath;
    QString fragmentShaderPath;
    QString vertexShaderSource;
    QString fragmentShaderSource;
    QString searchableText;
};

class CommunityShadersLoader : public QObject
{
    Q_OBJECT

public:
    explicit CommunityShadersLoader(const QLocale& locale, QObject *parent = nullptr);

    void setLocale(const QLocale& locale);
    void fetchLatestRelease();
    void loadRelease(const CommunityShaderReleaseInfo& release);

    const CommunityShaderReleaseInfo& releaseInfo() const;
    const QList<CommunityShaderPack>& packs() const;
    QString errorString() const;

signals:
    void latestReleaseChanged();
    void statusChanged(const QString& status);
    void progressRangeChanged(int minimum, int maximum);
    void progressValueChanged(int value);
    void loadingFinished();
    void errorOccurred(const QString& message);

private:
    void cancelCurrentReply();
    void clearLoadedState();
    void requestLatestRelease(bool continueLoadingAfterReply);
    void handleLatestReleaseReply(QNetworkReply* reply, bool continueLoadingAfterReply);
    void beginArchiveDownload(const CommunityShaderReleaseInfo& release);
    void handleArchiveReply(QNetworkReply* reply);
    void extractArchive();
    void parseExtractedPacks();
    void fail(const QString& message);

    QLocale m_locale;
    QTemporaryDir m_temporaryDir;
    CommunityShaderReleaseInfo m_releaseInfo;
    QList<CommunityShaderPack> m_packs;
    QString m_errorString;
    QPointer<QNetworkReply> m_currentReply;
};

#endif // COMMUNITYSHADERSLOADER_H
