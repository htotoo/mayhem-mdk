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
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include <vector>
#include <array>

namespace ui {

// Must match AppList::WIFIANALYZER in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t WIFIANALYZER_APP_ID = 5;

// Must match wifi_ap_entry_t in the ESP32 firmware (ep_app_wifianalyzer.hpp).
typedef struct __attribute__((packed)) {
    uint8_t bssid[6];
    char ssid[32];
    uint8_t channel;
    int8_t rssi;
    uint8_t second;
    uint8_t authmode;
} wifi_ap_entry_t;

// 2.4 GHz channel -> center frequency (MHz).
static inline int wifi_chan_freq(int ch) {
    if (ch == 14) return 2484;
    return 2412 + (ch - 1) * 5;
}

// 2.4 GHz channel -> suggested jam range (MHz), matching Mayhem's ~22 MHz WiFi
// channel width (center +-11), widened for 40 MHz bonding.
static inline void wifi_jam_range(const wifi_ap_entry_t& ap, int& lo, int& hi) {
    int f = wifi_chan_freq(ap.channel);
    lo = f - 11;
    hi = f + 11;
    if (ap.second == 1) hi += 20;  // HT40+ (secondary above)
    if (ap.second == 2) lo -= 20;  // HT40- (secondary below)
}

// Full-screen detail for one network: SSID, BSSID, exact frequency + jam range,
// plus a "Send to Jammer" action that writes a loadable FREQMAN range file.
class WifiAnalyzerDetailView : public View {
   public:
    WifiAnalyzerDetailView(NavigationView& nav, const wifi_ap_entry_t& ap);
    std::string title() const override { return "AP"; };
    void focus() override { menu.focus(); };

   private:
    void send_to_jammer();

    NavigationView& nav_;
    wifi_ap_entry_t ap_;

    Text text_status{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};
    MenuView menu{{0, UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(2)}, true};
};

// Bar graph of how many APs are on each 2.4 GHz channel (1-13). Drawn directly
// with a Painter; channels 1/6/11 (non-overlapping) are highlighted.
class ChannelGraphView : public View {
   public:
    ChannelGraphView(NavigationView& nav, std::array<uint16_t, 15> counts);
    std::string title() const override { return "Ch usage"; };
    void on_framesync() override;
    void focus() override { menu.focus(); }

   private:
    void draw();
    NavigationView& nav_;
    std::array<uint16_t, 15> counts_;
    ui::Painter painter{};
    uint8_t settle_ = 0;
    bool drawn_ = false;

    // a focusable menu with just a Back item gives reliable back navigation
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(2)}, true};
};

// Main view: scans on the ESP and lists the found 2.4 GHz networks.
class WifiAnalyzerView : public View {
   public:
    WifiAnalyzerView(NavigationView& nav);
    WifiAnalyzerView(const WifiAnalyzerView&) = delete;
    WifiAnalyzerView& operator=(const WifiAnalyzerView&) = delete;
    ~WifiAnalyzerView();

    std::string title() const override { return "WiFi Analyzer"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void send_app(uint16_t appid);  // start (id) / stop (0) the on-system app
    void send_auto(bool on);        // enable/disable auto rescan on the ESP
    void get_status();
    void load_list();
    bool fetch_entry(uint16_t idx, wifi_ap_entry_t& out);

    NavigationView& nav_;

    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t ap_count_ = 0;
    uint8_t scanning_ = 0;
    bool loaded_ = false;
    bool paused_ = false;  // freeze the list (stop auto-reload) for scrolling
    std::vector<wifi_ap_entry_t> entries_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Scanning..."};
    // column legend for the list rows below (fixed-width font, so it aligns)
    Text text_header{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "SSID       ch MHz  dBm"};
    MenuView menu{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(3)}, true};
};

}  // namespace ui
