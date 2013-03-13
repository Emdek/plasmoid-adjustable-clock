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

#include "Applet.h"
#include "DataSource.h"
#include "Clock.h"
#include "Configuration.h"

#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileSystemWatcher>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#include <KMenu>
#include <KLocale>
#include <KConfigDialog>
#include <KStandardDirs>

#include <Plasma/Theme>
#include <Plasma/Containment>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_source(new DataSource(this)),
    m_clock(new Clock(m_source)),
    m_clipboardAction(NULL),
    m_theme(-1)
{
    KGlobal::locale()->insertCatalog("libplasmaclock");
    KGlobal::locale()->insertCatalog("timezones4");
    KGlobal::locale()->insertCatalog("adjustableclock");

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setHasConfigurationInterface(true);
    resize(120, 80);
}

void Applet::init()
{
    ClockApplet::init();

    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    constraintsEvent(Plasma::SizeConstraint);
    configChanged();

    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPath(KStandardDirs::locate("data", "adjustableclock/themes.xml"));
    watcher->addPath(KStandardDirs::locateLocal("data", "adjustableclock/custom-themes.xml"));

    m_clock->setDocument(m_page.mainFrame());

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(&m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(repaint()));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(clockConfigChanged()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints(getTheme().background ? DefaultBackground : NoBackground);
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
    Configuration *configuration = new Configuration(this, parent);

    connect(configuration, SIGNAL(accepted()), this, SIGNAL(configNeedsSaving()));
    connect(configuration, SIGNAL(accepted()), this, SLOT(configChanged()));
}

void Applet::clockConfigChanged()
{
    const QString path = KStandardDirs::locate("data", "adjustableclock/themes.xml");

    m_themes = loadThemes(path, true);
    m_themes.append(loadThemes(KStandardDirs::locateLocal("data", "adjustableclock/custom-themes.xml"), false));

    if (m_themes.isEmpty()) {
        Theme theme;
        theme.id = "%default%";
        theme.title = i18n("Error");
        theme.html = i18n("Missing or invalid data file: %1.").arg(path);
        theme.background = true;
        theme.bundled = false;

        m_theme = 0;

        m_themes.append(theme);
    } else {
        const QString id = config().readEntry("theme", "%default%");

        m_theme = -1;

        for (int i = 0; i < m_themes.count(); ++i) {
            if (m_themes.at(i).id == id) {
                m_theme = i;

                break;
            }
        }

        if (m_theme < 0) {
            m_theme = 0;
        }
    }

    changeEngineTimezone(currentTimezone(), currentTimezone());
}

void Applet::clockConfigAccepted()
{
    emit configNeedsSaving();
}

void Applet::changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone)
{
    Q_UNUSED(oldTimezone)

    m_source->setTimezone(newTimezone);

    const Theme theme = getTheme();

    m_page.mainFrame()->setHtml(getPageLayout(theme.html, theme.css, theme.script));

    constraintsEvent(Plasma::SizeConstraint);
    updateSize();
}

void Applet::copyToClipboard()
{
    QApplication::clipboard()->setText(m_clock->evaluate(config().readEntry("fastCopyExpression", "Clock.toString(Clock.YearValue) + '-' + Clock.toString(Clock.MonthValue) + '-' + Clock.toString(Clock.DayOfMonthValue) + ' ' + Clock.toString(Clock.HourValue) + ':' + Clock.toString(Clock.MinuteValue) + ':' + Clock.toString(Clock.SecondValue)")));
}

void Applet::copyToClipboard(QAction *action)
{
    QApplication::clipboard()->setText(action->text());
}

void Applet::toolTipAboutToShow()
{
    connect(m_source, SIGNAL(dataChanged(QList<ClockTimeValue>)), this, SLOT(updateToolTipContent()));

    updateToolTipContent();
}

void Applet::toolTipHidden()
{
    disconnect(m_source, SIGNAL(dataChanged(QList<ClockTimeValue>)), this, SLOT(updateToolTipContent()));

    Plasma::ToolTipManager::self()->clearContent(this);
}

void Applet::repaint()
{
    update();
}

void Applet::updateClipboardMenu()
{
    const QStringList clipboardExpressions = getClipboardExpressions();

    qDeleteAll(m_clipboardAction->menu()->actions());

    m_clipboardAction->menu()->clear();

    for (int i = 0; i < clipboardExpressions.count(); ++i) {
        if (clipboardExpressions.at(i).isEmpty()) {
            m_clipboardAction->menu()->addSeparator();
        } else {
            m_clipboardAction->menu()->addAction(m_clock->evaluate(clipboardExpressions.at(i)));
        }
    }
}

void Applet::updateToolTipContent()
{
    Plasma::ToolTipContent toolTipData;
    const QString toolTipExpressionMain = (config().keyList().contains("toolTipExpressionMain") ? config().readEntry("toolTipExpressionMain", QString()) : "'<div style=\"text-align:center;\">' + Clock.toString(Clock.HourValue) + ':' + Clock.toString(Clock.MinuteValue) + ':' + Clock.toString(Clock.SecondValue) +'<br>' + Clock.toString(Clock.DayOfWeekValue, Clock.TextualFormOption) + ', ' + Clock.toString(Clock.DayOfMonthValue) + '.' + Clock.toString(Clock.MonthValue) + '.' + Clock.toString(Clock.YearValue) + '</div>'");
    const QString toolTipExpressionSub = (config().keyList().contains("toolTipExpressionSub") ? config().readEntry("toolTipExpressionSub", QString()) : "Clock.toString(Clock.TimezonesValue, Clock.ShortFormOption) + Clock.toString(Clock.EventsValue)");

    if (!toolTipExpressionMain.isEmpty() || !toolTipExpressionSub.isEmpty()) {
        toolTipData.setImage(KIcon("chronometer").pixmap(IconSize(KIconLoader::Desktop)));
        toolTipData.setMainText(m_clock->evaluate(toolTipExpressionMain));
        toolTipData.setSubText(m_clock->evaluate(toolTipExpressionSub));
        toolTipData.setAutohide(false);
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTipData);
}

void Applet::updateSize()
{
    QSizeF size;

    if (formFactor() == Plasma::Horizontal) {
        size = QSizeF(containment()->boundingRect().width(), boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSizeF(boundingRect().width(), containment()->boundingRect().height());
    } else {
        if (getTheme().background) {
            size = contentsRect().size();
        } else {
            size = boundingRect().size();
        }
    }

    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

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
}

void Applet::updateTheme()
{
    m_page.settings()->setUserStyleSheetUrl(QUrl(QString("data:text/css;charset=utf-8;base64,").append(QString(getPageStyleSheet().toAscii().toBase64()))));
    m_page.settings()->setFontFamily(QWebSettings::StandardFont, Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family());
    m_page.mainFrame()->setHtml(m_page.mainFrame()->toHtml());
}

Clock* Applet::getClock() const
{
    return m_clock;
}

DataSource* Applet::getDataSource() const
{
    return m_source;
}

QString Applet::getPageLayout(const QString &html, const QString &css, const QString &script)
{
    return QString("<!DOCTYPE html><html><head><style type=\"text/css\">%1</style></head><body><div>%2</div><script type=\"text/javascript\" id=\"script\">%3</script></body></html>").arg(css).arg(html).arg(script);
}

QString Applet::getPageStyleSheet()
{
    return QString("body {color: %1;} html, body, body > div {margin: 0; padding: 0; height: 100%; width: 100%; vertical-align: middle;} body {display: table;} body > div {display: table-cell;}").arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name());
}

Theme Applet::getTheme() const
{
    if (m_theme >= 0 && m_theme < m_themes.count()) {
        return m_themes[m_theme];
    }

    Theme theme;
    theme.id = "%default%";
    theme.title = i18n("Error");
    theme.html = i18n("Invalid theme identifier.");
    theme.background = true;
    theme.bundled = false;

    return theme;
}

QStringList Applet::getClipboardExpressions() const
{
    QStringList clipboardExpressions;
    clipboardExpressions << "Clock.toString(Clock.TimeValue, Clock.ShortFormOption)"
    << "Clock.toString(Clock.TimeValue)"
    << QString()
    << "Clock.toString(Clock.DateValue, Clock.ShortFormOption)"
    << "Clock.toString(Clock.DateValue)"
    << QString()
    << "Clock.toString(Clock.DateTimeValue, Clock.ShortFormOption)"
    << "Clock.toString(Clock.DateTimeValue)"
    << "Clock.toString(Clock.YearValue) + '-' + Clock.toString(Clock.MonthValue) + '-' + Clock.toString(Clock.DayOfMonthValue) + ' ' + Clock.toString(Clock.HourValue) + ':' + Clock.toString(Clock.MinuteValue) + ':' + Clock.toString(Clock.SecondValue)"
    << QString()
    << "Clock.toString(Clock.TimestampValue)";

    return config().readEntry("clipboardExpressions", clipboardExpressions);
}

QList<Theme> Applet::getThemes() const
{
    return m_themes;
}

QList<Theme> Applet::loadThemes(const QString &path, bool bundled) const
{
    QList<Theme> themes;
    QFile file(path);
    file.open(QFile::ReadOnly | QFile::Text);

    QXmlStreamReader reader(&file);
    Theme theme;
    theme.bundled = bundled;

    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement()) {
            if (reader.name().toString() == "theme") {
                themes.append(theme);
            }

            continue;
        }

        if (reader.name().toString() == "theme") {
            theme.id = QString();
            theme.title = QString();
            theme.description = QString();
            theme.author = QString();
            theme.html = QString();
            theme.css = QString();
            theme.script = QString();
            theme.background = true;
        }

        if (reader.name().toString() == "id") {
            if (bundled) {
                theme.id = QString("%%1%").arg(reader.readElementText());
            } else {
                theme.id = reader.readElementText();
           }
        }

        if (reader.name().toString() == "title") {
            theme.title = i18n(reader.readElementText().toUtf8().data());
        }

        if (reader.name().toString() == "description") {
            theme.description = i18n(reader.readElementText().toUtf8().data());
        }

        if (reader.name().toString() == "author") {
            theme.author = reader.readElementText();
        }

        if (reader.name().toString() == "background") {
            theme.background = (reader.readElementText().toLower() == "true");
        }

        if (reader.name().toString() == "html") {
            theme.html = reader.readElementText();
        }

        if (reader.name().toString() == "css") {
            theme.css = reader.readElementText();
        }

        if (reader.name().toString() == "script") {
            theme.script = reader.readElementText();
        }
    }

    file.close();

    return themes;
}

QList<QAction*> Applet::contextualActions()
{
    QList<QAction*> actions = ClockApplet::contextualActions();

    if (!m_clipboardAction) {
        m_clipboardAction = new QAction(SmallIcon("edit-copy"), i18n("C&opy to Clipboard"), this);
        m_clipboardAction->setMenu(new KMenu);

        connect(this, SIGNAL(destroyed()), m_clipboardAction->menu(), SLOT(deleteLater()));
        connect(m_clipboardAction->menu(), SIGNAL(aboutToShow()), this, SLOT(updateClipboardMenu()));
        connect(m_clipboardAction->menu(), SIGNAL(triggered(QAction*)), this, SLOT(copyToClipboard(QAction*)));
    }

    for (int i = 0; i < actions.count(); ++i) {
        if (actions.at(i)->text() == i18n("C&opy to Clipboard")) {
            actions.removeAt(i);
            actions.insert(i, m_clipboardAction);

            m_clipboardAction->setVisible(!getClipboardExpressions().isEmpty());
        }
    }

    return actions;
}

}
