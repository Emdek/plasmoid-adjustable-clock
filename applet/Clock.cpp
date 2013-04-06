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
#include "DataSource.h"

#include <QtWebKit/QWebPage>

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

namespace AdjustableClock
{

Clock::Clock(DataSource *source, QWebFrame *document, bool constant) : QObject(source),
    m_source(source),
    m_document(document),
    m_constant(constant)
{
    if (!m_constant) {
        connect(m_source, SIGNAL(componentChanged(ClockComponent)), this, SLOT(updateComponent(ClockComponent)));
    }

    m_engine.globalObject().setProperty("Clock", m_engine.newQObject(this), QScriptValue::Undeletable);

    for (int i = 0; i < LastComponent; ++i) {
        m_engine.evaluate(QString("Clock.%1 = %2;").arg(getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    updateTheme();

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Clock::updateComponent(ClockComponent component)
{
    const QLatin1String componentString = getComponentString(component);
    const QWebElementCollection elements = m_document->findAllElements(QString("[component=%1]").arg(componentString));

    for (int j = 0; j < elements.count(); ++j) {
        const QVariantMap options = (elements.at(j).hasAttribute("options") ? m_engine.evaluate(QString("JSON.parse('{%1}')").arg(elements.at(j).attribute("options").replace('\'', '"'))).toVariant().toMap() : QVariantMap());
        const QString value = m_source->toString(component, options, m_constant);

        if (elements.at(j).hasAttribute("attribute")) {
            elements.at(j).setAttribute(elements.at(j).attribute("attribute"), value);
        } else {
            elements.at(j).setInnerXml(value);
        }
    }

    m_document->evaluateJavaScript(QString("var event = document.createEvent('Event'); event.initEvent('Clock%1Changed', false, false); document.dispatchEvent(event);").arg(componentString));
}

void Clock::updateTheme()
{
    m_document->page()->settings()->setUserStyleSheetUrl(QUrl(QString("data:text/css;charset=utf-8;base64,").append(QString("html, body {margin: 0; padding: 0; height: 100%; width: 100%; vertical-align: middle;} html {display: table;} body {display: table-cell; color: %1;} [component] {border-radius: 0.3em; -webkit-transition: background 0.2s, border 0.2s;} [component]:hover {background: rgba(252, 255, 225, 0.8); box-shadow: 0 0 0 2px #F5C800;}").arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name()).toAscii().toBase64())));
    m_document->page()->settings()->setFontFamily(QWebSettings::StandardFont, Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family());
    m_document->evaluateJavaScript("var event = document.createEvent('Event'); event.initEvent('ClockThemeChanged', false, false); document.dispatchEvent(event);");
}

void Clock::setTheme(const QString &html, const QString &script)
{
    m_document->setHtml(html);
    m_document->addToJavaScriptWindowObject("Clock", this);

    for (int i = 0; i < LastComponent; ++i) {
        m_document->evaluateJavaScript(QString("Clock.%1 = %2;").arg(getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    m_document->evaluateJavaScript(script);

    for (int i = 0; i < LastComponent; ++i) {
        updateComponent(static_cast<ClockComponent>(i));
    }
}

QString Clock::evaluate(const QString &script)
{
    return m_engine.evaluate(script).toString();
}

QString Clock::toString(int component, const QVariantMap &options) const
{
    return m_source->toString(static_cast<ClockComponent>(component), options, m_constant);
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
