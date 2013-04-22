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

#ifndef ADJUSTABLECLOCKTHEMEDELEGATE_HEADER
#define ADJUSTABLECLOCKTHEMEDELEGATE_HEADER

#include <QtGui/QStyledItemDelegate>

namespace AdjustableClock
{

class Clock;

class ThemeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ThemeDelegate(Clock *clock, QObject *parent = NULL);
        ~ThemeDelegate();

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void setEditorData(QWidget *editor, const QModelIndex &index) const;
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    public slots:
        void clear();

    protected slots:
        void propagateSignal();

    private:
        Clock *m_clock;

    signals:
        void showAbout(QString theme);
        void showEditor(QString theme);
        void showOptions(QString theme);
};

}

#endif
