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

#include "WebView.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include <Plasma/Theme>

namespace AdjustableClock
{

WebView::WebView(QDeclarativeItem *parent) : QDeclarativeItem(parent),
    m_webView(new QGraphicsWebView(this)),
    m_clock(NULL)
{
    QPalette palette = m_webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_webView->setPalette(palette);
    m_webView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    m_webView->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    m_webView->installEventFilter(this);

    updateTheme();

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void WebView::setupClock(QWebFrame *document, ClockObject *clock, const QString &html, const QString &css)
{
    document->setHtml(html);
    document->addToJavaScriptWindowObject("Clock", clock, QScriptEngine::ScriptOwnership);

    for (int i = 1; i < LastComponent; ++i) {
        document->evaluateJavaScript(QString("Clock.%1 = %2;").arg(Clock::getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    QFile file(":/helper.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    document->evaluateJavaScript(stream.readAll());

    setupTheme(document, css);

    document->evaluateJavaScript("Clock.sendEvent('ClockOptionsChanged')");

    for (int i = 1; i < LastComponent; ++i) {
        updateComponent(document, static_cast<ClockComponent>(i));
    }
}

void WebView::setupTheme(QWebFrame *document, const QString &css)
{
    document->evaluateJavaScript(QString("Clock.setStyleSheet('%1'); Clock.sendEvent('ClockThemeChanged');").arg(QString("body {font-family: \\'%1\\', sans; color: %2;}").arg(Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family()).arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name()) + css));
}

void WebView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    updateZoom();
}

void WebView::updateComponent(ClockComponent component)
{
    updateComponent(m_webView->page()->mainFrame(), component);
}

void WebView::updateComponent(QWebFrame *document, ClockComponent component)
{
    const QLatin1String componentString = Clock::getComponentString(component);
    const QWebElementCollection elements = document->findAllElements(QString("[component=%1]").arg(componentString));

    for (int i = 0; i < elements.count(); ++i) {
        const QString value = document->evaluateJavaScript(QString("Clock.getValue(Clock.%1, {%2})").arg(componentString).arg(elements.at(i).attribute("options").replace('\'', '"'))).toString();

        if (elements.at(i).hasAttribute("attribute")) {
            elements.at(i).setAttribute(elements.at(i).attribute("attribute"), value);
        } else {
            elements.at(i).setInnerXml(value);
        }
    }

    document->evaluateJavaScript(QString("Clock.sendEvent('Clock%1Changed')").arg(componentString));
}

void WebView::updateTheme()
{
    setupTheme(m_webView->page()->mainFrame(), "body {cursor: default;}");
}

void WebView::updateZoom()
{
    m_webView->page()->setViewportSize(QSize(0, 0));
    m_webView->page()->mainFrame()->setZoomFactor(1);

    const qreal widthFactor = (boundingRect().width() / m_webView->page()->mainFrame()->contentsSize().width());
    const qreal heightFactor = (boundingRect().height() / m_webView->page()->mainFrame()->contentsSize().height());

    m_webView->page()->mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);
    m_webView->page()->setViewportSize(boundingRect().size().toSize());
    m_webView->resize(boundingRect().size());
}

void WebView::setTheme(Clock *clock, const QString &theme, const QString &html, bool constant)
{
    m_clock = clock;

    setupClock(m_webView->page()->mainFrame(), new ClockObject(m_clock, constant, theme), html);
    updateZoom();

    if (!constant) {
        connect(m_clock, SIGNAL(componentChanged(ClockComponent)), this, SLOT(updateComponent(ClockComponent)));
    }
}

bool WebView::getBackgroundFlag() const
{
    return (m_webView->page()->mainFrame()->findFirstElement("body").attribute("background").toLower() == "true");
}

bool WebView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_webView && event->type() == QEvent::GraphicsSceneMousePress) {
        QGraphicsSceneMouseEvent *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        const QUrl url = m_webView->page()->mainFrame()->hitTestContent(mouseEvent->pos().toPoint()).linkUrl();

        if (url.isValid() && mouseEvent->button() == Qt::LeftButton) {
            QDesktopServices::openUrl(url);

            return true;
        }

        mouseEvent->ignore();
    } else if (object == m_webView && event->type() == QEvent::GraphicsSceneContextMenu) {
        event->ignore();

        return true;
    }

    return QObject::eventFilter(object, event);
}

}
