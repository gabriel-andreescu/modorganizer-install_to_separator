#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QString>

#include <uibase/iplugininstaller.h>

class QComboBox;
class QDialog;
class QEvent;
class QWidget;
class QAbstractItemModel;
class QMainWindow;
class QTreeView;

namespace MOBase {
class IOrganizer;
}

class InstallToSeparator final : public QObject, public MOBase::IPluginInstaller {
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginInstaller)
    Q_PLUGIN_METADATA(IID "org.gabonz.InstallToSeparator" FILE "installtoseparator.json")

public:
    InstallToSeparator() = default;
    ~InstallToSeparator() override;

    bool init(MOBase::IOrganizer* organizer) override;
    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString localizedName() const override;
    [[nodiscard]] QString author() const override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] MOBase::VersionInfo version() const override;
    [[nodiscard]] QList<MOBase::PluginSetting> settings() const override;
    [[nodiscard]] bool enabledByDefault() const override;

    [[nodiscard]] unsigned int priority() const override;
    [[nodiscard]] bool isManualInstaller() const override;
    [[nodiscard]] bool isArchiveSupported(std::shared_ptr<const MOBase::IFileTree> tree) const override;
    void onInstallationStart(const QString& archive, bool reinstallation, MOBase::IModInterface* currentMod) override;
    void onInstallationEnd(EInstallResult result, MOBase::IModInterface* newMod) override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void markNextInstallDragPositioned();

private:
    struct SeparatorChoice {
        QString internalName;
        QString displayName;
        QColor color;
    };

    [[nodiscard]] QList<SeparatorChoice> separatorChoices() const;
    [[nodiscard]] QString persistentKey() const;
    [[nodiscard]] QString rememberedSeparator() const;
    [[nodiscard]] bool rememberLastSeparator() const;
    [[nodiscard]] QTreeView* modListView() const;
    [[nodiscard]] bool modListPriorityDescending() const;
    [[nodiscard]] QString defaultSeparatorLabel() const;
    [[nodiscard]] int targetPriorityFor(const QString& separatorName) const;
    [[nodiscard]] bool isTargetDialog(const QWidget* widget) const;
    [[nodiscard]] QAbstractItemModel* sourceModListModel() const;
    void connectDragInstallSignals();
    bool decorateDialog(QDialog* dialog);
    bool moveModThroughMo2Model(const QString& modName, int targetPriority) const;
    void rememberSeparator(const QString& separatorName) const;
    void resetInstallState();

    MOBase::IOrganizer* m_Organizer {};
    QPointer<QMainWindow> m_MainWindow;
    bool m_EventFilterInstalled {};
    bool m_DragInstallSignalsConnected {};
    int m_PendingDragPositionedInstalls {};
    bool m_TargetingActive {};
    bool m_HasPendingSeparator {};
    QString m_PendingSeparator;
    QString m_SelectedSeparator;
};
