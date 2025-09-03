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

#include "profiledata.h"

ProfileData::ProfileData() {}

QString ProfileData::profileName() const
{
    return m_profileName;
}

void ProfileData::setProfileName(const QString &newProfileName)
{
    m_profileName = newProfileName;
}

QString ProfileData::baseColor() const
{
    return m_baseColor;
}

void ProfileData::setBaseColor(const QString &newBaseColor)
{
    m_baseColor = newBaseColor;
}

QString ProfileData::baseTexture() const
{
    return m_baseTexture;
}

void ProfileData::setBaseTexture(const QString &newBaseTexture)
{
    m_baseTexture = newBaseTexture;
}

QString ProfileData::specularColor() const
{
    return m_specularColor;
}

void ProfileData::setSpecularColor(const QString &newSpecularColor)
{
    m_specularColor = newSpecularColor;
}

QString ProfileData::ambientColor() const
{
    return m_ambientColor;
}

void ProfileData::setAmbientColor(const QString &newAmbientColor)
{
    m_ambientColor = newAmbientColor;
}

QString ProfileData::lightColor() const
{
    return m_lightColor;
}

void ProfileData::setLightColor(const QString &newLightColor)
{
    m_lightColor = newLightColor;
}

QString ProfileData::decorationPath() const
{
    return m_decorationPath;
}

void ProfileData::setDecorationPath(const QString &newDecorationPath)
{
    m_decorationPath = newDecorationPath;
}

int ProfileData::lightBrightness() const
{
    return m_lightBrightness;
}

void ProfileData::setLightBrightness(int newLightBrightness)
{
    m_lightBrightness = newLightBrightness;
}

int ProfileData::glossiness() const
{
    return m_glossiness;
}

void ProfileData::setGlossiness(int newGlossiness)
{
    m_glossiness = newGlossiness;
}

int ProfileData::roughness() const
{
    return m_roughness;
}

void ProfileData::setRoughness(int newRoughness)
{
    m_roughness = newRoughness;
}

int ProfileData::metalness() const
{
    return m_metalness;
}

void ProfileData::setMetalness(int newMetalness)
{
    m_metalness = newMetalness;
}

double ProfileData::iteration() const
{
    return m_iteration;
}

void ProfileData::setIteration(double newIteration)
{
    m_iteration = newIteration;
}

QString ProfileData::font() const
{
    return m_font;
}

void ProfileData::setFont(const QString &newFont)
{
    m_font = newFont;
}

int ProfileData::refraction() const
{
    return m_refraction;
}

void ProfileData::setRefraction(int newRefraction)
{
    m_refraction = newRefraction;
}

IndexOfRefraction ProfileData::indexOfRefractionType() const
{
    return m_indexOfRefractionType;
}

void ProfileData::setIndexOfRefractionType(IndexOfRefraction newIndexOfRefractionType)
{
    m_indexOfRefractionType = newIndexOfRefractionType;
}

MaterialType ProfileData::materialType() const
{
    return m_materialType;
}

void ProfileData::setMaterialType(MaterialType newMaterialType)
{
    m_materialType = newMaterialType;
}

QString ProfileData::customVertexShader() const
{
    return m_customVertexShader;
}

void ProfileData::setCustomVertexShader(const QString &code)
{
    m_customVertexShader = code;
}

QString ProfileData::customFragmentShader() const
{
    return m_customFragmentShader;
}

void ProfileData::setCustomFragmentShader(const QString &code)
{
    m_customFragmentShader = code;
}
