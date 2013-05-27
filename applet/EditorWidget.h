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

#ifndef ADJUSTABLECLOCKEDITORWIDGET_HEADER
#define ADJUSTABLECLOCKEDITORWIDGET_HEADER

#include <KTextEditor/Document>

#include <Plasma/Package>

#include "ui_editor.h"

namespace AdjustableClock
{

class Clock;
class ThemeWidget;

class EditorWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EditorWidget(const QString &path, Clock *clock, QWidget *parent);

        QString getIdentifier() const;
        Plasma::PackageMetadata getMetaData() const;
        bool saveTheme();

    protected:
        void setStyle(const QString &property, const QString &value, const QString &tag = "span");

    protected slots:
        void triggerAction();
        void insertComponent(const QString &component, const QString &options);
        void selectionChanged();
        void tabChanged(int tab);
        void updateEditor();
        void updateWebView(const QString &theme = QString());
        void showContextMenu(const QPoint &position);
        void setBackground(bool enabled);
        void setColor();
        void setFontSize(const QString &size);
        void setFontFamily(const QFont &font);
        void setZoom(int zoom);

    private:
        Clock *m_clock;
        ThemeWidget *m_widget;
        KTextEditor::Document *m_document;
        QString m_path;
        bool m_qml;
        Ui::editor m_editorUi;
};

}

#endif
