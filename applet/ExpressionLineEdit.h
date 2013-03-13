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

#ifndef ADJUSTABLECLOCKEXPRESSIONLINEEDIT_HEADER
#define ADJUSTABLECLOCKEXPRESSIONLINEEDIT_HEADER

#include <QtGui/QMenu>

#include <KLineEdit>

namespace AdjustableClock
{

class Clock;

class ExpressionLineEdit : public KLineEdit
{
    Q_OBJECT

    public:
        ExpressionLineEdit(QWidget *parent = NULL);

        void setClock(Clock *clock);

    protected slots:
        void insertPlaceholder();
        void insertPlaceholder(const QString &placeholder);
        void extendContextMenu(QMenu *menu);

    private:
        Clock *m_clock;
};

}

#endif
