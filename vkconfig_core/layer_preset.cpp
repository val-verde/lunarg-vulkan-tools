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
 * - Christophe Riccio <christophe@lunarg.com>
 */

#include "layer_preset.h"

LayerSettingValue* FindSetting(LayerPreset& preset, const char* key) {
    for (std::size_t i = 0, n = preset.settings.size(); i < n; ++i) {
        if (preset.settings[i].key == key) {
            return &preset.settings[i];
        }
    }

    return nullptr;
}

const LayerPreset* GetPreset(const std::vector<LayerPreset>& presets, int preset_index) {
    for (std::size_t i = 0, n = presets.size(); i < n; ++i) {
        if (presets[i].preset_index == preset_index) {
            return &presets[i];
        }
    }

    return nullptr;
}
