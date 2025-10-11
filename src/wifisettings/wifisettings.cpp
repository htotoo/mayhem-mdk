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

#include "wifisettings.hpp"

#include <memory>
#include <string>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::WifiSettingsView>();
}

namespace ui {

WifiSettingsView::WifiSettingsView(NavigationView& nav) : nav_(nav) {
    add_children({&btn_ssid, &btn_password, &btn_send,
                  &text_ssid, &text_password, &labels});

    btn_ssid.on_select = [this](Button&) {
        text_prompt(nav_, ssid_, 32, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ssid_ = value;
            text_ssid.set("SSID: " + ssid_);
            text_ssid.set_dirty();
        });
    };
    btn_password.on_select = [this](Button&) {
        text_prompt(nav_, password_, 32, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            password_ = value;
            text_password.set("PWD: " + password_);
            text_password.set_dirty();
        });
    };
    btn_send.on_select = [this](Button&) {
        // pp_send_command_set_wifi(ssid_.c_str(), password_.c_str());
    };
}

void WifiSettingsView::focus() {
    btn_ssid.focus();
}

}  // namespace ui