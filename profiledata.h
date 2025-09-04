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

#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include <QString>

#include "indexofrefraction.h"
#include "materialtype.h"

class ProfileData {
public:
    ProfileData();

    QString profileName() const;
    void setProfileName(const QString &newProfileName);
    QString baseColor() const;
    void setBaseColor(const QString &newBaseColor);
    QString baseTexture() const;
    void setBaseTexture(const QString &newBaseTexture);
    QString specularColor() const;
    void setSpecularColor(const QString &newSpecularColor);
    QString ambientColor() const;
    void setAmbientColor(const QString &newAmbientColor);
    QString lightColor() const;
    void setLightColor(const QString &newLightColor);
    QString decorationPath() const;
    void setDecorationPath(const QString &newDecorationPath);
    int lightBrightness() const;
    void setLightBrightness(int newLightBrightness);
    int glossiness() const;
    void setGlossiness(int newShininess);
    int roughness() const;
    void setRoughness(int newRoughness);
    int metalness() const;
    void setMetalness(int newMetalness);
    double iteration() const;
    void setIteration(double newIteration);
    QString font() const;
    void setFont(const QString &newFont);

    int refraction() const;
    void setRefraction(int newRefraction);

    IndexOfRefraction indexOfRefractionType() const;
    void setIndexOfRefractionType(IndexOfRefraction newIndexOfRefractionType);

    MaterialType materialType() const;
    void setMaterialType(MaterialType newMaterialType);

    QString customVertexShader() const;
    void setCustomVertexShader(const QString &code);
    QString customFragmentShader() const;
    void setCustomFragmentShader(const QString &code);

private:
    QString m_profileName;
    QString m_baseColor;
    QString m_baseTexture;
    QString m_ambientColor;
    QString m_lightColor;
    MaterialType m_materialType;

    // Principled Material properties
    int m_roughness;
    int m_metalness;
    IndexOfRefraction m_indexOfRefractionType;
    int m_refraction;

    // Specular Glossy Material properties
    QString m_specularColor;
    int m_glossiness;

    int m_lightBrightness;
    double m_iteration;
    QString m_decorationPath;
    QString m_font;
    QString m_customVertexShader;
    QString m_customFragmentShader;
};

#endif // PROFILEDATA_H
