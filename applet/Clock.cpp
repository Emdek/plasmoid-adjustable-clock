/***********************************************************************************
* Adjustable Clock: Plasmoid to show date and time in adjustable format.
* Copyright (C) 2008 - 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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


namespace AdjustableClock
{

Clock::Clock(QObject *parent) : QObject(parent),
    m_document(NULL)
{
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

QString Clock::getTimeString(ClockTimeValue type, ValueOptions options) const
{
    return QString();
}

QVariantList Clock::getEventsList(ClockEventsValue type, ValueOptions options) const
{
    return QVariantList();
}

}
