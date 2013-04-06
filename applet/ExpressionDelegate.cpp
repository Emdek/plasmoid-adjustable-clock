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

#include "ExpressionDelegate.h"
#include "ExpressionLineEdit.h"
#include "Clock.h"

namespace AdjustableClock
{

ExpressionDelegate::ExpressionDelegate(Clock *clock, QObject *parent) : QStyledItemDelegate(parent),
    m_clock(clock)
{
}

void ExpressionDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

void ExpressionDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ExpressionLineEdit *lineEdit = static_cast<ExpressionLineEdit*>(editor);
    lineEdit->setText(index.data(Qt::EditRole).toString());
}

void ExpressionDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    model->setData(index, static_cast<ExpressionLineEdit*>(editor)->text(), Qt::EditRole);
}

QWidget* ExpressionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    ExpressionLineEdit *lineEdit = new ExpressionLineEdit(parent);
    lineEdit->setClock(m_clock);

    setEditorData(lineEdit, index);

    return lineEdit;
}

QString ExpressionDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    return (value.toString().isEmpty() ? QString() : m_clock->evaluate(value.toString(), true));
}

QSize ExpressionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    if (size.height() < 30) {
        size.setHeight(30);
    }

    return size;
}

}
