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

namespace AdjustableClock
{

enum ClockComponent
{
    SecondComponent = 0,
    MinuteComponent = 1,
    HourComponent = 2,
    TimeOfDayComponent = 3,
    DayOfWeekComponent = 4,
    DayOfMonthComponent = 5,
    DayOfYearComponent = 6,
    WeekComponent = 7,
    MonthComponent = 8,
    YearComponent = 9,
    TimestampComponent = 10,
    TimeComponent = 11,
    DateComponent = 12,
    DateTimeComponent = 13,
    TimeZoneNameComponent = 14,
    TimeZoneAbbreviationComponent = 15,
    TimeZoneOffsetComponent = 16,
    TimeZonesComponent = 17,
    EventsComponent = 18,
    HolidaysComponent = 19,
    SunriseComponent = 20,
    SunsetComponent = 21,
    LastComponent = 22
};

enum ClockMode
{
    StandardClock = 0,
    StaticClock = 2,
    EditorClock = 3
};

struct Rule
{
    QString query;
    QString attribute;
    ClockComponent component;
    QVariantMap options;
};

class DataSource;

class Clock : public QObject
{
    Q_OBJECT

    public:
        Clock(DataSource *parent, ClockMode mode = StandardClock);

        void setDocument(QWebFrame *document);
        Q_INVOKABLE void setRule(const QString &query, const QString &attribute, int component, const QVariantMap &options = QVariantMap());
        Q_INVOKABLE void setRule(const QString &query, int component, const QVariantMap &options = QVariantMap());
        Q_INVOKABLE void setValue(const QString &query, const QString &attribute, const QString &value);
        Q_INVOKABLE void setValue(const QString &query, const QString &value);
        QString evaluate(const QString &script);
        Q_INVOKABLE QString toString(int component, const QVariantMap &options = QVariantMap()) const;
        static QString getComponentName(ClockComponent component);
        static QString getComponentString(ClockComponent component);

    protected:
        void applyRule(const Rule &rule);
        void setValue(const QWebElementCollection &elements, const QString &attribute, const QString &value);

    protected slots:
        void exposeClock();
        void updateClock(const QList<ClockComponent> &changes);
        void updateTheme();

    private:
        DataSource *m_source;
        QWebFrame *m_document;
        QScriptEngine m_engine;
        QHash<ClockComponent, QList<Rule> > m_rules;
        ClockMode m_mode;
};

}

#endif
