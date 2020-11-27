/*
 * Copyright (c) 2020 Valve Corporation
 * Copyright (c) 2020 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors:
 * - Richard S. Wright Jr. <richard@lunarg.com>
 * - Christophe Riccio <christophe@lunarg.com>
 */

#include "layer.h"
#include "platform.h"
#include "util.h"
#include "path.h"
#include "json.h"

#include <QFile>
#include <QMessageBox>
#include <QJsonArray>

#include <cassert>

enum LayerID {
    LAYER_THIRD_PARTY = -1,
    LAYER_KHRONOS_VALIDATION = 0,
    LAYER_LUNARG_API_DUMP,
    LAYER_LUNARG_DEVICE_SIMULATION,
    LAYER_LUNARG_GFXRECONSTRUCT,
    LAYER_LUNARG_MONITOR,
    LAYER_LUNARG_SCREENSHOT,

    LAYER_FIRST = LAYER_KHRONOS_VALIDATION,
    LAYER_LAST = LAYER_LUNARG_SCREENSHOT
};

enum { LAYER_COUNT = LAYER_LAST - LAYER_FIRST + 1 };

struct DefaultLayer {
    LayerID id;
    const char* name;
    const char* file;
};

static const DefaultLayer default_layers[] = {
    {LAYER_KHRONOS_VALIDATION, "VK_LAYER_KHRONOS_validation", "VkLayer_khronos_validation.json"},
    {LAYER_LUNARG_API_DUMP, "VK_LAYER_LUNARG_api_dump", "VkLayer_api_dump.json"},
    {LAYER_LUNARG_DEVICE_SIMULATION, "VK_LAYER_LUNARG_device_simulation", "VkLayer_device_simulation.json"},
    {LAYER_LUNARG_GFXRECONSTRUCT, "VK_LAYER_LUNARG_gfxreconstruct", "VkLayer_gfxreconstruct.json"},
    {LAYER_LUNARG_MONITOR, "VK_LAYER_LUNARG_monitor", "VkLayer_monitor.json"},
    {LAYER_LUNARG_SCREENSHOT, "VK_LAYER_LUNARG_screenshot", "VkLayer_screenshot.json"}};
static_assert(countof(default_layers) == LAYER_COUNT, "The tranlation table size doesn't match the enum number of elements");

LayerID GetLayerID(const QString& name) {
    assert(!name.isEmpty());

    for (std::size_t i = 0, n = countof(default_layers); i < n; ++i) {
        if (default_layers[i].name == name) return default_layers[i].id;
    }

    return LAYER_THIRD_PARTY;
}

// TODO: add latest, add all layer versions
QString GetBuiltinFolder(const Version& version) {
    if (version <= Version(1, 2, 154))
        return "layers_1_2_154/";
    else
        return "layers_1_2_154/";
    /*
        if (version <= Version(1, 2, 148))
            return "layers_1_2_148/";
        else if (version == Version(1, 2, 154))
            return "layers_1_2_154/";
        else
            return "layers_latest/";
    */
}

// delimted string is a comma delimited string. If value is found remove it
void RemoveString(QString& delimitedString, QString default_value) {
    // Well, it's not there now is it...
    if (!delimitedString.contains(default_value)) return;

    QStringList list = delimitedString.split(",");
    for (int i = 0; i < list.size(); i++)
        if (list[i] == default_value) {
            list.removeAt(i);
            break;
        }

    delimitedString = list.join(",");
}

// Pretty simple, add to list if it's not already in it
void AppendString(QString& delimitedString, QString default_value) {
    // Do I have anything to do?
    if (delimitedString.contains(default_value))  // Nope
        return;

    if (!delimitedString.isEmpty()) delimitedString += ",";

    delimitedString += default_value;
}

Layer::Layer() {}

Layer::Layer(const QString& name, const LayerType layer_type) : name(name), _layer_type(layer_type) {}

Layer::Layer(const QString& name, const LayerType layer_type, const Version& file_format_version, const Version& api_version,
             const QString& implementation_version, const QString& library_path, const QString& type)
    : name(name),
      _layer_type(layer_type),
      _file_format_version(file_format_version),
      _api_version(api_version),
      _implementation_version(implementation_version),
      _library_path(library_path),
      _type(type) {}

// Todo: Load the layer with Vulkan API
bool Layer::IsValid() const {
    return _file_format_version != Version::VERSION_NULL && !name.isEmpty() && !_type.isEmpty() && !_library_path.isEmpty() &&
           _api_version != Version::VERSION_NULL && !_implementation_version.isEmpty();
}

/// Reports errors via a message box. This might be a bad idea?
bool Layer::Load(QString full_path_to_file, LayerType layer_type) {
    _layer_type = layer_type;  // Set layer type, no way to know this from the json file

    // Open the file, should be text. Read it into a
    // temporary string.
    if (full_path_to_file.isEmpty()) return false;

    QFile file(full_path_to_file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
        QMessageBox message_box;
        message_box.setText("Could not open layer file");
        message_box.exec();
        return false;
    }

    QString json_text = file.readAll();
    file.close();

    _layer_path = full_path_to_file;

    // Convert the text to a JSON document & validate it.
    // It does need to be a valid json formatted file.
    QJsonParseError json_parse_error;
    const QJsonDocument& json_document = QJsonDocument::fromJson(json_text.toUtf8(), &json_parse_error);
    if (json_parse_error.error != QJsonParseError::NoError) {
        QMessageBox message_box;
        message_box.setText(json_parse_error.errorString());
        message_box.exec();
        return false;
    }

    // Make sure it's not empty
    if (json_document.isNull() || json_document.isEmpty()) {
        QMessageBox message_box;
        message_box.setText("Json document is empty!");
        message_box.exec();
        return false;
    }

    // Populate key items about the layer
    const QJsonObject& json_root_object = json_document.object();
    _file_format_version = Version(ReadStringValue(json_root_object, "file_format_version").c_str());

    const QJsonObject& json_layer_object = ReadObject(json_root_object, "layer");

    name = ReadStringValue(json_layer_object, "name").c_str();
    _type = ReadStringValue(json_layer_object, "type").c_str();

    const QJsonValue& json_library_path_value = json_layer_object.value("library_path");
    assert((json_library_path_value != QJsonValue::Undefined && name != "VK_LAYER_LUNARG_override") ||
           json_library_path_value == QJsonValue::Undefined && name == "VK_LAYER_LUNARG_override");
    _library_path = json_library_path_value.toString();

    _api_version = Version(ReadStringValue(json_layer_object, "api_version").c_str());
    _implementation_version = ReadStringValue(json_layer_object, "implementation_version").c_str();
    _description = ReadStringValue(json_layer_object, "description").c_str();

    // Load default layer json file if necessary
    const LayerID layer_id = GetLayerID(name);
    const bool is_missing_layer_data =
        json_layer_object.value("settings") == QJsonValue::Undefined || json_layer_object.value("presets") == QJsonValue::Undefined;
    const bool is_builtin_layer_file = full_path_to_file.startsWith(":/resourcefiles/");

    Layer default_layer;
    if (layer_id != LAYER_THIRD_PARTY && is_missing_layer_data && !is_builtin_layer_file) {
        const bool result = default_layer.Load(
            QString(":/resourcefiles/") + GetBuiltinFolder(_api_version) + default_layers[layer_id].file, _layer_type);
        assert(result);
    }

    // Load layer settings
    const QJsonValue& json_settings_value = json_layer_object.value("settings");
    if (json_settings_value != QJsonValue::Undefined) {
        assert(json_settings_value.isArray());
        const QJsonArray& json_array = json_settings_value.toArray();
        for (int i = 0, n = json_array.size(); i < n; ++i) {
            const QJsonObject& json_setting = json_array[i].toObject();

            LayerSetting setting;

            setting.key = ReadStringValue(json_setting, "key").c_str();
            setting.label = ReadStringValue(json_setting, "label").c_str();
            setting.description = ReadStringValue(json_setting, "description").c_str();
            setting.type = GetSettingType(ReadStringValue(json_setting, "type").c_str());
            setting.default_value = ReadString(json_setting, "default").c_str();

            switch (setting.type) {
                case SETTING_EXCLUSIVE_LIST:
                case SETTING_INCLUSIVE_LIST: {
                    // Now we have a list of options, both the enum for the settings file, and the prompts
                    const QJsonValue& json_value_options = json_setting.value("options");
                    assert(json_value_options != QJsonValue::Undefined);

                    const QJsonObject& object = json_value_options.toObject();
                    const QStringList& keys = object.keys();
                    for (int v = 0; v < keys.size(); v++) {
                        QString key = keys[v];
                        const QString default_value = object.value(key).toString();

                        if (setting.type == SETTING_INCLUSIVE_LIST) {
                            setting.enum_values << key;
                            setting.enum_labels << default_value;
                        } else if (setting.type == SETTING_EXCLUSIVE_LIST) {
                            setting.enum_values << key;
                            setting.enum_labels << default_value;
                        } else
                            assert(0);
                    }
                } break;
                case SETTING_SAVE_FILE: {
                    setting.default_value = ValidatePath(setting.default_value.toStdString()).c_str();
                    setting.default_value = ReplacePathBuiltInVariables(setting.default_value.toStdString()).c_str();
                } break;
                case SETTING_LOAD_FILE:
                case SETTING_SAVE_FOLDER:
                case SETTING_BOOL:
                case SETTING_BOOL_NUMERIC:
                case SETTING_VUID_FILTER:
                case SETTING_STRING:
                    break;
                default:
                    assert(0);
                    break;
            }

            settings.push_back(setting);
        }
    } else {
        settings = default_layer.settings;
    }

    // Load layer presets
    const QJsonValue& json_presets_value = json_layer_object.value("presets");
    if (json_presets_value != QJsonValue::Undefined) {
        assert(json_presets_value.isArray());
        const QJsonArray& json_preset_array = json_presets_value.toArray();
        for (int preset_index = 0, preset_count = json_preset_array.size(); preset_index < preset_count; ++preset_index) {
            const QJsonObject& json_preset_object = json_preset_array[preset_index].toObject();

            LayerPreset preset;

            preset.preset_index = ReadIntValue(json_preset_object, "preset-index");
            preset.label = ReadStringValue(json_preset_object, "label");
            preset.description = ReadStringValue(json_preset_object, "description");
            preset.platform_flags = GetPlatformFlags(ReadStringArray(json_preset_object, "platforms"));
            preset.status_type = GetStatusType(ReadStringValue(json_preset_object, "status").c_str());
            preset.editor_state = ReadStringValue(json_preset_object, "editor_state");

            const QJsonArray& json_setting_array = ReadArray(json_preset_object, "settings");
            for (int setting_index = 0, setting_count = json_setting_array.size(); setting_index < setting_count; ++setting_index) {
                const QJsonObject& json_setting_object = json_setting_array[setting_index].toObject();

                SettingStorage setting_value;
                setting_value.key = ReadStringValue(json_setting_object, "key");
                setting_value.value = ReadString(json_setting_object, "value");

                preset.settings.push_back(setting_value);
            }

            presets.push_back(preset);
        }
    } else {
        presets = default_layer.presets;
    }

    return IsValid();  // Not all JSON file are layer JSON valid
}
