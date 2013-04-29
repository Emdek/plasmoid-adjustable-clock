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

#include <QtScript/QScriptEngine>

#include <Plasma/DataEngine>

namespace AdjustableClock
{

enum ClockComponent
{
    InvalidComponent = 0,
    SecondComponent = 1,
    MinuteComponent = 2,
    HourComponent = 3,
    TimeOfDayComponent = 4,
    DayOfWeekComponent = 5,
    DayOfMonthComponent = 6,
    DayOfYearComponent = 7,
    WeekComponent = 8,
    MonthComponent = 9,
    YearComponent = 10,
    TimestampComponent = 11,
    TimeComponent = 12,
    DateComponent = 13,
    DateTimeComponent = 14,
    TimeZoneNameComponent = 15,
    TimeZoneAbbreviationComponent = 16,
    TimeZoneOffsetComponent = 17,
    TimeZonesComponent = 18,
    EventsComponent = 19,
    HolidaysComponent = 20,
    SunriseComponent = 21,
    SunsetComponent = 22,
    LastComponent = 23
};

struct Event
{
    QString type;
    QString time;
    QString summary;
};

class Applet;
class Clock;

class ClockObject : public QObject
{
    Q_OBJECT

    public:
        explicit ClockObject(Clock *clock, bool constant, const QString &theme = QString());

        Q_INVOKABLE QVariant getOption(const QString &key, const QVariant &defaultValue = QVariant()) const;
        Q_INVOKABLE QVariant getValue(int component, const QVariantMap &options = QVariantMap()) const;

    private:
        Clock *m_clock;
        QString m_theme;
        bool m_constant;
};

class Clock : public QObject
{
    Q_OBJECT

    public:
        explicit Clock(Applet *applet);

        void updateTimeZone();
        QString evaluate(const QString &script, bool constant = false);
        QVariant getOption(const QString &key, const QVariant &defaultValue, const QString &theme = QString()) const;
        QVariant getValue(ClockComponent component, const QVariantMap &options = QVariantMap(), bool constant = false) const;
        static QString getComponentName(ClockComponent component);
        static QLatin1String getComponentString(ClockComponent component);

    protected:
        static QString formatNumber(int number, int length);

    protected slots:
        void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data, bool reload = false);

    private:
        Applet *m_applet;
        QDateTime m_dateTime;
        QDateTime m_constantDateTime;
        QTime m_sunrise;
        QTime m_sunset;
        QString m_timeZoneAbbreviation;
        QString m_timeZoneOffset;
        QString m_timeQuery;
        QString m_eventsQuery;
        QString m_timeZoneArea;
        QStringList m_holidays;
        QList<Event> m_events;
        QMap<QString, QString> m_timeZones;

    signals:
        void componentChanged(ClockComponent component);
        void tick();
};

}

#endif
