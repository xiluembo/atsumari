#ifndef PIXMAPSEQUENCEDELEGATE_H
#define PIXMAPSEQUENCEDELEGATE_H

#include <QStyledItemDelegate>

class PixmapSequenceDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PixmapSequenceDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // PIXMAPSEQUENCEDELEGATE_H
