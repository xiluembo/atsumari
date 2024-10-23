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
#include <QWebEngineProfile>

#include "settings_defaults.h"

TwitchAuthFlow::TwitchAuthFlow(const QString& state, QObject *parent)
    : QObject{parent}
    , m_state(state)
    , m_token(QString())
    , m_webView(nullptr)
{
    QSettings settings(this);
#ifdef Q_OS_WIN
    const QString userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36";
#else
    const QString userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0.0.0 Safari/537.36";
#endif
    QWebEngineProfile::defaultProfile()->setHttpUserAgent(userAgent);
    m_webView = new QWebEngineView();

    m_token = settings.value(CFG_TOKEN, "").toString();
    if (m_token == "") {
        startAuthFlow();
    } else {
        QDateTime dtNow = QDateTime::currentDateTime();
        QDateTime expiryDate = settings.value(CFG_EXPIRY_TOKEN, dtNow).toDateTime();
        if (expiryDate < dtNow) {
            startAuthFlow();
        } else {
            requestTokenValidation();
        }
    }

    connect(this, &TwitchAuthFlow::tokenFetched, this, &TwitchAuthFlow::requestTokenValidation);
    connect(this, &TwitchAuthFlow::tokenValidated, this, &TwitchAuthFlow::requestUserinfo);
    connect(m_webView, &QWebEngineView::loadFinished, m_webView, &QWebEngineView::show, Qt::SingleShotConnection);
    connect(m_webView, &QWebEngineView::urlChanged, this, &TwitchAuthFlow::parseToken);
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

void TwitchAuthFlow::parseToken(QUrl newUrl)
{
    if (newUrl.host() == "localhost") {
        QString fragment = newUrl.fragment();
        QStringList parms = fragment.split("&");
        foreach (const QString& kv, parms) {
            QString k = kv.split("=")[0];
            if (k == "access_token") {
                m_token = kv.split("=")[1];
                QSettings settings;
                settings.setValue(CFG_TOKEN, m_token);
                emit tokenFetched();
                m_webView->hide();
                m_webView->deleteLater();
            }
        }
    }
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
                startAuthFlow();
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
        startAuthFlow();
    }

    if (reply) reply->deleteLater();
}

void TwitchAuthFlow::startAuthFlow()
{
    QSettings settings;
    QUrl authorizeUrl("https://id.twitch.tv/oauth2/authorize");

    // Parameters
    QUrlQuery parms;
    parms.addQueryItem("client_id", settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString());
    parms.addQueryItem("scope", "user:read:email chat:read chat:edit");
    parms.addQueryItem("redirect_uri", "http://localhost:6876/");
    parms.addQueryItem("response_type", "token");
    parms.addQueryItem("state", m_state);

    authorizeUrl.setQuery(parms);

    m_webView->setWindowIcon(QIcon(":/appicon/atsumari.svg"));
    m_webView->setWindowTitle(tr("Twitch Authorization"));
    m_webView->load(authorizeUrl);
}

QString TwitchAuthFlow::token() const
{
    return m_token;
}
