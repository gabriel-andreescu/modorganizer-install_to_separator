#include "Settings.h"

#include <QVariant>

#include <uibase/imoinfo.h>

namespace {
constexpr auto enableQuickInstallKey = "enable_quick_install";
constexpr auto enableXmlFomodKey = "enable_xml_fomod";
constexpr auto enableCSharpFomodKey = "enable_csharp_fomod";
constexpr auto enableBainInstallerKey = "enable_bain_installer";
constexpr auto enableBainWizardInstallerKey = "enable_bain_wizard_installer";
constexpr auto enableManualInstallKey = "enable_manual_install";
constexpr auto rememberLastSeparatorKey = "remember_last_separator";

bool boolSetting(
    const MOBase::IOrganizer* organizer,
    const QString& pluginName,
    const QString& key,
    bool defaultValue
) {
    if (organizer == nullptr) {
        return defaultValue;
    }

    const QVariant value = organizer->pluginSetting(pluginName, key);
    return value.isValid() ? value.toBool() : defaultValue;
}

QString settingKeyForDialog(Settings::Dialog dialog) {
    switch (dialog) {
        case Settings::Dialog::QuickInstall:        return enableQuickInstallKey;
        case Settings::Dialog::XmlFomod:            return enableXmlFomodKey;
        case Settings::Dialog::CSharpFomod:         return enableCSharpFomodKey;
        case Settings::Dialog::BainInstaller:       return enableBainInstallerKey;
        case Settings::Dialog::BainWizardInstaller: return enableBainWizardInstallerKey;
        case Settings::Dialog::ManualInstall:       return enableManualInstallKey;
        case Settings::Dialog::Unknown:             return {};
    }

    return {};
}
}

namespace Settings {
QList<MOBase::PluginSetting> all() {
    return {
        MOBase::PluginSetting(
            enableQuickInstallKey,
            "show the separator picker in Quick Install dialogs",
            QVariant(true)
        ),
        MOBase::PluginSetting(
            enableXmlFomodKey,
            "show the separator picker in XML FOMOD installer dialogs",
            QVariant(true)
        ),
        MOBase::PluginSetting(enableCSharpFomodKey, "show the separator picker in C# FOMOD dialogs", QVariant(true)),
        MOBase::PluginSetting(
            enableBainInstallerKey,
            "show the separator picker in BAIN Installer dialogs",
            QVariant(true)
        ),
        MOBase::PluginSetting(
            enableBainWizardInstallerKey,
            "show the separator picker in BAIN Wizard Installer dialogs",
            QVariant(true)
        ),
        MOBase::PluginSetting(
            enableManualInstallKey,
            "show the separator picker in Manual Install dialogs",
            QVariant(true)
        ),
        MOBase::PluginSetting(
            rememberLastSeparatorKey,
            "remember the last selected separator per MO2 profile",
            QVariant(true)
        ),
    };
}

Dialog dialogForObjectName(const QString& objectName) {
    if (objectName == "SimpleInstallDialog") {
        return Dialog::QuickInstall;
    }
    if (objectName == "FomodInstallerDialog") {
        return Dialog::XmlFomod;
    }
    if (objectName == "FomodCSharpPredialog") {
        return Dialog::CSharpFomod;
    }
    if (objectName == "BainComplexInstallerDialog") {
        return Dialog::BainInstaller;
    }
    if (objectName == "WizardInstallerDialog") {
        return Dialog::BainWizardInstaller;
    }
    if (objectName == "InstallDialog") {
        return Dialog::ManualInstall;
    }

    return Dialog::Unknown;
}

bool dialogEnabled(const MOBase::IOrganizer* organizer, const QString& pluginName, Dialog dialog) {
    const QString key = settingKeyForDialog(dialog);
    if (key.isEmpty()) {
        return false;
    }

    return boolSetting(organizer, pluginName, key, true);
}

bool rememberLastSeparator(const MOBase::IOrganizer* organizer, const QString& pluginName) {
    return boolSetting(organizer, pluginName, rememberLastSeparatorKey, true);
}
}
