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

#include "OptionWidget.h"

namespace AdjustableClock
{

OptionWidget::OptionWidget(const Option &option, QWidget *parent) : QWidget(parent),
    m_widget(NULL),
    m_colorButton(NULL),
    m_comboBox(NULL),
    m_checkBox(NULL),
    m_spinBox(NULL),
    m_textEdit(NULL),
    m_option(option)
{
    if (!m_option.values.isEmpty()) {
        m_widget = m_comboBox = new QComboBox(this);

        for (int i = 0; i < m_option.values.count(); ++i) {
            m_comboBox->addItem(m_option.values.at(i).first, m_option.values.at(i).second);

            if (m_option.value.toString() == m_option.values.at(i).second) {
                m_comboBox->setCurrentIndex(i);
            }
        }

        connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateValue()));

        return;
    }

    switch (m_option.value.type()) {
    case QVariant::Bool:
        m_widget = m_checkBox = new QCheckBox(this);
        m_checkBox->setChecked(m_option.value.toBool());

        connect(m_checkBox, SIGNAL(toggled(bool)), this, SLOT(updateValue()));

        break;
    case QVariant::Int:
        m_widget = m_spinBox = new QSpinBox(this);
        m_spinBox->setValue(m_option.value.toInt());
        m_spinBox->setMinimum(m_option.minimum);
        m_spinBox->setMaximum(m_option.maximum);

        connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateValue()));

        break;
    case QVariant::Color:
        m_widget = m_colorButton = new KColorButton(m_option.value.value<QColor>(), this);

        connect(m_colorButton, SIGNAL(changed(QColor)), this, SLOT(updateValue()));

        break;
    default:
        m_widget = m_textEdit = new QPlainTextEdit(this);
        m_textEdit->setPlainText(m_option.value.toString());

        connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(updateValue()));

        break;
    }
}

void OptionWidget::updateValue()
{
    if (!m_widget) {
        return;
    }

    if (!m_option.values.isEmpty()) {
        m_currentValue = m_comboBox->itemData(m_comboBox->currentIndex());

        return;
    }

    switch (m_option.value.type()) {
    case QVariant::Bool:
        m_currentValue = QVariant(m_checkBox->isChecked());

        break;
    case QVariant::Int:
        m_currentValue = QVariant(m_spinBox->value());

        break;
    case QVariant::Color:
        m_currentValue = QVariant(m_colorButton->color());

        break;
    default:
        m_currentValue = QVariant(m_textEdit->toPlainText());

        break;
    }
}

QWidget *OptionWidget::getWidget()
{
    return m_widget;
}

QVariant OptionWidget::getValue() const
{
    return m_currentValue;
}

}
