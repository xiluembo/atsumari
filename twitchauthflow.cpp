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

TwitchAuthFlow::TwitchAuthFlow(QObject *parent)
    : QObject{parent}
    , m_token(QString())
    , m_refreshToken(QString())
    , m_authInProgress(false)
    , m_deviceFlow(nullptr)
    , m_pollTimer(nullptr)
    , m_retryCount(0)
{
    QSettings settings(this);
    m_token = settings.value(CFG_TOKEN, "").toString();
    m_refreshToken = settings.value(CFG_REFRESH_TOKEN, "").toString();
    
    if (m_token.isEmpty()) {
        setupDeviceFlow();
    } else {
        QDateTime dtNow = QDateTime::currentDateTime();
        QDateTime expiryDate = settings.value(CFG_EXPIRY_TOKEN, dtNow).toDateTime();
        
        if (expiryDate < dtNow) {
            if (!m_refreshToken.isEmpty()) {
                refreshAccessToken();
            } else {
                setupDeviceFlow();
            }
        } else {
            requestTokenValidation();
        }
    }

    connect(this, &TwitchAuthFlow::tokenFetched, this, &TwitchAuthFlow::requestTokenValidation);
    connect(this, &TwitchAuthFlow::tokenValidated, this, &TwitchAuthFlow::requestUserinfo);
}

TwitchAuthFlow::~TwitchAuthFlow()
{
    // Cleanup timer
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->deleteLater();
        m_pollTimer = nullptr;
    }
    
    // Cleanup device flow
    if (m_deviceFlow) {
        m_deviceFlow->disconnect();
        m_deviceFlow->deleteLater();
        m_deviceFlow = nullptr;
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
                if (!m_refreshToken.isEmpty()) {
                    refreshAccessToken();
                } else {
                    setupDeviceFlow();
                }
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
        if (!m_refreshToken.isEmpty()) {
            refreshAccessToken();
        } else {
            setupDeviceFlow();
        }
    }

    if (reply) reply->deleteLater();
}

void TwitchAuthFlow::setupDeviceFlow()
{
    if (m_authInProgress) {
        return;
    }
    
    // Cleanup existing device flow
    if (m_deviceFlow) {
        m_deviceFlow->disconnect();
        m_deviceFlow->deleteLater();
        m_deviceFlow = nullptr;
    }
    
    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();
    
    if (clientId.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Twitch Client ID is not configured. Please set it in the settings."));
        return;
    }
    
    // Create device authorization flow
    m_deviceFlow = new QOAuth2DeviceAuthorizationFlow(&m_nam, this);
    
    // Configure for Twitch
    m_deviceFlow->setAuthorizationUrl(QUrl("https://id.twitch.tv/oauth2/device"));
    m_deviceFlow->setTokenUrl(QUrl("https://id.twitch.tv/oauth2/token"));
    m_deviceFlow->setClientIdentifier(clientId);
    m_deviceFlow->setRequestedScopeTokens({"user:read:email", "chat:read", "chat:edit"});
    

    
    // Connect signals
    connect(m_deviceFlow, &QOAuth2DeviceAuthorizationFlow::authorizeWithUserCode, this,
        [this](const QUrl &verificationUrl, const QString &userCode, const QUrl &completeVerificationUrl) {
            // Store device code for manual polling
            m_deviceCode = m_deviceFlow->property("device_code").toString();
            
            // Stop Qt's automatic polling
            m_deviceFlow->stopTokenPolling();
            
            // Open the verification URL in browser first
            QDesktopServices::openUrl(verificationUrl);
            
            // Show message asking user to complete authentication and click OK
            QString message = tr("A browser window has been opened with the authentication page.\n\n"
                               "Please complete the authentication in your browser and then click OK to continue.");
            
            QMessageBox::information(nullptr, tr("Twitch Authentication"), message);
            
            // Start polling only after user confirms they completed authentication
            startPollingAfterUserConfirmation();
        });
    
    connect(m_deviceFlow, &QAbstractOAuth::granted, this, [this]() {
        m_token = m_deviceFlow->token();
        QSettings settings;
        settings.setValue(CFG_TOKEN, m_token);
        m_authInProgress = false;
        emit tokenFetched();
        QMessageBox::information(nullptr, tr("Success"), tr("Authentication completed successfully!"));
    });
    
    connect(m_deviceFlow, &QAbstractOAuth::requestFailed, this, [this](QAbstractOAuth::Error error) {
        // Don't show error message immediately - let manual polling handle it
        if (m_deviceCode.isEmpty()) {
            QMessageBox::critical(nullptr, tr("Error"), tr("Authentication failed. Please try again."));
            m_authInProgress = false;
        }
    });
    
    connect(m_deviceFlow, &QAbstractOAuth2::serverReportedErrorOccurred, this,
        [this](const QString &error, const QString &errorDescription, const QUrl &uri) {
            // Don't show error message immediately - let manual polling handle it
            if (m_deviceCode.isEmpty()) {
                QString message = tr("Authentication error: %1").arg(error);
                if (!errorDescription.isEmpty()) {
                    message += QString(" - %1").arg(errorDescription);
                }
                QMessageBox::critical(nullptr, tr("Error"), message);
                m_authInProgress = false;
            }
        });
    
    m_authInProgress = true;
    m_deviceFlow->grant();
}

void TwitchAuthFlow::refreshAccessToken()
{
    if (m_authInProgress) {
        return;
    }

    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();
    QString refreshToken = settings.value(CFG_REFRESH_TOKEN, "").toString();

    if (clientId.isEmpty() || refreshToken.isEmpty()) {
        setupDeviceFlow();
        return;
    }

    m_authInProgress = true;

    QUrl url("https://id.twitch.tv/oauth2/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("grant_type", "refresh_token");

    QNetworkReply *reply = m_nam.post(request, query.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            m_authInProgress = false;
            setupDeviceFlow();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull()) {
            m_authInProgress = false;
            setupDeviceFlow();
            return;
        }

        if (!applyTokenResponse(doc, false)) {
            setupDeviceFlow();
        }
    });
}

void TwitchAuthFlow::startPollingAfterUserConfirmation()
{
    if (m_deviceCode.isEmpty()) {
        return;
    }
    
    // Reset retry count for new authentication attempt
    m_retryCount = 0;
    
    // Create timer for polling
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->deleteLater();
    }
    
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &TwitchAuthFlow::pollForToken);
    m_pollTimer->start(3000); // Poll every 3 seconds for faster response
}

void TwitchAuthFlow::pollForToken()
{
    if (m_deviceCode.isEmpty()) {
        return;
    }
    
    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();
    
    // Create POST request to exchange device code for token
    QUrl url("https://id.twitch.tv/oauth2/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("scopes", "user:read:email chat:read chat:edit");
    query.addQueryItem("device_code", m_deviceCode);
    query.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");
    
    QNetworkReply* reply = m_nam.post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            return; // Continue polling
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull()) {
            return; // Continue polling
        }
        
        handleTokenResponse(doc);
    });
}

void TwitchAuthFlow::handleTokenResponse(const QJsonDocument& response)
{
    QJsonObject obj = response.object();
    
    if (obj.contains("error")) {
        QString error = obj["error"].toString();
        QString message = obj["message"].toString();
        
        if (error == "authorization_pending") {
            m_retryCount++;
            
            // If we've been polling for too long, ask user if they want to retry
            if (m_retryCount >= MAX_RETRIES) {
                QMessageBox::StandardButton reply = QMessageBox::question(nullptr, 
                    tr("Authentication Pending"), 
                    tr("Authentication is still pending. Have you completed the authentication in your browser?\n\n"
                       "Click 'Yes' to continue polling or 'No' to retry the authentication."),
                    QMessageBox::Yes | QMessageBox::No);
                
                if (reply == QMessageBox::Yes) {
                    // Reset retry count and continue polling
                    m_retryCount = 0;
                    return;
                } else {
                    // Retry authentication
                    retryAuthentication();
                    return;
                }
            }
            return; // Continue polling
        } else if (error == "invalid_device_code") {
            QMessageBox::critical(nullptr, tr("Error"), tr("Device code expired. Please try again."));
            m_authInProgress = false;
            if (m_pollTimer) {
                m_pollTimer->stop();
                m_pollTimer->deleteLater();
                m_pollTimer = nullptr;
            }
            return;
        } else {
            QMessageBox::critical(nullptr, tr("Error"), tr("Authentication failed: %1").arg(message));
            m_authInProgress = false;
            if (m_pollTimer) {
                m_pollTimer->stop();
                m_pollTimer->deleteLater();
                m_pollTimer = nullptr;
            }
            return;
        }
    }
    
    // Stop polling
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->deleteLater();
        m_pollTimer = nullptr;
    }
    
    applyTokenResponse(response, true);
}

bool TwitchAuthFlow::applyTokenResponse(const QJsonDocument& response, bool showSuccessMessage)
{
    QJsonObject obj = response.object();

    if (obj.contains("error")) {
        m_authInProgress = false;
        return false;
    }

    m_token = obj["access_token"].toString();
    if (m_token.isEmpty()) {
        m_authInProgress = false;
        return false;
    }

    QString newRefreshToken = obj["refresh_token"].toString();
    if (!newRefreshToken.isEmpty()) {
        m_refreshToken = newRefreshToken;
    }

    int expirySecs = obj["expires_in"].toInt(0);
    QDateTime dtNow = QDateTime::currentDateTime();
    QDateTime expiryDate = dtNow.addSecs(expirySecs);

    QSettings settings;
    settings.setValue(CFG_TOKEN, m_token);
    if (!m_refreshToken.isEmpty()) {
        settings.setValue(CFG_REFRESH_TOKEN, m_refreshToken);
    }
    if (expirySecs > 0) {
        settings.setValue(CFG_EXPIRY_TOKEN, expiryDate);
    }

    m_authInProgress = false;
    emit tokenFetched();

    if (showSuccessMessage) {
        QMessageBox::information(nullptr, tr("Success"), tr("Authentication completed successfully!"));
    }

    return true;
}

void TwitchAuthFlow::retryAuthentication()
{
    // Stop current polling
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->deleteLater();
        m_pollTimer = nullptr;
    }
    
    // Reset device code
    m_deviceCode.clear();
    
    // Restart device flow
    setupDeviceFlow();
}

QString TwitchAuthFlow::token() const
{
    return m_token;
}
