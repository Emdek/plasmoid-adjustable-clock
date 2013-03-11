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

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>

#include <Plasma/DataEngine>

#include "DataSource.h"

namespace AdjustableClock
{

enum ClockMode
{
    StandardClock = 0,
    StaticClock = 2,
    EditorClock = 3
};

struct Placeholder
{
    QString rule;
    QString attribute;
    ClockTimeValue value;
    ValueOptions options;
};

class Clock : public QObject
{
    Q_OBJECT

    public:
        Clock(DataSource *parent, ClockMode mode = StandardClock);

        void setDocument(QWebFrame *document);
        Q_INVOKABLE void setRule(const QString &rule, const QString &attribute, ClockTimeValue value, ValueOptions options = ValueDefaultForm);
        Q_INVOKABLE void setRule(const QString &rule, ClockTimeValue value, ValueOptions options = ValueDefaultForm);
        Q_INVOKABLE void setValue(const QString &rule, const QString &attribute, const QString &value);
        Q_INVOKABLE void setValue(const QString &rule, const QString &value);
        QString evaluate(const QString &script) const;
        Q_INVOKABLE QString getTimeString(ClockTimeValue value, ValueOptions options = ValueDefaultForm) const;
        Q_INVOKABLE QVariantList getEventsList(ClockEventsType value, ValueOptions options = ValueDefaultForm) const;

    protected:
        void applyRule(const Placeholder &rule);
        void setValue(const QWebElementCollection &elements, const QString &attribute, const QString &value);
        void setValue(const QWebElementCollection &elements, const QString &value);

    private:
        DataSource *m_source;
        QWebFrame *m_document;
        QScriptEngine m_engine;
        QHash<ClockTimeValue, QList<Placeholder> > m_rules;
        ClockMode m_mode;

    signals:
        void themeChanged();
        void eventsChanged();
        void timezoneChanged(QString timezone);
        void secondChanged(int second);
        void minuteChanged(int minute);
        void hourChanged(int hour);
        void dayChanged(int day);
        void weekChanged(int week);
        void monthChanged(int month);
        void yearChanged(int year);
};

}

#endif
