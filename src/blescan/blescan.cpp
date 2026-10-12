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

#include "blescan.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::BleScanView>();
}

namespace ui {

// A few common Bluetooth SIG company identifiers.
std::string ble_vendor(uint16_t company) {
    switch (company) {
        case 0x004C: return "Apple";
        case 0x0006: return "Microsoft";
        case 0x0075: return "Samsung";
        case 0x00E0: return "Google";
        case 0x0087: return "Garmin";
        case 0x0157: return "Huawei";
        case 0x038F: return "Xiaomi";
        case 0x0059: return "Nordic";
        case 0x02E5: return "Espressif";
        case 0x0499: return "Ruuvi";
        case 0x0171: return "Amazon";
        case 0x00D2: return "Logitech";
        case 0x0000: return "-";
        default: return "?";
    }
}

std::string ble_kind_name(uint8_t kind) {
    switch (kind) {
        case 1: return "AirTag";
        case 2: return "Tile";
        case 3: return "SmartTag";
        case 4: return "FastPair";
        case 5: return "MSSwift";
        default: return "";
    }
}
bool ble_is_tracker(uint8_t kind) { return kind == 1 || kind == 2 || kind == 3; }

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

static std::string name_of(const ble_dev_t& e) {
    char s[25];
    memcpy(s, e.name, 24);
    s[24] = 0;
    if (s[0] == 0) return "(no name)";
    return std::string(s);
}

// ============================ Detail view ============================
BleDetailView::BleDetailView(NavigationView& nav, const ble_dev_t& e, uint16_t index)
    : nav_(nav) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    ble_dev_t ec = e;
    menu.add_item({"[ Track (find it) ]", ui::Theme::getInstance()->fg_green->foreground, nullptr,
                   [this, ec, index](KeyEvent) { nav_.push<BleFollowView>(index, ec); }});

    std::vector<std::string> lines;
    lines.push_back("Name: " + name_of(e));
    lines.push_back("MAC: " + mac_str(e.addr));
    lines.push_back(std::string("Addr: ") + (e.addr_type == 0 ? "public" : "random"));
    lines.push_back("Vendor: " + ble_vendor(e.company));
    if (e.kind) lines.push_back("Type: " + ble_kind_name(e.kind) + (ble_is_tracker(e.kind) ? " (TRACKER)" : ""));
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

// ============================ Follow (find) view ============================
BleFollowView::BleFollowView(NavigationView& nav, uint16_t index, ble_dev_t dev)
    : nav_(nav), index_(index), dev_(dev) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
}

void BleFollowView::draw_meter(int rssi, bool have) {
    const ui::Font& big = ui::font::fixed_8x16();
    const ui::Font& small = ui::font::fixed_5x8();
    ui::Color bg = ui::Color::black();
    ui::Color grey = ui::Color(120, 120, 120);

    painter.fill_rectangle({0, 52, 240, 268}, bg);
    painter.draw_string({6, 56}, small, ui::Color::white(), bg, name_of(dev_).substr(0, 30));
    painter.draw_string({6, 68}, small, grey, bg, mac_str(dev_.addr));

    if (!have) {
        painter.draw_string({40, 120}, big, grey, bg, "no signal...");
        return;
    }

    // -100 dBm (far) -> 0%, -30 dBm (very close) -> 100%
    int pct = (rssi + 100) * 100 / 70;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    char num[16];
    snprintf(num, sizeof(num), "%d dBm", rssi);
    painter.draw_string({76, 92}, big, ui::Color::white(), bg, num);

    int bx = 14, by = 140, bw = 212, bh = 36;
    painter.fill_rectangle({(ui::Coord)bx, (ui::Coord)by, (ui::Coord)bw, (ui::Coord)bh}, ui::Color(35, 35, 35));
    int fill = bw * pct / 100;
    ui::Color c = pct > 66 ? ui::Color(255, 40, 0) : pct > 33 ? ui::Color(255, 170, 0) : ui::Color(0, 120, 255);
    if (fill > 0) painter.fill_rectangle({(ui::Coord)bx, (ui::Coord)by, (ui::Coord)fill, (ui::Coord)bh}, c);

    const char* lab = pct > 80 ? "HOT - very close" : pct > 60 ? "warm - closer" : pct > 35 ? "cool" : "cold - far away";
    painter.draw_string({14, 188}, small, ui::Color::white(), bg, lab);

    if (rssi > last_rssi_ + 2)
        painter.draw_string({14, 204}, small, ui::Color(80, 220, 120), bg, ">>> WARMER");
    else if (rssi < last_rssi_ - 2)
        painter.draw_string({14, 204}, small, ui::Color(0, 150, 255), bg, "<<< colder");
    last_rssi_ = rssi;
}

void BleFollowView::on_framesync() {
    if (++poll_ < 8) return;  // ~a few updates per second
    poll_ = 0;
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_BLESCAN_GET_ENTRY, index_};
    ble_dev_t d;
    memset(&d, 0, sizeof(d));
    bool ok = _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&d, sizeof(d));
    bool have = ok && (d.addr[0] || d.addr[1] || d.addr[2] || d.addr[3] || d.addr[4] || d.addr[5]);
    draw_meter((int)d.rssi, have);
    _api->set_dirty();
}

// ============================ Main view ============================
BleScanView::BleScanView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};  // physical back exits the app
}

BleScanView::~BleScanView() {
    send_app(0);
    Theme::destroy();
}

void BleScanView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void BleScanView::send_scan(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_BLESCAN_SET_AUTO;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void BleScanView::send_clear() {
    uint16_t cmd = (uint16_t)Command::PPCMD_BLESCAN_CLEAR;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

bool BleScanView::fetch_entry(uint16_t idx, ble_dev_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_BLESCAN_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

void BleScanView::load_list() {
    entries_.clear();
    menu.clear();
    menu.add_item({paused_ ? "[ Resume ]" : "[ Pause ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) {
                       paused_ = !paused_;
                       send_scan(!paused_);
                       load_list();
                   }});
    menu.add_item({"[ Clear ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       send_clear();
                       count_ = 0;
                       load_list();
                   }});
    menu.add_item({trackers_only_ ? "[ Show: trackers ]" : "[ Show: all ]",
                   ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       trackers_only_ = !trackers_only_;
                       load_list();
                   }});
    menu.add_item({"[ Save to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       save_csv();
                   }});

    for (uint16_t i = 0; i < count_; i++) {
        ble_dev_t e;
        if (!fetch_entry(i, e)) continue;
        entries_.push_back(e);
        if (trackers_only_ && !ble_is_tracker(e.kind)) continue;
        std::string name = name_of(e);
        if (name.size() > 12) name = name.substr(0, 12);
        // trackers show their type (red) instead of the vendor
        std::string tag = e.kind ? ble_kind_name(e.kind) : ble_vendor(e.company);
        if (tag.size() > 8) tag = tag.substr(0, 8);
        char row[48];
        snprintf(row, sizeof(row), "%-12s %-8s %d", name.c_str(), tag.c_str(), (int)e.rssi);
        ui::Color col = ble_is_tracker(e.kind) ? ui::Theme::getInstance()->fg_red->foreground : ui::Color::white();
        ble_dev_t ec = e;
        uint16_t idx = i;
        menu.add_item({std::string(row), col, nullptr,
                       [this, ec, idx](KeyEvent) { nav_.push<BleDetailView>(ec, idx); }});
    }
    loaded_ = true;
    set_dirty();
}

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

void BleScanView::save_csv() {
    TCHAR dir[16];
    to_tchar("BLE", dir, 16);
    _api->f_mkdir(dir);
    TCHAR path[32];
    to_tchar("BLE/devices.csv", path, 32);
    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        text_info.set("SD save failed");
        text_info.set_dirty();
        return;
    }
    UINT bw = 0;
    const char* hdr = "mac,name,vendor,type,addr,rssi\r\n";
    _api->f_write(&fil, hdr, strlen(hdr), &bw);
    for (auto& e : entries_) {
        std::string line = mac_str(e.addr) + "," + csv_field(name_of(e)) + "," +
                           csv_field(ble_vendor(e.company)) + "," + ble_kind_name(e.kind) + "," +
                           (e.addr_type == 0 ? "public" : "random") + "," + std::to_string((int)e.rssi) + "\r\n";
        _api->f_write(&fil, line.data(), line.size(), &bw);
    }
    _api->f_close(&fil);
    text_info.set("Saved devices.csv");
    text_info.set_dirty();
}

void BleScanView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    uint16_t new_count = buf[0];
    scanning_ = (uint8_t)buf[1];

    bool count_changed = (new_count != count_);
    count_ = new_count;

    text_info.set("Devices: " + to_string_dec_uint(count_) + (paused_ ? " (paused)" : ""));
    text_info.set_dirty();

    if (!paused_ && (!loaded_ || count_changed)) load_list();
    _api->set_dirty();
}

void BleScanView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != BLESCAN_APP_ID) send_app(BLESCAN_APP_ID);
        } else {
            send_app(BLESCAN_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
