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

#include <Plasma/Theme>

namespace AdjustableClock
{

Clock::Clock(DataSource *source, bool constant) : QObject(source),
    m_source(source),
    m_constant(constant)
{
    m_engine.globalObject().setProperty("Clock", m_engine.newQObject(this), QScriptValue::Undeletable);

    for (int i = 1; i < LastComponent; ++i) {
        m_engine.evaluate(QString("Clock.%1 = %2;").arg(getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    if (!constant) {
        connect(m_source, SIGNAL(componentChanged(ClockComponent)), this, SIGNAL(componentChanged(ClockComponent)));
    }
}

void Clock::setTheme(const QString &theme)
{
    m_theme = theme;
}

QVariant Clock::getOption(const QString &key, const QVariant &defaultValue) const
{
    const QVariant value = m_source->getOption(key, ((key == "themeFont") ? QVariant() : defaultValue), m_theme);

    if (key == "themeFont" && value.toString().isEmpty()) {
        return Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family();
    } else if (key.contains("color", Qt::CaseInsensitive) && QRegExp("\\d+,\\d+,\\d+.*").exactMatch(value.toString())) {
        const QString color = value.toString();

        if (color.count(QChar(',')) == 3) {
            const QStringList values = color.split(QChar(','));

            return QString("rgba(%1,%2,%3,%4)").arg(values.at(0)).arg(values.at(1)).arg(values.at(2)).arg(values.at(3).toDouble() / 255);
        }

        return QString("rgb(%1)").arg(color);
    }

    return value;
}

QVariant Clock::getValue(int component, const QVariantMap &options) const
{
    return m_source->getValue(static_cast<ClockComponent>(component), options, m_constant);
}

QString Clock::evaluate(const QString &script)
{
    return m_engine.evaluate(script).toString();
}

QString Clock::getComponentName(ClockComponent component)
{
    switch (component) {
    case SecondComponent:
        return i18n("Second");
    case MinuteComponent:
        return i18n("Minute");
    case HourComponent:
        return i18n("Hour");
    case TimeOfDayComponent:
        return i18n("The pm or am string");
    case DayOfMonthComponent:
        return i18n("Day of the month");
    case DayOfWeekComponent:
        return i18n("Weekday");
    case DayOfYearComponent:
        return i18n("Day of the year");
    case WeekComponent:
        return i18n("Week");
    case MonthComponent:
        return i18n("Month");
    case YearComponent:
        return i18n("Year");
    case TimestampComponent:
        return i18n("UNIX timestamp");
    case TimeComponent:
        return i18n("Time");
    case DateComponent:
        return i18n("Date");
    case DateTimeComponent:
        return i18n("Date and time");
    case TimeZoneNameComponent:
        return i18n("Timezone name");
    case TimeZoneAbbreviationComponent:
        return i18n("Timezone abbreviation");
    case TimeZoneOffsetComponent:
        return i18n("Timezone offset");
    case TimeZonesComponent:
        return i18n("Timezones list");
    case EventsComponent:
        return i18n("Events list");
    case HolidaysComponent:
        return i18n("Holidays list");
    case SunriseComponent:
        return i18n("Sunrise time");
    case SunsetComponent:
        return i18n("Sunset time");
    default:
        return QString();
    }

    return QString();
}

QLatin1String Clock::getComponentString(ClockComponent component)
{
    switch (component) {
    case SecondComponent:
        return QLatin1String("Second");
    case MinuteComponent:
        return QLatin1String("Minute");
    case HourComponent:
        return QLatin1String("Hour");
    case TimeOfDayComponent:
        return QLatin1String("TimeOfDay");
    case DayOfMonthComponent:
        return QLatin1String("DayOfMonth");
    case DayOfWeekComponent:
        return QLatin1String("DayOfWeek");
    case DayOfYearComponent:
        return QLatin1String("DayOfYear");
    case WeekComponent:
        return QLatin1String("Week");
    case MonthComponent:
        return QLatin1String("Month");
    case YearComponent:
        return QLatin1String("Year");
    case TimestampComponent:
        return QLatin1String("Timestamp");
    case TimeComponent:
        return QLatin1String("Time");
    case DateComponent:
        return QLatin1String("Date");
    case DateTimeComponent:
        return QLatin1String("DateTime");
    case TimeZoneNameComponent:
        return QLatin1String("TimeZoneName");
    case TimeZoneAbbreviationComponent:
        return QLatin1String("TimeZoneAbbreviation");
    case TimeZoneOffsetComponent:
        return QLatin1String("TimeZoneOffset");
    case TimeZonesComponent:
        return QLatin1String("TimeZones");
    case EventsComponent:
        return QLatin1String("Events");
    case HolidaysComponent:
        return QLatin1String("Holidays");
    case SunriseComponent:
        return QLatin1String("Sunrise");
    case SunsetComponent:
        return QLatin1String("Sunset");
    default:
        return QLatin1String("");
    }

    return QLatin1String("");
}

}
