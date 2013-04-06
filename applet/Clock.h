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

#ifndef ADJUSTABLECLOCKCLOCK_HEADER
#define ADJUSTABLECLOCKCLOCK_HEADER

#include "DataSource.h"

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>

#include <Plasma/DataEngine>

namespace AdjustableClock
{

class ClockObject : public QObject
{
    Q_OBJECT

    public:
        ClockObject(DataSource *source, bool constant);

        Q_INVOKABLE QString toString(int component, const QVariantMap &options = QVariantMap()) const;

    private:
        DataSource *m_source;
        bool m_constant;
};

class Clock : public QObject
{
    Q_OBJECT

    public:
        Clock(DataSource *source, QWebFrame *document);

        void setTheme(const QString &html, const QString &script);
        static void setupClock(QWebFrame *document, ClockObject *clock, const QString &html, const QString &script);
        static void setupTheme(QWebFrame *document);
        ClockObject* getClock(bool constant) const;
        DataSource* getDataSource() const;
        QString evaluate(const QString &script, bool constant = false);
        static QString getComponentName(ClockComponent component);
        static QLatin1String getComponentString(ClockComponent component);

    protected:
        static void setupEngine(QScriptEngine *engine, ClockObject *clock);
        static void updateComponent(QWebFrame *document, ClockObject *clock, ClockComponent component);

    protected slots:
        void updateComponent(ClockComponent component);
        void updateTheme();

    private:
        DataSource *m_source;
        QWebFrame *m_document;
        ClockObject *m_clock;
        ClockObject *m_constantClock;
        QScriptEngine m_engine;
};

}

#endif
