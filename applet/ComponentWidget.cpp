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

#include "ComponentWidget.h"

#include <QtGui/QCheckBox>
#include <QtGui/QWidgetAction>

#include <KMenu>
#include <KLocale>

namespace AdjustableClock
{

ComponentWidget::ComponentWidget(Clock *clock, QWidget *parent) : QWidget(parent),
    m_clock(clock),
    m_component(InvalidComponent)
{
    m_componentUi.setupUi(this);

    KMenu *menu = new KMenu(m_componentUi.componentButton);

    for (int i = 1; i < LastComponent; ++i) {
        QAction *action = new QAction(Clock::getComponentName(static_cast<ClockComponent>(i)), this);
        action->setData(i);

        menu->addAction(action);
    }

    m_componentUi.componentButton->setMenu(menu);
    m_componentUi.optionsButton->setMenu(new KMenu(m_componentUi.optionsButton));

    if (!parent) {
        m_componentUi.insertButton->hide();
        m_componentUi.verticalLayout->addWidget(m_componentUi.previewLabel);
    }

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(selectComponent(QAction*)));
    connect(m_componentUi.insertButton, SIGNAL(clicked()), this, SLOT(insertComponent()));
}

void ComponentWidget::insertComponent()
{
    QStringList options;
    QVariantMap::iterator iterator;

    for (iterator = m_options.begin(); iterator != m_options.end(); ++iterator) {
        options.append(QString("'%1': %2").arg(iterator.key()).arg(iterator.value().toString()));
    }

    emit insertComponent(Clock::getComponentString(m_component), (options.isEmpty() ? QString() : options.join(QString(", "))));
}

void ComponentWidget::addOption(QWidget *widget)
{
    QWidgetAction *action = new QWidgetAction(widget);
    action->setDefaultWidget(widget);

    m_componentUi.optionsButton->menu()->addAction(action);
}

void ComponentWidget::selectComponent(QAction *action)
{
    if (m_component == InvalidComponent) {
        m_componentUi.componentButton->setMinimumWidth(m_componentUi.componentButton->menu()->width());
        m_componentUi.insertButton->setEnabled(true);

        emit componentChanged(true);
    }

    m_component = static_cast<ClockComponent>(action->data().toInt());

    m_options.clear();

    m_componentUi.componentButton->setText(action->text());
    m_componentUi.optionsButton->menu()->clear();

    if (m_component == SecondComponent || m_component == MinuteComponent || m_component == HourComponent || m_component == DayOfWeekComponent || m_component == DayOfMonthComponent || m_component == DayOfYearComponent || m_component == WeekComponent || m_component == MonthComponent || m_component == YearComponent || m_component == TimeComponent || m_component == DateComponent || m_component == HolidaysComponent || m_component == EventsComponent) {
        QCheckBox *checkBox = new QCheckBox(i18n("Short Form"), m_componentUi.optionsButton->menu());

        addOption(checkBox);

        connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(setShortForm(bool)));
    }

    if (m_component == HourComponent) {
        QCheckBox *checkBox = new QCheckBox(i18n("12 Hour Mode"), m_componentUi.optionsButton->menu());
        checkBox->setTristate(true);
        checkBox->setCheckState(Qt::PartiallyChecked);

        addOption(checkBox);

        connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(setAlternativeForm(int)));
    }

    if (m_component == DayOfWeekComponent || m_component == MonthComponent) {
        QCheckBox *checkBox = new QCheckBox(i18n("Textual Form"), m_componentUi.optionsButton->menu());

        addOption(checkBox);

        connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(setTextualForm(bool)));
    }

    if (m_component == MonthComponent) {
        QCheckBox *checkBox = new QCheckBox(i18n("Possessive Form"), m_componentUi.optionsButton->menu());
        checkBox->setTristate(true);
        checkBox->setCheckState(Qt::PartiallyChecked);

        addOption(checkBox);

        connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(setPossessiveForm(int)));
    }

    m_componentUi.optionsButton->setEnabled(m_componentUi.optionsButton->menu()->actions().count() > 0);

    updatePreview();
}

void ComponentWidget::updatePreview()
{
    const QString preview = m_clock->toString(m_component, m_options, true);

    m_componentUi.previewLabel->setText(preview);
    m_componentUi.previewLabel->setToolTip(preview);
}

void ComponentWidget::setShortForm(bool form)
{
    if (form) {
        m_options["short"] = true;
    } else if (m_options.contains("short")) {
        m_options.remove("short");
    }

    updatePreview();
}

void ComponentWidget::setAlternativeForm(int form)
{
    if (form != Qt::PartiallyChecked) {
        m_options["alternative"] = (form == Qt::Checked);
    } else if (m_options.contains("alternative")) {
        m_options.remove("alternative");
    }

    updatePreview();
}

void ComponentWidget::setTextualForm(bool form)
{
    if (form) {
        m_options["text"] = true;
    } else if (m_options.contains("text")) {
        m_options.remove("text");
    }

    updatePreview();
}

void ComponentWidget::setPossessiveForm(int form)
{
    if (form != Qt::PartiallyChecked) {
        m_options["possessive"] = (form == Qt::Checked);
    } else if (m_options.contains("possessive")) {
        m_options.remove("possessive");
    }

    updatePreview();
}

}