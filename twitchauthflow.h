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

#ifndef TWITCHAUTHFLOW_H
#define TWITCHAUTHFLOW_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtNetworkAuth/qoauth2deviceauthorizationflow.h>



class TwitchAuthFlow : public QObject {
    Q_OBJECT
public:
    explicit TwitchAuthFlow(const QString& state, QObject *parent = nullptr);
    ~TwitchAuthFlow();
    QString token() const;

signals:
    void tokenFetched();
    void tokenValidated();
    void loginFetched(QString login);

private:
    void parseToken(QUrl newUrl);
    void requestUserinfo();
    void onUserinfoReply(QNetworkReply* reply);
    void requestTokenValidation();
    void onValidateReply(QNetworkReply* reply);

    void setupDeviceFlow();
    void startPollingAfterUserConfirmation();
    void pollForToken();
    void handleTokenResponse(const QJsonDocument& response);
    void retryAuthentication();

    QNetworkAccessManager m_nam;
    QString m_token;
    bool m_authInProgress;
    QOAuth2DeviceAuthorizationFlow* m_deviceFlow;
    QString m_deviceCode;
    QTimer* m_pollTimer;
    int m_retryCount;
    static const int MAX_RETRIES = 3;

};

#endif // TWITCHAUTHFLOW_H
