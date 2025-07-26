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

#include <QQuickView>
#include <QQuickItem>
#include <QUrl>
#include <QRandomGenerator64>
#include <QSettings>
#include <QVBoxLayout>
#include <QGuiApplication>

#include "settings_defaults.h"
#include "materialtype.h"


AtsumariLauncher::AtsumariLauncher()
    : m_twFlow(new TwitchAuthFlow(QString("a123%1").arg(QRandomGenerator64::global()->generate())))
    , m_emw(new EmoteWriter())
    , m_tReader(nullptr)
    , m_mw(new QMainWindow)
    , m_container(new QWidget)
{ }

AtsumariLauncher::~AtsumariLauncher()
{
    m_container->deleteLater();
    m_mw->deleteLater();
}

void AtsumariLauncher::launch()
{
    QSettings settings;

    int currentProfile = settings.value(CFG_CURRENT_PROFILE, DEFAULT_CURRENT_PROFILE).toInt();
    settings.beginReadArray(CFG_PROFILES);
    settings.setArrayIndex(currentProfile);

    // Create main window
    m_mw = new QMainWindow;
    m_container = new QWidget;
    m_mw->resize(300, 300);
    m_container->setLayout(new QVBoxLayout);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    m_mw->setCentralWidget(m_container);
    m_mw->show();
    m_mw->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    // Create QQuickView for 3D scene
    QQuickView* quickView = new QQuickView();
    quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    quickView->setSource(QUrl("qrc:/main.qml"));
    
    // Add QQuickView to main window
    m_container->layout()->addWidget(QWidget::createWindowContainer(quickView));
    
    // Get root item to set properties
    QQuickItem* rootItem = quickView->rootObject();
    
    // Load settings from current profile
    QString baseColor = settings.value(CFG_COLORS_BASE, DEFAULT_COLORS_BASE).toString();
    QString ambientColor = settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString();
    QString specularColor = settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString();
    QString lightColor = settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString();
    int lightBrightness = settings.value(CFG_LIGHT_BRIGHTNESS, DEFAULT_LIGHT_BRIGHTNESS).toInt();
    double iteration = settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble();
    int roughness = settings.value(CFG_ROUGHNESS, DEFAULT_ROUGHNESS).toInt();
    int metalness = settings.value(CFG_METALNESS, DEFAULT_METALNESS).toInt();
    int glossiness = settings.value(CFG_GLOSSINESS, DEFAULT_GLOSSINESS).toInt();
    int refraction = settings.value(CFG_REFRACTION, DEFAULT_REFRACTION).toInt();
    MaterialType materialType = static_cast<MaterialType>(settings.value(CFG_MATERIAL_TYPE, static_cast<int>(DEFAULT_MATERIAL_TYPE)).toInt());
    QString decorationPath = settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString();
    
    // Set QML properties based on settings
    rootItem->setProperty("baseColor", QColor(baseColor));
    rootItem->setProperty("ambientColor", QColor(ambientColor));
    rootItem->setProperty("lightColor", QColor(lightColor));
    rootItem->setProperty("brightness", lightBrightness / 100.0);
    rootItem->setProperty("rotationInterval", iteration * 1000);
    
    if (materialType == MaterialType::Principled) {
        rootItem->setProperty("useSpecularGlossyMaterial", false);
        rootItem->setProperty("roughness", roughness / 100.0);
        rootItem->setProperty("metalness", metalness / 100.0);
        rootItem->setProperty("refraction", refraction / 100.0);
    } else if (materialType == MaterialType::SpecularGlossy) {
        rootItem->setProperty("useSpecularGlossyMaterial", true);
        rootItem->setProperty("specularColor", QColor(specularColor));
        rootItem->setProperty("glossiness", glossiness / 100.0);
    }
    
    // Set decoration path
    if (decorationPath.startsWith(":/")) {
        // Resource path - adjust for QML
        decorationPath = decorationPath.mid(2);
        rootItem->setProperty("decorationPath", decorationPath);
    } else {
        // Local file path - convert to file:// URL
        rootItem->setProperty("decorationPath", QUrl::fromLocalFile(decorationPath).toString());
    }
    
    // Add initial emotes using QML function
    QMetaObject::invokeMethod(rootItem, "addEmoteAtIcosahedronVertex", Qt::QueuedConnection);
    
    // Setup Twitch chat connections
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

    // Connect emote writer to add emotes to QML scene
    QObject::connect(m_emw, &EmoteWriter::emoteWritten, [=](QString path) {
        QMetaObject::invokeMethod(rootItem, "addEmote", Qt::QueuedConnection,
                                 Q_ARG(QVariant, QUrl::fromLocalFile(path).toString()),
                                 Q_ARG(QVariant, -1.0), Q_ARG(QVariant, -1.0), Q_ARG(QVariant, 0.3));
    });

    QObject::connect(m_emw, &EmoteWriter::bigEmoteWritten, [=](QString path) {
        QMetaObject::invokeMethod(rootItem, "addEmote", Qt::QueuedConnection,
                                 Q_ARG(QVariant, QUrl::fromLocalFile(path).toString()),
                                 Q_ARG(QVariant, -1.0), Q_ARG(QVariant, -1.0), Q_ARG(QVariant, 0.25));
    });

    settings.endArray();

    // now we can close the app if the window is closed
    qGuiApp->setQuitOnLastWindowClosed(true);
}
