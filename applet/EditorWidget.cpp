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

#include "EditorWidget.h"
#include "WebView.h"

#include <QtGui/QMouseEvent>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElementCollection>

#include <KMenu>
#include <KLocale>
#include <KColorDialog>

#include <KTextEditor/View>
#include <KTextEditor/Editor>
#include <KTextEditor/EditorChooser>
#include <KTextEditor/ConfigInterface>

namespace AdjustableClock
{

EditorWidget::EditorWidget(const QString &path, const QString &identifier, Clock *clock, QWidget *parent) : QWidget(parent),
    m_clock(clock),
    m_document(NULL),
    m_path(path)
{
    const Plasma::PackageMetadata metaData(QString("%1/%2/metadata.desktop").arg(path).arg(identifier));

    m_editorUi.setupUi(this);
    m_editorUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_editorUi.webView->page()->setContentEditable(true);
    m_editorUi.webView->page()->action(QWebPage::Undo)->setText(i18n("Undo"));
    m_editorUi.webView->page()->action(QWebPage::Undo)->setIcon(KIcon("edit-undo"));
    m_editorUi.webView->page()->action(QWebPage::Redo)->setText(i18n("Redo"));
    m_editorUi.webView->page()->action(QWebPage::Redo)->setIcon(KIcon("edit-redo"));
    m_editorUi.webView->page()->action(QWebPage::Cut)->setText(i18n("Cut"));
    m_editorUi.webView->page()->action(QWebPage::Cut)->setIcon(KIcon("edit-cut"));
    m_editorUi.webView->page()->action(QWebPage::Copy)->setText(i18n("Copy"));
    m_editorUi.webView->page()->action(QWebPage::Copy)->setIcon(KIcon("edit-copy"));
    m_editorUi.webView->page()->action(QWebPage::Paste)->setText(i18n("Paste"));
    m_editorUi.webView->page()->action(QWebPage::Paste)->setIcon(KIcon("edit-paste"));
    m_editorUi.webView->page()->action(QWebPage::SelectAll)->setText(i18n("Select All"));
    m_editorUi.webView->page()->action(QWebPage::SelectAll)->setIcon(KIcon("select-all"));
    m_editorUi.boldButton->setDefaultAction(new QAction(KIcon("format-text-bold"), i18n("Bold"), this));
    m_editorUi.boldButton->defaultAction()->setData(QWebPage::ToggleBold);
    m_editorUi.italicButton->setDefaultAction(new QAction(KIcon("format-text-italic"), i18n("Italic"), this));
    m_editorUi.italicButton->defaultAction()->setData(QWebPage::ToggleItalic);
    m_editorUi.underlineButton->setDefaultAction(new QAction(KIcon("format-text-underline"), i18n("Underline"), this));
    m_editorUi.underlineButton->defaultAction()->setData(QWebPage::ToggleUnderline);
    m_editorUi.justifyLeftButton->setDefaultAction(new QAction(KIcon("format-justify-left"), i18n("Justify Left"), this));
    m_editorUi.justifyLeftButton->defaultAction()->setData(QWebPage::AlignLeft);
    m_editorUi.justifyCenterButton->setDefaultAction(new QAction(KIcon("format-justify-center"), i18n("Justify Center"), this));
    m_editorUi.justifyCenterButton->defaultAction()->setData(QWebPage::AlignCenter);
    m_editorUi.justifyRightButton->setDefaultAction(new QAction(KIcon("format-justify-right"), i18n("Justify Right"), this));
    m_editorUi.justifyRightButton->defaultAction()->setData(QWebPage::AlignRight);
    m_editorUi.backgroundButton->setIcon(KIcon("games-config-background"));
    m_editorUi.componentWidget->setClock(m_clock);
    m_editorUi.identifierLineEdit->setText(identifier);
    m_editorUi.identifierLineEdit->setValidator(new QRegExpValidator(QRegExp("[0-9a-z\\-_]+"), this));
    m_editorUi.nameLineEdit->setText(metaData.name());
    m_editorUi.descriptionLineEdit->setText(metaData.description());
    m_editorUi.authorLineEdit->setText(metaData.author());
    m_editorUi.emailLineEdit->setText(metaData.email());
    m_editorUi.websiteLineEdit->setText(metaData.website());
    m_editorUi.licenseLineEdit->setText(metaData.license());
    m_editorUi.versionLineEdit->setText(metaData.version());

    KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();

    if (!editor) {
        m_editorUi.toolBox->setCurrentIndex(1);
        m_editorUi.toolBox->setItemEnabled(0, false);

        return;
    }

    const bool qml = QFile::exists(QString("%1/%2/contents/ui/main.qml").arg(path).arg(identifier));

    m_document = editor->createDocument(this);
    m_document->openUrl(KUrl(QString("%1/%2/contents/ui/main.%3").arg(path).arg(identifier).arg(qml ? "qml" : "html")));
    m_document->setHighlightingMode(qml ? "qml" : "html");

    KTextEditor::View *view = m_document->createView(m_editorUi.sourceTab);
    view->setContextMenu(view->defaultContextMenu());

    KTextEditor::ConfigInterface *configuration = qobject_cast<KTextEditor::ConfigInterface*>(view);

    if (configuration) {
        configuration->setConfigValue("line-numbers", true);
        configuration->setConfigValue("folding-bar", false);
        configuration->setConfigValue("dynamic-word-wrap", false);
    }

    m_editorUi.sourceLayout->addWidget(view);

    sourceChanged(m_document->text());

    m_editorUi.backgroundButton->setChecked(m_editorUi.webView->page()->mainFrame()->findFirstElement("body").attribute("background").toLower() == "true");

    connect(m_editorUi.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(modeChanged(int)));
    connect(m_editorUi.webView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(m_editorUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_editorUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(richTextChanged()));
    connect(m_editorUi.zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)));
    connect(m_editorUi.boldButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.italicButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.underlineButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_editorUi.backgroundButton, SIGNAL(clicked(bool)), this, SLOT(setBackground(bool)));
    connect(m_editorUi.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(m_editorUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    connect(m_editorUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));
    connect(m_editorUi.componentWidget, SIGNAL(insertComponent(QString,QString)), this, SLOT(insertComponent(QString,QString)));
}

void EditorWidget::triggerAction()
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
        if (m_editorUi.tabWidget->currentIndex() == 0) {
            m_editorUi.webView->page()->triggerAction(action);
        } else {
            setStyle("text-align", ((action == QWebPage::AlignLeft) ? "left" : ((action == QWebPage::AlignRight) ? "right" : "center")), "div");
        }
    }
}

void EditorWidget::insertComponent(const QString &component, const QString &options)
{
    const QString title = Clock::getComponentName(static_cast<ClockComponent>(m_clock->evaluate(QString("Clock.%1").arg(component)).toInt()));
    const QString value = m_clock->evaluate((options.isEmpty() ? QString("Clock.getValue(Clock.%1)").arg(component) : QString("Clock.getValue(Clock.%1, {%2})").arg(component).arg(options)), true);

    if (m_editorUi.tabWidget->currentIndex() > 0) {
        const QString html = (options.isEmpty() ? QString("<span component=\"%1\" title=\"%2\">%3</span>").arg(component).arg(title).arg(value) : QString("<span component=\"%1\" options=\"%2\" title=\"%3\">%4</span>").arg(component).arg(options).arg(title).arg(value));

        if (m_document) {
            m_document->activeView()->insertText(html);
        }

        sourceChanged();
    } else {
        m_editorUi.webView->page()->mainFrame()->evaluateJavaScript(QString("insertComponent('%1', '%2', '%3', '%4')").arg(component).arg(QString(options).replace(QRegExp("'([a-z]+)'"), "\\'\\1\\'")).arg(title).arg(value));
    }
}

void EditorWidget::selectionChanged()
{
    m_editorUi.webView->page()->mainFrame()->evaluateJavaScript("fixSelection()");

    QRegExp expression = QRegExp("rgb\\((\\d+), (\\d+), (\\d+)\\)");
    expression.indexIn(m_editorUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('color')").toString());

    const QStringList rgb = expression.capturedTexts();

    QPalette palette = m_editorUi.colorButton->palette();
    palette.setBrush(QPalette::Button, QColor(rgb.at(1).toInt(), rgb.at(2).toInt(), rgb.at(3).toInt()));

    m_editorUi.colorButton->setPalette(palette);

    disconnect(m_editorUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    disconnect(m_editorUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));

    m_editorUi.fontSizeComboBox->setEditText(m_editorUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('font-size')").toString().remove("px"));
    m_editorUi.fontFamilyComboBox->setCurrentFont(QFont(m_editorUi.webView->page()->mainFrame()->evaluateJavaScript("getStyle('font-family')").toString()));

    connect(m_editorUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(setFontSize(QString)));
    connect(m_editorUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(setFontFamily(QFont)));

    m_editorUi.boldButton->setChecked(m_editorUi.webView->page()->action(QWebPage::ToggleBold)->isChecked());
    m_editorUi.italicButton->setChecked(m_editorUi.webView->page()->action(QWebPage::ToggleItalic)->isChecked());
    m_editorUi.underlineButton->setChecked(m_editorUi.webView->page()->action(QWebPage::ToggleUnderline)->isChecked());
}

void EditorWidget::modeChanged(int mode)
{
    m_editorUi.boldButton->setCheckable(mode == 0);
    m_editorUi.italicButton->setCheckable(mode == 0);
    m_editorUi.underlineButton->setCheckable(mode == 0);

    if (mode == 1) {
        richTextChanged();
    } else {
        sourceChanged();

        m_editorUi.webView->setFocus(Qt::OtherFocusReason);

        QMouseEvent event(QEvent::MouseButtonPress, QPoint(5, 5), Qt::LeftButton, Qt::LeftButton, 0);

        QCoreApplication::sendEvent(m_editorUi.webView, &event);
    }
}

void EditorWidget::richTextChanged()
{
    QWebPage page;
    page.mainFrame()->setHtml(m_editorUi.webView->page()->mainFrame()->toHtml());
    page.mainFrame()->findFirstElement("#theme_css").removeFromDocument();

    const QWebElementCollection elements = page.mainFrame()->findAllElements("[component]");

    for (int i = 0; i < elements.count(); ++i) {
        elements.at(i).removeAttribute("title");
    }

    const QString html = page.mainFrame()->toHtml();

    if (m_document) {
        m_document->setText(html);
    }
}

void EditorWidget::sourceChanged(const QString &theme)
{
    QFile file(":/editor.js");
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    WebView::setupClock(m_editorUi.webView->page()->mainFrame(), new ClockObject(m_clock, true), (m_document ? m_document->text() : theme), "[component] {-webkit-transition: background 0.2s;} [component]:hover {background: rgba(252, 255, 225, 0.8); box-shadow: 0 0 0 2px #F5C800;}");

    m_editorUi.webView->page()->mainFrame()->evaluateJavaScript(stream.readAll());

    const QWebElementCollection elements = m_editorUi.webView->page()->mainFrame()->findAllElements("[component]");

    for (int i = 0; i < elements.count(); ++i) {
        elements.at(i).setAttribute("title", Clock::getComponentName(static_cast<ClockComponent>(m_clock->evaluate(QString("Clock.%1").arg(elements.at(i).attribute("component"))).toInt())));
    }
}

void EditorWidget::showContextMenu(const QPoint &position)
{
    KMenu menu(m_editorUi.webView);
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::Undo));
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::Redo));
    menu.addSeparator();
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::Cut));
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::Copy));
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::Paste));
    menu.addAction(m_editorUi.webView->page()->action(QWebPage::SelectAll));
    menu.addSeparator();
    menu.addAction(m_editorUi.boldButton->defaultAction());
    menu.addAction(m_editorUi.italicButton->defaultAction());
    menu.addAction(m_editorUi.underlineButton->defaultAction());
    menu.addSeparator();
    menu.addAction(m_editorUi.justifyLeftButton->defaultAction());
    menu.addAction(m_editorUi.justifyCenterButton->defaultAction());
    menu.addAction(m_editorUi.justifyRightButton->defaultAction());
    menu.exec(m_editorUi.webView->mapToGlobal(position));
}

void EditorWidget::setStyle(const QString &property, const QString &value, const QString &tag)
{
    if (m_editorUi.tabWidget->currentIndex() > 0 && m_document) {
        m_document->activeView()->insertText(QString("<%1 style=\"%2:%3;\">%4</%1>").arg(tag).arg(property).arg(value).arg(m_document->activeView()->selectionText()));
    } else {
        m_editorUi.webView->page()->mainFrame()->evaluateJavaScript(QString("setStyle('%1', '%2')").arg(property).arg(QString(value).replace(QRegExp("'([a-z]+)'"), "\\'\\1\\'")));
    }
}

void EditorWidget::setBackground(bool enabled)
{
    m_editorUi.webView->page()->mainFrame()->evaluateJavaScript(enabled ? "document.body.setAttribute('background', 'true')" : "document.body.removeAttribute('background')");
}

void EditorWidget::setColor()
{
    KColorDialog colorDialog;
    colorDialog.setAlphaChannelEnabled(true);
    colorDialog.setColor(m_editorUi.colorButton->palette().button().color());
    colorDialog.setButtons(KDialog::Ok | KDialog::Cancel);

    if (colorDialog.exec() == QDialog::Accepted) {
        QPalette palette = m_editorUi.colorButton->palette();
        palette.setBrush(QPalette::Button, colorDialog.color());

        m_editorUi.colorButton->setPalette(palette);

        setStyle("color", colorDialog.color().name());
    }
}

void EditorWidget::setFontSize(const QString &size)
{
    setStyle("font-size", QString("%1px").arg(size));
}

void EditorWidget::setFontFamily(const QFont &font)
{
    setStyle("font-family", font.family());
}

void EditorWidget::setZoom(int zoom)
{
    m_editorUi.webView->setZoomFactor((qreal) zoom / 100);
    m_editorUi.zoomSlider->setToolTip(i18n("Zoom: %1%").arg(zoom));
}

bool EditorWidget::saveTheme() const
{
    if (m_document) {
        return m_document->documentSave();
    }

    return true;
}

QString EditorWidget::getIdentifier() const
{
    return m_editorUi.identifierLineEdit->text();
}

Plasma::PackageMetadata EditorWidget::getMetaData() const
{
    Plasma::PackageMetadata metaData;
    metaData.setName(m_editorUi.nameLineEdit->text());
    metaData.setDescription(m_editorUi.descriptionLineEdit->text());
    metaData.setAuthor(m_editorUi.authorLineEdit->text());
    metaData.setEmail(m_editorUi.emailLineEdit->text());
    metaData.setWebsite(m_editorUi.websiteLineEdit->text());
    metaData.setLicense(m_editorUi.licenseLineEdit->text());
    metaData.setVersion(m_editorUi.versionLineEdit->text());

    return metaData;
}

}