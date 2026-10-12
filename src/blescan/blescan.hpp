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
#include "ui/ui_painter.hpp"
#include "ui/ui_font_fixed_5x8.hpp"
#include "ui/ui_font_fixed_8x16.hpp"
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include <vector>

namespace ui {

// Must match AppList::BLESCAN in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t BLESCAN_APP_ID = 8;

// Must match ble_dev_t in the ESP32 firmware (ep_app_blescan.hpp).
typedef struct __attribute__((packed)) {
    uint8_t addr[6];
    uint8_t addr_type;
    int8_t rssi;
    char name[24];
    uint16_t company;
    uint8_t kind;  // 0=generic,1=AirTag,2=Tile,3=SmartTag,4=FastPair,5=MSSwift
} ble_dev_t;

std::string ble_vendor(uint16_t company);  // company id -> short vendor name
std::string ble_kind_name(uint8_t kind);
bool ble_is_tracker(uint8_t kind);

class BleDetailView : public View {
   public:
    BleDetailView(NavigationView& nav, const ble_dev_t& e, uint16_t index);
    std::string title() const override { return "BLE dev"; };
    void focus() override { menu.focus(); };

   private:
    NavigationView& nav_;
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, true};
};

// Live RSSI "warm/cold" meter to physically locate one device.
class BleFollowView : public View {
   public:
    BleFollowView(NavigationView& nav, uint16_t index, ble_dev_t dev);
    std::string title() const override { return "Find"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void draw_meter(int rssi, bool have);
    NavigationView& nav_;
    uint16_t index_;
    ble_dev_t dev_;
    ui::Painter painter{};
    uint8_t poll_ = 0;
    int last_rssi_ = -127;

    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(2)}, true};
};

class BleScanView : public View {
   public:
    BleScanView(NavigationView& nav);
    BleScanView(const BleScanView&) = delete;
    BleScanView& operator=(const BleScanView&) = delete;
    ~BleScanView();

    std::string title() const override { return "BLE Scanner"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_scan(bool on);
    void send_clear();
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, ble_dev_t& out);
    void save_csv();

    NavigationView& nav_;
    bool trackers_only_ = false;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t count_ = 0;
    uint8_t scanning_ = 1;
    bool loaded_ = false;
    bool paused_ = false;
    std::vector<ble_dev_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Scanning BLE..."};
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "name         vendor  dBm"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
