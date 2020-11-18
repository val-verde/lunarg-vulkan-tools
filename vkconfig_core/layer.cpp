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

#include <QFile>
#include <QMessageBox>
#include <QJsonArray>

#include <cassert>

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
    const QJsonValue& json_file_format_value = json_root_object.value("file_format_version");
    assert(json_file_format_value != QJsonValue::Undefined);
    _file_format_version = Version(json_file_format_value.toString());

    const QJsonValue& json_layer_value = json_root_object.value("layer");
    assert(json_layer_value != QJsonValue::Undefined);
    const QJsonObject& json_layer_object = json_layer_value.toObject();

    const QJsonValue& json_name_value = json_layer_object.value("name");
    assert(json_name_value != QJsonValue::Undefined);
    name = json_name_value.toString();

    const QJsonValue& json_type_value = json_layer_object.value("type");
    assert(json_type_value != QJsonValue::Undefined);
    _type = json_type_value.toString();

    const QJsonValue& json_library_path_value = json_layer_object.value("library_path");
    assert((json_library_path_value != QJsonValue::Undefined && name != "VK_LAYER_LUNARG_override") ||
           json_library_path_value == QJsonValue::Undefined && name == "VK_LAYER_LUNARG_override");
    _library_path = json_library_path_value.toString();

    const QJsonValue& json_api_version_value = json_layer_object.value("api_version");
    assert(json_api_version_value != QJsonValue::Undefined);
    _api_version = Version(json_api_version_value.toString());

    const QJsonValue& json_implementation_version_value = json_layer_object.value("implementation_version");
    assert(json_implementation_version_value != QJsonValue::Undefined);
    _implementation_version = json_implementation_version_value.toString();

    const QJsonValue& json_description_value = json_layer_object.value("description");
    assert(json_description_value != QJsonValue::Undefined);
    _description = json_description_value.toString();

    const QJsonValue& json_settings_value = json_layer_object.value("settings");
    VKC_ASSERT_VERSION(SUPPORT_VKCONFIG_2_0_2 && json_settings_value != QJsonValue::Undefined, Version("2.0.3"),
                       _file_format_version);
    if (json_settings_value != QJsonValue::Undefined) {
        assert(json_settings_value.isArray());
        const QJsonArray& json_array = json_settings_value.toArray();
        for (int i = 0, n = json_array.size(); i < n; ++i) {
            const QJsonObject& json_setting = json_array[i].toObject();

            LayerSetting setting;

            const QJsonValue& json_value_key = json_setting.value("key");
            assert(json_value_key != QJsonValue::Undefined);
            setting.key = json_value_key.toString();

            const QJsonValue& json_value_name = json_setting.value("name");
            assert(json_value_name != QJsonValue::Undefined);
            setting.label = json_value_name.toString();

            const QJsonValue& json_value_description = json_setting.value("description");
            assert(json_value_description != QJsonValue::Undefined);
            setting.description = json_value_description.toString();

            const QJsonValue& json_value_type = json_setting.value("type");
            assert(json_value_type != QJsonValue::Undefined);
            setting.type = GetSettingType(json_value_type.toString().toUtf8().constData());

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

            const QJsonValue& json_value_default = json_setting.value("default");
            if (json_value_default.isArray()) {
                const QJsonArray& array = json_value_default.toArray();
                for (int a = 0; a < array.size(); a++) {
                    setting.default_value += array[a].toString();
                    if (a != array.size() - 1) setting.default_value += ",";
                }

            } else
                setting.default_value = json_value_default.toString();

            settings.push_back(setting);
        }
    }

    return IsValid();  // Not all JSON file are layer JSON valid
}
