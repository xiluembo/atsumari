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

#include "atsumari.h"

#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>
#include <QQuaternion>
#include <QPixmap>
#include <QTemporaryFile>
#include <QDir>
#include <QSettings>

#include "settings_defaults.h"

Atsumari::Atsumari(QEntity *parent) : Qt3DCore::QEntity(parent) {
    QSettings settings;

    m_sphereMesh = new Qt3DExtras::QSphereMesh();
    m_sphereMesh->setRadius(0.97f);
    m_sphereMesh->setSlices(settings.value(CFG_SLICES, DEFAULT_SLICES).toInt());
    m_sphereMesh->setRings(settings.value(CFG_RINGS, DEFAULT_RINGS).toInt());

    m_material = new Qt3DExtras::QPhongMaterial();
    m_material->setDiffuse(QColor::fromString(settings.value(CFG_COLORS_DIFFUSE, DEFAULT_COLORS_DIFFUSE).toString()));
    m_material->setSpecular(QColor::fromString(settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString()));
    m_material->setAmbient(QColor::fromString(settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString()));
    m_material->setShininess(settings.value(CFG_SHININESS, DEFAULT_SHININESS).toInt() / 100.0f);

    m_transform = new Qt3DCore::QTransform();
    addComponent(m_sphereMesh);
    addComponent(m_material);
    addComponent(m_transform);

    // Animation
    m_rotationAnimation = new QPropertyAnimation(m_transform, QByteArrayLiteral("rotation"));
    m_rotationAnimation->setDuration(1000 * settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble());
    m_rotationAnimation->setStartValue(QQuaternion::fromEulerAngles(0, 0, 0));
    m_rotationAnimation->setEndValue(QQuaternion::fromEulerAngles(180, 360, 0));
    m_rotationAnimation->start();

    // Update random Rotation
    connect(m_rotationAnimation, &QPropertyAnimation::finished, this, &Atsumari::updateRandomRotation);
    updateRandomRotation();
}

void Atsumari::updateRandomRotation() {
    int newAngleX = QRandomGenerator64::global()->bounded(360);
    int newAngleY = QRandomGenerator64::global()->bounded(360);
    int newAngleZ = QRandomGenerator64::global()->bounded(360);

    QQuaternion qqnew = QQuaternion::fromEulerAngles(newAngleX, newAngleY, newAngleZ);
    QQuaternion qqold = QQuaternion::fromEulerAngles(m_transform->rotation().toEulerAngles().x(), m_transform->rotation().toEulerAngles().y(), m_transform->rotation().toEulerAngles().z());

    m_rotationAnimation->setStartValue(qqold);
    m_rotationAnimation->setEndValue(qqnew);
    m_rotationAnimation->start();
}

void Atsumari::setSlices(int slices)
{
    m_sphereMesh->setSlices(slices);
}

void Atsumari::setRings(int rings)
{
    m_sphereMesh->setRings(rings);
}

void Atsumari::setDiffuse(QString diffuseColor)
{
    m_material->setDiffuse(QColor::fromString(diffuseColor));
}

void Atsumari::setSpecular(QString specularColor)
{
    m_material->setSpecular(QColor::fromString(specularColor));
}

void Atsumari::setAmbient(QString ambientColor)
{
    m_material->setAmbient(QColor::fromString(ambientColor));
}

void Atsumari::setShininess(int shininess)
{
    m_material->setShininess(shininess / 100.0f);
}

void Atsumari::setIterationInterval(double seconds)
{
    m_rotationAnimation->setDuration(1000 * seconds);
}

void Atsumari::addEmote(const QUrl &emote, float theta, float phi, float emoteSize) {
    Qt3DRender::QTextureImage *textureImage = new Qt3DRender::QTextureImage();

    textureImage->setSource(emote);

    Qt3DRender::QTexture2D *texture = new Qt3DRender::QTexture2D();
    texture->addTextureImage(textureImage);

    Qt3DExtras::QTextureMaterial *material = new Qt3DExtras::QTextureMaterial();
    material->setAlphaBlendingEnabled(true);
    material->setTexture(texture);

    // Generate random spheric coordinates

    if ( qFuzzyCompare(theta, -1.0f) && qFuzzyCompare(phi,-1.0f) ) {
        theta = QRandomGenerator::global()->bounded(360.0f); // Azimute
        float u = QRandomGenerator::global()->bounded(2.0f) - 1.0f;
        phi = qRadiansToDegrees(qAcos(u));
    }

    // Convert to cartesian coordinates
    float x = sin(qDegreesToRadians(phi)) * cos(qDegreesToRadians(theta));
    float y = sin(qDegreesToRadians(phi)) * sin(qDegreesToRadians(theta));
    float z = cos(qDegreesToRadians(phi));

    QVector3D position(x, y, z);
    QVector3D normal = position.normalized(); // Sphere surface normal

    // Set up "up" vector
    QVector3D up(0, 1, 0);

    QVector3D right = QVector3D::crossProduct(normal, up).normalized();
    QVector3D correctedUp = QVector3D::crossProduct(right, normal).normalized();

    // Rotate to make it parallel to the surface
    QQuaternion rotation = QQuaternion::fromDirection(normal, correctedUp);

    // Create Emote entity
    Qt3DCore::QEntity *emoteEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QPlaneMesh *planeMesh = new Qt3DExtras::QPlaneMesh();
    planeMesh->setWidth(emoteSize);  // Emote Size
    planeMesh->setHeight(emoteSize);

    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform();
    transform->setTranslation(position);
    // Make it parallel to the surface
    transform->setRotation(rotation * QQuaternion::fromEulerAngles(90, 0, 0));

    // Add components to emoteEntity
    emoteEntity->addComponent(planeMesh);
    emoteEntity->addComponent(material);
    emoteEntity->addComponent(transform);

    // -------------- Reverse Emote

    // Convert to cartesian coordinates
    float x2 = 0.98f * sin(qDegreesToRadians(phi)) * cos(qDegreesToRadians(theta));
    float y2 = 0.98f * sin(qDegreesToRadians(phi)) * sin(qDegreesToRadians(theta));
    float z2 = 0.98f * cos(qDegreesToRadians(phi));

    QVector3D position2(x2, y2, z2);

    // Create Emote entity
    Qt3DCore::QEntity *emoteEntity2 = new Qt3DCore::QEntity(this);
    Qt3DExtras::QPlaneMesh *planeMesh2 = new Qt3DExtras::QPlaneMesh();
    planeMesh2->setWidth(emoteSize);  // Emote Size
    planeMesh2->setHeight(emoteSize);

    Qt3DCore::QTransform *transform2 = new Qt3DCore::QTransform();
    transform2->setTranslation(position2);
    // Make it parallel to the surface
    transform2->setRotation(rotation * QQuaternion::fromEulerAngles(-90, 0, 0));

    // add components to emoteEntity2
    emoteEntity2->addComponent(planeMesh2);
    emoteEntity2->addComponent(material);
    emoteEntity2->addComponent(transform2);

}

