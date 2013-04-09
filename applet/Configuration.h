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

#ifndef ADJUSTABLECLOCKCONFIGURATION_HEADER
#define ADJUSTABLECLOCKCONFIGURATION_HEADER

#include "Applet.h"

#include <QtGui/QStandardItemModel>

#include <KConfigDialog>

#include <KTextEditor/Document>

#include "ui_appearance.h"
#include "ui_clipboard.h"

namespace AdjustableClock
{

enum ModelRole
{
    IdRole = (Qt::UserRole + 1),
    TitleRole = (Qt::UserRole + 2),
    DescriptionRole = (Qt::UserRole + 3),
    AuthorRole = (Qt::UserRole + 4),
    HtmlRole = (Qt::UserRole + 5),
    BackgroundRole = (Qt::UserRole + 6),
    BundledRole = (Qt::UserRole + 7)
};

class ComponentWidget;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        Configuration(Applet *applet, KConfigDialog *parent);

        bool eventFilter(QObject *object, QEvent *event);

    protected:
        void setStyle(const QString &property, const QString &value, const QString &tag = "span");
        QString createIdentifier() const;
        int findRow(const QString &text, int role = TitleRole) const;

    protected slots:
        void save();
        void modify();
        void selectTheme(const QModelIndex &index);
        void createTheme(bool automatically = false);
        void deleteTheme();
        void renameTheme();
        void triggerAction();
        void insertComponent(bool show);
        void insertComponent(const QString &component, const QString &options);
        void selectionChanged();
        void appearanceModeChanged(int mode);
        void editorModeChanged(int mode);
        void themeChanged();
        void richTextChanged();
        void sourceChanged();
        void showEditorContextMenu(const QPoint &position);
        void selectAction(const QModelIndex &index);
        void editAction(QModelIndex index = QModelIndex());
        void insertAction();
        void deleteAction();
        void moveAction(bool up);
        void moveUpAction();
        void moveDownAction();
        void setColor();
        void setFontSize(const QString &size);
        void setFontFamily(const QFont &font);
        void setZoom(int zoom);

    private:
        Applet *m_applet;
        QStandardItemModel *m_themesModel;
        QStandardItemModel *m_actionsModel;
        ComponentWidget *m_componentWidget;
        KTextEditor::Document *m_document;
        QModelIndex m_editedAction;
        Ui::appearance m_appearanceUi;
        Ui::clipboard m_clipboardUi;

    signals:
        void accepted();
        void clearCache();
};

}

#endif
