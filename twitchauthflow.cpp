/*
 * Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
 * Copyright (C) 2024 - Andrius da Costa Ribas <andriusmao@gmail.com>
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

#include "twitchauthflow.h"

#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>

#include "settings_defaults.h"

namespace {
const QStringList kTwitchScopes = {"user:read:email", "chat:read", "chat:edit"};
} // namespace

TwitchAuthFlow::TwitchAuthFlow(QObject *parent)
    : QObject{parent}
    , m_token(QString())
    , m_authInProgress(false)
    , m_oauthFlow(nullptr)
    , m_replyHandler(nullptr)
{
    QSettings settings(this);
    m_token = settings.value(CFG_TOKEN, "").toString();
    
    if (m_token.isEmpty()) {
        setupAuthorizationCodeFlow();
    } else {
        QDateTime dtNow = QDateTime::currentDateTime();
        QDateTime expiryDate = settings.value(CFG_EXPIRY_TOKEN, dtNow).toDateTime();
        
        if (expiryDate < dtNow) {
            setupAuthorizationCodeFlow();
        } else {
            requestTokenValidation();
        }
    }

    connect(this, &TwitchAuthFlow::tokenFetched, this, &TwitchAuthFlow::requestTokenValidation);
    connect(this, &TwitchAuthFlow::tokenValidated, this, &TwitchAuthFlow::requestUserinfo);
}

TwitchAuthFlow::~TwitchAuthFlow()
{
    if (m_oauthFlow) {
        m_oauthFlow->disconnect();
        m_oauthFlow->deleteLater();
        m_oauthFlow = nullptr;
    }

    if (m_replyHandler) {
        m_replyHandler->deleteLater();
        m_replyHandler = nullptr;
    }
}

void TwitchAuthFlow::requestTokenValidation()
{
    QSettings settings;
    QUrl requestUrl("https://id.twitch.tv/oauth2/validate");

    QNetworkRequest req((QUrl(requestUrl)));
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    req.setRawHeader("Client-Id", settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString().toUtf8());

    QNetworkReply *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        onValidateReply(reply);
    });
}
void TwitchAuthFlow::requestUserinfo()
{
    QSettings settings;
    QUrl requestUrl("https://api.twitch.tv/helix/users");

    QNetworkRequest req((QUrl(requestUrl)));
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    req.setRawHeader("Client-Id", settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString().toUtf8());

    QNetworkReply *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        onUserinfoReply(reply);
    });
}

void TwitchAuthFlow::onUserinfoReply(QNetworkReply *reply)
{
    if (reply && reply->error() == QNetworkReply::NoError) {
        // Read JSON response
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.contains("data")) {
                QJsonArray arr = jsonObj.value("data").toArray();
                if (! arr.isEmpty()) {
                    if (arr.at(0).toObject().contains("login")) {
                        QString login = arr.at(0).toObject().value("login").toString();
                        emit loginFetched(login);
                    }
                }
            }
        }
    }

    if (reply) reply->deleteLater();
}

void TwitchAuthFlow::onValidateReply(QNetworkReply *reply)
{
    if (reply && reply->error() == QNetworkReply::NoError) {
        // Read JSON response
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.value("status").toInt(200) != 200) {
                setupAuthorizationCodeFlow();
            } else {
                int expirySecs = jsonObj.value("expires_in").toInt();
                QDateTime dtNow = QDateTime::currentDateTime();
                QDateTime expiryDate = dtNow.addSecs(expirySecs);

                QSettings settings(this);
                settings.setValue(CFG_EXPIRY_TOKEN, expiryDate);
                emit tokenValidated();
            }
        }
    } else {
        setupAuthorizationCodeFlow();
    }

    if (reply) reply->deleteLater();
}

void TwitchAuthFlow::setupAuthorizationCodeFlow()
{
    if (m_authInProgress) {
        return;
    }
    
    if (m_oauthFlow) {
        m_oauthFlow->disconnect();
        m_oauthFlow->deleteLater();
        m_oauthFlow = nullptr;
    }

    if (m_replyHandler) {
        m_replyHandler->deleteLater();
        m_replyHandler = nullptr;
    }

    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();
    
    if (clientId.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Twitch Client ID is not configured. Please set it in the settings."));
        return;
    }

    m_replyHandler = new QOAuthHttpServerReplyHandler(this);
    m_oauthFlow = new QOAuth2AuthorizationCodeFlow(&m_nam, this);
    m_oauthFlow->setAuthorizationUrl(QUrl("https://id.twitch.tv/oauth2/authorize"));
    m_oauthFlow->setAccessTokenUrl(QUrl("https://id.twitch.tv/oauth2/token"));
    m_oauthFlow->setClientIdentifier(clientId);
    m_oauthFlow->setScope(kTwitchScopes.join(' '));
    m_oauthFlow->setReplyHandler(m_replyHandler);

    connect(m_oauthFlow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
            [](const QUrl &url) {
                QDesktopServices::openUrl(url);
            });

    connect(m_oauthFlow, &QAbstractOAuth::granted, this, [this]() {
        m_token = m_oauthFlow->token();
        QSettings settings;
        settings.setValue(CFG_TOKEN, m_token);
        m_authInProgress = false;
        emit tokenFetched();
        QMessageBox::information(nullptr, tr("Success"), tr("Authentication completed successfully!"));
    });

    connect(m_oauthFlow, &QAbstractOAuth::requestFailed, this, [this](QAbstractOAuth::Error) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Authentication failed. Please try again."));
        m_authInProgress = false;
    });

    connect(m_oauthFlow, &QAbstractOAuth2::serverReportedErrorOccurred, this,
            [this](const QString &error, const QString &errorDescription, const QUrl &) {
                QString message = tr("Authentication error: %1").arg(error);
                if (!errorDescription.isEmpty()) {
                    message += QString(" - %1").arg(errorDescription);
                }
                QMessageBox::critical(nullptr, tr("Error"), message);
                m_authInProgress = false;
            });

    m_authInProgress = true;
    m_oauthFlow->grant();
}

QString TwitchAuthFlow::token() const
{
    return m_token;
}
