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

#include "FormatLineEdit.h"
#include "Clock.h"
#include "PlaceholderDialog.h"

#include <KLocale>

namespace AdjustableClock
{

FormatLineEdit::FormatLineEdit(QWidget *parent) : KLineEdit(parent),
    m_clock(NULL)
{
    connect(this, SIGNAL(aboutToShowContextMenu(QMenu*)), this, SLOT(extendContextMenu(QMenu*)));
}

void FormatLineEdit::insertPlaceholder()
{
    if (m_clock) {
        connect(new PlaceholderDialog(m_clock, this), SIGNAL(insertPlaceholder(QString)), this, SLOT(insertPlaceholder(QString)));
    }
}

void FormatLineEdit::insertPlaceholder(const QString &placeholder)
{
    insert(placeholder);
}

void FormatLineEdit::extendContextMenu(QMenu *menu)
{
    if (m_clock) {
        menu->addSeparator();
        menu->addAction(KIcon(QLatin1String("chronometer")), i18n("Insert Format Component..."), this, SLOT(insertPlaceholder()));
    }
}

void FormatLineEdit::setClock(Clock *clock)
{
    m_clock = clock;
}

}
