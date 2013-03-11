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

namespace AdjustableDataSource
{

DataSource::DataSource(Applet *parent) : QObject(parent),
    m_applet(parent)
{
}

void DataSource::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool force)
{
    if (!source.isEmpty() && source == m_eventDatasQuery) {

        m_events.clear();

        if (data.isEmpty()) {
            emit eventsChanged();

            return;
        }

        QHash<QString, QVariant>::iterator i;
        QStringList eventDatasShort;
        QStringList eventDatasLong;
        QPair<QDateTime, QDateTime> limits = qMakePair(QDateTime::currentDateTime().addSecs(-43200), QDateTime::currentDateTime().addSecs(43200));

        for (i = data.begin(); i != data.end(); ++i) {
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

        return;
    }

    m_dateTime = QDateTime(data["Date"].toDate(), data["Time"].toTime());

    const int second = ((m_features & SecondsDataSourceFeature || m_features & SecondsToolTipFeature) ? m_dateTime.time().second() : 0);

    if (QTime::currentTime().hour() == 0 && m_dateTime.time().minute() == 0 && second == 0) {
        m_applet->dataEngine("calendar")->disconnectSource(m_eventDatasQuery, this);

        m_eventDatasQuery = QString("eventDatas:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

        m_applet->dataEngine("calendar")->connectSource(m_eventDatasQuery, this);

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
    }
}

void DataSource::connectSource(const QString &timezone)
{
    if (!m_timeQuery.isEmpty()) {
        m_applet->dataEngine("time")->disconnectSource(m_timeQuery, this);
    }

    m_timeQuery = timezone;

    m_applet->dataEngine("time")->connectSource(m_timeQuery, this, 1000, Plasma::NoAlignment);

    if (m_eventDatasQuery.isEmpty()) {
        m_eventDatasQuery = QString("eventDatas:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

        m_applet->dataEngine("calendar")->connectSource(m_eventDatasQuery, this);
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

    emit timezoneChanged(m_timezoneAbbreviation);
}

QDateTime DataSource::getCurrentDateTime() const
{
    return m_dateTime;
}

QString DataSource::getTimeString(DataSourceTimeValue type, ValueOptions options) const
{
    return QString();
}

QVariantList DataSource::getEventsList(DataSourceEventsValue type, ValueOptions options) const
{
    return QVariantList();
}

}
