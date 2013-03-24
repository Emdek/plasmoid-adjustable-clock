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

#include <QtCore/QFile>
#include <QtWebKit/QWebPage>

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

namespace AdjustableClock
{

Clock::Clock(DataSource *parent, ClockMode mode) : QObject(parent),
    m_source(parent),
    m_document(NULL),
    m_mode(mode)
{
    if (m_mode == StandardClock) {
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
        connect(m_source, SIGNAL(dataChanged(QList<ClockComponent>)), this, SLOT(updateClock(QList<ClockComponent>)));
    }

    QFile file(":/clock.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    m_engine.globalObject().setProperty("Clock", m_engine.newQObject(this), QScriptValue::Undeletable);
    m_engine.evaluate(QString(file.readAll()));
}

void Clock::exposeClock()
{
    if (m_document) {
        QFile file(":/clock.js");
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        m_document->addToJavaScriptWindowObject("Clock", this);
        m_document->evaluateJavaScript(QString(file.readAll()));
    }
}

void Clock::updateClock(const QList<ClockComponent> &changes)
{
    for (int i = 0; i < changes.count(); ++i) {
        if (m_document) {
            m_document->evaluateJavaScript(QString("var event = document.createEvent('Event'); event.initEvent('Clock%1Changed', false, false); document.dispatchEvent(event);").arg(getComponentString(changes.at(i))));
        }

        if (m_rules.contains(changes.at(i))) {
            for (int j = 0; j < m_rules[changes.at(i)].count(); ++j) {
                applyRule(m_rules[changes.at(i)].at(j));
            }
        }
    }
}

void Clock::updateTheme()
{
    if (m_document) {
        m_document->page()->settings()->setUserStyleSheetUrl(QUrl(QString("data:text/css;charset=utf-8;base64,").append(QString("html, body {margin: 0; padding: 0; height: 100%; width: 100%; vertical-align: middle;} html {display: table;} body {display: table-cell; color: %1;} placeholder {border-radius: 0.3em; -webkit-transition: background 0.2s, border 0.2s;} placeholder:hover {background: rgba(252, 255, 225, 0.8); box-shadow: 0 0 0 2px #F5C800;} placeholder fix {font-size: 0;}").arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name()).toAscii().toBase64())));
        m_document->page()->settings()->setFontFamily(QWebSettings::StandardFont, Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family());
        m_document->setHtml(m_document->toHtml());
        m_document->evaluateJavaScript("var event = document.createEvent('Event'); event.initEvent('ClockThemeChanged', false, false); document.dispatchEvent(event);");
    }
}

void Clock::applyRule(const Rule &rule)
{
    if (m_document) {
        setValue(m_document->findAllElements(rule.query), rule.attribute, toString(rule.component, rule.options));
    }
}

void Clock::setDocument(QWebFrame *document)
{
    m_rules.clear();

    m_document = document;

    exposeClock();
    updateTheme();

    connect(m_document, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(exposeClock()));
}

void Clock::setRule(const QString &query, const QString &attribute, int component, const QVariantMap &options)
{
    const ClockComponent nativeComponent  = static_cast<ClockComponent>(component);

    if (m_mode == EditorClock && attribute.isEmpty() && m_document) {
        QString title;

        switch (nativeComponent) {
        case SecondComponent:
            title = i18n("Second");

            break;
        case MinuteComponent:
            title = i18n("Minute");

            break;
        case HourComponent:
            title = i18n("Hour");

            break;
        case TimeOfDayComponent:
            title = i18n("The pm or am string");

            break;
        case DayOfWeekComponent:
            title = i18n("Weekday");

            break;
        case DayOfMonthComponent:
            title = i18n("Day of the month");

            break;
        case DayOfYearComponent:
            title = i18n("Day of the year");

            break;
        case WeekComponent:
            title = i18n("Week");

            break;
        case MonthComponent:
            title = i18n("Month");

            break;
        case YearComponent:
            title = i18n("Year");

            break;
        case TimestampComponent:
            title = i18n("UNIX timestamp");

            break;
        case TimeComponent:
            title = i18n("Time");

            break;
        case DateComponent:
            title = i18n("Date");

            break;
        case DateTimeComponent:
            title = i18n("Date and time");

            break;
        case TimeZoneNameComponent:
            title = i18n("Timezone name");

            break;
        case TimeZoneAbbreviationComponent:
            title = i18n("Timezone abbreviation");

            break;
        case TimeZoneOffsetComponent:
            title = i18n("Timezone offset");

            break;
        case TimeZonesComponent:
            title = i18n("Timezones list");

            break;
        case HolidaysComponent:
            title = i18n("Holidays list");

            break;
        case EventsComponent:
            title = i18n("Events list");

            break;
        case SunriseComponent:
            title = i18n("Sunrise time");

            break;
        case SunsetComponent:
            title = i18n("Sunset time");

            break;
        default:
            title = QString();

            break;
        }

        const QWebElementCollection elements = m_document->findAllElements(query);

        for (int i = 0; i < elements.count(); ++i) {
            elements.at(i).setInnerXml(QString("<placeholder title=\"%1\"><fix> </fix>%2<fix> </fix></placeholder>").arg(title).arg(m_source->toString(nativeComponent, options, QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15)))));
        }

        return;
    }

    Rule rule;
    rule.query = query;
    rule.attribute = attribute;
    rule.component = nativeComponent;
    rule.options = options;

    if (m_mode == StandardClock) {
        if (!m_rules.contains(nativeComponent)) {
            m_rules[nativeComponent] = QList<Rule>();
        }

        m_rules[nativeComponent].append(rule);
    }

    applyRule(rule);
}

void Clock::setRule(const QString &query, int component, const QVariantMap &options)
{
    setRule(query, QString(), component, options);
}

void Clock::setValue(const QString &query, const QString &attribute, const QString &value)
{
    if (!m_document) {
        return;
    }

    const QWebElementCollection elements = m_document->findAllElements(query);

    for (int i = 0; i < elements.count(); ++i) {
        if (attribute.isEmpty()) {
            elements.at(i).setInnerXml(value);
        } else {
            elements.at(i).setAttribute(attribute, value);
        }
    }
}

void Clock::setValue(const QString &query, const QString &value)
{
    setValue(query, QString(), value);
}

void Clock::setValue(const QWebElementCollection &elements, const QString &attribute, const QString &value)
{
    for (int i = 0; i < elements.count(); ++i) {
        if (attribute.isEmpty()) {
            elements.at(i).setInnerXml(value);
        } else {
            elements.at(i).setAttribute(attribute, value);
        }
    }
}

QString Clock::evaluate(const QString &script)
{
    return m_engine.evaluate(script).toString();
}

QString Clock::toString(int component, const QVariantMap &options) const
{
    return m_source->toString(static_cast<ClockComponent>(component), options, ((m_mode == StandardClock) ? QDateTime() : QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15))));
}

QString Clock::getComponentString(ClockComponent component)
{
    switch (component) {
    case SecondComponent:
        return "Second";
    case MinuteComponent:
        return "Minute";
    case HourComponent:
        return "Hour";
    case TimeOfDayComponent:
        return "TimeOfDay";
    case DayOfMonthComponent:
        return "DayOfMonth";
    case DayOfWeekComponent:
        return "DayOfWeek";
    case DayOfYearComponent:
        return "DayOfYear";
    case WeekComponent:
        return "Week";
    case MonthComponent:
        return "Month";
    case YearComponent:
        return "Year";
    case TimestampComponent:
        return "Timestamp";
    case TimeComponent:
        return "Time";
    case DateComponent:
        return "Date";
    case DateTimeComponent:
        return "DateTime";
    case TimeZoneNameComponent:
        return "TimeZoneName";
    case TimeZoneAbbreviationComponent:
        return "TimeZoneAbbreviation";
    case TimeZoneOffsetComponent:
        return "TimeZoneOffset";
    case TimeZonesComponent:
        return "TimeZones";
    case EventsComponent:
        return "Events";
    case HolidaysComponent:
        return "Holidays";
    case SunriseComponent:
        return "Sunrise";
    case SunsetComponent:
        return "Sunset";
    default:
        return QString();
    }

    return QString();
}

}
