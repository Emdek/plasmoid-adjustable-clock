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
//     ForecastDurationComponent = 23,
//     TemperatureCurrentComponent = 24,
//     TemperatureMinimumComponent = 25,
//     TemperatureMaximumComponent = 26,
//     ConditionsComponent = 27,
//     WindDirectionComponent = 28,
//     WindSpeedComponent = 29,
//     PressureComponent = 30,
//     HumidityComponent = 31,
//     LocationComponent = 32,
    LastComponent = 23
};

struct Event
{
    QString type;
    QString time;
    QString summary;
};

class Applet;

class DataSource : public QObject
{
    Q_OBJECT

    public:
        explicit DataSource(Applet *applet);

        void updateTimeZone();
        QVariant getOption(const QString &key, const QVariant &defaultValue, const QString &theme = QString()) const;
        QVariant getValue(ClockComponent component, const QVariantMap &options = QVariantMap(), bool constant = false) const;

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
//         QString m_weatherQuery;
        QString m_timeZoneArea;
//         QString m_location;
//         QString m_conditionsText;
//         QString m_conditionsIcon;
//         QString m_windDirection;
        QStringList m_holidays;
        QList<Event> m_events;
        QMap<QString, QString> m_timeZones;
//         int m_windSpeed;
//         int m_pressure;
//         int m_humidity;
//         int m_temperatureCurrent;
//         int m_temperatureMinimum;
//         int m_temperatureMaximum;

    signals:
        void componentChanged(ClockComponent component);
        void tick();
};

}

#endif
