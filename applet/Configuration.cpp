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

#include "Configuration.h"
#include "ComponentDialog.h"
#include "PreviewDelegate.h"
#include "ExpressionDelegate.h"

#include <QtCore/QXmlStreamWriter>
#include <QtWebKit/QWebFrame>

#include <KMenu>
#include <KLocale>
#include <KMessageBox>
#include <KColorDialog>
#include <KInputDialog>
#include <KStandardDirs>

#include <Plasma/Theme>

namespace AdjustableClock
{

Configuration::Configuration(Applet *applet, KConfigDialog *parent) : QObject(parent),
    m_applet(applet),
    m_clock(new Clock(applet->getDataSource(), EditorClock)),
    m_themesModel(new QStandardItemModel(this)),
    m_editedItem(NULL)
{
    QWidget *appearanceConfiguration = new QWidget();
    QWidget *clipboardConfiguration = new QWidget();

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardConfiguration);

    const QList<Theme> themes = m_applet->getThemes();

    for (int i = 0; i < themes.count(); ++i) {
        QStandardItem *item = new QStandardItem();
        item->setData(themes.at(i).id, IdRole);
        item->setData(themes.at(i).title, TitleRole);
        item->setData(themes.at(i).description, DescriptionRole);
        item->setData(themes.at(i).author, AuthorRole);
        item->setData(themes.at(i).html, HtmlRole);
        item->setData(themes.at(i).script, ScriptRole);
        item->setData(themes.at(i).background, BackgroundRole);
        item->setData(themes.at(i).bundled, BundledRole);
        item->setToolTip(QString("<b>%1</b>%2").arg(themes.at(i).author.isEmpty() ? themes.at(i).title : i18n("\"%1\" by %2").arg(themes.at(i).title).arg(themes.at(i).author)).arg(themes.at(i).description.isEmpty() ? QString() : QString("<br />") + themes.at(i).description));

        m_themesModel->appendRow(item);
    }

    PreviewDelegate *delegate = new PreviewDelegate(m_applet->getDataSource(), m_appearanceUi.themesView);
    QPalette webViewPalette = m_appearanceUi.webView->page()->palette();
    webViewPalette.setBrush(QPalette::Base, Qt::transparent);

    m_appearanceUi.themesView->setModel(m_themesModel);
    m_appearanceUi.themesView->setItemDelegate(delegate);
    m_appearanceUi.themesView->installEventFilter(this);
    m_appearanceUi.themesView->viewport()->installEventFilter(this);
    m_appearanceUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_appearanceUi.webView->page()->setPalette(webViewPalette);
    m_appearanceUi.webView->page()->setContentEditable(true);
    m_appearanceUi.webView->page()->action(QWebPage::Undo)->setText(i18n("Undo"));
    m_appearanceUi.webView->page()->action(QWebPage::Undo)->setIcon(KIcon("edit-undo"));
    m_appearanceUi.webView->page()->action(QWebPage::Redo)->setText(i18n("Redo"));
    m_appearanceUi.webView->page()->action(QWebPage::Redo)->setIcon(KIcon("edit-redo"));
    m_appearanceUi.webView->page()->action(QWebPage::Cut)->setText(i18n("Cut"));
    m_appearanceUi.webView->page()->action(QWebPage::Cut)->setIcon(KIcon("edit-cut"));
    m_appearanceUi.webView->page()->action(QWebPage::Copy)->setText(i18n("Copy"));
    m_appearanceUi.webView->page()->action(QWebPage::Copy)->setIcon(KIcon("edit-copy"));
    m_appearanceUi.webView->page()->action(QWebPage::Paste)->setText(i18n("Paste"));
    m_appearanceUi.webView->page()->action(QWebPage::Paste)->setIcon(KIcon("edit-paste"));
    m_appearanceUi.webView->page()->action(QWebPage::SelectAll)->setText(i18n("Select All"));
    m_appearanceUi.webView->page()->action(QWebPage::SelectAll)->setIcon(KIcon("select-all"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleBold)->setText(i18n("Bold"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleBold)->setIcon(KIcon("format-text-bold"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleItalic)->setText(i18n("Italic"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleItalic)->setIcon(KIcon("format-text-italic"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleUnderline)->setText(i18n("Underline"));
    m_appearanceUi.webView->page()->action(QWebPage::ToggleUnderline)->setIcon(KIcon("format-text-underline"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignLeft)->setText(i18n("Justify Left"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignLeft)->setIcon(KIcon("format-justify-left"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignCenter)->setText(i18n("Justify Center"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignCenter)->setIcon(KIcon("format-justify-center"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignRight)->setText(i18n("Justify Right"));
    m_appearanceUi.webView->page()->action(QWebPage::AlignRight)->setIcon(KIcon("format-justify-right"));
    m_appearanceUi.boldButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::ToggleBold));
    m_appearanceUi.italicButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::ToggleItalic));
    m_appearanceUi.underlineButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::ToggleUnderline));
    m_appearanceUi.justifyLeftButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::AlignLeft));
    m_appearanceUi.justifyCenterButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::AlignCenter));
    m_appearanceUi.justifyRightButton->setDefaultAction(m_appearanceUi.webView->page()->action(QWebPage::AlignRight));
    m_appearanceUi.backgroundButton->setIcon(KIcon("games-config-background"));
    m_appearanceUi.componentButton->setIcon(KIcon("chronometer"));

    m_clipboardUi.moveUpButton->setIcon(KIcon("arrow-up"));
    m_clipboardUi.moveDownButton->setIcon(KIcon("arrow-down"));
    m_clipboardUi.clipboardActionsList->setItemDelegate(new ExpressionDelegate(m_clock, this));
    m_clipboardUi.clipboardActionsList->viewport()->installEventFilter(this);
    m_clipboardUi.fastCopyExpressionEdit->setText(m_applet->config().readEntry("fastCopyExpression", "Clock.toString(Clock.Year) + '-' + Clock.toString(Clock.Month) + '-' + Clock.toString(Clock.DayOfMonth) + ' ' + Clock.toString(Clock.Hour) + ':' + Clock.toString(Clock.Minute) + ':' + Clock.toString(Clock.Second)"));
    m_clipboardUi.fastCopyExpressionEdit->setClock(m_clock);

    const QStringList clipboardExpressions = m_applet->getClipboardExpressions();

    for (int i = 0; i < clipboardExpressions.count(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(clipboardExpressions.at(i));

        if (!clipboardExpressions.at(i).isEmpty()) {
            item->setToolTip(m_clock->evaluate(clipboardExpressions.at(i)));
        }

        m_clipboardUi.clipboardActionsList->addItem(item);
    }

    itemSelectionChanged();

    QPalette buttonPalette = m_appearanceUi.colorButton->palette();
    buttonPalette.setBrush(QPalette::Button, Qt::black);

    m_appearanceUi.colorButton->setPalette(buttonPalette);

    parent->addPage(appearanceConfiguration, i18n("Appearance"), "preferences-desktop-theme");
    parent->addPage(clipboardConfiguration, i18n("Clipboard actions"), "edit-copy");
    parent->resize(500, 400);

    connect(parent, SIGNAL(applyClicked()), this, SLOT(save()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(save()));
    connect(parent, SIGNAL(finished()), this, SLOT(disableUpdates()));
    connect(m_appearanceUi.mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateView(int)));
    connect(m_appearanceUi.editorTabWidget, SIGNAL(currentChanged(int)), this, SLOT(updateEditor(int)));
    connect(m_appearanceUi.themesView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTheme(QModelIndex)));
    connect(m_appearanceUi.newButton, SIGNAL(clicked()), this, SLOT(newTheme()));
    connect(m_appearanceUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteTheme()));
    connect(m_appearanceUi.renameButton, SIGNAL(clicked()), this, SLOT(renameTheme()));
    connect(m_appearanceUi.webView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showEditorContextMenu(QPoint)));
    connect(m_appearanceUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_appearanceUi.zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)));
    connect(m_appearanceUi.boldButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.italicButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.underlineButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(backgroundChanged()));
    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));
    connect(m_appearanceUi.componentButton, SIGNAL(clicked()), this, SLOT(insertComponent()));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertItem()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteItem()));
    connect(m_clipboardUi.editButton, SIGNAL(clicked()), this, SLOT(editItem()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveItemUp()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveItemDown()));
    connect(m_clipboardUi.clipboardActionsList, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(m_clipboardUi.clipboardActionsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(updateItem(QListWidgetItem*)));
    connect(m_clipboardUi.clipboardActionsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(editItem(QListWidgetItem*)));
    connect(m_clipboardUi.fastCopyExpressionEdit, SIGNAL(textChanged(QString)), this, SLOT(modify()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), delegate, SLOT(clear()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), m_appearanceUi.themesView->viewport(), SLOT(repaint()));
    connect(this, SIGNAL(clearCache()), delegate, SLOT(clear()));
}

void Configuration::save()
{
    updateEditor(m_appearanceUi.editorTabWidget->currentIndex() ? 0 : 1);

    if (m_editedItem) {
        m_clipboardUi.clipboardActionsList->closePersistentEditor(m_editedItem);
    }

    QStringList clipboardExpressions;

    for (int i = 0; i < m_clipboardUi.clipboardActionsList->count(); ++i) {
        clipboardExpressions.append(m_clipboardUi.clipboardActionsList->item(i)->text());
    }

    m_applet->config().writeEntry("theme", m_appearanceUi.themesView->currentIndex().data(IdRole).toString());
    m_applet->config().writeEntry("clipboardExpressions", clipboardExpressions);
    m_applet->config().writeEntry("fastCopyExpression", m_clipboardUi.fastCopyExpressionEdit->text());

    QFile file(KStandardDirs::locateLocal("data", "adjustableclock/custom-themes.xml"));
    file.open(QFile::WriteOnly | QFile::Text);

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("themes");

    for (int i = 0; i < m_themesModel->rowCount(); ++i) {
        QModelIndex index = m_themesModel->index(i, 0);

        if (index.data(BundledRole).toBool()) {
            continue;
        }

        stream.writeStartElement("theme");
        stream.writeStartElement("id");
        stream.writeCharacters(index.data(IdRole).toString());
        stream.writeEndElement();
        stream.writeStartElement("title");
        stream.writeCharacters(index.data(TitleRole).toString());
        stream.writeEndElement();
        stream.writeStartElement("background");
        stream.writeCharacters(index.data(BackgroundRole).toBool() ? "true" : "false");
        stream.writeEndElement();
        stream.writeStartElement("html");
        stream.writeCDATA(index.data(HtmlRole).toString());
        stream.writeEndElement();
        stream.writeStartElement("script");
        stream.writeCDATA(index.data(ScriptRole).toString());
        stream.writeEndElement();
        stream.writeEndElement();
    }

    stream.writeEndElement();
    stream.writeEndDocument();

    file.close();

    static_cast<KConfigDialog*>(parent())->enableButtonApply(false);

    emit accepted();
}

void Configuration::modify()
{
    static_cast<KConfigDialog*>(parent())->enableButtonApply(true);
}

void Configuration::enableUpdates()
{
    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(themeChanged()));
    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
    connect(m_appearanceUi.scriptTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
}

void Configuration::disableUpdates()
{
    disconnect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(themeChanged()));
    disconnect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
    disconnect(m_appearanceUi.scriptTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
}

void Configuration::insertComponent()
{
    connect(new ComponentDialog(m_applet->getClock(), m_appearanceUi.componentButton), SIGNAL(insertComponent(QString,ClockComponent)), this, SLOT(insertComponent(QString,ClockComponent)));
}

void Configuration::insertComponent(const QString &script, ClockComponent component)
{
    const QString type = Clock::getComponentString(component).toLower();
    QString identifier = type;
    int number = 1;

    while (m_appearanceUi.webView->page()->mainFrame()->findAllElements(QString("#%1").arg(identifier)).count() > 0) {
        ++number;

        identifier = QString("%1_%2").arg(type).arg(number);
    }

    const QString html = QString(" <span id=\"%1\">%2</span> ").arg(identifier).arg(m_clock->evaluate(QString("Clock.toString(%1)").arg(script)));

    m_appearanceUi.scriptTextEdit->moveCursor(QTextCursor::Start);
    m_appearanceUi.scriptTextEdit->insertPlainText(QString("Clock.setRule(\"#%1\", %2);\n").arg(identifier).arg(script));

    if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
        m_appearanceUi.htmlTextEdit->insertPlainText(html);

        sourceChanged();
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("document.execCommand('inserthtml', false, '%1')").arg(html));
    }
}

void Configuration::selectTheme(const QModelIndex &index)
{
    if (m_appearanceUi.themesView->selectionModel()->hasSelection()) {
        modify();
    }

    disableUpdates();

    m_appearanceUi.themesView->setCurrentIndex(index);
    m_appearanceUi.themesView->scrollTo(index, QAbstractItemView::EnsureVisible);
    m_appearanceUi.htmlTextEdit->setPlainText(index.data(HtmlRole).toString());
    m_appearanceUi.scriptTextEdit->setPlainText(index.data(ScriptRole).toString());
    m_appearanceUi.backgroundButton->setChecked(index.data(BackgroundRole).toBool());
    m_appearanceUi.deleteButton->setEnabled(!index.data(BundledRole).toBool());
    m_appearanceUi.renameButton->setEnabled(!index.data(BundledRole).toBool());

    sourceChanged();
    enableUpdates();
}

void Configuration::newTheme(bool automatically)
{
    QString title = m_appearanceUi.themesView->currentIndex().data(TitleRole).toString();

    if (automatically) {
        int i = 2;

        while (findRow(QString("%1 %2").arg(title).arg(i)) >= 0) {
            ++i;
        }

        title = QString("%1 %2").arg(title).arg(i);
    } else {
        bool ok;

        title = KInputDialog::getText(i18n("Add new theme"), i18n("Theme name:"), title, &ok);

        if (!ok) {
            return;
        }
    }

    if (findRow(title) >= 0) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("A theme with this name already exists."));

        return;
    }

    if (title.startsWith(QChar('%')) && title.endsWith(QChar('%'))) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Invalid theme name."));

        return;
    }

    if (title.isEmpty()) {
        return;
    }

    disconnect(m_appearanceUi.themesView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTheme(QModelIndex)));

    m_themesModel->insertRow(m_themesModel->rowCount());

    const QModelIndex index = m_themesModel->index((m_themesModel->rowCount() - 1), 0);

    m_themesModel->setData(index, title, IdRole);
    m_themesModel->setData(index, title, TitleRole);
    m_themesModel->setData(index, (QString("<b>%1</b>").arg(title)), Qt::ToolTipRole);
    m_themesModel->setData(index, m_appearanceUi.htmlTextEdit->toPlainText(), HtmlRole);
    m_themesModel->setData(index, m_appearanceUi.scriptTextEdit->toPlainText(), ScriptRole);
    m_themesModel->setData(index, m_appearanceUi.backgroundButton->isChecked(), BackgroundRole);
    m_themesModel->setData(index, false, BundledRole);

    m_appearanceUi.themesView->setCurrentIndex(index);

    modify();

    connect(m_appearanceUi.themesView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTheme(QModelIndex)));
}

void Configuration::deleteTheme()
{
    if (!m_appearanceUi.themesView->currentIndex().data(BundledRole).toBool() && KMessageBox::questionYesNo(m_appearanceUi.themesView, i18n("Do you really want to delete theme \"%1\"?").arg(m_appearanceUi.themesView->currentIndex().data(TitleRole).toString()), i18n("Delete Theme")) == KMessageBox::Yes) {
        const int row = m_appearanceUi.themesView->currentIndex().row();

        m_themesModel->removeRow(row);

        selectTheme(m_themesModel->index(qMax((row - 1), 0), 0));
    }
}

void Configuration::renameTheme()
{
    bool ok;
    const QString title = KInputDialog::getText(i18n("Add new theme"), i18n("Theme name:"), m_appearanceUi.themesView->currentIndex().data(TitleRole).toString(), &ok);

    if (!ok) {
        return;
    }

    if (findRow(title) >= 0) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("A theme with this name already exists."));

        return;
    }

    m_themesModel->setData(m_appearanceUi.themesView->currentIndex(), title, TitleRole);

    modify();

    emit clearCache();
}

void Configuration::updateTheme(const Theme &theme)
{
    const QModelIndex index = m_appearanceUi.themesView->currentIndex();

    m_themesModel->setData(index, theme.html, HtmlRole);
    m_themesModel->setData(index, theme.script, ScriptRole);
    m_themesModel->setData(index, theme.background, BackgroundRole);

    emit clearCache();
}

void Configuration::updateView(int tab)
{
    if (tab == 1 && m_appearanceUi.editorTabWidget->currentIndex() == 0) {
        QMouseEvent event(QEvent::MouseButtonPress, QPoint(5, 5), Qt::LeftButton, Qt::LeftButton, 0);

        QCoreApplication::sendEvent(m_appearanceUi.webView, &event);

        m_appearanceUi.webView->setFocus(Qt::OtherFocusReason);
    }

    updateEditor(m_appearanceUi.editorTabWidget->currentIndex() ? 0 : 1);
}

void Configuration::updateEditor(int tab)
{
    if (tab == 1) {
        richTextChanged();
    } else {
        sourceChanged();
    }
}

void Configuration::triggerAction()
{
    if (m_appearanceUi.editorTabWidget->currentIndex() == 0) {
        return;
    }

    QString actionName = sender()->objectName().remove("Button").toLower();
    QHash<QString, QWebPage::WebAction> actions;
    actions["bold"] = QWebPage::ToggleBold;
    actions["italic"] = QWebPage::ToggleItalic;
    actions["underline"] = QWebPage::ToggleUnderline;
    actions["justifyLeft"] = QWebPage::AlignLeft;
    actions["justifyCenter"] = QWebPage::AlignCenter;
    actions["justifyRight"] = QWebPage::AlignRight;

    if (!actions.contains(actionName)) {
        return;
    }

    QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();

    switch (actions[actionName]) {
    case QWebPage::ToggleBold:
        cursor.insertText(QString("<b>%1</b>").arg(cursor.selectedText()));

        break;
    case QWebPage::ToggleItalic:
        cursor.insertText(QString("<i>%1</i>").arg(cursor.selectedText()));

        break;
    case QWebPage::ToggleUnderline:
        cursor.insertText(QString("<u>%1</u>").arg(cursor.selectedText()));

        break;
    case QWebPage::AlignLeft:
        cursor.insertText(QString("<div style=\"text-align:left;\">%1</div>").arg(cursor.selectedText()));

        break;
    case QWebPage::AlignCenter:
        cursor.insertText(QString("<div style=\"text-align:center;\">%1</div>").arg(cursor.selectedText()));

        break;
    case QWebPage::AlignRight:
        cursor.insertText(QString("<div style=\"text-align:right;\">%1</div>").arg(cursor.selectedText()));

        break;
    default:
        return;
    }

    m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
}

void Configuration::selectionChanged()
{
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript("fixSelection()");

    QRegExp expression = QRegExp("rgb\\((\\d+), (\\d+), (\\d+)\\)");
    expression.indexIn(m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('color')").toString());

    const QStringList rgb = expression.capturedTexts();

    QPalette palette = m_appearanceUi.colorButton->palette();
    palette.setBrush(QPalette::Button, QColor(rgb.at(1).toInt(), rgb.at(2).toInt(), rgb.at(3).toInt()));

    m_appearanceUi.colorButton->setPalette(palette);

    disconnect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    disconnect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));

    m_appearanceUi.fontSizeComboBox->setEditText(m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('font-size')").toString().remove("px"));
    m_appearanceUi.fontFamilyComboBox->setCurrentFont(QFont(m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('font-family')").toString()));

    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));
}

void Configuration::setColor()
{
    KColorDialog colorDialog;
    colorDialog.setAlphaChannelEnabled(true);
    colorDialog.setColor(m_appearanceUi.colorButton->palette().button().color());
    colorDialog.setButtons(KDialog::Ok | KDialog::Cancel);

    if (colorDialog.exec() == QDialog::Accepted) {
        QPalette palette = m_appearanceUi.colorButton->palette();
        palette.setBrush(QPalette::Button, colorDialog.color());

        m_appearanceUi.colorButton->setPalette(palette);

        if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
            QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
            cursor.insertText(QString("<span style=\"color:%1;\">%2</span>").arg(colorDialog.color().name()).arg(cursor.selectedText()));

            m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
        } else {
            m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("setStyle('color', '%1')").arg(colorDialog.color().name()));
        }
    }
}

void Configuration::setFontSize(const QString &size)
{
    if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QString("<span style=\"font-size:%1px;\">%2</span>").arg(QString::number(size.toInt())).arg(cursor.selectedText()));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("setStyle('font-size', '%1px')").arg(size));
    }
}

void Configuration::setFontFamily(const QFont &font)
{
    if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QString("<span style=\"font-family:'%1';\">%2</span>").arg(font.family()).arg(cursor.selectedText()));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("setStyle('font-family', '\\\'%1\\\'')").arg(font.family()));
    }
}

void Configuration::setZoom(int zoom)
{
    m_appearanceUi.webView->setZoomFactor((qreal) zoom / 100);
    m_appearanceUi.zoomSlider->setToolTip(i18n("Zoom: %1%").arg(zoom));
}

void Configuration::showEditorContextMenu(const QPoint &position)
{
    KMenu menu(m_appearanceUi.webView);
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::Undo));
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::Redo));
    menu.addSeparator();
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::Cut));
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::Copy));
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::Paste));
    menu.addAction(m_appearanceUi.webView->page()->action(QWebPage::SelectAll));
    menu.addSeparator();
    menu.addAction(m_appearanceUi.boldButton->defaultAction());
    menu.addAction(m_appearanceUi.italicButton->defaultAction());
    menu.addAction(m_appearanceUi.underlineButton->defaultAction());
    menu.addSeparator();
    menu.addAction(m_appearanceUi.justifyLeftButton->defaultAction());
    menu.addAction(m_appearanceUi.justifyCenterButton->defaultAction());
    menu.addAction(m_appearanceUi.justifyRightButton->defaultAction());
    menu.exec(m_appearanceUi.webView->mapToGlobal(position));
}

void Configuration::themeChanged()
{
    modify();

    if (m_appearanceUi.themesView->currentIndex().data(BundledRole).toBool()) {
        newTheme(true);
    }
}

void Configuration::backgroundChanged()
{
    Theme theme;
    theme.html = m_appearanceUi.htmlTextEdit->toPlainText();
    theme.script = m_appearanceUi.scriptTextEdit->toPlainText();
    theme.background = m_appearanceUi.backgroundButton->isChecked();

    themeChanged();
    updateTheme(theme);
}

void Configuration::richTextChanged()
{
    QWebPage page;
    page.mainFrame()->setHtml(m_appearanceUi.webView->page()->mainFrame()->toHtml().remove(QRegExp(" class=\"Apple-style-span\"")));

    const QWebElementCollection elements = page.mainFrame()->findAllElements(".component");

    for (int i = 0; i < elements.count(); ++i) {
        if (!elements.at(i).firstChild().isNull()) {
            QWebElement element = elements.at(i).firstChild();
            QStringList styles;

            do {
                if (element.hasAttribute("style")) {
                    styles.append(element.attribute("style"));
                }

                element = element.firstChild();
            } while (!element.isNull());

            elements.at(i).setAttribute("style", styles.join(QString()));
        }

        elements.at(i).setInnerXml(elements.at(i).attribute("value"));
        elements.at(i).removeAttribute("title");
        elements.at(i).removeAttribute("value");
        elements.at(i).removeClass("component");
    }

    Theme theme;
    theme.html = page.mainFrame()->toHtml().remove(QRegExp("<head></head>"));
    theme.script = m_appearanceUi.scriptTextEdit->toPlainText();
    theme.background = m_appearanceUi.backgroundButton->isChecked();

    disableUpdates();

    m_appearanceUi.htmlTextEdit->setPlainText(theme.html);

    enableUpdates();
    updateTheme(theme);
}

void Configuration::sourceChanged()
{
    Theme theme;
    theme.html = m_appearanceUi.htmlTextEdit->toPlainText();
    theme.script = m_appearanceUi.scriptTextEdit->toPlainText();
    theme.background = m_appearanceUi.backgroundButton->isChecked();

    disableUpdates();

    m_clock->setDocument(m_appearanceUi.webView->page()->mainFrame());

    QFile file(":/editor.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    m_appearanceUi.webView->page()->mainFrame()->setHtml(theme.html);
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(theme.script);
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString(file.readAll()));

    enableUpdates();
    updateTheme(theme);
}

void Configuration::itemSelectionChanged()
{
    const QList<QListWidgetItem*> selectedItems = m_clipboardUi.clipboardActionsList->selectedItems();

    m_clipboardUi.moveUpButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsList->row(selectedItems.first()) != 0);
    m_clipboardUi.moveDownButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsList->row(selectedItems.last()) != (m_clipboardUi.clipboardActionsList->count() - 1));
    m_clipboardUi.editButton->setEnabled(!selectedItems.isEmpty());
    m_clipboardUi.deleteButton->setEnabled(!selectedItems.isEmpty());
}

void Configuration::editItem(QListWidgetItem *item)
{
    if (m_editedItem) {
        m_clipboardUi.clipboardActionsList->closePersistentEditor(m_editedItem);
    }

    if (!item) {
        item = m_clipboardUi.clipboardActionsList->currentItem();
    }

    if (!item) {
        return;
    }

    m_editedItem = item;

    modify();

    m_clipboardUi.clipboardActionsList->openPersistentEditor(m_editedItem);
}

void Configuration::insertItem()
{
    QListWidgetItem *item = new QListWidgetItem(QString());

    m_clipboardUi.clipboardActionsList->insertItem((m_clipboardUi.clipboardActionsList->currentRow() + 1), item);

    editItem(item);

    itemSelectionChanged();
    modify();
}

void Configuration::deleteItem()
{
    QListWidgetItem *item = m_clipboardUi.clipboardActionsList->takeItem(m_clipboardUi.clipboardActionsList->currentRow());

    if (item == m_editedItem) {
        m_editedItem = NULL;
    }

    if (item) {
        delete item;
    }

    itemSelectionChanged();
    modify();
}

void Configuration::moveItem(bool up)
{
    int sourceRow = m_clipboardUi.clipboardActionsList->currentRow();
    int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));

    QListWidgetItem *sourceItem = m_clipboardUi.clipboardActionsList->takeItem(sourceRow);
    QListWidgetItem *destinationItem = m_clipboardUi.clipboardActionsList->takeItem(destinationRow);

    m_clipboardUi.clipboardActionsList->insertItem(sourceRow, destinationItem);
    m_clipboardUi.clipboardActionsList->insertItem(destinationRow, sourceItem);
    m_clipboardUi.clipboardActionsList->setCurrentRow(destinationRow);

    itemSelectionChanged();
    modify();
}

void Configuration::moveItemUp()
{
    moveItem(true);
}

void Configuration::moveItemDown()
{
    moveItem(false);
}

void Configuration::updateItem(QListWidgetItem *item)
{
    if (item) {
        item->setToolTip(m_clock->evaluate(item->text()));
    }
}

int Configuration::findRow(const QString &text, int role)
{
    for (int i = 0; i < m_themesModel->rowCount(); ++i) {
        if (m_themesModel->index(i, 0).data(role).toString() == text) {
            return i;
        }
    }

    return -1;
}

bool Configuration::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_appearanceUi.themesView && event->type() == QEvent::Paint && !m_appearanceUi.themesView->currentIndex().isValid()) {
        selectTheme(m_themesModel->index(qMax(findRow(m_applet->config().readEntry("theme", "%default%"), IdRole), 0), 0));
    } else if (event->type() == QEvent::MouseButtonDblClick && object == m_appearanceUi.themesView->viewport()) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (!m_appearanceUi.themesView->indexAt(mouseEvent->pos()).isValid()) {
            newTheme();
        }
    } else if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) && object == m_clipboardUi.clipboardActionsList->viewport()) {
        if (event->type() == QEvent::MouseButtonPress && m_editedItem) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (m_clipboardUi.clipboardActionsList->itemAt(mouseEvent->pos()) != m_editedItem) {
                m_clipboardUi.clipboardActionsList->closePersistentEditor(m_editedItem);
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (!m_clipboardUi.clipboardActionsList->itemAt(mouseEvent->pos())) {
                insertItem();
            }
        }
    }

    return QObject::eventFilter(object, event);
}

}