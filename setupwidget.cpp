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

#include "setupwidget.h"
#include <QQuickItem>
#include "./ui_setupwidget.h"

#include <algorithm>

#include <QSettings>
#include <QColorDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStyleHints>
#include <QMenu>
#include <QCloseEvent>
#include <QUrl>
#include <QDir>
#include <QTableWidget>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QPainter>
#include <QPixmap>
#include <QRandomGenerator>
#include <QQuick3DTextureData>
#include <QImage>

#include "settings_defaults.h"
#include "logcommandcolors.h"
#include "materialtype.h"
#include "atsumarilauncher.h"
#include "localehelper.h"

SetupWidget::SetupWidget(QWidget *parent)
    : QWidget(parent)
    , m_shouldSave(false)
    , m_rebuildingCombo(false)
    , m_profileMenu(nullptr)
    , m_newProfileAction(nullptr)
    , m_duplicateProfileAction(nullptr)
    , m_renameProfileAction(nullptr)
    , m_deleteProfileAction(nullptr)
    , m_previewVertFile(new QTemporaryFile(this))
    , m_previewFragFile(new QTemporaryFile(this))
    , ui(new Ui::SetupWidget)
{
    ui->setupUi(this);
    setIcons();
    populateLanguages();
    populateMaterialTypes();
    populateRefractionTypes();
    loadSettings();

    // Appearance tab
    connect(ui->cboProfile, &QComboBox::currentIndexChanged, this, [=]() {
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        populateCurrentProfileControls();
        runPreview();
    });

    createProfileMenus();

    connect(ui->btnSelectBaseColor, &QPushButton::clicked, this, [=]() {
        selectColor(&ProfileData::baseColor, &ProfileData::setBaseColor, ui->frmBaseColor);
    });

    connect(ui->btnSelectSpecular, &QPushButton::clicked, this, [=]() {
        selectColor(&ProfileData::specularColor, &ProfileData::setSpecularColor, ui->frmSpecularColor);
    });

    connect(ui->btnSelectAmbient, &QPushButton::clicked, this, [=]() {
        selectColor(&ProfileData::ambientColor, &ProfileData::setAmbientColor, ui->frmAmbientColor);
    });

    connect(ui->btnSelectLight, &QPushButton::clicked, this, [=]() {
        selectColor(&ProfileData::lightColor, &ProfileData::setLightColor, ui->frmLightColor);
    });

    connect(ui->cboLanguage, &QComboBox::currentIndexChanged, this, [=]() {
        LocaleHelper::loadTranslation(ui->cboLanguage->currentData().toLocale());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        ui->retranslateUi(this);

        // Menus are not retranslated, let's recreate them
        createProfileMenus();
    });

    connect(ui->cboEmojiFont, &QComboBox::currentIndexChanged, this, [=]() {
        m_profiles[m_currentProfile]->setFont(ui->cboEmojiFont->currentFont().family());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->cboMaterialType, &QComboBox::currentIndexChanged, this, [=]() {
        m_profiles[m_currentProfile]->setMaterialType(static_cast<MaterialType>(ui->cboMaterialType->currentData().toInt()));
        ui->grpPrincipled->setVisible(ui->cboMaterialType->currentData() == static_cast<int>(MaterialType::Principled));
        ui->grpSpecularGlossy->setVisible(ui->cboMaterialType->currentData() == static_cast<int>(MaterialType::SpecularGlossy));
        ui->grpCustomMaterial->setVisible(ui->cboMaterialType->currentData() == static_cast<int>(MaterialType::Custom));
        bool isCustom = ui->cboMaterialType->currentData() == static_cast<int>(MaterialType::Custom);
        ui->frmBaseColor->setEnabled(!isCustom);
        ui->btnSelectBaseColor->setEnabled(!isCustom);
        ui->btnReloadShaders->setEnabled(false);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    // any of those values changing may trigger a preview update

    connect(ui->sldRoughness, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRoughness(ui->sldRoughness->value());
        ui->spnRoughness->setValue(ui->sldRoughness->value() / 100.0);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnRoughness, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRoughness(ui->spnRoughness->value() * 100);
        ui->sldRoughness->setValue(ui->spnRoughness->value() * 100);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldMetalness, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setMetalness(ui->sldMetalness->value());
        ui->spnMetalness->setValue(ui->sldMetalness->value() / 100.0);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnMetalness, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setMetalness(ui->spnMetalness->value() * 100);
        ui->sldMetalness->setValue(ui->spnMetalness->value() * 100);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldRefraction, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRefraction(ui->sldRefraction->value());
        ui->spnRefraction->setValue(ui->sldRefraction->value() / 100.0);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnRefraction, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRefraction(ui->spnRefraction->value() * 100);
        ui->sldRefraction->setValue(ui->spnRefraction->value() * 100);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->cboRefraction, &QComboBox::currentIndexChanged, this, [=]() {
        bool isCustom = ui->cboRefraction->currentData() == static_cast<int>(IndexOfRefraction::Custom);
        ui->spnRefraction->setEnabled(isCustom);
        ui->sldRefraction->setEnabled(isCustom);

        if (!isCustom) {
            int refractionValue = ui->cboRefraction->currentData().toInt();
            ui->spnRefraction->setValue(refractionValue / 100.0);
            ui->sldRefraction->setValue(refractionValue);
            m_profiles[m_currentProfile]->setRefraction(refractionValue);
        }

        m_profiles[m_currentProfile]->setIndexOfRefractionType(static_cast<IndexOfRefraction>(ui->cboRefraction->currentData().toInt()));
        runPreview();
    });

    connect(ui->spnIteration, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setIteration(ui->spnIteration->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldLightBrightness, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setLightBrightness(ui->sldLightBrightness->value());
        ui->spnLightBrightness->setValue(ui->sldLightBrightness->value() / 100.0);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnLightBrightness, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRefraction(ui->spnLightBrightness->value() * 100);
        ui->sldLightBrightness->setValue(ui->spnLightBrightness->value() * 100);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldGlossiness, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setGlossiness(ui->sldGlossiness->value());
        ui->spnGlossiness->setValue(ui->sldGlossiness->value() / 100.0);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnGlossiness, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRefraction(ui->spnGlossiness->value() * 100);
        ui->sldGlossiness->setValue(ui->spnGlossiness->value() * 100);
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->txtVertexShader, &QPlainTextEdit::textChanged, this, [=]() {
        m_profiles[m_currentProfile]->setCustomVertexShader(ui->txtVertexShader->toPlainText());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        ui->btnReloadShaders->setEnabled(true);
    });

    connect(ui->txtFragmentShader, &QPlainTextEdit::textChanged, this, [=]() {
        m_profiles[m_currentProfile]->setCustomFragmentShader(ui->txtFragmentShader->toPlainText());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        ui->btnReloadShaders->setEnabled(true);
    });

    connect(ui->btnReloadShaders, &QPushButton::clicked, this, [=]() {
        runPreview();
        ui->btnReloadShaders->setEnabled(false);
    });

    connect(ui->btnResetDecoration, &QPushButton::clicked, this, &SetupWidget::resetDecoration);
    connect(ui->btnSelectDecoration, &QPushButton::clicked, this, &SetupWidget::selectDecoration);

    setupPreview();
    runPreview();

    // Twitch Tab
    connect(ui->btnAddExcludeChat, &QPushButton::clicked, this, &SetupWidget::addToExcludeList);
    connect(ui->btnRemoveExcludeChat, &QPushButton::clicked, this, &SetupWidget::removeFromExcludeList);
    connect(ui->btnDevConsole, &QPushButton::clicked, this, &SetupWidget::openDevConsole);
    connect(ui->btnForceAuth, &QPushButton::clicked, this, &SetupWidget::resetAuth);
    connect(ui->edtClientId, &QLineEdit::textChanged, this, [=]() {
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
    });

    connect(ui->edtExcludeChat, &QLineEdit::textChanged, this, [=]() {
        QString userName = ui->edtExcludeChat->text();
        if (userName.startsWith("@")) {
            userName = userName.mid(1);
        }

        ui->btnAddExcludeChat->setEnabled(!userName.isEmpty());
    });

    // About
    connect(ui->btnAboutQt, &QPushButton::clicked, this, &SetupWidget::aboutQt);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [=](int index) {
        if (index == 3) {
            // Workaround as big labels may get incorrect size
            ui->lblAtsuLicense->adjustSize();
#ifdef Q_OS_WIN
            ui->lblBreezeLicense->adjustSize();
#else
            ui->lblBreezeLicense->hide();
            ui->hlBreeze->hide();
#endif
            ui->scrollAreaWidgetContents->layout()->update();
        }
    });

    // Actions
    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &SetupWidget::saveSettings);
    connect(ui->btnClose, &QPushButton::clicked, this, &SetupWidget::checkClose);
}

SetupWidget::~SetupWidget()
{
    delete ui;
    if (m_previewWindow) m_previewWindow->deleteLater();
}

void SetupWidget::runPreview()
{
    if (!m_previewRootItem) {
        return;
    }

    // Update QML properties for real-time preview

    // common material properties
    m_previewRootItem->setProperty("baseColor", QColor(m_profiles[m_currentProfile]->baseColor()));
    m_previewRootItem->setProperty("ambientColor", QColor(m_profiles[m_currentProfile]->ambientColor()));

    MaterialType materialType = static_cast<MaterialType>(ui->cboMaterialType->currentData().toInt());
    if(materialType == MaterialType::Principled) {
        // principled material properties
        m_previewRootItem->setProperty("useCustomMaterial", false);
        m_previewRootItem->setProperty("useSpecularGlossyMaterial", false);
        m_previewRootItem->setProperty("roughness", ui->spnRoughness->value());
        m_previewRootItem->setProperty("metalness", ui->spnMetalness->value());
        m_previewRootItem->setProperty("refraction", ui->spnRefraction->value());
    } else if (materialType == MaterialType::SpecularGlossy) {
        // specular glossy material properties
        m_previewRootItem->setProperty("useCustomMaterial", false);
        m_previewRootItem->setProperty("useSpecularGlossyMaterial", true);
        m_previewRootItem->setProperty("specularColor", QColor(m_profiles[m_currentProfile]->specularColor()));
        m_previewRootItem->setProperty("glossiness", ui->spnGlossiness->value());
    } else if (materialType == MaterialType::Custom) {
        m_previewRootItem->setProperty("useSpecularGlossyMaterial", false);
        m_previewRootItem->setProperty("useCustomMaterial", true);

        if (m_previewVertFile->open()) {
            m_previewVertFile->resize(0);
            m_previewVertFile->write(m_profiles[m_currentProfile]->customVertexShader().toUtf8());
            m_previewVertFile->flush();
            m_previewVertFile->close();
        }
        if (m_previewFragFile->open()) {
            m_previewFragFile->resize(0);
            m_previewFragFile->write(m_profiles[m_currentProfile]->customFragmentShader().toUtf8());
            m_previewFragFile->flush();
            m_previewFragFile->close();
        }

        m_previewRootItem->setProperty("vertexShaderPath", QUrl::fromLocalFile(m_previewVertFile->fileName()).toString());
        m_previewRootItem->setProperty("fragmentShaderPath", QUrl::fromLocalFile(m_previewFragFile->fileName()).toString());
    }

    // other common properties
    m_previewRootItem->setProperty("rotationInterval", ui->spnIteration->value() * 1000);
    m_previewRootItem->setProperty("lightColor", m_profiles[m_currentProfile]->lightColor());
    m_previewRootItem->setProperty("brightness", ui->spnLightBrightness->value());
    
    // Set decoration path from current profile
    QString decorationPath = m_profiles[m_currentProfile]->decorationPath();
    // Convert to QUrl format for QML compatibility
    if (decorationPath.startsWith(":/")) {
        // Resource path - adjust for QML
        decorationPath = decorationPath.mid(2);
        m_previewRootItem->setProperty("decorationPath", decorationPath);
    } else {
        // Local file path - convert to file:// URL
        m_previewRootItem->setProperty("decorationPath", QUrl::fromLocalFile(decorationPath).toString());
    }
    
    // Add emotes at icosahedron vertices using QML function
    QMetaObject::invokeMethod(m_previewRootItem, "addEmoteAtIcosahedronVertex", Qt::QueuedConnection);
    
    // Add test emojis for font testing (random positions)
    QStringList testEmojis = {"💜", "🦋", "👁️", "❄️", "🥑", "🙂"};
    for (const QString& emoji : testEmojis) {
        QPixmap emojiPixmap(64, 64);
        emojiPixmap.fill(Qt::transparent);
        QPainter painter(&emojiPixmap);
        painter.setFont(QFont(m_profiles[m_currentProfile]->font(), 32));
        painter.setPen(Qt::black);
        painter.drawText(emojiPixmap.rect(), Qt::AlignCenter, emoji);
        painter.end();

        QImage image = emojiPixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
        auto *texData = new QQuick3DTextureData();
        texData->setParent(m_previewRootItem);
        texData->setSize(image.size());
        texData->setFormat(QQuick3DTextureData::RGBA8);
        texData->setTextureData(QByteArray(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes()));

        QMetaObject::invokeMethod(m_previewRootItem, "addEmote", Qt::QueuedConnection,
                                Q_ARG(QVariant, QVariant::fromValue(texData)),
                                Q_ARG(QVariant, -1.0), Q_ARG(QVariant, -1.0), Q_ARG(QVariant, 0.3));
    }
}

void SetupWidget::resetDecoration()
{
    m_profiles[m_currentProfile]->setDecorationPath(DEFAULT_DECORATION_PATH);
    ui->lblDecoration->setPixmap(QPixmap(DEFAULT_DECORATION_PATH));
    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::loadSettings()
{
    QSettings settings;

    // Appearance & Behavior
    QLocale defaultLocale = settings.value(CFG_LANGUAGE, LocaleHelper::findBestLocale()).toLocale();
    int langIndex = ui->cboLanguage->findData(defaultLocale);
    if (langIndex == -1) {
        langIndex = ui->cboLanguage->findData(LocaleHelper::findBestLocale());
    }
    ui->cboLanguage->setCurrentIndex(langIndex);

    // profiles
    int size = settings.beginReadArray(CFG_PROFILES);
    if (size == 0) {
        settings.endArray();

        // populate default profile
        m_currentProfile = 0;
        ProfileData* profileData = new ProfileData;
        QString defaultProfileName = tr("Default");
        profileData->setProfileName(defaultProfileName);
        profileData->setMaterialType(DEFAULT_MATERIAL_TYPE);
        profileData->setBaseColor(DEFAULT_COLORS_BASE);
        profileData->setAmbientColor(DEFAULT_COLORS_AMBIENT);
        profileData->setLightColor(DEFAULT_COLORS_LIGHT);
        profileData->setRoughness(DEFAULT_ROUGHNESS);
        profileData->setMetalness(DEFAULT_METALNESS);
        profileData->setIndexOfRefractionType(DEFAULT_REFRACTION_TYPE);
        profileData->setRefraction(DEFAULT_REFRACTION);
        profileData->setSpecularColor(DEFAULT_COLORS_SPECULAR);
        profileData->setGlossiness(DEFAULT_GLOSSINESS);
        profileData->setDecorationPath(DEFAULT_DECORATION_PATH);
        profileData->setLightBrightness(DEFAULT_LIGHT_BRIGHTNESS);
        profileData->setIteration(DEFAULT_ITERATION_TIME);
        profileData->setFont(DEFAULT_EMOJI_FONT);

        m_profiles.append(profileData);

        ui->cboProfile->addItem(defaultProfileName);
    } else {
        for (qsizetype i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            ProfileData* profileData = new ProfileData;
            QString profileName = settings.value(CFG_PROFILE_NAME).toString();

            int version;

            version = settings.value(CFG_VERSION, DEFAULT_VERSION).toInt();
            profileData->setProfileName(profileName);

            if ( version == 1 ) {
                // migrate to Specular Glossy
                profileData->setMaterialType(DEFAULT_MATERIAL_TYPE);
                profileData->setBaseColor(settings.value(CFG_V1_COLORS_DIFFUSE, DEFAULT_COLORS_BASE).toString()); // from v1
                profileData->setAmbientColor(settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString());
                profileData->setLightColor(settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString());

                // default values for Principled Material
                profileData->setRoughness(DEFAULT_ROUGHNESS);
                profileData->setMetalness(DEFAULT_METALNESS);
                profileData->setIndexOfRefractionType(DEFAULT_REFRACTION_TYPE);
                profileData->setRefraction(DEFAULT_REFRACTION);

                profileData->setSpecularColor(settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString());
                profileData->setGlossiness(settings.value(CFG_V1_SHININESS, DEFAULT_GLOSSINESS).toInt() / 2); // from v1

                profileData->setLightBrightness(settings.value(CFG_V1_LIGHT_INTENSITY, DEFAULT_LIGHT_BRIGHTNESS).toInt()); // from v1
                profileData->setIteration(settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble());
                profileData->setDecorationPath(settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString());
                profileData->setFont(settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString());
                profileData->setCustomVertexShader(DEFAULT_CUSTOM_VERT);
                profileData->setCustomFragmentShader(DEFAULT_CUSTOM_FRAG);
            } else if ( version == 2 ) {
                int materialType = settings.value(CFG_MATERIAL_TYPE).toInt();
                profileData->setMaterialType(static_cast<MaterialType>(materialType));
                profileData->setBaseColor(settings.value(CFG_COLORS_BASE, DEFAULT_COLORS_BASE).toString());
                profileData->setAmbientColor(settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString());
                profileData->setLightColor(settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString());

                // Principled Material
                profileData->setRoughness(settings.value(CFG_ROUGHNESS, DEFAULT_ROUGHNESS).toInt());
                profileData->setMetalness(settings.value(CFG_METALNESS, DEFAULT_METALNESS).toInt());
                int indexOfRefraction = settings.value(CFG_REFRACTION_TYPE, static_cast<int>(DEFAULT_REFRACTION_TYPE)).toInt();
                profileData->setIndexOfRefractionType(static_cast<IndexOfRefraction>(indexOfRefraction));
                profileData->setRefraction(settings.value(CFG_ROUGHNESS, DEFAULT_ROUGHNESS).toInt());

                // Specular Glossy Material
                profileData->setSpecularColor(settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString());
                profileData->setGlossiness(settings.value(CFG_GLOSSINESS, DEFAULT_GLOSSINESS).toInt());

                profileData->setLightBrightness(settings.value(CFG_LIGHT_BRIGHTNESS, DEFAULT_LIGHT_BRIGHTNESS).toInt());
                profileData->setIteration(settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble());
                profileData->setDecorationPath(settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString());
                profileData->setFont(settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString());
                profileData->setCustomVertexShader(settings.value(CFG_CUSTOM_VERT, DEFAULT_CUSTOM_VERT).toString());
                profileData->setCustomFragmentShader(settings.value(CFG_CUSTOM_FRAG, DEFAULT_CUSTOM_FRAG).toString());
            }

            ui->cboProfile->addItem(profileName);
            m_profiles.append(profileData);
        }
        settings.endArray();

        m_currentProfile = settings.value(CFG_CURRENT_PROFILE, DEFAULT_CURRENT_PROFILE).toInt();
    }

    ui->cboProfile->setCurrentIndex(m_currentProfile);

    populateCurrentProfileControls();

    // Twitch
    ui->lstExcludeChat->clear();
    QStringList excludeChatList = settings.value(CFG_EXCLUDE_CHAT, DEFAULT_EXCLUDE_CHAT).toStringList();
    excludeChatList.sort();
    ui->lstExcludeChat->addItems(excludeChatList);
    ui->edtClientId->setText(settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString());

    loadLogSettings();
}

void SetupWidget::saveSettings()
{
    bool fontWarn = false;
    QSettings settings;
    settings.setValue(CFG_LANGUAGE, ui->cboLanguage->currentData());
    settings.setValue(CFG_CURRENT_PROFILE, m_currentProfile);

    settings.beginWriteArray(CFG_PROFILES, m_profiles.size());
    for (qsizetype i = 0; i < m_profiles.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(CFG_VERSION, 2);
        settings.setValue(CFG_MATERIAL_TYPE, static_cast<int>(m_profiles[i]->materialType()));
        settings.setValue(CFG_PROFILE_NAME, m_profiles[i]->profileName());
        settings.setValue(CFG_COLORS_BASE, m_profiles[i]->baseColor());
        settings.setValue(CFG_COLORS_AMBIENT, m_profiles[i]->ambientColor());
        settings.setValue(CFG_COLORS_LIGHT, m_profiles[i]->lightColor());

        // Principled Material Properties
        settings.setValue(CFG_ROUGHNESS, m_profiles[i]->roughness());
        settings.setValue(CFG_METALNESS, m_profiles[i]->metalness());
        settings.setValue(CFG_REFRACTION_TYPE, static_cast<int>(m_profiles[i]->indexOfRefractionType()));
        settings.setValue(CFG_REFRACTION, m_profiles[i]->refraction());

        // Specular Glossy Material Properties
        settings.setValue(CFG_COLORS_SPECULAR, m_profiles[i]->specularColor());
        settings.setValue(CFG_GLOSSINESS, m_profiles[i]->glossiness());

        settings.setValue(CFG_LIGHT_BRIGHTNESS, m_profiles[i]->lightBrightness());
        settings.setValue(CFG_ITERATION_TIME, m_profiles[i]->iteration());
        settings.setValue(CFG_DECORATION_PATH, m_profiles[i]->decorationPath());
        settings.setValue(CFG_CUSTOM_VERT, m_profiles[i]->customVertexShader());
        settings.setValue(CFG_CUSTOM_FRAG, m_profiles[i]->customFragmentShader());

        QString newFont = m_profiles[i]->font();
        QString previousFont = settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
        if (newFont != previousFont && newFont != DEFAULT_EMOJI_FONT) {
            fontWarn = true;
        }

        settings.setValue(CFG_EMOJI_FONT, newFont);
    }
    settings.endArray();

    if (fontWarn) {
        QMessageBox::warning(this, tr("Changing default font"),
                             tr("Changing default font is not advised. SVG Based fonts may not work correctly on Windows, and non-SVG Based fonts may also fail to render correctly on other platforms"));
    }

    // Twitch
    QStringList excludeChat;

    for (int i = 0; i < ui->lstExcludeChat->count(); ++i) {
        excludeChat << (ui->lstExcludeChat->item(i)->text());
    }

    excludeChat.removeDuplicates();

    settings.setValue(CFG_EXCLUDE_CHAT, excludeChat);

    QString clientId = ui->edtClientId->text();
    if (clientId.isEmpty()) {
        QMessageBox::warning(this, tr("Empty ClientId"), tr("Twitch Client Id is empty, you'll be not able to connect to your chat, use the Twitch Dev Console button to create a Client on Twitch."));
    }
    settings.setValue(CFG_CLIENT_ID, ui->edtClientId->text());

    saveLogSettings();

    ui->btnSaveSettings->setEnabled(false);
    m_shouldSave = false;

    QMessageBox::information(this, tr("Success"), tr("Settings saved successfully!"));
}

void SetupWidget::checkClose()
{
    if (m_shouldSave) {
        int ret = QMessageBox::question(this, tr("There are unsaved changes"),
                                        tr("Do you want to save changes before running?"),
                                        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                                        QMessageBox::Yes);

        if (ret == QMessageBox::Yes) {
            saveSettings();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    QSettings settings;

    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();

    if (clientId.isEmpty()) {
        QMessageBox::critical(this, tr("Empty ClientId"), tr("Twitch Client Id is empty, you'll be not able to connect to your chat, use the Twitch Dev Console button to create a Client on Twitch."));
        return;
    }

    this->setEnabled(false);

    // It's only safe to launch a new 3D Window when the previous one was destroyed
    connect(m_previewWindow, &QQuickView::destroyed, [=]() {
        AtsumariLauncher* launcher = new AtsumariLauncher;
        launcher->launch();
    });
    this->deleteLater();
}

void SetupWidget::selectColor(QString(ProfileData::*getter)() const, void (ProfileData::*setter)(const QString&), QFrame *frame)
{
    QString currentColor = (m_profiles[m_currentProfile]->*getter)();
    QColorDialog* dlg = new QColorDialog(QColor(currentColor), this);

    dlg->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    connect(dlg, &QColorDialog::colorSelected, this, [=](const QColor& color) {
        (m_profiles[m_currentProfile]->*setter)(color.name());
        frame->setStyleSheet("background-color: " + color.name() + ";");
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
        dlg->deleteLater();
    });

    dlg->open();
}

void SetupWidget::selectDecoration()
{
    QFileDialog* dlg = new QFileDialog(this, tr("Select decoration file"), QString(), tr("Image Files (*.svg *.png *.jpg *.bmp)"));

    dlg->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    connect(dlg, &QFileDialog::fileSelected, this, [=](const QString& file) {
        m_profiles[m_currentProfile]->setDecorationPath(file);
        ui->lblDecoration->setPixmap(QPixmap(file));
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
        dlg->deleteLater();
    });

    dlg->open();
}

void SetupWidget::openDevConsole()
{
    QDesktopServices::openUrl(QUrl("https://dev.twitch.tv/console"));
}

void SetupWidget::resetAuth()
{
    QSettings settings;
    settings.setValue(CFG_TOKEN, "");

    QMessageBox::information(this, tr("Success"), tr("Authentication token has been reset! Twitch login will be required on next startup."));
}

void SetupWidget::setIcons()
{
#ifdef Q_OS_WIN
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark) {
        QIcon::setThemeName("breeze-dark");
    } else {
        QIcon::setThemeName("breeze");
    }
#endif
    ui->btnProfileActions->setIcon(QIcon::fromTheme("application-menu"));
    ui->btnSelectBaseColor->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectSpecular->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectAmbient->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectLight->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnResetDecoration->setIcon(QIcon::fromTheme("edit-undo"));
    ui->btnSelectDecoration->setIcon(QIcon::fromTheme("document-open"));
    ui->btnAddExcludeChat->setIcon(QIcon::fromTheme("list-add"));
    ui->btnRemoveExcludeChat->setIcon(QIcon::fromTheme("list-remove"));
    ui->btnSaveSettings->setIcon(QIcon::fromTheme("document-save"));
    ui->btnClose->setIcon(QIcon::fromTheme("system-run"));
    ui->btnDevConsole->setIcon(QIcon::fromTheme("applications-development"));
    ui->btnForceAuth->setIcon(QIcon::fromTheme("security-high"));
    ui->btnAboutQt->setIcon(QIcon::fromTheme("help-about"));
    ui->tabWidget->setTabIcon(0, QIcon::fromTheme("configure"));
    ui->tabWidget->setTabIcon(1, QIcon::fromTheme("computer"));
    ui->tabWidget->setTabIcon(2, QIcon::fromTheme("dialog-messages"));
    ui->tabWidget->setTabIcon(3, QIcon::fromTheme("help-about"));
}

void SetupWidget::populateLanguages()
{
    ui->cboLanguage->clear();

    QList<QLocale> availableLocales = LocaleHelper::availableLocales();
    std::sort(availableLocales.begin(), availableLocales.end(), [=](QLocale l1, QLocale l2) {
        QString s1 = l1.nativeLanguageName();
        s1.front() = s1.front().toUpper();
        s1 = QString("%1 (%2)").arg(s1, l1.nativeTerritoryName());

        QString s2 = l2.nativeLanguageName();
        s2.front() = s2.front().toUpper();
        s2 = QString("%1 (%2)").arg(s2, l2.nativeTerritoryName());
        return s1 < s2;
    });

    for (const QLocale& l: availableLocales) {
        QString s = l.nativeLanguageName();
        s.front() = s.front().toUpper();
        s = QString("%1 (%2)").arg(s, l.nativeTerritoryName());
        ui->cboLanguage->addItem(s, l);
    }
}

void SetupWidget::populateMaterialTypes()
{
    ui->cboMaterialType->clear();
    ui->cboMaterialType->addItem(tr("Principled Material"), static_cast<int>(MaterialType::Principled));
    ui->cboMaterialType->addItem(tr("Specular Glossy Material"), static_cast<int>(MaterialType::SpecularGlossy));
    ui->cboMaterialType->addItem(tr("Custom Material"), static_cast<int>(MaterialType::Custom));
}

void SetupWidget::populateRefractionTypes()
{
    ui->cboRefraction->clear();
    ui->cboRefraction->addItem(tr("Custom"), static_cast<int>(IndexOfRefraction::Custom));
    ui->cboRefraction->addItem(tr("Air"), static_cast<int>(IndexOfRefraction::Air));
    ui->cboRefraction->addItem(tr("Water"), static_cast<int>(IndexOfRefraction::Water));
    ui->cboRefraction->addItem(tr("Glass"), static_cast<int>(IndexOfRefraction::Glass));
    ui->cboRefraction->addItem(tr("Sapphire"), static_cast<int>(IndexOfRefraction::Sapphire));
    ui->cboRefraction->addItem(tr("Diamond"), static_cast<int>(IndexOfRefraction::Diamond));
}

void SetupWidget::setupPreview()
{
    // Create QQuickView for preview using the same QML as standalone
    m_previewWindow = new QQuickView;
    m_previewWindow->setColor(Qt::black);
    m_previewWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    
    // Set the same QML source as standalone window
    m_previewWindow->setSource(QUrl("qrc:/main.qml"));
    
    // Add to preview group
    ui->grpPreview->layout()->addWidget(QWidget::createWindowContainer(m_previewWindow));
    m_previewWindow->show();
    
    // Get the root QML object for property updates
    m_previewRootItem = m_previewWindow->rootObject();
}

void SetupWidget::newProfile()
{
    bool ok = true;
    QString profileName;
    QStringList existingProfiles;

    for (const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        profileName = QInputDialog::getText(this, tr("New Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            tr("Profile"), &ok);

        if (!ok) {
            return;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while (existingProfiles.contains(profileName));

    ProfileData* profile = new ProfileData;
    profile->setProfileName(profileName);
    profile->setMaterialType(DEFAULT_MATERIAL_TYPE);
    profile->setBaseColor(DEFAULT_COLORS_BASE);
    profile->setAmbientColor(DEFAULT_COLORS_AMBIENT);
    profile->setLightColor(DEFAULT_COLORS_LIGHT);
    profile->setRoughness(DEFAULT_ROUGHNESS);
    profile->setMetalness(DEFAULT_METALNESS);
    profile->setIndexOfRefractionType(DEFAULT_REFRACTION_TYPE);
    profile->setRefraction(DEFAULT_REFRACTION);
    profile->setSpecularColor(DEFAULT_COLORS_SPECULAR);
    profile->setGlossiness(DEFAULT_GLOSSINESS);
    profile->setCustomVertexShader(DEFAULT_CUSTOM_VERT);
    profile->setCustomFragmentShader(DEFAULT_CUSTOM_FRAG);
    profile->setDecorationPath(DEFAULT_DECORATION_PATH);
    profile->setLightBrightness(DEFAULT_LIGHT_BRIGHTNESS);
    profile->setIteration(DEFAULT_ITERATION_TIME);
    profile->setFont(DEFAULT_EMOJI_FONT);

    m_profiles.append(profile);
    ui->cboProfile->addItem(profileName);
    ui->cboProfile->setCurrentIndex(ui->cboProfile->count() - 1);
    m_deleteProfileAction->setEnabled(true);

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::duplicateProfile()
{
    bool ok = true;
    QString profileName;
    QStringList existingProfiles;
    ProfileData* currentProfile = m_profiles[m_currentProfile];
    QString currentName = currentProfile->profileName();

    for (const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        //: Note to translators: this refers to the action of copying/duplicating a profile
        profileName = QInputDialog::getText(this, tr("Duplicate Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            tr("Copy of %1").arg(currentName), &ok);

        if (!ok) {
            return;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while (existingProfiles.contains(profileName));

    ProfileData* profile = new ProfileData(*currentProfile);
    profile->setProfileName(profileName);

    m_profiles.append(profile);
    ui->cboProfile->addItem(profileName);
    ui->cboProfile->setCurrentIndex(ui->cboProfile->count() - 1);
    m_deleteProfileAction->setEnabled(true);

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::renameProfile()
{
    bool ok = true;
    QString profileName;
    QStringList existingProfiles;
    ProfileData* currentProfile = m_profiles[m_currentProfile];
    QString currentName = currentProfile->profileName();

    for (const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        profileName = QInputDialog::getText(this, tr("Rename Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            currentName, &ok);

        if (!ok || (profileName == currentName)) {
            return;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while (existingProfiles.contains(profileName));

    currentProfile->setProfileName(profileName);

    m_rebuildingCombo = true;
    ui->cboProfile->clear();
    for (const ProfileData* pd: m_profiles) {
        ui->cboProfile->addItem(pd->profileName());
    }
    ui->cboProfile->setCurrentIndex(m_currentProfile);
    m_rebuildingCombo = false;
    populateCurrentProfileControls();

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::deleteProfile()
{
    if (m_profiles.size() == 1) {
        QMessageBox::critical(this, tr("Cannot delete profile"),
                              tr("Cannot delete the only remaining profile."));
        return;
    }

    ProfileData* currentProfile = m_profiles[m_currentProfile];
    QString currentName = currentProfile->profileName();

    int ret = QMessageBox::question(this, tr("Delete Profile"),
                                    tr("Are you sure you want to delete profile '%1'?").arg(currentName));

    if (ret == QMessageBox::Yes) {
        m_profiles.removeAt(m_currentProfile);
        delete currentProfile;
    }
    m_currentProfile = 0;

    if (m_profiles.size() == 1) {
        m_deleteProfileAction->setEnabled(false);
    }

    m_rebuildingCombo = true;
    ui->cboProfile->clear();
    for (const ProfileData* pd: m_profiles) {
        ui->cboProfile->addItem(pd->profileName());
    }
    m_rebuildingCombo = false;
    ui->cboProfile->setCurrentIndex(qMax(0, m_currentProfile - 1));
    populateCurrentProfileControls();

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::createProfileMenus()
{
    // delete previous entries if not already deleted
    if (m_newProfileAction) {
        m_newProfileAction->deleteLater();
    }
    if (m_duplicateProfileAction) {
        m_duplicateProfileAction->deleteLater();
    }
    if (m_renameProfileAction) {
        m_renameProfileAction->deleteLater();
    }
    if (m_deleteProfileAction) {
        m_deleteProfileAction->deleteLater();
    }
    if (m_profileMenu) {
        m_profileMenu->deleteLater();
    }

    m_profileMenu = new QMenu(ui->btnProfileActions);
    m_newProfileAction = m_profileMenu->addAction(QIcon::fromTheme("document-new"), tr("New Profile..."));
    m_duplicateProfileAction = m_profileMenu->addAction(QIcon::fromTheme("document-new-from-template"), tr("Duplicate this profile..."));
    m_renameProfileAction = m_profileMenu->addAction(QIcon::fromTheme("document-edit"), tr("Rename this Profile"));
    m_deleteProfileAction = m_profileMenu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete this Profile"));
    ui->btnProfileActions->setMenu(m_profileMenu);

    connect(m_newProfileAction, &QAction::triggered, this, &SetupWidget::newProfile);
    connect(m_duplicateProfileAction, &QAction::triggered, this, &SetupWidget::duplicateProfile);
    connect(m_renameProfileAction, &QAction::triggered, this, &SetupWidget::renameProfile);
    connect(m_deleteProfileAction, &QAction::triggered, this, &SetupWidget::deleteProfile);

    if (m_profiles.size() == 1) {
        m_deleteProfileAction->setEnabled(false);
    }
}

void SetupWidget::populateCurrentProfileControls()
{
    if (m_rebuildingCombo) {
        return;
    }

    m_currentProfile = qMax(0, ui->cboProfile->currentIndex());

    const ProfileData* p = m_profiles[m_currentProfile];

    ui->cboMaterialType->setCurrentIndex(static_cast<int>(p->materialType()));

    // Initialize GroupBox visibility based on material type
    ui->grpPrincipled->setVisible(p->materialType() == MaterialType::Principled);
    ui->grpSpecularGlossy->setVisible(p->materialType() == MaterialType::SpecularGlossy);
    ui->grpCustomMaterial->setVisible(p->materialType() == MaterialType::Custom);

    {
        QSignalBlocker b1(ui->txtVertexShader);
        QSignalBlocker b2(ui->txtFragmentShader);
        ui->txtVertexShader->setPlainText(p->customVertexShader());
        ui->txtFragmentShader->setPlainText(p->customFragmentShader());
    }
    ui->btnReloadShaders->setEnabled(false);

    bool isCustom = p->materialType() == MaterialType::Custom;
    ui->frmBaseColor->setEnabled(!isCustom);
    ui->btnSelectBaseColor->setEnabled(!isCustom);
    ui->frmBaseColor->setStyleSheet(QString("background-color: %1;").arg(p->baseColor()));
    ui->frmAmbientColor->setStyleSheet(QString("background-color: %1;").arg(p->ambientColor()));
    ui->frmLightColor->setStyleSheet(QString("background-color: %1;").arg(p->lightColor()));
    ui->frmSpecularColor->setStyleSheet(QString("background-color: %1;").arg(p->specularColor()));

    ui->sldRoughness->setValue(p->roughness());
    ui->spnRoughness->setValue(p->roughness() / 100.0);
    ui->sldMetalness->setValue(p->metalness());
    ui->spnMetalness->setValue(p->metalness() / 100.0);
    ui->cboRefraction->setCurrentIndex(static_cast<int>(p->indexOfRefractionType()));
    ui->sldRefraction->setEnabled(p->indexOfRefractionType() == IndexOfRefraction::Custom);
    ui->spnRefraction->setEnabled(p->indexOfRefractionType() == IndexOfRefraction::Custom);
    ui->sldRefraction->setValue(p->refraction());
    ui->spnRefraction->setValue(p->refraction() / 100.0);

    ui->sldGlossiness->setValue(p->glossiness());
    ui->spnGlossiness->setValue(p->glossiness() / 100.0);

    ui->sldLightBrightness->setValue(p->lightBrightness());
    ui->spnLightBrightness->setValue(p->lightBrightness() / 100.0);
    ui->spnIteration->setValue(p->iteration());
    ui->lblDecoration->setPixmap(p->decorationPath());
    ui->cboEmojiFont->setCurrentFont(QFont(p->font(), -1));
}

void SetupWidget::closeEvent(QCloseEvent *event)
{
    if (m_shouldSave) {
        int ret = QMessageBox::question(this, tr("There are unsaved changes"),
                                        tr("Do you want to save changes before closing?"),
                                        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                                        QMessageBox::Cancel);

        if (ret == QMessageBox::No) {
            event->accept();
        } else if (ret == QMessageBox::Yes) {
            saveSettings();
            event->accept();
        } else {
            event->ignore();
            return;
        }
    }

    qGuiApp->quit();
}

void SetupWidget::aboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void SetupWidget::addToExcludeList()
{
    QString userName = ui->edtExcludeChat->text();
    if (userName.startsWith("@")) {
        userName = userName.mid(1);
    }
    userName = userName.toLower();

    ui->lstExcludeChat->addItem(userName);
    ui->edtExcludeChat->clear();

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
}

void SetupWidget::removeFromExcludeList()
{
    QListWidgetItem* item = ui->lstExcludeChat->currentItem();
    if (item) {
        delete ui->lstExcludeChat->takeItem(ui->lstExcludeChat->row(item));

        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
    }
}

void SetupWidget::loadLogSettings()
{
    QSettings settings;
    QStringList cols = settings.value(CFG_LOG_COLUMNS, DEFAULT_LOG_COLUMNS).toStringList();
    ui->chkDirection->setChecked(cols.contains("Direction"));
    ui->chkTimestamp->setChecked(cols.contains("Timestamp"));
    ui->chkCommand->setChecked(cols.contains("Command"));
    ui->chkSender->setChecked(cols.contains("Sender"));
    ui->chkMessage->setChecked(cols.contains("Message"));
    ui->chkTags->setChecked(cols.contains("Tags"));
    ui->chkEmotes->setChecked(cols.contains("Emotes"));

    ui->tblCommandColors->setColumnCount(4);
    ui->tblCommandColors->setHorizontalHeaderLabels(QStringList() << tr("Command") << tr("Show") << tr("Foreground") << tr("Background"));
    QStringList cmds = DEFAULT_LOG_COMMANDS;
    QStringList hidden = settings.value(CFG_LOG_HIDE_CMDS, DEFAULT_LOG_HIDE_CMDS).toStringList();
    ui->tblCommandColors->setRowCount(cmds.size());
    for (int i = 0; i < cmds.size(); ++i) {
        QString cmd = cmds.at(i);
        settings.beginGroup(QStringLiteral("log/colors/%1").arg(cmd));
        const auto defaults = defaultCommandColors(cmd);
        QColor fg = settings.value("fg", defaults.first).value<QColor>();
        QColor bg = settings.value("bg", defaults.second).value<QColor>();
        settings.endGroup();

        QTableWidgetItem* cmdItem = new QTableWidgetItem(cmd);
        cmdItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->tblCommandColors->setItem(i, 0, cmdItem);

        QTableWidgetItem* showItem = new QTableWidgetItem();
        showItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        showItem->setCheckState(hidden.contains(cmd) ? Qt::Unchecked : Qt::Checked);
        ui->tblCommandColors->setItem(i, 1, showItem);

        QTableWidgetItem* fgItem = new QTableWidgetItem(fg.name());
        fgItem->setForeground(fg);
        fgItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->tblCommandColors->setItem(i, 2, fgItem);

        QTableWidgetItem* bgItem = new QTableWidgetItem(bg.name());
        bgItem->setBackground(bg);
        bgItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->tblCommandColors->setItem(i, 3, bgItem);
    }

    connect(ui->tblCommandColors, &QTableWidget::itemDoubleClicked, this, [=](QTableWidgetItem* item) {
        QColor initial;
        if (item->column() == 2)
            initial = item->foreground().color();
        else if (item->column() == 3)
            initial = item->background().color();
        else
            return;
        QColor c = QColorDialog::getColor(initial, this, tr("Select color"));
        if (!c.isValid()) return;
        if (item->column() == 2) {
            item->setForeground(c);
            item->setText(c.name());
        } else {
            item->setBackground(c);
            item->setText(c.name());
        }
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
    });

    auto markDirty = [=]() {
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
    };

    connect(ui->tblCommandColors, &QTableWidget::itemChanged, this, [=](QTableWidgetItem* item) {
        if (item->column() == 1)
            markDirty();
    });

    connect(ui->chkDirection, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkTimestamp, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkCommand, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkSender, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkMessage, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkTags, &QCheckBox::checkStateChanged, this, markDirty);
    connect(ui->chkEmotes, &QCheckBox::checkStateChanged, this, markDirty);
}

void SetupWidget::saveLogSettings()
{
    QSettings settings;
    QStringList cols;
    if (ui->chkDirection->isChecked()) cols << "Direction";
    if (ui->chkTimestamp->isChecked()) cols << "Timestamp";
    if (ui->chkCommand->isChecked()) cols << "Command";
    if (ui->chkSender->isChecked()) cols << "Sender";
    if (ui->chkMessage->isChecked()) cols << "Message";
    if (ui->chkTags->isChecked()) cols << "Tags";
    if (ui->chkEmotes->isChecked()) cols << "Emotes";
    settings.setValue(CFG_LOG_COLUMNS, cols);

    settings.beginGroup("log/colors");
    settings.remove("");
    QStringList hidden;
    for (int r = 0; r < ui->tblCommandColors->rowCount(); ++r) {
        QString cmd = ui->tblCommandColors->item(r,0)->text();
        bool show = ui->tblCommandColors->item(r,1)->checkState() == Qt::Checked;
        if (!show)
            hidden << cmd;
        QColor fg = ui->tblCommandColors->item(r,2)->foreground().color();
        QColor bg = ui->tblCommandColors->item(r,3)->background().color();
        settings.beginGroup(cmd);
        settings.setValue("fg", fg);
        settings.setValue("bg", bg);
        settings.endGroup();
    }
    settings.endGroup();
    settings.setValue(CFG_LOG_HIDE_CMDS, hidden);
}
