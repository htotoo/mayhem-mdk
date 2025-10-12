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
namespace ui {

// wifi config from companion app
typedef struct wifi_config_comp_t {
    char ssid[30];
    char password[30];
} wifi_config_comp_t;

typedef struct wifi_current_data_t {
    uint8_t ip[4];
    char sta_ssid[30];
    char sta_password[30];
    char ap_ssid[30];
    char ap_password[30];
} wifi_current_data_t;

class WifiSettingsView : public View {
   public:
    WifiSettingsView(NavigationView& nav);
    ~WifiSettingsView() {
        Theme::destroy();
    };

    std::string title() const override { return "WiFi Settings"; };
    void on_framesync() override;

   private:
    void get_current_config();
    uint8_t config_loaded = 0;

    std::string ssid_ = "-";
    std::string password_ = "-";
    std::string ssid_ap_ = "-";
    std::string password_ap_ = "-";
    NavigationView& nav_;
    // sta
    Button btn_ssid{{UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Edit SSID"};
    Button btn_password{{UI_POS_X(15), UI_POS_Y(1), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Edit PWD"};
    Text text_ssid{{UI_POS_X(0), UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "SSID: myssid"};
    Text text_password{{UI_POS_X(0), UI_POS_Y(4), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "PWD: mypass"};
    Button btn_send{{UI_POS_X_CENTER(12), UI_POS_Y(5), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)}, "Set to ESP"};
    // ap
    Button btn_ssid_ap{{UI_POS_X(0), UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Edit SSID"};
    Button btn_password_ap{{UI_POS_X(15), UI_POS_Y(9), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Edit PWD"};
    Text text_ssid_ap{{UI_POS_X(0), UI_POS_Y(11), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "SSID: myssid"};
    Text text_password_ap{{UI_POS_X(0), UI_POS_Y(12), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "PWD: mypass"};
    Button btn_send_ap{{UI_POS_X_CENTER(12), UI_POS_Y(13), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)}, "Set to ESP"};

    Text text_ip{{UI_POS_X(0), UI_POS_Y_BOTTOM(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "IP: 192.168.4.1"};
    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "STA (Wifi client)", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "AP (Wifi AP)", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y_BOTTOM(2)}, "Note: max 30 chars", ui::Theme::getInstance()->fg_yellow->foreground}};
};
}  // namespace ui