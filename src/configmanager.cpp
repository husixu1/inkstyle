#include "configmanager.hpp"

#include "constants.hpp"

#include <QDebug>
#include <yaml-cpp/yaml.h>

ConfigManager::ConfigManager(const QString configFile, QObject *parent)
    : QObject(parent) {
    using namespace C::CK;

    YAML::Node config = YAML::LoadFile(configFile.toStdString());
    if (config[globalConfig].IsDefined()) {
        if (!config[globalConfig].IsMap())
            qWarning() << "'" << globalConfig << "' is not a map, skpping";

        // TODO: load global configs
        if (config[G::panelBgColor].IsScalar()) {
        }
        if (config[G::panelRadius].IsScalar()) {
        }
        //...
    }

    if (config[buttonsConfig].IsDefined()) {
        if (!config[buttonsConfig].IsSequence())
            qWarning() << "'" << buttonsConfig << "' is not a list, skipping";

        size_t numButtons = 0;
        for (const YAML::Node &node : config[buttonsConfig]) {
            if (!node.IsMap())
                qWarning() << "The " << (numButtons + 1)
                           << "-th button is not defined as a map. Skipping...";
            if (!node[B::slot].IsDefined())
                qWarning() << "The " << (numButtons + 1)
                           << "-th button does not have a 'slot' key. Skipping";
            quint16 slot = node[B::slot].as<quint16>();
            qDebug() << "Registering button " << Qt::hex << slot;

            if (buttons.contains(slot)) {
                qWarning() << "";
            }
            // TODO: load buttons

            ++numButtons;
        }
    }
}
