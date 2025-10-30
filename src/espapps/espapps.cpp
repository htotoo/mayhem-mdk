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

#include "espapps.hpp"
#include <memory>
#include <string>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::ESPAppsView>();
}

namespace ui {

ESPAppsView::ESPAppsView(NavigationView& nav) : nav_(nav) {
    add_children({&options_apps, &button_startstop});

    button_startstop.on_select = [this](Button&) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t appcmd = 0;
        if (current_app == 0) {  // start app
            appcmd = options_apps.selected_index_value();
        } else {  // stop app
            appcmd = 0;
        }
        current_app = appcmd;
        std::vector<uint16_t> data;
        data.push_back(static_cast<uint16_t>(cmd));
        data.push_back(static_cast<uint16_t>(appcmd));
        if (_api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0) == false) return;
        update_ui_for_current_app();
    };
}

void ESPAppsView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        get_current_app();
    }
}

void ESPAppsView::get_current_app() {
    Command cmd = Command::PPCMD_APPMGR_APPMGR;
    uint16_t capp = 0;

    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2) == false) return;

    current_app = capp;
    update_ui_for_current_app();
}

void ESPAppsView::update_ui_for_current_app() {
    options_apps.set_by_value(current_app);
    button_startstop.set_text(current_app == 0 ? "Start" : "Stop");
    if (currappview) {
        remove_child(currappview);
        delete currappview;
        currappview = nullptr;
        set_dirty();
    }
    if (current_app == 1) {  // Wifi SSID spam
        currappview = new AppWifiSSIDSpam();
    }

    // add other apps here

    if (currappview) {
        add_child(currappview);
        currappview->set_parent_rect({0, UI_POS_HEIGHT(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)});
    }
}

}  // namespace ui