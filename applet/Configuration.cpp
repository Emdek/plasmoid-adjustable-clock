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
#include "Applet.h"
#include "Clock.h"
#include "EditorWidget.h"
#include "OptionWidget.h"
#include "ThemeDelegate.h"
#include "ExpressionDelegate.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QFormLayout>

#include <KMenu>
#include <KLocale>
#include <KFileDialog>
#include <KMessageBox>
#include <KInputDialog>
#include <KStandardDirs>
#include <KAboutApplicationDialog>

#include <Plasma/Theme>
#include <Plasma/ConfigLoader>

namespace AdjustableClock
{

Configuration::Configuration(Applet *applet, KConfigDialog *parent) : QObject(parent),
    m_applet(applet),
    m_themesModel(new QStandardItemModel(this)),
    m_actionsModel(new QStandardItemModel(this))
{
    QWidget *appearanceConfiguration = new QWidget();
    QWidget *clipboardConfiguration = new QWidget();

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardConfiguration);

    const QStringList locations = KGlobal::dirs()->findDirs("data", "plasma/adjustableclock");

    for (int i = 0; i < locations.count(); ++i) {
        const QStringList themes = Plasma::Package::listInstalled(locations.at(i));

        for (int j = 0; j < themes.count(); ++j) {
            loadTheme(locations.at(i), themes.at(j));
        }
    }

    m_themesModel->setSortRole(SortRole);
    m_themesModel->sort(0);

    const QStringList clipboardExpressions = m_applet->getClipboardExpressions();

    for (int i = 0; i < clipboardExpressions.count(); ++i) {
        QStandardItem *item = new QStandardItem(clipboardExpressions.at(i));

        if (!clipboardExpressions.at(i).isEmpty()) {
            item->setToolTip(m_applet->getClock()->evaluate(clipboardExpressions.at(i), true));
        }

        m_actionsModel->appendRow(item);
    }

    ThemeDelegate *delegate = new ThemeDelegate(m_applet->getClock(), m_appearanceUi.themesView);

    m_appearanceUi.themesView->setModel(m_themesModel);
    m_appearanceUi.themesView->setItemDelegate(delegate);
    m_appearanceUi.themesView->installEventFilter(this);
    m_appearanceUi.themesView->viewport()->installEventFilter(this);

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

    for (int i = 0; i < m_themesModel->rowCount(); ++i) {
        m_appearanceUi.themesView->openPersistentEditor(m_themesModel->index(i, 0));
    }

    selectAction(m_actionsModel->index(0, 0));

    connect(parent, SIGNAL(applyClicked()), this, SLOT(save()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(save()));
    connect(m_actionsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(modify()));
    connect(m_appearanceUi.themesView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectTheme(QModelIndex)));
    connect(m_appearanceUi.themesView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(m_appearanceUi.installButton, SIGNAL(clicked()), this, SLOT(installTheme()));
    connect(m_appearanceUi.createButton, SIGNAL(clicked()), this, SLOT(createTheme()));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertAction()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteAction()));
    connect(m_clipboardUi.editButton, SIGNAL(clicked()), this, SLOT(editAction()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveUpAction()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveDownAction()));
    connect(m_clipboardUi.actionsView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectAction(QModelIndex)));
    connect(m_clipboardUi.actionsView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editAction(QModelIndex)));
    connect(m_clipboardUi.fastCopyExpressionEdit, SIGNAL(textChanged(QString)), this, SLOT(modify()));
    connect(delegate, SIGNAL(showAbout(QString)), this, SLOT(aboutTheme(QString)));
    connect(delegate, SIGNAL(showEditor(QString)), this, SLOT(editTheme(QString)));
    connect(delegate, SIGNAL(showOptions(QString)), this, SLOT(configureTheme(QString)));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), delegate, SLOT(clear()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), m_appearanceUi.themesView->viewport(), SLOT(repaint()));
    connect(this, SIGNAL(clearCache()), delegate, SLOT(clear()));
}

void Configuration::save()
{
    if (m_editedAction.isValid()) {
        m_clipboardUi.actionsView->closePersistentEditor(m_editedAction);
    }

    QStringList clipboardExpressions;

    for (int i = 0; i < m_actionsModel->rowCount(); ++i) {
        clipboardExpressions.append(m_actionsModel->index(i, 0).data(Qt::EditRole).toString());
    }

    m_applet->config().writeEntry("theme", m_appearanceUi.themesView->currentIndex().data(IdentifierRole).toString());
    m_applet->config().writeEntry("clipboardExpressions", clipboardExpressions);
    m_applet->config().writeEntry("fastCopyExpression", m_clipboardUi.fastCopyExpressionEdit->text());

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

    if (index != m_appearanceUi.themesView->currentIndex()) {
        m_appearanceUi.themesView->setCurrentIndex(index);
    }

    m_appearanceUi.themesView->setCurrentIndex(index);
    m_appearanceUi.themesView->scrollTo(index, QAbstractItemView::EnsureVisible);
}

void Configuration::installTheme()
{
    KFileDialog installDialog(KUrl("~/"), QString(), NULL);
    installDialog.setWindowModality(Qt::NonModal);
    installDialog.setMode(KFile::File);
    installDialog.setOperationMode(KFileDialog::Opening);

    if (installDialog.exec() == QDialog::Rejected) {
        return;
    }

    const QString path = KStandardDirs::locateLocal("data", "plasma/adjustableclock");

    if (Plasma::Package::installPackage(installDialog.selectedFile(), path, QString())) {
        const QStringList themes = Plasma::Package::listInstalled(path);

        for (int i = 0; i < themes.count(); ++i) {
            if (findRow(themes.at(i), IdentifierRole) < 0) {
                loadTheme(path, themes.at(i));

                const QModelIndex index = m_themesModel->index((m_themesModel->rowCount() - 1), 0);

                selectTheme(index);

                m_appearanceUi.themesView->openPersistentEditor(index);

                return;
            }
        }
    } else {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to install theme."));
    }
}

void Configuration::createTheme()
{
    QString title = i18n("New Theme");

    if (findRow(title) >= 0) {
        int i = 2;

        title.append(" (%1)");

        while (findRow(title.arg(i)) >= 0) {
            ++i;
        }

        title = title.arg(i);
    }

    bool ok;

    title = KInputDialog::getText(i18n("Create Theme"), i18n("Theme name:"), title, &ok);

    if (!ok) {
        return;
    }

    const QString identifier = createIdentifier();
    const QString path = KStandardDirs::locateLocal("data", "plasma/adjustableclock");
    QStandardItem *item = new QStandardItem();
    item->setData(identifier, IdentifierRole);
    item->setData(path, PathRole);
    item->setData(title, NameRole);
    item->setData(true, WritableRole);

    Plasma::PackageMetadata metaData = getMetaData(path, identifier);
    metaData.setName(title);

    m_themesModel->appendRow(item);
    m_appearanceUi.themesView->openPersistentEditor(item->index());

    saveTheme(path, identifier, metaData);
    selectTheme(item->index());
    editTheme(identifier);
}

void Configuration::copyTheme()
{
    copyTheme(m_themesModel->item(m_appearanceUi.themesView->currentIndex().row()));
}

void Configuration::exportTheme()
{
    const QModelIndex index = m_appearanceUi.themesView->currentIndex();

    KFileDialog exportDialog(KUrl(QString("~/%1.zip").arg(index.data(IdentifierRole).toString())), QString(), NULL);
    exportDialog.setWindowModality(Qt::NonModal);
    exportDialog.setMode(KFile::File);
    exportDialog.setOperationMode(KFileDialog::Saving);

    if (exportDialog.exec() == QDialog::Accepted) {
        const QString path = QString("%1/%2/").arg(index.data(PathRole).toString()).arg(index.data(IdentifierRole).toString());

        if (!Plasma::Package::createPackage(Plasma::PackageMetadata(path + "metadata.desktop"), (path + "contents/"), exportDialog.selectedFile())) {
            KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to export theme."));
        }
    }
}

void Configuration::deleteTheme()
{
    const QModelIndex index = m_appearanceUi.themesView->currentIndex();

    if (KMessageBox::questionYesNo(m_appearanceUi.themesView, i18n("Do you really want to delete theme \"%1\"?").arg(index.data(NameRole).toString()), i18n("Delete Theme")) == KMessageBox::Yes) {
        const int row = index.row();

        if (QFile::exists(QString("%1/%2").arg(index.data(PathRole).toString()).arg(index.data(IdentifierRole).toString())) && !Plasma::Package::uninstallPackage(index.data(IdentifierRole).toString(), index.data(PathRole).toString(), "plasma-adjustable-clock-addon-")) {
            KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to delete theme."));

            return;
        }

        m_themesModel->removeRow(row);

        selectTheme(m_themesModel->index(qMax((row - 1), 0), 0));

        emit clearCache();
    }
}

void Configuration::renameTheme()
{
    bool ok;
    QStandardItem *item = m_themesModel->itemFromIndex(m_appearanceUi.themesView->currentIndex());
    const QString title = KInputDialog::getText(i18n("Rename Theme"), i18n("Theme name:"), item->data(NameRole).toString(), &ok);

    if (!ok) {
        return;
    }

    item->setData(title, NameRole);

    Plasma::PackageMetadata metaData = getMetaData(item->data(PathRole).toString(), item->data(IdentifierRole).toString());
    metaData.setName(title);

    saveTheme(item->data(PathRole).toString(), item->data(IdentifierRole).toString(), metaData);
}

void Configuration::aboutTheme(const QString &theme)
{
    QStandardItem *item = (theme.isEmpty() ? m_themesModel->itemFromIndex(m_appearanceUi.themesView->currentIndex()) : m_themesModel->item(findRow(theme, IdentifierRole)));

    if (!item || !item->data(AboutRole).toBool()) {
        return;
    }

    const Plasma::PackageMetadata metaData = getMetaData(item->data(PathRole).toString(), item->data(IdentifierRole).toString());
    const QStringList authors = metaData.author().split(QChar(','), QString::KeepEmptyParts);
    const QStringList emails = metaData.email().split(QChar(','), QString::KeepEmptyParts);
    const QStringList websites = metaData.website().split(QChar(','), QString::KeepEmptyParts);
    KAboutData aboutData(item->data(IdentifierRole).toByteArray(), QByteArray(), ki18n(metaData.name().toUtf8().data()), metaData.version().toUtf8());
    aboutData.setProgramIconName("chronometer");
    aboutData.setLicense(KAboutLicense::byKeyword(metaData.license()).key());
    aboutData.setShortDescription(ki18n(metaData.description().toUtf8().data()));

    for (int i = 0; i < authors.count(); ++i) {
        aboutData.addCredit(ki18n(authors.at(i).toUtf8().data()), KLocalizedString(), emails.value(i).toUtf8(), websites.value(i).toUtf8());
    }

    KAboutApplicationDialog(&aboutData, m_appearanceUi.themesView).exec();
}

void Configuration::editTheme(const QString &theme)
{
    QStandardItem *item = (theme.isEmpty() ? m_themesModel->itemFromIndex(m_appearanceUi.themesView->currentIndex()) : m_themesModel->item(findRow(theme, IdentifierRole)));

    if (!item) {
        return;
    }

    if (!item->data(WritableRole).toBool()) {
        if (!copyTheme(item)) {
            return;
        }

        item = m_themesModel->itemFromIndex(m_appearanceUi.themesView->currentIndex());
    }

    EditorWidget *editor = new EditorWidget(item->data(PathRole).toString(), item->data(IdentifierRole).toString(), m_applet->getClock(), m_appearanceUi.themesView);
    KDialog editorDialog;
    editorDialog.setMainWidget(editor);
    editorDialog.setModal(true);
    editorDialog.setButtons(KDialog::Ok | KDialog::Cancel);
    editorDialog.setWindowTitle(i18n("\"%1\" Editor").arg(item->data(NameRole).toString()));

    if (editorDialog.exec() == QDialog::Rejected) {
        return;
    }

    if (editor->getIdentifier().isEmpty() || (editor->getIdentifier() != item->data(IdentifierRole).toString() && findRow(editor->getIdentifier(), IdentifierRole) >= 0)) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Invalid theme identifier."));

        return;
    }

    if (editor->getIdentifier() != item->data(IdentifierRole).toString() && !QDir().rename(QString("%1/%2").arg(item->data(PathRole).toString()).arg(item->data(IdentifierRole).toString()), QString("%1/%2").arg(item->data(PathRole).toString()).arg(editor->getIdentifier()))) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to change theme identifier."));

        return;
    }

    const Plasma::PackageMetadata metaData = editor->getMetaData();

    item->setData(editor->getIdentifier(), IdentifierRole);
    item->setData(metaData.name(), NameRole);
    item->setData(metaData.description(), DescriptionRole);

    if (saveTheme(item->data(PathRole).toString(), item->data(IdentifierRole).toString(), metaData) && editor->saveTheme()) {
        emit clearCache();
    } else {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to save theme."));
    }
}

void Configuration::configureTheme(const QString &theme)
{
    QStandardItem *item = (theme.isEmpty() ? m_themesModel->itemFromIndex(m_appearanceUi.themesView->currentIndex()) : m_themesModel->item(findRow(theme, IdentifierRole)));

    if (!item || !item->data(OptionsRole).toBool()) {
        return;
    }

    QFile file(QString("%1/%2/contents/config/main.xml").arg(item->data(PathRole).toString()).arg(item->data(IdentifierRole).toString()));
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    const QString configName = ("theme-" + item->data(IdentifierRole).toString());
    KConfigGroup configGroup = m_applet->config().group(configName);
    Plasma::ConfigLoader configLoader(&configGroup, &file);

    QList<KConfigSkeletonItem*> items = configLoader.items();

    if (items.isEmpty()) {
        return;
    }

    QHash<QString, OptionWidget*> widgets;
    QWidget *mainWidget = new QWidget();
    QFormLayout *layout = new QFormLayout(mainWidget);
    KDialog configDialog;
    configDialog.setMainWidget(mainWidget);
    configDialog.setModal(true);
    configDialog.setButtons(KDialog::Ok | KDialog::Default | KDialog::Cancel);
    configDialog.setWindowTitle(i18n("\"%1\" Options").arg(item->data(NameRole).toString()));

    const QColor themeTextColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    const QColor themeBackgroundColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    const QFont themeFont = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);

    for (int i = 0; i < items.count(); ++i) {
        if (items.at(i)->key() == "themeTextColor") {
            items.at(i)->setProperty(m_applet->config().group(configName).readEntry(items.at(i)->key(), QVariant(themeTextColor)));
        } else if (items.at(i)->key() == "themeBackgroundColor") {
            items.at(i)->setProperty(m_applet->config().group(configName).readEntry(items.at(i)->key(), QVariant(themeBackgroundColor)));
        } else if (items.at(i)->key() == "themeFont") {
            items.at(i)->setProperty(m_applet->config().group(configName).readEntry(items.at(i)->key(), QVariant(themeFont)));
        } else {
            items.at(i)->setProperty(m_applet->config().group(configName).readEntry(items.at(i)->key(), items.at(i)->property()));
        }

        OptionWidget *widget = new OptionWidget(items.at(i), mainWidget);
        QLabel *label = new QLabel(i18n(items.at(i)->label().toUtf8().data()), mainWidget);
        label->setBuddy(widget->getWidget());

        widgets[items.at(i)->key()] = widget;

        layout->addRow(label, widget);

        connect(&configDialog, SIGNAL(defaultClicked()), widget, SLOT(setDefaultValue()));
    }

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    if (configDialog.exec() == QDialog::Rejected) {
        return;
    }

    for (int i = 0; i < items.count(); ++i) {
        OptionWidget *widget = widgets[items.at(i)->key()];

        if (!widget) {
            continue;
        }

        if (items.at(i)->key() == "themeTextColor" && widget->getValue().value<QColor>() == themeTextColor) {
            m_applet->config().group(configName).deleteEntry(items.at(i)->key());
        } else if (items.at(i)->key() == "themeBackgroundColor" && widget->getValue().value<QColor>() == themeBackgroundColor) {
            m_applet->config().group(configName).deleteEntry(items.at(i)->key());
        } else if (items.at(i)->key() == "themeFont" && widget->getValue().value<QFont>() == themeFont) {
            m_applet->config().group(configName).deleteEntry(items.at(i)->key());
        } else if (widget->getValue().type() == QVariant::Font) {
            m_applet->config().group(configName).writeEntry(items.at(i)->key(), widget->getValue().value<QFont>().family());
        } else {
            m_applet->config().group(configName).writeEntry(items.at(i)->key(), widget->getValue());
        }
    }

    modify();

    emit clearCache();
}

void Configuration::showContextMenu(const QPoint &position)
{
    const QModelIndex index = m_appearanceUi.themesView->indexAt(position);

    if (!index.isValid()) {
        return;
    }

    KMenu menu(m_appearanceUi.themesView);
    menu.addAction(KIcon("help-about"), i18n("About..."), this, SLOT(aboutTheme()));
    menu.addSeparator();
    menu.addAction(KIcon("configure"), i18n("Options..."), this, SLOT(configureTheme()));
    menu.addAction(KIcon("edit-copy"), i18n("Copy..."), this, SLOT(copyTheme()));
    menu.addAction(KIcon("document-export"), i18n("Export..."), this, SLOT(exportTheme()));

    if (index.data(WritableRole).toBool()) {
        menu.addAction(KIcon("document-rename"), i18n("Rename..."), this, SLOT(renameTheme()));
        menu.addAction(KIcon("document-edit"), i18n("Edit..."), this, SLOT(editTheme()));
        menu.addSeparator();
        menu.addAction(KIcon("edit-delete"), i18n("Delete..."), this, SLOT(deleteTheme()));
    }

    menu.exec(m_appearanceUi.themesView->viewport()->mapToGlobal(position));
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

QString Configuration::createIdentifier(const QString &base) const
{
    QString identifier = QString("custom-%1");

    if (!base.isEmpty()) {
        if (base.startsWith("custom-")) {
            identifier = QString(base).replace(QRegExp("\\d+$"), "%1");

            if (!identifier.endsWith("%1")) {
                identifier.append("-%1");
            }
        } else {
            identifier = QString("custom-%1-").arg(base).append("%1");
        }
    }

    int i = 1;

    while (findRow(identifier.arg(i), IdentifierRole) >= 0) {
       ++i;
    }

    return identifier.arg(i);
}

Plasma::PackageMetadata Configuration::getMetaData(const QString &path, const QString &identifier) const
{
    return Plasma::PackageMetadata(QString("%1/%2/metadata.desktop").arg(path).arg(identifier));
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

bool Configuration::loadTheme(const QString &path, const QString &identifier)
{
    Plasma::PackageMetadata metaData(QString("%1/%2/metadata.desktop").arg(path).arg(identifier));
    QStandardItem *item = new QStandardItem();
    item->setData(identifier, IdentifierRole);
    item->setData(path, PathRole);
    item->setData(metaData.name().toLower(), SortRole);
    item->setData(metaData.name(), NameRole);
    item->setData(metaData.description(), DescriptionRole);
    item->setData(!metaData.author().isEmpty(), AboutRole);
    item->setData(QFile::exists(QString("%1/%2/contents/config/main.xml").arg(path).arg(identifier)), OptionsRole);
    item->setData(QFileInfo(path).isWritable(), WritableRole);

    m_themesModel->appendRow(item);

    return true;
}

bool Configuration::copyTheme(QStandardItem *item)
{
    QString title = item->data(NameRole).toString().replace(QRegExp("\\s+\\(\\d+\\)$"), QString()).append(" (%1)");
    int i = 2;

    while (findRow(title.arg(i)) >= 0) {
        ++i;
    }

    title = title.arg(i);

    bool ok;

    title = KInputDialog::getText(i18n("Copy Theme"), i18n("Theme name:"), title, &ok);

    if (!ok) {
        return false;
    }

    const QString identifier = createIdentifier(item->data(IdentifierRole).toString());
    const QString path = KStandardDirs::locateLocal("data", "plasma/adjustableclock");
    Plasma::PackageMetadata metaData = getMetaData(item->data(PathRole).toString(), item->data(IdentifierRole).toString());
    metaData.setName(title);

    if (!copyDirectory(QString("%1/%2/contents/").arg(item->data(PathRole).toString()).arg(item->data(IdentifierRole).toString()), QString("%1/%2/contents/").arg(path).arg(identifier)) || !saveTheme(path, identifier, metaData)) {
        KMessageBox::error(m_appearanceUi.themesView, i18n("Failed to copy theme."));

        return false;
    }

    QStandardItem *clonedItem = item->clone();
    clonedItem->setData(identifier, IdentifierRole);
    clonedItem->setData(title, NameRole);
    clonedItem->setData(path, PathRole);
    clonedItem->setData(true, WritableRole);

    m_themesModel->appendRow(clonedItem);
    m_appearanceUi.themesView->openPersistentEditor(clonedItem->index());

    selectTheme(clonedItem->index());

    return true;
}

bool Configuration::saveTheme(const QString &path, const QString &identifier, Plasma::PackageMetadata metaData)
{
    const QString packagePath = QString("%1/%2/").arg(path).arg(identifier);

    if (!QDir().mkpath(packagePath + "contents/ui/")) {
        return false;
    }

    metaData.setPluginName(identifier);
    metaData.setType("Service");
    metaData.setServiceType("Plasma/AdjustableClock");
    metaData.write(packagePath + "metadata.desktop");

    return true;
}

bool Configuration::copyDirectory(const QString &source, const QString &destination)
{
    if (!QDir(destination).mkpath(destination)) {
        return false;
    }

    QDir sourceDirectory(source);

    foreach (const QFileInfo &entry, sourceDirectory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
        const QString sourceEntry = (source + QDir::separator() + entry.fileName());
        const QString destinationEntry = (destination + QDir::separator() + entry.fileName());

        if ((entry.isDir() && !copyDirectory(sourceEntry, destinationEntry)) || (entry.isFile() && !QFile::copy(sourceEntry, destinationEntry))) {
            return false;
        }
    }

    return true;
}

bool Configuration::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_appearanceUi.themesView && event->type() == QEvent::Paint && !m_appearanceUi.themesView->currentIndex().isValid()) {
        selectTheme(m_themesModel->index(qMax(findRow(m_applet->config().readEntry("theme", "digital"), IdentifierRole), 0), 0));
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