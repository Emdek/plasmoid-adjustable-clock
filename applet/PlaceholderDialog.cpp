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

#include "PlaceholderDialog.h"
#include "Clock.h"

namespace AdjustableClock
{

PlaceholderDialog::PlaceholderDialog(Clock *clock, QWidget *parent) : KDialog(parent),
    m_clock(clock)
{
    m_placeholderUi.setupUi(mainWidget());

    setButtons(KDialog::Cancel | KDialog::Ok);
    setModal(true);

    m_placeholderUi.placeholderComboBox->addItem(i18n("Second"), QVariant(SecondValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Minute"), QVariant(MinuteValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Hour"), QVariant(HourValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("The pm or am string"), QVariant(TimeOfDayValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Weekday"), QVariant(DayOfWeekValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Day of the month"), QVariant(DayOfMonthValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Day of the year"), QVariant(DayOfYearValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Week"), QVariant(WeekValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Month"), QVariant(MonthValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Year"), QVariant(YearValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("UNIX timestamp"), QVariant(TimestampValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Time"), QVariant(TimeValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Date"), QVariant(DateValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Date and time"), QVariant(DateTimeValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezone name"), QVariant(TimeZoneNameValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezone abbreviation"), QVariant(TimeZoneAbbreviationValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezone offset"), QVariant(TimeZoneOffsetValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezones list"), QVariant(TimeZonesValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Holidays list"), QVariant(HolidaysValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Events list"), QVariant(EventsValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Sunrise time"), QVariant(SunriseValue));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Sunset time"), QVariant(SunsetValue));

    selectPlaceholder(0);
    adjustSize();
    show();

    connect(m_placeholderUi.placeholderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectPlaceholder(int)));
    connect(m_placeholderUi.shortFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setShortForm(bool)));
    connect(m_placeholderUi.leadingZerosCheckBox, SIGNAL(toggled(bool)), this, SLOT(setShortForm(bool)));
    connect(m_placeholderUi.hoursModeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAlternativeForm(bool)));
    connect(m_placeholderUi.possessiveFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAlternativeForm(bool)));
    connect(m_placeholderUi.textualFormCheckBox, SIGNAL(toggled(bool)), this, SLOT(setTextualForm(bool)));
    connect(this, SIGNAL(okClicked()), this, SLOT(sendSignal()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void PlaceholderDialog::sendSignal()
{
    QString scriptValue;
    QStringList scriptOptions;
    const QVariantMap options = getOptions();

    switch (getPlaceholder()) {
    case SecondValue:
        scriptValue = "Second";

        break;
    case MinuteValue:
        scriptValue = "Minute";

        break;
    case HourValue:
        scriptValue = "Hour";

        break;
    case TimeOfDayValue:
        scriptValue = "TimeOfDay";

        break;
    case DayOfMonthValue:
        scriptValue = "DayOfMonth";

        break;
    case DayOfWeekValue:
        scriptValue = "DayOfWeek";

        break;
    case DayOfYearValue:
        scriptValue = "DayOfYear";

        break;
    case WeekValue:
        scriptValue = "Week";

        break;
    case MonthValue:
        scriptValue = "Month";

        break;
    case YearValue:
        scriptValue = "Year";

        break;
    case TimestampValue:
        scriptValue = "Timestamp";

        break;
    case TimeValue:
        scriptValue = "Time";

        break;
    case DateValue:
        scriptValue = "Date";

        break;
    case DateTimeValue:
        scriptValue = "DateTime";

        break;
    case TimeZoneNameValue:
        scriptValue = "TimeZoneName";

        break;
    case TimeZoneAbbreviationValue:
        scriptValue = "TimeZoneAbbreviation";

        break;
    case TimeZoneOffsetValue:
        scriptValue = "TimeZoneOffset";

        break;
    case TimeZonesValue:
        scriptValue = "TimeZones";

        break;
    case EventsValue:
        scriptValue = "Events";

        break;
    case HolidaysValue:
        scriptValue = "Holidays";

        break;
    case SunriseValue:
        scriptValue = "Sunrise";

        break;
    case SunsetValue:
        scriptValue = "Sunset";

        break;
    default:
        scriptValue = QString();

        break;
    }

    QVariantMap::const_iterator iterator;

    for (iterator = options.constBegin(); iterator != options.constEnd(); ++iterator) {
        scriptOptions.append(QString("'%1': %2").arg(iterator.key()).arg(iterator.value().toString()));
    }

    if (scriptOptions.isEmpty()) {
        emit insertPlaceholder(QString("Clock.").append(scriptValue));
    } else {
        emit insertPlaceholder(QString("Clock.%1, {%2}").arg(scriptValue).arg(scriptOptions.join(QString(", "))));
    }
}

void PlaceholderDialog::updatePreview()
{
    m_placeholderUi.previewLabel->setText(m_clock->toString(getPlaceholder(), getOptions()));
}

void PlaceholderDialog::selectPlaceholder(int index)
{
    const ClockTimeValue placeholder = static_cast<ClockTimeValue>(m_placeholderUi.placeholderComboBox->itemData(index).toInt());

    if (placeholder == TimeOfDayValue || placeholder == TimeZoneNameValue || placeholder == TimeZoneAbbreviationValue || placeholder == TimeZoneOffsetValue || placeholder == TimeZonesValue || placeholder == HolidaysValue || placeholder == EventsValue) {
        m_placeholderUi.textualFormCheckBox->setChecked(true);
    } else if (!(placeholder == MinuteValue || placeholder == DayOfWeekValue)) {
        m_placeholderUi.textualFormCheckBox->setChecked(false);
    }

    m_placeholderUi.shortFormCheckBox->setEnabled(placeholder == DayOfWeekValue || placeholder == MonthValue || placeholder == YearValue || placeholder == TimeValue || placeholder == DateValue || placeholder == TimeZoneNameValue || placeholder == TimeZonesValue || placeholder == HolidaysValue || placeholder == EventsValue);
    m_placeholderUi.hoursModeCheckBox->setEnabled(placeholder == HourValue);
    m_placeholderUi.textualFormCheckBox->setEnabled(placeholder == DayOfWeekValue || placeholder == MonthValue);
    m_placeholderUi.possessiveFormCheckBox->setEnabled(placeholder == MonthValue);
    m_placeholderUi.leadingZerosCheckBox->setEnabled(placeholder == SecondValue || placeholder == MinuteValue || placeholder == HourValue || placeholder == DayOfMonthValue || placeholder == DayOfYearValue || placeholder == DayOfWeekValue || placeholder == WeekValue || placeholder == MonthValue);
    m_placeholderUi.leadingZerosCheckBox->setChecked(m_placeholderUi.leadingZerosCheckBox->isEnabled());
    m_placeholderUi.shortFormLabel->setEnabled(m_placeholderUi.shortFormCheckBox->isEnabled());
    m_placeholderUi.hoursModeLabel->setEnabled(m_placeholderUi.hoursModeCheckBox->isEnabled());
    m_placeholderUi.textualFormLabel->setEnabled(m_placeholderUi.textualFormCheckBox->isEnabled());
    m_placeholderUi.possessiveFormLabel->setEnabled(m_placeholderUi.possessiveFormCheckBox->isEnabled());
    m_placeholderUi.leadingZerosLabel->setEnabled(m_placeholderUi.leadingZerosCheckBox->isEnabled());

    if (!m_placeholderUi.shortFormCheckBox->isEnabled()) {
        m_placeholderUi.shortFormCheckBox->setChecked(false);
    }

    if (!m_placeholderUi.hoursModeCheckBox->isEnabled()) {
        m_placeholderUi.hoursModeCheckBox->setCheckState(Qt::PartiallyChecked);
    }

    if (!m_placeholderUi.possessiveFormCheckBox->isEnabled()) {
        m_placeholderUi.possessiveFormCheckBox->setCheckState(Qt::PartiallyChecked);
    }

    updatePreview();
}

void PlaceholderDialog::setShortForm(bool shortForm)
{
    const ClockTimeValue placeholder = static_cast<ClockTimeValue>(m_placeholderUi.placeholderComboBox->itemData(m_placeholderUi.placeholderComboBox->currentIndex()).toInt());

    if (sender() == m_placeholderUi.leadingZerosCheckBox && (placeholder == DayOfWeekValue || placeholder == MonthValue)) {
        m_placeholderUi.textualFormCheckBox->setEnabled(!shortForm);
        m_placeholderUi.textualFormLabel->setEnabled(!shortForm);
    }

    updatePreview();
}

void PlaceholderDialog::setTextualForm(bool textualForm)
{
    m_placeholderUi.leadingZerosCheckBox->setEnabled(!textualForm);
    m_placeholderUi.leadingZerosLabel->setEnabled(!textualForm);

    updatePreview();
}

void PlaceholderDialog::setAlternativeForm(bool alternativeForm)
{
    Q_UNUSED(alternativeForm)

    updatePreview();
}

ClockTimeValue PlaceholderDialog::getPlaceholder() const
{
    return static_cast<ClockTimeValue>(m_placeholderUi.placeholderComboBox->itemData(m_placeholderUi.placeholderComboBox->currentIndex()).toInt());
}

QVariantMap PlaceholderDialog::getOptions() const
{
    QVariantMap options;

    if (m_placeholderUi.shortFormCheckBox->isChecked() || (m_placeholderUi.leadingZerosCheckBox->isEnabled() && !m_placeholderUi.leadingZerosCheckBox->isChecked())) {
        options["short"] = true;
    }

    if (m_placeholderUi.textualFormCheckBox->isEnabled() && m_placeholderUi.textualFormCheckBox->isChecked()) {
        options["text"] = true;
    }

    if (m_placeholderUi.textualFormCheckBox->isChecked() && m_placeholderUi.possessiveFormCheckBox->checkState() != Qt::PartiallyChecked) {
        options["possessive"] = m_placeholderUi.possessiveFormCheckBox->isChecked();
    } else  if (m_placeholderUi.hoursModeCheckBox->checkState() != Qt::PartiallyChecked) {
        options["alternative"] = m_placeholderUi.hoursModeCheckBox->isChecked();
    }

    return options;
}

}