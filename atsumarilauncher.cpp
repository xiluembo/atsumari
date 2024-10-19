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

#include "atsumarilauncher.h"

#include <Qt3DCore/QEntity>

#include <Qt3DRender/QCamera>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DRender/QPointLight>
#include <QUrl>
#include <QRandomGenerator64>
#include <QSettings>
#include <QMainWindow>
#include <QVBoxLayout>

#include "settings_defaults.h"
#include "atsumari.h"


AtsumariLauncher::AtsumariLauncher()
    : m_twFlow(new TwitchAuthFlow(QString("a123%1").arg(QRandomGenerator64::global()->generate())))
    , m_emw(new EmoteWriter())
    , m_tReader(nullptr)
    , m_window()
{ }

void AtsumariLauncher::launch()
{
    QSettings settings;

    int currentProfile = settings.value(CFG_CURRENT_PROFILE, DEFAULT_CURRENT_PROFILE).toInt();
    settings.beginReadArray(CFG_PROFILES);
    settings.setArrayIndex(currentProfile);

    QMainWindow* mw = new QMainWindow;
    QWidget* container = new QWidget;
    mw->resize(300,300);
    container->setLayout(new QVBoxLayout);
    container->layout()->setContentsMargins(0, 0, 0, 0);
    container->layout()->addWidget(QWidget::createWindowContainer(&m_window));
    mw->setCentralWidget(container);
    mw->show();

    // View window
    // m_window.setWidth(300);
    // m_window.setHeight(300);
    m_window.defaultFrameGraph()->setClearColor(QColor(Qt::black));
    m_window.setIcon(QIcon(":/appicon/atsumari.svg"));
    m_window.show();

    // Setting up Camera
    Qt3DRender::QCamera *camera = m_window.camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, 3.0));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // Root Entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    // Adding Atsumari
    Atsumari *atsumari = new Atsumari(rootEntity);

    // Camera Controller
    Qt3DExtras::QOrbitCameraController *cameraController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    cameraController->setCamera(camera);

    // Add Lighting
    auto *lightEntity = new Qt3DCore::QEntity(rootEntity);
    auto *light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString());
    light->setIntensity(settings.value(CFG_LIGHT_INTENSITY, DEFAULT_LIGHT_INTENSITY).toFloat() / 100.0f);
    auto *lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(camera->position());
    lightEntity->addComponent(light);
    lightEntity->addComponent(lightTransform);

    // Setting up the root entity into the Window
    m_window.setRootEntity(rootEntity);
    m_window.resize(301,301);

    QUrl kata_deco = QUrl::fromLocalFile(settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString());

    atsumari->addEmote(kata_deco, 0.0f, 0.0f, 0.35f);
    atsumari->addEmote(kata_deco, 0.0f, 180.0f, 0.35f);
    atsumari->addEmote(kata_deco, 0.0f, 63.435f, 0.35f);
    atsumari->addEmote(kata_deco, 72.0f, 63.435f, 0.35f);
    atsumari->addEmote(kata_deco, 144.0f, 63.435f, 0.35f);
    atsumari->addEmote(kata_deco, 216.0f, 63.435f, 0.35f);
    atsumari->addEmote(kata_deco, 288.0f, 63.435f, 0.35f);
    atsumari->addEmote(kata_deco, 36.0f, 116.565f, 0.35f);
    atsumari->addEmote(kata_deco, 108.0f, 116.565f, 0.35f);
    atsumari->addEmote(kata_deco, 180.0f, 116.565f, 0.35f);
    atsumari->addEmote(kata_deco, 252.0f, 116.565f, 0.35f);
    atsumari->addEmote(kata_deco, 324.0f, 116.565f, 0.35f);

    QObject::connect(m_twFlow, &TwitchAuthFlow::loginFetched, m_twFlow, [&](const QString& a) {
        m_tReader = new TwitchChatReader("ws://irc-ws.chat.twitch.tv:80/", m_twFlow->token(), a);

        QObject::connect(m_tReader, &TwitchChatReader::emoteSent, m_emw, [=](const QString& id, const QString& emoName) {
            Q_UNUSED(emoName);
            m_emw->saveEmote(id);
        });

        QObject::connect(m_tReader, &TwitchChatReader::bigEmoteSent, m_emw, [=](const QString& id, const QString& emoName) {
            Q_UNUSED(emoName);
            m_emw->saveBigEmote(id);
        });

        QObject::connect(m_tReader, &TwitchChatReader::emojiSent, m_emw, &EmoteWriter::saveEmoji);
    });

    QObject::connect(m_emw, &EmoteWriter::emoteWritten, atsumari, [=](QString path) {
        atsumari->addEmote(QUrl::fromLocalFile(path), -1.0f, -1.0f, 0.4f);
    });

    QObject::connect(m_emw, &EmoteWriter::bigEmoteWritten, atsumari, [=](QString path) {
        atsumari->addEmote(QUrl::fromLocalFile(path), -1.0f, -1.0f, 0.70f);
    });

    settings.endArray();
}
