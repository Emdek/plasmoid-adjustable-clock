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

#include "ThemeWidget.h"

#include <QtGui/QPainter>
#include <QtGui/QDesktopServices>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>

#include <Plasma/Theme>

namespace AdjustableClock
{

ThemeWidget::ThemeWidget(Clock *clock, bool constant, QGraphicsWidget *parent) : Plasma::DeclarativeWidget(parent),
    m_clock(clock),
    m_rootObject(NULL),
    m_constant(constant)
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);

    if (!m_constant) {
        connect(m_clock, SIGNAL(componentChanged(ClockComponent)), this, SLOT(updateComponent(ClockComponent)));
    }

    connect(&m_page, SIGNAL(repaintRequested(QRect)), this, SLOT(update()));
}

void ThemeWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);

    updateSize();
}

void ThemeWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    const QPoint position = (event->pos() - m_offset).toPoint();

    setCursor(m_page.mainFrame()->hitTestContent(position).linkUrl().isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);

    QMouseEvent mouseEvent(QEvent::MouseMove, position, Qt::NoButton, Qt::NoButton, Qt::NoModifier);

    m_page.event(&mouseEvent);
}

void ThemeWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_rootObject) {
        Plasma::DeclarativeWidget::mousePressEvent(event);

        return;
    }

    const QUrl url = m_page.mainFrame()->hitTestContent((event->pos() - m_offset).toPoint()).linkUrl();

    if (url.isValid()) {
        QDesktopServices::openUrl(url);

        event->accept();
    } else {
        Plasma::DeclarativeWidget::mousePressEvent(event);
    }
}

void ThemeWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->translate(m_offset);

    m_page.mainFrame()->render(painter, QWebFrame::ContentsLayer);
}

void ThemeWidget::update()
{
    Plasma::DeclarativeWidget::update();
}

void ThemeWidget::updateComponent(ClockComponent component)
{
    const QLatin1String componentString = Clock::getComponentString(component);

    if (m_rootObject) {
        const QList<QObject*> elements = m_rootObject->findChildren<QObject*>();

        for (int i = 0; i < elements.count(); ++i) {
            const QVariantMap options = elements.at(i)->property("adjustableClock").toMap();

            if (!options.isEmpty() && options.value("component").toString() == componentString) {
                elements.at(i)->setProperty(options.value("attribute", "text").toString().toLatin1(), m_clock->evaluate(QString("Clock.getValue(Clock.%1, {%2})").arg(componentString).arg(options.value("options").toString().replace('\'', '"')), m_constant));
            }
        }

        return;
    }

    const QWebElementCollection elements = m_page.mainFrame()->findAllElements(QString("[component=%1]").arg(componentString));

    for (int i = 0; i < elements.count(); ++i) {
        const QString value = m_page.mainFrame()->evaluateJavaScript(QString("Clock.getValue(Clock.%1, {%2})").arg(componentString).arg(elements.at(i).attribute("options").replace('\'', '"'))).toString();

        if (elements.at(i).hasAttribute("attribute")) {
            elements.at(i).setAttribute(elements.at(i).attribute("attribute"), value);
        } else {
            elements.at(i).setInnerXml(value);
        }
    }

    m_page.mainFrame()->evaluateJavaScript(QString("Clock.sendEvent('Clock%1Changed')").arg(componentString));
}

void ThemeWidget::updateTheme()
{
    m_page.mainFrame()->evaluateJavaScript(QString("Clock.setStyleSheet('%1'); Clock.sendEvent('ClockThemeChanged');").arg(QString("body {font-family: \\'%1\\', sans; color: %2;}").arg(Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont).family()).arg(Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name()) + m_css));
}

void ThemeWidget::updateSize()
{
    const QSizeF size = boundingRect().size();

    if (m_rootObject) {
        m_rootObject->setProperty("width", size.width());
        m_rootObject->setProperty("height", size.height());

        return;
    }

    disconnect(m_page.mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SLOT(updateSize()));

    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    const qreal widthFactor = (size.width() / m_page.mainFrame()->contentsSize().width());
    const qreal heightFactor = (size.height() / m_page.mainFrame()->contentsSize().height());

    m_page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);
    m_page.setViewportSize(m_page.mainFrame()->contentsSize());

    m_offset = QPointF(((size.width() - m_page.mainFrame()->contentsSize().width()) / 2), ((size.height() - m_page.mainFrame()->contentsSize().height()) / 2));

    connect(m_page.mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SLOT(updateSize()));
}

void ThemeWidget::setHtml(const QString &theme, const QString &html, const QString &css)
{
    if (m_rootObject) {
        m_rootObject->deleteLater();

        m_rootObject = NULL;
    }

    m_css = css;

    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemHasNoContents, false);

    m_page.mainFrame()->setHtml(html);
    m_page.mainFrame()->addToJavaScriptWindowObject("Clock", new ClockObject(m_clock, m_constant, theme), QScriptEngine::ScriptOwnership);

    for (int i = 1; i < LastComponent; ++i) {
        m_page.mainFrame()->evaluateJavaScript(QString("Clock.%1 = %2;").arg(Clock::getComponentString(static_cast<ClockComponent>(i))).arg(i));
    }

    QFile file(":/helper.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    m_page.mainFrame()->evaluateJavaScript(stream.readAll());

    updateTheme();

    m_page.mainFrame()->evaluateJavaScript("Clock.sendEvent('ClockOptionsChanged')");

    for (int i = 1; i < LastComponent; ++i) {
        updateComponent(static_cast<ClockComponent>(i));
    }

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

QWebPage* ThemeWidget::getPage()
{
    return &m_page;
}

QSize ThemeWidget::getPreferredSize(const QSize &constraints)
{
    QSize contents;

    if (m_rootObject) {
        contents = QSize(m_rootObject->property("minimumWidth").toInt(), m_rootObject->property("minimumHeight").toInt());

        if (contents.width() <= 0) {
            contents.setWidth(150);
        }

        if (contents.height() <= 0) {
            contents.setHeight(100);
        }
    } else {
        m_page.setViewportSize(QSize(0, 0));
        m_page.mainFrame()->setZoomFactor(1);

        contents = m_page.mainFrame()->contentsSize();
    }

    QSize size;

    if (constraints.width() > -1) {
        size.setHeight(contents.height() * ((qreal) constraints.width() / contents.width()));
    } else if (constraints.height() > -1) {
        size.setWidth(contents.width() * ((qreal) constraints.height() / contents.height()));
    }

    updateSize();

    return size;
}

bool ThemeWidget::setTheme(const QString &path)
{
    disconnect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));

    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MidButton | Qt::RightButton);
    setFlag(QGraphicsItem::ItemHasNoContents, true);

    m_page.mainFrame()->setHtml(QString());

    m_css = QString();

    const QString qmlPath = (path + "/contents/ui/main.qml");

    if (QFile::exists(qmlPath)) {
        if (m_rootObject) {
            m_rootObject->deleteLater();
        }

        setQmlPath(qmlPath);

        m_rootObject = rootObject();

        for (int i = 1; i < LastComponent; ++i) {
            updateComponent(static_cast<ClockComponent>(i));
        }

        updateSize();

        return true;
    }

    QFile file(path + "/contents/ui/main.html");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    const QString html = stream.readAll();

    if (html.isEmpty()) {
        return false;
    }

    setHtml(QFileInfo(path).fileName(), html);
    updateSize();

    return true;
}

bool ThemeWidget::getBackgroundFlag() const
{
    return (m_rootObject ? m_rootObject->property("background").toBool() : (m_page.mainFrame()->findFirstElement("body").attribute("background").toLower() == "true"));
}

}
