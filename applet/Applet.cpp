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

#include "Applet.h"
#include "PlaceholderDialog.h"

#include <QtCore/QRegExp>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
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

K_EXPORT_PLASMA_APPLET(adjustableclock, AdjustableClock::Applet)

namespace AdjustableClock
{

Applet::Applet(QObject *parent, const QVariantList &args) : ClockApplet(parent, args),
    m_clipboardAction(NULL),
    m_controlsTimer(0)
{
    KGlobal::locale()->insertCatalog(QLatin1String("libplasmaclock"));
    KGlobal::locale()->insertCatalog(QLatin1String("timezones4"));
    KGlobal::locale()->insertCatalog(QLatin1String("adjustableclock"));

    setHasConfigurationInterface(true);
    resize(150, 80);

    m_clipboardFormats << QLatin1String("%!t")
    << QLatin1String("%t")
    << QLatin1String("%h:%m:%s")
    << QString()
    << QLatin1String("%!T")
    << QLatin1String("%T")
    << QString()
    << QLatin1String("%!A")
    << QLatin1String("%A")
    << QLatin1String("%Y-%M-%d %h:%m:%s")
    << QString()
    << QLatin1String("%U");
}

void Applet::init()
{
    ClockApplet::init();

    m_page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    m_page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    updateTheme();
    connectSource(currentTimezone());
    constraintsEvent(Plasma::SizeConstraint);
    configChanged();

    connect(this, SIGNAL(activate()), this, SLOT(copyToClipboard()));
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(updateTheme()));
}

void Applet::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data, bool force)
{
    Q_UNUSED(source)

    m_dateTime = QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());

    const int second = m_dateTime.time().second();

    if (force || (m_features & HolidaysFeature && m_dateTime.time().hour() == 0 && m_dateTime.time().minute() == 0 && (second == 0 || !(m_features & SecondsClockFeature || m_features & SecondsToolTipFeature)))) {
        m_holiday = holiday();
    }

    if (force || ((m_features & SunriseFeature || m_features & SunsetFeature) && m_dateTime.time().minute() == 0 && second == 0)) {
        Plasma::DataEngine::Data data = dataEngine(QLatin1String("time"))->query(currentTimezone() + QLatin1String("|Solar"));

        if (m_features & SunriseFeature) {
            m_sunrise = data[QLatin1String("Sunrise")].toDateTime().time();
        }

        if (m_features & SunsetFeature) {
            m_sunset = data[QLatin1String("Sunset")].toDateTime().time();
        }
    }

    if (force || m_features & SecondsClockFeature || second == 0) {
        setHtml(evaluateFormat(format().html, m_dateTime), format().css);
    }

    if (Plasma::ToolTipManager::self()->isVisible(this) && (force || m_features & SecondsToolTipFeature || second == 0)) {
        updateToolTipContent();
    }
}

void Applet::constraintsEvent(Plasma::Constraints constraints)
{
    Q_UNUSED(constraints)

    setBackgroundHints((m_features & NoBackgroundFeature) ? NoBackground : DefaultBackground);
}

void Applet::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    ClockApplet::resizeEvent(event);

    updateSize();
}

void Applet::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::MidButton) {
        copyToClipboard();
    }

    const QUrl url = m_page.mainFrame()->hitTestContent(event->pos().toPoint()).linkUrl();

    if (url.isValid() && event->button() == Qt::LeftButton) {
        QDesktopServices::openUrl(url);

        event->ignore();
    } else {
        ClockApplet::mousePressEvent(event);
    }
}

void Applet::timerEvent(QTimerEvent *event)
{
    updateControls();

    killTimer(event->timerId());
}

void Applet::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)

    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    m_page.mainFrame()->render(painter);
}

void Applet::createClockConfigurationInterface(KConfigDialog *parent)
{
    QWidget *appearanceConfiguration = new QWidget;
    QWidget *clipboardActions = new QWidget;
    QStringList clipboardFormats = config().readEntry("clipboardFormats", m_clipboardFormats);
    QString preview;
    int row;

    m_appearanceUi.setupUi(appearanceConfiguration);
    m_clipboardUi.setupUi(clipboardActions);

    const QStringList formats = this->formats();

    for (int i = 0; i < formats.count(); ++i) {
        if (formats.at(i).isEmpty()) {
            m_appearanceUi.formatComboBox->insertSeparator(i);
        } else {
            Format format = this->format(formats.at(i));

            m_appearanceUi.formatComboBox->addItem(format.title, formats.at(i));
            m_appearanceUi.formatComboBox->setItemData(i, format.html, (Qt::UserRole + 1));
            m_appearanceUi.formatComboBox->setItemData(i, format.css, (Qt::UserRole + 2));
            m_appearanceUi.formatComboBox->setItemData(i, format.background, (Qt::UserRole + 3));
        }
    }

    QPalette webViewPalette = m_appearanceUi.webView->page()->palette();
    webViewPalette.setBrush(QPalette::Base, Qt::transparent);

    m_appearanceUi.webView->setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_appearanceUi.webView->page()->setPalette(webViewPalette);
    m_appearanceUi.webView->page()->setContentEditable(true);
    m_appearanceUi.addButton->setIcon(KIcon(QLatin1String("list-add")));
    m_appearanceUi.removeButton->setIcon(KIcon(QLatin1String("list-remove")));
    m_appearanceUi.placeholdersButton->setIcon(KIcon(QLatin1String("chronometer")));
    m_appearanceUi.boldButton->setIcon(KIcon(QLatin1String("format-text-bold")));
    m_appearanceUi.italicButton->setIcon(KIcon(QLatin1String("format-text-italic")));
    m_appearanceUi.underlineButton->setIcon(KIcon(QLatin1String("format-text-underline")));
    m_appearanceUi.justifyLeftButton->setIcon(KIcon(QLatin1String("format-justify-left")));
    m_appearanceUi.justifyCenterButton->setIcon(KIcon(QLatin1String("format-justify-center")));
    m_appearanceUi.justifyRightButton->setIcon(KIcon(QLatin1String("format-justify-right")));
    m_appearanceUi.backgroundButton->setIcon(KIcon(QLatin1String("games-config-background")));

    m_clipboardUi.moveUpButton->setIcon(KIcon(QLatin1String("arrow-up")));
    m_clipboardUi.moveDownButton->setIcon(KIcon(QLatin1String("arrow-down")));
    m_clipboardUi.fastCopyFormat->setText(config().readEntry("fastCopyFormat", "%Y-%M-%d %h:%m:%s"));

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        row = m_clipboardUi.clipboardActionsTable->rowCount();

        m_clipboardUi.clipboardActionsTable->insertRow(row);
        m_clipboardUi.clipboardActionsTable->setItem(row, 0, new QTableWidgetItem(clipboardFormats.at(i)));

        preview = evaluateFormat(clipboardFormats.at(i), m_dateTime);

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

    appearanceConfiguration->resize(600, 500);

    parent->addPage(appearanceConfiguration, i18n("Appearance"), QLatin1String("preferences-desktop-theme"));
    parent->addPage(clipboardActions, i18n("Clipboard actions"), QLatin1String("edit-copy"));

    connect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
    connect(m_appearanceUi.addButton, SIGNAL(clicked()), this, SLOT(addFormat()));
    connect(m_appearanceUi.removeButton, SIGNAL(clicked()), this, SLOT(removeFormat()));
    connect(m_appearanceUi.webView->page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.boldButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.italicButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.underlineButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyLeftButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyCenterButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.justifyRightButton, SIGNAL(clicked()), this, SLOT(triggerAction()));
    connect(m_appearanceUi.colorButton, SIGNAL(clicked()), this, SLOT(selectColor()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.fontSizeComboBox, SIGNAL(editTextChanged(QString)), this, SLOT(selectFontSize(QString)));
    connect(m_appearanceUi.fontFamilyComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(selectFontFamily(QFont)));
    connect(m_appearanceUi.placeholdersButton, SIGNAL(clicked()), this, SLOT(insertPlaceholder()));
    connect(m_clipboardUi.addButton, SIGNAL(clicked()), this, SLOT(insertRow()));
    connect(m_clipboardUi.deleteButton, SIGNAL(clicked()), this, SLOT(deleteRow()));
    connect(m_clipboardUi.moveUpButton, SIGNAL(clicked()), this, SLOT(moveRowUp()));
    connect(m_clipboardUi.moveDownButton, SIGNAL(clicked()), this, SLOT(moveRowDown()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(m_clipboardUi.clipboardActionsTable, SIGNAL(cellChanged(int, int)), this, SLOT(updateRow(int, int)));

    const int currentFormat = m_appearanceUi.formatComboBox->findData(config().readEntry("format", "%default%"));

    m_appearanceUi.formatComboBox->setCurrentIndex(currentFormat);

    loadFormat(currentFormat);
}

void Applet::clockConfigChanged()
{
    setHtml(evaluateFormat(format().html), format().css);

    updateSize();
}

void Applet::clockConfigAccepted()
{
    QStringList clipboardFormats;

    killTimer(m_controlsTimer);

    config().deleteGroup("Formats");

    KConfigGroup formatsConfiguration = config().group("Formats");
    const int builInFormats = formats(false).count();

    for (int i = 0; i < m_appearanceUi.formatComboBox->count(); ++i) {
        if (m_appearanceUi.formatComboBox->itemText(i).isEmpty()) {
            continue;
        }

        Format format;
        format.title = m_appearanceUi.formatComboBox->itemText(i);
        format.html = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 1)).toString();
        format.css = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 2)).toString();
        format.background = m_appearanceUi.formatComboBox->itemData(i, (Qt::UserRole + 3)).toBool();

        if (i < builInFormats) {
            Format existing = this->format(m_appearanceUi.formatComboBox->itemData(i).toString());

            if (format.html == existing.html && format.css == existing.css) {
                continue;
            }
        }

        KConfigGroup formatConfiguration = formatsConfiguration.group(m_appearanceUi.formatComboBox->itemData(i).toString());
        formatConfiguration.writeEntry("title", format.title);
        formatConfiguration.writeEntry("html", format.html);
        formatConfiguration.writeEntry("css", format.css);
        formatConfiguration.writeEntry("background", format.background);
    }

    for (int i = 0; i < m_clipboardUi.clipboardActionsTable->rowCount(); ++i) {
        clipboardFormats.append(m_clipboardUi.clipboardActionsTable->item(i, 0)->text());
    }

    config().writeEntry("format", m_appearanceUi.formatComboBox->itemData(m_appearanceUi.formatComboBox->currentIndex()).toString());
    config().writeEntry("clipboardFormats", clipboardFormats);
    config().writeEntry("fastCopyFormat", m_clipboardUi.fastCopyFormat->text());

    emit configNeedsSaving();
}

void Applet::connectSource(const QString &timezone)
{
    QRegExp formatWithSeconds = QRegExp(QLatin1String("%[\\d\\!\\$\\:\\+\\-]*s"));
    QFlags<ClockFeature> features;

    m_format.html = QString();

    const Format format = this->format();

    if (format.html.contains(formatWithSeconds)) {
        features |= SecondsClockFeature;
    }

    if ((config().keyList().contains(QLatin1String("toolTipFormat")) ? config().readEntry("toolTipFormat", QString()) : QLatin1String("<div style=\"text-align:center;\">%Y-%M-%d<br />%h:%m:%s</div>")).contains(formatWithSeconds)) {
        features |= SecondsToolTipFeature;
    }

    if (format.html.contains(QLatin1String("%H"))) {
        features |= HolidaysFeature;
    }

    if (format.html.contains(QLatin1String("%E"))) {
        features |= EventsFeature;
    }

    if (format.html.contains(QLatin1String("%S"))) {
        features |= SunsetFeature;
    }

    if (format.html.contains(QLatin1String("%R"))) {
        features |= SunriseFeature;
    }

    if (!format.background) {
        features |= NoBackgroundFeature;
    }

    m_features = features;

    const bool alignToSeconds = (features & SecondsClockFeature || features & SecondsToolTipFeature);

    dataEngine(QLatin1String("time"))->connectSource(timezone, this, (alignToSeconds ? 1000 : 60000), (alignToSeconds ? Plasma::NoAlignment : Plasma::AlignToMinute));

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

    constraintsEvent(Plasma::SizeConstraint);
    updateSize();
    dataUpdated(QString(), dataEngine(QLatin1String("time"))->query(currentTimezone()), true);
}

void Applet::copyToClipboard()
{
    QApplication::clipboard()->setText(evaluateFormat(config().readEntry("fastCopyFormat", "%Y-%M-%d %h:%m:%s")));
}

void Applet::insertPlaceholder()
{
    connect(new PlaceholderDialog(m_appearanceUi.placeholdersButton, this), SIGNAL(insertPlaceholder(QString)), this, SLOT(insertPlaceholder(QString)));
}

void Applet::insertPlaceholder(const QString &placeholder)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        m_appearanceUi.htmlTextEdit->insertPlainText(placeholder);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('inserthtml', false, '") + placeholder + QLatin1String("')"));
    }
}

void Applet::loadFormat(int index)
{
    disconnect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    m_appearanceUi.htmlTextEdit->setPlainText(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 1)).toString());
    m_appearanceUi.cssTextEdit->setPlainText(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 2)).toString());
    m_appearanceUi.backgroundButton->setChecked(m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 3)).toBool());
    m_appearanceUi.removeButton->setEnabled(index >= formats(false).count());

    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    changeFormat();
}

void Applet::changeFormat()
{
    Format format;
    format.background = m_appearanceUi.backgroundButton->isChecked();

    if (sender() == m_appearanceUi.webView->page()) {
        QRegExp fontSize = QRegExp(QLatin1String(" class=\"Apple-style-span\""));
        QRegExp fontColor = QRegExp(QLatin1String("<font color=\"(#?[\\w\\s]+)\">(.+)</font>"));
        fontColor.setMinimal(true);

        QRegExp fontFamily = QRegExp(QLatin1String("<font face=\"'?([\\w\\s]+)'?\">(.+)</font>"));
        fontFamily.setMinimal(true);

        QString html = m_appearanceUi.webView->page()->mainFrame()->toHtml().remove(QLatin1String("<style type=\"text/css\"></style>")).remove(QLatin1String("<head></head>")).remove(QLatin1String("<html><body>")).remove(QLatin1String("</body></html>")).remove(fontSize).replace(fontColor, QLatin1String("<span style=\"color:\\1;\">\\2</span>")).replace(fontFamily, QLatin1String("<span style=\"font-family:'\\1';\">\\2</span>"));

        QRegExp css = QRegExp(QLatin1String("<style type=\"text/css\">(.+)</style>"));
        css.setMinimal(true);
        css.indexIn(html);

        format.html = html.remove(css);
        format.css = css.cap(1);
    } else {
        format.html = m_appearanceUi.htmlTextEdit->toPlainText();
        format.css = m_appearanceUi.cssTextEdit->toPlainText();
    }

    disconnect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    disconnect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));

    if (sender() == m_appearanceUi.webView->page()) {
        m_appearanceUi.htmlTextEdit->setPlainText(format.html);
        m_appearanceUi.cssTextEdit->setPlainText(format.css);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->setHtml(QLatin1String("<style type=\"text/css\">") + format.css + QLatin1String("</style>") + format.html);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("boldButton"), m_appearanceUi.boldButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("italicButton"), m_appearanceUi.italicButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("underlineButton"), m_appearanceUi.underlineButton);
        m_appearanceUi.webView->page()->mainFrame()->addToJavaScriptWindowObject(QLatin1String("designModeEditor"), this);
    }

    const int index = m_appearanceUi.formatComboBox->currentIndex();

    if (index < formats(false).count() && (m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 1)).toString() != format.html || m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 2)).toString() != format.css || m_appearanceUi.formatComboBox->itemData(index, (Qt::UserRole + 3)).toBool() != format.background)) {
        addFormat(true);
    }

    m_appearanceUi.formatComboBox->setItemData(index, format.html, (Qt::UserRole + 1));
    m_appearanceUi.formatComboBox->setItemData(index, format.css, (Qt::UserRole + 2));
    m_appearanceUi.formatComboBox->setItemData(index, format.background, (Qt::UserRole + 3));

    connect(m_appearanceUi.webView->page(), SIGNAL(contentsChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.htmlTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.cssTextEdit, SIGNAL(textChanged()), this, SLOT(changeFormat()));
    connect(m_appearanceUi.backgroundButton, SIGNAL(clicked()), this, SLOT(changeFormat()));
}

void Applet::addFormat(bool automatically)
{
    QString title = m_appearanceUi.formatComboBox->itemText(m_appearanceUi.formatComboBox->currentIndex());

    if (automatically) {
        int i = 2;

        while (m_appearanceUi.formatComboBox->findText(QString(QLatin1String("%1 %2")).arg(title).arg(i)) >= 0) {
            ++i;
        }

        title = QString(QLatin1String("%1 %2")).arg(title).arg(i);
    } else {
        bool ok;

        title = KInputDialog::getText(i18n("Add new format"), i18n("Format name:"), title, &ok);

        if (!ok) {
            return;
        }
    }

    if (m_appearanceUi.formatComboBox->findText(title) >= 0) {
        KMessageBox::error(m_appearanceUi.formatComboBox, i18n("A format with this name already exists."));

        return;
    }

    if (title.startsWith(QLatin1Char('%')) && title.endsWith(QLatin1Char('%'))) {
        KMessageBox::error(m_appearanceUi.formatComboBox, i18n("Invalid format name."));

        return;
    }

    if (title.isEmpty()) {
        return;
    }

    int index = (m_appearanceUi.formatComboBox->currentIndex() + 1);
    const int builInFormats = formats(false).count();

    if (index <= builInFormats) {
        index = m_appearanceUi.formatComboBox->count();
    }

    disconnect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));

    if (index == builInFormats && builInFormats == m_appearanceUi.formatComboBox->count())
    {
        m_appearanceUi.formatComboBox->insertSeparator(index);

        ++index;
    }

    m_appearanceUi.formatComboBox->insertItem(index, title, m_appearanceUi.htmlTextEdit->toPlainText());
    m_appearanceUi.formatComboBox->setCurrentIndex(index);
    m_appearanceUi.removeButton->setEnabled(true);

    connect(m_appearanceUi.formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFormat(int)));
}

void Applet::removeFormat()
{
    if (m_appearanceUi.formatComboBox->currentIndex() > formats(false).count()) {
        m_appearanceUi.formatComboBox->removeItem(m_appearanceUi.formatComboBox->currentIndex());

        if (m_appearanceUi.formatComboBox->itemData((m_appearanceUi.formatComboBox->count() - 1), Qt::DisplayRole).toString().isEmpty()) {
            m_appearanceUi.formatComboBox->removeItem(m_appearanceUi.formatComboBox->count() - 1);
        }
    }
}

void Applet::updateControls()
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

void Applet::triggerAction()
{
    QString actionName = sender()->objectName().remove(QLatin1String("Button")).toLower();
    QHash<QString, QWebPage::WebAction> actions;
    actions[QLatin1String("bold")] = QWebPage::ToggleBold;
    actions[QLatin1String("italic")] = QWebPage::ToggleItalic;
    actions[QLatin1String("underline")] = QWebPage::ToggleUnderline;
    actions[QLatin1String("justifyLeft")] = QWebPage::AlignLeft;
    actions[QLatin1String("justifyCenter")] = QWebPage::AlignCenter;
    actions[QLatin1String("justifyRight")] = QWebPage::AlignRight;

    if (!actions.contains(actionName)) {
        return;
    }

    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();

        switch (actions[actionName]) {
        case QWebPage::ToggleBold:
            cursor.insertText(QLatin1String("<b>") + cursor.selectedText() + QLatin1String("</b>"));

            break;
        case QWebPage::ToggleItalic:
            cursor.insertText(QLatin1String("<i>") + cursor.selectedText() + QLatin1String("</i>"));

            break;
        case QWebPage::ToggleUnderline:
            cursor.insertText(QLatin1String("<u>") + cursor.selectedText() + QLatin1String("</u>"));

            break;
        case QWebPage::AlignLeft:
            cursor.insertText(QLatin1String("<div style=\"text-align:left;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        case QWebPage::AlignCenter:
            cursor.insertText(QLatin1String("<div style=\"text-align:center;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        case QWebPage::AlignRight:
            cursor.insertText(QLatin1String("<div style=\"text-align:right;\">") + cursor.selectedText() + QLatin1String("</div>"));

            break;
        default:
            return;
        }

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->triggerAction(actions[actionName]);
    }
}

void Applet::selectColor()
{
    KColorDialog colorDialog;
    colorDialog.setAlphaChannelEnabled(true);
    colorDialog.setColor(m_appearanceUi.colorButton->palette().button().color());
    colorDialog.setButtons(KDialog::Ok | KDialog::Cancel);

    if (colorDialog.exec() == QDialog::Accepted) {
        QPalette palette = m_appearanceUi.colorButton->palette();
        palette.setBrush(QPalette::Button, colorDialog.color());

        m_appearanceUi.colorButton->setPalette(palette);

        if (m_appearanceUi.tabWidget->currentIndex() > 0) {
            QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
            cursor.insertText(QLatin1String("<span style=\"color:") + colorDialog.color().name() + QLatin1String(";\">") + cursor.selectedText() + QLatin1String("</span>"));

            m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
        } else {
            m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('forecolor', false, '") + colorDialog.color().name() + QLatin1String("')"));
        }
    }
}

void Applet::selectFontSize(const QString &size)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QLatin1String("<span style=\"font-size:") + QString::number(size.toInt()) + QLatin1String("px;\">") + cursor.selectedText() + QLatin1String("</span>"));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontsizedelta', false, ") + QString::number(size.toInt() - m_fontSize) + QLatin1String(")"));
    }

    m_fontSize = size.toInt();
}

void Applet::selectFontFamily(const QFont &font)
{
    if (m_appearanceUi.tabWidget->currentIndex() > 0) {
        QTextCursor cursor = m_appearanceUi.htmlTextEdit->textCursor();
        cursor.insertText(QLatin1String("<span style=\"font-family:'") + font.family()+ QLatin1String("';\">") + cursor.selectedText() + QLatin1String("</span>"));

        m_appearanceUi.htmlTextEdit->setTextCursor(cursor);
    } else {
        m_appearanceUi.webView->page()->mainFrame()->evaluateJavaScript(QLatin1String("document.execCommand('fontname', false, '") + font.family() + QLatin1String("')"));
    }
}

void Applet::setColor(const QString &color)
{
    if (color == QLatin1String("false")) {
        return;
    }

    QRegExp expression = QRegExp(QLatin1String("rgb\\((\\d+), (\\d+), (\\d+)\\)"));
    expression.indexIn(color);

    const QStringList rgb = expression.capturedTexts();

    QPalette palette = m_appearanceUi.colorButton->palette();
    palette.setBrush(QPalette::Button, QColor(rgb.at(1).toInt(), rgb.at(2).toInt(), rgb.at(3).toInt()));

    m_appearanceUi.colorButton->setPalette(palette);
}

void Applet::setFontSize(const QString &size)
{
    if (!m_appearanceUi.fontSizeComboBox->hasFocus()) {
        m_appearanceUi.fontSizeComboBox->setEditText(size);
    }

    m_fontSize = size.toInt();
}

void Applet::setFontFamily(const QString &font)
{
    m_appearanceUi.fontFamilyComboBox->setCurrentFont(QFont(font));
}

void Applet::selectionChanged()
{
    m_controlsTimer = startTimer(250);

    if (m_appearanceUi.webView->page()->selectedText().endsWith(QLatin1Char('%'))) {
        m_appearanceUi.webView->page()->triggerAction(QWebPage::SelectNextChar);
    }
}

void Applet::itemSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = m_clipboardUi.clipboardActionsTable->selectedItems();

    m_clipboardUi.moveUpButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.first()) != 0);
    m_clipboardUi.moveDownButton->setEnabled(!selectedItems.isEmpty() && m_clipboardUi.clipboardActionsTable->row(selectedItems.last()) != (m_clipboardUi.clipboardActionsTable->rowCount() - 1));
    m_clipboardUi.deleteButton->setEnabled(!selectedItems.isEmpty());
}

void Applet::insertRow()
{
    const int row = ((m_clipboardUi.clipboardActionsTable->rowCount() && m_clipboardUi.clipboardActionsTable->currentRow() >= 0) ? m_clipboardUi.clipboardActionsTable->currentRow() : 0);

    m_clipboardUi.clipboardActionsTable->insertRow(row);
    m_clipboardUi.clipboardActionsTable->setItem(row, 0, new QTableWidgetItem(QString()));

    QTableWidgetItem *item = new QTableWidgetItem(QString());
    item->setFlags(0);

    m_clipboardUi.clipboardActionsTable->setItem(row, 1, item);
    m_clipboardUi.clipboardActionsTable->setCurrentCell(row, 0);
}

void Applet::deleteRow()
{
    m_clipboardUi.clipboardActionsTable->removeRow(m_clipboardUi.clipboardActionsTable->row(m_clipboardUi.clipboardActionsTable->selectedItems().at(0)));
}

void Applet::moveRow(bool up)
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

void Applet::moveRowUp()
{
    moveRow(true);
}

void Applet::moveRowDown()
{
    moveRow(false);
}

void Applet::updateRow(int row, int column)
{
    Q_UNUSED(column)

    if (!m_clipboardUi.clipboardActionsTable->item(row, 1)) {
        return;
    }

    const QString preview = evaluateFormat(m_clipboardUi.clipboardActionsTable->item(row, 0)->text(), m_dateTime);

    m_clipboardUi.clipboardActionsTable->item(row, 1)->setText(preview);
    m_clipboardUi.clipboardActionsTable->item(row, 1)->setToolTip(preview);
}

void Applet::toolTipAboutToShow()
{
    updateToolTipContent();
}

void Applet::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);
}

void Applet::setHtml(const QString &html, const QString &css)
{
    if (html != m_currentHtml) {
        m_page.mainFrame()->setHtml(QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"><html><head><style type=\"text/css\">html, body, table, td {margin:0; padding:0; height:100%; width:100%; vertical-align:middle;}") + css + QLatin1String("</style></head><body><table><tr><td id=\"clock\">") + html + QLatin1String("</td></tr></table></body></html>"));

        m_currentHtml = html;

        update();
    }
}

void Applet::copyToClipboard(QAction *action)
{
    QApplication::clipboard()->setText(action->text());
}

void Applet::updateClipboardMenu()
{
    const QDateTime dateTime = currentDateTime();
    const QStringList clipboardFormats = config().readEntry("clipboardFormats", m_clipboardFormats);

    qDeleteAll(m_clipboardAction->menu()->actions());

    m_clipboardAction->menu()->clear();

    for (int i = 0; i < clipboardFormats.count(); ++i) {
        if (clipboardFormats.at(i).isEmpty()) {
            m_clipboardAction->menu()->addSeparator();
        } else {
            m_clipboardAction->menu()->addAction(evaluateFormat(clipboardFormats.at(i), dateTime));
        }
    }
}

void Applet::changeEngineTimezone(const QString &oldTimezone, const QString &newTimezone)
{
    dataEngine(QLatin1String("time"))->disconnectSource(oldTimezone, this);

    connectSource(newTimezone);
}

void Applet::updateToolTipContent()
{
    Plasma::ToolTipContent toolTipData;
    QString toolTipFormat;

    if (config().keyList().contains(QLatin1String("toolTipFormat"))) {
        toolTipFormat = config().readEntry("toolTipFormat", QString());
    } else {
        toolTipFormat = QLatin1String("<div style=\"text-align:center;\">%Y-%M-%d<br />%h:%m:%s</div>");
    }

    if (!toolTipFormat.isEmpty()) {
        toolTipData.setImage(KIcon(QLatin1String("chronometer")).pixmap(IconSize(KIconLoader::Desktop)));
        toolTipData.setMainText(evaluateFormat(toolTipFormat, m_dateTime));
        toolTipData.setAutohide(false);
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTipData);
}

void Applet::updateSize()
{
    const Format format = this->format();
    QString string;
    QString longest;
    QString temporary;
    int placeholder;
    const int length = (format.html.length() - 1);
    int number = 0;

    for (int index = 0; index < length; ++index) {
        if (format.html.at(index) == QLatin1Char('%') && format.html.at(index + 1) != QLatin1Char('%')) {
            ++index;

            if (format.html.at(index).isDigit()) {
                while (format.html.at(index).isDigit()) {
                    ++index;
                }

                ++index;

                string.append(QLatin1Char('W'));

                continue;
            }

            placeholder = format.html.at(index).unicode();

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
                string.append(format.html.at(index));

                break;
            }
        } else {
            string.append(format.html.at(index));
        }
    }

    if (format.html.at(length - 1) != QLatin1Char('%')) {
        string.append(format.html.at(length));
    }

    setHtml(string, format.css);

    m_page.setViewportSize(QSize(0, 0));
    m_page.mainFrame()->setZoomFactor(1);

    QSizeF size;

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

    setHtml(evaluateFormat(this->format().html, m_dateTime), this->format().css);
}

void Applet::updateTheme()
{
    QPalette palette = m_page.palette();
    palette.setBrush(QPalette::Base, Qt::transparent);

    m_page.setPalette(palette);
    m_page.mainFrame()->evaluateJavaScript(QLatin1String("document.fgColor = '") + Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor).name() + QLatin1Char('\''));

    update();
}

QDateTime Applet::currentDateTime() const
{
    Plasma::DataEngine::Data data = dataEngine(QLatin1String("time"))->query(currentTimezone());

    return QDateTime(data[QLatin1String("Date")].toDate(), data[QLatin1String("Time")].toTime());
}

QString Applet::extractExpression(const QString &format) const
{
    if (format.length() < 2 || !format.contains(QLatin1Char('}'))) {
        return QString();
    }

    QString expression;
    int braces = 1;
    int i = 0;

    while (i < format.length()) {
        if (format.at(i) == QLatin1Char('{')) {
            ++braces;
        } else if (format.at(i) == QLatin1Char('}')) {
            --braces;

            if (braces == 0) {
                ++i;

                break;
            }
        }

        expression.append(format.at(i));

        ++i;
    }

    return expression;
}

QString Applet::formatNumber(int number, int length) const
{
    QString string = QString::number(number);
    int multiplier = 1;

    for (int i = 1; i < length; ++i) {
        multiplier *= 10;

        if (number < multiplier) {
            string.prepend(QLatin1Char('0'));
        }
    }

    return string;
}

QString Applet::evaluateFormat(const QString &format, QDateTime dateTime)
{
    if (format.isEmpty()) {
        return QString();
    }

    if (!dateTime.isValid()) {
        dateTime = currentDateTime();
    }

    QString string;
    const int length = (format.length() - 1);

    for (int i = 0; i < length; ++i) {
        if (format.at(i) != QLatin1Char('%')) {
            string.append(format.at(i));

            continue;
        }

        QString substitution;
        QPair<int, int> range = qMakePair(-1, -1);
        int alternativeForm = 0;
        bool shortForm = false;
        bool textualForm = false;

        ++i;

        if (format.at(i).isDigit() || (format.at(i) == QLatin1Char('-') && format.at(i + 1).isDigit())) {
            QString number;

            while ((format.at(i).isDigit() || format.at(i) == QLatin1Char('-')) && i < length) {
                number.append(format.at(i));

                ++i;
            }

            range.first = number.toInt();

            if (format.at(i) == QLatin1Char(':')) {
                number = QString();

                ++i;

                while ((format.at(i).isDigit() || format.at(i) == QLatin1Char('-')) && i < length) {
                    number.append(format.at(i));

                    ++i;
                }

                range.second = number.toInt();
            }
        }

        if (format.at(i) == QLatin1Char('!')) {
            ++i;

            shortForm = true;
        }

        if (format.at(i) == QLatin1Char('$')) {
            ++i;

            textualForm = true;
        }

        if (format.at(i) == QLatin1Char('+')) {
            ++i;

            alternativeForm = 1;
        } else if (format.at(i) == QLatin1Char('-')) {
            ++i;

            alternativeForm = -1;
        }

        if (format.at(i) == QLatin1Char('{')) {
            QString expression = extractExpression(format.mid(i + 1));
            QScriptValue scriptExpression = m_engine.evaluate(evaluateFormat(expression, dateTime));

            i += (expression.length() + 2);

            if ((format.at(i) == QLatin1Char('?') || format.at(i) == QLatin1Char(':')) && format.at(i + 1) == QLatin1Char('{')) {
                QString expression = extractExpression(format.mid(i + 2));

                if ((format.at(i) == QLatin1Char('?') && scriptExpression.toBool()) || (format.at(i) == QLatin1Char(':') && !scriptExpression.toBool())) {
                    substitution.append(evaluateFormat(expression, dateTime));
                }

                i += (expression.length() + 3);

                if (format.at(i) == QLatin1Char(':') && format.at(i + 1) == QLatin1Char('{')) {
                    expression = extractExpression(format.mid(i + 2));

                    if (!scriptExpression.toBool()) {
                        substitution.append(evaluateFormat(expression, dateTime));
                    }

                    i += (expression.length() + 2);
                } else {
                    --i;
                }
            } else {
                substitution.append(scriptExpression.toString());

                --i;
            }
        } else {
            switch (format.at(i).unicode()) {
            case 's': // Second
                substitution = formatNumber(dateTime.time().second(), (shortForm ? 2 : 0));

                break;
            case 'm': // Minute
                substitution = formatNumber(dateTime.time().minute(), (shortForm ? 2 : 0));

                break;
            case 'h': // Hour
                alternativeForm = ((alternativeForm == 0) ? KGlobal::locale()->use12Clock() : (alternativeForm == 1));

                substitution = formatNumber((alternativeForm ? (((dateTime.time().hour() + 11) % 12) + 1) : dateTime.time().hour()), (shortForm ? 2 : 0));

                break;
            case 'p': // The pm or am string
                substitution = ((dateTime.time().hour() >= 12) ? i18n("pm") : i18n("am"));

                break;
            case 'd': // Day of the month
                substitution = formatNumber(dateTime.date().day(), (shortForm ? 2 : 0));

                break;
            case 'w': // Weekday
                if (textualForm) {
                    substitution = calendar()->weekDayName(calendar()->dayOfWeek(dateTime.date()), (shortForm ? KCalendarSystem::ShortDayName : KCalendarSystem::LongDayName));
                } else {
                    substitution = formatNumber(calendar()->dayOfWeek(dateTime.date()), (shortForm ? QString::number(calendar()->daysInWeek(dateTime.date())).length() : 0));
                }

                break;
            case 'D': // Day of the year
                substitution = formatNumber(calendar()->dayOfYear(dateTime.date()), (shortForm ? QString::number(calendar()->daysInYear(dateTime.date())).length() : 0));

                break;
            case 'W': // Week
                substitution = formatNumber(calendar()->week(dateTime.date()), (shortForm ? QString::number(calendar()->weeksInYear(dateTime.date())).length() : 0));

                break;
            case 'M': // Month

                if (textualForm) {
                    alternativeForm = ((alternativeForm == 0) ? KGlobal::locale()->dateMonthNamePossessive() : (alternativeForm == 1));

                    substitution = calendar()->monthName(dateTime.date(), (shortForm ? (alternativeForm ? KCalendarSystem::ShortNamePossessive : KCalendarSystem::ShortName) : (alternativeForm ? KCalendarSystem::LongNamePossessive : KCalendarSystem::LongName)));
                } else {
                    substitution = formatNumber(calendar()->month(dateTime.date()), (shortForm ? 0 : QString::number(calendar()->monthsInYear(dateTime.date())).length()));
                }

                break;
            case 'Y': // Year
                substitution = calendar()->formatDate(dateTime.date(), KLocale::Year, (shortForm ? KLocale::ShortNumber : KLocale::LongNumber));

                break;
            case 'U': // UNIX timestamp
                substitution = QString::number(dateTime.toTime_t());

                break;
            case 't': // Time
                substitution = KGlobal::locale()->formatTime(dateTime.time(), !shortForm);

                break;
            case 'T': // Date
                substitution = KGlobal::locale()->formatDate(dateTime.date(), (shortForm ? KLocale::ShortDate : KLocale::LongDate));

                break;
            case 'A': // Date and time
                substitution = KGlobal::locale()->formatDateTime(dateTime, (shortForm ? KLocale::ShortDate : KLocale::LongDate));

                break;
            case 'c': // Timezone city
                substitution = prettyTimezone();

                break;
            case 'a': // Timezone abbreviation
                substitution = m_timeZoneAbbreviation;

                break;
            case 'o': // Timezone UTC offset
                substitution = m_timeZoneOffset;

                break;
            case 'H': // Holiday name
                substitution = m_holiday;

                break;
            case 'E': // Events list
                substitution = m_events;

                break;
            case 'R': // Sunrise time
                substitution = KGlobal::locale()->formatTime(m_sunrise, false);

                break;
            case 'S': // Sunset time
                substitution = KGlobal::locale()->formatTime(m_sunset, false);

                break;
            default:
                substitution.append(format.at(i));

                break;
            }
        }

        if (range.first != -1 || range.second != -1) {
            if (range.first < 0) {
                range.first = (substitution.length() + range.first);
            }

            if (range.second < -1) {
                range.second = (substitution.length() + range.second);
            }

            string.append(substitution.mid(range.first, range.second));
        } else {
            string.append(substitution);
        }
    }

    return string;
}

QString Applet::holiday() const
{
    const QString region = config().readEntry("holidaysRegions", dataEngine(QLatin1String("calendar"))->query(QLatin1String("holidaysDefaultRegion"))[QLatin1String("holidaysDefaultRegion")]).toString().split(QLatin1Char(',')).first();
    const QString key = QLatin1String("holidays:") + region + QLatin1Char(':') + currentDateTime().date().toString(Qt::ISODate);
    Plasma::DataEngine::Data holidays = dataEngine(QLatin1String("calendar"))->query(key);

    if (holidays.isEmpty() || holidays[key].toList().isEmpty()) {
        return QString();
    }

    return holidays[key].toList().first().toHash()[QLatin1String("Name")].toString();
}

Format Applet::format(QString name) const
{
    if (name.isEmpty()) {
        if (!m_format.html.isEmpty()) {
            return m_format;
        }

        name = config().readEntry("format", "%default%");
    }

    QHash<QString, Format> formats;
    formats[QLatin1String("%default%")] = Format();
    formats[QLatin1String("%default%")].title = i18n("Default");
    formats[QLatin1String("%default%")].html = QLatin1String("<div style=\"text-align:center; margin:5px; white-space:pre;\"><big>%h:%m:%s</big>\n<small>%d.%M.%Y</small></div>");
    formats[QLatin1String("%default%")].background = true;
    formats[QLatin1String("%flat%")] = Format();
    formats[QLatin1String("%flat%")].title = i18n("Flat");
    formats[QLatin1String("%flat%")].html = QLatin1String("<div style=\"text-align:center; margin:5px; white-space:pre;\"><big style=\"font-family:'Nimbus Sans L Condensed';\">%h:%m:%s</big>\n<span style=\"font-size:small; font-family:'Nimbus Sans L';\">%d.%M.%Y</small></div>");
    formats[QLatin1String("%flat%")].background = true;
    formats[QLatin1String("%simple%")] = Format();
    formats[QLatin1String("%simple%")].title = i18n("Simple");
    formats[QLatin1String("%simple%")].html = QLatin1String("<div style=\"text-align:center; white-space:pre; font-size:25px; margin:5px;\">%h:%m</div>");
    formats[QLatin1String("%simple%")].background = true;
    formats[QLatin1String("%verbose%")] = Format();
    formats[QLatin1String("%verbose%")].title = i18n("Verbose");
    formats[QLatin1String("%verbose%")].html = QLatin1String("<div style=\"text-align:center; white-space:pre; opacity:0.85;\"><span style=\"font-size:30px;\">%h:%m:%s</span><br><span style=\"font-size:12px;\">%$w, %d.%M.%Y</span></div>");
    formats[QLatin1String("%verbose%")].background = false;
    formats[QLatin1String("%dbclock%")] = Format();
    formats[QLatin1String("%dbclock%")].title = i18n("dbClock");
    formats[QLatin1String("%dbclock%")].html = QLatin1String("<div style=\"height:50px;\"><div style=\"text-align:center; white-space:pre; font-size:25px; margin:-10px 0 5px 5px; -webkit-box-reflect:below -5px -webkit-gradient(linear, left top, left bottom, from(transparent), color-stop(0.5, transparent), to(white));\">%h:%m<span style=\"font-size:30px; position:relative; left:-8px; top:4px; z-index:-1; opacity:0.5;\">%s</span></div></div>");
    formats[QLatin1String("%dbclock%")].background = false;
    formats[QLatin1String("%calendar%")] = Format();
    formats[QLatin1String("%calendar%")].title = i18n("Calendar");
    formats[QLatin1String("%calendar%")].html = QLatin1String("<div style=\"width:295px; min-height:295px; text-shadow:0 0 5px #AAA;\"><div style=\"margin:30px 0 0 0; padding:30px 20px 20px 20px; position:relative; font-weight:bold; font-size:30px; text-align:center; background:-webkit-gradient(linear, left top, left bottom, from(#E5702B), to(#A33B03)); color:white; border-radius:20px; box-shadow:5px 5px 15px #888; opacity:0.7;\">%$w<br /><span style=\"font-size:130px; line-height:140px;\">%!d</span><br /><span style=\"font-size:35px;\">%$M %Y</span><br />%H<div class=\"decor\" style=\"position:absolute; top:-30px; left:-10px; width:310px; height:60px; padding:10px 20px;\"><div></div><div></div><div></div><div></div><div></div><div></div></div></div></div>");
    formats[QLatin1String("%calendar%")].css = QLatin1String(".decor div{width:13px; height:40px; margin:0 16px; float:left; background:-webkit-gradient(linear, left top, left bottom, color-stop(0, #757575), color-stop(0.5, #F7F7F7), color-stop(1, #757575)); border:1px solid #999; box-shadow:0 0 5px #AAA;}");
    formats[QLatin1String("%calendar%")].background = false;

    if (formats.contains(name)) {
        return formats[name];
    }

    if (config().group("Formats").groupList().contains(name)) {
        KConfigGroup formatConfiguration = config().group("Formats").group(name);
        Format format;
        format.title = formatConfiguration.readEntry("title", i18n("Custom"));
        format.html = formatConfiguration.readEntry("html", QString());
        format.css = formatConfiguration.readEntry("css", QString());
        format.background = formatConfiguration.readEntry("background", true);

        if (!format.html.isEmpty()) {
            return format;
        }
    }

    return formats[QLatin1String("%default%")];
}

QStringList Applet::formats(bool all) const
{
    QStringList formats;
    formats << QLatin1String("%default%") << QLatin1String("%flat%") << QLatin1String("%simple%") << QLatin1String("%verbose%") << QLatin1String("%dbclock%") << QLatin1String("%dbclock2%") << QLatin1String("%calendar%");

    if (all) {
        const int count = formats.count();

        QStringList userFormats = config().group("Formats").groupList();

        for (int i = 0; i < userFormats.count(); ++i) {
            if (!formats.contains(userFormats.at(i))) {
                formats.append(userFormats.at(i));
            }
        }

        if (count != formats.count()) {
            formats.insert(count,  QLatin1String(""));
        }
    }

    return formats;
}

QList<QAction*> Applet::contextualActions()
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

}
