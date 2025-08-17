#include "pixmapsequencedelegate.h"
#include "twitchlogmodel.h"

#include <QApplication>
#include <QPainter>
#include <QIcon>
#include <QStyle>

PixmapSequenceDelegate::PixmapSequenceDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PixmapSequenceDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.icon = QIcon();

    painter->save();
    painter->setClipRect(opt.rect);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    QList<QPixmap> emotes = TwitchLogModel::instance()->emotesForRow(index.row());
    if (!emotes.isEmpty()) {
        int x = opt.rect.x();
        int maxX = opt.rect.x() + opt.rect.width();
        for (const QPixmap &p : emotes) {
            QSize size = p.size();
            size.scale(opt.rect.height(), opt.rect.height(), Qt::KeepAspectRatio);
            QPixmap scaled = p.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            if (x + scaled.width() > maxX)
                break;
            painter->drawPixmap(x, opt.rect.y() + (opt.rect.height() - scaled.height()) / 2, scaled);
            x += scaled.width();
        }
    }

    painter->restore();
}
