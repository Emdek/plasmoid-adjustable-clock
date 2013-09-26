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

#include "Clock.h"
#include "Applet.h"

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

namespace AdjustableClock
{

DataSource::DataSource(Applet *applet) : QObject(applet),
    m_applet(applet),
    m_timeZoneOffsetSeconds(0)
{
    m_constantDateTime = QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15));

    m_eventsQuery = QString("events:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

    m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);

    updateTimeZone();
}

void DataSource::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool reload)
{
    if (source == m_eventsQuery) {
        m_events.clear();

        if (data.isEmpty()) {
            emit componentChanged(EventsComponent);

            return;
        }

        QHash<QString, QVariant>::const_iterator iterator;
        const QPair<QDateTime, QDateTime> limits = qMakePair(QDateTime::currentDateTime().addSecs(-43200), QDateTime::currentDateTime().addSecs(43200));

        for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator) {
            const QVariantHash eventData = iterator.value().toHash();

            if (eventData["Type"] == "Event" || eventData["Type"] == "Todo") {
                const KDateTime startTime = eventData["StartDate"].value<KDateTime>();
                const KDateTime endTime = eventData["EndDate"].value<KDateTime>();

                if ((endTime.isValid() && endTime.dateTime() < limits.first && endTime != startTime) || startTime.dateTime() > limits.second) {
                    continue;
                }

                Event event;
                event.type = ((eventData["Type"] == "Event") ? i18n("Event") : i18n("To do"));
                event.summary = eventData["Summary"].toString();

                if (startTime.time().hour() == 0 && startTime.time().minute() == 0 && endTime.time().hour() == 0 && endTime.time().minute() == 0) {
                    event.time = i18n("All day");
                } else if (startTime.isValid()) {
                    event.time = KGlobal::locale()->formatTime(startTime.time(), false);

                    if (endTime.isValid()) {
                        event.time.append(QString(" - %1").arg(KGlobal::locale()->formatTime(endTime.time(), false)));
                    }
                }

                m_events.append(event);
            }
        }

        emit componentChanged(EventsComponent);

        return;
    }

    if (source == m_weatherQuery && !m_weatherQuery.isEmpty()) {
        return;
    }

    const QDateTime previous = m_dateTime;

    m_dateTime = QDateTime(data["Date"].toDate(), data["Time"].toTime());

    emit tick();
    emit componentChanged(SecondComponent);
    emit componentChanged(TimestampComponent);
    emit componentChanged(TimeComponent);
    emit componentChanged(DateTimeComponent);

    if (reload || m_dateTime.time().minute() != previous.time().minute()) {
        emit componentChanged(MinuteComponent);
    }

    if (reload || m_dateTime.time().hour() != previous.time().hour()) {
        emit componentChanged(HourComponent);

        if (reload || m_dateTime.toString("ap") != previous.toString("ap")) {
            emit componentChanged(TimeOfDayComponent);
        }
    }

    if (reload || m_dateTime.date() != previous.date()) {
        if (reload || m_applet->calendar()->month(m_dateTime.date()) != m_applet->calendar()->month(previous.date())) {
            if (reload || m_applet->calendar()->year(m_dateTime.date()) != m_applet->calendar()->year(previous.date())) {
                emit componentChanged(YearComponent);
            }

            emit componentChanged(MonthComponent);
        }

        m_applet->dataEngine("calendar")->disconnectSource(m_eventsQuery, this);

        m_eventsQuery = QString("evens:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

        m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);

        const KTimeZone timeZone = KSystemTimeZones::zone(m_applet->isLocalTimezone() ? KSystemTimeZones::local().name() : m_applet->currentTimezone());
        const Plasma::DataEngine::Data sunData = m_applet->dataEngine("time")->query((timeZone.latitude() == KTimeZone::UNKNOWN) ? QString("%1|Solar").arg(m_applet->currentTimezone()) : QString("%1|Solar|Latitude=%2|Longitude=%3").arg(m_applet->currentTimezone()).arg(timeZone.latitude()).arg(timeZone.longitude()));

        m_sunrise = sunData["Sunrise"].toDateTime().time();
        m_sunset = sunData["Sunset"].toDateTime().time();

        const QString key = QString("holidays:%1:%2").arg(m_applet->config().readEntry("holidaysRegions", m_applet->dataEngine("calendar")->query("holidaysDefaultRegion")["holidaysDefaultRegion"]).toString().split(QChar(',')).first()).arg(m_dateTime.date().toString(Qt::ISODate));
        const Plasma::DataEngine::Data holidaysData = m_applet->dataEngine("calendar")->query(key);

        m_holidays.clear();

        if (!holidaysData.isEmpty() && holidaysData.contains(key)) {
            const QVariantList holidays = holidaysData[key].toList();

            for (int i = 0; i < holidays.length(); ++i) {
                m_holidays.append(holidays[i].toHash()["Name"].toString());
            }
        }

        emit componentChanged(DayOfWeekComponent);
        emit componentChanged(DayOfMonthComponent);
        emit componentChanged(DayOfYearComponent);
        emit componentChanged(DateComponent);
        emit componentChanged(SunriseComponent);
        emit componentChanged(SunsetComponent);
        emit componentChanged(HolidaysComponent);
    }
}

void DataSource::updateTimeZone()
{
    const QString currentTimeZone = (m_applet->isLocalTimezone() ? KSystemTimeZones::local().name() : m_applet->currentTimezone());

    if (!m_timeQuery.isEmpty()) {
        m_applet->dataEngine("time")->disconnectSource(m_timeQuery, this);
    }

    m_timeQuery = currentTimeZone;

    m_applet->dataEngine("time")->connectSource(m_timeQuery, this, 1000, Plasma::NoAlignment);

    QStringList timeZones = m_applet->config().readEntry("timeZones", QStringList());

    if (!timeZones.contains(KSystemTimeZones::local().name())) {
        timeZones.append(KSystemTimeZones::local().name());
    }

    m_timeZones.clear();

    for (int i = 0; i < timeZones.length(); ++i) {
        const KTimeZone timeZone = KSystemTimeZones::zone(timeZones.at(i));
        const QString name = i18n(timeZone.name().toUtf8().data()).replace(QChar('_'), QChar(' ')).split(QChar('/')).last();

        m_timeZones[name] = timeZones.at(i);

        if (timeZones.at(i) == currentTimeZone) {
            m_timeZoneArea = name;
            m_timeZoneAbbreviation = QString::fromLatin1(timeZone.abbreviation(QDateTime::currentDateTime().toUTC()));

            if (m_timeZoneAbbreviation.isEmpty()) {
                m_timeZoneAbbreviation = i18n("UTC");
            }

            const int seconds = timeZone.currentOffset(Qt::UTC);
            const int hours = abs(seconds / 3600);
            const int minutes = (abs(seconds / 60) - (hours * 60));

            m_timeZoneOffsetString = QString("%1%2").arg((seconds >= 0) ? QChar('+') : QChar('-')).arg(hours);
            m_timeZoneOffsetSeconds = seconds;

            if (minutes) {
                m_timeZoneOffsetString.append(QChar(':'));
                m_timeZoneOffsetString.append(formatNumber(minutes, 2));
            }
        }
    }

    emit componentChanged(TimeZoneNameComponent);
    emit componentChanged(TimeZoneAbbreviationComponent);
    emit componentChanged(TimeZoneOffsetComponent);

    dataUpdated(QString(), m_applet->dataEngine("time")->query(m_applet->currentTimezone()), true);
}

QString DataSource::formatNumber(int number, int length)
{
    return QString("%1").arg(number, length, 10, QChar('0'));
}

QVariant DataSource::getOption(const QString &key, const QVariant &defaultValue, const QString &theme) const
{
    return m_applet->config().group("theme-" + (theme.isEmpty() ? m_applet->config().readEntry("theme", "digital") : theme)).readEntry(key, defaultValue);
}

QVariant DataSource::getValue(ClockComponent component, const QVariantMap &options, bool constant) const
{
    const QDateTime dateTime = (constant ? m_constantDateTime : m_dateTime);

    switch (component) {
    case SecondComponent:
        return formatNumber(dateTime.time().second(), (options.contains("short") ? 0 : 2));
    case MinuteComponent:
        return formatNumber(dateTime.time().minute(), (options.contains("short") ? 0 : 2));
    case HourComponent:
        return formatNumber(((options.contains("alternative") ? options["alternative"].toBool() : KGlobal::locale()->use12Clock()) ? (((dateTime.time().hour() + 11) % 12) + 1) : dateTime.time().hour()), (options.contains("short") ? 0 : 2));
    case TimeOfDayComponent:
        return ((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));
    case DayOfMonthComponent:
        return formatNumber(m_applet->calendar()->day(dateTime.date()), (options.contains("short") ? 0 : 2));
    case DayOfWeekComponent:
        if (options.contains("text")) {
            return m_applet->calendar()->weekDayName(m_applet->calendar()->dayOfWeek(dateTime.date()), (options.contains("short") ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));
        }

        return formatNumber(m_applet->calendar()->dayOfWeek(dateTime.date()), (options.contains("short") ? 0 : QString::number(m_applet->calendar()->daysInWeek(dateTime.date())).length()));
    case DayOfYearComponent:
        return formatNumber(m_applet->calendar()->dayOfYear(dateTime.date()), (options.contains("short") ? 0 : QString::number(m_applet->calendar()->daysInYear(dateTime.date())).length()));
    case WeekComponent:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Week, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case MonthComponent:
        if (options.contains("text")) {
            const bool possessiveForm = (options.contains("possessive") ? options["possessive"].toBool() : KGlobal::locale()->dateMonthNamePossessive());

            return m_applet->calendar()->monthName(dateTime.date(), (options.contains("short") ? (possessiveForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (possessiveForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));
        }

        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Month, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case YearComponent:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Year, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case TimestampComponent:
        return QString::number((dateTime.addSecs(-m_timeZoneOffsetSeconds)).toTime_t());
    case TimeComponent:
        return KGlobal::locale()->formatTime(dateTime.time(), !options.contains("short"));
    case DateComponent:
        return KGlobal::locale()->formatDate(dateTime.date(), (options.contains("short") ? KLocale::ShortDate : KLocale::LongDate));
    case DateTimeComponent:
        return KGlobal::locale()->formatDateTime(dateTime, (options.contains("short") ? KLocale::ShortDate : KLocale::LongDate));
    case TimeZoneNameComponent:
        return m_timeZoneArea;
    case TimeZoneAbbreviationComponent:
        return m_timeZoneAbbreviation;
    case TimeZoneOffsetComponent:
        return m_timeZoneOffsetString;
    case TimeZonesComponent:
        if (m_timeZones.count() > 1) {
            QStringList timeZones;
            QMapIterator<QString, QString> iterator(m_timeZones);

            while (iterator.hasNext()) {
                iterator.next();

                const Plasma::DataEngine::Data data = m_applet->dataEngine("time")->query(iterator.value());

                timeZones.append(QString("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\"><nobr>%2 %3</nobr></td>").arg(iterator.key()).arg(KGlobal::locale()->formatTime(data["Time"].toTime(), false)).arg(KGlobal::locale()->formatDate(data["Date"].toDate(), KLocale::LongDate)));
            }

            return QString("<table>\n<tr>%1</tr>\n</table>").arg(timeZones.join("</tr>\n<tr>"));
        }

        return QString();
    case EventsComponent:
        if (!m_events.isEmpty()) {
            QStringList events;

            for (int i = 0; i < m_events.count(); ++i) {
                if (options.contains("short")) {
                    events.append(QString("<td align=\"right\"><nobr><i>%1</i>:</nobr></td>\n<td align=\"left\">%2</td>\n").arg(m_events.at(i).type).arg(m_events.at(i).summary));
                } else {
                    events.append(QString("<td align=\"right\"><nobr><i>%1</i>:</nobr></td>\n<td align=\"left\">%2 <nobr>(%3)</nobr></td>\n").arg(m_events.at(i).type).arg(m_events.at(i).summary).arg(m_events.at(i).time));
                }
            }

            return QString("<table>\n<tr>\n%1</tr>\n</table>").arg(events.join("</tr>\n<tr>\n"));
        }

        return QString();
    case HolidaysComponent:
        return (constant ? i18n("New Year") : (options.contains("short") ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join("<br>\n")));
    case SunriseComponent:
        return KGlobal::locale()->formatTime(m_sunrise, false);
    case SunsetComponent:
        return KGlobal::locale()->formatTime(m_sunset, false);
    default:
        return QString();
    }

    return QString();
}

}
