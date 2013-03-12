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

#include "FormatDelegate.h"
#include "Clock.h"
#include "FormatLineEdit.h"

namespace AdjustableClock
{

FormatDelegate::FormatDelegate(Clock *clock, QObject *parent) : QStyledItemDelegate(parent),
    m_clock(clock)
{
}

void FormatDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

void FormatDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    FormatLineEdit *lineEdit = static_cast<FormatLineEdit*>(editor);
    lineEdit->setText(index.data(Qt::EditRole).toString());
}

void FormatDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    model->setData(index, static_cast<FormatLineEdit*>(editor)->text(), Qt::EditRole);
}

QWidget* FormatDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    FormatLineEdit *lineEdit = new FormatLineEdit(parent);
    lineEdit->setClock(m_clock);

    setEditorData(lineEdit, index);

    return lineEdit;
}

QString FormatDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    return (value.toString().isEmpty() ? QString() : m_clock->evaluate(value.toString()));
}

QSize FormatDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    if (size.height() < 30) {
        size.setHeight(30);
    }

    return size;
}

}
