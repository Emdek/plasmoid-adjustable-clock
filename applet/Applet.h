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
#include <QtWebKit/QWebPage>

#include <Plasma/Applet>
#include <Plasma/DeclarativeWidget>

#include <plasmaclock/clockapplet.h>

namespace AdjustableClock
{

class Clock;

class Applet : public ClockApplet
{
    Q_OBJECT

    public:
        explicit Applet(QObject *parent, const QVariantList &args);

        void init();
        Clock* getClock() const;
        QStringList getClipboardExpressions() const;

    protected:
        void constraintsEvent(Plasma::Constraints constraints);
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        void createClockConfigurationInterface(KConfigDialog *parent);
        void changeEngineTimezone(const QString &oldTimeZone, const QString &newTimeZone);
        QString readTheme(const QString &path, const QString &identifier) const;
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
        Plasma::DeclarativeWidget *m_widget;
        Clock *m_clock;
        QAction *m_clipboardAction;
};

}

#endif
