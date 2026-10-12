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

// Must match AppList::BLESPAM in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t BLESPAM_APP_ID = 9;

// File picker for BLUETOOTH/BLESPAM/*.txt custom payload sets.
class BleSpamFilesView : public View {
   public:
    BleSpamFilesView(NavigationView& nav);
    std::string title() const override { return "BLESPAM SD"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void load_dir();
    void upload_file(const std::string& name);

    NavigationView& nav_;
    bool loaded_ = false;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "BLUETOOTH/BLESPAM/"};
    MenuView menu{{0, UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(2)}, true};
};

class BleSpamView : public View {
   public:
    BleSpamView(NavigationView& nav);
    BleSpamView(const BleSpamView&) = delete;
    BleSpamView& operator=(const BleSpamView&) = delete;
    ~BleSpamView();

    std::string title() const override { return "BLE Spam"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_mode(uint8_t m);
    void send_active(bool on);
    void get_status();
    void build_menu();

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint8_t mode_ = 0;  // 0=Apple,1=FastPair,2=SwiftPair,3=All
    bool active_ = false;
    uint16_t sent_ = 0;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Idle"};
    Labels warn{{{UI_POS_X(0), UI_POS_Y(2)}, "Authorized demo only.", ui::Color(128, 128, 128)}};
    MenuView menu{{0, UI_POS_Y(4), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(5)}, true};
};

}  // namespace ui
