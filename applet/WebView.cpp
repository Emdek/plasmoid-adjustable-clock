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

#include <QtGui/QPainter>
#include <QtGui/QDesktopServices>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include <Plasma/Theme>

namespace AdjustableClock
{

WebView::WebView(QDeclarativeItem *parent) : QDeclarativeItem(parent),
    m_clock(NULL)
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    updateTheme();

    connect(&m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(repaint(QRect)));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void WebView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(m_page.mainFrame()->hitTestContent(event->pos().toPoint()).linkUrl().isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);

    QMouseEvent mouseEvent = QMouseEvent(QEvent::MouseMove, event->pos().toPoint(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);

    m_page.event(&mouseEvent);
}

void WebView::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    const QUrl url = m_page.mainFrame()->hitTestContent(event->pos().toPoint()).linkUrl();

    if (url.isValid()) {
        QDesktopServices::openUrl(url);

        event->accept();
    } else {
        QDeclarativeItem::mousePressEvent(event);
    }
}

void WebView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    m_page.mainFrame()->render(painter, QWebFrame::ContentsLayer);
}

void WebView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    updateZoom();
}

void WebView::repaint(const QRect &rectangle)
{
    update(QRectF(rectangle));
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

void WebView::updateComponent(ClockComponent component)
{
    updateComponent(m_page.mainFrame(), component);
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
    setupTheme(m_page.mainFrame());
}

void WebView::updateZoom()
{
    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    const qreal widthFactor = (boundingRect().width() / m_page.mainFrame()->contentsSize().width());
    const qreal heightFactor = (boundingRect().height() / m_page.mainFrame()->contentsSize().height());

    m_page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);
    m_page.setViewportSize(boundingRect().size().toSize());
}

void WebView::setTheme(Clock *clock, const QString &theme, const QString &html, bool constant)
{
    m_clock = clock;

    setupClock(m_page.mainFrame(), new ClockObject(m_clock, constant, theme), html);
    updateZoom();

    if (!constant) {
        connect(m_clock, SIGNAL(componentChanged(ClockComponent)), this, SLOT(updateComponent(ClockComponent)));
    }
}

QSize WebView::getPreferredSize(const QSize &constraints)
{
    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    const QSize contents = m_page.mainFrame()->contentsSize();
    QSize size;

    if (constraints.width() > -1) {
        size.setHeight(contents.height() * ((qreal) constraints.width() / contents.width()));
    } else if (constraints.height() > -1) {
        size.setWidth(contents.width() * ((qreal) constraints.height() / contents.height()));
    }

    updateZoom();

    return size;
}

bool WebView::getBackgroundFlag() const
{
    return (m_page.mainFrame()->findFirstElement("body").attribute("background").toLower() == "true");
}

}
