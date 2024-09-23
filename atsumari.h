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

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DCore/QTransform>
#include <QTimer>
#include <QRandomGenerator>
#include <QPropertyAnimation>

class Atsumari : public Qt3DCore::QEntity {
    Q_OBJECT
public:
    Atsumari(Qt3DCore::QEntity *parent = nullptr);

    void updateRandomRotation();

    void setSlices(int slices);
    void setRings(int rings);
    void setDiffuse(QString diffuseColor);
    void setSpecular(QString specularColor);
    void setAmbient(QString ambientColor);
    void setShininess(int shininess);
    void setIterationInterval(double seconds);

public slots:
    void addEmote(const QUrl &emote, float theta = -1.0f, float phi = -1.0f, float emoteSize = 0.4f);
private:
    Qt3DCore::QTransform *m_transform;
    QPropertyAnimation *m_rotationAnimation;

    Qt3DExtras::QSphereMesh *m_sphereMesh;
    Qt3DExtras::QPhongMaterial *m_material;
};
