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

#include "communityshadersdialog.h"

#include <QColor>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickView>
#include <QScrollBar>
#include <QTimer>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

#include "settings_defaults.h"

namespace {
QString contactLinksText(const CommunityShaderPack& pack)
{
    QStringList parts;
    if (!pack.authorEmail.isEmpty()) {
        parts << QObject::tr("Email: <a href=\"mailto:%1\">%1</a>").arg(pack.authorEmail.toHtmlEscaped());
    }
    if (!pack.authorWebsite.isEmpty()) {
        const QString displayText = QUrl(pack.authorWebsite).toDisplayString(QUrl::FullyDecoded).toHtmlEscaped();
        parts << QObject::tr("Website: <a href=\"%1\">%2</a>")
                     .arg(pack.authorWebsite.toHtmlEscaped(), displayText);
    }

    return parts.join(QStringLiteral(" | "));
}

class CommunityShaderCardWidget : public QFrame
{
public:
    explicit CommunityShaderCardWidget(const CommunityShaderPack& pack,
                                       const std::function<void()>& applyHandler,
                                       const std::function<void(const QUrl&)>& externalLinkHandler,
                                       QWidget *parent = nullptr)
        : QFrame(parent)
    {
        setFrameShape(QFrame::StyledPanel);
        setFrameShadow(QFrame::Raised);

        auto* mainLayout = new QHBoxLayout(this);
        auto* textLayout = new QVBoxLayout;
        mainLayout->addLayout(textLayout, 1);

        auto* nameLabel = new QLabel(QStringLiteral("<b>%1</b>").arg(pack.name.toHtmlEscaped()), this);
        nameLabel->setTextFormat(Qt::RichText);
        nameLabel->setWordWrap(true);
        textLayout->addWidget(nameLabel);

        auto* descriptionLabel = new QLabel(pack.description, this);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        textLayout->addWidget(descriptionLabel);

        auto* authorLabel = new QLabel(QObject::tr("Author: %1").arg(pack.authorName), this);
        authorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        textLayout->addWidget(authorLabel);

        const QString contactText = contactLinksText(pack);
        if (!contactText.isEmpty()) {
            auto* contactLabel = new QLabel(contactText, this);
            contactLabel->setTextFormat(Qt::RichText);
            contactLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
            contactLabel->setOpenExternalLinks(false);
            QObject::connect(contactLabel, &QLabel::linkActivated, this, [externalLinkHandler](const QString& link) {
                externalLinkHandler(QUrl(link));
            });
            textLayout->addWidget(contactLabel);
        }

        textLayout->addStretch();

        auto* applyButton = new QPushButton(QObject::tr("Apply Shader Pack..."), this);
        QObject::connect(applyButton, &QPushButton::clicked, this, [applyHandler]() {
            applyHandler();
        });
        textLayout->addWidget(applyButton, 0, Qt::AlignLeft);

        auto* previewLabel = new QLabel(QObject::tr("Loading preview..."), this);
        previewLabel->setObjectName(QStringLiteral("lblCommunityShaderPreview"));
        previewLabel->setMinimumSize(220, 220);
        previewLabel->setMaximumSize(220, 220);
        previewLabel->setAlignment(Qt::AlignCenter);
        previewLabel->setFrameShape(QFrame::StyledPanel);
        previewLabel->setStyleSheet(QStringLiteral("background-color: black; color: white;"));
        mainLayout->addWidget(previewLabel, 0, Qt::AlignTop);
    }
};
}

CommunityShadersDialog::CommunityShadersDialog(const QLocale& locale,
                                               const CommunityShaderReleaseInfo& initialRelease,
                                               PackHandler applyCurrentProfileHandler,
                                               PackHandler createProfileHandler,
                                               ReleaseSeenHandler releaseSeenHandler,
                                               QWidget *parent)
    : QDialog(parent)
    , m_locale(locale)
    , m_initialRelease(initialRelease)
    , m_applyCurrentProfileHandler(std::move(applyCurrentProfileHandler))
    , m_createProfileHandler(std::move(createProfileHandler))
    , m_releaseSeenHandler(std::move(releaseSeenHandler))
    , m_releaseAlreadyMarked(initialRelease.isValid())
    , m_loader(new CommunityShadersLoader(locale, this))
    , m_filterEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_reloadButton(nullptr)
    , m_emptyStateLabel(nullptr)
    , m_contributeLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_cardsContainer(nullptr)
    , m_cardsLayout(nullptr)
    , m_previewRenderWindow(nullptr)
    , m_previewRenderIndex(0)
{
    buildUi();

    connect(m_loader, &CommunityShadersLoader::latestReleaseChanged, this, [this]() {
        m_initialRelease = m_loader->releaseInfo();
        markReleaseAsSeenIfNeeded();
    });
    connect(m_loader, &CommunityShadersLoader::statusChanged, this, [this](const QString& status) {
        updateStatus(status);
    });
    connect(m_loader, &CommunityShadersLoader::progressRangeChanged, m_progressBar, &QProgressBar::setRange);
    connect(m_loader, &CommunityShadersLoader::progressValueChanged, m_progressBar, &QProgressBar::setValue);
    connect(m_loader, &CommunityShadersLoader::loadingFinished, this, [this]() {
        m_packs = m_loader->packs();
        rebuildCards();
        setLoadingState(false);
    });
    connect(m_loader, &CommunityShadersLoader::errorOccurred, this, [this](const QString& message) {
        setLoadingState(false);
        updateStatus(message, true);
        m_reloadButton->setVisible(true);
    });

    loadPacks();
}

CommunityShadersDialog::~CommunityShadersDialog()
{
    QObject::disconnect(m_previewFrameConnection);
    if (m_previewRenderWindow) {
        m_previewRenderWindow->hide();
        delete m_previewRenderWindow;
        m_previewRenderWindow = nullptr;
    }
}

void CommunityShadersDialog::buildUi()
{
    setModal(true);
    setWindowTitle(tr("Community Shaders"));
    resize(1100, 760);

    auto* mainLayout = new QVBoxLayout(this);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter packs by name, description or author"));
    connect(m_filterEdit, &QLineEdit::textChanged, this, &CommunityShadersDialog::applyFilter);
    mainLayout->addWidget(m_filterEdit);

    auto* statusLayout = new QHBoxLayout;
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    statusLayout->addWidget(m_statusLabel, 1);

    m_reloadButton = new QPushButton(tr("Reload"), this);
    m_reloadButton->setVisible(false);
    connect(m_reloadButton, &QPushButton::clicked, this, &CommunityShadersDialog::loadPacks);
    statusLayout->addWidget(m_reloadButton);
    mainLayout->addLayout(statusLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_progressBar->setRange(0, 0);
    mainLayout->addWidget(m_progressBar);

    m_emptyStateLabel = new QLabel(tr("No community shader packs match the current filter."), this);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setVisible(false);
    mainLayout->addWidget(m_emptyStateLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_cardsContainer = new QWidget(m_scrollArea);
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(12);
    m_scrollArea->setWidget(m_cardsContainer);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        startPreviewRendering();
    });
    connect(m_scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        startPreviewRendering();
    });
    mainLayout->addWidget(m_scrollArea, 1);

    m_contributeLabel = new QLabel(this);
    m_contributeLabel->setWordWrap(true);
    m_contributeLabel->setTextFormat(Qt::RichText);
    m_contributeLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_contributeLabel->setOpenExternalLinks(false);
    m_contributeLabel->setText(
        tr("Want to contribute your shaders? Submit them via "
           "<a href=\"https://docs.google.com/forms/d/e/1FAIpQLScu85yV-Pz6F_oTNxM37AiPFI3cXKWr9EvXlD3dHs3IVsIG1g/viewform?usp=publish-editor\">Google Forms</a> "
           "or open a "
           "<a href=\"https://github.com/xiluembo/atsumari-communityshaders/pulls\">Pull Request</a>."));
    connect(m_contributeLabel, &QLabel::linkActivated, this, [this](const QString& link) {
        confirmAndOpenExternalLink(QUrl(link));
    });
    mainLayout->addWidget(m_contributeLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CommunityShadersDialog::loadPacks()
{
    setLoadingState(true);
    m_filterEdit->clear();
    updateStatus(tr("Checking the latest community shader release..."));
    m_loader->loadRelease(m_initialRelease);
}

void CommunityShadersDialog::setLoadingState(bool isLoading)
{
    m_filterEdit->setEnabled(!isLoading);
    m_progressBar->setVisible(true);
    m_reloadButton->setVisible(false);
    m_scrollArea->setEnabled(!isLoading);
    if (isLoading) {
        m_emptyStateLabel->setVisible(false);
    }
}

void CommunityShadersDialog::updateStatus(const QString& text, bool isError)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError ? QStringLiteral("color: #b00020;") : QString());
}

void CommunityShadersDialog::rebuildCards()
{
    for (QWidget* cardWidget : std::as_const(m_cardWidgets)) {
        delete cardWidget;
    }
    m_cardWidgets.clear();

    QLayoutItem* child = nullptr;
    while ((child = m_cardsLayout->takeAt(0)) != nullptr) {
        delete child;
    }

    for (const CommunityShaderPack& pack : std::as_const(m_packs)) {
        auto* card = new CommunityShaderCardWidget(
            pack,
            [this, pack]() { handlePackAction(pack); },
            [this](const QUrl& url) { confirmAndOpenExternalLink(url); },
            m_cardsContainer);
        m_cardWidgets.append(card);
        m_cardsLayout->addWidget(card);
    }

    m_cardsLayout->addStretch();
    applyFilter(m_filterEdit->text());
    startPreviewRendering();
}

void CommunityShadersDialog::applyFilter(const QString& filterText)
{
    const QString normalizedFilter = filterText.trimmed().toCaseFolded();
    int visibleCards = 0;

    for (int i = 0; i < m_cardWidgets.size() && i < m_packs.size(); ++i) {
        const bool isVisible = normalizedFilter.isEmpty()
            || m_packs.at(i).searchableText.contains(normalizedFilter);
        m_cardWidgets.at(i)->setVisible(isVisible);
        if (isVisible) {
            ++visibleCards;
        }
    }

    m_emptyStateLabel->setVisible(!m_packs.isEmpty() && visibleCards == 0);
}

void CommunityShadersDialog::handlePackAction(const CommunityShaderPack& pack)
{
    QMessageBox messageBox(this);
    messageBox.setWindowTitle(tr("Apply Community Shader Pack"));
    messageBox.setText(tr("How do you want to apply '%1'?").arg(pack.name));

    QPushButton* overwriteButton = messageBox.addButton(tr("Overwrite Current Profile"), QMessageBox::AcceptRole);
    QPushButton* createButton = messageBox.addButton(tr("Create New Profile"), QMessageBox::ActionRole);
    messageBox.addButton(QMessageBox::Cancel);
    messageBox.exec();

    if (messageBox.clickedButton() == overwriteButton) {
        if (m_applyCurrentProfileHandler) {
            m_applyCurrentProfileHandler(pack);
        }
        accept();
    } else if (messageBox.clickedButton() == createButton) {
        if (m_createProfileHandler) {
            m_createProfileHandler(pack);
        }
        accept();
    }
}

void CommunityShadersDialog::confirmAndOpenExternalLink(const QUrl& url)
{
    if (!url.isValid()) {
        return;
    }

    const auto result = QMessageBox::question(
        this,
        tr("Open External Link"),
        tr("This action will open an external link outside Atsumari.\n\n%1")
            .arg(url.toDisplayString(QUrl::FullyDecoded)));

    if (result == QMessageBox::Yes) {
        QDesktopServices::openUrl(url);
    }
}

void CommunityShadersDialog::markReleaseAsSeenIfNeeded()
{
    if (m_releaseAlreadyMarked || !m_releaseSeenHandler) {
        return;
    }

    const QString releaseTag = m_loader->releaseInfo().tag;
    if (releaseTag.isEmpty()) {
        return;
    }

    m_releaseSeenHandler(releaseTag);
    m_releaseAlreadyMarked = true;
}

void CommunityShadersDialog::startPreviewRendering()
{
    m_previewRenderIndex = 0;
    if (m_previewRenderWindow) {
        m_previewRenderWindow->hide();
    }
    QTimer::singleShot(0, this, &CommunityShadersDialog::renderNextPreview);
}

void CommunityShadersDialog::renderNextPreview()
{
    if (m_previewRenderIndex >= m_packs.size()) {
        if (m_previewRenderWindow) {
            m_previewRenderWindow->hide();
        }
        return;
    }

    QLabel* previewLabel = previewLabelForCard(m_previewRenderIndex);
    if (!previewLabel
        || !previewLabel->isVisible()
        || !previewLabel->pixmap(Qt::ReturnByValue).isNull()) {
        ++m_previewRenderIndex;
        QTimer::singleShot(0, this, &CommunityShadersDialog::renderNextPreview);
        return;
    }

    QWidget* topLevelWidget = window();
    if (!topLevelWidget || !topLevelWidget->windowHandle()) {
        QTimer::singleShot(50, this, &CommunityShadersDialog::renderNextPreview);
        return;
    }

    if (!m_previewRenderWindow) {
        m_previewRenderWindow = new QQuickView;
        m_previewRenderWindow->setParent(topLevelWidget->windowHandle());
        m_previewRenderWindow->setColor(Qt::black);
        m_previewRenderWindow->setResizeMode(QQuickView::SizeRootObjectToView);
        m_previewRenderWindow->setFlags(Qt::SubWindow | Qt::FramelessWindowHint);
        m_previewRenderWindow->setSource(QUrl(QStringLiteral("qrc:/main.qml")));
    }

    const QPoint topLeft = previewLabel->mapTo(topLevelWidget, QPoint(0, 0));
    m_previewRenderWindow->setGeometry(QRect(topLeft, previewLabel->size()));

    QObject::disconnect(m_previewFrameConnection);
    if (QQuickItem* previewRoot = m_previewRenderWindow->rootObject()) {
        const CommunityShaderPack& pack = m_packs.at(m_previewRenderIndex);
        previewRoot->setProperty("baseColor", QColor(QStringLiteral(DEFAULT_COLORS_BASE)));
        previewRoot->setProperty("ambientColor", QColor(QStringLiteral(DEFAULT_COLORS_AMBIENT)));
        previewRoot->setProperty("lightColor", QColor(QStringLiteral(DEFAULT_COLORS_LIGHT)));
        previewRoot->setProperty("brightness", DEFAULT_LIGHT_BRIGHTNESS / 100.0);
        previewRoot->setProperty("rotationInterval", DEFAULT_ITERATION_TIME * 1000);
        previewRoot->setProperty("useSpecularGlossyMaterial", false);
        previewRoot->setProperty("useCustomMaterial", true);
        previewRoot->setProperty("vertexShaderPath", QUrl::fromLocalFile(pack.vertexShaderPath).toString());
        previewRoot->setProperty("fragmentShaderPath", QUrl::fromLocalFile(pack.fragmentShaderPath).toString());
    }

    m_previewFrameConnection = connect(m_previewRenderWindow, &QQuickWindow::frameSwapped, this, [this]() {
        QObject::disconnect(m_previewFrameConnection);

        QLabel* currentPreviewLabel = previewLabelForCard(m_previewRenderIndex);
        if (currentPreviewLabel && m_previewRenderWindow) {
            const QImage image = m_previewRenderWindow->grabWindow();
            currentPreviewLabel->setPixmap(QPixmap::fromImage(image).scaled(
                currentPreviewLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
            currentPreviewLabel->setText(QString());
        }

        ++m_previewRenderIndex;
        QTimer::singleShot(0, this, &CommunityShadersDialog::renderNextPreview);
    });

    m_previewRenderWindow->show();
    m_previewRenderWindow->raise();
    m_previewRenderWindow->requestUpdate();
}

QLabel* CommunityShadersDialog::previewLabelForCard(int index) const
{
    if (index < 0 || index >= m_cardWidgets.size()) {
        return nullptr;
    }

    return m_cardWidgets.at(index)->findChild<QLabel*>(QStringLiteral("lblCommunityShaderPreview"));
}
