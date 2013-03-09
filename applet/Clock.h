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

namespace AdjustableClock
{

enum ClockTimeValue
{
    SecondValue = 0,
    MinuteValue = 1,
    HourValue = 2,
    TimeOfDayValue = 3,
    DayOfWeekValue = 4,
    DayOfMonthValue = 5,
    DayOfYearValue = 6,
    WeekValue = 7,
    MonthValue = 8,
    YearValue = 9,
    TimestampValue = 10,
    TimeValue = 11,
    DateValue = 12,
    TimezoneValue = 13,
    SunriseValue = 14,
    SunsetValue = 15
};

enum ClockEventsValue
{
    EventsValue = 0,
    HolidaysValue = 1
};

enum ValueOption
{
    ValueDefaultForm = 0,
    ValueShortForm = 1,
    ValueLongForm = 2,
    ValueTextualForm = 4,
    ValuePossessiveForm = 8,
    ValueTimezoneName = 16,
    ValueTimezoneAbbreviation = 32,
    ValueTimezoneOffset = 64
};

Q_DECLARE_FLAGS(ValueOptions, ValueOption)

enum IntervalAlignment
{
    NoAlignment = 0,
    SecondAlignment = 1,
    MinuteAlignment = 2,
    HourAlignment = 3,
    DayAlignment = 4,
    WeekAlignment = 5,
    MonthAlignment = 6,
    YearAlignment = 7
};

// enum ClockFeature
// {
//     NoFeatures = 0,
//     SecondsClockFeature = 1,
//     SecondsToolTipFeature = 2,
//     HolidaysFeature = 4,
//     EventsFeature = 8
// };
//
// Q_DECLARE_FLAGS(ClockFeatures, ClockFeature)

struct PlaceholderRule
{
    QString rule;
    QString attribute;
    QString expression;
    IntervalAlignment alignment;
};

class Clock : public QObject
{
    Q_OBJECT

    public:
        Clock(QObject *parent);

        void setDocument(QWebFrame *document);
        Q_INVOKABLE void setRule(const QString &rule, const QString &attribute, const QString &expression, IntervalAlignment alignment);
        Q_INVOKABLE void setRule(const QString &rule, const QString &expression, IntervalAlignment alignment);
        Q_INVOKABLE void setValue(const QString &rule, const QString &attribute, const QString &value);
        Q_INVOKABLE void setValue(const QString &rule, const QString &value);
        Q_INVOKABLE QString getTimeString(ClockTimeValue type, ValueOptions options) const;
        Q_INVOKABLE QVariantList getEventsList(ClockEventsValue type, ValueOptions options) const;

    protected:
        void applyRule(const PlaceholderRule &rule);
        void setValue(const QWebElementCollection &elements, const QString &attribute, const QString &value);
        void setValue(const QWebElementCollection &elements, const QString &value);

    private:
        QHash<IntervalAlignment, QList<PlaceholderRule> > m_rules;
        QWebFrame *m_document;
        QScriptEngine m_engine;

    signals:
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
