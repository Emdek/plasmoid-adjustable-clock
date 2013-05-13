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

#include "DataSource.h"

#include <QtScript/QScriptEngine>

namespace AdjustableClock
{

enum ThemeType
{
    UnknownType = 0,
    HtmlType = 1,
    QmlType = 2
};

class Clock : public QObject
{
    Q_OBJECT

    public:
        explicit Clock(DataSource *source, bool constant = false);

        void setTheme(const QString &path, ThemeType type);
        Q_INVOKABLE QVariant getColor(const QString &role = QString()) const;
        Q_INVOKABLE QVariant getFile(const QString &path, bool base64 = true) const;
        Q_INVOKABLE QVariant getFont(const QString &role = QString()) const;
        Q_INVOKABLE QVariant getIcon(const QString &path, int size = 64) const;
        Q_INVOKABLE QVariant getImage(const QString &path, bool base64 = true) const;
        Q_INVOKABLE QVariant getOption(const QString &key, const QVariant &defaultValue = QVariant()) const;
        Q_INVOKABLE QVariant getValue(int component, const QVariantMap &options = QVariantMap()) const;
        QString evaluate(const QString &script);
        static QString getComponentName(ClockComponent component);
        static QLatin1String getComponentString(ClockComponent component);

    private:
        DataSource *m_source;
        QScriptEngine m_engine;
        QString m_path;
        QString m_theme;
        ThemeType m_type;
        bool m_constant;

    signals:
        void componentChanged(ClockComponent component);
};

}

#endif
