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

#include <QtCore/QDir>
#include <QtCore/QBuffer>

#include <KIcon>
#include <KMimeType>
#include <KFilterDev>

#include <Plasma/Theme>

namespace AdjustableClock
{

Clock::Clock(DataSource *source, bool constant) : QObject(source),
    m_source(source),
    m_type(UnknownType),
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

void Clock::setTheme(const QString &path, ThemeType type)
{
    m_path = path;
    m_theme = QFileInfo(path).fileName();
    m_type = type;
}

QVariant Clock::getColor(const QString &role) const
{
    Plasma::Theme::ColorRole nativeRole = Plasma::Theme::TextColor;

    if (role == "highlight") {
        nativeRole = Plasma::Theme::HighlightColor;
    } else if (role == "background") {
        nativeRole = Plasma::Theme::BackgroundColor;
    } else if (role == "buttonText") {
        nativeRole = Plasma::Theme::ButtonTextColor;
    } else if (role == "buttonBackground") {
        nativeRole = Plasma::Theme::ButtonBackgroundColor;
    } else if (role == "link") {
        nativeRole = Plasma::Theme::LinkColor;
    } else if (role == "visitedLink") {
        nativeRole = Plasma::Theme::VisitedLinkColor;
    } else if (role == "buttonHover") {
        nativeRole = Plasma::Theme::ButtonHoverColor;
    } else if (role == "buttonFocus") {
        nativeRole = Plasma::Theme::ButtonFocusColor;
    } else if (role == "viewText") {
        nativeRole = Plasma::Theme::ViewTextColor;
    } else if (role == "viewBackground") {
        nativeRole = Plasma::Theme::ViewBackgroundColor;
    } else if (role == "viewHover") {
        nativeRole = Plasma::Theme::ViewHoverColor;
    } else if (role == "viewFocus") {
        nativeRole = Plasma::Theme::ViewFocusColor;
    }

    if (m_type == HtmlType) {
        const QColor color = Plasma::Theme::defaultTheme()->color(nativeRole);

        return QString("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alphaF());
    }

    return Plasma::Theme::defaultTheme()->color(nativeRole);
}

QVariant Clock::getFile(const QString &path, bool base64) const
{
    QString filePath = path;

    if (QFileInfo(filePath).isRelative()) {
        filePath = QFileInfo(QDir(m_path).absoluteFilePath(filePath)).absolutePath();
    }

    QScopedPointer<QIODevice> file(KFilterDev::deviceForFile(filePath, "application/x-gzip"));
    file->open(QIODevice::ReadOnly);

    if (base64) {
        return QString("data:%1;base64,%2").arg(KMimeType::findByPath(filePath)->name()).arg(QString(file->readAll().toBase64()));
    }

    QTextStream stream(file.data());
    stream.setCodec("UTF-8");

    return stream.readAll();
}

QVariant Clock::getFont(const QString &role) const
{
    Plasma::Theme::FontRole nativeRole = Plasma::Theme::DefaultFont;

    if (role == "desktop") {
        nativeRole = Plasma::Theme::DesktopFont;
    } else if (role == "smallest") {
        nativeRole = Plasma::Theme::SmallestFont;
    }

    if (m_type == HtmlType) {
        return Plasma::Theme::defaultTheme()->font(nativeRole).family();
    }

    return Plasma::Theme::defaultTheme()->font(nativeRole);
}

QVariant Clock::getIcon(const QString &path, int size) const
{
    const KIcon icon(path);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);

    icon.pixmap(size, size).save(&buffer, "PNG");

    return QString("data:image/png;base64," + byteArray.toBase64());
}

QVariant Clock::getImage(const QString &path, bool base64) const
{
    const QString imagePath = Plasma::Theme::defaultTheme()->imagePath(path);

    if (imagePath.isEmpty()) {
        return QString();
    }

    QScopedPointer<QIODevice> file(KFilterDev::deviceForFile(imagePath, "application/x-gzip"));
    file->open(QIODevice::ReadOnly);

    if (base64) {
        return QString("data:image/svg+xml;base64," + QString(file->readAll().toBase64()));
    }

    QTextStream stream(file.data());
    stream.setCodec("UTF-8");

    return stream.readAll();
}

QVariant Clock::getOption(const QString &key, const QVariant &defaultValue) const
{
    const QVariant value = m_source->getOption(key, defaultValue, m_theme);

    if (m_type == HtmlType && key.contains("color", Qt::CaseInsensitive) && QRegExp("\\d+,\\d+,\\d+.*").exactMatch(value.toString())) {
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
