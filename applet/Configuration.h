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

#include <KConfigDialog>

#include "ui_appearance.h"
#include "ui_clipboard.h"

namespace AdjustableClock
{

class Applet;

class Configuration : public QObject
{
    Q_OBJECT

    public:
        Configuration(Applet *applet, KConfigDialog *parent);

    protected:
        void timerEvent(QTimerEvent *event);

    protected slots:
        void accepted();
        void insertPlaceholder();
        void insertPlaceholder(const QString &placeholder);
        void loadFormat(int index);
        void addFormat(bool automatically = false);
        void removeFormat();
        void changeFormat();
        void updateControls();
        void triggerAction();
        void selectColor();
        void selectFontSize(const QString &size);
        void selectFontFamily(const QFont &font);
        void setColor(const QString &color);
        void setFontSize(const QString &size);
        void setFontFamily(const QString &font);
        void selectionChanged();
        void itemSelectionChanged();
        void insertRow();
        void deleteRow();
        void moveRow(bool up);
        void moveRowUp();
        void moveRowDown();
        void updateRow(int row, int column);

    private:
        Applet *m_applet;
        int m_controlsTimer;
        int m_fontSize;
        Ui::appearance m_appearanceUi;
        Ui::clipboard m_clipboardUi;
};

}

#endif
