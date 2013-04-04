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

#ifndef ADJUSTABLECLOCKDATASOURCE_HEADER
#define ADJUSTABLECLOCKDATASOURCE_HEADER

#include "Clock.h"

#include <QtCore/QObject>
#include <QtScript/QScriptEngine>
#include <QtWebKit/QWebElementCollection>
#include <QtWebKit/QWebFrame>

#include <Plasma/DataEngine>

namespace AdjustableClock
{

struct Event
{
    QString type;
    QString time;
    QString summary;
};

class Applet;

class DataSource : public QObject
{
    Q_OBJECT

    public:
        DataSource(Applet *parent);

        void updateTimeZone();
        QDateTime getCurrentDateTime() const;
        QString toString(ClockComponent component, const QVariantMap &options = QVariantMap(), QDateTime dateTime = QDateTime()) const;

    protected:
        static QString formatNumber(int number, int length);

    protected slots:
        void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);

    private:
        Applet *m_applet;
        QDateTime m_dateTime;
        QTime m_sunrise;
        QTime m_sunset;
        QString m_timeZoneAbbreviation;
        QString m_timeZoneOffset;
        QString m_timeQuery;
        QString m_eventsQuery;
        QString m_timeZoneArea;
        QStringList m_holidays;
        QList<Event> m_events;
        QMap<QString, QString> m_timeZones;

    signals:
        void dataChanged(QList<ClockComponent> changes);
};

}

#endif
