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

#ifndef SETUPWIDGET_H
#define SETUPWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QFrame>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QPointLight>

#include "atsumari.h"
#include "profiledata.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SetupWidget;
}
QT_END_NAMESPACE

// setColor functions for ProfileData
typedef  void (ProfileData::*SetColorFn)(const QString&);

class SetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SetupWidget(QWidget *parent = nullptr);
    ~SetupWidget();

private:
    void loadSettings();
    void saveSettings();
    void checkClose();
    void selectColor(QString(ProfileData::*getter)() const, void (ProfileData::*setter)(const QString&), QFrame *frame);
    void addToExcludeList();
    void removeFromExcludeList();
    void runPreview();
    void resetDecoration();
    void selectDecoration();
    void openDevConsole();
    void selectEmotePath();
    void selectEmojiPath();
    void resetAuth();
    void setIcons();
    void populateLanguages();
    void setupPreview();
    void validatePaths(QLineEdit* edt);
    void newProfile();
    void duplicateProfile();
    void renameProfile();
    void deleteProfile();
    void createProfileMenus();
    void populateCurrentProfileControls();
    void aboutQt();

    QList<ProfileData*> m_profiles;
    int m_currentProfile;

    Qt3DExtras::Qt3DWindow* m_previewWindow;
    Qt3DRender::QCamera *m_camera;
    Qt3DCore::QEntity *m_rootEntity;
    Qt3DExtras::QOrbitCameraController *m_cameraController;
    Atsumari *m_previewEntity;
    Qt3DCore::QEntity *m_lightEntity;
    Qt3DRender::QPointLight *m_light;
    Qt3DCore::QTransform *m_lightTransform;

    bool m_shouldSave;
    bool m_rebuildingCombo;

    QMenu* m_profileMenu;
    QAction* m_newProfileAction;
    QAction* m_duplicateProfileAction;
    QAction* m_renameProfileAction;
    QAction* m_deleteProfileAction;

    Ui::SetupWidget *ui;

};
#endif // SETUPWIDGET_H
