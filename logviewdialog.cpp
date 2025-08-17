#include "logviewdialog.h"
#include "ui_logviewdialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QPixmap>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include "twitchlogmodel.h"
#include "settings_defaults.h"
#include "pixmapsequencedelegate.h"

LogViewDialog::LogViewDialog(QWidget *parent)
    : QDialog(parent)
{
    Ui::LogViewDialog ui;
    ui.setupUi(this);
    m_table = ui.tblLogs;
    m_exportButton = ui.btnExport;
    m_table->setModel(TwitchLogModel::instance());
    int size = m_table->verticalHeader()->defaultSectionSize();
    m_table->setIconSize(QSize(size, size));
    m_table->setItemDelegateForColumn(TwitchLogModel::Emotes, new PixmapSequenceDelegate(m_table));
    connect(m_exportButton, &QPushButton::clicked, this, [=]() {
        QString fn = QFileDialog::getSaveFileName(this, tr("Export logs"), QString(), tr("Text Files (*.txt)"));
        if (!fn.isEmpty()) {
            TwitchLogModel::instance()->exportToFile(fn);
        }
    });
    connect(m_table, &QTableView::clicked, this, [=](const QModelIndex &idx){
        if (idx.column() == TwitchLogModel::Emotes) {
            QList<QPixmap> emotes = TwitchLogModel::instance()->emotesForRow(idx.row());
            if (!emotes.isEmpty())
                showEmotes(emotes);
        }
    });
    applyColumnVisibility();
}

void LogViewDialog::applyColumnVisibility()
{
    QSettings settings;
    QStringList cols = settings.value(CFG_LOG_COLUMNS, DEFAULT_LOG_COLUMNS).toStringList();
    for (int i = 0; i < TwitchLogModel::ColumnCount; ++i) {
        QString name = TwitchLogModel::instance()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        m_table->setColumnHidden(i, !cols.contains(name));
    }
}

void LogViewDialog::showEmotes(const QList<QPixmap> &emotes)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Emotes"));
    QScrollArea *scroll = new QScrollArea(&dlg);
    QWidget *container = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(container);
    for (const QPixmap &p : emotes) {
        QLabel *lbl = new QLabel;
        lbl->setPixmap(p);
        layout->addWidget(lbl);
    }
    layout->addStretch();
    scroll->setWidget(container);
    scroll->setWidgetResizable(true);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    mainLayout->addWidget(scroll);
    dlg.resize(400, 200);
    dlg.exec();
}
