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
#include "DeclarativeWidget.h"
#include "Configuration.h"

#include <QtCore/QDir>
#include <QtGui/QClipboard>

#include <KMenu>
#include <KLocale>
#include <KConfigDialog>
#include <KStandardDirs>

#include <Plasma/Package>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clock(new Clock(this)),
    m_widget(new DeclarativeWidget(m_clock, false, this)),
    m_clipboardAction(NULL)
{
    KGlobal::locale()->insertCatalog("libplasmaclock");
    KGlobal::locale()->insertCatalog("timezones4");
    KGlobal::locale()->insertCatalog("adjustableclock");

    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    resize(150, 100);
}

void Applet::init()
{
    ClockApplet::init();

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
}

void Applet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::MidButton) {
        copyToClipboard();
    } else {
        ClockApplet::mousePressEvent(event);
    }
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    ClockApplet::resizeEvent(event);

    m_widget->resize(event->newSize());

    updateSize();
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    const bool drawBackground = m_widget->getBackgroundFlag();

    if (formFactor() != Plasma::Horizontal && formFactor() != Plasma::Vertical && drawBackground) {
        qreal left, top, right, bottom;

        getContentsMargins(&left, &top, &right, &bottom);

        m_widget->setContentsMargins(left, top, right, bottom);
    } else {
        m_widget->setContentsMargins(0, 0, 0, 0);
    }

    if (constraints & Plasma::SizeConstraint) {
        updateSize();
    }

    setBackgroundHints(drawBackground ? DefaultBackground : NoBackground);
}

void Applet::createClockConfigurationInterface(KConfigDialog *parent)
{
    Configuration *configuration = new Configuration(this, parent);

    connect(configuration, SIGNAL(accepted()), this, SIGNAL(configNeedsSaving()));
    connect(configuration, SIGNAL(accepted()), this, SLOT(configChanged()));
}

void Applet::clockConfigChanged()
{
    if (!config().readEntry("themeHtml", QString()).isEmpty()) {
        m_widget->setHtml(config().readEntry("themeHtml", QString()));

        constraintsEvent(Plasma::SizeConstraint);

        return;
    }

    const QString id = config().readEntry("theme", "digital");
    const QStringList locations = KGlobal::dirs()->findDirs("data", "plasma/adjustableclock");

    for (int i = 0; i < locations.count(); ++i) {
        const QStringList themes = Plasma::Package::listInstalled(locations.at(i));

        for (int j = 0; j < themes.count(); ++j) {
            if (themes.at(j) == id && m_widget->setTheme(locations.at(i) + QDir::separator() + themes.at(j))) {
                constraintsEvent(Plasma::SizeConstraint);

                return;
            }
        }
    }

    if (!m_widget->setTheme(locations.first() + QDir::separator() + "digital")) {
        m_widget->setHtml("<div style=\"text-align: center;\"><span component=\"Hour\">12</span>:<span component=\"Minute\">30</span></div>");
    }

    constraintsEvent(Plasma::SizeConstraint);
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
    m_widget->updateSize();

    if (formFactor() != Plasma::Horizontal && formFactor() != Plasma::Vertical) {
        setMinimumSize(-1, -1);

        return;
    }

    QSize size;

    if (formFactor() == Plasma::Horizontal) {
        size = QSize(-1, boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSize(boundingRect().height(), -1);
    }

    setMinimumSize(m_widget->getPreferredSize(size));
}

Clock* Applet::getClock() const
{
    return m_clock;
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
