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

#include "DeclarativeWidget.h"
#include "WebView.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtDeclarative/QDeclarativeEngine>

namespace AdjustableClock
{

DeclarativeWidget::DeclarativeWidget(Clock *clock, bool constant, QGraphicsWidget *parent) : Plasma::DeclarativeWidget(parent),
    m_clock(clock),
    m_rootObject(NULL),
    m_constant(constant)
{
}

void DeclarativeWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);

    updateSize();
}

void DeclarativeWidget::updateSize()
{
    if (m_rootObject) {
        m_rootObject->setProperty("pos", contentsRect().topLeft());
        m_rootObject->setProperty("width", contentsRect().width());
        m_rootObject->setProperty("height", contentsRect().height());
    }
}

void DeclarativeWidget::setHtml(const QString &html, const QString &theme)
{
    if (m_rootObject) {
        m_rootObject->deleteLater();
    }

    qmlRegisterType<WebView>("org.kde.plasma.adjustableclock", 1, 0, "ClockWebView");

    mainComponent()->setData("import QtQuick 1.0\nimport org.kde.plasma.adjustableclock 1.0\nClockWebView{}", QUrl());

    m_rootObject = mainComponent()->create(engine()->rootContext());

    dynamic_cast<QGraphicsItem*>(m_rootObject)->setParentItem(this);

    QMetaObject::invokeMethod(m_rootObject, "setTheme", Q_ARG(Clock*, m_clock), Q_ARG(QString, theme), Q_ARG(QString, html), Q_ARG(bool, m_constant));

    updateSize();
}

QSize DeclarativeWidget::getPreferredSize(const QSize &constraints) const
{
    QSize size;

    QMetaObject::invokeMethod(m_rootObject, "getPreferredSize", Q_RETURN_ARG(QSize, size), Q_ARG(QSize, constraints));

    return size;
}

bool DeclarativeWidget::setTheme(const QString &path)
{
    const QString qmlPath = (path + "/contents/ui/main.qml");

    if (QFile::exists(qmlPath)) {
        if (m_rootObject) {
            m_rootObject->deleteLater();
        }

        setQmlPath(qmlPath);

        m_rootObject = rootObject();

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

    setHtml(html, QFileInfo(path).fileName());

    return true;
}

bool DeclarativeWidget::getBackgroundFlag() const
{
    bool result;

    QMetaObject::invokeMethod(m_rootObject, "getBackgroundFlag", Q_RETURN_ARG(bool, result));

    return result;
}

}
