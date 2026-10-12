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

// Must match AppList::DRONE in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t DRONE_APP_ID = 10;

// Must match drone_t in the ESP32 firmware (ep_app_drone.hpp).
typedef struct __attribute__((packed)) {
    uint8_t source;  // 1=BLE-RID, 2=WiFi-RID, 3=WiFi-OUI
    char id[20];
    int32_t lat;
    int32_t lon;
    int32_t op_lat;
    int32_t op_lon;
    int8_t rssi;
    uint8_t mac[6];
} drone_t;

std::string drone_coord(int32_t v);  // deg*1e7 -> "52.0917400" (no float)

class DroneDetailView : public View {
   public:
    DroneDetailView(NavigationView& nav, const drone_t& e);
    std::string title() const override { return "Drone"; };
    void focus() override { menu.focus(); };

   private:
    void save_freqman();
    NavigationView& nav_;
    drone_t d_;
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, true};
};

class DroneView : public View {
   public:
    DroneView(NavigationView& nav);
    DroneView(const DroneView&) = delete;
    DroneView& operator=(const DroneView&) = delete;
    ~DroneView();

    std::string title() const override { return "Drone Detect"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_clear();
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, drone_t& out);

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t count_ = 0;
    bool loaded_ = false;
    std::vector<drone_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Scanning WiFi+BLE..."};
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "src  id            dBm"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
