#ifndef LOGVIEWDIALOG_H
#define LOGVIEWDIALOG_H

#include <QDialog>
#include <QList>
#include <QPixmap>

class QTableView;
class QPushButton;

class LogViewDialog : public QDialog {
    Q_OBJECT
public:
    explicit LogViewDialog(QWidget *parent = nullptr);

private:
    void applyColumnVisibility();
    void showEmotes(const QList<QPixmap> &emotes);
    QTableView *m_table;
    QPushButton *m_exportButton;
};

#endif // LOGVIEWDIALOG_H
