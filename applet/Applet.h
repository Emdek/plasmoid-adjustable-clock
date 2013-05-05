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

#ifndef ADJUSTABLECLOCKAPPLET_HEADER
#define ADJUSTABLECLOCKAPPLET_HEADER

#include <QtCore/QList>
#include <QtCore/QDateTime>

#include <plasmaclock/clockapplet.h>

namespace AdjustableClock
{

class Clock;
class DataSource;
class ThemeWidget;

class Applet : public ClockApplet
{
    Q_OBJECT

    public:
        explicit Applet(QObject *parent, const QVariantList &args);

        void init();
        QStringList getClipboardExpressions() const;

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void constraintsEvent(Plasma::Constraints constraints);
        void createClockConfigurationInterface(KConfigDialog *parent);
        void changeEngineTimezone(const QString &oldTimeZone, const QString &newTimeZone);
        QList<QAction*> contextualActions();

    protected slots:
        void clockConfigChanged();
        void clockConfigAccepted();
        void copyToClipboard();
        void copyToClipboard(QAction *action);
        void toolTipAboutToShow();
        void toolTipHidden();
        void updateToolTipContent();
        void updateClipboardMenu();

    private:
        DataSource *m_source;
        Clock *m_clock;
        ThemeWidget *m_widget;
        QAction *m_clipboardAction;
};

}

#endif
