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

#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>
#include <QtXml/QXmlStreamReader>

#include <KMenu>
#include <KLocale>
#include <KDateTime>
#include <KConfigDialog>
#include <KStandardDirs>
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
QString m_eventsQuery;
QDateTime m_dateTime;
QTime m_sunrise;
QTime m_sunset;
QFlags<ClockFeature> m_features;

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clipboardAction(NULL),
    m_theme(-1)
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
    constraintsEvent(Plasma::SizeConstraint);
    configChanged();

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Applet::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool force)
{
    if (!source.isEmpty() && source == m_eventsQuery) {
        updateEvents();

        return;
    }

    m_dateTime = QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());

    const int second = ((m_features & SecondsClockFeature || m_features & SecondsToolTipFeature) ? m_dateTime.time().second() : 0);

    if (m_features & HolidaysFeature && (force || (m_dateTime.time().hour() == 0 && m_dateTime.time().minute() == 0 && second == 0))) {
        updateHolidays();
    }

    if (m_features & EventsFeature && QTime::currentTime().hour() == 0 && m_dateTime.time().minute() == 0 && second == 0) {
        dataEngine(QLatin1String("calendar"))->connectSource(m_eventsQuery, this);

        m_eventsQuery = QLatin1String("events:") + QDate::currentDate().toString(Qt::ISODate) + QLatin1Char(':') + QDate::currentDate().addDays(1).toString(Qt::ISODate);

        dataEngine(QLatin1String("calendar"))->connectSource(m_eventsQuery, this);
    }

    if (force || (m_dateTime.time().minute() == 0 && second == 0)) {
        Plasma::DataEngine::Data sunData = dataEngine(QLatin1String("time"))->query(currentTimezone() + QLatin1String("|Solar"));

        m_sunrise = sunData[QLatin1String("Sunrise")].toDateTime().time();
        m_sunset = sunData[QLatin1String("Sunset")].toDateTime().time();
    }

    if (force || m_features & SecondsClockFeature || second == 0) {
        setTheme(evaluateFormat(theme().html, m_dateTime), theme().css);
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

    setBackgroundHints(theme().background ? DefaultBackground : NoBackground);
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

    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    if (theme().background && formFactor() != Plasma::Horizontal && formFactor() != Plasma::Vertical) {
        painter->translate(QPointF(contentsRect.x(), contentsRect.y()));
    }

    m_page.mainFrame()->render(painter);
}

void Applet::createClockConfigurationInterface(KConfigDialog *parent)
{
    new Configuration(this, parent);
}

void Applet::clockConfigChanged()
{
    const QString path = KStandardDirs::locate("data", QLatin1String("adjustableclock/themes.xml"));
    const QString id = config().readEntry("format", "%default%");
    QFile file(path);
    file.open(QFile::ReadOnly | QFile::Text);

    m_themes.clear();
    m_theme = -1;

    QXmlStreamReader reader(&file);
    Theme theme;
    theme.bundled = true;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement()) {
            if (reader.name().toString() == QLatin1String("theme")) {
                m_themes.append(theme);

                if (id == theme.id) {
                    m_theme = (m_themes.count() - 1);
                }
            }

            continue;
        }

        if (reader.name().toString() == QLatin1String("theme")) {
            QXmlStreamAttributes attributes = reader.attributes();

            theme.id = QLatin1Char('%') + attributes.value(QLatin1String("id")).toString() + QLatin1Char('%');
            theme.title = i18n(attributes.value(QLatin1String("title")).toString().toUtf8().data());
            theme.description = i18n(attributes.value(QLatin1String("description")).toString().toUtf8().data());
            theme.author = attributes.value(QLatin1String("author")).toString();
            theme.html = QString();
            theme.css = QString();
            theme.background = (attributes.value(QLatin1String("background")).toString().toLower() == QLatin1String("true"));
        }

        if (reader.name().toString() == QLatin1String("html")) {
            theme.html = reader.readElementText();
        }

        if (reader.name().toString() == QLatin1String("css")) {
            theme.css = reader.readElementText();
        }
    }

    file.close();

    if (m_themes.isEmpty()) {
        theme.id = QLatin1String("%default%");
        theme.title = i18n("Error");
        theme.html = i18n("Missing or invalid data file: %1.").arg(path);
        theme.background = true;
        theme.bundled = false;

        m_theme = 0;

        m_themes.append(theme);
    }

    const QStringList userThemes = config().group("Formats").groupList();

    for (int i = 0; i < userThemes.count(); ++i) {
        KConfigGroup themeConfiguration = config().group("Formats").group(userThemes.at(i));
        Theme theme;
        theme.id = themeConfiguration.readEntry("title", i18n("Custom"));
        theme.title = themeConfiguration.readEntry("title", i18n("Custom"));
        theme.html = themeConfiguration.readEntry("html", QString());
        theme.css = themeConfiguration.readEntry("css", QString());
        theme.background = themeConfiguration.readEntry("background", true);
        theme.bundled = false;

        m_themes.append(theme);

        if (id == theme.id) {
            m_theme = (m_themes.count() - 1);
        }
    }

    if (m_theme < 0 && m_themes.count()) {
        m_theme = 0;
    }

    changeEngineTimezone(currentTimezone(), currentTimezone());

    setTheme(evaluateFormat(this->theme().html, currentDateTime()), this->theme().css);

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
    const Theme theme = this->theme();
    const QPair<QString, QString> toolTipFormat = this->toolTipFormat();
    const QString toolTip = (toolTipFormat.first + QLatin1Char('|') + toolTipFormat.second);
    const QString string = (theme.html + QLatin1Char('|')) + toolTip;

    if (theme.html.contains(formatWithSeconds)) {
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

        if (m_eventsQuery.isEmpty()) {
            m_eventsQuery = QLatin1String("events:") + QDate::currentDate().toString(Qt::ISODate) + QLatin1Char(':') + QDate::currentDate().addDays(1).toString(Qt::ISODate);

            dataEngine(QLatin1String("calendar"))->connectSource(m_eventsQuery, this);
        }
    } else if (!m_eventsQuery.isEmpty()) {
        dataEngine(QLatin1String("calendar"))->disconnectSource(m_eventsQuery, this);

        m_eventsQuery = QString();
    }

    m_features = features;

    const bool alignToSeconds = (features & SecondsClockFeature || features & SecondsToolTipFeature);

    dataEngine(QLatin1String("time"))->connectSource(timezone, this, (alignToSeconds ? 1000 : 60000), (alignToSeconds ? Plasma::NoAlignment : Plasma::AlignToMinute));

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

    constraintsEvent(Plasma::SizeConstraint);
    updateSize();
    dataUpdated(QString(), dataEngine(QLatin1String("time"))->query(currentTimezone()), true);
}

void Applet::changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone)
{
    dataEngine(QLatin1String("time"))->disconnectSource(oldTimezone, this);

    connectSource(newTimezone);
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

void Applet::setTheme(const QString &html, const QString &css)
{
    if (html != m_currentHtml) {
        m_page.mainFrame()->setHtml(QLatin1String("<!DOCTYPE html><html><head><style type=\"text/css\">* {font-family: sans, '") + Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family() + QLatin1String("';} html, body, body > div {margin: 0; padding: 0; height: 100%; width: 100%; vertical-align: middle;} body {display: table;} body > div {display: table-cell;}") + css + QLatin1String("</style></head><body><div>") + html + QLatin1String("</div></body></html>"));

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

void Applet::updateEvents()
{
    Plasma::DataEngine::Data eventsData = m_applet->dataEngine(QLatin1String("calendar"))->query(QLatin1String("events:") + QDate::currentDate().toString(Qt::ISODate) + QLatin1Char(':') + QDate::currentDate().addDays(1).toString(Qt::ISODate));

    m_eventsShort = m_eventsLong = QString();

    if (eventsData.isEmpty()) {
        return;
    }

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

void Applet::updateHolidays()
{
    const QString region = m_applet->config().readEntry("holidaysRegions", m_applet->dataEngine(QLatin1String("calendar"))->query(QLatin1String("holidaysDefaultRegion"))[QLatin1String("holidaysDefaultRegion")]).toString().split(QLatin1Char(',')).first();
    const QString key = QLatin1String("holidays:") + region + QLatin1Char(':') + m_applet->currentDateTime().date().toString(Qt::ISODate);
    Plasma::DataEngine::Data holidaysData = m_applet->dataEngine(QLatin1String("calendar"))->query(key);

    m_holidays.clear();

    if (!holidaysData.isEmpty() && holidaysData.contains(key)) {
        QVariantList holidaysList = holidaysData[key].toList();
        QStringList holidays;

        for (int i = 0; i < holidaysList.length(); ++i) {
            m_holidays.append(holidaysList[i].toHash()[QLatin1String("Name")].toString());
        }
    }
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
    const Theme theme = this->theme();

    setTheme(evaluateFormat(theme.html), theme.css);

    QSizeF size;

    if (formFactor() == Plasma::Horizontal) {
        size = QSizeF(containment()->boundingRect().width(), boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSizeF(boundingRect().width(), containment()->boundingRect().height());
    } else {
        if (theme.background) {
            size = contentsRect().size();
        } else {
            size = boundingRect().size();
        }
    }

    m_page.mainFrame()->setZoomFactor(zoomFactor(m_page, size));

    if (formFactor() == Plasma::Horizontal) {
        setMinimumWidth(m_page.mainFrame()->contentsSize().width());
        setMinimumHeight(0);
    } else if (formFactor() == Plasma::Vertical) {
        setMinimumHeight(m_page.mainFrame()->contentsSize().height());
        setMinimumWidth(0);
    }

    m_page.setViewportSize(size.toSize());

    setTheme(evaluateFormat(theme.html, m_dateTime), theme.css);
}

void Applet::updateTheme()
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->evaluateJavaScript(QLatin1String("document.fgColor = '") + Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name() + QLatin1Char('\''));

    const QString html = m_currentHtml;

    m_currentHtml = QString();

    setTheme(html, theme().css);
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

QString Applet::evaluateFormat(const QString &format, QDateTime dateTime, bool special)
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
        const int start = i;
        int alternativeForm = 0;
        bool shortForm = false;
        bool textualForm = false;
        bool exclude = false;

        ++i;

        if (format.at(i) == QLatin1Char('~')) {
            ++i;

            exclude = true;
        }

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

            substitution = substitution.mid(range.first, range.second);
        }

        if (special && !exclude) {
            substitution = QLatin1String("<placeholder title=\"") + format.mid(start, (1 + i - start)) + QLatin1String("\" draggable=\"true\">") + substitution + QLatin1String("</placeholder>");
        }

        string.append(substitution);
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

        if (timezones.length() == 1) {
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
    case 'E': // Events list
        if (!(m_features & EventsFeature)) {
            updateEvents();
        }

        return (shortForm ? m_eventsShort : m_eventsLong);
    case 'H': // Holidays list
        if (!(m_features & HolidaysFeature)) {
            updateHolidays();
        }

        return (shortForm ? (m_holidays.isEmpty() ? QString() : m_holidays.last()) : m_holidays.join(QLatin1String("<br>\n")));
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
    case 'E':
    case 'H':
        return evaluatePlaceholder(placeholder, QDateTime::currentDateTime(), alternativeForm, shortForm, textualForm);
    case 'R':
    case 'S':
        return KGlobal::locale()->formatTime(QTime(), false);
    default:
        return QString(placeholder);
    }

    return QString();
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

QList<Theme> Applet::themes() const
{
    return m_themes;
}

Theme Applet::theme() const
{
    if (m_theme >= 0 && m_theme < m_themes.count()) {
        return m_themes[m_theme];
    }

    Theme theme;
    theme.id = QLatin1String("%default%");
    theme.title = i18n("Error");
    theme.html = i18n("Invalid theme identifier.");
    theme.background = true;
    theme.bundled = false;

    return theme;
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

qreal Applet::zoomFactor(const QWebPage &page, const QSizeF &size)
{
    page.setViewportSize(QSize(0, 0));
    page.mainFrame()->setZoomFactor(1);

    const qreal widthFactor = (size.width() / page.mainFrame()->contentsSize().width());
    const qreal heightFactor = (size.height() / page.mainFrame()->contentsSize().height());

    return ((widthFactor > heightFactor) ? heightFactor : widthFactor);
}

}
