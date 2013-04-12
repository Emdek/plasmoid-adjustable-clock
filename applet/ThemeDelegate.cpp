/***********************************************************************************
* Adjustable Clock: Plasmoid to show date and time in adjustable format.
* Copyright (C) 2008 - 2013 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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

#include "ThemeDelegate.h"
#include "Applet.h"
#include "Clock.h"
#include "Configuration.h"

#include <QtGui/QStyle>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include <KLocale>
#include <KPixmapCache>

#include <Plasma/Theme>
#include <Plasma/FrameSvg>

namespace AdjustableClock
{

KPixmapCache *m_cache = NULL;

ThemeDelegate::ThemeDelegate(Clock *clock, QObject *parent) : QStyledItemDelegate(parent),
    m_clock(clock)
{
    m_cache = new KPixmapCache("AdjustableClockPreviews");
    m_cache->discard();
}

ThemeDelegate::~ThemeDelegate()
{
    delete m_cache;
}

void ThemeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QPixmap pixmap;

    if (!m_cache->find((index.data(IdRole).toString()), pixmap)) {
        pixmap = QPixmap(180, 90);
        pixmap.fill(Qt::transparent);

        QSizeF size(180, 90);
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        QWebPage page;
        page.setViewportSize(QSize(0, 0));
        page.mainFrame()->setZoomFactor(1);

        Clock::setupClock(page.mainFrame(), m_clock->getClock(true), index.data(HtmlRole).toString());

        if (page.mainFrame()->findFirstElement("body").attribute("background").toLower() == "true") {
            Plasma::FrameSvg background;
            background.setImagePath(Plasma::Theme::defaultTheme()->imagePath("widgets/background"));
            background.setEnabledBorders(Plasma::FrameSvg::AllBorders);
            background.resizeFrame(size);
            background.paintFrame(&pixmapPainter);

            size = background.contentsRect().size();
        } else {
            pixmapPainter.setOpacity(0.1);
            pixmapPainter.setBrush(QBrush(Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor)));
            pixmapPainter.setPen(QPen(Qt::transparent));
            pixmapPainter.drawRoundedRect(QRect(QPoint(0, 0), size.toSize()), 10, 10);
            pixmapPainter.setOpacity(1);
        }

        const qreal widthFactor = (size.width() / page.mainFrame()->contentsSize().width());
        const qreal heightFactor = (size.height() / page.mainFrame()->contentsSize().height());
        QPalette palette = page.palette();
        palette.setBrush(QPalette::Base, Qt::transparent);

        page.setPalette(palette);
        page.setViewportSize(pixmap.size());
        page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);
        page.mainFrame()->render(&pixmapPainter, QWebFrame::ContentsLayer);

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
    painter->drawText(QRectF(210, (option.rect.y() + 35), (option.rect.width() - 215), 70), (Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap), index.data(CommentRole).toString());
}

void ThemeDelegate::clear()
{
    m_cache->discard();
}

QWidget* ThemeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    if (!index.data(OptionsRole).toBool()) {
        return NULL;
    }

    QWidget *widget = new QWidget(parent);
    QPushButton *button = new QPushButton(i18n("Options"), widget);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, widget);
    layout->addWidget(button);
    layout->setAlignment(button, (Qt::AlignBottom | Qt::AlignRight));

    connect(button, SIGNAL(clicked()), this, SIGNAL(showOptions()));

    return widget;
}

QSize ThemeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(500, 100);
}

}
