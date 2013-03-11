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

    if (!source.isEmpty() && source == m_eventsQuery) {
        m_events.clear();

        changes.append(EventsValue);

        if (data.isEmpty()) {
            emit eventsChanged();
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

        emit eventsChanged();
        emit dataChanged(changes);

        return;
    }

    m_dateTime = QDateTime(data["Date"].toDate(), data["Time"].toTime());

    emit secondChanged();

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
                        emit yearChanged();

                        changes.append(YearValue);
                    }

                    emit monthChanged();

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

                emit dayChanged();

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

            emit hourChanged();

            changes.append(HourValue);
        }

        emit minuteChanged();

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

    m_timezoneAbbreviation = QString::fromLatin1(timezoneData.abbreviation(QDateTime::currentDateTime().toUTC()));

    if (m_timezoneAbbreviation.isEmpty()) {
        m_timezoneAbbreviation = i18n("UTC");
    }

    m_timezoneArea = i18n(timezoneData.name().toUtf8().data()).replace(QChar('_'), QChar(' ')).split(QChar('/'));

    int seconds = timezoneData.currentOffset(Qt::UTC);
    int minutes = abs(seconds / 60);
    int hours = abs(minutes / 60);

    minutes = (minutes - (hours * 60));

    m_timezoneOffset = QString::number(hours);

    if (minutes) {
        m_timezoneOffset.append(QChar(':'));
        m_timezoneOffset.append(formatNumber(minutes, 2));
    }

    m_timezoneOffset = (QChar((seconds >= 0) ? QChar('+') : QChar('-')) + m_timezoneOffset);

    dataUpdated(QString(), m_applet->dataEngine("time")->query(m_applet->currentTimezone()));

    emit timezoneChanged();
}

QString DataSource::formatNumber(int number, int length)
{
    return QString("%1").arg(number, length, 10, QChar('0'));
}

QDateTime DataSource::getCurrentDateTime() const
{
    return m_dateTime;
}

QString DataSource::getTimeString(ClockTimeValue value, ValueOptions options, QDateTime dateTime) const
{
    QStringList timezones;
    bool possesiveForm = false;

    if (!dateTime.isValid()) {
        dateTime = m_dateTime;
    }

    switch (value) {
    case SecondValue:
        return formatNumber(dateTime.time().second(), ((options & ShortFormOption) ? 0 : 2));
    case MinuteValue:
        return formatNumber(dateTime.time().minute(), ((options & ShortFormOption) ? 0 : 2));
    case HourValue:
        return formatNumber(((options & AlternativeFormOption || KGlobal::locale()->use12Clock()) ? (((dateTime.time().hour() + 11) % 12) + 1) : dateTime.time().hour()), ((options & ShortFormOption) ? 0 : 2));
    case TimeOfDayValue:
        return ((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));
    case DayOfMonthValue:
        return formatNumber(m_applet->calendar()->day(dateTime.date()), ((options & ShortFormOption) ? 0 : 2));
    case DayOfWeekValue:
        if (options & TextualFormOption) {
            return m_applet->calendar()->weekDayName(m_applet->calendar()->dayOfWeek(dateTime.date()), ((options & ShortFormOption) ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));
        }

        return formatNumber(m_applet->calendar()->dayOfWeek(dateTime.date()), ((options & ShortFormOption) ? 0 : QString::number(m_applet->calendar()->daysInWeek(dateTime.date())).length()));
    case DayOfYearValue:
        return formatNumber(m_applet->calendar()->dayOfYear(dateTime.date()), ((options & ShortFormOption) ? 0 : QString::number(m_applet->calendar()->daysInYear(dateTime.date())).length()));
    case WeekValue:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Week, ((options & ShortFormOption) ? KLocale::ShortNumber : KLocale::LongNumber));
    case MonthValue:
        if (options & TextualFormOption) {
            possesiveForm = ((!(options & PossessiveFormOption) && !(options & NonPossessiveFormOption)) ? KGlobal::locale()->dateMonthNamePossessive() : (options & PossessiveFormOption));

            return m_applet->calendar()->monthName(dateTime.date(), ((options & ShortFormOption) ? (possesiveForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (possesiveForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));
        }

        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Month, ((options & ShortFormOption) ? KLocale::ShortNumber : KLocale::LongNumber));
    case YearValue:
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Year, ((options & ShortFormOption) ? KLocale::ShortNumber : KLocale::LongNumber));
    case TimestampValue:
        return QString::number(dateTime.toTime_t());
    case TimeValue:
        return KGlobal::locale()->formatTime(dateTime.time(), !(options & ShortFormOption));
    case DateValue:
        return KGlobal::locale()->formatDate(dateTime.date(), ((options & ShortFormOption) ? KLocale::ShortDate : KLocale::LongDate));
    case DateTimeValue:
        return KGlobal::locale()->formatDateTime(dateTime, ((options & ShortFormOption) ? KLocale::ShortDate : KLocale::LongDate));
    case TimezoneNameValue:
        return ((options & ShortFormOption) ? (m_timezoneArea.isEmpty() ? QString() : m_timezoneArea.last()) : m_timezoneArea.join(QString(QChar('/'))));
    case TimezoneAbbreviationValue:
        return m_timezoneAbbreviation;
    case TimezoneOffsetValue:
        return m_timezoneOffset;
    case TimezoneListValue:
        timezones = m_applet->config().readEntry("timeZones", QStringList());
        timezones.prepend("");

        if (timezones.length() == 1) {
            return QString();
        }

        for (int i = 0; i < timezones.length(); ++i) {
            QString timezone = i18n((timezones.at(i).isEmpty() ? KSystemTimeZones::local() : KSystemTimeZones::zone(timezones.at(i))).name().toUtf8().data()).replace(QChar('_'), QChar(' '));

            if ((options & ShortFormOption) && timezone.contains(QChar('/'))) {
                timezone = timezone.split(QChar('/')).last();
            }

            Plasma::DataEngine::Data data = m_applet->dataEngine("time")->query(timezones.at(i));

            timezones[i] = QString("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\"><nobr>%2 %3</nobr></td>").arg(timezone).arg(KGlobal::locale()->formatTime(data["Time"].toTime(), false)).arg(KGlobal::locale()->formatDate(data["Date"].toDate(), KLocale::LongDate));
        }

        return QString("<table>\n<tr>%1</tr>\n</table>").arg(timezones.join("</tr>\n<tr>"));
    case EventsValue:
//        return ((options & ShortFormOption) ? m_eventsShort : m_eventsLong);
    case HolidaysValue:
        return ((options & ShortFormOption) ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join("<br>\n"));
    case SunriseValue:
        return KGlobal::locale()->formatTime(m_sunrise, false);
    case SunsetValue:
        return KGlobal::locale()->formatTime(m_sunset, false);
    default:
        return QString();
    }

    return QString();
}

QVariantList DataSource::getEventsList(ClockEventsType value, ValueOptions options) const
{
    return QVariantList();
}

}
