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
#include "Clock.h"
#include "Configuration.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#include <KMenu>
#include <KLocale>
#include <KConfigDialog>
#include <KStandardDirs>

#include <Plasma/Containment>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clock(NULL),
    m_clipboardAction(NULL),
    m_newTheme(false)
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
    if (!m_clock) {
        m_clock = new Clock(this, m_page.mainFrame());
    }

    ClockApplet::init();

    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    QTimer::singleShot(100, this, SLOT(configChanged()));

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(&m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(repaint()));
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints(m_theme.background ? DefaultBackground : NoBackground);
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
    m_theme.id = config().readEntry("theme", "default");
    m_theme.html = QString("<div style=\"text-align: center;\"><span component=\"Hour\">12</span>:<span component=\"Minute\">30</span></div>");
    m_theme.background = true;
    m_theme.bundled = false;
    m_newTheme = false;

    if (config().readEntry("themeHtml", QString()).isEmpty()) {
        const QList<Theme> themes = getThemes();

        for (int i = 0; i < themes.count(); ++i) {
            if (m_theme.id == themes.at(i).id) {
                m_theme = themes.at(i);

                break;
            }
        }

        if (m_theme.html.isEmpty() && !themes.isEmpty()) {
            m_theme = themes.first();
        }
    } else {
        m_theme.html = config().readEntry("themeHtml", QString());
        m_theme.background = config().readEntry("themeBackground", true);

        m_newTheme = true;
    }

    m_clock->setTheme(m_theme.html);

    constraintsEvent(Plasma::SizeConstraint);
    updateSize();
}

void Applet::clockConfigAccepted()
{
    emit configNeedsSaving();
}

void Applet::changeEngineTimezone(const QString &oldTimeZone, const QString &newTimeZone)
{
    Q_UNUSED(oldTimeZone)
    Q_UNUSED(newTimeZone)

    m_clock->updateTimeZone();
}

void Applet::copyToClipboard()
{
    QApplication::clipboard()->setText(m_clock->evaluate(config().readEntry("fastCopyExpression", "Clock.toString(Clock.Year) + '-' + Clock.toString(Clock.Month) + '-' + Clock.toString(Clock.DayOfMonth) + ' ' + Clock.toString(Clock.Hour) + ':' + Clock.toString(Clock.Minute) + ':' + Clock.toString(Clock.Second)")));
}

void Applet::copyToClipboard(QAction *action)
{
    QApplication::clipboard()->setText(action->text());
}

void Applet::repaint()
{
    update();
}

void Applet::toolTipAboutToShow()
{
    if (config().keyList().contains("toolTipExpressionMain") || config().keyList().contains("toolTipExpressionSub")) {
        if (config().readEntry("toolTipExpressionMain", QString()).isEmpty() && config().readEntry("toolTipExpressionSub", QString()).isEmpty()) {
            return;
        }

        connect(m_clock, SIGNAL(tick()), this, SLOT(updateToolTipContent()));

        updateToolTipContent();
    } else {
        ClockApplet::toolTipAboutToShow();
    }
}

void Applet::toolTipHidden()
{
    disconnect(m_clock, SIGNAL(tick()), this, SLOT(updateToolTipContent()));

    Plasma::ToolTipManager::self()->clearContent(this);
}

void Applet::updateToolTipContent()
{
    Plasma::ToolTipContent toolTipData;
    toolTipData.setImage(KIcon("chronometer").pixmap(IconSize(KIconLoader::Desktop)));
    toolTipData.setMainText(m_clock->evaluate(config().readEntry("toolTipExpressionMain", QString())));
    toolTipData.setSubText(m_clock->evaluate(config().readEntry("toolTipExpressionSub", QString())));
    toolTipData.setAutohide(false);

    Plasma::ToolTipManager::self()->setContent(this, toolTipData);
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

void Applet::updateSize()
{
    QSizeF size;

    if (formFactor() == Plasma::Horizontal) {
        size = QSizeF(containment()->boundingRect().width(), boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSizeF(boundingRect().width(), containment()->boundingRect().height());
    } else {
        if (m_theme.background) {
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

Clock* Applet::getClock() const
{
    return m_clock;
}

QStringList Applet::getClipboardExpressions() const
{
    QStringList clipboardExpressions;
    clipboardExpressions << "Clock.toString(Clock.Time, {'short': true})"
    << "Clock.toString(Clock.Time)"
    << QString()
    << "Clock.toString(Clock.Date, {'short': true})"
    << "Clock.toString(Clock.Date)"
    << QString()
    << "Clock.toString(Clock.DateTime, {'short': true})"
    << "Clock.toString(Clock.DateTime)"
    << "Clock.toString(Clock.Year) + '-' + Clock.toString(Clock.Month) + '-' + Clock.toString(Clock.DayOfMonth) + ' ' + Clock.toString(Clock.Hour) + ':' + Clock.toString(Clock.Minute) + ':' + Clock.toString(Clock.Second)"
    << QString()
    << "Clock.toString(Clock.Timestamp)";

    return config().readEntry("clipboardExpressions", clipboardExpressions);
}

QList<Theme> Applet::getThemes() const
{
    QList<Theme> themes;

    for (int i = 0; i < 2; ++i) {
        QFile file(KStandardDirs::locate("data", QString("adjustableclock/%1themes.xml").arg((i == 0) ? QString() : "custom-")));
        file.open(QFile::ReadOnly | QFile::Text);

        QXmlStreamReader reader(&file);
        Theme theme;
        theme.bundled = (i == 0);

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
                theme.background = true;
            }

            if (reader.name().toString() == "id") {
                theme.id = reader.readElementText();
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
        }

        file.close();
    }

    if (m_newTheme) {
        themes.append(m_theme);
    }

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

            break;
        }
    }

    return actions;
}

}
