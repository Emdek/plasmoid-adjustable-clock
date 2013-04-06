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
        DataSource(Applet *parent);

        void updateTimeZone();
        QString toString(ClockComponent component, const QVariantMap &options = QVariantMap(), bool constant = false) const;

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
        void tick();
        void componentChanged(ClockComponent component);
};

}

#endif
