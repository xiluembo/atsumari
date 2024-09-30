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
#include <QResizeEvent>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStyleHints>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DRender/QPointLight>

#include "atsumari.h"
#include "settings_defaults.h"
#include "atsumarilauncher.h"
#include "localehelper.h"

SetupWidget::SetupWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SetupWidget)
{
    ui->setupUi(this);
    setIcons();
    populateLanguages();
    loadSettings();

    // Appearance tab
    connect(ui->btnSelectDiffuse, &QPushButton::clicked, this, [=]() {
        selectColor(&m_diffuse, ui->frmDiffuseColor);
    });

    connect(ui->btnSelectSpecular, &QPushButton::clicked, this, [=]() {
        selectColor(&m_specular, ui->frmSpecularColor);
    });

    connect(ui->btnSelectAmbient, &QPushButton::clicked, this, [=]() {
        selectColor(&m_ambient, ui->frmAmbientColor);
    });

    connect(ui->btnSelectLight, &QPushButton::clicked, this, [=]() {
        selectColor(&m_light, ui->frmLightColor);
    });

    connect(ui->cboLanguage, &QComboBox::currentIndexChanged, this, [=]() {
        LocaleHelper::loadTranslation(ui->cboLanguage->currentData().toLocale());
        ui->retranslateUi(this);
    });

    // any of those values changing may trigger a preview update
    connect(ui->spnSlices, &QSpinBox::valueChanged, this, &SetupWidget::runPreview);
    connect(ui->spnRings, &QSpinBox::valueChanged, this, &SetupWidget::runPreview);
    connect(ui->spnIteration, &QDoubleSpinBox::valueChanged, this, &SetupWidget::runPreview);
    connect(ui->sldLightIntensity, &QSlider::valueChanged, this, &SetupWidget::runPreview);
    connect(ui->sldShininess, &QSlider::valueChanged, this, &SetupWidget::runPreview);

    connect(ui->btnResetDecoration, &QPushButton::clicked, this, &SetupWidget::resetDecoration);
    connect(ui->btnSelectDecoration, &QPushButton::clicked, this, &SetupWidget::selectDecoration);

    // preview window
    m_previewWindow = new Qt3DExtras::Qt3DWindow;
    m_previewWindow->defaultFrameGraph()->setClearColor(QColor(Qt::black));

    ui->grpPreview->layout()->addWidget(QWidget::createWindowContainer(m_previewWindow));
    m_previewWindow->show();

    runPreview();

    // Directories tab
    connect(ui->btnEmojiPath, &QPushButton::clicked, this, &SetupWidget::selectEmojiPath);
    connect(ui->btnEmotePath, &QPushButton::clicked, this, &SetupWidget::selectEmotePath);

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
    delete m_previewWindow;
}

void SetupWidget::runPreview()
{
    // Root Entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    // Setting up Camera
    Qt3DRender::QCamera *camera = m_previewWindow->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, 3.0));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // Camera Controller
    Qt3DExtras::QOrbitCameraController *cameraController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    cameraController->setCamera(camera);

    // Adding Atsumari
    Atsumari *previewEntity = new Atsumari(rootEntity);
    previewEntity->setSlices(ui->spnSlices->value());
    previewEntity->setRings(ui->spnRings->value());
    previewEntity->setDiffuse(m_diffuse);
    previewEntity->setSpecular(m_specular);
    previewEntity->setAmbient(m_ambient);
    previewEntity->setShininess(ui->sldShininess->value());
    previewEntity->setIterationInterval(ui->spnIteration->value());

    QUrl kata_deco = QUrl::fromLocalFile(m_decorationPath);

    previewEntity->addEmote(kata_deco, 0.00f, 0.00f, 0.35f);
    previewEntity->addEmote(kata_deco, 0.00f, 180.00f, 0.35f);
    previewEntity->addEmote(kata_deco, 0.00f, 63.435f, 0.35f);
    previewEntity->addEmote(kata_deco, 72.0f, 63.435f, 0.35f);
    previewEntity->addEmote(kata_deco, 144.0f, 63.435f, 0.35f);
    previewEntity->addEmote(kata_deco, 216.0f, 63.435f, 0.35f);
    previewEntity->addEmote(kata_deco, 288.0f, 63.435f, 0.35f);
    previewEntity->addEmote(kata_deco, 36.0f, 116.565f, 0.35f);
    previewEntity->addEmote(kata_deco, 108.0f, 116.565f, 0.35f);
    previewEntity->addEmote(kata_deco, 180.0f, 116.565f, 0.35f);
    previewEntity->addEmote(kata_deco, 252.0f, 116.565f, 0.35f);
    previewEntity->addEmote(kata_deco, 324.0f, 116.565f, 0.35f);

    // Add Lighting
    auto *lightEntity = new Qt3DCore::QEntity(rootEntity);
    auto *light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(m_light);
    light->setIntensity(ui->sldLightIntensity->value() / 100.0f);
    auto *lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(camera->position());
    lightEntity->addComponent(light);
    lightEntity->addComponent(lightTransform);

    m_previewWindow->setRootEntity(rootEntity);

}

void SetupWidget::resetDecoration()
{
    m_decorationPath = DEFAULT_DECORATION_PATH;
    ui->lblDecoration->setPixmap(QPixmap(DEFAULT_DECORATION_PATH));
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
    m_diffuse = settings.value(CFG_COLORS_DIFFUSE, DEFAULT_COLORS_DIFFUSE).toString();
    m_specular = settings.value(CFG_COLORS_SPECULAR, DEFAULT_COLORS_SPECULAR).toString();
    m_ambient = settings.value(CFG_COLORS_AMBIENT, DEFAULT_COLORS_AMBIENT).toString();
    m_light = settings.value(CFG_COLORS_LIGHT, DEFAULT_COLORS_LIGHT).toString();
    m_decorationPath = settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString();

    ui->frmDiffuseColor->setStyleSheet(QString("background-color: %1;").arg(m_diffuse));
    ui->frmSpecularColor->setStyleSheet(QString("background-color: %1;").arg(m_specular));
    ui->frmAmbientColor->setStyleSheet(QString("background-color: %1;").arg(m_ambient));
    ui->frmLightColor->setStyleSheet(QString("background-color: %1;").arg(m_light));
    ui->lblDecoration->setPixmap(m_decorationPath);

    ui->sldLightIntensity->setValue(settings.value(CFG_LIGHT_INTENSITY, DEFAULT_LIGHT_INTENSITY).toInt());
    ui->spnSlices->setValue(settings.value(CFG_SLICES, DEFAULT_SLICES).toInt());
    ui->spnRings->setValue(settings.value(CFG_RINGS, DEFAULT_RINGS).toInt());
    ui->sldShininess->setValue(settings.value(CFG_SHININESS, DEFAULT_SHININESS).toInt());
    ui->spnIteration->setValue(settings.value(CFG_ITERATION_TIME, DEFAULT_ITERATION_TIME).toDouble());
    ui->lblDecoration->setPixmap(QPixmap(settings.value(CFG_DECORATION_PATH, DEFAULT_DECORATION_PATH).toString()));
    ui->cboEmojiFont->setCurrentFont(QFont(settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString(), -1));

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
    QSettings settings;
    settings.setValue(CFG_LANGUAGE, ui->cboLanguage->currentData());

    settings.setValue(CFG_COLORS_DIFFUSE, m_diffuse);
    settings.setValue(CFG_COLORS_SPECULAR, m_specular);
    settings.setValue(CFG_COLORS_AMBIENT, m_ambient);
    settings.setValue(CFG_COLORS_LIGHT, m_light);
    settings.setValue(CFG_LIGHT_INTENSITY, ui->sldLightIntensity->value());
    settings.setValue(CFG_SLICES, ui->spnSlices->value());
    settings.setValue(CFG_RINGS, ui->spnRings->value());
    settings.setValue(CFG_SHININESS, ui->sldShininess->value());
    settings.setValue(CFG_ITERATION_TIME, ui->spnIteration->value());
    settings.setValue(CFG_DECORATION_PATH, m_decorationPath);


    QString newFont = ui->cboEmojiFont->currentFont().family();
    QString previousFont = settings.value(CFG_EMOJI_FONT, DEFAULT_EMOJI_FONT).toString();
    if ( newFont != previousFont && newFont != DEFAULT_EMOJI_FONT) {
        QMessageBox::warning(this, tr("Changing default font"),
                                 tr("Changing default font is not advised. SVG Based fonts may not work correctly on Windows, and non-SVG Based fonts may also fail to render correctly on other platforms"));
    }

    settings.setValue(CFG_EMOJI_FONT, newFont);

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
    const AtsumariLauncher* launcher = new AtsumariLauncher;
    QTimer::singleShot(1000, this, [=]() { this->deleteLater(); } );
}

void SetupWidget::selectColor(QString *colorEnv, QFrame *frame)
{
    QColorDialog* dlg = new QColorDialog(QColor(*colorEnv), this);

    dlg->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    connect(dlg, &QColorDialog::colorSelected, this, [=](const QColor& color) {
        *colorEnv = color.name();
        frame->setStyleSheet("background-color: " + (*colorEnv) + ";");
        runPreview();
        dlg->deleteLater();
    });

    dlg->open();
}

void SetupWidget::selectDecoration()
{
    QFileDialog* dlg = new QFileDialog(this, tr("Select decoration file"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));

    dlg->setWindowIcon(QIcon(":/appicon/atsumari.svg"));

    connect(dlg, &QFileDialog::fileSelected, this, [=](const QString& file) {
        m_decorationPath = file;
        ui->lblDecoration->setPixmap(QPixmap(file));
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
