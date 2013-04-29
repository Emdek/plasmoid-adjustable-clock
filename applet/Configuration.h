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

#include <QtGui/QStandardItemModel>

#include <KConfigDialog>

#include <Plasma/Package>

#include "ui_appearance.h"
#include "ui_clipboard.h"

namespace AdjustableClock
{

enum ModelRole
{
    IdentifierRole = (Qt::UserRole + 1),
    PathRole,
    SortRole,
    NameRole,
    DescriptionRole,
    AboutRole,
    WritableRole
};

class Applet;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        explicit Configuration(Applet *applet, KConfigDialog *parent);

        bool eventFilter(QObject *object, QEvent *event);

    protected:
        QString createIdentifier(const QString &base = QString()) const;
        Plasma::PackageMetadata getMetaData(const QString &path) const;
        int findRow(const QString &text, int role = NameRole) const;
        bool copyTheme(QStandardItem *item);
        bool loadTheme(const QString &path);
        bool saveTheme(const QString &path, Plasma::PackageMetadata metaData);
        static bool copyDirectory(const QString &source, const QString &destination);

    protected slots:
        void save();
        void modify();
        void selectTheme(const QModelIndex &index);
        void installTheme();
        void createTheme();
        void copyTheme();
        void exportTheme();
        void deleteTheme();
        void renameTheme();
        void aboutTheme(const QString &theme = QString());
        void editTheme(const QString &theme = QString());
        void configureTheme(const QString &theme = QString());
        void showContextMenu(const QPoint &position);
        void selectAction(const QModelIndex &index);
        void editAction(QModelIndex index = QModelIndex());
        void insertAction();
        void deleteAction();
        void moveAction(bool up);
        void moveUpAction();
        void moveDownAction();

    private:
        Applet *m_applet;
        QStandardItemModel *m_themesModel;
        QStandardItemModel *m_actionsModel;
        QModelIndex m_editedAction;
        Ui::appearance m_appearanceUi;
        Ui::clipboard m_clipboardUi;

    signals:
        void accepted();
        void clearCache();
};

}

#endif
