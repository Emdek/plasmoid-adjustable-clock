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

#include "Configuration.h"
#include "Applet.h"
#include "FormatDelegate.h"
#include "FormatLineEdit.h"
#include "PlaceholderDialog.h"

#include <QtWebKit/QWebFrame>

#include <KLocale>
#include <KMessageBox>
#include <KColorDialog>
#include <KInputDialog>

namespace AdjustableClock
{

Configuration::Configuration(Applet *applet, KConfigDialog *parent) : QObject(parent),
    m_applet(applet),
    m_editedItem(NULL),
    m_controlsTimer(0)
{
    QWidget *appearanceConfiguration = new QWidget;
    QWidget *clipboardActions = new QWidget;
    const QStringList clipboardFormats = m_applet->clipboardFormats();
    QString preview;
    int row;

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardActions);

    const QStringList formats = m_applet->formats();

    for (int i = 0; i < formats.count(); ++i) {
        if (formats.at(i).isEmpty()) {
            m_appearanceUi.formatComboBox->insertSeparator(i);
        } else {
            Format format = m_applet->format(formats.at(i));

            m_appearanceUi.formatComboBox->addItem(format.title, formats.at(i));
            m_appearanceUi.formatComboBox->setItemData(i, format.html, (Qt::UserRole + 1));
            m_appearanceUi.formatComboBox->setItemData(i, format.css, (Qt::UserRole + 2));
            m_appearanceUi.formatComboBox->setItemData(i, format.background, (Qt::UserRole + 3));
        }
    }

    QPalette webViewPalette = m_appearanceUi.webView->page()->palette();
    webViewPalette.setBrush(QPalette::Base, Qt::transparent);

    m_appearanceUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_appearanceUi.webView->page()->setPalette(webViewPalette);
    m_appearanceUi.webView->page()->setContentEditable(true);
    m_appearanceUi.addButton->setIcon(KIcon(QLatin1String("list-add")));
    m_appearanceUi.removeButton->setIcon(KIcon(QLatin1String("list-remove")));
    m_appearanceUi.placeholdersButton->setIcon(KIcon(QLatin1String("chronometer")));
    m_appearanceUi.boldButton->setIcon(KIcon(QLatin1String("format-text-bold")));
    m_appearanceUi.italicButton->setIcon(KIcon(QLatin1String("format-text-italic")));
    m_appearanceUi.underlineButton->setIcon(KIcon(QLatin1String("format-text-underline")));
    m_appearanceUi.justifyLeftButton->setIcon(KIcon(QLatin1String("format-justify-left")));
    m_appearanceUi.justifyCenterButton->setIcon(KIcon(QLatin1String("format-justify-center")));
    m_appearanceUi.justifyRightButton->setIcon(KIcon(QLatin1String("format-justify-right")));
    m_appearanceUi.backgroundButton->setIcon(KIcon(QLatin1String("games-config-background")));

    m_clipboardUi.moveUpButton->setIcon(KIcon(QLatin1String("arrow-up")));
    m_clipboardUi.moveDownButton->setIcon(KIcon(QLatin1String("arrow-down")));
    m_clipboardUi.clipboardActionsTable->setItemDelegate(new FormatDelegate(this));
    m_clipboardUi.clipboardActionsTable->viewport()->installEventFilter(this);
    m_clipboardUi.fastCopyFormatEdit->setText(m_applet->config().readEntry("fastCopyFormat", "%Y-%M-%d %h:%m:%s"));

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        row = m_clipboardUi.clipboardActionsTable->rowCount();

        m_clipboardUi.clipboardActionsTable->insertRow(row);
        m_clipboardUi.clipboardActionsTable->setItem(row, 0, new QTableWidgetItem(clipboardFormats.at(i)));

        preview = m_applet->evaluateFormat(clipboardFormats.at(i), QDateTime::currentDateTime());

        QTableWidgetItem *item = new QTableWidgetItem(preview);
        item->setFlags(Qt::ItemIsSelectable);
        item->setToolTip(preview);

        m_clipboardUi.clipboardActionsTable->setItem(row, 1, item);
    }

    updateControls();
    itemSelectionChanged();

    QPalette buttonPalette = m_appearanceUi.colorButton->palette();
    buttonPalette.setBrush(QPalette::Button, Qt::black);

    m_appearanceUi.colorButton->setPalette(buttonPalette);

    parent->addPage(appearanceConfiguration, i18n("Appearance"), QLatin1String("preferences-desktop-theme"));
    parent->addPage(clipboardActions, i18n("Clipboard actions"), QLatin1String("edit-copy"));
    parent->resize(500, 400);

    connect(parent, SIGNAL(okClicked()), this, SLOT(accepted()));
    connect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
    connect(m_appearanceUi.addButton, SIGNAL(clicked()), this, SLOT(addFormat()));
    connect(m_appearanceUi.removeButton, SIGNAL(clicked()), this, SLOT(removeFormat()));
    connect(m_appearanceUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.boldButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.italicButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.underlineButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(selectFontFamily(QFont)));
    connect(m_appearanceUi.placeholdersButton, SIGNAL(clicked()), this, SLOT(insertPlaceholder()));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertRow()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteRow()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveRowUp()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveRowDown()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(cellChanged(int,int)), this, SLOT(updateRow(int)));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(editRow(QTableWidgetItem*)));

    const int currentFormat = m_appearanceUi.formatComboBox->findData(m_applet->config().readEntry("format", "%default%"));

    m_appearanceUi.formatComboBox->setCurrentIndex(currentFormat);

    loadFormat(currentFormat);
}

void Configuration::timerEvent(QTimerEvent *event)
{
    updateControls();

    killTimer(event->timerId());
}

void Configuration::accepted()
{
    QStringList clipboardFormats;

    killTimer(m_controlsTimer);

    if (m_editedItem) {
        m_clipboardUi.clipboardActionsTable->closePersistentEditor(m_editedItem);
    }

    m_applet->config().deleteGroup("Formats");

    KConfigGroup formatsConfiguration = m_applet->config().group("Formats");
    const int builInFormats = m_applet->formats(false).count();

    for (int i = 0; i < m_appearanceUi.formatComboBox->count(); ++i) {
        if (m_appearanceUi.formatComboBox->itemText(i).isEmpty()) {
            continue;
        }

        Format format;
        format.title = m_appearanceUi.formatComboBox->itemText(i);
        format.html = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 1)).toString();
        format.css = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 2)).toString();
        format.background = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 3)).toBool();

        if (i < builInFormats) {
            Format existing = m_applet->format(m_appearanceUi.formatComboBox->itemData(i).toString());

            if (format.html == existing.html && format.css == existing.css) {
                continue;
            }
        }

        KConfigGroup formatConfiguration = formatsConfiguration.group(m_appearanceUi.formatComboBox->itemData(i).toString());
        formatConfiguration.writeEntry("title", format.title);
        formatConfiguration.writeEntry("html", format.html);
        formatConfiguration.writeEntry("css", format.css);
        formatConfiguration.writeEntry("background", format.background);
    }

    for (int i = 0; i < m_clipboardUi.clipboardActionsTable->rowCount(); ++i) {
        clipboardFormats.append(m_clipboardUi.clipboardActionsTable->item(i, 0)->text());
    }

    m_applet->config().writeEntry("format", m_appearanceUi.formatComboBox->itemData(m_appearanceUi.formatComboBox->currentIndex()).toString());
    m_applet->config().writeEntry("clipboardFormats", clipboardFormats);
    m_applet->config().writeEntry("fastCopyFormat", m_clipboardUi.fastCopyFormatEdit->text());
}

void Configuration::insertPlaceholder()
{
    connect(new PlaceholderDialog(m_appearanceUi.placeholdersButton), SIGNAL(insertPlaceholder(QString)), this, SLOT(insertPlaceholder(QString)));
}

void Configuration::insertPlaceholder(const QString &placeholder)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        m_appearanceUi.htmlTextEdit->insertPlainText(placeholder);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('inserthtml', false, '") + placeholder + QLatin1String("')"));
    }
}

void Configuration::loadFormat(int index)
{
    disconnect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    m_appearanceUi.htmlTextEdit->setPlainText(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 1)).toString());
    m_appearanceUi.cssTextEdit->setPlainText(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 2)).toString());
    m_appearanceUi.backgroundButton->setChecked(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 3)).toBool());
    m_appearanceUi.removeButton->setEnabled(index >= m_applet->formats(false).count());

    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    changeFormat();
}

void Configuration::changeFormat()
{
    Format format;
    format.background = m_appearanceUi.backgroundButton->isChecked();

    if (sender() == m_appearanceUi.webView->page()) {
        QRegExp fontSize = QRegExp(QLatin1String(" class=\"Apple-style-span\""));
        QRegExp fontColor = QRegExp(QLatin1String("<font color=\"(#?[\\w\\s]+)\">(.+)</font>"));
        fontColor.setMinimal(true);

        QRegExp fontFamily = QRegExp(QLatin1String("<font face=\"'?([\\w\\s]+)'?\">(.+)</font>"));
        fontFamily.setMinimal(true);

        QString html = m_appearanceUi.webView->page()->mainFrame()->toHtml().remove(QLatin1String("<style type=\"text/css\"></style>")).remove(QLatin1String("<head></head>")).remove(QLatin1String("<html><body>")).remove(QLatin1String("</body></html>")).remove(fontSize).replace(fontColor, QLatin1String("<span style=\"color:\\1;\">\\2</span>")).replace(fontFamily, QLatin1String("<span style=\"font-family:'\\1';\">\\2</span>"));

        QRegExp css = QRegExp(QLatin1String("<style type=\"text/css\">(.+)</style>"));
        css.setMinimal(true);
        css.indexIn(html);

        format.html = html.remove(css);
        format.css = css.cap(1);
    } else {
        format.html = m_appearanceUi.htmlTextEdit->toPlainText();
        format.css = m_appearanceUi.cssTextEdit->toPlainText();
    }

    disconnect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    if (sender() == m_appearanceUi.webView->page()) {
        m_appearanceUi.htmlTextEdit->setPlainText(format.html);
        m_appearanceUi.cssTextEdit->setPlainText(format.css);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->setHtml(QLatin1String("<style type=\"text/css\">") + format.css + QLatin1String("</style>") + format.html);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("boldButton"), m_appearanceUi.boldButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("italicButton"), m_appearanceUi.italicButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("underlineButton"), m_appearanceUi.underlineButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("designModeEditor"), this);
    }

    const int index = m_appearanceUi.formatComboBox->currentIndex();

    if (index < m_applet->formats(false).count() && (m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 1)).toString() != format.html || m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 2)).toString() != format.css || m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 3)).toBool() != format.background)) {
        addFormat(true);
    }

    m_appearanceUi.formatComboBox->setItemData(index, format.html, (Qt::UserRole + 1));
    m_appearanceUi.formatComboBox->setItemData(index, format.css, (Qt::UserRole + 2));
    m_appearanceUi.formatComboBox->setItemData(index, format.background, (Qt::UserRole + 3));

    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));
}

void Configuration::addFormat(bool automatically)
{
    QString title = m_appearanceUi.formatComboBox->itemText(m_appearanceUi.formatComboBox->currentIndex());

    if (automatically) {
        int i = 2;

        while (m_appearanceUi.formatComboBox->findText(QString(QLatin1String("%1 %2")).arg(title).arg(i)) >= 0) {
            ++i;
        }

        title = QString(QLatin1String("%1 %2")).arg(title).arg(i);
    } else {
        bool ok;

        title = KInputDialog::getText(i18n("Add new format"), i18n("Format name:"), title, &ok);

        if (!ok) {
            return;
        }
    }

    if (m_appearanceUi.formatComboBox->findText(title) >= 0) {
        KMessageBox::error(m_appearanceUi.formatComboBox, i18n("A format with this name already exists."));

        return;
    }

    if (title.startsWith(QLatin1Char('%')) && title.endsWith(QLatin1Char('%'))) {
        KMessageBox::error(m_appearanceUi.formatComboBox, i18n("Invalid format name."));

        return;
    }

    if (title.isEmpty()) {
        return;
    }

    int index = (m_appearanceUi.formatComboBox->currentIndex() + 1);
    const int builInFormats = m_applet->formats(false).count();

    if (index <= builInFormats) {
        index = m_appearanceUi.formatComboBox->count();
    }

    disconnect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));

    if (index == builInFormats && builInFormats == m_appearanceUi.formatComboBox->count())
    {
        m_appearanceUi.formatComboBox->insertSeparator(index);

        ++index;
    }

    m_appearanceUi.formatComboBox->insertItem(index, title, m_appearanceUi.htmlTextEdit->toPlainText());
    m_appearanceUi.formatComboBox->setCurrentIndex(index);
    m_appearanceUi.removeButton->setEnabled(true);

    connect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
}

void Configuration::removeFormat()
{
    if (m_appearanceUi.formatComboBox->currentIndex() > m_applet->formats(false).count()) {
        m_appearanceUi.formatComboBox->removeItem(m_appearanceUi.formatComboBox->currentIndex());

        if (m_appearanceUi.formatComboBox->itemData((m_appearanceUi.formatComboBox->count() - 1), Qt::DisplayRole).toString().isEmpty()) {
            m_appearanceUi.formatComboBox->removeItem(m_appearanceUi.formatComboBox->count() - 1);
        }
    }
}

void Configuration::updateControls()
{
    if (!m_applet->isUserConfiguring()) {
        return;
    }

    disconnect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));

    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("boldButton.setChecked(document.queryCommandState('bold'));"
            "italicButton.setChecked(document.queryCommandState('italic'));"
            "underlineButton.setChecked(document.queryCommandState('underline'));"
            "designModeEditor.setColor(document.queryCommandValue('forecolor'));"
            "designModeEditor.setFontSize(document.queryCommandValue('fontsize').replace('px', ''));"
            "designModeEditor.setFontFamily(document.queryCommandValue('fontname'))"));

    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));
}

void Configuration::triggerAction()
{
    QString actionName = sender()->objectName().remove(QLatin1String("Button")).toLower();
    QHash<QString, QWebPage::WebAction> actions;
    actions[QLatin1String("bold")] = QWebPage::ToggleBold;
    actions[QLatin1String("italic")] = QWebPage::ToggleItalic;
    actions[QLatin1String("underline")] = QWebPage::ToggleUnderline;
    actions[QLatin1String("justifyLeft")] = QWebPage::AlignLeft;
    actions[QLatin1String("justifyCenter")] = QWebPage::AlignCenter;
    actions[QLatin1String("justifyRight")] = QWebPage::AlignRight;

    if (!actions.contains(actionName)) {
        return;
    }

    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();

        switch (actions[actionName]) {
        case QWebPage::ToggleBold:
            cursor.insertText(QLatin1String("<b>") + cursor.selectedText() + QLatin1String("</b>"));

            break;
        case QWebPage::ToggleItalic:
            cursor.insertText(QLatin1String("<i>") + cursor.selectedText() + QLatin1String("</i>"));

            break;
        case QWebPage::ToggleUnderline:
            cursor.insertText(QLatin1String("<u>") + cursor.selectedText() + QLatin1String("</u>"));

            break;
        case QWebPage::AlignLeft:
            cursor.insertText(QLatin1String("<div style=\"text-align:left;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        case QWebPage::AlignCenter:
            cursor.insertText(QLatin1String("<div style=\"text-align:center;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        case QWebPage::AlignRight:
            cursor.insertText(QLatin1String("<div style=\"text-align:right;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        default:
            return;
        }

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->triggerAction(actions[actionName]);
    }
}

void Configuration::selectColor()
{
    KColorDialog colorDialog;
    colorDialog.setAlphaChannelEnabled(true);
    colorDialog.setColor(m_appearanceUi.colorButton->palette().button().color());
    colorDialog.setButtons(KDialog::Ok | KDialog::Cancel);

    if (colorDialog.exec() == QDialog::Accepted) {
        QPalette palette = m_appearanceUi.colorButton->palette();
        palette.setBrush(QPalette::Button, colorDialog.color());

        m_appearanceUi.colorButton->setPalette(palette);

        if (m_appearanceUi.tabWidget->currentIndex() > 0) {
            QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
            cursor.insertText(QLatin1String("<span style=\"color:") + colorDialog.color().name() + QLatin1String(";\">") + cursor.selectedText() + QLatin1String("</span>"));

            m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
        } else {
            m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('forecolor', false, '") + colorDialog.color().name() + QLatin1String("')"));
        }
    }
}

void Configuration::selectFontSize(const QString &size)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QLatin1String("<span style=\"font-size:") + QString::number(size.toInt()) + QLatin1String("px;\">") + cursor.selectedText() + QLatin1String("</span>"));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontsizedelta', false, ") + QString::number(size.toInt() - m_fontSize) + QLatin1String(")"));
    }

    m_fontSize = size.toInt();
}

void Configuration::selectFontFamily(const QFont &font)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QLatin1String("<span style=\"font-family:'") + font.family()+ QLatin1String("';\">") + cursor.selectedText() + QLatin1String("</span>"));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontname', false, '") + font.family() + QLatin1String("')"));
    }
}

void Configuration::setColor(const QString &color)
{
    if (color == QLatin1String("false")) {
        return;
    }

    QRegExp expression = QRegExp(QLatin1String("rgb\\((\\d+), (\\d+), (\\d+)\\)"));
    expression.indexIn(color);

    const QStringList rgb = expression.capturedTexts();

    QPalette palette = m_appearanceUi.colorButton->palette();
    palette.setBrush(QPalette::Button, QColor(rgb.at(1).toInt(), rgb.at(2).toInt(), rgb.at(3).toInt()));

    m_appearanceUi.colorButton->setPalette(palette);
}

void Configuration::setFontSize(const QString &size)
{
    if (!m_appearanceUi.fontSizeComboBox->hasFocus()) {
        m_appearanceUi.fontSizeComboBox->setEditText(size);
    }

    m_fontSize = size.toInt();
}

void Configuration::setFontFamily(const QString &font)
{
    m_appearanceUi.fontFamilyComboBox->setCurrentFont(QFont(font));
}

void Configuration::selectionChanged()
{
    m_controlsTimer = startTimer(250);

    if (m_appearanceUi.webView->page()->selectedText().endsWith(QLatin1Char('%'))) {
        m_appearanceUi.webView->page()->triggerAction(QWebPage::SelectNextChar);
    }
}

void Configuration::itemSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_clipboardUi.clipboardActionsTable->selectedItems();

    m_clipboardUi.moveUpButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.first()) != 0);
    m_clipboardUi.moveDownButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.last()) != (m_clipboardUi.clipboardActionsTable->rowCount() - 1));
    m_clipboardUi.deleteButton->setEnabled(!selectedItems.isEmpty());
}

void Configuration::editRow(QTableWidgetItem *item)
{
    if (m_editedItem) {
        m_clipboardUi.clipboardActionsTable->closePersistentEditor(m_editedItem);
    }

    m_editedItem = item;

    m_clipboardUi.clipboardActionsTable->openPersistentEditor(m_editedItem);
}

void Configuration::insertRow()
{
    const int row = ((m_clipboardUi.clipboardActionsTable->rowCount() && m_clipboardUi.clipboardActionsTable->currentRow() >= 0) ? m_clipboardUi.clipboardActionsTable->currentRow() : 0);
    QTableWidgetItem *item = new QTableWidgetItem(QString());

    m_clipboardUi.clipboardActionsTable->insertRow(row);
    m_clipboardUi.clipboardActionsTable->setItem(row, 0, item);

    editRow(item);

    item = new QTableWidgetItem(QString());
    item->setFlags(0);

    m_clipboardUi.clipboardActionsTable->setItem(row, 1, item);
    m_clipboardUi.clipboardActionsTable->setCurrentCell(row, 0);
}

void Configuration::deleteRow()
{
    m_clipboardUi.clipboardActionsTable->removeRow(m_clipboardUi.clipboardActionsTable->row(m_clipboardUi.clipboardActionsTable->selectedItems().at(0)));
}

void Configuration::moveRow(bool up)
{
    int sourceRow = m_clipboardUi.clipboardActionsTable->row(m_clipboardUi.clipboardActionsTable->selectedItems().at(0));
    int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));

    QList<QTableWidgetItem*> sourceItems;
    QList<QTableWidgetItem*> destinationItems;

    for (int i = 0; i < 2; ++i) {
        sourceItems.append(m_clipboardUi.clipboardActionsTable->takeItem(sourceRow, i));

        destinationItems.append(m_clipboardUi.clipboardActionsTable->takeItem(destinationRow, i));
    }

    for (int i = 0; i < 2; ++i) {
        m_clipboardUi.clipboardActionsTable->setItem(sourceRow, i, destinationItems.at(i));
        m_clipboardUi.clipboardActionsTable->setItem(destinationRow, i, sourceItems.at(i));
    }

    m_clipboardUi.clipboardActionsTable->setCurrentCell(destinationRow, 0);
}

void Configuration::moveRowUp()
{
    moveRow(true);
}

void Configuration::moveRowDown()
{
    moveRow(false);
}

void Configuration::updateRow(int row)
{
    if (!m_clipboardUi.clipboardActionsTable->item(row, 1)) {
        return;
    }

    const QString preview = m_applet->evaluateFormat(m_clipboardUi.clipboardActionsTable->item(row, 0)->text(), QDateTime::currentDateTime());

    m_clipboardUi.clipboardActionsTable->item(row, 1)->setText(preview);
    m_clipboardUi.clipboardActionsTable->item(row, 1)->setToolTip(preview);
}

bool Configuration::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && m_editedItem) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (m_clipboardUi.clipboardActionsTable->itemAt(mouseEvent->pos()) != m_editedItem) {
            m_clipboardUi.clipboardActionsTable->closePersistentEditor(m_editedItem);
        }
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (!m_clipboardUi.clipboardActionsTable->itemAt(mouseEvent->pos())) {
            insertRow();
        }
    }

    return QObject::eventFilter(object, event);
}

}