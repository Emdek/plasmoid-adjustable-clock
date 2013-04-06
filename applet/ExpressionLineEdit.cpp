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

#include "ExpressionLineEdit.h"
#include "Clock.h"
#include "ComponentDialog.h"

#include <KLocale>

namespace AdjustableClock
{

ExpressionLineEdit::ExpressionLineEdit(QWidget *parent) : KLineEdit(parent),
    m_clock(NULL)
{
    connect(this, SIGNAL(aboutToShowContextMenu(QMenu*)), this, SLOT(extendContextMenu(QMenu*)));
}

void ExpressionLineEdit::insertComponent()
{
    if (m_clock) {
        connect(new ComponentDialog(m_clock->getDataSource(), this), SIGNAL(insertComponent(QString,ClockComponent)), this, SLOT(insertComponent(QString)));
    }
}

void ExpressionLineEdit::insertComponent(const QString &component, const QString &options)
{
    insert(options.isEmpty() ? QString("Clock.toString(Clock.%1)").arg(component) : QString("Clock.toString(Clock.%1, {%2})").arg(component).arg(options));
}

void ExpressionLineEdit::updateToolTip(const QString &expression)
{
    if (!expression.isEmpty()) {
        setToolTip(m_clock->evaluate(expression, true));
    }
}

void ExpressionLineEdit::extendContextMenu(QMenu *menu)
{
    if (m_clock) {
        menu->addSeparator();
        menu->addAction(KIcon("chronometer"), i18n("Insert Clock Component..."), this, SLOT(insertComponent()));
    }
}

void ExpressionLineEdit::setClock(Clock *clock)
{
    m_clock = clock;

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(updateToolTip(QString)));

    updateToolTip(text());
}

}
