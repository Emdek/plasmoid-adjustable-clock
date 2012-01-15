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

#include "Applet.h"
#include "Configuration.h"

#include <QtCore/QRegExp>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#include <KMenu>
#include <KLocale>
#include <KDateTime>
#include <KConfigDialog>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/Containment>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet *m_applet = NULL;
QScriptEngine m_engine;
QStringList m_holidays;
QStringList m_timezoneArea;
QString m_timezoneAbbreviation;
QString m_timezoneOffset;
QString m_eventsShort;
QString m_eventsLong;
QDateTime m_dateTime;
QTime m_sunrise;
QTime m_sunset;

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clipboardAction(NULL)
{
    KGlobal::locale()->insertCatalog(QLatin1String("libplasmaclock"));
    KGlobal::locale()->insertCatalog(QLatin1String("timezones4"));
    KGlobal::locale()->insertCatalog(QLatin1String("adjustableclock"));

    m_applet = this;

    setHasConfigurationInterface(true);
    resize(150, 80);
}

void Applet::init()
{
    ClockApplet::init();

    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    updateTheme();
    connectSource(currentTimezone());
    constraintsEvent(Plasma::SizeConstraint);
    configChanged();

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Applet::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool force)
{
    Q_UNUSED(source)

    m_dateTime = QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());

    const int second = m_dateTime.time().second();

    if (force || (m_features & HolidaysFeature && m_dateTime.time().hour() == 0 && m_dateTime.time().minute() == 0 && (second == 0 || !(m_features & SecondsClockFeature || m_features & SecondsToolTipFeature)))) {
        const QString region = config().readEntry("holidaysRegions", dataEngine(QLatin1String("calendar"))->query(QLatin1String("holidaysDefaultRegion"))[QLatin1String("holidaysDefaultRegion")]).toString().split(QLatin1Char(',')).first();
        const QString key = QLatin1String("holidays:") + region + QLatin1Char(':') + currentDateTime().date().toString(Qt::ISODate);
        Plasma::DataEngine::Data holidaysData = dataEngine(QLatin1String("calendar"))->query(key);

        m_holidays.clear();

        if (!holidaysData.isEmpty() && holidaysData.contains(key)) {
            QVariantList holidaysList = holidaysData[key].toList();
            QStringList holidays;

            for (int i = 0; i < holidaysList.length(); ++i) {
                m_holidays.append(holidaysList[i].toHash()[QLatin1String("Name")].toString());
            }
        }
    }

    if (force || (m_dateTime.time().minute() == 0 && second == 0)) {
        Plasma::DataEngine::Data sunData = dataEngine(QLatin1String("time"))->query(currentTimezone() + QLatin1String("|Solar"));

        if (m_features & EventsFeature) {
            Plasma::DataEngine::Data eventsData = dataEngine(QLatin1String("calendar"))->query(QLatin1String("events:") + QDate::currentDate().toString(Qt::ISODate) + QLatin1Char(':') + QDate::currentDate().addDays(1).toString(Qt::ISODate));

            m_eventsShort = m_eventsLong = QString();

            if (!eventsData.isEmpty()) {
                QHash<QString, QVariant>::iterator i;
                QStringList eventsShort;
                QStringList eventsLong;
                QPair<QDateTime, QDateTime> limits = qMakePair(QDateTime::currentDateTime().addSecs(-43200), QDateTime::currentDateTime().addSecs(43200));

                for (i = eventsData.begin(); i != eventsData.end(); ++i) {
                    QVariantHash event = i.value().toHash();

                    if (event[QLatin1String("Type")] == QLatin1String("Event") || event[QLatin1String("Type")] == QLatin1String("Todo")) {
                        KDateTime startTime = event[QLatin1String("StartDate")].value<KDateTime>();
                        KDateTime endTime = event[QLatin1String("EndDate")].value<KDateTime>();

                        if ((endTime.isValid() && endTime.dateTime() < limits.first && endTime != startTime) || startTime.dateTime() > limits.second) {
                            continue;
                        }

                        QString type = ((event[QLatin1String("Type")] == QLatin1String("Event")) ? i18n("Event") : i18n("To do"));
                        QString time;

                        if (startTime.time().hour() == 0 && startTime.time().minute() == 0 && endTime.time().hour() == 0 && endTime.time().minute() == 0) {
                            time = i18n("All day");
                        } else if (startTime.isValid()) {
                            time = KGlobal::locale()->formatTime(startTime.time(), false);

                            if (endTime.isValid()) {
                                time.append(QLatin1String(" - ") + KGlobal::locale()->formatTime(endTime.time(), false));
                            }
                        }

                        eventsShort.append(QString(QLatin1String("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\">%2</td>")).arg(type).arg(event[QLatin1String("Summary")].toString()));
                        eventsLong.append(QString(QLatin1String("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\">%2 <nobr>(%3)</nobr></td>")).arg(type).arg(event[QLatin1String("Summary")].toString()).arg(time));
                    }
                }

                m_eventsShort = QLatin1String("<table>\n<tr>") + eventsShort.join(QLatin1String("</tr>\n<tr>")) + QLatin1String("</tr>\n</table>");
                m_eventsLong = QLatin1String("<table>\n<tr>") + eventsLong.join(QLatin1String("</tr>\n<tr>")) + QLatin1String("</tr>\n</table>");
            }
        }

        if (m_features & SunriseFeature) {
            m_sunrise = sunData[QLatin1String("Sunrise")].toDateTime().time();
        }

        if (m_features & SunsetFeature) {
            m_sunset = sunData[QLatin1String("Sunset")].toDateTime().time();
        }
    }

    if (force || m_features & SecondsClockFeature || second == 0) {
        setHtml(evaluateFormat(format().html, m_dateTime), format().css);
    }

    if (Plasma::ToolTipManager::self()->isVisible(this) && (force || m_features & SecondsToolTipFeature || second == 0)) {
        updateToolTipContent();
    }

    if (force) {
        updateSize();
    }
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints((m_features & NoBackgroundFeature) ? NoBackground : DefaultBackground);
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    ClockApplet::resizeEvent(event);

    updateSize();
}

void Applet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::MidButton) {
        copyToClipboard();
    }

    const QUrl url = m_page.mainFrame()->hitTestContent(event->pos().toPoint()).linkUrl();

    if (url.isValid() && event->button() == Qt::LeftButton) {
        QDesktopServices::openUrl(url);

        event->ignore();
    } else {
        ClockApplet::mousePressEvent(event);
    }
}

void Applet::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)

    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    m_page.mainFrame()->render(painter);
}

void Applet::createClockConfigurationInterface(KConfigDialog *parent)
{
    new Configuration(this, parent);
}

void Applet::clockConfigChanged()
{
    setHtml(evaluateFormat(format().html, currentDateTime()), format().css);

    updateSize();
}

void Applet::clockConfigAccepted()
{
    emit configNeedsSaving();
}

void Applet::connectSource(const QString &timezone)
{
    QRegExp formatWithSeconds = QRegExp(QLatin1String("%[\\d\\!\\$\\:\\+\\-]*[ast]"));
    QFlags<ClockFeature> features;

    m_format.html = QString();

    const Format format = this->format();
    const QPair<QString, QString> toolTipFormat = this->toolTipFormat();
    const QString toolTip = (toolTipFormat.first + QLatin1Char('|') + toolTipFormat.second);
    const QString string = (format.html + QLatin1Char('|')) + toolTip;

    if (format.html.contains(formatWithSeconds)) {
        features |= SecondsClockFeature;
    }

    if (toolTip.contains(formatWithSeconds)) {
        features |= SecondsToolTipFeature;
    }

    if (string.contains(QRegExp(QLatin1String("%[\\d\\!\\$\\:\\+\\-]*H")))) {
        features |= HolidaysFeature;
    }

    if (string.contains(QRegExp(QLatin1String("%[\\d\\!\\$\\:\\+\\-]*E")))) {
        features |= EventsFeature;
    }

    if (string.contains(QRegExp(QLatin1String("%[\\d\\!\\$\\:\\+\\-]*z")))) {
        features |= TimezoneFeature;
    }

    if (string.contains(QLatin1String("%S"))) {
        features |= SunsetFeature;
    }

    if (string.contains(QLatin1String("%R"))) {
        features |= SunriseFeature;
    }

    if (!format.background) {
        features |= NoBackgroundFeature;
    }

    m_features = features;

    const bool alignToSeconds = (features & SecondsClockFeature || features & SecondsToolTipFeature);

    dataEngine(QLatin1String("time"))->connectSource(timezone, this, (alignToSeconds ? 1000 : 60000), (alignToSeconds ? Plasma::NoAlignment : Plasma::AlignToMinute));

    if (features & TimezoneFeature) {
        const KTimeZone timezoneData = (isLocalTimezone() ? KSystemTimeZones::local() : KSystemTimeZones::zone(currentTimezone()));

        m_timezoneAbbreviation = QString::fromLatin1(timezoneData.abbreviation(QDateTime::currentDateTime().toUTC()));

        if (m_timezoneAbbreviation.isEmpty()) {
            m_timezoneAbbreviation = i18n("UTC");
        }

        m_timezoneArea = i18n(timezoneData.name().toUtf8().data()).replace(QLatin1Char('_'), QLatin1Char(' ')).split(QLatin1Char('/'));

        int seconds = timezoneData.currentOffset(Qt::UTC);
        int minutes = abs(seconds / 60);
        int hours = abs(minutes / 60);

        minutes = (minutes - (hours * 60));

        m_timezoneOffset = QString::number(hours);

        if (minutes) {
            m_timezoneOffset.append(QLatin1Char(':'));
            m_timezoneOffset.append(formatNumber(minutes, 2));
        }

        m_timezoneOffset = (QChar((seconds >= 0) ? QLatin1Char('+') : QLatin1Char('-')) + m_timezoneOffset);
    }

    constraintsEvent(Plasma::SizeConstraint);
    updateSize();
    dataUpdated(QString(), dataEngine(QLatin1String("time"))->query(currentTimezone()), true);
}

void Applet::copyToClipboard()
{
    QApplication::clipboard()->setText(evaluateFormat(config().readEntry("fastCopyFormat", "%Y-%M-%d %h:%m:%s"), currentDateTime()));
}

void Applet::toolTipAboutToShow()
{
    updateToolTipContent();
}

void Applet::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);
}

void Applet::setHtml(const QString &html, const QString &css)
{
    if (html != m_currentHtml) {
        m_page.mainFrame()->setHtml(QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"><html><head><style type=\"text/css\">html, body, body > table, #clock {margin:0; padding:0; height:100%; width:100%; vertical-align:middle;}") + css + QLatin1String("</style></head><body><table><tr><td id=\"clock\">") + html + QLatin1String("</td></tr></table></body></html>"));

        m_currentHtml = html;

        update();
    }
}

void Applet::copyToClipboard(QAction *action)
{
    QApplication::clipboard()->setText(action->text());
}

void Applet::updateClipboardMenu()
{
    const QDateTime dateTime = currentDateTime();
    const QStringList clipboardFormats = this->clipboardFormats();

    qDeleteAll(m_clipboardAction->menu()->actions());

    m_clipboardAction->menu()->clear();

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        if (clipboardFormats.at(i).isEmpty()) {
            m_clipboardAction->menu()->addSeparator();
        } else {
            m_clipboardAction->menu()->addAction(evaluateFormat(clipboardFormats.at(i), dateTime));
        }
    }
}

void Applet::changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone)
{
    dataEngine(QLatin1String("time"))->disconnectSource(oldTimezone, this);

    connectSource(newTimezone);
}

void Applet::updateToolTipContent()
{
    Plasma::ToolTipContent toolTipData;
    QPair<QString, QString> toolTipFormat = this->toolTipFormat();

    if (!toolTipFormat.first.isEmpty() || !toolTipFormat.second.isEmpty()) {
        toolTipData.setImage(KIcon(QLatin1String("chronometer")).pixmap(IconSize(KIconLoader::Desktop)));
        toolTipData.setMainText(evaluateFormat(toolTipFormat.first, m_dateTime));
        toolTipData.setSubText(evaluateFormat(toolTipFormat.second, m_dateTime));
        toolTipData.setAutohide(false);
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTipData);
}

void Applet::updateSize()
{
    const Format format = this->format();

    setHtml(evaluateFormat(format.html), format.css);

    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    QSizeF size;

    if (formFactor() == Plasma::Horizontal) {
        size = QSizeF(containment()->boundingRect().width(), boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSizeF(boundingRect().width(), containment()->boundingRect().height());
    } else {
        size = boundingRect().size();
    }

    const qreal widthFactor = (size.width() / m_page.mainFrame()->contentsSize().width());
    const qreal heightFactor = (size.height() / m_page.mainFrame()->contentsSize().height());

    m_page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);

    if (formFactor() == Plasma::Horizontal) {
        setMinimumWidth(m_page.mainFrame()->contentsSize().width());
        setMinimumHeight(0);
    } else if (formFactor() == Plasma::Vertical) {
        setMinimumHeight(m_page.mainFrame()->contentsSize().height());
        setMinimumWidth(0);
    }

    m_page.setViewportSize(boundingRect().size().toSize());

    setHtml(evaluateFormat(format.html, m_dateTime), format.css);
}

void Applet::updateTheme()
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->evaluateJavaScript(QLatin1String("document.fgColor = '") + Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name() + QLatin1Char('\''));

    update();
}

QDateTime Applet::currentDateTime() const
{
    Plasma::DataEngine::Data data = dataEngine(QLatin1String("time"))->query(currentTimezone());

    return QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());
}

QString Applet::extractExpression(const QString &format, int &i)
{
    if ((format.length() - i) < 2 || !format.mid(i).contains(QLatin1Char('}'))) {
        return QString();
    }

    ++i;

    QString expression;
    int braces = 1;

    while (i < format.length()) {
        if (format.at(i) == QLatin1Char('{')) {
            ++braces;
        } else if (format.at(i) == QLatin1Char('}')) {
            --braces;

            if (braces == 0) {
                ++i;

                break;
            }
        }

        expression.append(format.at(i));

        ++i;
    }

    return expression;
}

QString Applet::extractNumber(const QString &format, int &i)
{
    QString number;

    while ((format.at(i).isDigit() || format.at(i) == QLatin1Char('-')) && i < format.length()) {
        number.append(format.at(i));

        ++i;
    }

    return number;
}

QString Applet::formatNumber(int number, int length)
{
    return QString(QLatin1String("%1")).arg(number, length, 10, QLatin1Char('0'));
}

QString Applet::evaluateFormat(const QString &format, QDateTime dateTime)
{
    if (format.isEmpty()) {
        return QString();
    }

    QString string;

    for (int i = 0; i < format.length(); ++i) {
        if (format.at(i) != QLatin1Char('%')) {
            string.append(format.at(i));

            continue;
        }

        QString substitution;
        QPair<int, int> range = qMakePair(-1, -1);
        int alternativeForm = 0;
        bool shortForm = false;
        bool textualForm = false;

        ++i;

        if (format.at(i).isDigit() || ((format.at(i) == QLatin1Char('-') || format.at(i) == QLatin1Char(':')) && format.at(i + 1).isDigit())) {
            if (format.at(i) == QLatin1Char(':')) {
                range.first = 0;
            } else {
                range.first = extractNumber(format, i).toInt();
            }

            if (format.at(i) == QLatin1Char(':')) {
                range.second = extractNumber(format, ++i).toInt();
            }
        }

        if (format.at(i) == QLatin1Char('!')) {
            ++i;

            shortForm = true;
        }

        if (format.at(i) == QLatin1Char('$')) {
            ++i;

            textualForm = true;
        }

        if (format.at(i) == QLatin1Char('+')) {
            ++i;

            alternativeForm = 1;
        } else if (format.at(i) == QLatin1Char('-')) {
            ++i;

            alternativeForm = -1;
        }

        if (format.at(i) == QLatin1Char('{')) {
            QString expression = extractExpression(format, i);
            QScriptValue scriptExpression = m_engine.evaluate(evaluateFormat(expression, dateTime));

            if ((format.at(i) == QLatin1Char('?') || format.at(i) == QLatin1Char(':')) && format.at(i + 1) == QLatin1Char('{')) {
                QString trueSubstitution;
                QString falseSubstitution;

                if (format.at(i) == QLatin1Char('?')) {
                    trueSubstitution = extractExpression(format, ++i);
                }

                if (format.at(i) == QLatin1Char(':')) {
                    falseSubstitution = extractExpression(format, ++i);
                }

                if (scriptExpression.toBool()) {
                    substitution.append(evaluateFormat(trueSubstitution, dateTime));
                } else {
                    substitution.append(evaluateFormat(falseSubstitution, dateTime));
                }

                --i;
            } else {
                substitution.append(scriptExpression.toString());
            }
        } else {
            if (dateTime.isValid()) {
                substitution = evaluatePlaceholder(format.at(i).unicode(), dateTime, alternativeForm, shortForm, textualForm);
            } else {
                substitution = evaluatePlaceholder(format.at(i).unicode(), alternativeForm, shortForm, textualForm);
            }
        }

        if (range.first != -1 || range.second != -1) {
            if (range.first < 0) {
                range.first = (substitution.length() + range.first);
            }

            if (range.second < -1) {
                range.second = (substitution.length() + range.second);
            }

            string.append(substitution.mid(range.first, range.second));
        } else {
            string.append(substitution);
        }
    }

    return string;
}

QString Applet::evaluatePlaceholder(ushort placeholder, QDateTime dateTime, int alternativeForm, bool shortForm, bool textualForm)
{
    QStringList timezones;

    switch (placeholder) {
    case 's': // Second
        return formatNumber(dateTime.time().second(), (shortForm ? 0 : 2));
    case 'm': // Minute
        return formatNumber(dateTime.time().minute(), (shortForm ? 0 : 2));
    case 'h': // Hour
        alternativeForm = ((alternativeForm == 0) ? KGlobal::locale()->use12Clock() : (alternativeForm == 1));

        return formatNumber((alternativeForm ? (((dateTime.time().hour() + 11) % 12) + 1) : dateTime.time().hour()), (shortForm ? 0 : 2));
    case 'p': // The pm or am string
        return ((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));
    case 'd': // Day of the month
        return formatNumber(dateTime.date().day(), (shortForm ? 0 : 2));
    case 'w': // Weekday
        if (textualForm) {
            return m_applet->calendar()->weekDayName(m_applet->calendar()->dayOfWeek(dateTime.date()), (shortForm ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));
        }

        return formatNumber(m_applet->calendar()->dayOfWeek(dateTime.date()), (shortForm ? 0 : QString::number(m_applet->calendar()->daysInWeek(dateTime.date())).length()));
    case 'D': // Day of the year
        return formatNumber(m_applet->calendar()->dayOfYear(dateTime.date()), (shortForm ? 0 : QString::number(m_applet->calendar()->daysInYear(dateTime.date())).length()));
    case 'W': // Week
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Week, (shortForm ? KLocale::ShortNumber : KLocale::LongNumber));
    case 'M': // Month
        if (textualForm) {
            alternativeForm = ((alternativeForm == 0) ? KGlobal::locale()->dateMonthNamePossessive() : (alternativeForm == 1));

            return m_applet->calendar()->monthName(dateTime.date(), (shortForm ? (alternativeForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (alternativeForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));
        }

        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Month, (shortForm ? KLocale::ShortNumber : KLocale::LongNumber));
    case 'Y': // Year
        return m_applet->calendar()->formatDate(dateTime.date(), KLocale::Year, (shortForm ? KLocale::ShortNumber : KLocale::LongNumber));
    case 'U': // UNIX timestamp
        return QString::number(dateTime.toTime_t());
    case 't': // Time
        return KGlobal::locale()->formatTime(dateTime.time(), !shortForm);
    case 'T': // Date
        return KGlobal::locale()->formatDate(dateTime.date(), (shortForm ? KLocale::ShortDate : KLocale::LongDate));
    case 'A': // Date and time
        return KGlobal::locale()->formatDateTime(dateTime, (shortForm ? KLocale::ShortDate : KLocale::LongDate));
    case 'z': // Timezone
        if (textualForm) {
            if (alternativeForm) {
                return m_timezoneAbbreviation;
            }

            return (shortForm ? (m_timezoneArea.isEmpty() ? QString() : m_timezoneArea.last()) : m_timezoneArea.join(QString(QLatin1Char('/'))));
        }

        return m_timezoneOffset;
    case 'Z':
        timezones = m_applet->config().readEntry("timeZones", QStringList());
        timezones.prepend(QLatin1String(""));

        if (timezones.length() == 1 && shortForm) {
            return QString();
        }

        for (int i = 0; i < timezones.length(); ++i) {
            QString timezone = i18n((timezones.at(i).isEmpty() ? KSystemTimeZones::local() : KSystemTimeZones::zone(timezones.at(i))).name().toUtf8().data()).replace(QLatin1Char('_'), QLatin1Char(' '));

            if (shortForm && timezone.contains(QLatin1Char('/'))) {
                timezone = timezone.split(QLatin1Char('/')).last();
            }

            Plasma::DataEngine::Data data = m_applet->dataEngine(QLatin1String("time"))->query(timezones.at(i));

            timezones[i] = QString(QLatin1String("<td align=\"right\"><nobr><i>%1</i>:</nobr></td><td align=\"left\"><nobr>%2 %3</nobr></td>")).arg(timezone).arg(KGlobal::locale()->formatTime(data[QLatin1String("Time")].toTime(), false)).arg(KGlobal::locale()->formatDate(data[QLatin1String("Date")].toDate(), KLocale::LongDate));
        }

        return QLatin1String("<table>\n<tr>") + timezones.join(QLatin1String("</tr>\n<tr>")) + QLatin1String("</tr>\n</table>");
    case 'H': // Holidays list
        return (shortForm ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join(QLatin1String("<br>\n")));
    case 'E': // Events list
        return (shortForm ? m_eventsShort : m_eventsLong);
    case 'R': // Sunrise time
        return KGlobal::locale()->formatTime(m_sunrise, false);
    case 'S': // Sunset time
        return KGlobal::locale()->formatTime(m_sunset, false);
    default:
        return QString(placeholder);
    }

    return QString();
}

QString Applet::evaluatePlaceholder(ushort placeholder, int alternativeForm, bool shortForm, bool textualForm)
{
    QString longest;
    QString temporary;
    int amount;

    switch (placeholder) {
    case 's':
    case 'm':
    case 'h':
    case 'd':
        return QLatin1String("00");
    case 'p':
        return ((i18n("pm").length() > i18n("am").length()) ? i18n("pm") : i18n("am"));
    case 'w':
        if (textualForm) {
            amount = m_applet->calendar()->daysInWeek(m_dateTime.date());

            for (int i = 0; i <= amount; ++i) {
                temporary = m_applet->calendar()->weekDayName(i, (shortForm ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));

                if (temporary.length() > longest.length()) {
                    longest = temporary;
                }
            }

            return longest;
        }

        return QString(QLatin1Char('0')).repeated(QString::number(m_applet->calendar()->daysInWeek(m_dateTime.date())).length());
    case 'D':
        return QString(QLatin1Char('0')).repeated(QString::number(m_applet->calendar()->daysInYear(m_dateTime.date())).length());
    case 'W':
        return QString(QLatin1Char('0')).repeated(QString::number(m_applet->calendar()->weeksInYear(m_dateTime.date())).length());
    case 'M':
        if (textualForm) {
            alternativeForm = ((alternativeForm == 0) ? KGlobal::locale()->dateMonthNamePossessive() : (alternativeForm == 1));

            amount = m_applet->calendar()->monthsInYear(m_dateTime.date());

            for (int i = 0; i < amount; ++i) {
                temporary = m_applet->calendar()->monthName(i, m_applet->calendar()->year(m_dateTime.date()), (shortForm ? (alternativeForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (alternativeForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));

                if (temporary.length() > longest.length()) {
                    longest = temporary;
                }
            }

            return longest;
        }

        return QString(QLatin1Char('0')).repeated(QString::number(m_applet->calendar()->monthsInYear(m_dateTime.date())).length());
    case 'Y':
        return (shortForm ? QLatin1String("00") : QLatin1String("0000"));
    case 'U':
        return QString(QLatin1Char('0')).repeated(QString::number(m_dateTime.toTime_t()).length());
    case 't':
    case 'T':
    case 'A':
    case 'z':
    case 'H':
    case 'E':
        return evaluatePlaceholder(placeholder, QDateTime::currentDateTime(), alternativeForm, shortForm, textualForm);
    case 'R':
    case 'S':
        return KGlobal::locale()->formatTime(QTime(), false);
    default:
        return QString(placeholder);
    }

    return QString();
}

Format Applet::format(QString name) const
{
    if (name.isEmpty()) {
        if (!m_format.html.isEmpty()) {
            return m_format;
        }

        name = config().readEntry("format", "%default%");
    }

    QHash<QString, Format> formats;
    formats[QLatin1String("%default%")] = Format();
    formats[QLatin1String("%default%")].title = i18n("Default");
    formats[QLatin1String("%default%")].html = QLatin1String("<div style=\"text-align:center; margin:5px;\"><big>%h:%m:%s</big><br><small>%d.%M.%Y</small></div>");
    formats[QLatin1String("%default%")].background = true;
    formats[QLatin1String("%flat%")] = Format();
    formats[QLatin1String("%flat%")].title = i18n("Flat");
    formats[QLatin1String("%flat%")].html = QLatin1String("<div style=\"text-align:center; margin:5px;\"><big style=\"font-family:'Nimbus Sans L Condensed';\">%h:%m:%s</big><br><span style=\"font-size:small; font-family:'Nimbus Sans L';\">%d.%M.%Y</small></div>");
    formats[QLatin1String("%flat%")].background = true;
    formats[QLatin1String("%simple%")] = Format();
    formats[QLatin1String("%simple%")].title = i18n("Simple");
    formats[QLatin1String("%simple%")].html = QLatin1String("<div style=\"text-align:center; font-size:25px; margin:5px;\">%h:%m</div>");
    formats[QLatin1String("%simple%")].background = true;
    formats[QLatin1String("%verbose%")] = Format();
    formats[QLatin1String("%verbose%")].title = i18n("Verbose");
    formats[QLatin1String("%verbose%")].html = QLatin1String("<div style=\"text-align:center; opacity:0.85;\"><span style=\"font-size:30px;\">%h:%m:%s</span><br><span style=\"font-size:12px;\">%$w, %d.%M.%Y</span></div>");
    formats[QLatin1String("%verbose%")].background = false;
    formats[QLatin1String("%dbclock%")] = Format();
    formats[QLatin1String("%dbclock%")].title = i18n("dbClock");
    formats[QLatin1String("%dbclock%")].html = QLatin1String("<div style=\"height:50px;\"><div style=\"text-align:center; white-space:pre; font-size:25px; margin:-10px 0 5px 5px; -webkit-box-reflect:below -5px -webkit-gradient(linear, left top, left bottom, from(transparent), color-stop(0.5, transparent), to(white));\">%h:%m<span style=\"font-size:30px; position:relative; left:-8px; top:4px; z-index:-1; opacity:0.5;\">%s</span></div></div>");
    formats[QLatin1String("%dbclock%")].background = false;
    formats[QLatin1String("%calendar%")] = Format();
    formats[QLatin1String("%calendar%")].title = i18n("Calendar");
    formats[QLatin1String("%calendar%")].html = QLatin1String("<div style=\"width:295px; min-height:295px; text-shadow:0 0 5px #AAA;\"><div style=\"margin:30px 0 0 0; padding:30px 20px 20px 20px; position:relative; font-weight:bold; font-size:30px; text-align:center; background:-webkit-gradient(linear, left top, left bottom, from(#E5702B), to(#A33B03)); color:white; border-radius:20px; box-shadow:5px 5px 15px #888; opacity:0.7;\">%$w<br><span style=\"font-size:130px; line-height:140px;\">%!d</span><br><span style=\"font-size:35px;\">%$M %Y</span><br>%!H<div class=\"decor\" style=\"position:absolute; top:-30px; left:-10px; width:310px; height:60px; padding:10px 20px;\"><div></div><div></div><div></div><div></div><div></div><div></div></div></div></div>");
    formats[QLatin1String("%calendar%")].css = QLatin1String(".decor div{width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;}");
    formats[QLatin1String("%calendar%")].background = false;

    if (formats.contains(name)) {
        return formats[name];
    }

    if (config().group("Formats").groupList().contains(name)) {
        KConfigGroup formatConfiguration = config().group("Formats").group(name);
        Format format;
        format.title = formatConfiguration.readEntry("title", i18n("Custom"));
        format.html = formatConfiguration.readEntry("html", QString());
        format.css = formatConfiguration.readEntry("css", QString());
        format.background = formatConfiguration.readEntry("background", true);

        if (!format.html.isEmpty()) {
            return format;
        }
    }

    return formats[QLatin1String("%default%")];
}

QStringList Applet::formats(bool all) const
{
    QStringList formats;
    formats << QLatin1String("%default%") << QLatin1String("%flat%") << QLatin1String("%simple%") << QLatin1String("%verbose%") << QLatin1String("%dbclock%") << QLatin1String("%calendar%");

    if (all) {
        const int count = formats.count();

        QStringList userFormats = config().group("Formats").groupList();

        for (int i = 0; i < userFormats.count(); ++i) {
            if (!formats.contains(userFormats.at(i))) {
                formats.append(userFormats.at(i));
            }
        }

        if (count != formats.count()) {
            formats.insert(count,  QLatin1String(""));
        }
    }

    return formats;
}

QStringList Applet::clipboardFormats() const
{
    QStringList clipboardFormats;
    clipboardFormats << QLatin1String("%!t")
    << QLatin1String("%t")
    << QLatin1String("%h:%m:%s")
    << QString()
    << QLatin1String("%!T")
    << QLatin1String("%T")
    << QString()
    << QLatin1String("%!A")
    << QLatin1String("%A")
    << QLatin1String("%Y-%M-%d %h:%m:%s")
    << QString()
    << QLatin1String("%U");

    return config().readEntry("clipboardFormats", clipboardFormats);
}

QPair<QString, QString> Applet::toolTipFormat() const
{
    QPair<QString, QString> toolTipFormat;
    toolTipFormat.first = (config().keyList().contains(QLatin1String("toolTipFormatMain")) ? config().readEntry("toolTipFormatMain", QString()) : QLatin1String("<div style=\"text-align:center;\">%h:%m:%s<br>%$w, %d.%M.%Y</div>"));
    toolTipFormat.second = (config().keyList().contains(QLatin1String("toolTipFormatSub")) ? config().readEntry("toolTipFormatSub", QString()) : QLatin1String("%!Z%E"));

    return toolTipFormat;
}

QList<QAction*> Applet::contextualActions()
{
    QList<QAction*> actions = ClockApplet::contextualActions();

    if (!m_clipboardAction) {
        m_clipboardAction = new QAction(SmallIcon(QLatin1String("edit-copy")), i18n("C&opy to Clipboard"), this);
        m_clipboardAction->setMenu(new KMenu);

        connect(this, SIGNAL(destroyed()), m_clipboardAction->menu(), SLOT(deleteLater()));
        connect(m_clipboardAction->menu(), SIGNAL(aboutToShow()), this, SLOT(updateClipboardMenu()));
        connect(m_clipboardAction->menu(), SIGNAL(triggered(QAction*)), this, SLOT(copyToClipboard(QAction*)));
    }

    for (int i = 0; i < actions.count(); ++i) {
        if (actions.at(i)->text() == i18n("C&opy to Clipboard")) {
            actions.removeAt(i);
            actions.insert(i, m_clipboardAction);

            m_clipboardAction->setVisible(!clipboardFormats().isEmpty());
        }
    }

    return actions;
}

}
