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

#ifndef ATSUMARILAUNCHER_H
#define ATSUMARILAUNCHER_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTemporaryFile>
#include <QEvent>

#include "twitchauthflow.h"
#include "twitchchatreader.h"
#include "emotewriter.h"
#include "logviewdialog.h"

class AtsumariLauncher : public QObject {
    Q_OBJECT
public:
    AtsumariLauncher(QObject *parent = nullptr);
    ~AtsumariLauncher();
    AtsumariLauncher(const AtsumariLauncher&) = delete;
    AtsumariLauncher& operator=(const AtsumariLauncher&) = delete;
    void launch();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void showDesktopNotification(const QString &title, const QString &message);

    TwitchAuthFlow* m_twFlow;
    EmoteWriter* m_emw;
    TwitchChatReader* m_tReader;
    QMainWindow* m_mw = new QMainWindow;
    QWidget* m_container = new QWidget;
    QSystemTrayIcon* m_tray = nullptr;
    LogViewDialog* m_logDialog = nullptr;
    QTemporaryFile* m_vertexShaderFile = nullptr;
    QTemporaryFile* m_fragmentShaderFile = nullptr;
};

#endif // ATSUMARILAUNCHER_H
