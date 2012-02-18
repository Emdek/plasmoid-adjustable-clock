/***********************************************************************************
* Adjustable Clock: Plasmoid to show date and time in adjustable format.
* Copyright (C) 2008 - 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1801, USA.
*
***********************************************************************************/

#include "PreviewDelegate.h"
#include "Configuration.h"
#include "Applet.h"

#include <QtGui/QStyle>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#include <KPixmapCache>

#include <Plasma/Theme>
#include <Plasma/FrameSvg>

namespace AdjustableClock
{

KPixmapCache *m_cache = NULL;

PreviewDelegate::PreviewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    m_cache = new KPixmapCache(QLatin1String("AdjustableClockPreviews"));
    m_cache->discard();
}

PreviewDelegate::~PreviewDelegate()
{
    delete m_cache;
}

void PreviewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QPixmap pixmap;

    if (!m_cache->find((index.data(IdRole).toString()), pixmap)) {
        pixmap = QPixmap(180, 90);
        pixmap.fill(Qt::transparent);

        QSizeF size(180, 90);

        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        if (index.data(BackgroundRole).toBool()) {
            Plasma::FrameSvg background;
            background.setImagePath(Plasma::Theme::defaultTheme()->imagePath(QLatin1String("widgets/background")));
            background.setEnabledBorders(Plasma::FrameSvg::AllBorders);
            background.resizeFrame(size);
            background.paintFrame(&pixmapPainter);

            size = background.contentsRect().size();

            pixmapPainter.translate(QPointF(background.contentsRect().x(), background.contentsRect().y()));
        }

        QWebPage page;
        page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
        page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
        page.mainFrame()->setHtml(Applet::pageLayout(Applet::evaluateFormat(index.data(HtmlRole).toString(), QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15))), index.data(CssRole).toString()));
        page.setViewportSize(QSize(0, 0));
        page.mainFrame()->setZoomFactor(1);

        const qreal widthFactor = (size.width() / page.mainFrame()->contentsSize().width());
        const qreal heightFactor = (size.height() / page.mainFrame()->contentsSize().height());
        QPalette palette = page.palette();
        palette.setBrush(QPalette::Base, Qt::transparent);

        page.setPalette(palette);
        page.setViewportSize(size.toSize());
        page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);
        page.mainFrame()->render(&pixmapPainter);

        m_cache->insert((index.data(IdRole).toString()), pixmap);
    }

    QFont font = painter->font();

    painter->drawPixmap(QRect((option.rect.x() + 5), (option.rect.y() + 5), 180, 90), pixmap, QRect(0, 0, 180, 90));
    painter->setRenderHints(QPainter::TextAntialiasing);
    painter->setPen(option.palette.color(QPalette::WindowText));

    font.setBold(true);

    painter->setFont(font);
    painter->drawText(QRectF(210, (option.rect.y() + 10), (option.rect.width() - 215), 20), (Qt::AlignLeft | Qt::AlignVCenter), (index.data(AuthorRole).toString().isEmpty() ? index.data(TitleRole).toString() : i18n("\"%1\" by %2").arg(index.data(TitleRole).toString()).arg(index.data(AuthorRole).toString())));

    font.setBold(false);

    painter->setFont(font);
    painter->drawText(QRectF(210, (option.rect.y() + 35), (option.rect.width() - 215), 70), (Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap), index.data(DescriptionRole).toString());
}

void PreviewDelegate::clear()
{
    m_cache->discard();
}

QSize PreviewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(500, 100);
}

}
