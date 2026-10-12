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

#include "drone.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::DroneView>();
}

namespace ui {

// deg*1e7 int -> decimal string, no float (nano-printf has no %f)
std::string drone_coord(int32_t v) {
    if (v == 0) return "?";
    bool neg = v < 0;
    uint32_t a = neg ? (uint32_t)(-(int64_t)v) : (uint32_t)v;
    uint32_t whole = a / 10000000u;
    uint32_t frac = a % 10000000u;
    char b[24];
    snprintf(b, sizeof(b), "%s%lu.%07lu", neg ? "-" : "", (unsigned long)whole, (unsigned long)frac);
    return std::string(b);
}

static const char* src_name(uint8_t s) {
    switch (s) {
        case 1: return "BLE";
        case 2: return "WIFI";
        case 3: return "OUI";
        default: return "?";
    }
}

static std::string id_of(const drone_t& e) {
    char s[21];
    memcpy(s, e.id, 20);
    s[20] = 0;
    if (s[0] == 0) return "(no id)";
    return std::string(s);
}

static std::string mac_str(const uint8_t* m) {
    char b[20];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    return std::string(b);
}

static void to_tchar(const char* s, TCHAR* out, size_t maxlen) {
    size_t i = 0;
    for (; s[i] != 0 && i + 1 < maxlen; i++) out[i] = (TCHAR)(unsigned char)s[i];
    out[i] = 0;
}

// ============================ Detail view ============================
DroneDetailView::DroneDetailView(NavigationView& nav, const drone_t& e)
    : nav_(nav), d_(e) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    menu.add_item({"[ Send bands to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) { save_freqman(); }});

    std::vector<std::string> lines;
    lines.push_back(std::string("Source: ") + src_name(e.source) +
                    (e.source == 3 ? " (maker)" : " Remote ID"));
    lines.push_back("ID: " + id_of(e));
    lines.push_back("MAC: " + mac_str(e.mac));
    lines.push_back("Drone lat: " + drone_coord(e.lat));
    lines.push_back("Drone lon: " + drone_coord(e.lon));
    lines.push_back("Pilot lat: " + drone_coord(e.op_lat));
    lines.push_back("Pilot lon: " + drone_coord(e.op_lon));
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

// Layer-3 bridge: hand the HackRF the common drone bands as a FREQMAN range
// file so you can inspect / jam the control+video links on the PortaPack.
void DroneDetailView::save_freqman() {
    TCHAR dir[16];
    to_tchar("FREQMAN", dir, 16);
    _api->f_mkdir(dir);
    TCHAR path[40];
    to_tchar("FREQMAN/DRONE_BANDS.txt", path, 40);
    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return;
    const char* lines =
        "a=2400000000,b=2483500000,d=Drone 2.4G ctrl/video\r\n"
        "a=5725000000,b=5875000000,d=Drone 5.8G FPV\r\n"
        "a=5170000000,b=5250000000,d=Drone 5.2G video\r\n";
    UINT bw = 0;
    _api->f_write(&fil, lines, strlen(lines), &bw);
    _api->f_close(&fil);
    menu.add_item({"saved DRONE_BANDS", ui::Theme::getInstance()->fg_green->foreground, nullptr, [](KeyEvent) {}});
    set_dirty();
}

// ============================ Main view ============================
DroneView::DroneView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};
}

DroneView::~DroneView() {
    send_app(0);
    Theme::destroy();
}

void DroneView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void DroneView::send_clear() {
    uint16_t cmd = (uint16_t)Command::PPCMD_DRONE_CLEAR;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

bool DroneView::fetch_entry(uint16_t idx, drone_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_DRONE_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

void DroneView::load_list() {
    entries_.clear();
    menu.clear();
    menu.add_item({"[ Clear ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       send_clear();
                       count_ = 0;
                       load_list();
                   }});

    for (uint16_t i = 0; i < count_; i++) {
        drone_t e;
        if (!fetch_entry(i, e)) continue;
        entries_.push_back(e);
        std::string id = id_of(e);
        if (id.size() > 12) id = id.substr(0, 12);
        char row[48];
        snprintf(row, sizeof(row), "%-4s %-12s %d", src_name(e.source), id.c_str(), (int)e.rssi);
        drone_t ec = e;
        menu.add_item({std::string(row), ui::Theme::getInstance()->fg_red->foreground, nullptr,
                       [this, ec](KeyEvent) { nav_.push<DroneDetailView>(ec); }});
    }
    loaded_ = true;
    set_dirty();
}

void DroneView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    uint16_t new_count = buf[0];
    bool count_changed = (new_count != count_);
    count_ = new_count;

    if (count_ == 0)
        text_info.set("Scanning WiFi+BLE...");
    else
        text_info.set("Drones: " + to_string_dec_uint(count_));
    text_info.set_dirty();

    if (!loaded_ || count_changed) load_list();
    _api->set_dirty();
}

void DroneView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != DRONE_APP_ID) send_app(DRONE_APP_ID);
        } else {
            send_app(DRONE_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
