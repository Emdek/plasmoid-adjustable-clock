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

#ifndef ADJUSTABLECLOCK_OPTIONWIDGET_HEADER
#define ADJUSTABLECLOCK_OPTIONWIDGET_HEADER

#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QPlainTextEdit>

#include <KColorButton>
#include <KFontComboBox>
#include <KConfigSkeletonItem>

namespace AdjustableClock
{

class OptionWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit OptionWidget(KConfigSkeletonItem *option, QWidget *parent);

        void setFocus(Qt::FocusReason reason);
        QWidget *getWidget();
        QVariant getValue() const;

    protected slots:
        void updateValue();

    private:
        QWidget *m_widget;
        KColorButton *m_colorButton;
        QComboBox *m_comboBox;
        KFontComboBox *m_fontComboBox;
        QCheckBox *m_checkBox;
        QSlider *m_slider;
        QSpinBox *m_spinBox;
        QPlainTextEdit *m_textEdit;
        KConfigSkeletonItem *m_option;
        QVariant m_value;
};

}

#endif
