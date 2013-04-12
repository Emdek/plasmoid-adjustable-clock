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

#include <plasmaclock/clockapplet.h>

namespace AdjustableClock
{

struct Option
{
    QString id;
    QString title;
    QVariant initial;
    QVariant value;
    int minimum;
    int maximum;
    QList<QPair<QString, QString> > values;
};

struct Theme
{
    QString id;
    QString title;
    QString description;
    QString author;
    QString html;
    QList<Option> options;
    bool bundled;
};

class Clock;

class Applet : public ClockApplet
{
    Q_OBJECT

    public:
        Applet(QObject *parent, const QVariantList &args);

        void init();
        Clock* getClock() const;
        QStringList getClipboardExpressions() const;
        QList<Theme> getThemes() const;

    protected:
        void constraintsEvent(Plasma::Constraints constraints);
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
        void createClockConfigurationInterface(KConfigDialog *parent);
        void changeEngineTimezone(const QString &oldTimeZone, const QString &newTimeZone);
        QList<QAction*> contextualActions();

    protected slots:
        void clockConfigChanged();
        void clockConfigAccepted();
        void copyToClipboard();
        void copyToClipboard(QAction *action);
        void repaint();
        void toolTipAboutToShow();
        void toolTipHidden();
        void updateToolTipContent();
        void updateClipboardMenu();
        void updateSize();

    private:
        Clock *m_clock;
        QAction *m_clipboardAction;
        QWebPage m_page;
        Theme m_theme;
        bool m_newTheme;
};

}

#endif
