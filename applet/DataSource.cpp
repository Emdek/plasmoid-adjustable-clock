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
}

void DataSource::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data)
{
    QList<ClockTimeValue> changes;

    if (source == m_eventsQuery) {
        m_events.clear();

        changes.append(EventsValue);

        if (data.isEmpty()) {
            emit dataChanged(changes);

            return;
        }

        QHash<QString, QVariant>::const_iterator i;
        QPair<QDateTime, QDateTime> limits = qMakePair(QDateTime::currentDateTime().addSecs(-43200), QDateTime::currentDateTime().addSecs(43200));

        for (i = data.constBegin(); i != data.constEnd(); ++i) {
            QVariantHash eventData = i.value().toHash();

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

    changes.append(SecondValue);
    changes.append(TimestampValue);
    changes.append(TimeValue);
    changes.append(DateTimeValue);

    if (m_dateTime.time().second() == 0) {
        if (m_dateTime.time().minute() == 0) {
            const int hour = m_dateTime.time().hour();

            if (hour == 0) {
                if (m_applet->calendar()->day(m_dateTime.date()) == 1) {
                    if (m_applet->calendar()->dayOfYear(m_dateTime.date()) == 1) {
                        changes.append(YearValue);
                    }

                    changes.append(MonthValue);
                }

                m_applet->dataEngine("calendar")->disconnectSource(m_eventsQuery, this);

                m_eventsQuery = QString("evens:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

                m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);

                Plasma::DataEngine::Data sunData = m_applet->dataEngine("time")->query(QString("%1|Solar").arg(m_applet->currentTimezone()));

                m_sunrise = sunData["Sunrise"].toDateTime().time();
                m_sunset = sunData["Sunset"].toDateTime().time();

                const QString region = m_applet->config().readEntry("holidaysRegions", m_applet->dataEngine("calendar")->query("holidaysDefaultRegion")["holidaysDefaultRegion"]).toString().split(QChar(',')).first();
                const QString key = QString("holidays:%1:%2").arg(region).arg(getCurrentDateTime().date().toString(Qt::ISODate));
                Plasma::DataEngine::Data holidaysData = m_applet->dataEngine("calendar")->query(key);

                m_holidays.clear();

                if (!holidaysData.isEmpty() && holidaysData.contains(key)) {
                    QVariantList holidays = holidaysData[key].toList();

                    for (int i = 0; i < holidays.length(); ++i) {
                        m_holidays.append(holidays[i].toHash()["Name"].toString());
                    }
                }

                m_applet->updateSize();

                changes.append(DayOfWeekValue);
                changes.append(DayOfMonthValue);
                changes.append(DayOfYearValue);
                changes.append(DateValue);
                changes.append(SunriseValue);
                changes.append(SunsetValue);
                changes.append(HolidaysValue);
            } else if (hour == 0 || hour == 12) {
                changes.append(TimeOfDayValue);
            }

            changes.append(HourValue);
        }

        changes.append(MinuteValue);
    }

    emit dataChanged(changes);
}

void DataSource::setTimezone(const QString &timezone)
{
    if (!m_timeQuery.isEmpty()) {
        m_applet->dataEngine("time")->disconnectSource(m_timeQuery, this);
    }

    m_timeQuery = timezone;

    m_applet->dataEngine("time")->connectSource(m_timeQuery, this, 1000, Plasma::NoAlignment);

    if (m_eventsQuery.isEmpty()) {
        m_eventsQuery = QString("events:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

        m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);
    }

    const KTimeZone timezoneData = (m_applet->isLocalTimezone() ? KSystemTimeZones::local() : KSystemTimeZones::zone(m_applet->currentTimezone()));

    m_timeZoneAbbreviation = QString::fromLatin1(timezoneData.abbreviation(QDateTime::currentDateTime().toUTC()));

    if (m_timeZoneAbbreviation.isEmpty()) {
        m_timeZoneAbbreviation = i18n("UTC");
    }

    m_timeZoneArea = i18n(timezoneData.name().toUtf8().data()).replace(QChar('_'), QChar(' ')).split(QChar('/'));

    m_timeZones = m_applet->config().readEntry("timeZones", QStringList());
    m_timeZones.prepend("");

    int seconds = timezoneData.currentOffset(Qt::UTC);
    int minutes = abs(seconds / 60);
    int hours = abs(minutes / 60);

    minutes = (minutes - (hours * 60));

    m_timeZoneOffset = QString::number(hours);

    if (minutes) {
        m_timeZoneOffset.append(QChar(':'));
        m_timeZoneOffset.append(formatNumber(minutes, 2));
    }

    m_timeZoneOffset = (QChar((seconds >= 0) ? QChar('+') : QChar('-')) + m_timeZoneOffset);

    dataUpdated(QString(), m_applet->dataEngine("time")->query(m_applet->currentTimezone()));
}

QString DataSource::formatNumber(int number, int length)
{
    return QString("%1").arg(number, length, 10, QChar('0'));
}

QDateTime DataSource::getCurrentDateTime() const
{
    return m_dateTime;
}

QString DataSource::toString(ClockTimeValue value, const QVariantMap &options, QDateTime dateTime) const
{
    if (!dateTime.isValid()) {
        dateTime = m_dateTime;
    }

    switch (value) {
    case SecondValue:
        return formatNumber(dateTime.time().second(), (options.contains("short") ? 0 : 2));
    case MinuteValue:
        return formatNumber(dateTime.time().minute(), (options.contains("short") ? 0 : 2));
    case HourValue:
        return formatNumber(((options.contains("alternative") ? options["alternative"].toBool() : KGlobal::locale()->use12Clock()) ? (((dateTime.time().hour() + 11) % 12) + 1) : dateTime.time().hour()), (options.contains("short") ? 0 : 2));
    case TimeOfDayValue:
        return ((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));
    case DayOfMonthValue:
        return formatNumber(m_applet->calendar()->day(dateTime.date()), (options.contains("short") ? 0 : 2));
    case DayOfWeekValue:
        if (options.contains("text")) {
            return m_applet->calendar()->weekDayName(m_applet->calendar()->dayOfWeek(dateTime.date()), (options.contains("short") ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));
        }

        return formatNumber(m_applet->calendar()->dayOfWeek(dateTime.date()), (options.contains("short") ? 0 : QString::number(m_applet->calendar()->daysInWeek(dateTime.date())).length()));
    case DayOfYearValue:
        return formatNumber(m_applet->calendar()->dayOfYear(dateTime.date()), (options.contains("short") ? 0 : QString::number(m_applet->calendar()->daysInYear(dateTime.date())).length()));
    case WeekValue:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Week, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case MonthValue:
        if (options.contains("text")) {
            const bool possessiveForm = (options.contains("possessive") ? options["possessive"].toBool() : KGlobal::locale()->dateMonthNamePossessive());

            return m_applet->calendar()->monthName(dateTime.date(), (options.contains("short") ? (possessiveForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (possessiveForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));
        }

        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Month, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case YearValue:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Year, (options.contains("short") ? KLocale::ShortNumber : KLocale::LongNumber));
    case TimestampValue:
        return QString::number(dateTime.toTime_t());
    case TimeValue:
        return KGlobal::locale()->formatTime(dateTime.time(), !options.contains("short"));
    case DateValue:
        return KGlobal::locale()->formatDate(dateTime.date(), (options.contains("short") ? KLocale::ShortDate : KLocale::LongDate));
    case DateTimeValue:
        return KGlobal::locale()->formatDateTime(dateTime, (options.contains("short") ? KLocale::ShortDate : KLocale::LongDate));
    case TimeZoneNameValue:
        return (options.contains("short") ? (m_timeZoneArea.isEmpty() ? QString() : m_timeZoneArea.last()) : m_timeZoneArea.join(QString(QChar('/'))));
    case TimeZoneAbbreviationValue:
        return m_timeZoneAbbreviation;
    case TimeZoneOffsetValue:
        return m_timeZoneOffset;
    case TimeZonesValue:
        if (m_timeZones.length() == 1) {
            return QString();
        } else {
            QStringList timeZones;

            for (int i = 0; i < m_timeZones.length(); ++i) {
                QString timeZone = i18n((m_timeZones.at(i).isEmpty() ? KSystemTimeZones::local() : KSystemTimeZones::zone(m_timeZones.at(i))).name().toUtf8().data()).replace(QChar('_'), QChar(' '));

                if (options.contains("short") && timeZone.contains(QChar('/'))) {
                    timeZone = timeZone.split(QChar('/')).last();
                }

                Plasma::DataEngine::Data data = m_applet->dataEngine("time")->query(m_timeZones.at(i));

                timeZones.append(QString("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\"><nobr>%2 %3</nobr></td>").arg(timeZone).arg(KGlobal::locale()->formatTime(data["Time"].toTime(), false)).arg(KGlobal::locale()->formatDate(data["Date"].toDate(), KLocale::LongDate)));
            }

            return QString("<table>\n<tr>%1</tr>\n</table>").arg(timeZones.join("</tr>\n<tr>"));
        }
    case EventsValue:
        if (m_events.isEmpty()) {
            return QString();
        } else {
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
    case HolidaysValue:
        return (options.contains("short") ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join("<br>\n"));
    case SunriseValue:
        return KGlobal::locale()->formatTime(m_sunrise, false);
    case SunsetValue:
        return KGlobal::locale()->formatTime(m_sunset, false);
    default:
        return QString();
    }

    return QString();
}

}
