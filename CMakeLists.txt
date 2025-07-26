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

project(atsumari VERSION 1.3.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Quick3D WebSockets NetworkAuth Gui LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Quick3D WebSockets NetworkAuth Gui LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES pt_BR)

set(PROJECT_SOURCES
        main.cpp
        setupwidget.cpp
        setupwidget.h
        setupwidget.ui

        res.qrc
        twitchchatreader.h twitchchatreader.cpp
        twitchauthflow.h twitchauthflow.cpp
        emotewriter.h emotewriter.cpp
        emojimapper.h emojimapper.cpp

        settings_defaults.h
        atsumarilauncher.h atsumarilauncher.cpp
        localehelper.h localehelper.cpp
        profiledata.h profiledata.cpp
)

if(WIN32)
    enable_language(RC)
    set(PROJECT_SOURCES
        ${PROJECT_SOURCES}
        appicon/appicon.rc
        3rdparty/breeze-icons.qrc
    )
endif()

qt_add_translations(atsumari
    TS_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/i18n
    NO_GENERATE_PLURALS_TS_FILE
)

qt_add_executable(atsumari
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    indexofrefraction.h
    materialtype.h
)

target_link_libraries(atsumari PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Quick3D
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::NetworkAuth
    Qt${QT_VERSION_MAJOR}::Gui
)

set_target_properties(atsumari PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_generate_deploy_app_script(
    TARGET atsumari
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(TARGETS atsumari
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(SCRIPT ${deploy_script})

qt_finalize_executable(atsumari)
