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

#ifndef ADJUSTABLECLOCKCOMPONENTWIDGET_HEADER
#define ADJUSTABLECLOCKCOMPONENTWIDGET_HEADER

#include "Clock.h"

#include <QtGui/QWidget>

#include "ui_component.h"

namespace AdjustableClock
{

class ComponentWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit ComponentWidget(Clock *clock, QWidget *parent = NULL);

    public slots:
        void insertComponent();

    protected:
        void addOption(QWidget *widget);

    protected slots:
        void selectComponent(QAction *action);
        void updatePreview();
        void setShortForm(bool form);
        void setAlternativeForm(int form);
        void setTextualForm(bool form);
        void setPossessiveForm(int form);

    private:
        Clock *m_clock;
        QVariantMap m_options;
        ClockComponent m_component;
        Ui::component m_componentUi;

    signals:
        void componentChanged(bool valid);
        void insertComponent(QString component, QString options);
};

}

#endif
