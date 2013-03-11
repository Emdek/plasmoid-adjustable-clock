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

#include <KDateTime>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

namespace AdjustableClock
{

Clock::Clock(DataSource *parent, bool dynamic) : QObject(parent),
    m_source(parent),
    m_document(NULL),
    m_dynamic(dynamic)
{
    if (dynamic) {
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SIGNAL(themeChanged()));
        connect(m_source, SIGNAL(timezoneChanged(QString)), this, SIGNAL(timezoneChanged(QString)));
        connect(m_source, SIGNAL(eventsChanged()), this, SIGNAL(eventsChanged()));
        connect(m_source, SIGNAL(secondChanged(int)), this, SIGNAL(secondChanged(int)));
        connect(m_source, SIGNAL(minuteChanged(int)), this, SIGNAL(minuteChanged(int)));
        connect(m_source, SIGNAL(hourChanged(int)), this, SIGNAL(hourChanged(int)));
        connect(m_source, SIGNAL(dayChanged(int)), this, SIGNAL(dayChanged(int)));
        connect(m_source, SIGNAL(weekChanged(int)), this, SIGNAL(weekChanged(int)));
        connect(m_source, SIGNAL(monthChanged(int)), this, SIGNAL(monthChanged(int)));
        connect(m_source, SIGNAL(yearChanged(int)), this, SIGNAL(yearChanged(int)));
    }
}


void Clock::applyRule(const PlaceholderRule &rule)
{
    if (m_document) {
        setValue(m_document->findAllElements(rule.rule), rule.attribute, m_engine.evaluate(rule.expression).toString());
    }
}

void Clock::setDocument(QWebFrame *document)
{
    m_document = document;
}

void Clock::setRule(const QString &rule, const QString &attribute, const QString &expression, IntervalAlignment alignment)
{
}

void Clock::setRule(const QString &rule, const QString &expression, IntervalAlignment alignment)
{
    setRule(rule, QString(), expression, alignment);
}

void Clock::setValue(const QString &rule, const QString &attribute, const QString &value)
{
    const QWebElementCollection elements = m_document->findAllElements(rule);

    for (int i = 0; i < elements.count(); ++i) {
        if (attribute.isEmpty()) {
            elements.at(i).setInnerXml(value);
        } else {
            elements.at(i).setAttribute(attribute, value);
        }
    }
}

void Clock::setValue(const QString &rule, const QString &value)
{
    setValue(rule, QString(), value);
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

void Clock::setValue(const QWebElementCollection &elements, const QString &value)
{
    setValue(elements, QString(), value);
}

QString Clock::evaluate(const QString &script) const
{
    return QString();
}

QString Clock::getTimeString(ClockTimeValue type, ValueOptions options) const
{
    return m_source->getTimeString(type, options, (m_dynamic ? QDateTime() : QDateTime(QDate(2000, 1, 1), QTime(12, 30, 15))));
}

QVariantList Clock::getEventsList(ClockEventsType type, ValueOptions options) const
{
    return m_source->getEventsList(type, options);
}

}
