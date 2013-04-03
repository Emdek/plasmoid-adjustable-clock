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
#include "Clock.h"

#include <QtGui/QListWidgetItem>
#include <QtGui/QStandardItemModel>

#include <KConfigDialog>

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
    ScriptRole = (Qt::UserRole + 6),
    BackgroundRole = (Qt::UserRole + 7),
    BundledRole = (Qt::UserRole + 8)
};

class Clock;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        Configuration(Applet *applet, KConfigDialog *parent);

        bool eventFilter(QObject *object, QEvent *event);

    protected:
        int findRow(const QString &text, int role = TitleRole);

    protected slots:
        void save();
        void modify();
        void enableUpdates();
        void disableUpdates();
        void insertComponent();
        void insertComponent(const QString &script, ClockComponent component);
        void selectTheme(const QModelIndex &index);
        void newTheme(bool automatically = false);
        void deleteTheme();
        void renameTheme();
        void updateTheme(const Theme &theme);
        void updateView(int tab);
        void updateEditor(int tab);
        void triggerAction();
        void fixSelection();
        void selectColor();
        void selectFontSize(const QString &size);
        void selectFontFamily(const QFont &font);
        void setColor(const QString &color);
        void setFontSize(const QString &size);
        void setFontFamily(const QString &font);
        void setZoom(int zoom);
        void showEditorContextMenu(const QPoint &position);
        void themeChanged();
        void backgroundChanged();
        void richTextChanged();
        void sourceChanged();
        void itemSelectionChanged();
        void editItem(QListWidgetItem *item = NULL);
        void insertItem();
        void deleteItem();
        void moveItem(bool up);
        void moveItemUp();
        void moveItemDown();
        void updateItem(QListWidgetItem *item);

    private:
        Applet *m_applet;
        Clock *m_clock;
        QStandardItemModel *m_themesModel;
        QListWidgetItem *m_editedItem;
        Ui::appearance m_appearanceUi;
        Ui::clipboard m_clipboardUi;

    signals:
        void accepted();
        void clearCache();
};

}

#endif
