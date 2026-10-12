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

// Must match AppList::PMKIDCAPTURE in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t PMKID_APP_ID = 6;

// Must match hs_disp_t in the ESP32 firmware (ep_app_pmkid.hpp).
typedef struct __attribute__((packed)) {
    uint8_t ap[6];
    uint8_t sta[6];
    char ssid[32];
    uint8_t type;  // 1 = PMKID, 2 = EAPOL handshake
    uint8_t channel;
} hs_disp_t;

// Full-screen detail: type, SSID, AP/STA MACs, channel.
class PmkidDetailView : public View {
   public:
    PmkidDetailView(NavigationView& nav, const hs_disp_t& e);
    std::string title() const override { return "Capture"; };
    void focus() override { menu.focus(); };

   private:
    NavigationView& nav_;
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, true};
};

// Main view: lists captured PMKIDs (network + AP), Save to SD as .hc22000.
class PmkidView : public View {
   public:
    PmkidView(NavigationView& nav);
    PmkidView(const PmkidView&) = delete;
    PmkidView& operator=(const PmkidView&) = delete;
    ~PmkidView();

    std::string title() const override { return "PMKID Capture"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);
    void send_capture(bool on);
    void send_clear();
    void send_channel(uint8_t ch);
    void send_deauth();
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, hs_disp_t& out);
    void save_hc22000();

    uint16_t line_len(uint16_t idx);
    int line_chunk(uint16_t idx, uint16_t off, uint8_t* out);  // up to 96 bytes

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t count_ = 0;
    uint8_t capturing_ = 1;
    bool loaded_ = false;
    bool paused_ = false;
    uint8_t chsel_ = 0;  // 0 = hop all channels; 1-13 = locked channel
    std::vector<hs_disp_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Listening..."};
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "t network    ch ap"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
