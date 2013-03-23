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

    setButtons(KDialog::Cancel | KDialog::Ok);
    setModal(true);

    m_componentUi.componentComboBox->addItem(i18n("Second"), QVariant(SecondComponent));
    m_componentUi.componentComboBox->addItem(i18n("Minute"), QVariant(MinuteComponent));
    m_componentUi.componentComboBox->addItem(i18n("Hour"), QVariant(HourComponent));
    m_componentUi.componentComboBox->addItem(i18n("The pm or am string"), QVariant(TimeOfDayComponent));
    m_componentUi.componentComboBox->addItem(i18n("Weekday"), QVariant(DayOfWeekComponent));
    m_componentUi.componentComboBox->addItem(i18n("Day of the month"), QVariant(DayOfMonthComponent));
    m_componentUi.componentComboBox->addItem(i18n("Day of the year"), QVariant(DayOfYearComponent));
    m_componentUi.componentComboBox->addItem(i18n("Week"), QVariant(WeekComponent));
    m_componentUi.componentComboBox->addItem(i18n("Month"), QVariant(MonthComponent));
    m_componentUi.componentComboBox->addItem(i18n("Year"), QVariant(YearComponent));
    m_componentUi.componentComboBox->addItem(i18n("UNIX timestamp"), QVariant(TimestampComponent));
    m_componentUi.componentComboBox->addItem(i18n("Time"), QVariant(TimeComponent));
    m_componentUi.componentComboBox->addItem(i18n("Date"), QVariant(DateComponent));
    m_componentUi.componentComboBox->addItem(i18n("Date and time"), QVariant(DateTimeComponent));
    m_componentUi.componentComboBox->addItem(i18n("Timezone name"), QVariant(TimeZoneNameComponent));
    m_componentUi.componentComboBox->addItem(i18n("Timezone abbreviation"), QVariant(TimeZoneAbbreviationComponent));
    m_componentUi.componentComboBox->addItem(i18n("Timezone offset"), QVariant(TimeZoneOffsetComponent));
    m_componentUi.componentComboBox->addItem(i18n("Timezones list"), QVariant(TimeZonesComponent));
    m_componentUi.componentComboBox->addItem(i18n("Holidays list"), QVariant(HolidaysComponent));
    m_componentUi.componentComboBox->addItem(i18n("Events list"), QVariant(EventsComponent));
    m_componentUi.componentComboBox->addItem(i18n("Sunrise time"), QVariant(SunriseComponent));
    m_componentUi.componentComboBox->addItem(i18n("Sunset time"), QVariant(SunsetComponent));

    selectComponent(0);
    adjustSize();
    show();

    connect(m_componentUi.componentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(m_componentUi.shortFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setShortForm(bool)));
    connect(m_componentUi.leadingZerosCheckBox, SIGNAL(toggled(bool)), this, SLOT(setShortForm(bool)));
    connect(m_componentUi.hoursModeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAlternativeForm(bool)));
    connect(m_componentUi.possessiveFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAlternativeForm(bool)));
    connect(m_componentUi.textualFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setTextualForm(bool)));
    connect(this, SIGNAL(okClicked()), this, SLOT(sendSignal()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void ComponentDialog::sendSignal()
{
    const QString scriptComponent = Clock::getComponentString(getComponent());
    QStringList scriptOptions;
    const QVariantMap options = getOptions();

    QVariantMap::const_iterator iterator;

    for (iterator = options.constBegin(); iterator != options.constEnd(); ++iterator) {
        scriptOptions.append(QString("'%1': %2").arg(iterator.key()).arg(iterator.value().toString()));
    }

    if (scriptOptions.isEmpty()) {
        emit insertComponent(QString("Clock.").append(scriptComponent), getComponent());
    } else {
        emit insertComponent(QString("Clock.%1, {%2}").arg(scriptComponent).arg(scriptOptions.join(QString(", "))), getComponent());
    }
}

void ComponentDialog::updatePreview()
{
    m_componentUi.previewLabel->setText(m_clock->toString(getComponent(), getOptions()));
}

void ComponentDialog::selectComponent(int index)
{
    const ClockComponent component = static_cast<ClockComponent>(m_componentUi.componentComboBox->itemData(index).toInt());

    if (component == TimeOfDayComponent || component == TimeZoneNameComponent || component == TimeZoneAbbreviationComponent || component == TimeZoneOffsetComponent || component == TimeZonesComponent || component == HolidaysComponent || component == EventsComponent) {
        m_componentUi.textualFormCheckBox->setChecked(true);
    } else if (!(component == MinuteComponent || component == DayOfWeekComponent)) {
        m_componentUi.textualFormCheckBox->setChecked(false);
    }

    m_componentUi.shortFormCheckBox->setEnabled(component == DayOfWeekComponent || component == MonthComponent || component == YearComponent || component == TimeComponent || component == DateComponent || component == TimeZoneNameComponent || component == TimeZonesComponent || component == HolidaysComponent || component == EventsComponent);
    m_componentUi.hoursModeCheckBox->setEnabled(component == HourComponent);
    m_componentUi.textualFormCheckBox->setEnabled(component == DayOfWeekComponent || component == MonthComponent);
    m_componentUi.possessiveFormCheckBox->setEnabled(component == MonthComponent);
    m_componentUi.leadingZerosCheckBox->setEnabled(component == SecondComponent || component == MinuteComponent || component == HourComponent || component == DayOfMonthComponent || component == DayOfYearComponent || component == DayOfWeekComponent || component == WeekComponent || component == MonthComponent);
    m_componentUi.leadingZerosCheckBox->setChecked(m_componentUi.leadingZerosCheckBox->isEnabled());
    m_componentUi.shortFormLabel->setEnabled(m_componentUi.shortFormCheckBox->isEnabled());
    m_componentUi.hoursModeLabel->setEnabled(m_componentUi.hoursModeCheckBox->isEnabled());
    m_componentUi.textualFormLabel->setEnabled(m_componentUi.textualFormCheckBox->isEnabled());
    m_componentUi.possessiveFormLabel->setEnabled(m_componentUi.possessiveFormCheckBox->isEnabled());
    m_componentUi.leadingZerosLabel->setEnabled(m_componentUi.leadingZerosCheckBox->isEnabled());

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

void ComponentDialog::setShortForm(bool shortForm)
{
    const ClockComponent component = static_cast<ClockComponent>(m_componentUi.componentComboBox->itemData(m_componentUi.componentComboBox->currentIndex()).toInt());

    if (sender() == m_componentUi.leadingZerosCheckBox && (component == DayOfWeekComponent || component == MonthComponent)) {
        m_componentUi.textualFormCheckBox->setEnabled(!shortForm);
        m_componentUi.textualFormLabel->setEnabled(!shortForm);
    }

    updatePreview();
}

void ComponentDialog::setTextualForm(bool textualForm)
{
    m_componentUi.leadingZerosCheckBox->setEnabled(!textualForm);
    m_componentUi.leadingZerosLabel->setEnabled(!textualForm);

    updatePreview();
}

void ComponentDialog::setAlternativeForm(bool alternativeForm)
{
    Q_UNUSED(alternativeForm)

    updatePreview();
}

ClockComponent ComponentDialog::getComponent() const
{
    return static_cast<ClockComponent>(m_componentUi.componentComboBox->itemData(m_componentUi.componentComboBox->currentIndex()).toInt());
}

QVariantMap ComponentDialog::getOptions() const
{
    QVariantMap options;

    if (m_componentUi.shortFormCheckBox->isChecked() || (m_componentUi.leadingZerosCheckBox->isEnabled() && !m_componentUi.leadingZerosCheckBox->isChecked())) {
        options["short"] = true;
    }

    if (m_componentUi.textualFormCheckBox->isEnabled() && m_componentUi.textualFormCheckBox->isChecked()) {
        options["text"] = true;
    }

    if (m_componentUi.textualFormCheckBox->isChecked() && m_componentUi.possessiveFormCheckBox->checkState() != Qt::PartiallyChecked) {
        options["possessive"] = m_componentUi.possessiveFormCheckBox->isChecked();
    } else  if (m_componentUi.hoursModeCheckBox->checkState() != Qt::PartiallyChecked) {
        options["alternative"] = m_componentUi.hoursModeCheckBox->isChecked();
    }

    return options;
}

}