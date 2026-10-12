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

#include "wifianalyzer.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::WifiAnalyzerView>();
}

namespace ui {

// FatFs on the PortaPack uses 16-bit TCHAR paths (LFN Unicode); widen ASCII.
static void to_tchar(const char* s, TCHAR* out, size_t maxlen) {
    size_t i = 0;
    for (; s[i] != 0 && i + 1 < maxlen; i++) out[i] = (TCHAR)(unsigned char)s[i];
    out[i] = 0;
}

static std::string auth_name(uint8_t a) {
    switch (a) {
        case 0: return "Open";
        case 1: return "WEP";
        case 2: return "WPA";
        case 3: return "WPA2";
        case 4: return "WPA/WPA2";
        case 5: return "WPA2-Ent";
        case 6: return "WPA3";
        case 7: return "WPA2/WPA3";
        case 8: return "WAPI";
        default: return "?";
    }
}

static std::string mac_str(const uint8_t* m) {
    char b[20];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    return std::string(b);
}

static std::string ssid_str(const wifi_ap_entry_t& ap) {
    char s[33];
    memcpy(s, ap.ssid, 32);
    s[32] = 0;
    if (s[0] == 0) return "<hidden>";
    return std::string(s);
}

// ============================ Channel graph ============================
ChannelGraphView::ChannelGraphView(NavigationView& nav, std::array<uint16_t, 15> counts)
    : nav_(nav), counts_(counts) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
}

void ChannelGraphView::on_framesync() {
    // let the framework paint the background first, then draw once
    if (drawn_) return;
    if (settle_ < 2) {
        settle_++;
        return;
    }
    draw();
    drawn_ = true;
}

void ChannelGraphView::draw() {
    const ui::Font& font = ui::font::fixed_5x8();
    ui::Color bg = ui::Color::black();
    ui::Color axis = ui::Color(90, 90, 90);
    ui::Color barN = ui::Color(0, 190, 235);    // normal channels
    ui::Color barKey = ui::Color(90, 220, 120);  // 1 / 6 / 11 (non-overlapping)

    // draw below the Back menu (which occupies the top two rows)
    painter.fill_rectangle({0, 52, 240, 268}, bg);
    painter.draw_string({6, 56}, font, ui::Color::white(), bg, "APs per 2.4GHz channel");

    uint16_t maxc = 1;
    for (int c = 1; c <= 13; c++)
        if (counts_[c] > maxc) maxc = counts_[c];

    const int base_y = 288;   // bottom of the bars
    const int top_y = 78;     // top of the plot
    const int maxh = base_y - top_y;
    painter.fill_rectangle({8, base_y, 224, 1}, axis);  // baseline

    for (int c = 1; c <= 13; c++) {
        int x = 10 + (c - 1) * 17;
        int h = (counts_[c] * maxh) / maxc;
        if (counts_[c] > 0 && h < 2) h = 2;
        bool key = (c == 1 || c == 6 || c == 11);
        if (h > 0)
            painter.fill_rectangle({(ui::Coord)x, (ui::Coord)(base_y - h), 14, (ui::Coord)h}, key ? barKey : barN);
        // channel number under the axis
        std::string cs = std::to_string(c);
        painter.draw_string({(ui::Coord)(x + (c < 10 ? 4 : 2)), base_y + 4}, font, ui::Color::white(), bg, cs);
        // count above the bar
        if (counts_[c] > 0)
            painter.draw_string({(ui::Coord)(x + 4), (ui::Coord)(base_y - h - 9)}, font, ui::Color::white(), bg, std::to_string(counts_[c]));
    }
    painter.draw_string({6, 302}, font, axis, bg, "green = ch 1/6/11");
}

// ============================ Detail view ============================
WifiAnalyzerDetailView::WifiAnalyzerDetailView(NavigationView& nav, const wifi_ap_entry_t& ap)
    : nav_(nav), ap_(ap) {
    add_children({&text_status, &menu});
    text_status.set(ssid_str(ap));
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    menu.add_item({"[ Send to Jammer ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) { send_to_jammer(); }});

    int freq = wifi_chan_freq(ap.channel);
    std::string width = (ap.second == 0) ? "20 MHz" : (ap.second == 1) ? "40 MHz (+)" : "40 MHz (-)";
    int lo, hi;
    wifi_jam_range(ap, lo, hi);

    std::vector<std::string> lines;
    lines.push_back("BSSID: " + mac_str(ap.bssid));
    lines.push_back("Channel: " + std::to_string(ap.channel));
    lines.push_back("Freq: " + std::to_string(freq) + " MHz");
    lines.push_back("Width: " + width);
    lines.push_back("Jam: " + std::to_string(lo) + "-" + std::to_string(hi) + " MHz");
    lines.push_back("Auth: " + auth_name(ap.authmode));
    lines.push_back("RSSI: " + std::to_string((int)ap.rssi) + " dBm");

    const size_t w = 28;
    for (auto& ln : lines) {
        std::string l = ln;
        do {
            menu.add_item({l.substr(0, w), ui::Color::white(), nullptr, [](KeyEvent) {}});
            l = l.size() > w ? l.substr(w) : std::string();
        } while (!l.empty());
    }
}

// Write the AP's jam range as a single-entry FREQMAN file the Mayhem Jammer can
// load: Jammer -> Range -> Load range -> JAMMER_ANALYZER -> START.
void WifiAnalyzerDetailView::send_to_jammer() {
    int lo, hi;
    wifi_jam_range(ap_, lo, hi);
    // Hz fits a 32-bit unsigned long (<4.29e9); newlib-nano printf has no %lld,
    // so use %lu below, NOT %lld (that was writing garbage like "a=1d").
    unsigned long a = (unsigned long)lo * 1000000UL;
    unsigned long b = (unsigned long)hi * 1000000UL;

    TCHAR dir[16];
    to_tchar("FREQMAN", dir, 16);
    _api->f_mkdir(dir);

    TCHAR path[40];
    to_tchar("FREQMAN/JAMMER_ANALYZER.txt", path, 40);
    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        text_status.set("SD write failed");
        text_status.set_dirty();
        return;
    }
    std::string name = ssid_str(ap_);
    if (name.size() > 16) name = name.substr(0, 16);
    char line[96];
    snprintf(line, sizeof(line), "a=%lu,b=%lu,d=%s ch%d\r\n", a, b, name.c_str(), ap_.channel);
    UINT bw = 0;
    _api->f_write(&fil, line, strlen(line), &bw);
    _api->f_close(&fil);

    text_status.set("Sent -> JAMMER_ANALYZER");
    text_status.set_dirty();
}

// ============================ Main view ============================
WifiAnalyzerView::WifiAnalyzerView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};  // physical back exits the whole app
}

WifiAnalyzerView::~WifiAnalyzerView() {
    send_app(0);  // stop the on-system scanner when leaving the app
    Theme::destroy();
}

void WifiAnalyzerView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void WifiAnalyzerView::send_auto(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_WIFIANALYZER_SET_AUTO;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

bool WifiAnalyzerView::fetch_entry(uint16_t idx, wifi_ap_entry_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_WIFIANALYZER_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

void WifiAnalyzerView::load_list() {
    entries_.clear();
    menu.clear();
    menu.add_item({"[ Ch graph ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       std::array<uint16_t, 15> counts{};
                       for (auto& e : entries_)
                           if (e.channel >= 1 && e.channel <= 14) counts[e.channel]++;
                       nav_.push<ChannelGraphView>(counts);
                   }});
    // Single toggle: pause freezes the list (for scrolling); resume continues
    // auto-scan and kicks an immediate scan for instant fresh data.
    menu.add_item({paused_ ? "[ Resume scan ]" : "[ Pause scan ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) {
                       paused_ = !paused_;
                       send_auto(!paused_);  // stop/start scanning on the ESP
                       if (!paused_) {        // resume -> scan now
                           uint16_t cmd = (uint16_t)Command::PPCMD_WIFIANALYZER_RESCAN;
                           _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
                           text_info.set("Scanning...");
                           text_info.set_dirty();
                       }
                       load_list();  // rebuild with the new label; freezes when paused
                   }});

    for (uint16_t i = 0; i < ap_count_; i++) {
        wifi_ap_entry_t ap;
        if (!fetch_entry(i, ap)) continue;
        entries_.push_back(ap);
        std::string name = ssid_str(ap);
        if (name.size() > 10) name = name.substr(0, 10);
        int freq = wifi_chan_freq(ap.channel);
        char row[40];
        // SSID  <ch>  <freq>  <rssi>   e.g. "MyNet      6  2437 -42"
        snprintf(row, sizeof(row), "%-10s %-2d %4d %d", name.c_str(), ap.channel, freq, (int)ap.rssi);
        wifi_ap_entry_t apc = ap;
        menu.add_item({std::string(row), ui::Color::white(), nullptr,
                       [this, apc](KeyEvent) { nav_.push<WifiAnalyzerDetailView>(apc); }});
    }
    loaded_ = true;
    set_dirty();
}

void WifiAnalyzerView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    uint16_t new_count = buf[0];
    uint8_t new_scan = (uint8_t)buf[1];

    bool scan_finished = (scanning_ == 1 && new_scan == 0);
    bool count_changed = (new_count != ap_count_);
    ap_count_ = new_count;
    scanning_ = new_scan;

    text_info.set("Networks: " + to_string_dec_uint(ap_count_) +
                  (scanning_ ? " (scan)" : paused_ ? " (paused)" : ""));
    text_info.set_dirty();

    // While paused the list is frozen for scrolling; only reload when running.
    if (!paused_ && (!loaded_ || scan_finished || count_changed)) load_list();
    _api->set_dirty();
}

void WifiAnalyzerView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != WIFIANALYZER_APP_ID) send_app(WIFIANALYZER_APP_ID);
        } else {
            send_app(WIFIANALYZER_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
