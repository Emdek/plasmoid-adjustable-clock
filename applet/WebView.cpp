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
#include "Clock.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtWebKit/QWebFrame>

#include <Plasma/Theme>

namespace AdjustableClock
{

WebView::WebView(QDeclarativeItem *parent) : QDeclarativeItem(parent),
    m_webView(new QGraphicsWebView(this)),
    m_clock(NULL)
{
    QPalette palette = m_webView->page()->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_webView->page()->setPalette(palette);
    m_webView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    m_webView->installEventFilter(this);

    updateTheme();

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void WebView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);

    updateZoom();
}

void WebView::updateTheme()
{
    Clock::setupTheme(m_webView->page()->mainFrame(), "body {cursor: default;}");
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

void WebView::setClock(Clock *clock)
{
    m_clock = clock;
}

void WebView::setHtml(const QString &html)
{
    Clock::setupClock(m_webView->page()->mainFrame(), m_clock, QString(), html);

    updateZoom();
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
    }

    return QObject::eventFilter(object, event);
}

}
