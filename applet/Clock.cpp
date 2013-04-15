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

#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebElementCollection>

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

namespace AdjustableClock
{

ClockObject::ClockObject(Clock *clock, bool constant, const QString &theme) : QObject(clock),
    m_clock(clock),
    m_theme(theme),
    m_constant(constant)
{
}

QString ClockObject::getOption(const QString &key, const QString &defaultValue) const
{
    return m_clock->getOption(key, defaultValue, m_theme);
}

QString ClockObject::getValue(int component, const QVariantMap &options) const
{
    return m_clock->getValue(static_cast<ClockComponent>(component), options, m_constant);
}

bool ClockObject::isConstant() const
{
    return m_constant;
}

Clock::Clock(Applet *applet, QWebFrame *document) : QObject(applet),
    m_applet(applet),
    m_document(document),
    m_clock(new ClockObject(this, false)),
    m_constantClock(NULL)
{
    m_constantDateTime = QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15));

    m_eventsQuery = QString("events:%1:%2").arg(QDate::currentDate().toString(Qt::ISODate)).arg(QDate::currentDate().addDays(1).toString(Qt::ISODate));

    m_applet->dataEngine("calendar")->connectSource(m_eventsQuery, this);

    setupEngine(&m_engine, m_clock);
    updateTimeZone();
    updateTheme();

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Clock::setupEngine(QScriptEngine *engine, ClockObject *clock)
{
    engine->globalObject().setProperty("Clock", engine->newQObject(clock), QScriptValue::Undeletable);

    for (int i = 1; i < LastComponent; ++i) {
        engine->evaluate(QString("Clock.%1 = %2;").arg(getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }
}

void Clock::setupClock(QWebFrame *document, ClockObject *clock, const QString &html)
{
    document->setHtml(html);
    document->addToJavaScriptWindowObject("Clock", clock, (clock->isConstant() ? QScriptEngine::ScriptOwnership : QScriptEngine::QtOwnership));

    for (int i = 1; i < LastComponent; ++i) {
        document->evaluateJavaScript(QString("Clock.%1 = %2;").arg(getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    QFile file(":/helper.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    document->evaluateJavaScript(stream.readAll());

    sendEvent(document, "Options");

    for (int i = 1; i < LastComponent; ++i) {
        updateComponent(document, clock, static_cast<ClockComponent>(i));
    }

    setupTheme(document);
}

void Clock::setupTheme(QWebFrame *document)
{
    document->page()->settings()->setUserStyleSheetUrl(QUrl(QString("data:text/css;charset=utf-8;base64,").append(QString("html, body {margin: 0; padding: 0; height: 100%; width: 100%; vertical-align: middle;} html {display: table;} body {display: table-cell; text-align: center; color: %1;} [component] {border-radius: 0.3em; -webkit-transition: background 0.2s, border 0.2s;} [component]:hover {background: rgba(252, 255, 225, 0.8); box-shadow: 0 0 0 2px #F5C800;}").arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name()).toAscii().toBase64())));
    document->page()->settings()->setFontFamily(QWebSettings::StandardFont, Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family());

    sendEvent(document, "Theme");
}

void Clock::sendEvent(QWebFrame *document, const QString &event)
{
    document->evaluateJavaScript(QString("var event = document.createEvent('Event'); event.initEvent('Clock%1Changed', false, false); document.dispatchEvent(event);").arg(event));
}

void Clock::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool reload)
{
    if (source == m_eventsQuery) {
        m_events.clear();

        if (data.isEmpty()) {
            updateComponent(EventsComponent);

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

        updateComponent(EventsComponent);

        return;
    }

    m_dateTime = QDateTime(data["Date"].toDate(), data["Time"].toTime());

    emit tick();

    updateComponent(SecondComponent);
    updateComponent(TimestampComponent);
    updateComponent(TimeComponent);
    updateComponent(DateTimeComponent);

    if (m_dateTime.time().second() == 0 || reload) {
        if (m_dateTime.time().minute() == 0 || reload) {
            const int hour = m_dateTime.time().hour();

            if (hour == 0 || reload) {
                if (m_applet->calendar()->day(m_dateTime.date()) == 1) {
                    if (m_applet->calendar()->dayOfYear(m_dateTime.date()) == 1) {
                        updateComponent(YearComponent);
                    }

                    updateComponent(MonthComponent);
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

                updateComponent(DayOfWeekComponent);
                updateComponent(DayOfMonthComponent);
                updateComponent(DayOfYearComponent);
                updateComponent(DateComponent);
                updateComponent(SunriseComponent);
                updateComponent(SunsetComponent);
                updateComponent(HolidaysComponent);
            }

            if (hour == 0 || hour == 12) {
                updateComponent(TimeOfDayComponent);
            }

            updateComponent(HourComponent);
        }

        updateComponent(MinuteComponent);
    }
}

void Clock::updateTimeZone()
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

void Clock::updateComponent(QWebFrame *document, ClockObject *clock, ClockComponent component)
{
    const QLatin1String componentString = getComponentString(component);
    const QWebElementCollection elements = document->findAllElements(QString("[component=%1]").arg(componentString));

    for (int j = 0; j < elements.count(); ++j) {
        const QVariantMap options = (elements.at(j).hasAttribute("options") ? QScriptEngine().evaluate(QString("JSON.parse('{%1}')").arg(elements.at(j).attribute("options").replace('\'', '"'))).toVariant().toMap() : QVariantMap());
        const QString value = clock->getValue(component, options);

        if (elements.at(j).hasAttribute("attribute")) {
            elements.at(j).setAttribute(elements.at(j).attribute("attribute"), value);
        } else {
            elements.at(j).setInnerXml(value);
        }
    }

    sendEvent(document, componentString);
}

void Clock::updateComponent(ClockComponent component)
{
    updateComponent(m_document, m_clock, component);
}

void Clock::updateTheme()
{
    setupTheme(m_document);
}

void Clock::setTheme(const QString &html)
{
    setupClock(m_document, m_clock, html);
}

ClockObject* Clock::createClock(const QString &theme)
{
    return new ClockObject(this, true, theme);
}

QString Clock::formatNumber(int number, int length)
{
    return QString("%1").arg(number, length, 10, QChar('0'));
}

QString Clock::getOption(const QString &key, const QString &defaultValue, const QString &theme) const
{
    return m_applet->config().group("theme-" + (theme.isEmpty() ? m_applet->config().readEntry("theme", "digital") : theme)).readEntry(key, defaultValue);
}

QString Clock::getValue(ClockComponent component, const QVariantMap &options, bool constant) const
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

QString Clock::evaluate(const QString &script, bool constant)
{
    if (constant) {
        QScriptEngine engine;

        if (!m_constantClock) {
            m_constantClock = new ClockObject(this, true);
        }

        setupEngine(&engine, m_constantClock);

        return engine.evaluate(script).toString();
    }

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
