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
    add_children({&btn_set,
                  &text_ssid});

    btn_set.on_select = [this](Button&) {
        text_prompt(nav_, ssid_, 32, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ssid_ = value;
            text_ssid.set("SSID: " + ssid_);
            text_ssid.set_dirty();
        });
    };
    btn_set.on_touch_press = [this](Button& b) {
        btn_set.set_text("Pressed");
        text_ssid.set("pressed");
        btn_set.set_dirty();
    };
    btn_set.on_touch_release = [this](Button& b) {
        btn_set.set_text("Depressed");
        text_ssid.set("depressed");
        btn_set.set_dirty();
    };
}

void WifiSettingsView::focus() {
    btn_set.focus();
}

}  // namespace ui