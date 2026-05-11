#include "InstallToSeparator.h"
#include "Settings.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QBoxLayout>
#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QDialog>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QMetaObject>
#include <QModelIndexList>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <limits>

#include <uibase/imodinterface.h>
#include <uibase/imodlist.h>
#include <uibase/imoinfo.h>

namespace {
constexpr auto pluginInternalName = "Install to Separator";
constexpr auto decoratedProperty = "installToSeparatorDecorated";
constexpr auto guardedComboProperty = "installToSeparatorGuardedCombo";
constexpr auto invalidComboProperty = "installToSeparatorInvalid";
constexpr auto buttonBlockedProperty = "installToSeparatorButtonBlocked";
constexpr auto buttonBaseEnabledProperty = "installToSeparatorButtonBaseEnabled";
constexpr auto modListObjectName = "modList";
constexpr auto changeModsPrioritySlot = "changeModsPriority(QModelIndexList,int)";
constexpr auto invalidComboStyleSheet = R"(
QComboBox#installToSeparatorCombo[installToSeparatorInvalid="true"] {
  border: 1px solid #d84a4a;
  border-radius: 4px;
}

QComboBox#installToSeparatorCombo[installToSeparatorInvalid="true"]:focus {
  border: 2px solid #e05252;
}
)";

struct LayoutInsertionPoint {
    QBoxLayout* parent {};
    int index {};
};

QString displayNameForSeparator(MOBase::IModList* modList, const QString& internalName) {
    QString displayName = modList->displayName(internalName);
    if (displayName.endsWith("_separator", Qt::CaseInsensitive)) {
        displayName.chop(10);
    }
    return displayName.isEmpty() ? internalName : displayName;
}

QIcon colorSwatchIcon(const QColor& color) {
    if (!color.isValid()) {
        return {};
    }

    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF rect(3.0, 4.0, 12.0, 10.0);
    QPainterPath path;
    path.addRoundedRect(rect, 3.0, 3.0);
    painter.fillPath(path, color);

    QColor outline = color;
    outline.setAlpha(std::min(180, std::max(80, color.alpha())));
    painter.setPen(QPen(outline.darker(135), 1.0));
    painter.drawPath(path);
    return {pixmap};
}

QHBoxLayout* horizontalLayoutContainingWidget(QLayout* layout, const QWidget* widget) {
    if (layout == nullptr || widget == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < layout->count(); ++i) {
        auto* item = layout->itemAt(i);
        if (item == nullptr) {
            continue;
        }

        if (item->widget() == widget) {
            return qobject_cast<QHBoxLayout*>(layout);
        }

        if (auto* nested = horizontalLayoutContainingWidget(item->layout(), widget); nested != nullptr) {
            return nested;
        }

        if (item->widget() != nullptr) {
            auto* nested = horizontalLayoutContainingWidget(item->widget()->layout(), widget);
            if (nested != nullptr) {
                return nested;
            }
        }
    }

    return nullptr;
}

LayoutInsertionPoint insertionPointAfterLayout(QLayout* layout, const QLayout* target) {
    if (layout == nullptr || target == nullptr) {
        return {};
    }

    for (int i = 0; i < layout->count(); ++i) {
        auto* item = layout->itemAt(i);
        if (item == nullptr) {
            continue;
        }

        if (item->layout() == target) {
            return {
                .parent = qobject_cast<QBoxLayout*>(layout),
                .index = i + 1,
            };
        }

        if (auto insertionPoint = insertionPointAfterLayout(item->layout(), target); insertionPoint.parent != nullptr) {
            return insertionPoint;
        }

        if (item->widget() != nullptr) {
            auto insertionPoint = insertionPointAfterLayout(item->widget()->layout(), target);
            if (insertionPoint.parent != nullptr) {
                return insertionPoint;
            }
        }
    }

    return {};
}

void addNameAlignedRowWidgets(QHBoxLayout* row, QLabel* label, QComboBox* separatorCombo, const QHBoxLayout* nameRow) {
    if (nameRow != nullptr) {
        row->setSpacing(nameRow->spacing());
        row->setContentsMargins(nameRow->contentsMargins());
        row->addWidget(label, nameRow->stretch(0));
        row->addWidget(separatorCombo, std::max(1, nameRow->stretch(1)));
        return;
    }

    row->addWidget(label);
    row->addWidget(separatorCombo, 1);
}

int exactComboTextIndex(const QComboBox* combo) {
    if (combo == nullptr) {
        return -1;
    }
    return combo->findText(combo->currentText(), Qt::MatchExactly | Qt::MatchCaseSensitive);
}

QString comboItemData(const QComboBox* combo, int index) {
    if (combo == nullptr || index < 0) {
        return {};
    }
    return combo->itemData(index).toString();
}

void updatePendingSeparator(const QComboBox* combo, QString& pendingSeparator, bool& hasPendingSeparator) {
    const int index = exactComboTextIndex(combo);
    if (index < 0) {
        return;
    }

    pendingSeparator = comboItemData(combo, index);
    hasPendingSeparator = true;
}

bool hasExactComboText(const QComboBox* combo) {
    return exactComboTextIndex(combo) >= 0;
}

QComboBox* guardedComboFor(QObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return qobject_cast<QComboBox*>(object->property(guardedComboProperty).value<QObject*>());
}

bool isAcceptAttempt(QEvent* event, const QComboBox* combo) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        return true;
    }

    if (event->type() != QEvent::KeyPress) {
        return false;
    }

    if (combo
        != nullptr
        && combo->completer()
        != nullptr
        && combo->completer()->popup()
        != nullptr
        && combo->completer()->popup()->isVisible()) {
        return false;
    }

    const auto* keyEvent = static_cast<QKeyEvent*>(event);
    return keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Space;
}

QList<QPushButton*> acceptButtonsFor(QDialog* dialog) {
    QList<QPushButton*> buttons;
    for (const auto& objectName : {QStringLiteral("okBtn"), QStringLiteral("nextBtn"), QStringLiteral("okButton")}) {
        if (auto* button = dialog->findChild<QPushButton*>(objectName); button != nullptr) {
            buttons.append(button);
        }
    }
    return buttons;
}

void blockAcceptButton(QPushButton* button) {
    if (button == nullptr) {
        return;
    }

    if (!button->property(buttonBlockedProperty).toBool()) {
        button->setProperty(buttonBaseEnabledProperty, button->isEnabled());
        button->setProperty(buttonBlockedProperty, true);
    }
    button->setEnabled(false);
}

void setComboInvalid(QComboBox* combo, bool invalid) {
    if (combo == nullptr || combo->property(invalidComboProperty).toBool() == invalid) {
        return;
    }

    combo->setProperty(invalidComboProperty, invalid);
    combo->style()->unpolish(combo);
    combo->style()->polish(combo);
    combo->update();

    if (combo->lineEdit() != nullptr) {
        combo->lineEdit()->style()->unpolish(combo->lineEdit());
        combo->lineEdit()->style()->polish(combo->lineEdit());
        combo->lineEdit()->update();
    }
}

void updateAcceptButtons(QComboBox* combo, const QList<QPushButton*>& buttons) {
    const bool valid = hasExactComboText(combo);
    setComboInvalid(combo, !valid);

    for (auto* button : buttons) {
        if (button == nullptr) {
            continue;
        }

        if (!valid) {
            blockAcceptButton(button);
        } else if (button->property(buttonBlockedProperty).toBool()) {
            const bool baseEnabled = button->property(buttonBaseEnabledProperty).toBool();
            button->setProperty(buttonBlockedProperty, false);
            button->setEnabled(baseEnabled);
        }
    }
}
}

InstallToSeparator::~InstallToSeparator() {
    if (m_EventFilterInstalled && qApp != nullptr) {
        qApp->removeEventFilter(this);
    }
}

bool InstallToSeparator::init(MOBase::IOrganizer* organizer) {
    m_Organizer = organizer;
    if (m_Organizer == nullptr) {
        return false;
    }

    m_Organizer->onUserInterfaceInitialized([this](QMainWindow* mainWindow) {
        m_MainWindow = mainWindow;
        if (!m_EventFilterInstalled && qApp != nullptr) {
            qApp->installEventFilter(this);
            m_EventFilterInstalled = true;
        }
    });

    return true;
}

QString InstallToSeparator::name() const {
    return pluginInternalName;
}

QString InstallToSeparator::localizedName() const {
    return tr("Install to Separator");
}

QString InstallToSeparator::author() const {
    return "GabonZ";
}

QString InstallToSeparator::description() const {
    return tr("Adds a separator picker to MO2 install dialogs.");
}

MOBase::VersionInfo InstallToSeparator::version() const {
    return {0, 1, 0, 0, MOBase::VersionInfo::RELEASE_FINAL};
}

QList<MOBase::PluginSetting> InstallToSeparator::settings() const {
    return Settings::all();
}

bool InstallToSeparator::enabledByDefault() const {
    return true;
}

unsigned int InstallToSeparator::priority() const {
    return 0;
}

bool InstallToSeparator::isManualInstaller() const {
    return false;
}

bool InstallToSeparator::isArchiveSupported(std::shared_ptr<const MOBase::IFileTree> tree) const {
    Q_UNUSED(tree);
    return false;
}

void InstallToSeparator::onInstallationStart(
    const QString& archive,
    bool reinstallation,
    MOBase::IModInterface* currentMod
) {
    Q_UNUSED(archive);

    m_SelectedSeparator.clear();
    m_PendingSeparator.clear();
    m_HasPendingSeparator = false;
    m_TargetingActive = !reinstallation && currentMod == nullptr;
}

void InstallToSeparator::onInstallationEnd(EInstallResult result, MOBase::IModInterface* newMod) {
    if (!m_TargetingActive || result != RESULT_SUCCESS || newMod == nullptr || m_SelectedSeparator.isEmpty()) {
        resetInstallState();
        return;
    }

    if (newMod->isSeparator() || newMod->isBackup() || newMod->isOverwrite() || newMod->isForeign()) {
        resetInstallState();
        return;
    }

    const int targetPriority = targetPriorityFor(m_SelectedSeparator);
    if (targetPriority < 0) {
        qWarning() << "Install to Separator: selected separator is no longer available:" << m_SelectedSeparator;
        resetInstallState();
        return;
    }

    if (!moveModThroughMo2Model(newMod->name(), targetPriority)) {
        qWarning() << "Install to Separator: failed to move" << newMod->name() << "to priority" << targetPriority;
    }

    resetInstallState();
}

bool InstallToSeparator::eventFilter(QObject* watched, QEvent* event) {
    if (auto* combo = guardedComboFor(watched); combo != nullptr && !hasExactComboText(combo)) {
        if (event->type() == QEvent::EnabledChange) {
            if (auto* button = qobject_cast<QPushButton*>(watched); button != nullptr && button->isEnabled()) {
                button->setProperty(buttonBaseEnabledProperty, true);
                blockAcceptButton(button);
            }
        }

        if (isAcceptAttempt(event, combo)) {
            if (auto* button = qobject_cast<QPushButton*>(watched); button != nullptr) {
                blockAcceptButton(button);
            }
            return true;
        }
    }

    if (event->type() == QEvent::Show && m_TargetingActive) {
        if (auto* dialog = qobject_cast<QDialog*>(watched); dialog != nullptr && isTargetDialog(dialog)) {
            decorateDialog(dialog);
        }
    }

    return QObject::eventFilter(watched, event);
}

QList<InstallToSeparator::SeparatorChoice> InstallToSeparator::separatorChoices() const {
    QList<SeparatorChoice> choices;
    if (m_Organizer == nullptr || m_Organizer->modList() == nullptr) {
        return choices;
    }

    auto* modList = m_Organizer->modList();
    for (const auto& modName : modList->allModsByProfilePriority()) {
        auto* mod = modList->getMod(modName);
        if (mod != nullptr && mod->isSeparator()) {
            choices.append({
                .internalName = modName,
                .displayName = displayNameForSeparator(modList, modName),
                .color = mod->color(),
            });
        }
    }
    return choices;
}

QString InstallToSeparator::persistentKey() const {
    const QString profileName = m_Organizer != nullptr ? m_Organizer->profileName() : QString();
    return QString("last_separator/%1").arg(profileName);
}

QString InstallToSeparator::rememberedSeparator() const {
    if (m_Organizer == nullptr || !rememberLastSeparator()) {
        return {};
    }
    return m_Organizer->persistent(name(), persistentKey(), QString()).toString();
}

bool InstallToSeparator::rememberLastSeparator() const {
    return Settings::rememberLastSeparator(m_Organizer, name());
}

int InstallToSeparator::targetPriorityFor(const QString& separatorName) const {
    if (m_Organizer == nullptr || m_Organizer->modList() == nullptr) {
        return -1;
    }

    auto* modList = m_Organizer->modList();
    const int separatorPriority = modList->priority(separatorName);
    if (separatorPriority < 0) {
        return -1;
    }

    for (const auto& modName : modList->allModsByProfilePriority()) {
        auto* mod = modList->getMod(modName);
        if (mod == nullptr || !mod->isSeparator() || modName == separatorName) {
            continue;
        }

        const int priority = modList->priority(modName);
        if (priority > separatorPriority) {
            return priority;
        }
    }

    return std::numeric_limits<int>::max();
}

bool InstallToSeparator::isTargetDialog(const QWidget* widget) const {
    if (widget == nullptr || widget->property(decoratedProperty).toBool()) {
        return false;
    }

    const auto dialog = Settings::dialogForObjectName(widget->objectName());
    return Settings::dialogEnabled(m_Organizer, name(), dialog);
}

bool InstallToSeparator::decorateDialog(QDialog* dialog) {
    if (dialog == nullptr || dialog->property(decoratedProperty).toBool()) {
        return false;
    }

    auto* layout = dialog->findChild<QVBoxLayout*>("verticalLayout");
    auto* nameCombo = dialog->findChild<QComboBox*>("nameCombo");
    auto* nameRow = horizontalLayoutContainingWidget(layout, nameCombo);
    const auto insertionPoint = insertionPointAfterLayout(layout, nameRow);
    if (layout == nullptr || nameCombo == nullptr || nameRow == nullptr || insertionPoint.parent == nullptr) {
        qWarning() << "Install to Separator: could not decorate install dialog:" << dialog->objectName();
        dialog->setProperty(decoratedProperty, true);
        return false;
    }

    auto* row = new QHBoxLayout();
    auto* label = new QLabel(tr("Separator"), dialog);
    auto* separatorCombo = new QComboBox(dialog);
    separatorCombo->setObjectName("installToSeparatorCombo");
    separatorCombo->setStyleSheet(invalidComboStyleSheet);
    separatorCombo->setEditable(true);
    separatorCombo->setInsertPolicy(QComboBox::NoInsert);
    if (separatorCombo->completer() != nullptr) {
        separatorCombo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
        separatorCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
        separatorCombo->completer()->setFilterMode(Qt::MatchContains);
    }

    label->setBuddy(separatorCombo);
    addNameAlignedRowWidgets(row, label, separatorCombo, nameRow);

    const auto choices = separatorChoices();
    for (const auto& choice : choices) {
        separatorCombo->addItem(colorSwatchIcon(choice.color), choice.displayName, choice.internalName);
    }
    const int defaultOptionIndex = separatorCombo->count();
    separatorCombo->addItem(tr("<default: end of list>"), QString());
    auto defaultOptionFont = separatorCombo->font();
    defaultOptionFont.setItalic(true);
    separatorCombo->setItemData(defaultOptionIndex, defaultOptionFont, Qt::FontRole);

    const QString defaultSeparator = m_HasPendingSeparator ? m_PendingSeparator : rememberedSeparator();
    const int defaultIndex = (m_HasPendingSeparator || !defaultSeparator.isEmpty())
                                 ? separatorCombo->findData(defaultSeparator)
                                 : defaultOptionIndex;
    separatorCombo->setCurrentIndex(defaultIndex >= 0 ? defaultIndex : defaultOptionIndex);
    updatePendingSeparator(separatorCombo, m_PendingSeparator, m_HasPendingSeparator);

    insertionPoint.parent->insertLayout(insertionPoint.index, row);
    dialog->setProperty(decoratedProperty, true);
    dialog->setProperty(guardedComboProperty, QVariant::fromValue<QObject*>(separatorCombo));

    const QPointer<QComboBox> comboPointer(separatorCombo);
    const QList<QPushButton*> acceptButtons = acceptButtonsFor(dialog);
    for (auto* button : acceptButtons) {
        button->setProperty(guardedComboProperty, QVariant::fromValue<QObject*>(separatorCombo));
    }
    if (separatorCombo->lineEdit() != nullptr) {
        separatorCombo->lineEdit()->setProperty(guardedComboProperty, QVariant::fromValue<QObject*>(separatorCombo));
    }

    QObject::connect(separatorCombo, &QComboBox::currentTextChanged, dialog, [this, comboPointer, acceptButtons] {
        updateAcceptButtons(comboPointer, acceptButtons);
        updatePendingSeparator(comboPointer, m_PendingSeparator, m_HasPendingSeparator);
    });
    QObject::connect(
        separatorCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        dialog,
        [this, comboPointer, acceptButtons] {
            updateAcceptButtons(comboPointer, acceptButtons);
            updatePendingSeparator(comboPointer, m_PendingSeparator, m_HasPendingSeparator);
        }
    );
    updateAcceptButtons(separatorCombo, acceptButtons);

    QObject::connect(dialog, &QDialog::accepted, this, [this, comboPointer] {
        if (!m_TargetingActive || comboPointer.isNull()) {
            return;
        }

        const int index = exactComboTextIndex(comboPointer);
        if (index < 0) {
            m_SelectedSeparator.clear();
            return;
        }

        m_SelectedSeparator = comboItemData(comboPointer, index);
        rememberSeparator(m_SelectedSeparator);
    });

    return true;
}

QAbstractItemModel* InstallToSeparator::sourceModListModel() const {
    if (m_MainWindow.isNull()) {
        return nullptr;
    }

    auto* modListView = m_MainWindow->findChild<QTreeView*>(modListObjectName);
    if (modListView == nullptr) {
        return nullptr;
    }

    auto* model = modListView->model();
    while (auto* proxyModel = qobject_cast<QAbstractProxyModel*>(model)) {
        model = proxyModel->sourceModel();
    }
    return model;
}

bool InstallToSeparator::moveModThroughMo2Model(const QString& modName, int targetPriority) const {
    if (m_Organizer == nullptr || m_Organizer->modList() == nullptr) {
        return false;
    }

    auto* sourceModel = sourceModListModel();
    if (sourceModel == nullptr || sourceModel->metaObject()->indexOfSlot(changeModsPrioritySlot) < 0) {
        return false;
    }

    const qsizetype sourceRow = m_Organizer->modList()->allMods().indexOf(modName);
    if (sourceRow < 0 || sourceRow >= sourceModel->rowCount()) {
        return false;
    }

    const QModelIndex sourceIndex = sourceModel->index(static_cast<int>(sourceRow), 0);
    if (!sourceIndex.isValid()) {
        return false;
    }

    const QModelIndexList indices {sourceIndex};
    return QMetaObject::invokeMethod(
        sourceModel,
        "changeModsPriority",
        Qt::DirectConnection,
        Q_ARG(QModelIndexList, indices),
        Q_ARG(int, targetPriority)
    );
}

void InstallToSeparator::rememberSeparator(const QString& separatorName) const {
    if (m_Organizer != nullptr && rememberLastSeparator()) {
        m_Organizer->setPersistent(name(), persistentKey(), separatorName);
    }
}

void InstallToSeparator::resetInstallState() {
    m_TargetingActive = false;
    m_HasPendingSeparator = false;
    m_PendingSeparator.clear();
    m_SelectedSeparator.clear();
}
