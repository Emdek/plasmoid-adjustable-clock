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

#ifndef ADJUSTABLECLOCKDECLARATIVEWIDGET_HEADER
#define ADJUSTABLECLOCKDECLARATIVEWIDGET_HEADER

#include <Plasma/DeclarativeWidget>

namespace AdjustableClock
{

class Clock;

class DeclarativeWidget : public Plasma::DeclarativeWidget
{
    Q_OBJECT

    public:
        explicit DeclarativeWidget(Clock *clock, bool constant = true, QGraphicsWidget *parent = NULL);

        void setHtml(const QString &html, const QString &theme = QString());
        bool setTheme(const QString &path);
        bool getBackgroundFlag() const;

    protected:
        void resizeEvent(QGraphicsSceneResizeEvent *event);

    private:
        Clock *m_clock;
        QObject *m_rootObject;
        bool m_constant;
};

}

#endif
