/***********************************************************************************
* Adjustable DataSource: Plasmoid to show date and time in adjustable format.
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

#include "DataSource.h"
#include "Applet.h"

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

namespace AdjustableClock
{

DataSource::DataSource(Applet *parent) : QObject(parent),
    m_applet(parent)
{
    m_eventsQuery = QString("events:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

    m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);
}

void DataSource::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool reload)
{
    QList<ClockComponent> changes;

    if (source == m_eventsQuery) {
        m_events.clear();

        changes.append(EventsComponent);

        if (data.isEmpty()) {
            emit dataChanged(changes);

            return;
        }

        QHash<QString, QVariant>::const_iterator iterator;
        const QPair<QDateTime, QDateTime> limits = qMakePair(QDateTime::currentDateTime().addSecs(-43200), QDateTime::currentDateTime().addSecs(43200));

        for (iterator = data.constBegin(); iterator != data.constEnd(); ++iterator) {
            QVariantHash eventData = iterator.value().toHash();

            if (eventData["Type"] == "Event" || eventData["Type"] == "Todo") {
                KDateTime startTime = eventData["StartDate"].value<KDateTime>();
                KDateTime endTime = eventData["EndDate"].value<KDateTime>();

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

        emit dataChanged(changes);

        return;
    }

    m_dateTime = QDateTime(data["Date"].toDate(), data["Time"].toTime());

    changes.append(SecondComponent);
    changes.append(TimestampComponent);
    changes.append(TimeComponent);
    changes.append(DateTimeComponent);

    if (m_dateTime.time().second() == 0 || reload) {
        if (m_dateTime.time().minute() == 0 || reload) {
            const int hour = m_dateTime.time().hour();

            if (hour == 0 || reload) {
                if (m_applet->calendar()->day(m_dateTime.date()) == 1) {
                    if (m_applet->calendar()->dayOfYear(m_dateTime.date()) == 1) {
                        changes.append(YearComponent);
                    }

                    changes.append(MonthComponent);
                }

                m_applet->dataEngine("calendar")->disconnectSource(m_eventsQuery, this);

                m_eventsQuery = QString("evens:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

                m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);

                const KTimeZone timeZone = KSystemTimeZones::zone(m_applet->isLocalTimezone() ? KSystemTimeZones::local().name() : m_applet->currentTimezone());
                const Plasma::DataEngine::Data sunData = m_applet->dataEngine("time")->query((timeZone.latitude() == KTimeZone::UNKNOWN) ? QString("%1|Solar").arg(m_applet->currentTimezone()) : QString("%1|Solar|Latitude=%2|Longitude=%3").arg(m_applet->currentTimezone()).arg(timeZone.latitude()).arg(timeZone.longitude()));

                m_sunrise = sunData["Sunrise"].toDateTime().time();
                m_sunset = sunData["Sunset"].toDateTime().time();

                const QString region = m_applet->config().readEntry("holidaysRegions", m_applet->dataEngine("calendar")->query("holidaysDefaultRegion")["holidaysDefaultRegion"]).toString().split(QChar(',')).first();
                const QString key = QString("holidays:%1:%2").arg(region).arg(m_dateTime.date().toString(Qt::ISODate));
                const Plasma::DataEngine::Data holidaysData = m_applet->dataEngine("calendar")->query(key);

                m_holidays.clear();

                if (!holidaysData.isEmpty() && holidaysData.contains(key)) {
                    const QVariantList holidays = holidaysData[key].toList();

                    for (int i = 0; i < holidays.length(); ++i) {
                        m_holidays.append(holidays[i].toHash()["Name"].toString());
                    }
                }

                m_applet->updateSize();

                changes.append(DayOfWeekComponent);
                changes.append(DayOfMonthComponent);
                changes.append(DayOfYearComponent);
                changes.append(DateComponent);
                changes.append(SunriseComponent);
                changes.append(SunsetComponent);
                changes.append(HolidaysComponent);
            }

            if (hour == 0 || hour == 12) {
                changes.append(TimeOfDayComponent);
            }

            changes.append(HourComponent);
        }

        changes.append(MinuteComponent);
    }

    emit dataChanged(changes);
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

            m_timeZoneOffset = QString("%1%2").arg((seconds >= 0) ? QChar('+') : QChar('-')).arg(hours);

            if (minutes) {
                m_timeZoneOffset.append(QChar(':'));
                m_timeZoneOffset.append(formatNumber(minutes, 2));
            }
        }
    }

    dataUpdated(QString(), m_applet->dataEngine("time")->query(m_applet->currentTimezone()), true);
}

QString DataSource::formatNumber(int number, int length)
{
    return QString("%1").arg(number, length, 10, QChar('0'));
}

QString DataSource::toString(ClockComponent component, const QVariantMap &options, QDateTime dateTime) const
{
    if (!dateTime.isValid()) {
        dateTime = m_dateTime;
    }

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
        return QString::number(dateTime.toTime_t());
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
        return m_timeZoneOffset;
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
        return (options.contains("short") ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join("<br>\n"));
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
