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
#include "./ui_setupwidget.h"

#include <algorithm>

#include <QSettings>
#include <QColorDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStyleHints>
#include <QFileInfo>
#include <QMenu>

#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QForwardRenderer>

#include "settings_defaults.h"
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
    , ui(new Ui::SetupWidget)
{
    ui->setupUi(this);
    setIcons();
    populateLanguages();
    loadSettings();

    // Appearance tab
    connect(ui->cboProfile, &QComboBox::currentIndexChanged, this, [=]() {
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        populateCurrentProfileControls();
        runPreview();
    });

    createProfileMenus();

    connect(ui->btnSelectDiffuse, &QPushButton::clicked, this, [=]() {
        selectColor(&ProfileData::diffuseColor, &ProfileData::setDiffuseColor, ui->frmDiffuseColor);
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

    // any of those values changing may trigger a preview update
    connect(ui->spnSlices, &QSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setSlices(ui->spnSlices->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnRings, &QSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setRings(ui->spnRings->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->spnIteration, &QDoubleSpinBox::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setIteration(ui->spnIteration->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldLightIntensity, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setLightIntensity(ui->sldLightIntensity->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->sldShininess, &QSlider::valueChanged, this, [=]() {
        m_profiles[m_currentProfile]->setShininess(ui->sldShininess->value());
        m_shouldSave = true;
        ui->btnSaveSettings->setEnabled(true);
        runPreview();
    });

    connect(ui->btnResetDecoration, &QPushButton::clicked, this, &SetupWidget::resetDecoration);
    connect(ui->btnSelectDecoration, &QPushButton::clicked, this, &SetupWidget::selectDecoration);

    setupPreview();
    runPreview();

    // Directories tab
    connect(ui->btnEmojiPath, &QPushButton::clicked, this, &SetupWidget::selectEmojiPath);
    connect(ui->btnEmotePath, &QPushButton::clicked, this, &SetupWidget::selectEmotePath);
    connect(ui->edtEmojiDir, &QLineEdit::textChanged, this, [=]() { validatePaths(ui->edtEmojiDir); });
    connect(ui->edtEmoteDir, &QLineEdit::textChanged, this, [=]() { validatePaths(ui->edtEmoteDir); });

    // Twitch Tab
    connect(ui->btnAddExcludeChat, &QPushButton::clicked, this, &SetupWidget::addToExcludeList);
    connect(ui->btnRemoveExcludeChat, &QPushButton::clicked, this, &SetupWidget::removeFromExcludeList);
    connect(ui->btnDevConsole, &QPushButton::clicked, this, &SetupWidget::openDevConsole);
    connect(ui->btnForceAuth, &QPushButton::clicked, this, &SetupWidget::resetAuth);

    // About
    connect(ui->btnAboutQt, &QPushButton::clicked, this, &SetupWidget::aboutQt);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [=](int index) {
        if ( index == 3 ) {
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
    } );

    // Actions
    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &SetupWidget::saveSettings);
    connect(ui->btnClose, &QPushButton::clicked, this, &SetupWidget::checkClose);
}

SetupWidget::~SetupWidget()
{
    delete ui;
    m_lightTransform->deleteLater();
    m_light->deleteLater();
    m_lightEntity->deleteLater();
    m_previewEntity->deleteLater();
    m_cameraController->deleteLater();
    m_rootEntity->deleteLater();
    m_camera->deleteLater();
    m_previewWindow->deleteLater();
}

void SetupWidget::runPreview()
{
    m_previewEntity->setSlices(ui->spnSlices->value());
    m_previewEntity->setRings(ui->spnRings->value());
    m_previewEntity->setDiffuse(m_profiles[m_currentProfile]->diffuseColor());
    m_previewEntity->setSpecular(m_profiles[m_currentProfile]->specularColor());
    m_previewEntity->setAmbient(m_profiles[m_currentProfile]->ambientColor());
    m_previewEntity->setShininess(ui->sldShininess->value());
    m_previewEntity->setIterationInterval(ui->spnIteration->value());

    QUrl kata_deco = QUrl::fromLocalFile(m_profiles[m_currentProfile]->decorationPath());

    m_previewEntity->clearEmotes();

    m_previewEntity->addEmote(kata_deco, 0.00f, 0.00f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 0.00f, 180.00f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 0.00f, 63.435f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 72.0f, 63.435f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 144.0f, 63.435f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 216.0f, 63.435f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 288.0f, 63.435f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 36.0f, 116.565f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 108.0f, 116.565f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 180.0f, 116.565f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 252.0f, 116.565f, 0.35f);
    m_previewEntity->addEmote(kata_deco, 324.0f, 116.565f, 0.35f);

    // lighting
    m_light->setColor(m_profiles[m_currentProfile]->lightColor());
    m_light->setIntensity(ui->sldLightIntensity->value() / 100.0f);
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
    if ( langIndex == -1 ) {
        langIndex = ui->cboLanguage->findData(LocaleHelper::findBestLocale());
    }
    ui->cboLanguage->setCurrentIndex(langIndex);

    // profiles
    int size = settings.beginReadArray(CFG_PROFILES);
    if ( size == 0 ) {
        settings.endArray();

        // populate default profile
        m_currentProfile = 0;
        ProfileData* profileData = new ProfileData;
        QString defaultProfileName = tr("Default");
        profileData->setProfileName(defaultProfileName);
        profileData->setDiffuseColor(DEFAULT_COLORS_DIFFUSE);
        profileData->setSpecularColor(DEFAULT_COLORS_SPECULAR);
        profileData->setAmbientColor(DEFAULT_COLORS_AMBIENT);
        profileData->setLightColor(DEFAULT_COLORS_LIGHT);
        profileData->setDecorationPath(DEFAULT_DECORATION_PATH);
        profileData->setLightIntensity(DEFAULT_LIGHT_INTENSITY);
        profileData->setSlices(DEFAULT_SLICES);
        profileData->setRings(DEFAULT_RINGS);
        profileData->setShininess(DEFAULT_SHININESS);
        profileData->setIteration(DEFAULT_ITERATION_TIME);
        profileData->setFont(DEFAULT_EMOJI_FONT);

        m_profiles.append(profileData);

        ui->cboProfile->addItem(defaultProfileName);
    } else {
        for(qsizetype i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            ProfileData* profileData = new ProfileData;
            QString profileName = settings.value(CFG_PROFILE_NAME).toString();

            profileData->setProfileName(profileName);
            profileData->setDiffuseColor(settings.value(CFG_COLORS_DIFFUSE, DEFAULT_COLORS_DIFFUSE).toString());
            profileData->setSpecularColor(settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString());
            profileData->setAmbientColor(settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString());
            profileData->setLightColor(settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString());
            profileData->setDecorationPath(settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString());
            profileData->setLightIntensity(settings.value(CFG_LIGHT_INTENSITY, DEFAULT_LIGHT_INTENSITY).toInt());
            profileData->setSlices(settings.value(CFG_SLICES, DEFAULT_SLICES).toInt());
            profileData->setRings(settings.value(CFG_RINGS, DEFAULT_RINGS).toInt());
            profileData->setShininess(settings.value(CFG_SHININESS, DEFAULT_SHININESS).toInt());
            profileData->setIteration(settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble());
            profileData->setFont(settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString());

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
    ui->lstExcludeChat->addItems(settings.value(CFG_EXCLUDE_CHAT).toStringList());
    ui->edtClientId->setText(settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString());

    // Directories
    ui->edtEmojiDir->setText(settings.value(CFG_EMOJI_DIR, DEFAULT_EMOJI_DIR).toString());
    ui->edtEmoteDir->setText(settings.value(CFG_EMOTE_DIR, DEFAULT_EMOTE_DIR).toString());
}

void SetupWidget::saveSettings()
{
    bool fontWarn = false;
    QSettings settings;
    settings.setValue(CFG_LANGUAGE, ui->cboLanguage->currentData());

    settings.beginWriteArray(CFG_PROFILES, m_profiles.size());
    for(qsizetype i = 0; i < m_profiles.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(CFG_PROFILE_NAME, m_profiles[i]->profileName());
        settings.setValue(CFG_COLORS_DIFFUSE, m_profiles[i]->diffuseColor());
        settings.setValue(CFG_COLORS_SPECULAR, m_profiles[i]->specularColor());
        settings.setValue(CFG_COLORS_AMBIENT, m_profiles[i]->ambientColor());
        settings.setValue(CFG_COLORS_LIGHT, m_profiles[i]->lightColor());
        settings.setValue(CFG_LIGHT_INTENSITY, m_profiles[i]->lightIntensity());
        settings.setValue(CFG_SLICES, m_profiles[i]->slices());
        settings.setValue(CFG_RINGS, m_profiles[i]->rings());
        settings.setValue(CFG_SHININESS, m_profiles[i]->shininess());
        settings.setValue(CFG_ITERATION_TIME, m_profiles[i]->iteration());
        settings.setValue(CFG_DECORATION_PATH, m_profiles[i]->decorationPath());

        QString newFont = ui->cboEmojiFont->currentFont().family();
        QString previousFont = settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
        if ( newFont != previousFont && newFont != DEFAULT_EMOJI_FONT) {
            fontWarn = true;
        }

        settings.setValue(CFG_EMOJI_FONT, newFont);
    }
    settings.endArray();

    if ( fontWarn ) {
        QMessageBox::warning(this, tr("Changing default font"),
                             tr("Changing default font is not advised. SVG Based fonts may not work correctly on Windows, and non-SVG Based fonts may also fail to render correctly on other platforms"));
    }

    // Directories
    settings.setValue(CFG_EMOJI_DIR, ui->edtEmojiDir->text());
    settings.setValue(CFG_EMOTE_DIR, ui->edtEmoteDir->text());

    // Twitch
    QStringList excludeChat;

    for(int i = 0; i < ui->lstExcludeChat->count(); ++i){
        excludeChat << (ui->lstExcludeChat->item(i)->text());
    }

    excludeChat.removeDuplicates();

    settings.setValue(CFG_EXCLUDE_CHAT, excludeChat);

    QString clientId = ui->edtClientId->text();
    if ( clientId.isEmpty() ) {
        QMessageBox::warning(this, tr("Empty ClientId"), tr("Twitch Client Id is empty, you'll be not able to connect to your chat, use the Twitch Dev Console button to create a Client on Twitch."));
    }
    settings.setValue(CFG_CLIENT_ID, ui->edtClientId->text());

    ui->btnSaveSettings->setEnabled(false);
    m_shouldSave = false;

    QMessageBox::information(this, tr("Success"), tr("Settings saved successfully!"));

}

void SetupWidget::checkClose()
{
    QSettings settings;

    QString clientId = settings.value(CFG_CLIENT_ID, DEFAULT_CLIENT_ID).toString();

    if ( clientId.isEmpty() ) {
        QMessageBox::critical(this, tr("Empty ClientId"), tr("Twitch Client Id is empty, you'll be not able to connect to your chat, use the Twitch Dev Console button to create a Client on Twitch."));
        return;
    }

    this->setEnabled(false);
    AtsumariLauncher* launcher = new AtsumariLauncher;
    launcher->launch();
    QTimer::singleShot(1000, this, [=]() { this->deleteLater(); } );
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

void SetupWidget::selectEmotePath()
{
    ui->edtEmoteDir->setText(QFileDialog::getExistingDirectory(this, tr("Select emotes directory"), ui->edtEmoteDir->text()));
}

void SetupWidget::selectEmojiPath()
{
    ui->edtEmojiDir->setText(QFileDialog::getExistingDirectory(this, tr("Select emojis directory"), ui->edtEmoteDir->text()));
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
    if ( scheme == Qt::ColorScheme::Dark ) {
        QIcon::setThemeName("breeze-dark");
    } else {
        QIcon::setThemeName("breeze");
    }
#endif
    ui->btnProfileActions->setIcon(QIcon::fromTheme("application-menu"));
    ui->btnSelectDiffuse->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectSpecular->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectAmbient->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnSelectLight->setIcon(QIcon::fromTheme("color-picker"));
    ui->btnResetDecoration->setIcon(QIcon::fromTheme("edit-undo"));
    ui->btnSelectDecoration->setIcon(QIcon::fromTheme("document-open"));
    ui->btnAddExcludeChat->setIcon(QIcon::fromTheme("list-add"));
    ui->btnRemoveExcludeChat->setIcon(QIcon::fromTheme("list-remove"));
    ui->btnSaveSettings->setIcon(QIcon::fromTheme("document-save"));
    ui->btnClose->setIcon(QIcon::fromTheme("system-run"));
    ui->btnEmojiPath->setIcon(QIcon::fromTheme("folder-open"));
    ui->btnEmotePath->setIcon(QIcon::fromTheme("folder-open"));
    ui->btnDevConsole->setIcon(QIcon::fromTheme("applications-development"));
    ui->btnForceAuth->setIcon(QIcon::fromTheme("security-high"));
    ui->btnAboutQt->setIcon(QIcon::fromTheme("help-about"));
    ui->tabWidget->setTabIcon(0, QIcon::fromTheme("configure"));
    ui->tabWidget->setTabIcon(1, QIcon::fromTheme("folder"));
    ui->tabWidget->setTabIcon(2, QIcon::fromTheme("computer"));
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

    for(const QLocale& l: availableLocales) {
        QString s = l.nativeLanguageName();
        s.front() = s.front().toUpper();
        s = QString("%1 (%2)").arg(s, l.nativeTerritoryName());
        ui->cboLanguage->addItem(s, l);
    }
}

void SetupWidget::setupPreview()
{
    // preview window
    m_previewWindow = new Qt3DExtras::Qt3DWindow;
    m_previewWindow->defaultFrameGraph()->setClearColor(QColor(Qt::black));

    ui->grpPreview->layout()->addWidget(QWidget::createWindowContainer(m_previewWindow));
    m_previewWindow->show();

    // Setting up Camera
    m_camera = m_previewWindow->camera();
    m_camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_camera->setPosition(QVector3D(0, 0, 3.0));
    m_camera->setViewCenter(QVector3D(0, 0, 0));

    // Root Entity
    m_rootEntity = new Qt3DCore::QEntity();

    // Adding Atsumari
    m_previewEntity = new Atsumari(m_rootEntity);

    // Camera Controller
    m_cameraController = new Qt3DExtras::QOrbitCameraController(m_rootEntity);
    m_cameraController->setCamera(m_camera);

    // Add Lighting
    m_lightEntity = new Qt3DCore::QEntity(m_rootEntity);
    m_light = new Qt3DRender::QPointLight(m_lightEntity);
    m_lightTransform = new Qt3DCore::QTransform(m_lightEntity);

    m_lightTransform->setTranslation(m_camera->position());
    m_lightEntity->addComponent(m_light);
    m_lightEntity->addComponent(m_lightTransform);

    m_previewWindow->setRootEntity(m_rootEntity);
}

void SetupWidget::validatePaths(QLineEdit *edt)
{
    QString path = edt->text();

    QFileInfo fi(path);

    if(fi.isRelative()) {
        edt->setStyleSheet("color: red");
    } else {
        if(fi.exists()) {
            if(fi.isDir()) {
                edt->setStyleSheet(QString());
            } else {
                edt->setStyleSheet("color: red");
            }
        } else {
            edt->setStyleSheet("color: green");
        }
    }
}

void SetupWidget::newProfile()
{
    bool ok = true;
    QString profileName;
    QStringList existingProfiles;

    for(const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        profileName = QInputDialog::getText(this, tr("New Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            tr("Profile"), &ok);

        if ( !ok ) {
            break;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while(existingProfiles.contains(profileName));

    ProfileData* profile = new ProfileData;
    profile->setProfileName(profileName);
    profile->setDiffuseColor(DEFAULT_COLORS_DIFFUSE);
    profile->setSpecularColor(DEFAULT_COLORS_SPECULAR);
    profile->setAmbientColor(DEFAULT_COLORS_AMBIENT);
    profile->setLightColor(DEFAULT_COLORS_LIGHT);
    profile->setDecorationPath(DEFAULT_DECORATION_PATH);
    profile->setLightIntensity(DEFAULT_LIGHT_INTENSITY);
    profile->setSlices(DEFAULT_SLICES);
    profile->setRings(DEFAULT_RINGS);
    profile->setShininess(DEFAULT_SHININESS);
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

    for(const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        //: Note to translators: this refers to the action of copying/duplicating a profile
        profileName = QInputDialog::getText(this, tr("Duplicate Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            tr("Copy of %1").arg(currentName), &ok);

        if ( !ok ) {
            return;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while(existingProfiles.contains(profileName));

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

    for(const ProfileData* pd: m_profiles) {
        existingProfiles << pd->profileName();
    }

    do {
        profileName = QInputDialog::getText(this, tr("Rename Profile"),
                                            tr("Profile name:"), QLineEdit::Normal,
                                            currentName, &ok);

        if ( !ok || (profileName == currentName) ) {
            return;
        }

        if (existingProfiles.contains(profileName)) {
            QMessageBox::critical(this, tr("Profile already exists!"), tr("A profile named '%1' already exists. Try a different name.").arg(profileName));
        }
    } while(existingProfiles.contains(profileName));

    currentProfile->setProfileName(profileName);

    m_rebuildingCombo = true;
    ui->cboProfile->clear();
    for(const ProfileData* pd: m_profiles) {
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
    if ( m_profiles.size() == 1 ) {
        QMessageBox::critical(this, tr("Cannot delete profile"),
                              tr("Cannot delete the only remaining profile."));
        return;
    }

    ProfileData* currentProfile = m_profiles[m_currentProfile];
    QString currentName = currentProfile->profileName();

    int ret = QMessageBox::question(this, tr("Delete Profile"),
                                   tr("Are you sure you want to delete profile '%1'?").arg(currentName));

    if(ret == QMessageBox::Yes) {
        m_profiles.removeAt(m_currentProfile);
        delete currentProfile;
    }
    m_currentProfile = 0;

    if ( m_profiles.size() == 1 ) {
        m_deleteProfileAction->setEnabled(false);
    }

    m_rebuildingCombo = true;
    ui->cboProfile->clear();
    for(const ProfileData* pd: m_profiles) {
        ui->cboProfile->addItem(pd->profileName());
    }
    m_rebuildingCombo = false;
    ui->cboProfile->setCurrentIndex(qMin(0, m_currentProfile - 1));
    populateCurrentProfileControls();

    m_shouldSave = true;
    ui->btnSaveSettings->setEnabled(true);
    runPreview();
}

void SetupWidget::createProfileMenus()
{
    // delete previous entries if not already deleted
    if ( m_newProfileAction ) {
        m_newProfileAction->deleteLater();
    }
    if ( m_duplicateProfileAction ) {
        m_duplicateProfileAction->deleteLater();
    }
    if ( m_renameProfileAction ) {
        m_renameProfileAction->deleteLater();
    }
    if ( m_deleteProfileAction ) {
        m_deleteProfileAction->deleteLater();
    }
    if ( m_profileMenu ) {
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

    if ( m_profiles.size() == 1 ) {
        m_deleteProfileAction->setEnabled(false);
    }
}

void SetupWidget::populateCurrentProfileControls()
{
    if ( m_rebuildingCombo ) {
        return;
    }

    m_currentProfile = ui->cboProfile->currentIndex();
    const ProfileData* p = m_profiles[m_currentProfile];

    ui->frmDiffuseColor->setStyleSheet(QString("background-color: %1;").arg(p->diffuseColor()));
    ui->frmSpecularColor->setStyleSheet(QString("background-color: %1;").arg(p->specularColor()));
    ui->frmAmbientColor->setStyleSheet(QString("background-color: %1;").arg(p->ambientColor()));
    ui->frmLightColor->setStyleSheet(QString("background-color: %1;").arg(p->lightColor()));
    ui->lblDecoration->setPixmap(p->decorationPath());

    ui->sldLightIntensity->setValue(p->lightIntensity());
    ui->spnSlices->setValue(p->slices());
    ui->spnRings->setValue(p->rings());
    ui->sldShininess->setValue(p->shininess());
    ui->spnIteration->setValue(p->iteration());
    ui->cboEmojiFont->setCurrentFont(QFont(p->font(), -1));
}

void SetupWidget::aboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void SetupWidget::addToExcludeList()
{
    QString userName = ui->edtExcludeChat->text();
    if ( userName.startsWith("@") ) {
        userName = userName.mid(1);
    }
    userName = userName.toLower();

    ui->lstExcludeChat->addItem(userName);
    ui->edtExcludeChat->clear();
}

void SetupWidget::removeFromExcludeList()
{
    QListWidgetItem* item = ui->lstExcludeChat->currentItem();
    if (item) {
        delete ui->lstExcludeChat->takeItem(ui->lstExcludeChat->row(item));
    }
}
