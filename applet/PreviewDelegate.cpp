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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <Plasma/Theme>
#include <Plasma/FrameSvg>

namespace AdjustableClock
{

PreviewDelegate::PreviewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void PreviewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QPixmap pixmap(230, 90);
    pixmap.fill(Qt::transparent);

    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    if (index.data(BackgroundRole).toBool()) {
        Plasma::FrameSvg background;
        background.setImagePath(Plasma::Theme::defaultTheme()->imagePath(QLatin1String("widgets/background")));
        background.setEnabledBorders(Plasma::FrameSvg::AllBorders);
        background.resizeFrame(QSizeF(230, 90));
        background.paintFrame(&pixmapPainter);
    }

    QWebPage page;
    page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    page.mainFrame()->setHtml(QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"><html><head><style type=\"text/css\">html, body, body > table, #clock {margin:0; padding:0; height:100%; width:100%; vertical-align:middle;}") + index.data(CssRole).toString() + QLatin1String("</style></head><body><table><tr><td id=\"clock\">") + Applet::evaluateFormat(index.data(HtmlRole).toString(), QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15))) + QLatin1String("</td></tr></table></body></html>"));
    page.mainFrame()->setZoomFactor(Applet::zoomFactor(page, QSizeF(230, 90)));
    page.setViewportSize(QSize(230, 90));

    QPalette palette = page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    page.setPalette(palette);
    page.mainFrame()->evaluateJavaScript(QLatin1String("document.fgColor = '") + Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name() + QLatin1Char('\''));
    page.mainFrame()->render(&pixmapPainter);

    painter->drawPixmap(QRect((option.rect.x() + 5), (option.rect.y() + 5), 230, 90), pixmap, QRect(0, 0, 230, 90));
}

QSize PreviewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(400, 100);
}

}
