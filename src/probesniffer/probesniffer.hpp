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

// Must match AppList::WIFIPROBESNIFFER in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t PROBESNIFFER_APP_ID = 3;

// Must match probe_entry_t in the ESP32 firmware (ep_app_probesniffer.hpp).
typedef struct __attribute__((packed)) {
    uint8_t mac[6];
    char ssid[32];
    int8_t rssi;
    uint8_t channel;
} probe_entry_t;

// Full-screen detail: the device MAC and the network it is looking for.
class ProbeSnifferDetailView : public View {
   public:
    ProbeSnifferDetailView(NavigationView& nav, const probe_entry_t& e);
    std::string title() const override { return "Probe"; };
    void focus() override { menu.focus(); };

   private:
    NavigationView& nav_;
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, true};
};

// Main view: lists captured probe requests (device + requested SSID + RSSI).
class ProbeSnifferView : public View {
   public:
    ProbeSnifferView(NavigationView& nav);
    ProbeSnifferView(const ProbeSnifferView&) = delete;
    ProbeSnifferView& operator=(const ProbeSnifferView&) = delete;
    ~ProbeSnifferView();

    std::string title() const override { return "Probe Sniffer"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_capture(bool on);
    void send_clear();
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, probe_entry_t& out);
    void save_csv();  // dump the whole list to ProbeSniffer/probes.csv

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t count_ = 0;
    uint8_t capturing_ = 1;
    bool loaded_ = false;
    bool paused_ = false;
    std::vector<probe_entry_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Listening..."};
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "dev  looking-for      dBm"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
