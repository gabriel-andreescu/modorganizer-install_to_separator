#pragma once

#include <QList>
#include <QString>

#include <uibase/pluginsetting.h>

namespace MOBase {
class IOrganizer;
}

namespace Settings {
enum class Dialog {
    Unknown,
    QuickInstall,
    XmlFomod,
    CSharpFomod,
    BainInstaller,
    BainWizardInstaller,
    ManualInstall,
};

[[nodiscard]] QList<MOBase::PluginSetting> all();
[[nodiscard]] Dialog dialogForObjectName(const QString& objectName);
[[nodiscard]] bool dialogEnabled(const MOBase::IOrganizer* organizer, const QString& pluginName, Dialog dialog);
[[nodiscard]] bool rememberLastSeparator(const MOBase::IOrganizer* organizer, const QString& pluginName);
}
