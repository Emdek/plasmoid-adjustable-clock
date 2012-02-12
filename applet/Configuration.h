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

#ifndef ADJUSTABLECLOCKCONFIGURATION_HEADER
#define ADJUSTABLECLOCKCONFIGURATION_HEADER

#include "Applet.h"

#include <QtGui/QStandardItemModel>

#include <KConfigDialog>

#include "ui_appearance.h"
#include "ui_clipboard.h"

namespace AdjustableClock
{

enum ModelRole { IdRole = (Qt::UserRole + 1), TitleRole = (Qt::UserRole + 2), DescriptionRole = (Qt::UserRole + 3), AuthorRole = (Qt::UserRole + 4), HtmlRole = (Qt::UserRole + 5), CssRole = (Qt::UserRole + 6), BackgroundRole = (Qt::UserRole + 7), BundledRole = (Qt::UserRole + 8) };

class Applet;
class FormatLineEdit;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        Configuration(Applet *applet, KConfigDialog *parent);

        bool eventFilter(QObject *object, QEvent *event);

    protected:
        void timerEvent(QTimerEvent *event);
        int findRow(const QString &text, int role = TitleRole);

    protected slots:
        void accepted();
        void insertPlaceholder();
        void insertPlaceholder(const QString &placeholder);
        void selectFormat(const QModelIndex &index);
        void addFormat(bool automatically = false);
        void deleteFormat();
        void renameFormat();
        void updateFormat(const Format &format);
        void updateControls();
        void triggerAction();
        void selectColor();
        void selectFontSize(const QString &size);
        void selectFontFamily(const QFont &font);
        void setColor(const QString &color);
        void setFontSize(const QString &size);
        void setFontFamily(const QString &font);
        void backgroundChanged();
        void richTextChanged();
        void sourceChanged();
        void selectionChanged();
        void itemSelectionChanged();
        void editRow(QTableWidgetItem *item);
        void insertRow();
        void deleteRow();
        void moveRow(bool up);
        void moveRowUp();
        void moveRowDown();
        void updateRow(int row);

    private:
        Applet *m_applet;
        QStandardItemModel *m_themesModel;
        QTableWidgetItem *m_editedItem;
        int m_controlsTimer;
        int m_fontSize;
        Ui::appearance m_appearanceUi;
        Ui::clipboard m_clipboardUi;

    signals:
        void clearCache();
};

}

#endif
