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
#include "ui/ui_menu.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include <vector>

namespace ui {

// Must match AppList::DEAUTH in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t DEAUTH_APP_ID = 7;

// Must match deauth_ap_t in the ESP32 firmware (ep_app_deauth.hpp).
typedef struct __attribute__((packed)) {
    uint8_t bssid[6];
    char ssid[32];
    uint8_t channel;
    int8_t rssi;
} deauth_ap_t;

class DeauthView : public View {
   public:
    DeauthView(NavigationView& nav);
    DeauthView(const DeauthView&) = delete;
    DeauthView& operator=(const DeauthView&) = delete;
    ~DeauthView();

    std::string title() const override { return "Deauth"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_rescan();
    void send_target(uint16_t idx);
    void send_attack(bool on);
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, deauth_ap_t& out);

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t ap_count_ = 0;
    uint8_t attacking_ = 0;
    uint8_t scanning_ = 0;
    bool loaded_ = false;
    bool have_target_ = false;
    std::string target_name_ = "";
    uint8_t target_ch_ = 0;
    std::vector<deauth_ap_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Scanning..."};
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "pick a target AP:"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
