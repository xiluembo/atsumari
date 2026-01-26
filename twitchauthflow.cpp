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
#include <QInputDialog>

#include "settings_defaults.h"

namespace {
const QStringList kTwitchScopes = {"user:read:email", "chat:read", "chat:edit"};

QString extractTokenFromUrl(const QUrl &url, int *expiresIn)
{
    if (expiresIn) {
        *expiresIn = 0;
    }

    QUrlQuery fragmentQuery(url.fragment());
    QString token = fragmentQuery.queryItemValue("access_token");
    QString expiresValue = fragmentQuery.queryItemValue("expires_in");

    if (token.isEmpty()) {
        QUrlQuery query(url.query());
        token = query.queryItemValue("access_token");
        if (expiresValue.isEmpty()) {
            expiresValue = query.queryItemValue("expires_in");
        }
    }

    bool ok = false;
    int parsedExpires = expiresValue.toInt(&ok);
    if (ok && expiresIn) {
        *expiresIn = parsedExpires;
    }

    return token;
}
} // namespace

TwitchAuthFlow::TwitchAuthFlow(QObject *parent)
    : QObject{parent}
    , m_token(QString())
    , m_authInProgress(false)
{
    QSettings settings(this);
    m_token = settings.value(CFG_TOKEN, "").toString();
    
    if (m_token.isEmpty()) {
        setupImplicitFlow();
    } else {
        QDateTime dtNow = QDateTime::currentDateTime();
        QDateTime expiryDate = settings.value(CFG_EXPIRY_TOKEN, dtNow).toDateTime();
        
        if (expiryDate < dtNow) {
            setupImplicitFlow();
        } else {
            requestTokenValidation();
        }
    }

    connect(this, &TwitchAuthFlow::tokenFetched, this, &TwitchAuthFlow::requestTokenValidation);
    connect(this, &TwitchAuthFlow::tokenValidated, this, &TwitchAuthFlow::requestUserinfo);
}

TwitchAuthFlow::~TwitchAuthFlow()
{
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
                setupImplicitFlow();
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
        setupImplicitFlow();
    }

    if (reply) reply->deleteLater();
}

void TwitchAuthFlow::setupImplicitFlow()
{
    if (m_authInProgress) {
        return;
    }
    
    QSettings settings;
    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();
    
    if (clientId.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Twitch Client ID is not configured. Please set it in the settings."));
        return;
    }

    const QUrl redirectUrl("http://localhost");

    QUrl authUrl("https://id.twitch.tv/oauth2/authorize");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("redirect_uri", redirectUrl.toString());
    query.addQueryItem("response_type", "token");
    query.addQueryItem("scope", kTwitchScopes.join(' '));
    authUrl.setQuery(query);

    m_authInProgress = true;
    QDesktopServices::openUrl(authUrl);

    QString message = tr("A browser window has been opened to authorize Twitch.\n\n"
                         "After approving access, you will be redirected to:\n%1\n\n"
                         "Copy the full redirect URL from your browser and paste it below.")
                          .arg(redirectUrl.toString());

    bool ok = false;
    QString redirectResponse = QInputDialog::getText(
        nullptr,
        tr("Twitch Authentication"),
        message,
        QLineEdit::Normal,
        QString(),
        &ok);

    if (!ok || redirectResponse.trimmed().isEmpty()) {
        m_authInProgress = false;
        return;
    }

    int expiresIn = 0;
    QUrl redirectedUrl = QUrl::fromUserInput(redirectResponse.trimmed());
    QString token = extractTokenFromUrl(redirectedUrl, &expiresIn);

    if (token.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Error"),
                              tr("Authentication failed. Please ensure you pasted the full redirect URL."));
        m_authInProgress = false;
        return;
    }

    m_token = token;
    settings.setValue(CFG_TOKEN, m_token);
    if (expiresIn > 0) {
        QDateTime expiryDate = QDateTime::currentDateTime().addSecs(expiresIn);
        settings.setValue(CFG_EXPIRY_TOKEN, expiryDate);
    }

    m_authInProgress = false;
    emit tokenFetched();

    QMessageBox::information(nullptr, tr("Success"), tr("Authentication completed successfully!"));
}

QString TwitchAuthFlow::token() const
{
    return m_token;
}
