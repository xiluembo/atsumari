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

#include <QApplication>
#include <QResource>
#include <QStyleHints>
#include <QString>
#include <QDir>

#include "setupwidget.h"
#include "localehelper.h"

int main(int argc, char *argv[])
{
    qputenv("QT3D_RENDERER", "opengl");
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Push X!");
    QApplication::setApplicationName("Atsumari");
    app.setQuitOnLastWindowClosed(false);

    LocaleHelper::loadBestTranslation();

#ifdef Q_OS_WIN
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    if ( scheme == Qt::ColorScheme::Dark ) {
        QResource::registerResource(":/3rdparty/breeze-icons-dark.rcc", "/icons/breeze-dark");
    } else {
        QResource::registerResource(":/3rdparty/breeze-icons.rcc", "/icons/breeze");
    }
#endif

    SetupWidget* setupw = new SetupWidget();
    setupw->show();

    return app.exec();
}

