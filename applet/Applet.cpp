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
#include "WebView.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QClipboard>
#include <QtGui/QGraphicsLinearLayout>

#include <KMenu>
#include <KLocale>
#include <KConfigDialog>
#include <KStandardDirs>

#include <Plasma/Package>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_widget(new Plasma::DeclarativeWidget(this)),
    m_clock(new Clock(this)),
    m_clipboardAction(NULL)
{
    KGlobal::locale()->insertCatalog("libplasmaclock");
    KGlobal::locale()->insertCatalog("timezones4");
    KGlobal::locale()->insertCatalog("adjustableclock");

    qmlRegisterType<WebView>("org.kde.plasma.adjustableclock", 1, 0, "ClockWebView");

    m_widget->setQmlPath("/home/michal/Programowanie/plasma/adjustableclock/adjustableclock/applet/data/view.qml");

    QMetaObject::invokeMethod(m_widget->rootObject(), "setClock", Q_ARG(Clock*, m_clock));

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal, this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addItem(m_widget);

    setLayout(layout);
    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    resize(120, 80);
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

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    bool result;

    QMetaObject::invokeMethod(m_widget->rootObject(), "getBackgroundFlag", Q_RETURN_ARG(bool, result));

    setBackgroundHints(result ? DefaultBackground : NoBackground);
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
                    html = readTheme(locations.at(i), themes.at(j));

                    break;
                }

                if (themes.at(j) == "digital") {
                    fallback = readTheme(locations.at(i), themes.at(j));
                }
            }
        }
    } else {
        html = config().readEntry("themeHtml", QString());
    }

    if (html.isEmpty()) {
        html = fallback;
    }

    QMetaObject::invokeMethod(m_widget->rootObject(), "setHtml", Q_ARG(QString, html));

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

Clock* Applet::getClock() const
{
    return m_clock;
}

QString Applet::readTheme(const QString &path, const QString &identifier) const
{
    QFile file(QString("%1/%2/contents/ui/main.html").arg(path).arg(identifier));
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
