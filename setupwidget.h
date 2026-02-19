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
#include <QQuickView>
#include <QQuickItem>
#include <QTemporaryFile>

#include "profiledata.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SetupWidget;
}
QT_END_NAMESPACE

// setColor functions for ProfileData
typedef void (ProfileData::*SetColorFn)(const QString&);

class SetupWidget : public QWidget {
    Q_OBJECT

public:
    explicit SetupWidget(QWidget *parent = nullptr);
    ~SetupWidget();

    void handleShaderReloadFailure(const QString& message);
    void handleShaderCompilerMessage(const QString& message);

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
    void resetBaseTexture();
    void selectBaseTexture();
    void openDevConsole();
    void resetAuth();
    void setIcons();
    void populateLanguages();
    void populateMaterialTypes();
    void populateRefractionTypes();
    void setupPreview();
    void checkForUpdates();
    void newProfile();
    void duplicateProfile();
    void renameProfile();
    void deleteProfile();
    void createProfileMenus();
    void populateCurrentProfileControls();
    void closeEvent(QCloseEvent* event);
    void aboutQt();
    void loadLogSettings();
    void saveLogSettings();

    QList<ProfileData*> m_profiles;
    int m_currentProfile;

    QQuickView* m_previewWindow;
    QQuickItem* m_previewRootItem;

    QTemporaryFile* m_previewVertFile;
    QTemporaryFile* m_previewFragFile;

    bool m_shouldSave;
    bool m_rebuildingCombo;
    bool m_waitingForShaderReloadError;
    bool m_shaderReloadErrorShown;

    QMenu* m_profileMenu;
    QAction* m_newProfileAction;
    QAction* m_duplicateProfileAction;
    QAction* m_renameProfileAction;
    QAction* m_deleteProfileAction;

    Ui::SetupWidget *ui;

};
#endif // SETUPWIDGET_H
