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
#include <QSettings>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QMenu>
#include <QTranslator>
#include <QPixmap>
#include <QQuick3DTextureData>
#include <QTemporaryFile>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>

#include "settings_defaults.h"
#include "materialtype.h"
#include "twitchlogmodel.h"


AtsumariLauncher::AtsumariLauncher(QObject *parent)
    : QObject(parent)
    , m_twFlow(new TwitchAuthFlow(this))
    , m_emw(new EmoteWriter())
    , m_tReader(nullptr)
    , m_mw(new QMainWindow)
    , m_container(new QWidget)
    , m_tray(nullptr)
    , m_logDialog(new LogViewDialog)
{ }

AtsumariLauncher::~AtsumariLauncher()
{
    m_container->deleteLater();
    m_mw->deleteLater();
    if (m_tray)
        m_tray->deleteLater();
    if (m_logDialog)
        m_logDialog->deleteLater();
}

void AtsumariLauncher::launch()
{
    QSettings settings;

    int currentProfile = settings.value(CFG_CURRENT_PROFILE, DEFAULT_CURRENT_PROFILE).toInt();
    settings.beginReadArray(CFG_PROFILES);
    settings.setArrayIndex(currentProfile);

    // Create main window
    m_mw = new QMainWindow;
    m_mw->installEventFilter(this);
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
    QString baseTexture = settings.value(CFG_BASE_TEXTURE, DEFAULT_BASE_TEXTURE).toString();
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
    QString customVert = settings.value(CFG_CUSTOM_VERT, DEFAULT_CUSTOM_VERT).toString();
    QString customFrag = settings.value(CFG_CUSTOM_FRAG, DEFAULT_CUSTOM_FRAG).toString();
    
    // Set QML properties based on settings
    rootItem->setProperty("baseColor", QColor(baseColor));
    if (baseTexture.startsWith(":/"))
        rootItem->setProperty("baseColorTexture", QString("qrc") + baseTexture);
    else if (!baseTexture.isEmpty())
        rootItem->setProperty("baseColorTexture", QUrl::fromLocalFile(baseTexture).toString());
    rootItem->setProperty("ambientColor", QColor(ambientColor));
    rootItem->setProperty("lightColor", QColor(lightColor));
    rootItem->setProperty("brightness", lightBrightness / 100.0);
    rootItem->setProperty("rotationInterval", iteration * 1000);
    
    if (materialType == MaterialType::Principled) {
        rootItem->setProperty("useCustomMaterial", false);
        rootItem->setProperty("useSpecularGlossyMaterial", false);
        rootItem->setProperty("roughness", roughness / 100.0);
        rootItem->setProperty("metalness", metalness / 100.0);
        rootItem->setProperty("refraction", refraction / 100.0);
    } else if (materialType == MaterialType::SpecularGlossy) {
        rootItem->setProperty("useCustomMaterial", false);
        rootItem->setProperty("useSpecularGlossyMaterial", true);
        rootItem->setProperty("specularColor", QColor(specularColor));
        rootItem->setProperty("glossiness", glossiness / 100.0);
    } else if (materialType == MaterialType::Custom) {
        rootItem->setProperty("useSpecularGlossyMaterial", false);
        rootItem->setProperty("useCustomMaterial", true);

        if (!m_vertexShaderFile) {
            m_vertexShaderFile = new QTemporaryFile(this);
            m_vertexShaderFile->setFileTemplate(QDir::tempPath() + "/atsumariXXXXXX.vert");
        }
        if (!m_fragmentShaderFile) {
            m_fragmentShaderFile = new QTemporaryFile(this);
            m_fragmentShaderFile->setFileTemplate(QDir::tempPath() + "/atsumariXXXXXX.frag");
        }

        if (m_vertexShaderFile->open()) {
            m_vertexShaderFile->resize(0);
            m_vertexShaderFile->write(customVert.toUtf8());
            m_vertexShaderFile->flush();
            m_vertexShaderFile->close();
        }
        if (m_fragmentShaderFile->open()) {
            m_fragmentShaderFile->resize(0);
            m_fragmentShaderFile->write(customFrag.toUtf8());
            m_fragmentShaderFile->flush();
            m_fragmentShaderFile->close();
        }

        rootItem->setProperty("vertexShaderPath", QUrl::fromLocalFile(m_vertexShaderFile->fileName()).toString());
        rootItem->setProperty("fragmentShaderPath", QUrl::fromLocalFile(m_fragmentShaderFile->fileName()).toString());
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
        m_tReader = new TwitchChatReader("wss://irc-ws.chat.twitch.tv:443/", m_twFlow->token(), a, m_emw);

        QObject::connect(m_tReader, &TwitchChatReader::emoteSent, m_emw, [=](const QString& id, const QString& emoName) {
            Q_UNUSED(emoName);
            m_emw->saveEmote(id);
        });

        QObject::connect(m_tReader, &TwitchChatReader::bigEmoteSent, m_emw, [=](const QString& id, const QString& emoName) {
            Q_UNUSED(emoName);
            m_emw->saveBigEmote(id);
        });

        QObject::connect(m_tReader, &TwitchChatReader::emojiSent, m_emw,
                         [=](const QString &slug, const QString &emoji) {
            QSettings settings;
            int profile = settings.value(CFG_CURRENT_PROFILE, 0).toInt();
            settings.beginReadArray(CFG_PROFILES);
            settings.setArrayIndex(profile);
            QString font = settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
            settings.endArray();
            m_emw->saveEmoji(slug, emoji, font);
        });
    });

    // Connect emote writer to add emotes to QML scene
    QObject::connect(m_emw, &EmoteWriter::emoteReady,
                     [=](const QString &id, QQuick3DTextureData *tex, const QPixmap &pix) {
        QMetaObject::invokeMethod(rootItem, "addEmote", Qt::QueuedConnection,
                                 Q_ARG(QVariant, QVariant::fromValue(tex)),
                                 Q_ARG(QVariant, -1.0), Q_ARG(QVariant, -1.0), Q_ARG(QVariant, 0.25));
        TwitchLogModel::instance()->loadEmote(id, pix);
    });

    QObject::connect(m_emw, &EmoteWriter::bigEmoteReady,
                     [=](const QString &id, QQuick3DTextureData *tex, const QPixmap &pix) {
        QMetaObject::invokeMethod(rootItem, "addEmote", Qt::QueuedConnection,
                                 Q_ARG(QVariant, QVariant::fromValue(tex)),
                                 Q_ARG(QVariant, -1.0), Q_ARG(QVariant, -1.0), Q_ARG(QVariant, 0.30));
        TwitchLogModel::instance()->loadEmote(id, pix);
    });

    settings.endArray();

    m_logDialog->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new QSystemTrayIcon(QIcon(":/appicon/atsumari.svg"), m_mw);
        QMenu* menu = new QMenu(m_mw);
        QAction* act = menu->addAction(tr("Show Logs"));
        QObject::connect(act, &QAction::triggered, m_logDialog, &QDialog::show);
        QAction* quitAct = menu->addAction(tr("Quit"));
        QObject::connect(quitAct, &QAction::triggered, this, &QCoreApplication::quit);
        m_tray->setContextMenu(menu);
        QObject::connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick)
                m_logDialog->show();
        });
        m_tray->show();
    } else {
        m_container->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(m_container, &QWidget::customContextMenuRequested, [=](const QPoint& p) {
            QMenu menu;
            QAction* act = menu.addAction(tr("Show Logs"));
            QObject::connect(act, &QAction::triggered, m_logDialog, &QDialog::show);
            menu.exec(m_container->mapToGlobal(p));
        });
    }

    // now we can close the app if the window is closed
    qGuiApp->setQuitOnLastWindowClosed(true);
}

bool AtsumariLauncher::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_mw && event->type() == QEvent::Close) {
        int ret = QMessageBox::question(m_mw,
                                        tr("Save logs"),
                                        tr("Do you want to save logs before closing?"),
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                        QMessageBox::Cancel);

        if (ret == QMessageBox::Yes) {
            QString fn = QFileDialog::getSaveFileName(m_mw,
                                                     tr("Export logs"),
                                                     QString(),
                                                     tr("Text Files (*.txt)"));
            if (!fn.isEmpty()) {
                TwitchLogModel::instance()->exportToFile(fn);
            }
            event->accept();
            return false;
        } else if (ret == QMessageBox::No) {
            event->accept();
            return false;
        } else {
            event->ignore();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}
