# Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
# Copyright (C) 2024 - Andrius da Costa Ribas <andriusmao@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)

project(Atsumari VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets 3DCore 3DExtras WebSockets WebEngineWidgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets 3DCore 3DExtras WebSockets WebEngineWidgets Gui)

set(PROJECT_SOURCES
        main.cpp
        setupwidget.cpp
        setupwidget.h
        setupwidget.ui
        atsumari.h atsumari.cpp
        res.qrc
        twitchchatreader.h twitchchatreader.cpp
        twitchauthflow.h twitchauthflow.cpp
        emotewriter.h emotewriter.cpp
        emojimapper.h emojimapper.cpp
)

qt_add_executable(Atsumari
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}

    settings_defaults.h
    atsumarilauncher.h atsumarilauncher.cpp
)

target_link_libraries(Atsumari PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DExtras
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::Gui
)

set_target_properties(Atsumari PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Atsumari
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(Atsumari)
