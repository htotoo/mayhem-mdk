/*
 * Copyright (C) 2025 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "espmanager.hpp"

#include <memory>
#include <string>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::ESPManagerView>();
}

namespace ui {

ESPManagerView::ESPManagerView(NavigationView& nav) : nav_(nav) {
    add_children({&btn_airplane_on,
                  &btn_airplane_off,
                  &labels});

    btn_airplane_on.on_select = [this](Button&) {
        uint8_t mode = 2;
        Command cmd = Command::PPCMD_AIRPLANE_MODE;
        std::vector<uint8_t> data(sizeof(uint8_t));
        memcpy(data.data(), &mode, sizeof(mode));
        data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
        if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
    };
    btn_airplane_off.on_select = [this](Button&) {
        uint8_t mode = 1;
        Command cmd = Command::PPCMD_AIRPLANE_MODE;
        std::vector<uint8_t> data(sizeof(uint8_t));
        memcpy(data.data(), &mode, sizeof(mode));
        data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
        if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
    };
}

void ESPManagerView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        get_current_config();
    }
}

void ESPManagerView::get_current_config() {
    Command cmd = Command::PPCMD_AIRPLANE_MODE;
    uint8_t mode = 0;

    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&mode, sizeof(mode)) == false) return;
    if (mode == 2) {
        btn_airplane_on.focus();
    } else {
        btn_airplane_off.focus();
    }
}

}  // namespace ui