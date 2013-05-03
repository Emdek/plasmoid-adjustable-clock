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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
***********************************************************************************/

#ifndef ADJUSTABLECLOCKTHEMEWIDGET_HEADER
#define ADJUSTABLECLOCKTHEMEWIDGET_HEADER

#include "Clock.h"

#include <QtWebKit/QWebPage>

#include <Plasma/DeclarativeWidget>

namespace AdjustableClock
{

class ThemeWidget : public Plasma::DeclarativeWidget
{
    Q_OBJECT

    public:
        explicit ThemeWidget(Clock *clock, bool constant = true, QGraphicsWidget *parent = NULL);

        void setHtml(const QString &theme, const QString &html, const QString &css = QString());
        QWebPage* getPage();
        QSize getPreferredSize(const QSize &constraints);
        bool setTheme(const QString &path);
        bool getBackgroundFlag() const;

    protected:
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = NULL);

    protected slots:
        void update();
        void updateComponent(ClockComponent component);
        void updateTheme();
        void updateSize();

    private:
        Clock *m_clock;
        QObject *m_rootObject;
        QWebPage m_page;
        QPointF m_offset;
        QString m_css;
        bool m_constant;
};

}

#endif
