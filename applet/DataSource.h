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

#ifndef ADJUSTABLECLOCKDATASOURCE_HEADER
#define ADJUSTABLECLOCKDATASOURCE_HEADER

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>

#include <Plasma/DataEngine>

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

struct Event
{

}

class Applet;

class DataSource : public QObject
{
    Q_OBJECT

    public:
        DataSource(Applet *parent);

        void connectSource(const QString &timezone);
        QDateTime getCurrentDateTime() const;
        QString getTimeString(ClockTimeValue type, ValueOptions options) const;
        QVariantList getEventsList(ClockEventsValue type, ValueOptions options) const;

    protected slots:
        void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);

    private:
        Applet *m_applet;
        QDateTime m_dateTime;
        QTime m_sunrise;
        QTime m_sunset;
        QString m_timezoneAbbreviation;
        QString m_timezoneOffset;
        QString m_timeQuery;
        QString m_eventsQuery;
        QStringList m_holidays;

    signals:
        void timezoneChanged(QString timezone);
        void eventsChanged();
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
