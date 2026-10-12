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

#include "probesniffer.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::ProbeSnifferView>();
}

namespace ui {

// FatFs on the PortaPack uses 16-bit TCHAR paths (LFN Unicode); widen ASCII.
static void to_tchar(const char* s, TCHAR* out, size_t maxlen) {
    size_t i = 0;
    for (; s[i] != 0 && i + 1 < maxlen; i++) out[i] = (TCHAR)(unsigned char)s[i];
    out[i] = 0;
}

static std::string mac_str(const uint8_t* m) {
    char b[20];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    return std::string(b);
}

static std::string ssid_str(const probe_entry_t& e) {
    char s[33];
    memcpy(s, e.ssid, 32);
    s[32] = 0;
    if (s[0] == 0) return "<broadcast>";
    return std::string(s);
}

// RFC-4180 CSV field: quote if it contains a comma/quote/newline, doubling quotes.
static std::string csv_field(const std::string& s) {
    bool needs = false;
    for (char c : s)
        if (c == ',' || c == '"' || c == '\n' || c == '\r') { needs = true; break; }
    if (!needs) return s;
    std::string o = "\"";
    for (char c : s) {
        if (c == '"') o += '"';
        o += c;
    }
    o += "\"";
    return o;
}

// ============================ Detail view ============================
ProbeSnifferDetailView::ProbeSnifferDetailView(NavigationView& nav, const probe_entry_t& e)
    : nav_(nav) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});

    std::vector<std::string> lines;
    lines.push_back("Device: " + mac_str(e.mac));
    lines.push_back("Looking for:");
    lines.push_back("  " + ssid_str(e));
    lines.push_back("Channel: " + std::to_string(e.channel));
    lines.push_back("RSSI: " + std::to_string((int)e.rssi) + " dBm");

    const size_t w = 28;
    for (auto& ln : lines) {
        std::string l = ln;
        do {
            menu.add_item({l.substr(0, w), ui::Color::white(), nullptr, [](KeyEvent) {}});
            l = l.size() > w ? l.substr(w) : std::string();
        } while (!l.empty());
    }
}

// ============================ Main view ============================
ProbeSnifferView::ProbeSnifferView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};  // physical back exits the whole app
}

ProbeSnifferView::~ProbeSnifferView() {
    send_app(0);  // stop the on-system sniffer when leaving the app
    Theme::destroy();
}

void ProbeSnifferView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void ProbeSnifferView::send_capture(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_PROBESNIFFER_SET_AUTO;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void ProbeSnifferView::send_clear() {
    uint16_t cmd = (uint16_t)Command::PPCMD_PROBESNIFFER_CLEAR;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

bool ProbeSnifferView::fetch_entry(uint16_t idx, probe_entry_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_PROBESNIFFER_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

void ProbeSnifferView::load_list() {
    entries_.clear();
    menu.clear();
    menu.add_item({paused_ ? "[ Resume ]" : "[ Pause ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) {
                       paused_ = !paused_;
                       send_capture(!paused_);
                       load_list();
                   }});
    menu.add_item({"[ Clear ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       send_clear();
                       count_ = 0;
                       load_list();
                   }});
    menu.add_item({"[ Save to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       save_csv();
                   }});

    for (uint16_t i = 0; i < count_; i++) {
        probe_entry_t e;
        if (!fetch_entry(i, e)) continue;
        entries_.push_back(e);
        std::string name = ssid_str(e);
        if (name.size() > 12) name = name.substr(0, 12);
        char row[40];
        // last 2 MAC bytes as a short device tag, requested SSID, RSSI
        snprintf(row, sizeof(row), "%02X%02X %-12s %d", e.mac[4], e.mac[5], name.c_str(), (int)e.rssi);
        probe_entry_t ec = e;
        menu.add_item({std::string(row), ui::Color::white(), nullptr,
                       [this, ec](KeyEvent) { nav_.push<ProbeSnifferDetailView>(ec); }});
    }
    loaded_ = true;
    set_dirty();
}

// Dump the whole captured list to ProbeSniffer/probes.csv (mac,ssid,rssi,channel).
void ProbeSnifferView::save_csv() {
    TCHAR dir[24];
    to_tchar("WIFI", dir, 24);  // f_mkdir makes one level at a time
    _api->f_mkdir(dir);
    to_tchar("WIFI/ProbeSniffer", dir, 24);
    _api->f_mkdir(dir);

    TCHAR path[40];
    to_tchar("WIFI/ProbeSniffer/probes.csv", path, 40);
    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        text_info.set("SD save failed");
        text_info.set_dirty();
        return;
    }
    UINT bw = 0;
    const char* hdr = "mac,ssid,rssi,channel\r\n";
    _api->f_write(&fil, hdr, strlen(hdr), &bw);
    for (auto& e : entries_) {
        char line[160];  // ssid may grow when CSV-quoted/escaped
        // rssi/channel are small ints; nano-printf handles %d/%s fine (no %lld).
        snprintf(line, sizeof(line), "%s,%s,%d,%d\r\n",
                 mac_str(e.mac).c_str(), csv_field(ssid_str(e)).c_str(), (int)e.rssi, e.channel);
        _api->f_write(&fil, line, strlen(line), &bw);
    }
    _api->f_close(&fil);
    text_info.set("Saved probes.csv");
    text_info.set_dirty();
}

void ProbeSnifferView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    uint16_t new_count = buf[0];
    capturing_ = (uint8_t)buf[1];

    bool count_changed = (new_count != count_);
    count_ = new_count;

    text_info.set("Devices: " + to_string_dec_uint(count_) + (paused_ ? " (paused)" : ""));
    text_info.set_dirty();

    // While paused the list is frozen for scrolling; only reload when capturing.
    if (!paused_ && (!loaded_ || count_changed)) load_list();
    _api->set_dirty();
}

void ProbeSnifferView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != PROBESNIFFER_APP_ID) send_app(PROBESNIFFER_APP_ID);
        } else {
            send_app(PROBESNIFFER_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
