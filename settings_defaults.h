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
#define CFG_BASE_TEXTURE "base_texture"
#define CFG_MATERIAL_TYPE "material"
#define CFG_LIGHT_BRIGHTNESS "light_brightness"
#define CFG_ROUGHNESS "roughness"
#define CFG_METALNESS "metalness"
#define CFG_REFRACTION_TYPE "refraction_type"
#define CFG_REFRACTION "refraction_value"
#define CFG_GLOSSINESS "glossiness"
#define CFG_CUSTOM_VERT "custom_vert"
#define CFG_CUSTOM_FRAG "custom_frag"

#define CFG_COLORS_SPECULAR "colors/specular"
#define CFG_COLORS_AMBIENT "colors/ambient"
#define CFG_COLORS_LIGHT "colors/light"
#define CFG_ITERATION_TIME "iteration_time"
#define CFG_EXCLUDE_CHAT "exclude_chat"
#define CFG_DECORATION_PATH "decoration"
#define CFG_CLIENT_ID "client_id"
#define CFG_TOKEN "token"
#define CFG_EXPIRY_TOKEN "expiry_token"
#define CFG_EMOJI_FONT "emoji_font"
#define CFG_LANGUAGE "language"
#define CFG_PROFILES "profiles"
#define CFG_PROFILE_NAME "profile_name"
#define CFG_CURRENT_PROFILE "current_profile"

#define CFG_LOG_COLUMNS "log/columns"
#define CFG_LOG_COLOR_PREFIX "log/colors/"
#define CFG_LOG_HIDE_CMDS "log/hide"
#define DEFAULT_LOG_COLUMNS (QStringList() << "Direction" << "Timestamp" << "Command" << "Sender" << "Message" << "Tags" << "Emotes")

#define DEFAULT_LOG_COMMANDS (QStringList() << "001" << "002" << "003" << "004" << "353" << "366" << "372" << "375" << "376" \
                                       << "CAP" << "CLEARCHAT" << "CLEARMSG" << "GLOBALUSERSTATE" << "JOIN" << "NICK" << "NOTICE" \
                                       << "PART" << "PASS" << "PING" << "PONG" << "PRIVMSG" << "RECONNECT" << "ROOMSTATE" \
                                       << "USERNOTICE" << "USERSTATE")
#define DEFAULT_LOG_HIDE_CMDS (QStringList() << "PING" << "PONG")

#define DEFAULT_VERSION 1 // So we can migrate from v1
#define DEFAULT_ROUGHNESS 0
#define DEFAULT_METALNESS 0
#define DEFAULT_REFRACTION_TYPE IndexOfRefraction::Custom
#define DEFAULT_REFRACTION 150

#define DEFAULT_MATERIAL_TYPE MaterialType::SpecularGlossy
#define DEFAULT_COLORS_BASE "#5a4584"
#define DEFAULT_BASE_TEXTURE ""
#define DEFAULT_COLORS_SPECULAR "#5a4584"
#define DEFAULT_COLORS_AMBIENT "#000000"
#define DEFAULT_COLORS_LIGHT "#ffffff"
#define DEFAULT_LIGHT_BRIGHTNESS 100
#define DEFAULT_GLOSSINESS 40
#define DEFAULT_ITERATION_TIME 3
#define DEFAULT_CUSTOM_VERT "VARYING vec3 pos;\n" \
"void MAIN()\n" \
"{\n" \
"    pos = VERTEX;\n" \
"    pos.x += sin(1.0 * 4.0 + pos.y) * 1.0;\n" \
"    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);\n" \
"}\n"
#define DEFAULT_CUSTOM_FRAG "VARYING vec3 pos;\n" \
"void MAIN()\n" \
"{\n" \
"    BASE_COLOR = vec4(vec3(pos.x * 0.02, pos.y * 0.02, pos.z * 0.02), 1.0);\n" \
"    FRAGCOLOR = vec4(vec3(pos.x * 0.02, pos.y * 0.02, pos.z * 0.02), 1.0);\n" \
"}\n"
#define DEFAULT_DECORATION_PATH ":/decoration/kata_deco.png"
#define DEFAULT_CURRENT_PROFILE 0
#define DEFAULT_EXCLUDE_CHAT (QStringList() << "nightbot" << "sery_bot" << "streamelements" << "fossabot" << "wizebot" << "botisimo" << "moobot")

// This client id is set as public, but please don't use it for other purposes
// Creating a new application on Twitch dev Console is easy and free
#define DEFAULT_CLIENT_ID "3x2mzlmm9mz8qpml51m5mxc8dbdk4d"

#ifdef Q_OS_WIN
# define DEFAULT_EMOJI_FONT "Segoe UI Emoji"
#else
# define DEFAULT_EMOJI_FONT "Noto Color Emoji"
#endif

#endif // SETTINGS_DEFAULTS_H
