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

#include <QtGui/QBoxLayout>

namespace AdjustableClock
{

OptionWidget::OptionWidget(KConfigSkeletonItem *option, QWidget *parent) : QWidget(parent),
    m_widget(NULL),
    m_colorButton(NULL),
    m_comboBox(NULL),
    m_checkBox(NULL),
    m_spinBox(NULL),
    m_textEdit(NULL),
    m_option(option),
    m_value(option->property())
{
    setFocusPolicy(Qt::StrongFocus);

    KCoreConfigSkeleton::ItemEnum *enumOption = dynamic_cast<KCoreConfigSkeleton::ItemEnum*>(m_option);

    if (enumOption) {
        m_widget = m_comboBox = new QComboBox(this);

        for (int i = 0; i < enumOption->choices().count(); ++i) {
            m_comboBox->addItem((enumOption->choices().at(i).label.isEmpty() ? enumOption->choices().at(i).name : enumOption->choices().at(i).label), enumOption->choices().at(i).name);

            if (i == m_option->property().toInt()) {
                m_comboBox->setCurrentIndex(i);
            }
        }

        connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateValue()));
    } else {
        switch (m_option->property().type()) {
        case QVariant::Bool:
            m_widget = m_checkBox = new QCheckBox(this);
            m_checkBox->setChecked(m_option->property().toBool());

            connect(m_checkBox, SIGNAL(toggled(bool)), this, SLOT(updateValue()));

            break;
        case QVariant::Int:
            m_widget = m_spinBox = new QSpinBox(this);

            if (m_option->minValue().toInt() != m_option->maxValue().toInt()) {
                m_spinBox->setRange(m_option->minValue().toInt(), m_option->maxValue().toInt());
            } else {
                m_spinBox->setRange(-9999, 9999);
            }

            m_spinBox->setValue(m_option->property().toInt());

            connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateValue()));

            break;
        case QVariant::Color:
            m_widget = m_colorButton = new KColorButton(m_option->property().value<QColor>(), this);

            connect(m_colorButton, SIGNAL(changed(QColor)), this, SLOT(updateValue()));

            break;
        default:
            m_widget = m_textEdit = new QPlainTextEdit(this);
            m_textEdit->setPlainText(m_option->property().toString());

            connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(updateValue()));

            break;
        }
    }

    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->addWidget(m_widget);
}

void OptionWidget::updateValue()
{
    if (!m_widget) {
        return;
    }

    if (m_comboBox) {
        m_value = m_comboBox->currentIndex();

        return;
    }

    switch (m_option->property().type()) {
    case QVariant::Bool:
        m_value = QVariant(m_checkBox->isChecked());

        break;
    case QVariant::Int:
        m_value = QVariant(m_spinBox->value());

        break;
    case QVariant::Color:
        m_value = QVariant(m_colorButton->color());

        break;
    default:
        m_value = QVariant(m_textEdit->toPlainText());

        break;
    }
}

void OptionWidget::setFocus(Qt::FocusReason reason)
{
    if (m_widget) {
        m_widget->setFocus(reason);
    } else {
        QWidget::setFocus(reason);
    }
}

QWidget *OptionWidget::getWidget()
{
    return m_widget;
}

QVariant OptionWidget::getValue() const
{
    return m_value;
}

}
