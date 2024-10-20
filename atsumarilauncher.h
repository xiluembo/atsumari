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

#include <Qt3DExtras/Qt3DWindow>
#include <QMainWindow>

#include "twitchauthflow.h"
#include "twitchchatreader.h"
#include "emotewriter.h"

class AtsumariLauncher
{
public:
    AtsumariLauncher();
    ~AtsumariLauncher();
    AtsumariLauncher(const AtsumariLauncher&) = delete;
    AtsumariLauncher& operator=(const AtsumariLauncher&) = delete;
    void launch();

private:
    TwitchAuthFlow* m_twFlow;
    EmoteWriter* m_emw;
    TwitchChatReader* m_tReader;
    Qt3DExtras::Qt3DWindow m_window;
    QMainWindow* m_mw = new QMainWindow;
    QWidget* m_container = new QWidget;
};

#endif // ATSUMARILAUNCHER_H
