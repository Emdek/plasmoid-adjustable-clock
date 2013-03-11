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

struct Theme
{
    QString id;
    QString title;
    QString description;
    QString author;
    QString html;
    QString css;
    QString script;
    bool background;
    bool bundled;
};

class Clock;
class DataSource;

class Applet : public ClockApplet
{
    Q_OBJECT

    public:
        Applet(QObject *parent, const QVariantList &args);

        void init();
        void saveCustomThemes(const QList<Theme> &getThemes);
        Clock* getClock() const;
        static QString getPageLayout(const QString &html, const QString &css, const QString &script, const QString &head = QString());
        static QString getPageStyleSheet();
        Theme getTheme() const;
        QPair<QString, QString> getToolTipFormat() const;
        QStringList getClipboardFormats() const;
        QList<Theme> getThemes() const;

    public slots:
        void updateToolTipContent();

    protected:
        void constraintsEvent(Plasma::Constraints constraints);
        void resizeEvent(QGraphicsSceneResizeEvent *event);
        void mousePressEvent(QGraphicsSceneMouseEvent *event);
        void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
        void createClockConfigurationInterface(KConfigDialog *parent);
        void changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone);
        QList<Theme> loadThemes(const QString &path, bool bundled) const;
        QList<QAction*> contextualActions();

    protected slots:
        void clockConfigChanged();
        void clockConfigAccepted();
        void copyToClipboard();
        void copyToClipboard(QAction *action);
        void toolTipAboutToShow();
        void toolTipHidden();
        void repaint();
        void updateClipboardMenu();
        void updateSize();
        void updateTheme();

    private:
        DataSource *m_source;
        Clock *m_clock;
        QAction *m_clipboardAction;
        QWebPage m_page;
        QList<Theme> m_themes;
        int m_theme;
};

}

#endif
