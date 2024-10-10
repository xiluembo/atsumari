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

QString ProfileData::diffuseColor() const
{
    return m_diffuseColor;
}

void ProfileData::setDiffuseColor(const QString &newDiffuseColor)
{
    m_diffuseColor = newDiffuseColor;
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

int ProfileData::lightIntensity() const
{
    return m_lightIntensity;
}

void ProfileData::setLightIntensity(int newLightIntensity)
{
    m_lightIntensity = newLightIntensity;
}

int ProfileData::slices() const
{
    return m_slices;
}

void ProfileData::setSlices(int newSlices)
{
    m_slices = newSlices;
}

int ProfileData::rings() const
{
    return m_rings;
}

void ProfileData::setRings(int newRings)
{
    m_rings = newRings;
}

int ProfileData::shininess() const
{
    return m_shininess;
}

void ProfileData::setShininess(int newShininess)
{
    m_shininess = newShininess;
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
