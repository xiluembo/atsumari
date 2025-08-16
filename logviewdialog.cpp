#include "logviewdialog.h"
#include "ui_logviewdialog.h"

#include <QFileDialog>
#include <QSettings>

#include "twitchlogmodel.h"
#include "settings_defaults.h"

LogViewDialog::LogViewDialog(QWidget *parent)
    : QDialog(parent)
{
    Ui::LogViewDialog ui;
    ui.setupUi(this);
    m_table = ui.tblLogs;
    m_exportButton = ui.btnExport;
    m_table->setModel(TwitchLogModel::instance());
    connect(m_exportButton, &QPushButton::clicked, this, [=]() {
        QString fn = QFileDialog::getSaveFileName(this, tr("Export logs"), QString(), tr("Text Files (*.txt)"));
        if (!fn.isEmpty()) {
            TwitchLogModel::instance()->exportToFile(fn);
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
