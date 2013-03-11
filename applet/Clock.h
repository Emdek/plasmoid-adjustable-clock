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

#ifndef ADJUSTABLECLOCKCLOCK_HEADER
#define ADJUSTABLECLOCKCLOCK_HEADER

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>

#include <Plasma/DataEngine>

#include "DataSource.h"

namespace AdjustableClock
{

enum IntervalAlignment
{
    NoAlignment = 0,
    SecondAlignment = 1,
    MinuteAlignment = 2,
    HourAlignment = 3,
    DayAlignment = 4,
    WeekAlignment = 5,
    MonthAlignment = 6,
    YearAlignment = 7
};

enum ClockFeature
{
    NoFeatures = 0,
    SecondsClockFeature = 1,
    SecondsToolTipFeature = 2,
    HolidaysFeature = 4,
    EventsFeature = 8
};

Q_DECLARE_FLAGS(ClockFeatures, ClockFeature)

struct PlaceholderRule
{
    QString rule;
    QString attribute;
    QString expression;
    IntervalAlignment alignment;
};

class Applet;

class Clock : public QObject
{
    Q_OBJECT

    public:
        Clock(Applet *parent);

        void connectSource(const QString &timezone);
        void setDocument(QWebFrame *document);
        Q_INVOKABLE void setRule(const QString &rule, const QString &attribute, const QString &expression, IntervalAlignment alignment);
        Q_INVOKABLE void setRule(const QString &rule, const QString &expression, IntervalAlignment alignment);
        Q_INVOKABLE void setValue(const QString &rule, const QString &attribute, const QString &value);
        Q_INVOKABLE void setValue(const QString &rule, const QString &value);
        Q_INVOKABLE QString getTimeString(ClockTimeValue type, ValueOptions options) const;
        QDateTime getCurrentDateTime(bool refresh = true) const;
        QString evaluateFormat(const QString &format, QDateTime dateTime = QDateTime(), bool special = false);
        QString evaluatePlaceholder(ushort placeholder, QDateTime dateTime, int alternativeForm, bool shortForm, bool textualForm);
        QString evaluatePlaceholder(ushort placeholder, int alternativeForm, bool shortForm, bool textualForm);
        Q_INVOKABLE QVariantList getEventsList(ClockEventsValue type, ValueOptions options) const;

    public slots:
        void reset();

    protected:
        void updateEvents();
        void updateHolidays();
        void applyRule(const PlaceholderRule &rule);
        void setValue(const QWebElementCollection &elements, const QString &attribute, const QString &value);
        void setValue(const QWebElementCollection &elements, const QString &value);
        static QString extractNumber(const QString &format, int &i);
        static QString formatNumber(int number, int length);

    protected slots:
        void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data, bool force = false);

    private:
        Applet *m_applet;
        QWebFrame *m_document;
        QScriptEngine m_engine;
        QStringList m_holidays;
        QStringList m_timezoneArea;
        QString m_timezoneAbbreviation;
        QString m_timezoneOffset;
        QString m_eventsShort;
        QString m_eventsLong;
        QString m_eventsQuery;
        QDateTime m_dateTime;
        QTime m_sunrise;
        QTime m_sunset;
        QHash<IntervalAlignment, QList<PlaceholderRule> > m_rules;
        QFlags<ClockFeature> m_features;

    signals:
        void themeChanged();
        void timezoneChanged(QString timezone);
        void secondChanged(int second);
        void minuteChanged(int minute);
        void hourChanged(int hour);
        void dayChanged(int day);
        void weekChanged(int week);
        void monthChanged(int month);
        void yearChanged(int year);
};

}

#endif
