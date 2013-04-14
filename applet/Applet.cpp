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

#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include <KMenu>
#include <KLocale>
#include <KConfigDialog>
#include <KStandardDirs>

#include <Plasma/Package>
#include <Plasma/Containment>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clock(NULL),
    m_clipboardAction(NULL)
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

    QTimer::singleShot(100, this, SLOT(configChanged()));

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(&m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(repaint()));
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints((m_page.mainFrame()->findFirstElement("body").attribute("background").toLower() == "true") ? DefaultBackground : NoBackground);
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

    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    m_page.mainFrame()->render(painter, QWebFrame::ContentsLayer);
}

void Applet::createClockConfigurationInterface(KConfigDialog *parent)
{
    Configuration *configuration = new Configuration(this, parent);

    connect(configuration, SIGNAL(accepted()), this, SIGNAL(configNeedsSaving()));
    connect(configuration, SIGNAL(accepted()), this, SLOT(configChanged()));
}

void Applet::clockConfigChanged()
{
    const QString id = config().readEntry("theme", "digital");
    QString fallback = QString("<div style=\"text-align: center;\"><span component=\"Hour\">12</span>:<span component=\"Minute\">30</span></div>");
    QString html;

    if (config().readEntry("themeHtml", QString()).isEmpty()) {
        const QStringList locations = KGlobal::dirs()->findDirs("data", "plasma/adjustableclock");

        for (int i = 0; i < locations.count(); ++i) {
            if (!html.isEmpty()) {
                break;
            }

            const QStringList themes = Plasma::Package::listInstalled(locations.at(i));

            for (int j = 0; j < themes.count(); ++j) {
                if (themes.at(j) == id) {
                    html = readTheme(QString("%1/%2/contents/ui/main.html").arg(locations.at(i)).arg(themes.at(j)));

                    break;
                }

                if (themes.at(j) == "digital") {
                    fallback = readTheme(QString("%1/%2/contents/ui/main.html").arg(locations.at(i)).arg(themes.at(j)));
                }
            }
        }
    } else {
        html = config().readEntry("themeHtml", QString());
    }

    if (html.isEmpty()) {
        html = fallback;
    }

    m_clock->setTheme(html);

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
    QApplication::clipboard()->setText(m_clock->evaluate(config().readEntry("fastCopyExpression", "Clock.getValue(Clock.Year) + '-' + Clock.getValue(Clock.Month) + '-' + Clock.getValue(Clock.DayOfMonth) + ' ' + Clock.getValue(Clock.Hour) + ':' + Clock.getValue(Clock.Minute) + ':' + Clock.getValue(Clock.Second)")));
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
        if (m_page.mainFrame()->findFirstElement("body").attribute("background").toLower() == "true") {
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

QString Applet::readTheme(const QString &path) const
{
    QFile file(path);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    return stream.readAll();
}

QStringList Applet::getClipboardExpressions() const
{
    QStringList clipboardExpressions;
    clipboardExpressions << "Clock.getValue(Clock.Time, {'short': true})"
    << "Clock.getValue(Clock.Time)"
    << QString()
    << "Clock.getValue(Clock.Date, {'short': true})"
    << "Clock.getValue(Clock.Date)"
    << QString()
    << "Clock.getValue(Clock.DateTime, {'short': true})"
    << "Clock.getValue(Clock.DateTime)"
    << "Clock.getValue(Clock.Year) + '-' + Clock.getValue(Clock.Month) + '-' + Clock.getValue(Clock.DayOfMonth) + ' ' + Clock.getValue(Clock.Hour) + ':' + Clock.getValue(Clock.Minute) + ':' + Clock.getValue(Clock.Second)"
    << QString()
    << "Clock.getValue(Clock.Timestamp)";

    return config().readEntry("clipboardExpressions", clipboardExpressions);
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
