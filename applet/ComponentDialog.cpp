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

#include "ComponentDialog.h"

namespace AdjustableClock
{

ComponentDialog::ComponentDialog(Clock *clock, QWidget *parent) : KDialog(parent),
    m_clock(clock)
{
    m_componentUi.setupUi(mainWidget());

    for (int i = 0; i < LastComponent; ++i) {
        m_componentUi.componentComboBox->addItem(Clock::getComponentName(static_cast<ClockComponent>(i)), i);
    }

    setButtons(KDialog::Cancel | KDialog::Ok);
    setModal(true);
    selectComponent(0);
    adjustSize();
    show();

    connect(m_componentUi.componentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(m_componentUi.shortFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(updatePreview()));
    connect(m_componentUi.hoursModeCheckBox, SIGNAL(toggled(bool)), this, SLOT(updatePreview()));
    connect(m_componentUi.possessiveFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(updatePreview()));
    connect(m_componentUi.textualFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(updatePreview()));
    connect(this, SIGNAL(okClicked()), this, SLOT(sendSignal()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void ComponentDialog::sendSignal()
{
    const QVariantMap options = getOptions();
    QStringList scriptOptions;
    QVariantMap::const_iterator iterator;

    for (iterator = options.constBegin(); iterator != options.constEnd(); ++iterator) {
        scriptOptions.append(QString("'%1': %2").arg(iterator.key()).arg(iterator.value().toString()));
    }

    emit insertComponent(Clock::getComponentString(getComponent()), (scriptOptions.isEmpty() ? QString() : scriptOptions.join(QString(", "))));
}

void ComponentDialog::selectComponent(int index)
{
    const ClockComponent component = static_cast<ClockComponent>(m_componentUi.componentComboBox->itemData(index).toInt());

    if (component == TimeOfDayComponent || component == TimeZoneNameComponent || component == TimeZoneAbbreviationComponent || component == TimeZoneOffsetComponent || component == TimeZonesComponent || component == HolidaysComponent || component == EventsComponent) {
        m_componentUi.textualFormCheckBox->setChecked(true);
    } else if (!(component == MinuteComponent || component == DayOfWeekComponent)) {
        m_componentUi.textualFormCheckBox->setChecked(false);
    }

    m_componentUi.shortFormCheckBox->setEnabled(component == SecondComponent || component == MinuteComponent || component == HourComponent || component == DayOfWeekComponent || component == DayOfMonthComponent || component == DayOfYearComponent || component == WeekComponent || component == MonthComponent || component == YearComponent || component == TimeComponent || component == DateComponent || component == HolidaysComponent || component == EventsComponent);
    m_componentUi.hoursModeCheckBox->setEnabled(component == HourComponent);
    m_componentUi.textualFormCheckBox->setEnabled(component == DayOfWeekComponent || component == MonthComponent);
    m_componentUi.possessiveFormCheckBox->setEnabled(component == MonthComponent);
    m_componentUi.shortFormLabel->setEnabled(m_componentUi.shortFormCheckBox->isEnabled());
    m_componentUi.hoursModeLabel->setEnabled(m_componentUi.hoursModeCheckBox->isEnabled());
    m_componentUi.textualFormLabel->setEnabled(m_componentUi.textualFormCheckBox->isEnabled());
    m_componentUi.possessiveFormLabel->setEnabled(m_componentUi.possessiveFormCheckBox->isEnabled());

    if (!m_componentUi.shortFormCheckBox->isEnabled()) {
        m_componentUi.shortFormCheckBox->setChecked(false);
    }

    if (!m_componentUi.hoursModeCheckBox->isEnabled()) {
        m_componentUi.hoursModeCheckBox->setCheckState(Qt::PartiallyChecked);
    }

    if (!m_componentUi.possessiveFormCheckBox->isEnabled()) {
        m_componentUi.possessiveFormCheckBox->setCheckState(Qt::PartiallyChecked);
    }

    updatePreview();
}

void ComponentDialog::updatePreview()
{
    m_componentUi.previewLabel->setText(m_clock->toString(getComponent(), getOptions()));
}

ClockComponent ComponentDialog::getComponent() const
{
    return static_cast<ClockComponent>(m_componentUi.componentComboBox->itemData(m_componentUi.componentComboBox->currentIndex()).toInt());
}

QVariantMap ComponentDialog::getOptions() const
{
    QVariantMap options;

    if (m_componentUi.shortFormCheckBox->isEnabled() && m_componentUi.shortFormCheckBox->isChecked()) {
        options["short"] = true;
    }

    if (m_componentUi.hoursModeCheckBox->isEnabled() && m_componentUi.hoursModeCheckBox->checkState() != Qt::PartiallyChecked) {
        options["alternative"] = m_componentUi.hoursModeCheckBox->isChecked();
    }

    if (m_componentUi.textualFormCheckBox->isEnabled() && m_componentUi.textualFormCheckBox->isChecked()) {
        options["text"] = true;

        if (m_componentUi.possessiveFormCheckBox->isEnabled() && m_componentUi.possessiveFormCheckBox->checkState() != Qt::PartiallyChecked) {
            options["possessive"] = m_componentUi.possessiveFormCheckBox->isChecked();
        }
    }

    return options;
}

}