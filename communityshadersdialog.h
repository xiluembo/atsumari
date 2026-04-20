/*
 * Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
 * Copyright (C) 2026 - Andrius da Costa Ribas <andriusmao@gmail.com>
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

#ifndef COMMUNITYSHADERSDIALOG_H
#define COMMUNITYSHADERSDIALOG_H

#include <QDialog>
#include <QMetaObject>

#include <functional>

#include "communityshadersloader.h"

class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QQuickView;
class QScrollArea;
class QVBoxLayout;
class QWidget;

class CommunityShadersDialog : public QDialog
{
    Q_OBJECT

public:
    using PackHandler = std::function<void(const CommunityShaderPack&)>;
    using ReleaseSeenHandler = std::function<void(const QString&)>;

    explicit CommunityShadersDialog(const QLocale& locale,
                                    const CommunityShaderReleaseInfo& initialRelease,
                                    PackHandler applyCurrentProfileHandler,
                                    PackHandler createProfileHandler,
                                    ReleaseSeenHandler releaseSeenHandler,
                                    QWidget *parent = nullptr);
    ~CommunityShadersDialog() override;

private:
    void buildUi();
    void loadPacks();
    void setLoadingState(bool isLoading);
    void updateStatus(const QString& text, bool isError = false);
    void rebuildCards();
    void applyFilter(const QString& filterText);
    void handlePackAction(const CommunityShaderPack& pack);
    void confirmAndOpenExternalLink(const QUrl& url);
    void markReleaseAsSeenIfNeeded();
    void startPreviewRendering();
    void renderNextPreview();
    QLabel* previewLabelForCard(int index) const;

    QLocale m_locale;
    CommunityShaderReleaseInfo m_initialRelease;
    PackHandler m_applyCurrentProfileHandler;
    PackHandler m_createProfileHandler;
    ReleaseSeenHandler m_releaseSeenHandler;
    bool m_releaseAlreadyMarked;
    CommunityShadersLoader* m_loader;
    QLineEdit* m_filterEdit;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_reloadButton;
    QLabel* m_emptyStateLabel;
    QScrollArea* m_scrollArea;
    QWidget* m_cardsContainer;
    QVBoxLayout* m_cardsLayout;
    QList<CommunityShaderPack> m_packs;
    QList<QWidget*> m_cardWidgets;
    QQuickView* m_previewRenderWindow;
    QMetaObject::Connection m_previewFrameConnection;
    int m_previewRenderIndex;
};

#endif // COMMUNITYSHADERSDIALOG_H
