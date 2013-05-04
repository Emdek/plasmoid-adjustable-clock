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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1801, USA.
*
***********************************************************************************/

#include "ThemeDelegate.h"
#include "Applet.h"
#include "ThemeWidget.h"
#include "Configuration.h"

#include <QtGui/QStyle>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QApplication>

#include <KLocale>
#include <KPixmapCache>

#include <Plasma/Theme>
#include <Plasma/FrameSvg>

namespace AdjustableClock
{

KPixmapCache *m_cache = NULL;

ThemeDelegate::ThemeDelegate(Clock *clock) : QStyledItemDelegate(clock),
    m_clock(clock)
{
    m_cache = new KPixmapCache("AdjustableClockPreviews");
    m_cache->discard();
}

ThemeDelegate::~ThemeDelegate()
{
    delete m_cache;
}

void ThemeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QPixmap pixmap;

    if (!m_cache->find((index.data(IdentifierRole).toString()), pixmap)) {
        pixmap = QPixmap(200, 100);
        pixmap.fill(Qt::transparent);

        QRectF rectangle(0, 0, 200, 100);
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

        QGraphicsScene scene;
        ThemeWidget widget(m_clock);
        widget.setTheme(index.data(PathRole).toString());

        if (widget.getBackgroundFlag()) {
            Plasma::FrameSvg background;
            background.setImagePath(Plasma::Theme::defaultTheme()->imagePath("widgets/background"));
            background.setEnabledBorders(Plasma::FrameSvg::AllBorders);
            background.resizeFrame(rectangle.size());
            background.paintFrame(&pixmapPainter);

            rectangle = background.contentsRect();
        } else {
            pixmapPainter.setOpacity(0.1);
            pixmapPainter.setBrush(QBrush(Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor)));
            pixmapPainter.setPen(QPen(Qt::transparent));
            pixmapPainter.drawRoundedRect(rectangle.toRect(), 10, 10);
            pixmapPainter.setOpacity(1);
        }

        widget.resize(rectangle.size());

        scene.addItem(&widget);
        scene.render(&pixmapPainter, rectangle);

        m_cache->insert((index.data(IdentifierRole).toString()), pixmap);
    }

    QFont font = painter->font();

    painter->drawPixmap(option.rect.topLeft(), pixmap);
    painter->setRenderHints(QPainter::TextAntialiasing);
    painter->setPen(option.palette.color(QPalette::WindowText));

    font.setBold(true);

    painter->setFont(font);
    painter->drawText(QRectF(210, (option.rect.y() + 10), (option.rect.width() - 215), 20), (Qt::AlignLeft | Qt::AlignVCenter), index.data(NameRole).toString());

    font.setBold(false);

    painter->setFont(font);
    painter->drawText(QRectF(210, (option.rect.y() + 35), (option.rect.width() - 215), 70), (Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap), index.data(DescriptionRole).toString());
}

void ThemeDelegate::clear()
{
    m_cache->discard();
}

void ThemeDelegate::propagateSignal()
{
    const QString name = sender()->objectName();

    if (name.startsWith("about-")) {
        emit showAbout(name.mid(6));
    } else if (name.startsWith("edit-")) {
        emit showEditor(name.mid(5));
    } else if (name.startsWith("options-")) {
        emit showOptions(name.mid(8));
    }
}

void ThemeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const QList<QPushButton*> buttons = editor->findChildren<QPushButton*>();

    for (int i = 0; i < buttons.count(); ++i) {
        buttons.at(i)->setObjectName(buttons.at(i)->objectName().replace(QRegExp("(options|edit|about)-.+"), ("\\1-" + index.data(IdentifierRole).toString())));
    }
}

QWidget* ThemeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    QWidget *widget = new QWidget(parent);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, widget);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    if (QFile::exists(index.data(PathRole).toString() + "/contents/config/main.xml")) {
        QPushButton *optionsButton = new QPushButton(KIcon("configure"), QString(), widget);
        optionsButton->setToolTip(i18n("Options..."));
        optionsButton->setObjectName("options-" + index.data(IdentifierRole).toString());

        layout->addWidget(optionsButton);
        layout->setAlignment(optionsButton, Qt::AlignBottom);

        connect(optionsButton, SIGNAL(clicked()), this, SLOT(propagateSignal()));
    }

    QPushButton *editButton = new QPushButton(KIcon("document-edit"), QString(), widget);
    editButton->setToolTip(index.data(EditableRole).toBool() ? i18n("Edit...") : i18n("Copy and Edit..."));
    editButton->setObjectName("edit-" + index.data(IdentifierRole).toString());

    layout->addWidget(editButton);
    layout->setAlignment(editButton, Qt::AlignBottom);

    connect(editButton, SIGNAL(clicked()), this, SLOT(propagateSignal()));

    if (index.data(AboutRole).toBool()) {
        QPushButton *aboutButton = new QPushButton(KIcon("help-about"), QString(), widget);
        aboutButton->setToolTip(i18n("About..."));
        aboutButton->setObjectName("about-" + index.data(IdentifierRole).toString());

        layout->addWidget(aboutButton);
        layout->setAlignment(aboutButton, Qt::AlignBottom);

        connect(aboutButton, SIGNAL(clicked()), this, SLOT(propagateSignal()));
    }

    return widget;
}

QSize ThemeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(400, 100);
}

}
