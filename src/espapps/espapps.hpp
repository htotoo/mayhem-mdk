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

#pragma once

#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include "ui_textentry.hpp"
#include "app_wifissidspam.hpp"

namespace ui {

class ESPAppsView : public View {
   public:
    ESPAppsView(NavigationView& nav);
    ESPAppsView(const ESPAppsView&) = delete;
    ESPAppsView& operator=(const ESPAppsView&) = delete;
    ESPAppsView(ESPAppsView&&) = delete;
    ESPAppsView& operator=(ESPAppsView&&) = delete;
    ~ESPAppsView() {
        if (currappview != nullptr) {
            delete currappview;
            currappview = nullptr;
        }
        Theme::destroy();
    };

    std::string title() const override { return "ESP Apps"; };
    void on_framesync() override;

   private:
    uint16_t current_app = 0;
    uint8_t config_loaded = 0;

    void get_current_app();
    void update_ui_for_current_app();

    NavigationView& nav_;

    OptionsField options_apps{
        {UI_POS_X(0), UI_POS_Y(0)},
        20,
        {{"Wifi SSID spam", 1}}};

    Button button_startstop{{UI_POS_X(22), UI_POS_Y(0), UI_POS_WIDTH(7), UI_POS_HEIGHT(1.5)}, "Start"};

    View* currappview = nullptr;
};
}  // namespace ui