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

#include "AdjustableClock.h"

#include <QtCore/QRegExp>
#include <QtGui/QClipboard>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>

#include <KMenu>
#include <KLocale>
#include <KMessageBox>
#include <KColorDialog>
#include <KInputDialog>
#include <KConfigDialog>
#include <KCalendarSystem>
#include <KSystemTimeZones>

#include <Plasma/Theme>
#include <Plasma/Containment>

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock)

AdjustableClock::AdjustableClock(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clipboardAction(NULL),
    m_controlsTimer(0)
{
    KGlobal::locale()->insertCatalog(QLatin1String("libplasmaclock"));
    KGlobal::locale()->insertCatalog(QLatin1String("timezones4"));
    KGlobal::locale()->insertCatalog(QLatin1String("adjustableclock"));

    setHasConfigurationInterface(true);
    resize(150, 80);

    m_timeFormatStrings << QLatin1String("<div style=\"text-align:center; margin:5px; white-space:pre;\"><big>%H:%M:%S</big>\n<small>%d.%m.%Y</small></div>")
    << QLatin1String("<div style=\"text-align:center; margin:5px; white-space:pre;\"><big style=\"font-family:'Nimbus Sans L Condensed';\">%H:%M:%S</big>\n<span style=\"font-size:small; font-family:'Nimbus Sans L';\">%d.%m.%Y</small></div>")
    << QLatin1String("<div style=\"text-align:center; white-space:pre; font-size:25px; margin:5px;\">%H:%M</div>")
    << QLatin1String("<div style=\"text-align:center; white-space:pre; opacity:0.85; background:none;\"><span style=\"font-size:30px;\">%H:%M:%S</span><br><span style=\"font-size:12px;\">%A, %d.%m.%Y</span></div>")
    << QLatin1String("<div style=\"text-align:center; white-space:pre; font-size:25px; margin:0 0 5px 5px; background:none;\">%H:%M<span style=\"font-size:30px; position:relative; left:-8px; top:4px; z-index:-1; opacity:0.5;\">%S</span></div>")
    << QLatin1String("<div style=\"height:50px; background:none;\"><div style=\"text-align:center; white-space:pre; font-size:25px; margin:-10px 0 5px 5px; -webkit-box-reflect:below -5px -webkit-gradient(linear, left top, left bottom, from(transparent), color-stop(0.5, transparent), to(white));\">%H:%M<span style=\"font-size:30px; position:relative; left:-8px; top:4px; z-index:-1; opacity:0.5;\">%S</span></div></div>")
    << QLatin1String("<div style=\"width:295px; min-height:295px; background:none; text-shadow:0 0 5px #AAA;\"><div style=\"margin:30px 0 0 0; padding:30px 20px 20px 20px; position:relative; font-weight:bold; font-size:30px; text-align:center; background:-webkit-gradient(linear, left top, left bottom, from(#E5702B), to(#A33B03)); color:white; border-radius:20px; box-shadow:5px 5px 15px #888; opacity:0.7;\">%A<br /><span style=\"font-size:130px; line-height:140px;\">%e</span><br /><span style=\"font-size:35px;\">%B %Y</span><br />%h<div style=\"position:absolute; top:-30px; left:-10px; width:310px; height:60px; padding:10px 20px;\"><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div><div style=\"width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;\"></div></div></div></div>");

    m_timeFormatNames << i18n("Default")
    << i18n("Flat")
    << i18n("Simple")
    << i18n("Verbose")
    << i18n("dbClock")
    << i18n("dbClock with reflection")
    << i18n("Calendar");

    m_clipboardFormats << QLatin1String("%x")
    << QLatin1String("%f")
    << QLatin1String("%H:%M:%S")
    << QString()
    << QLatin1String("%X")
    << QLatin1String("%F")
    << QString()
    << QLatin1String("%c")
    << QLatin1String("%C")
    << QLatin1String("%Y-%m-%d %H:%M:%S")
    << QString()
    << QLatin1String("%t");
}

void AdjustableClock::init()
{
    ClockApplet::init();

    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    m_holiday = holiday();

    updateTheme();
    connectSource(currentTimezone());
    constraintsEvent(Plasma::SizeConstraint);
    configChanged();

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void AdjustableClock::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(source)

    m_dateTime = QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());

    if (m_features & HolidaysFeature && m_dateTime.time().minute() == 0 && (!(m_features & SecondsClockFeature || m_features & SecondsToolTipFeature) || m_dateTime.time().second() == 0)) {
        m_holiday = holiday();
    }

    if (m_features & SunriseFeature) {
        m_sunrise = data[QLatin1String("Sunrise")].toTime();
    }

    if (m_features & SunsetFeature) {
        m_sunset = data[QLatin1String("Sunset")].toTime();
    }

    setText(formatDateTime(m_dateTime, m_timeFormat));

    if (Plasma::ToolTipManager::self()->isVisible(this)) {
        updateToolTipContent();
    }
}

void AdjustableClock::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints(QRegExp(QLatin1String("<[a-z].*\\sstyle=('|\").*background(-image)?\\s*:\\s*none.*('|\").*>"), Qt::CaseInsensitive).exactMatch(m_timeFormat) ? NoBackground : DefaultBackground);
}

void AdjustableClock::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    ClockApplet::resizeEvent(event);

    updateSize();
}

void AdjustableClock::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)

    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    m_page.mainFrame()->render(painter);
}

void AdjustableClock::createClockConfigurationInterface(KConfigDialog *parent)
{
    KConfigGroup configuration = config();
    QWidget *appearanceConfiguration = new QWidget;
    QWidget *clipboardActions = new QWidget;
    QStringList timeFormatStrings = m_timeFormatStrings;
    QStringList timeFormatNames = m_timeFormatNames;
    QStringList clipboardFormats = configuration.readEntry("clipboardFormats", m_clipboardFormats);
    QString preview;
    int row;

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardActions);

    KMenu *placeholdersMenu = new KMenu(m_appearanceUi.placeholdersButton);

    QList<QPair<QLatin1Char, QString> > placeholders;
    placeholders << qMakePair(QLatin1Char(0), i18n("Seconds"))
    << qMakePair(QLatin1Char('s'), i18n("Short form"))
    << qMakePair(QLatin1Char('S'), i18n("Long form"))
    << qMakePair(QLatin1Char('t'), i18n("UNIX timestamp"))
    << qMakePair(QLatin1Char(0), i18n("Minutes"))
    << qMakePair(QLatin1Char('M'), i18n("Long form"))
    << qMakePair(QLatin1Char(0), i18n("Hours"))
    << qMakePair(QLatin1Char('H'), i18n("24h format, long form"))
    << qMakePair(QLatin1Char('k'), i18n("24h format, short form"))
    << qMakePair(QLatin1Char('I'), i18n("12h format, long form"))
    << qMakePair(QLatin1Char('l'), i18n("12h format, short form"))
    << qMakePair(QLatin1Char('p'), i18n("The pm or am string"))
    << qMakePair(QLatin1Char(0), i18n("Days"))
    << qMakePair(QLatin1Char('d'), i18n("Day of the month, long form"))
    << qMakePair(QLatin1Char('e'), i18n("Day of the month, short form"))
    << qMakePair(QLatin1Char('a'), i18n("Weekday name, short form"))
    << qMakePair(QLatin1Char('A'), i18n("Weekday name, long form"))
    << qMakePair(QLatin1Char('w'), i18n("Weekday number, short form"))
    << qMakePair(QLatin1Char('j'), i18n("Day of the year, short form"))
    << qMakePair(QLatin1Char(0), i18n("Weeks"))
    << qMakePair(QLatin1Char('W'), i18n("Short form"))
    << qMakePair(QLatin1Char(0), i18n("Months"))
    << qMakePair(QLatin1Char('n'), i18n("Number, short form"))
    << qMakePair(QLatin1Char('m'), i18n("Number, long form"))
    << qMakePair(QLatin1Char('b'), i18n("Name, short form"))
    << qMakePair(QLatin1Char('B'), i18n("Name, long form"))
    << qMakePair(QLatin1Char(0), i18n("Years"))
    << qMakePair(QLatin1Char('Y'), i18n("Long form"))
    << qMakePair(QLatin1Char('y'), i18n("Short form"))
    << qMakePair(QLatin1Char(0), i18n("Timezone"))
    << qMakePair(QLatin1Char('Z'), i18n("Abbreviation"))
    << qMakePair(QLatin1Char('z'), i18n("Offset to UTC"))
    << qMakePair(QLatin1Char('g'), i18n("City"))
    << qMakePair(QLatin1Char(0), i18n("Other"))
    << qMakePair(QLatin1Char('h'), i18n("Holiday name"))
    << qMakePair(QLatin1Char('o'), i18n("Sunrise time"))
    << qMakePair(QLatin1Char('O'), i18n("Sunset time"))
    << qMakePair(QLatin1Char('F'), i18n("Time, short form"))
    << qMakePair(QLatin1Char('X'), i18n("Time, long form"))
    << qMakePair(QLatin1Char('f'), i18n("Date, short form"))
    << qMakePair(QLatin1Char('x'), i18n("Date, long form"))
    << qMakePair(QLatin1Char('c'), i18n("Date and time, short form"))
    << qMakePair(QLatin1Char('C'), i18n("Date and time, long form"));

    for (int i = 0; i < placeholders.count(); ++i) {
        if (placeholders.at(i).first == QLatin1Char(0)) {
            placeholdersMenu->addTitle(placeholders.at(i).second);
        } else {
            QAction *action = placeholdersMenu->addAction(QString(QLatin1String("%1 (%%2)\t%3")).arg(placeholders.at(i).second).arg(placeholders.at(i).first).arg(formatDateTime(m_dateTime, (QString(QLatin1Char('%')).append(placeholders.at(i).first)))));
            action->setData(QVariant(placeholders.at(i).first));
        }
    }

    timeFormatStrings.append(configuration.readEntry("timeFormatStrings", QStringList()));

    timeFormatNames.append(configuration.readEntry("timeFormatNames", QStringList()));

    for (int i = 0; i < timeFormatStrings.count(); ++i) {
        if (i == m_timeFormatNames.count()) {
            m_appearanceUi.timeFormatComboBox->insertSeparator(i);
        }

        m_appearanceUi.timeFormatComboBox->addItem(timeFormatNames.at(i), timeFormatStrings.at(i));
    }

    QPalette webViewPalette = m_appearanceUi.webView->page()->palette();
    webViewPalette.setBrush(QPalette::Base, Qt::transparent);

    m_appearanceUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_appearanceUi.webView->page()->setPalette(webViewPalette);
    m_appearanceUi.webView->page()->setContentEditable(true);
    m_appearanceUi.addButton->setIcon(KIcon(QLatin1String("list-add")));
    m_appearanceUi.removeButton->setIcon(KIcon(QLatin1String("list-remove")));
    m_appearanceUi.placeholdersButton->setIcon(KIcon(QLatin1String("chronometer")));
    m_appearanceUi.placeholdersButton->setMenu(placeholdersMenu);
    m_appearanceUi.boldButton->setIcon(KIcon(QLatin1String("format-text-bold")));
    m_appearanceUi.italicButton->setIcon(KIcon(QLatin1String("format-text-italic")));
    m_appearanceUi.underlineButton->setIcon(KIcon(QLatin1String("format-text-underline")));
    m_appearanceUi.justifyLeftButton->setIcon(KIcon(QLatin1String("format-justify-left")));
    m_appearanceUi.justifyCenterButton->setIcon(KIcon(QLatin1String("format-justify-center")));
    m_appearanceUi.justifyRightButton->setIcon(KIcon(QLatin1String("format-justify-right")));

    m_clipboardUi.moveUpButton->setIcon(KIcon(QLatin1String("arrow-up")));
    m_clipboardUi.moveDownButton->setIcon(KIcon(QLatin1String("arrow-down")));
    m_clipboardUi.fastCopyFormat->setText(config().readEntry("fastCopyFormat", "%Y-%m-%d %H:%M:%S"));

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        row = m_clipboardUi.clipboardActionsTable->rowCount();

        m_clipboardUi.clipboardActionsTable->insertRow(row);
        m_clipboardUi.clipboardActionsTable->setItem(row, 0, new QTableWidgetItem(clipboardFormats.at(i)));

        preview = formatDateTime(m_dateTime, clipboardFormats.at(i));

        QTableWidgetItem *item = new QTableWidgetItem(preview);
        item->setFlags(Qt::ItemIsSelectable);
        item->setToolTip(preview);

        m_clipboardUi.clipboardActionsTable->setItem(row, 1, item);
    }

    updateControls();
    itemSelectionChanged();

    QPalette buttonPalette = m_appearanceUi.colorButton->palette();
    buttonPalette.setBrush(QPalette::Button, Qt::black);

    m_appearanceUi.colorButton->setPalette(buttonPalette);

    parent->addPage(appearanceConfiguration, i18n("Appearance"), QLatin1String("preferences-desktop-theme"));
    parent->addPage(clipboardActions, i18n("Clipboard actions"), QLatin1String("edit-copy"));

    connect(placeholdersMenu, SIGNAL(triggered(QAction*)), this, SLOT(insertPlaceholder(QAction*)));
    connect(m_appearanceUi.timeFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
    connect(m_appearanceUi.addButton, SIGNAL(clicked()), this, SLOT(addFormat()));
    connect(m_appearanceUi.removeButton, SIGNAL(clicked()), this, SLOT(removeFormat()));
    connect(m_appearanceUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.timeFormat, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.boldButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.italicButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.underlineButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(toggleState()));
    connect(m_appearanceUi.colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(selectFontFamily(QFont)));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertRow()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteRow()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveRowUp()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveRowDown()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(cellChanged(int, int)), this, SLOT(updateRow(int, int)));

    int currentFormat = m_appearanceUi.timeFormatComboBox->findData(m_timeFormat);

    if (currentFormat < 0) {
        m_appearanceUi.timeFormatComboBox->insertItem(0, i18n("Custom"), m_timeFormat);

        currentFormat = 0;
    }

    m_appearanceUi.timeFormatComboBox->setCurrentIndex(currentFormat);

    loadFormat(currentFormat);
}

void AdjustableClock::clockConfigChanged()
{
    m_timeFormat = config().readEntry("timeFormat", m_timeFormatStrings.at(0));

    m_holiday = holiday();

    setText(formatDateTime(currentDateTime(), m_timeFormat));

    updateSize();
}

void AdjustableClock::clockConfigAccepted()
{
    KConfigGroup configuration = config();
    QStringList timeFormatStrings;
    QStringList timeFormatNames;
    QStringList clipboardFormats;

    killTimer(m_controlsTimer);

    for (int i = 0; i < m_appearanceUi.timeFormatComboBox->count(); ++i) {
        if (m_timeFormatNames.contains(m_appearanceUi.timeFormatComboBox->itemText(i)) || m_appearanceUi.timeFormatComboBox->itemText(i).isEmpty()) {
            continue;
        }

        timeFormatStrings.append(m_appearanceUi.timeFormatComboBox->itemData(i).toString());

        timeFormatNames.append(m_appearanceUi.timeFormatComboBox->itemText(i));
    }

    for (int i = 0; i < m_clipboardUi.clipboardActionsTable->rowCount(); ++i) {
        clipboardFormats.append(m_clipboardUi.clipboardActionsTable->item(i, 0)->text());
    }

    configuration.writeEntry("timeFormat", m_appearanceUi.timeFormat->toPlainText());
    configuration.writeEntry("timeFormatStrings", timeFormatStrings);
    configuration.writeEntry("timeFormatNames", timeFormatNames);
    configuration.writeEntry("clipboardFormats", clipboardFormats);
    configuration.writeEntry("fastCopyFormat", m_clipboardUi.fastCopyFormat->text());

    emit configNeedsSaving();
}

void AdjustableClock::connectSource(const QString &timezone)
{
    QRegExp formatWithSeconds = QRegExp(QLatin1String("%\\d*(S|c|C|t|F|X)"));
    QFlags<ClockFeature> features;

    if (m_timeFormat.contains(formatWithSeconds)) {
        features |= SecondsClockFeature;
    }

    if ((config().keyList().contains(QLatin1String("toolTipFormat")) ? config().readEntry("toolTipFormat", QString()) : QLatin1String("<div style=\"text-align:center;\">%Y-%m-%d<br />%H:%M:%S</div>")).contains(formatWithSeconds)) {
        features |= SecondsToolTipFeature;
    }

    if (m_timeFormat.contains(QLatin1String("%h"))) {
        features |= HolidaysFeature;
    }

    if (m_timeFormat.contains(QLatin1String("%o"))) {
        features |= SunsetFeature;
    }

    if (m_timeFormat.contains(QLatin1String("%O"))) {
        features |= SunriseFeature;
    }

    const bool alignToSeconds = (features & SecondsClockFeature || features & SecondsToolTipFeature);

    m_features = features;

    dataEngine(QLatin1String("time"))->connectSource((timezone + QLatin1String((features & SecondsClockFeature || features & SecondsToolTipFeature) ? "|Solar" : "")), this, (alignToSeconds ? 1000 : 60000), (alignToSeconds ? Plasma::NoAlignment : Plasma::AlignToMinute));

    m_timeZoneAbbreviation = QString::fromLatin1(KSystemTimeZones::zone(timezone).abbreviation(QDateTime::currentDateTime().toUTC()));

    if (m_timeZoneAbbreviation.isEmpty()) {
        m_timeZoneAbbreviation = i18n("UTC");
    }

    int seconds = KSystemTimeZones::zone(currentTimezone()).currentOffset();
    int minutes = abs(seconds / 60);
    int hours = abs(minutes / 60);

    minutes = (minutes - (hours * 60));

    m_timeZoneOffset = QString::number(hours);

    if (minutes) {
        m_timeZoneOffset.append(QLatin1Char(':'));

        if (minutes < 10) {
            m_timeZoneOffset.append(QLatin1Char('0'));
        }

        m_timeZoneOffset.append(QString::number(minutes));
    }

    m_timeZoneOffset = (QChar((seconds >= 0) ? QLatin1Char('+') : QLatin1Char('-')) + m_timeZoneOffset);

    updateSize();
}

void AdjustableClock::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::MidButton) {
        copyToClipboard();
    }

    ClockApplet::mousePressEvent(event);
}

void AdjustableClock::timerEvent(QTimerEvent *event)
{
    updateControls();

    killTimer(event->timerId());
}

void AdjustableClock::copyToClipboard()
{
    QApplication::clipboard()->setText(formatDateTime(currentDateTime(), config().readEntry("fastCopyFormat", "%Y-%m-%d %H:%M:%S")));
}

void AdjustableClock::insertPlaceholder(QAction *action)
{
    QString placeholder = QString(QLatin1Char('%')).append(action->data().toChar());

    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        m_appearanceUi.timeFormat->insertPlainText(placeholder);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('inserthtml', false, '") + placeholder + QLatin1String("')"));
    }
}

void AdjustableClock::loadFormat(int index)
{
    m_appearanceUi.timeFormat->setPlainText(m_appearanceUi.timeFormatComboBox->itemData(index).toString());
    m_appearanceUi.removeButton->setEnabled(index >= m_timeFormatNames.count());
}

void AdjustableClock::changeFormat()
{
    QString text;

    if (sender() == m_appearanceUi.webView->page()) {
        QRegExp fontSize = QRegExp(QLatin1String(" class=\"Apple-style-span\""));
        QRegExp fontColor = QRegExp(QLatin1String("<font color=\"(#?[\\w\\s]+)\">(.+)</font>"));
        fontColor.setMinimal(true);

        QRegExp fontFamily = QRegExp(QLatin1String("<font face=\"'?([\\w\\s]+)'?\">(.+)</font>"));
        fontFamily.setMinimal(true);

        text = m_appearanceUi.webView->page()->mainFrame()->toHtml().remove(QLatin1String("<html><head></head><body>")).remove(QLatin1String("<html><body>")).remove(QLatin1String("</body></html>")).remove(fontSize).replace(fontColor, QLatin1String("<span style=\"color:\\1;\">\\2</span>")).replace(fontFamily, QLatin1String("<span style=\"font-family:'\\1';\">\\2</span>"));
    } else {
        text = m_appearanceUi.timeFormat->toPlainText();
    }

    const QString toolTip = formatDateTime(m_dateTime, text);

    disconnect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.timeFormat, SIGNAL(textChanged()), this, SLOT(changeFormat()));

    m_appearanceUi.timeFormat->setToolTip(toolTip);
    m_appearanceUi.webView->setToolTip(toolTip);

    if (sender() == m_appearanceUi.webView->page()) {
        m_appearanceUi.timeFormat->setPlainText(text);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->setHtml(text);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("boldButton"), m_appearanceUi.boldButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("italicButton"), m_appearanceUi.italicButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("underlineButton"), m_appearanceUi.underlineButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("designModeEditor"), this);
    }

    if (m_appearanceUi.timeFormatComboBox->currentIndex() < m_timeFormatNames.count() && m_appearanceUi.timeFormatComboBox->itemData(m_appearanceUi.timeFormatComboBox->currentIndex(), Qt::UserRole).toString() != text) {
        addFormat(true);
    }

    m_appearanceUi.timeFormatComboBox->setItemData(m_appearanceUi.timeFormatComboBox->currentIndex(), text);

    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.timeFormat, SIGNAL(textChanged()), this, SLOT(changeFormat()));
}

void AdjustableClock::addFormat(bool automatically)
{
    QString formatName = m_appearanceUi.timeFormatComboBox->itemText(m_appearanceUi.timeFormatComboBox->currentIndex());

    if (automatically) {
        int i = 2;

        while (m_appearanceUi.timeFormatComboBox->findText(QString(QLatin1String("%1 %2")).arg(formatName).arg(i)) >= 0) {
            ++i;
        }

        formatName = QString(QLatin1String("%1 %2")).arg(formatName).arg(i);
    } else {
        formatName = KInputDialog::getText(i18n("Add new format"), i18n("Format name:"), formatName);
    }

    if (m_appearanceUi.timeFormatComboBox->findText(formatName) >= 0) {
        KMessageBox::error(m_appearanceUi.timeFormatComboBox, i18n("A format with this name already exists."));
    } else if (!formatName.isEmpty()) {
        int index = (m_appearanceUi.timeFormatComboBox->currentIndex() + 1);

        if (index <= m_timeFormatNames.count()) {
            index = m_appearanceUi.timeFormatComboBox->count();
        }

        disconnect(m_appearanceUi.timeFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));

        if (index == m_timeFormatNames.count() && m_timeFormatNames.count() == m_appearanceUi.timeFormatComboBox->count())
        {
            m_appearanceUi.timeFormatComboBox->insertSeparator(index);

            ++index;
        }

        m_appearanceUi.timeFormatComboBox->insertItem(index, formatName, m_appearanceUi.timeFormat->toPlainText());
        m_appearanceUi.timeFormatComboBox->setCurrentIndex(index);
        m_appearanceUi.removeButton->setEnabled(true);

        connect(m_appearanceUi.timeFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
    }
}

void AdjustableClock::removeFormat()
{
    if (m_appearanceUi.timeFormatComboBox->currentIndex() > m_timeFormatNames.count()) {
        m_appearanceUi.timeFormatComboBox->removeItem(m_appearanceUi.timeFormatComboBox->currentIndex());

        if (m_appearanceUi.timeFormatComboBox->itemData((m_appearanceUi.timeFormatComboBox->count() - 1), Qt::DisplayRole).toString().isEmpty()) {
            m_appearanceUi.timeFormatComboBox->removeItem(m_appearanceUi.timeFormatComboBox->count() - 1);
        }
    }
}

void AdjustableClock::updateControls()
{
    if (!isUserConfiguring()) {
        return;
    }

    disconnect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));

    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("boldButton.setChecked(document.queryCommandState('bold'));"
            "italicButton.setChecked(document.queryCommandState('italic'));"
            "underlineButton.setChecked(document.queryCommandState('underline'));"
            "designModeEditor.setColor(document.queryCommandValue('forecolor'));"
            "designModeEditor.setFontSize(document.queryCommandValue('fontsize').replace('px', ''));"
            "designModeEditor.setFontFamily(document.queryCommandValue('fontname'))"));

    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));
}

void AdjustableClock::toggleState()
{
    QString actionName = sender()->objectName().remove(QLatin1String("Button")).toLower();
    QHash<QString, QWebPage::WebAction> actions;
    actions[QLatin1String("bold")] = QWebPage::ToggleBold;
    actions[QLatin1String("italic")] = QWebPage::ToggleItalic;
    actions[QLatin1String("underline")] = QWebPage::ToggleUnderline;
    actions[QLatin1String("justifyLeft")] = QWebPage::AlignLeft;
    actions[QLatin1String("justifyCenter")] = QWebPage::AlignCenter;
    actions[QLatin1String("justifyRight")] = QWebPage::AlignRight;

    if (actions.contains(actionName))
    {
        m_appearanceUi.webView->page()->triggerAction(actions[actionName]);
    }
}

void AdjustableClock::selectColor()
{
    KColorDialog colorDialog;
    colorDialog.setAlphaChannelEnabled(true);
    colorDialog.setColor(m_appearanceUi.colorButton->palette().button().color());
    colorDialog.setButtons(KDialog::Ok | KDialog::Cancel);

    if (colorDialog.exec() == QDialog::Accepted) {
        QPalette palette = m_appearanceUi.colorButton->palette();
        palette.setBrush(QPalette::Button, colorDialog.color());

        m_appearanceUi.colorButton->setPalette(palette);
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('forecolor', false, '") + colorDialog.color().name() + QLatin1String("')"));
    }
}

void AdjustableClock::selectFontSize(const QString &size)
{
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontsizedelta', false, ") + QString::number(size.toInt() - m_fontSize) + QLatin1String(")"));

    m_fontSize = size.toInt();
}

void AdjustableClock::selectFontFamily(const QFont &font)
{
    m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontname', false, '") + font.family() + QLatin1String("')"));
}

void AdjustableClock::setColor(const QString &color)
{
    if (color != QLatin1String("false")) {
        QRegExp expression = QRegExp(QLatin1String("rgb\\((\\d+), (\\d+), (\\d+)\\)"));
        expression.indexIn(color);

        const QStringList rgb = expression.capturedTexts();

        QPalette palette = m_appearanceUi.colorButton->palette();
        palette.setBrush(QPalette::Button, QColor(rgb.at(1).toInt(), rgb.at(2).toInt(), rgb.at(3).toInt()));

        m_appearanceUi.colorButton->setPalette(palette);
    }
}

void AdjustableClock::setFontSize(const QString &size)
{
    if (!m_appearanceUi.fontSizeComboBox->hasFocus()) {
        m_appearanceUi.fontSizeComboBox->setEditText(size);
    }

    m_fontSize = size.toInt();
}

void AdjustableClock::setFontFamily(const QString &font)
{
    m_appearanceUi.fontFamilyComboBox->setCurrentFont(QFont(font));
}

void AdjustableClock::selectionChanged()
{
    m_controlsTimer = startTimer(250);

    if (m_appearanceUi.webView->page()->selectedText().endsWith(QLatin1Char('%'))) {
        m_appearanceUi.webView->page()->triggerAction(QWebPage::SelectNextChar);
    }
}

void AdjustableClock::itemSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_clipboardUi.clipboardActionsTable->selectedItems();

    m_clipboardUi.moveUpButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.first()) != 0);
    m_clipboardUi.moveDownButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.last()) != (m_clipboardUi.clipboardActionsTable->rowCount() - 1));
    m_clipboardUi.deleteButton->setEnabled(!selectedItems.isEmpty());
}

void AdjustableClock::insertRow()
{
    const int row = ((m_clipboardUi.clipboardActionsTable->rowCount() && m_clipboardUi.clipboardActionsTable->currentRow() >= 0) ? m_clipboardUi.clipboardActionsTable->currentRow() : 0);

    m_clipboardUi.clipboardActionsTable->insertRow(row);
    m_clipboardUi.clipboardActionsTable->setItem(row, 0, new QTableWidgetItem(QString()));

    QTableWidgetItem *item = new QTableWidgetItem(QString());
    item->setFlags(0);

    m_clipboardUi.clipboardActionsTable->setItem(row, 1, item);
    m_clipboardUi.clipboardActionsTable->setCurrentCell(row, 0);
}

void AdjustableClock::deleteRow()
{
    m_clipboardUi.clipboardActionsTable->removeRow(m_clipboardUi.clipboardActionsTable->row(m_clipboardUi.clipboardActionsTable->selectedItems().at(0)));
}

void AdjustableClock::moveRow(bool up)
{
    int sourceRow = m_clipboardUi.clipboardActionsTable->row(m_clipboardUi.clipboardActionsTable->selectedItems().at(0));
    int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));

    QList<QTableWidgetItem*> sourceItems;
    QList<QTableWidgetItem*> destinationItems;

    for (int i = 0; i < 2; ++i) {
        sourceItems.append(m_clipboardUi.clipboardActionsTable->takeItem(sourceRow, i));

        destinationItems.append(m_clipboardUi.clipboardActionsTable->takeItem(destinationRow, i));
    }

    for (int i = 0; i < 2; ++i) {
        m_clipboardUi.clipboardActionsTable->setItem(sourceRow, i, destinationItems.at(i));
        m_clipboardUi.clipboardActionsTable->setItem(destinationRow, i, sourceItems.at(i));
    }

    m_clipboardUi.clipboardActionsTable->setCurrentCell(destinationRow, 0);
}

void AdjustableClock::moveRowUp()
{
    moveRow(true);
}

void AdjustableClock::moveRowDown()
{
    moveRow(false);
}

void AdjustableClock::updateRow(int row, int column)
{
    Q_UNUSED(column)

    if (!m_clipboardUi.clipboardActionsTable->item(row, 1)) {
        return;
    }

    const QString preview = formatDateTime(m_dateTime, m_clipboardUi.clipboardActionsTable->item(row, 0)->text());

    m_clipboardUi.clipboardActionsTable->item(row, 1)->setText(preview);
    m_clipboardUi.clipboardActionsTable->item(row, 1)->setToolTip(preview);
}

void AdjustableClock::toolTipAboutToShow()
{
    updateToolTipContent();
}

void AdjustableClock::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);
}

void AdjustableClock::setText(const QString &text)
{
    if (text != m_currentText) {
        m_page.mainFrame()->setHtml(QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"><html><head><style type=\"text/css\">html, body, table, td {margin:0; padding:0; height:100%; width:100%; vertical-align:middle;}</style></head><body><table><tr><td id=\"clock\">") + text + QLatin1String("</td></tr></table></body></html>"));

        m_currentText = text;

        update();
    }
}

void AdjustableClock::copyToClipboard(QAction* action)
{
    QApplication::clipboard()->setText(action->text());
}

void AdjustableClock::updateClipboardMenu()
{
    const QDateTime dateTime = currentDateTime();
    const QStringList clipboardFormats = config().readEntry("clipboardFormats", m_clipboardFormats);

    qDeleteAll(m_clipboardAction->menu()->actions());

    m_clipboardAction->menu()->clear();

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        if (clipboardFormats.at(i).isEmpty()) {
            m_clipboardAction->menu()->addSeparator();
        } else {
            m_clipboardAction->menu()->addAction(formatDateTime(dateTime, clipboardFormats.at(i)));
        }
    }
}

void AdjustableClock::changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone)
{
    dataEngine(QLatin1String("time"))->disconnectSource((oldTimezone + QLatin1String((m_features & SecondsClockFeature || m_features & SecondsToolTipFeature) ? "|Solar" : "")), this);

    connectSource(newTimezone);
}

void AdjustableClock::updateToolTipContent()
{
    Plasma::ToolTipContent toolTipData;
    QString toolTipFormat;

    if (config().keyList().contains(QLatin1String("toolTipFormat"))) {
        toolTipFormat = config().readEntry("toolTipFormat", QString());
    } else {
        toolTipFormat = QLatin1String("<div style=\"text-align:center;\">%Y-%m-%d<br />%H:%M:%S</div>");
    }

    if (!toolTipFormat.isEmpty()) {
        toolTipData.setImage(KIcon(QLatin1String("chronometer")).pixmap(IconSize(KIconLoader::Desktop)));
        toolTipData.setMainText(formatDateTime(m_dateTime, toolTipFormat));
        toolTipData.setAutohide(false);
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTipData);
}

void AdjustableClock::updateSize()
{
    QSizeF size;
    QString string;
    QString longest;
    QString temporary;
    int placeholder;
    const int length = (m_timeFormat.length() - 1);
    int number = 0;

    for (int index = 0; index < length; ++index) {
        if (m_timeFormat.at(index) == QLatin1Char('%') && m_timeFormat.at(index + 1) != QLatin1Char('%')) {
            ++index;

            if (m_timeFormat.at(index).isDigit()) {
                while (m_timeFormat.at(index).isDigit()) {
                    ++index;
                }

                ++index;

                string.append(QLatin1Char('W'));

                continue;
            }

            placeholder = m_timeFormat.at(index).unicode();

            longest.clear();

            switch (placeholder) {
            case 'a':
            case 'A':
                number = calendar()->daysInWeek(m_dateTime.date());

                for (int i = 0; i <= number; ++i) {
                    temporary = calendar()->weekDayName(i, ((placeholder == 'a') ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));

                    if (temporary.length() > longest.length()) {
                        longest = temporary;
                    }
                }

                string.append(longest);
                break;
            case 'b':
            case 'B':
                number = calendar()->monthsInYear(m_dateTime.date());

                for (int i = 0; i < number; ++i) {
                    temporary = calendar()->monthName(i, calendar()->year(m_dateTime.date()), ((placeholder == 'b') ? KCalendarSystem::ShortName : KCalendarSystem::LongName));

                    if (temporary.length() > longest.length()) {
                        longest = temporary;
                    }
                }

                string.append(longest);
                break;
            case 'c':
                string.append(KGlobal::locale()->formatDateTime(m_dateTime, KLocale::LongDate));
                break;
            case 'C':
                string.append(KGlobal::locale()->formatDateTime(m_dateTime, KLocale::ShortDate));
                break;
            case 'h':
                string.append(QLatin1String("XXXXXXXXXX"));
                break;
            case 'd':
            case 'e':
            case 'H':
            case 'I':
            case 'k':
            case 'l':
            case 'm':
            case 'M':
            case 'n':
            case 'S':
            case 'W':
            case 'U':
            case 'y':
                string.append(QLatin1String("00"));
                break;
            case 'f':
                string.append(KGlobal::locale()->formatDate(m_dateTime.date(), KLocale::ShortDate));
                break;
            case 'x':
                string.append(KGlobal::locale()->formatDate(m_dateTime.date(), KLocale::LongDate));
                break;
            case 'o':
                string.append(KGlobal::locale()->formatTime(m_sunrise, false));
                break;
            case 'O':
                string.append(KGlobal::locale()->formatTime(m_sunset, false));
                break;
            case 'F':
                string.append(KGlobal::locale()->formatTime(m_dateTime.time(), false));
                break;
            case 'X':
                string.append(KGlobal::locale()->formatTime(m_dateTime.time(), true));
                break;
            case 'g':
                string.append(prettyTimezone());
                break;
            case 'j':
                string.append(QLatin1String("000"));
                break;
            case 'p':
                string.append((i18n("pm").length() > i18n("am").length()) ? i18n("pm") : i18n("am"));
                break;
            case 't':
                string.append(QString::number(m_dateTime.toTime_t()));
                break;
            case 'w':
                string.append(QLatin1Char('0'));
                break;
            case 'Y':
                string.append(QLatin1String("0000"));
                break;
            case 'Z':
                string.append(m_timeZoneAbbreviation);
                break;
            case 'z':
                string.append(m_timeZoneOffset);
            default:
                string.append(m_timeFormat.at(index));
                break;
            }
        } else {
            string.append(m_timeFormat.at(index));
        }
    }

    if (m_timeFormat.at(length - 1) != QLatin1Char('%')) {
        string.append(m_timeFormat.at(length));
    }

    setText(string);

    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    if (formFactor() == Plasma::Horizontal) {
        size = QSizeF(containment()->boundingRect().width(), boundingRect().height());
    } else if (formFactor() == Plasma::Vertical) {
        size = QSizeF(boundingRect().width(), containment()->boundingRect().height());
    } else {
        size = boundingRect().size();
    }

    qreal widthFactor = (size.width() / m_page.mainFrame()->contentsSize().width());
    qreal heightFactor = (size.height() / m_page.mainFrame()->contentsSize().height());

    m_page.mainFrame()->setZoomFactor((widthFactor > heightFactor) ? heightFactor : widthFactor);

    if (formFactor() == Plasma::Horizontal) {
        setMinimumWidth(m_page.mainFrame()->contentsSize().width());
        setMinimumHeight(0);
    } else if (formFactor() == Plasma::Vertical) {
        setMinimumHeight(m_page.mainFrame()->contentsSize().height());
        setMinimumWidth(0);
    }

    m_page.setViewportSize(boundingRect().size().toSize());

    setText(formatDateTime(m_dateTime, m_timeFormat));
}

void AdjustableClock::updateTheme()
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->evaluateJavaScript(QLatin1String("document.fgColor = '") + Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name() + QLatin1Char('\''));

    update();
}

QDateTime AdjustableClock::currentDateTime() const
{
    Plasma::DataEngine::Data data = dataEngine(QLatin1String("time"))->query(currentTimezone());
    QDateTime dateTime = QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());

    return dateTime;
}

QString AdjustableClock::holiday() const
{
    const QString region = config().readEntry("holidaysRegions", dataEngine(QLatin1String("calendar"))->query(QLatin1String("holidaysDefaultRegion"))[QLatin1String("holidaysDefaultRegion")]).toString().split(QLatin1Char(',')).first();
    const QString key = QLatin1String("holidays:") + region + QLatin1Char(':') + currentDateTime().date().toString(Qt::ISODate);
    Plasma::DataEngine::Data holidays = dataEngine(QLatin1String("calendar"))->query(key);

    if (holidays.isEmpty())
    {
        return QString();
    }

    return holidays[key].toList().first().toHash()[QLatin1String("Name")].toString();
}

QString AdjustableClock::formatDateTime(const QDateTime dateTime, const QString &format) const
{
    if (format.isEmpty()) {
        return QString();
    }

    QString string;
    const int length = (format.length() - 1);

    for (int index = 0; index < length; ++index) {
        if (format.at(index) == QLatin1Char('%') && format.at(index + 1) != QLatin1Char('%')) {
            QString placeholderString;
            int charNumber = -1;

            ++index;

            if (format.at(index).isDigit()) {
                QString numberString;

                while (format.at(index).isDigit()) {
                    numberString.append(format.at(index));

                    ++index;
                }

                charNumber = numberString.toInt();
            }

            switch (format.at(index).unicode()) {
            case 'a': // weekday, short form
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::DayOfWeekName, KLocale::ShortName));
                break;
            case 'A': // weekday, long form
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::DayOfWeekName, KLocale::LongName));
                break;
            case 'b': // month, short form
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::MonthName, KLocale::ShortName));
                break;
            case 'B': // month, long form
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::MonthName, KLocale::LongName));
                break;
            case 'c': // date and time format, short
                placeholderString.append(KGlobal::locale()->formatDateTime(dateTime, KLocale::LongDate));
                break;
            case 'C': // date and time format, long
                placeholderString.append(KGlobal::locale()->formatDateTime(dateTime, KLocale::ShortDate));
                break;
            case 'd': // day of the month, two digits
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Day, KLocale::LongNumber));
                break;
            case 'e': // day of the month, one digit
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Day, KLocale::ShortNumber));
                break;
            case 'f': // date format, short
                placeholderString.append(KGlobal::locale()->formatDate(dateTime.date(), KLocale::ShortDate));
                break;
            case 'F': // time format, short
                placeholderString.append(KGlobal::locale()->formatTime(dateTime.time(), false));
                break;
            case 'g': // timezone city
                placeholderString.append(prettyTimezone());
                break;
            case 'h': // holiday
                placeholderString.append(m_holiday);
                break;
            case 'H': // hour, 24h format
                if (dateTime.time().hour() < 10) {
                    placeholderString.append(QLatin1Char('0'));
                }

                placeholderString.append(QString::number(dateTime.time().hour()));
                break;
            case 'I': // hour, 12h format
                if ((((dateTime.time().hour() + 11) % 12) + 1) < 10) {
                    placeholderString.append(QLatin1Char('0'));
                }

                placeholderString.append(QString::number(((dateTime.time().hour() + 11) % 12) + 1));
                break;
            case 'j': // day of the year
                placeholderString.append(QString::number(calendar()->dayOfYear(dateTime.date())));
                break;
            case 'k': // hour, 24h format, one digit
                placeholderString.append(QString::number(dateTime.time().hour()));
                break;
            case 'l': // hour, 12h format, one digit
                placeholderString.append(QString::number(((dateTime.time().hour() + 11) % 12) + 1));
                break;
            case 'm': // month, two digits
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Month, KLocale::LongNumber));
                break;
            case 'M': // minute, two digits
                if (dateTime.time().minute() < 10) {
                    placeholderString.append(QLatin1Char('0'));
                }

                placeholderString.append(QString::number(dateTime.time().minute()));
                break;
            case 'n': // month, one digit
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Month, KLocale::ShortNumber));
                break;
            case 'o': // sunrise time
                placeholderString.append(KGlobal::locale()->formatTime(m_sunrise, false));
                break;
            case 'O': // sunset time
                placeholderString.append(KGlobal::locale()->formatTime(m_sunset, false));
                break;
            case 'p': // pm or am
                placeholderString.append((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));
                break;
            case 's': // second, one digit
                placeholderString.append(QString::number(dateTime.time().second()));
                break;
            case 'S': // second, two digits
                if (dateTime.time().second() < 10) {
                    placeholderString.append(QLatin1Char('0'));
                }

                placeholderString.append(QString::number(dateTime.time().second()));
                break;
            case 't': // UNIX timestamp
                placeholderString.append(QString::number(dateTime.toTime_t()));
                break;
            case 'w': // day of week
                placeholderString.append(QString::number(calendar()->dayOfWeek(dateTime.date())));
                break;
            case 'W': // week number
            case 'U':
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Week, KLocale::ShortNumber));
                break;
            case 'x': // date format, long
                placeholderString.append(KGlobal::locale()->formatDate(dateTime.date(), KLocale::LongDate));
                break;
            case 'X': // time format, long
                placeholderString.append(KGlobal::locale()->formatTime(dateTime.time(), true));
                break;
            case 'Y': // year, four digits
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Year, KLocale::LongNumber));
                break;
            case 'y': // year, two digits
                placeholderString.append(calendar()->formatDate(dateTime.date(), KLocale::Year, KLocale::ShortNumber));
                break;
            case 'Z': // timezone abbreviation
                placeholderString.append(m_timeZoneAbbreviation);
                break;
            case 'z': // timezone offset
                placeholderString.append(m_timeZoneOffset);
                break;
            default:
                placeholderString.append(format.at(index));
                break;
            }

            if (charNumber >= 0) {
                string.append(placeholderString.at((charNumber >= placeholderString.count()) ? (placeholderString.count() - 1) : charNumber));
            } else {
                string.append(placeholderString);
            }
        } else {
            string.append(format.at(index));
        }
    }

    if (format.at(length - 1) != QLatin1Char('%')) {
        string.append(format.at(length));
    }

    return string;
}

QList<QAction*> AdjustableClock::contextualActions()
{
    QList<QAction*> actions = ClockApplet::contextualActions();

    if (!m_clipboardAction) {
        m_clipboardAction = new QAction(SmallIcon(QLatin1String("edit-copy")), i18n("C&opy to Clipboard"), this);
        m_clipboardAction->setMenu(new KMenu);

        connect(this, SIGNAL(destroyed()), m_clipboardAction->menu(), SLOT(deleteLater()));
        connect(m_clipboardAction->menu(), SIGNAL(aboutToShow()), this, SLOT(updateClipboardMenu()));
        connect(m_clipboardAction->menu(), SIGNAL(triggered(QAction*)), this, SLOT(copyToClipboard(QAction*)));
    }

    for (int i = 0; i < actions.count(); ++i) {
        if (actions.at(i)->text() == i18n("C&opy to Clipboard")) {
            actions.removeAt(i);
            actions.insert(i, m_clipboardAction);

            m_clipboardAction->setVisible(!config().readEntry("clipboardFormats", m_clipboardFormats).isEmpty());
        }
    }

    return actions;

}
