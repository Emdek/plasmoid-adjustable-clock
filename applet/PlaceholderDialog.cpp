/***********************************************************************************
* Adjustable Clock: Plasmoid to show date and time in adjustable format.
* Copyright (C) 2008 - 2012 Michal Dutkiewicz aka Emdek <emdeck@gmail.com>
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
#include "Applet.h"

namespace AdjustableClock
{

PlaceholderDialog::PlaceholderDialog(QWidget *parent) : KDialog(parent)
{
    m_placeholderUi.setupUi(mainWidget());

    setButtons(KDialog::Cancel | KDialog::Ok);
    setModal(true);

    m_placeholderUi.placeholderComboBox->addItem(i18n("Second"), QVariant(QLatin1Char('s')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Minute"), QVariant(QLatin1Char('m')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Hour"), QVariant(QLatin1Char('h')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("The pm or am string"), QVariant(QLatin1Char('p')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Day of the month"), QVariant(QLatin1Char('d')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Weekday"), QVariant(QLatin1Char('w')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Day of the year"), QVariant(QLatin1Char('D')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Week"), QVariant(QLatin1Char('W')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Month"), QVariant(QLatin1Char('M')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Year"), QVariant(QLatin1Char('Y')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("UNIX timestamp"), QVariant(QLatin1Char('U')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Time"), QVariant(QLatin1Char('t')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Date"), QVariant(QLatin1Char('T')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Date and time"), QVariant(QLatin1Char('A')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezone"), QVariant(QLatin1Char('z')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Timezones list"), QVariant(QLatin1Char('Z')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Holidays list"), QVariant(QLatin1Char('H')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Events list"), QVariant(QLatin1Char('E')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Sunrise time"), QVariant(QLatin1Char('R')));
    m_placeholderUi.placeholderComboBox->addItem(i18n("Sunset time"), QVariant(QLatin1Char('S')));

    selectPlaceholder(0);
    adjustSize();
    show();

    connect(m_placeholderUi.placeholderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectPlaceholder(int)));
    connect(m_placeholderUi.timezoneModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectTimezoneMode(int)));
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
    emit insertPlaceholder(placeholder());
}

void PlaceholderDialog::updatePreview()
{
    m_placeholderUi.previewLabel->setText(Applet::evaluateFormat(placeholder(), QDateTime::currentDateTime()));
}

void PlaceholderDialog::selectPlaceholder(int index)
{
    const QChar placeholder = m_placeholderUi.placeholderComboBox->itemData(index).toChar();

    if (placeholder == QLatin1Char('p') || placeholder == QLatin1Char('z') || placeholder == QLatin1Char('Z') || placeholder == QLatin1Char('H') || placeholder == QLatin1Char('E')) {
        m_placeholderUi.textualFormCheckBox->setChecked(true);
    } else if (!(placeholder == QLatin1Char('m') || placeholder == QLatin1Char('w'))) {
        m_placeholderUi.textualFormCheckBox->setChecked(false);
    }

    m_placeholderUi.timezoneModeComboBox->setEnabled(placeholder == QLatin1Char('z'));
    m_placeholderUi.shortFormCheckBox->setEnabled(placeholder == QLatin1Char('w') || placeholder == QLatin1Char('M') || placeholder == QLatin1Char('Y') || placeholder == QLatin1Char('t') || placeholder == QLatin1Char('T') || placeholder == QLatin1Char('z') || placeholder == QLatin1Char('Z') || placeholder == QLatin1Char('H') || placeholder == QLatin1Char('E'));
    m_placeholderUi.hoursModeCheckBox->setEnabled(placeholder == QLatin1Char('h'));
    m_placeholderUi.textualFormCheckBox->setEnabled(placeholder == QLatin1Char('w') || placeholder == QLatin1Char('M'));
    m_placeholderUi.possessiveFormCheckBox->setEnabled(placeholder == QLatin1Char('M'));
    m_placeholderUi.leadingZerosCheckBox->setEnabled(placeholder == QLatin1Char('s') || placeholder == QLatin1Char('m') || placeholder == QLatin1Char('h') || placeholder == QLatin1Char('d') || placeholder == QLatin1Char('D') || placeholder == QLatin1Char('w') || placeholder == QLatin1Char('W') || placeholder == QLatin1Char('M'));
    m_placeholderUi.leadingZerosCheckBox->setChecked(m_placeholderUi.leadingZerosCheckBox->isEnabled());
    m_placeholderUi.timezoneModeLabel->setEnabled(m_placeholderUi.timezoneModeComboBox->isEnabled());
    m_placeholderUi.shortFormLabel->setEnabled(m_placeholderUi.shortFormCheckBox->isEnabled());
    m_placeholderUi.hoursModeLabel->setEnabled(m_placeholderUi.hoursModeCheckBox->isEnabled());
    m_placeholderUi.textualFormLabel->setEnabled(m_placeholderUi.textualFormCheckBox->isEnabled());
    m_placeholderUi.possessiveFormLabel->setEnabled(m_placeholderUi.possessiveFormCheckBox->isEnabled());
    m_placeholderUi.leadingZerosLabel->setEnabled(m_placeholderUi.leadingZerosCheckBox->isEnabled());

    if (!m_placeholderUi.timezoneModeComboBox->isEnabled()) {
        m_placeholderUi.timezoneModeComboBox->setCurrentIndex(0);
    }

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

void PlaceholderDialog::selectTimezoneMode(int index)
{
    m_placeholderUi.shortFormCheckBox->setEnabled(index == 0);
    m_placeholderUi.shortFormLabel->setEnabled(index == 0);

    updatePreview();
}

void PlaceholderDialog::setShortForm(bool shortForm)
{
    const QChar placeholder = m_placeholderUi.placeholderComboBox->itemData(m_placeholderUi.placeholderComboBox->currentIndex()).toChar();

    if (sender() == m_placeholderUi.leadingZerosCheckBox && (placeholder == QLatin1Char('w') || placeholder == QLatin1Char('M'))) {
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

QString PlaceholderDialog::placeholder()
{
    QString placeholder(QLatin1Char('%'));

    if (m_placeholderUi.shortFormCheckBox->isChecked() || (m_placeholderUi.leadingZerosCheckBox->isEnabled() && !m_placeholderUi.leadingZerosCheckBox->isChecked())) {
        placeholder.append(QLatin1Char('!'));
    }

    if ((m_placeholderUi.textualFormCheckBox->isEnabled() && m_placeholderUi.textualFormCheckBox->isChecked()) || (m_placeholderUi.timezoneModeComboBox->isEnabled() && m_placeholderUi.timezoneModeComboBox->currentIndex() != 2)) {
        placeholder.append(QLatin1Char('$'));
    }

    if (m_placeholderUi.textualFormCheckBox->isChecked() && m_placeholderUi.possessiveFormCheckBox->checkState() != Qt::PartiallyChecked) {
        placeholder.append(m_placeholderUi.possessiveFormCheckBox->isChecked() ? QLatin1Char('+') : QLatin1Char('-'));
    } else  if (m_placeholderUi.hoursModeCheckBox->checkState() != Qt::PartiallyChecked) {
        placeholder.append(m_placeholderUi.hoursModeCheckBox->isChecked() ? QLatin1Char('+') : QLatin1Char('-'));
    } else if (m_placeholderUi.timezoneModeComboBox->isEnabled() && m_placeholderUi.timezoneModeComboBox->currentIndex() == 1) {
        placeholder.append(QLatin1Char('+'));
    }

    placeholder.append(m_placeholderUi.placeholderComboBox->itemData(m_placeholderUi.placeholderComboBox->currentIndex(), Qt::UserRole).toString());

    return placeholder;
}

}