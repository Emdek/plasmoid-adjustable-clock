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
#include "Clock.h"
#include "ComponentWidget.h"
#include "OptionWidget.h"
#include "ThemeDelegate.h"
#include "ExpressionDelegate.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QFormLayout>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElementCollection>

#include <KMenu>
#include <KDialog>
#include <KLocale>
#include <KMessageBox>
#include <KColorDialog>
#include <KDesktopFile>
#include <KInputDialog>
#include <KStandardDirs>

#include <KTextEditor/View>
#include <KTextEditor/Editor>
#include <KTextEditor/EditorChooser>
#include <KTextEditor/ConfigInterface>

#include <Plasma/Theme>
#include <Plasma/Package>

namespace AdjustableClock
{

Configuration::Configuration(Applet *applet, KConfigDialog *parent) : QObject(parent),
    m_applet(applet),
    m_themesModel(new QStandardItemModel(this)),
    m_actionsModel(new QStandardItemModel(this)),
    m_componentWidget(NULL),
    m_document(NULL)
{
    QWidget *appearanceConfiguration = new QWidget();
    QWidget *clipboardConfiguration = new QWidget();

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardConfiguration);

    const QStringList locations = KGlobal::dirs()->findDirs("data", "plasma/adjustableclock");

    for (int i = 0; i < locations.count(); ++i) {
        QStringList themes = Plasma::Package::listInstalled(locations.at(i));
        themes.sort();

        for (int j = 0; j < themes.count(); ++j) {
            KDesktopFile desktopFile("data", QString("%1/%2/metadata.desktop").arg(locations.at(i)).arg(themes.at(j)));
            QFile file(QString("%1/%2/contents/ui/main.html").arg(locations.at(i)).arg(themes.at(j)));
            file.open(QIODevice::ReadOnly | QIODevice::Text);

            QTextStream stream(&file);
            stream.setCodec("UTF-8");

            QStandardItem *item = new QStandardItem();
            item->setData(themes.at(j), IdRole);
            item->setData(locations.at(i), PathRole);
            item->setData(desktopFile.readName(), TitleRole);
            item->setData(desktopFile.readComment(), CommentRole);
            item->setData(desktopFile.desktopGroup().readEntry("X-KDE-PluginInfo-Author", QString()), AuthorRole);
            item->setData(stream.readAll(), HtmlRole);
            item->setData(false, OptionsRole);
            item->setData(false, ModificationRole);

            QString toolTip;

            if (item->data(AuthorRole).isNull()) {
                toolTip = QString("<b>\"%1\"</b>").arg(item->data(TitleRole).toString());
            } else {
                toolTip = i18n("<b>\"%1\" by %2</b>").arg(item->data(TitleRole).toString()).arg(item->data(AuthorRole).toString());
            }

            if (!item->data(CommentRole).isNull()) {
                toolTip.append("<br>\n").append(item->data(CommentRole).toString());
            }

            item->setToolTip(toolTip);

//             if (!themes.at(i).options.isEmpty()) {
//                 m_options[item->data(IdRole).toString()] = themes.at(i).options;
//             }

            m_themesModel->appendRow(item);
        }
    }

    const QStringList clipboardExpressions = m_applet->getClipboardExpressions();

    for (int i = 0; i < clipboardExpressions.count(); ++i) {
        QStandardItem *item = new QStandardItem(clipboardExpressions.at(i));

        if (!clipboardExpressions.at(i).isEmpty()) {
            item->setToolTip(m_applet->getClock()->evaluate(clipboardExpressions.at(i), true));
        }

        m_actionsModel->appendRow(item);
    }

    selectAction(m_actionsModel->index(0, 0));

    ThemeDelegate *delegate = new ThemeDelegate(m_applet->getClock(), m_appearanceUi.themesView);

    m_appearanceUi.themesView->setModel(m_themesModel);
    m_appearanceUi.themesView->setItemDelegate(delegate);
    m_appearanceUi.themesView->installEventFilter(this);
    m_appearanceUi.themesView->viewport()->installEventFilter(this);
    m_appearanceUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
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
    m_appearanceUi.boldButton->setDefaultAction(new QAction(KIcon("format-text-bold"), i18n("Bold"), this));
    m_appearanceUi.boldButton->defaultAction()->setData(QWebPage::ToggleBold);
    m_appearanceUi.italicButton->setDefaultAction(new QAction(KIcon("format-text-italic"), i18n("Italic"), this));
    m_appearanceUi.italicButton->defaultAction()->setData(QWebPage::ToggleItalic);
    m_appearanceUi.underlineButton->setDefaultAction(new QAction(KIcon("format-text-underline"), i18n("Underline"), this));
    m_appearanceUi.underlineButton->defaultAction()->setData(QWebPage::ToggleUnderline);
    m_appearanceUi.justifyLeftButton->setDefaultAction(new QAction(KIcon("format-justify-left"), i18n("Justify Left"), this));
    m_appearanceUi.justifyLeftButton->defaultAction()->setData(QWebPage::AlignLeft);
    m_appearanceUi.justifyCenterButton->setDefaultAction(new QAction(KIcon("format-justify-center"), i18n("Justify Center"), this));
    m_appearanceUi.justifyCenterButton->defaultAction()->setData(QWebPage::AlignCenter);
    m_appearanceUi.justifyRightButton->setDefaultAction(new QAction(KIcon("format-justify-right"), i18n("Justify Right"), this));
    m_appearanceUi.justifyRightButton->defaultAction()->setData(QWebPage::AlignRight);
    m_appearanceUi.backgroundButton->setIcon(KIcon("games-config-background"));
    m_appearanceUi.componentButton->setIcon(KIcon("chronometer"));

    m_clipboardUi.moveUpButton->setIcon(KIcon("arrow-up"));
    m_clipboardUi.moveDownButton->setIcon(KIcon("arrow-down"));
    m_clipboardUi.actionsView->setModel(m_actionsModel);
    m_clipboardUi.actionsView->setItemDelegate(new ExpressionDelegate(m_applet->getClock(), this));
    m_clipboardUi.actionsView->viewport()->installEventFilter(this);
    m_clipboardUi.fastCopyExpressionEdit->setText(m_applet->config().readEntry("fastCopyExpression", "Clock.getValue(Clock.Year) + '-' + Clock.getValue(Clock.Month) + '-' + Clock.getValue(Clock.DayOfMonth) + ' ' + Clock.getValue(Clock.Hour) + ':' + Clock.getValue(Clock.Minute) + ':' + Clock.getValue(Clock.Second)"));
    m_clipboardUi.fastCopyExpressionEdit->setClock(m_applet->getClock());

    parent->addPage(appearanceConfiguration, i18n("Appearance"), "preferences-desktop-theme");
    parent->addPage(clipboardConfiguration, i18n("Clipboard actions"), "edit-copy");
    parent->resize(500, 400);

    connect(parent, SIGNAL(applyClicked()), this, SLOT(save()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(save()));
    connect(m_actionsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(modify()));
    connect(m_appearanceUi.mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(appearanceModeChanged(int)));
    connect(m_appearanceUi.editorTabWidget, SIGNAL(currentChanged(int)), this, SLOT(editorModeChanged(int)));
    connect(m_appearanceUi.themesView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTheme(QModelIndex)));
    connect(m_appearanceUi.newButton, SIGNAL(clicked()), this, SLOT(createTheme()));
    connect(m_appearanceUi.copyButton, SIGNAL(clicked()), this, SLOT(copyTheme()));
    connect(m_appearanceUi.renameButton, SIGNAL(clicked()), this, SLOT(renameTheme()));
    connect(m_appearanceUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteTheme()));
    connect(m_appearanceUi.webView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showEditorContextMenu(QPoint)));
    connect(m_appearanceUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(richTextChanged()));
    connect(m_appearanceUi.editorTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
    connect(m_appearanceUi.zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)));
    connect(m_appearanceUi.boldButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.italicButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.underlineButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked(bool)), this, SLOT(setBackground(bool)));
    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));
    connect(m_appearanceUi.componentButton, SIGNAL(toggled(bool)), this, SLOT(insertComponent(bool)));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertAction()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteAction()));
    connect(m_clipboardUi.editButton, SIGNAL(clicked()), this, SLOT(editAction()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpAction()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownAction()));
    connect(m_clipboardUi.actionsView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectAction(QModelIndex)));
    connect(m_clipboardUi.actionsView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editAction(QModelIndex)));
    connect(m_clipboardUi.fastCopyExpressionEdit, SIGNAL(textChanged(QString)), this, SLOT(modify()));
    connect(delegate, SIGNAL(showOptions()), this, SLOT(showOptions()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), delegate, SLOT(clear()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), m_appearanceUi.themesView->viewport(), SLOT(repaint()));
    connect(this, SIGNAL(clearCache()), delegate, SLOT(clear()));
}

void Configuration::save()
{
    if (m_appearanceUi.mainTabWidget->currentIndex() == 1) {
        if (m_appearanceUi.editorTabWidget->currentIndex() == 1) {
            sourceChanged();
        }

        editorModeChanged(m_appearanceUi.mainTabWidget->currentIndex());
    }

    if (m_editedAction.isValid()) {
        m_clipboardUi.actionsView->closePersistentEditor(m_editedAction);
    }

    QStringList clipboardExpressions;

    for (int i = 0; i < m_actionsModel->rowCount(); ++i) {
        clipboardExpressions.append(m_actionsModel->index(i, 0).data(Qt::EditRole).toString());
    }

    m_applet->config().writeEntry("theme", m_appearanceUi.themesView->currentIndex().data(IdRole).toString());
    m_applet->config().writeEntry("clipboardExpressions", clipboardExpressions);
    m_applet->config().writeEntry("fastCopyExpression", m_clipboardUi.fastCopyExpressionEdit->text());

    const QString dataPath = KStandardDirs::locateLocal("data", "plasma/adjustableclock");
    QStringList failed;

    for (int i = 0; i < m_themesModel->rowCount(); ++i) {
        const QModelIndex index = m_themesModel->index(i, 0);

        if (!index.data(ModificationRole).toBool()) {
            continue;
        }

        const QString packagePath = QString("%1/%2/").arg(index.data(PathRole).isValid() ? index.data(PathRole).toString() : dataPath).arg(index.data(IdRole).toString());
        const QString dataPath = QString(packagePath).append("contents/ui/");

        if (!QFile::exists(dataPath)) {
            QDir(dataPath).mkpath(dataPath);
        }

        QFile file(dataPath + "main.html");

        if (!file.open(QIODevice::WriteOnly)) {
            failed.append(index.data(TitleRole).toString());

            continue;
        }

        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << index.data(HtmlRole).toString();

        KDesktopFile desktopFile("data", (packagePath + "metadata.desktop"));
        desktopFile.desktopGroup().writeEntry("Name", index.data(TitleRole).toString());
        desktopFile.desktopGroup().writeEntry("Comment", QString());
        desktopFile.desktopGroup().writeEntry("Type", "Service");
        desktopFile.desktopGroup().writeEntry("ServiceTypes", "Plasma/AdjustableClock");
        desktopFile.desktopGroup().writeEntry("X-Plasma-API", "adjustableclock_html");
        desktopFile.desktopGroup().writeEntry("X-KDE-Library", ("adjustableclock_theme_" + index.data(IdRole).toString()));
        desktopFile.desktopGroup().writeEntry("X-KDE-PluginInfo-Name", index.data(IdRole).toString());
    }

    if (!failed.isEmpty()) {
        KMessageBox::errorList(m_appearanceUi.themesView, i18n("Failed saving the following themes:"), failed);
    }

    static_cast<KConfigDialog*>(parent())->enableButtonApply(false);

    emit accepted();
}

void Configuration::modify()
{
    static_cast<KConfigDialog*>(parent())->enableButtonApply(true);
}

void Configuration::selectTheme(const QModelIndex &index)
{
    if (m_appearanceUi.themesView->selectionModel()->hasSelection()) {
        modify();
    }

    const bool writable = (index.data(ModificationRole).toBool() || QFileInfo(index.data(PathRole).toString()).isWritable());

    m_appearanceUi.themesView->setCurrentIndex(index);
    m_appearanceUi.themesView->scrollTo(index, QAbstractItemView::EnsureVisible);
    m_appearanceUi.deleteButton->setEnabled(writable);
    m_appearanceUi.renameButton->setEnabled(writable);
    m_appearanceUi.mainTabWidget->setTabEnabled(1, writable);
}

void Configuration::createTheme()
{
    bool ok;
    const QString title = KInputDialog::getText(i18n("Add new theme"), i18n("Theme name:"), i18n("New Theme"), &ok);

    if (!ok) {
        return;
    }

    QStandardItem *item = new QStandardItem();
    item->setData(createIdentifier(), IdRole);
    item->setData(title, TitleRole);
    item->setData(true, ModificationRole);
    item->setToolTip(QString("<b>%1</b>").arg(title));

    m_themesModel->appendRow(item);

    selectTheme(item->index());
}

void Configuration::copyTheme()
{
    const QModelIndex index = m_appearanceUi.themesView->currentIndex();
    QString title = index.data(TitleRole).toString().append(" (%1)");
    int i = 2;

    while (findRow(title.arg(i)) >= 0) {
       ++i;
    }

    title = title.arg(i);

    bool ok;

    title = KInputDialog::getText(i18n("Add new theme"), i18n("Theme name:"), title, &ok);

    if (!ok) {
        return;
    }

    QStandardItem *item = new QStandardItem();
    item->setData(createIdentifier(), IdRole);
    item->setData(title, TitleRole);
    item->setData(index.data(HtmlRole), HtmlRole);
    item->setToolTip(QString("<b>%1</b>").arg(title));

    m_themesModel->appendRow(item);

    selectTheme(item->index());
}

void Configuration::deleteTheme()
{
    const QModelIndex index = m_appearanceUi.themesView->currentIndex();

    if (KMessageBox::questionYesNo(m_appearanceUi.themesView, i18n("Do you really want to delete theme \"%1\"?").arg(index.data(TitleRole).toString()), i18n("Delete Theme")) == KMessageBox::Yes) {
        const int row = index.row();

        if (!Plasma::Package::uninstallPackage(index.data(IdRole).toString(), index.data(PathRole).toString(), "plasma-adjustable-clock-addon-")) {
            KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to delete theme."));

            return;
        }

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

    m_themesModel->setData(m_appearanceUi.themesView->currentIndex(), title, TitleRole);

    modify();

    emit clearCache();
}

void Configuration::triggerAction()
{
    QToolButton *button = qobject_cast<QToolButton*>(sender());

    if (!button) {
        return;
    }

    const QWebPage::WebAction action = static_cast<QWebPage::WebAction>(button->defaultAction()->data().toInt());

    if (action == QWebPage::ToggleBold) {
        setStyle("font-weight", (button->isChecked() ? "normal" : "bold"));
    } else if (action == QWebPage::ToggleItalic) {
        setStyle("font-style", (button->isChecked() ? "normal" : "italic"));
    } else if (action == QWebPage::ToggleItalic) {
        setStyle("text-decoration", (button->isChecked() ? "none" : "underline"));
    } else {
        if (m_appearanceUi.editorTabWidget->currentIndex() == 0) {
            m_appearanceUi.webView->page()->triggerAction(action);
        } else {
            setStyle("text-align", ((action == QWebPage::AlignLeft) ? "left" : ((action == QWebPage::AlignRight) ? "right" : "center")), "div");
        }
    }
}

void Configuration::insertComponent(bool show)
{
    if (!m_componentWidget) {
        m_componentWidget = new ComponentWidget(m_applet->getClock(), m_appearanceUi.componentButton);

        m_appearanceUi.headerLayout->addWidget(m_componentWidget);

        connect(m_componentWidget, SIGNAL(insertComponent(QString,QString)), this, SLOT(insertComponent(QString,QString)));
    }

    m_componentWidget->setVisible(show);
}

void Configuration::insertComponent(const QString &component, const QString &options)
{
    const QString title = Clock::getComponentName(static_cast<ClockComponent>(m_applet->getClock()->evaluate(QString("Clock.%1").arg(component)).toInt()));
    const QString value = m_applet->getClock()->evaluate((options.isEmpty() ? QString("Clock.getValue(Clock.%1)").arg(component) : QString("Clock.getValue(Clock.%1, {%2})").arg(component).arg(options)), true);

    if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
        const QString html = (options.isEmpty() ? QString("<span component=\"%1\" title=\"%2\">%3</span>").arg(component).arg(title).arg(value) : QString("<span component=\"%1\" options=\"%2\" title=\"%3\">%4</span>").arg(component).arg(options).arg(title).arg(value));

        if (m_document) {
            m_document->activeView()->insertText(html);
        } else {
            m_appearanceUi.editorTextEdit->insertPlainText(html);
        }

        sourceChanged();
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("insertComponent('%1', '%2', '%3', '%4')").arg(component).arg(QString(options).replace(QRegExp("'([a-z]+)'"), "\\'\\1\\'")).arg(title).arg(value));
    }
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

    m_appearanceUi.boldButton->setChecked(m_appearanceUi.webView->page()->action(QWebPage::ToggleBold)->isChecked());
    m_appearanceUi.italicButton->setChecked(m_appearanceUi.webView->page()->action(QWebPage::ToggleItalic)->isChecked());
    m_appearanceUi.underlineButton->setChecked(m_appearanceUi.webView->page()->action(QWebPage::ToggleUnderline)->isChecked());
}

void Configuration::appearanceModeChanged(int mode)
{
    if (mode == 0) {
        return;
    }

    KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();

    if (editor) {
        if (m_document) {
            m_document->activeView()->deleteLater();
            m_document->deleteLater();
        }

        m_document = editor->createDocument(this);
        m_document->setHighlightingMode("html");

        KTextEditor::View *view = m_document->createView(m_appearanceUi.sourceTab);
        view->setContextMenu(view->defaultContextMenu());

        KTextEditor::ConfigInterface *configuration = qobject_cast<KTextEditor::ConfigInterface*>(view);

        if (configuration) {
            configuration->setConfigValue("line-numbers", true);
            configuration->setConfigValue("folding-bar", false);
            configuration->setConfigValue("dynamic-word-wrap", false);
        }

        m_appearanceUi.editorTextEdit->hide();
        m_appearanceUi.sourceLayout->addWidget(view);
    }

    const QModelIndex index = m_appearanceUi.themesView->currentIndex();

    if (m_document) {
        m_document->setText(index.data(HtmlRole).toString());
    } else {
        disconnect(m_appearanceUi.editorTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));

        m_appearanceUi.editorTextEdit->setPlainText(index.data(HtmlRole).toString());
    }

    m_appearanceUi.backgroundButton->setChecked(m_appearanceUi.webView->page()->mainFrame()->findFirstElement("body").attribute("background").toLower() == "true");

    sourceChanged();

    if (m_document) {
        connect(m_document, SIGNAL(textChanged(KTextEditor::Document*)), this, SLOT(themeChanged()));
    } else {
        connect(m_appearanceUi.editorTextEdit, SIGNAL(textChanged()), this, SLOT(themeChanged()));
    }

    editorModeChanged(m_appearanceUi.editorTabWidget->currentIndex());
}

void Configuration::editorModeChanged(int mode)
{
    m_appearanceUi.boldButton->setCheckable(mode == 0);
    m_appearanceUi.italicButton->setCheckable(mode == 0);
    m_appearanceUi.underlineButton->setCheckable(mode == 0);

    if (mode == 1) {
        richTextChanged();
    } else {
        sourceChanged();

        m_appearanceUi.webView->setFocus(Qt::OtherFocusReason);

        QMouseEvent event(QEvent::MouseButtonPress, QPoint(5, 5), Qt::LeftButton, Qt::LeftButton, 0);

        QCoreApplication::sendEvent(m_appearanceUi.webView, &event);
    }
}

void Configuration::themeChanged()
{
    m_themesModel->setData(m_appearanceUi.themesView->currentIndex(), (m_document ? m_document->text() : m_appearanceUi.editorTextEdit->toPlainText()), HtmlRole);
    m_themesModel->setData(m_appearanceUi.themesView->currentIndex(), true, ModificationRole);

    modify();

    emit clearCache();
}

void Configuration::richTextChanged()
{
    QWebPage page;
    page.mainFrame()->setHtml(m_appearanceUi.webView->page()->mainFrame()->toHtml());

    const QWebElementCollection elements = page.mainFrame()->findAllElements("[component]");

    for (int i = 0; i < elements.count(); ++i) {
        elements.at(i).removeAttribute("title");
    }

    const QString html = page.mainFrame()->toHtml().remove(QRegExp("<head></head>"));

    if (m_document) {
        m_document->setText(html);
    } else {
        m_appearanceUi.editorTextEdit->setPlainText(html);
    }
}

void Configuration::sourceChanged()
{
    QFile file(":/editor.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    Clock::setupClock(m_appearanceUi.webView->page()->mainFrame(), m_applet->getClock()->getClock(true), (m_document ? m_document->text() : m_appearanceUi.editorTextEdit->toPlainText()));

    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(stream.readAll());

    const QWebElementCollection elements = m_appearanceUi.webView->page()->mainFrame()->findAllElements("[component]");

    for (int i = 0; i < elements.count(); ++i) {
        elements.at(i).setAttribute("title", Clock::getComponentName(static_cast<ClockComponent>(m_applet->getClock()->evaluate(QString("Clock.%1").arg(elements.at(i).attribute("component"))).toInt())));
    }
}

void Configuration::showOptions()
{
    const QString theme = m_appearanceUi.themesView->currentIndex().data(IdRole).toString();

    if (!m_options.contains(theme) || m_options[theme].isEmpty()) {
        return;
    }

    QWidget *mainWidget = new QWidget();
    QFormLayout *layout = new QFormLayout(mainWidget);
    QHash<QString, OptionWidget*> widgets;

    for (int i = 0; i < m_options[theme].count(); ++i)
    {
        const Option option = m_options[theme].at(i);

        widgets[option.id] = new OptionWidget(option, mainWidget);

        layout->addRow(i18n(option.title.toUtf8().data()), widgets[option.id]);
    }

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    KDialog dialog;
    dialog.setMainWidget(mainWidget);
    dialog.setModal(true);
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setWindowTitle(i18n("Theme Options"));

    if (dialog.exec() == QDialog::Rejected) {
        return;
    }

    for (int i = 0; i < m_options[theme].count(); ++i)
    {
        m_options[theme][i].value = widgets[m_options[theme][i].id]->getValue();
    }
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

void Configuration::selectAction(const QModelIndex &index)
{
    if (index != m_clipboardUi.actionsView->currentIndex()) {
        m_clipboardUi.actionsView->setCurrentIndex(index);
    }

    m_clipboardUi.moveUpButton->setEnabled(index.isValid() && index.row() != 0);
    m_clipboardUi.moveDownButton->setEnabled(index.isValid() && index.row() != (m_actionsModel->rowCount() - 1));
    m_clipboardUi.editButton->setEnabled(index.isValid());
    m_clipboardUi.deleteButton->setEnabled(index.isValid());
}

void Configuration::editAction(QModelIndex index)
{
    if (m_editedAction.isValid()) {
        m_clipboardUi.actionsView->closePersistentEditor(m_editedAction);
    }

    if (!index.isValid()) {
        index = m_clipboardUi.actionsView->currentIndex();
    }

    if (!index.isValid()) {
        return;
    }

    m_editedAction = index;

    m_clipboardUi.actionsView->openPersistentEditor(m_editedAction);
}

void Configuration::insertAction()
{
    const int row = (m_clipboardUi.actionsView->currentIndex().row() + 1);

    if (!m_actionsModel->insertRow(row)) {
        return;
    }

    const QModelIndex index = m_actionsModel->index(row, 0);

    selectAction(index);
    editAction(index);
    modify();
}

void Configuration::deleteAction()
{
    m_actionsModel->removeRow(m_clipboardUi.actionsView->currentIndex().row());

    modify();
}

void Configuration::moveAction(bool up)
{
    const int sourceRow = m_clipboardUi.actionsView->currentIndex().row();
    const int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));
    QStandardItem *item = m_actionsModel->takeItem(sourceRow);

    m_actionsModel->setItem(sourceRow, m_actionsModel->takeItem(destinationRow));
    m_actionsModel->setItem(destinationRow, item);

    selectAction(m_actionsModel->index(destinationRow, 0));
    modify();
}

void Configuration::moveUpAction()
{
    moveAction(true);
}

void Configuration::moveDownAction()
{
    moveAction(false);
}

void Configuration::setStyle(const QString &property, const QString &value, const QString &tag)
{
    if (m_appearanceUi.editorTabWidget->currentIndex() > 0) {
        const QString html = QString("<%1 style=\"%2:%3;\">%4</%1>").arg(tag).arg(property).arg(value).arg(m_document ? m_document->activeView()->selectionText() : m_appearanceUi.editorTextEdit->textCursor().selectedText());

        if (m_document) {
            m_document->activeView()->insertText(html);
        } else {
            m_appearanceUi.editorTextEdit->insertPlainText(html);
        }
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QString("setStyle('%1', '%2')").arg(property).arg(QString(value).replace(QRegExp("'([a-z]+)'"), "\\'\\1\\'")));
    }

    modify();
}

void Configuration::setBackground(bool enabled)
{
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(enabled ? "document.body.setAttribute('background', 'true')" : "document.body.removeAttribute('background')");
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

        setStyle("color", colorDialog.color().name());
    }
}

void Configuration::setFontSize(const QString &size)
{
    setStyle("font-size", QString("%1px").arg(size));
}

void Configuration::setFontFamily(const QFont &font)
{
    setStyle("font-family", font.family());
}

void Configuration::setZoom(int zoom)
{
    m_appearanceUi.webView->setZoomFactor((qreal) zoom / 100);
    m_appearanceUi.zoomSlider->setToolTip(i18n("Zoom: %1%").arg(zoom));
}

QString Configuration::createIdentifier() const
{
    QString identifier = QString("custom_%1");
    int i = 1;

    while (findRow(identifier.arg(i), IdRole) >= 0) {
       ++i;
    }

    return identifier.arg(i);
}

int Configuration::findRow(const QString &text, int role) const
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
        selectTheme(m_themesModel->index(qMax(findRow(m_applet->config().readEntry("theme", "default"), IdRole), 0), 0));
    } else if (event->type() == QEvent::MouseButtonDblClick && object == m_appearanceUi.themesView->viewport()) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (!m_appearanceUi.themesView->indexAt(mouseEvent->pos()).isValid()) {
            createTheme();
        }
    } else if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) && object == m_clipboardUi.actionsView->viewport()) {
        if (event->type() == QEvent::MouseButtonPress && m_editedAction.isValid()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (m_clipboardUi.actionsView->indexAt(mouseEvent->pos()) != m_editedAction) {
                m_clipboardUi.actionsView->closePersistentEditor(m_editedAction);
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (!m_clipboardUi.actionsView->indexAt(mouseEvent->pos()).isValid()) {
                insertAction();
            }
        }
    }

    return QObject::eventFilter(object, event);
}

}