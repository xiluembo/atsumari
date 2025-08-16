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
#ifndef SETTINGS_DEFAULTS_H
#define SETTINGS_DEFAULTS_H

#include <QtGlobal>


#define CFG_VERSION "version"

#define CFG_V1_COLORS_DIFFUSE "colors/diffuse"
#define CFG_V1_LIGHT_INTENSITY "light_intensity"
#define CFG_V1_SLICES "slices"
#define CFG_V1_RINGS "rings"
#define CFG_V1_SHININESS "shininess"

// v2
#define CFG_COLORS_BASE "colors/base"
#define CFG_MATERIAL_TYPE "material"
#define CFG_LIGHT_BRIGHTNESS "light_brightness"
#define CFG_ROUGHNESS "roughness"
#define CFG_METALNESS "metalness"
#define CFG_REFRACTION_TYPE "refraction_type"
#define CFG_REFRACTION "refraction_value"
#define CFG_GLOSSINESS "glossiness"

#define CFG_COLORS_SPECULAR "colors/specular"
#define CFG_COLORS_AMBIENT "colors/ambient"
#define CFG_COLORS_LIGHT "colors/light"
#define CFG_ITERATION_TIME "iteration_time"
#define CFG_EXCLUDE_CHAT "exclude_chat"
#define CFG_DECORATION_PATH "decoration"
#define CFG_CLIENT_ID "client_id"
#define CFG_TOKEN "token"
#define CFG_EXPIRY_TOKEN "expiry_token"
#define CFG_EMOJI_DIR "dirs/emoji"
#define CFG_EMOTE_DIR "dirs/emotes"
#define CFG_EMOJI_FONT "emoji_font"
#define CFG_LANGUAGE "language"
#define CFG_PROFILES "profiles"
#define CFG_PROFILE_NAME "profile_name"
#define CFG_CURRENT_PROFILE "current_profile"

#define CFG_LOG_COLUMNS "log/columns"
#define CFG_LOG_COLOR_PREFIX "log/colors/"
#define DEFAULT_LOG_COLUMNS (QStringList() << "Timestamp" << "Command" << "Badges" << "Sender" << "Message" << "Tags" << "Emotes")

#define DEFAULT_VERSION 1 // So we can migrate from v1
#define DEFAULT_ROUGHNESS 0
#define DEFAULT_METALNESS 0
#define DEFAULT_REFRACTION_TYPE IndexOfRefraction::Custom
#define DEFAULT_REFRACTION 150

#define DEFAULT_MATERIAL_TYPE MaterialType::SpecularGlossy
#define DEFAULT_COLORS_BASE "#5a4584"
#define DEFAULT_COLORS_SPECULAR "#5a4584"
#define DEFAULT_COLORS_AMBIENT "#000000"
#define DEFAULT_COLORS_LIGHT "#ffffff"
#define DEFAULT_LIGHT_BRIGHTNESS 100
#define DEFAULT_GLOSSINESS 40
#define DEFAULT_ITERATION_TIME 3
#define DEFAULT_DECORATION_PATH ":/decoration/kata_deco.png"
#define DEFAULT_CURRENT_PROFILE 0
#define DEFAULT_EXCLUDE_CHAT (QStringList() << "nightbot" << "sery_bot" << "streamelements" << "fossabot" << "wizebot" << "botisimo" << "moobot")

// This client id is set as public, but please don't use it for other purposes
// Creating a new application on Twitch dev Console is easy and free
#define DEFAULT_CLIENT_ID "3x2mzlmm9mz8qpml51m5mxc8dbdk4d"

#ifdef Q_OS_WIN
# define DEFAULT_EMOJI_DIR QString("%1/%2").arg(qApp->applicationDirPath(), "emojiInfo")
# define DEFAULT_EMOTE_DIR QString("%1/%2").arg(qApp->applicationDirPath(), "emoteInfo")
# define DEFAULT_EMOJI_FONT "Segoe UI Emoji"
#else
# define DEFAULT_EMOJI_DIR QString("%1/%2").arg(qEnvironmentVariable("HOME"), ".atsumari/emojiInfo")
# define DEFAULT_EMOTE_DIR QString("%1/%2").arg(qEnvironmentVariable("HOME"), ".atsumari/emoteInfo")
# define DEFAULT_EMOJI_FONT "Noto Color Emoji"
#endif

#endif // SETTINGS_DEFAULTS_H
