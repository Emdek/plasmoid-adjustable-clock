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

#ifndef ADJUSTABLECLOCKAPPLET_HEADER
#define ADJUSTABLECLOCKAPPLET_HEADER

#include <QtCore/QList>
#include <QtCore/QDateTime>
#include <QtWebKit/QWebPage>
#include <QtScript/QScriptEngine>

#include <Plasma/Applet>
#include <Plasma/DataEngine>

#include <plasmaclock/clockapplet.h>

#include "ui_appearance.h"
#include "ui_clipboard.h"

namespace AdjustableClock
{

enum ClockFeature { NoFeatures = 0, SecondsClockFeature = 1, SecondsToolTipFeature = 2, HolidaysFeature = 4, EventsFeature = 8, SunsetFeature = 16, SunriseFeature = 32, NoBackgroundFeature = 64 };

struct Format
{
    QString title;
    QString html;
    QString css;
    bool background;
};

class Applet : public ClockApplet
{
    Q_OBJECT

public:
    Applet(QObject *parent, const QVariantList &args);

    void init();
    QString evaluateFormat(const QString &format, QDateTime dateTime = QDateTime());
    QString evaluatePlaceholder(ushort placeholder, QDateTime dateTime, int alternativeForm, bool shortForm, bool textualForm) const;

protected:
    void constraintsEvent(Plasma::Constraints constraints);
    void resizeEvent(QGraphicsSceneResizeEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void timerEvent(QTimerEvent *event);
    void paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
    void createClockConfigurationInterface(KConfigDialog *parent);
    void changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone);
    void connectSource(const QString &timezone);
    void setHtml(const QString &html, const QString &css);
    QDateTime currentDateTime() const;
    QString extractExpression(const QString &format, int &i) const;
    QString extractNumber(const QString &format, int &i) const;
    QString formatNumber(int number, int length) const;
    QString holiday() const;
    Format format(QString name = QString()) const;
    QStringList formats(bool all = true) const;
    QList<QAction*> contextualActions();

protected slots:
    void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data, bool force = false);
    void clockConfigChanged();
    void clockConfigAccepted();
    void copyToClipboard();
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
    void toolTipAboutToShow();
    void toolTipHidden();
    void copyToClipboard(QAction *action);
    void updateClipboardMenu();
    void updateToolTipContent();
    void updateSize();
    void updateTheme();

private:
    QScriptEngine m_engine;
    QWebPage m_page;
    QString m_timeZoneAbbreviation;
    QString m_timeZoneOffset;
    QString m_events;
    QString m_holiday;
    QString m_currentHtml;
    QStringList m_clipboardFormats;
    Format m_format;
    QDateTime m_dateTime;
    QTime m_sunrise;
    QTime m_sunset;
    QAction *m_clipboardAction;
    QFlags<ClockFeature> m_features;
    int m_controlsTimer;
    int m_fontSize;
    Ui::appearance m_appearanceUi;
    Ui::clipboard m_clipboardUi;

    Q_DECLARE_FLAGS(ClockFeatures, ClockFeature)
};

}

#endif
