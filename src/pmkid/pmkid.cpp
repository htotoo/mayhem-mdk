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

#include "pmkid.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::PmkidView>();
}

namespace ui {

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

static std::string ssid_of(const hs_disp_t& e) {
    char s[33];
    memcpy(s, e.ssid, 32);
    s[32] = 0;
    return std::string(s);
}

// ============================ Detail view ============================
PmkidDetailView::PmkidDetailView(NavigationView& nav, const hs_disp_t& e)
    : nav_(nav) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});

    std::string ss = ssid_of(e);
    std::vector<std::string> lines;
    lines.push_back(std::string("Type: ") + (e.type == 1 ? "PMKID" : "Handshake"));
    lines.push_back("Network: " + (ss.empty() ? std::string("<unknown>") : ss));
    lines.push_back("AP:  " + mac_str(e.ap));
    lines.push_back("STA: " + mac_str(e.sta));
    lines.push_back("Ch: " + std::to_string(e.channel));

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
PmkidView::PmkidView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};  // physical back exits the whole app
}

PmkidView::~PmkidView() {
    send_app(0);
    Theme::destroy();
}

void PmkidView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void PmkidView::send_capture(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_SET_AUTO;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void PmkidView::send_clear() {
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_CLEAR;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

void PmkidView::send_channel(uint8_t ch) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_SET_CHANNEL;
    memcpy(tx, &cmd, 2);
    tx[2] = ch;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void PmkidView::send_deauth() {
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_DEAUTH;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

bool PmkidView::fetch_entry(uint16_t idx, hs_disp_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_PMKID_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

uint16_t PmkidView::line_len(uint16_t idx) {
    uint8_t tx[6];
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_SET_LINEPOS;
    memcpy(tx, &cmd, 2);
    tx[2] = idx & 0xFF;
    tx[3] = idx >> 8;
    tx[4] = 0;
    tx[5] = 0;
    _api->i2c_read(tx, 6, nullptr, 0);
    uint16_t rcmd = (uint16_t)Command::PPCMD_PMKID_GET_LINELEN;
    uint8_t rb[2] = {0, 0};
    if (!_api->i2c_read((uint8_t*)&rcmd, 2, rb, 2)) return 0;
    return rb[0] | (rb[1] << 8);
}

int PmkidView::line_chunk(uint16_t idx, uint16_t off, uint8_t* out) {
    // caller passes a 96-byte buffer; we set (idx,off) then read exactly 96
    uint8_t tx[6];
    uint16_t cmd = (uint16_t)Command::PPCMD_PMKID_SET_LINEPOS;
    memcpy(tx, &cmd, 2);
    tx[2] = idx & 0xFF;
    tx[3] = idx >> 8;
    tx[4] = off & 0xFF;
    tx[5] = off >> 8;
    _api->i2c_read(tx, 6, nullptr, 0);
    uint16_t rcmd = (uint16_t)Command::PPCMD_PMKID_GET_LINECHUNK;
    if (!_api->i2c_read((uint8_t*)&rcmd, 2, out, 96)) return 0;
    return 96;
}

void PmkidView::load_list() {
    entries_.clear();
    menu.clear();
    {
        std::string chlabel = chsel_ == 0 ? "[ Ch: auto ]" : ("[ Ch: " + std::to_string((int)chsel_) + " ]");
        menu.add_item({chlabel, ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                           chsel_ = (chsel_ >= 13) ? 0 : (chsel_ + 1);
                           send_channel(chsel_);
                           load_list();
                       }});
    }
    menu.add_item({paused_ ? "[ Resume ]" : "[ Pause ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) {
                       paused_ = !paused_;
                       send_capture(!paused_);
                       load_list();
                   }});
    menu.add_item({"[ Deauth ]", ui::Theme::getInstance()->fg_red->foreground, nullptr, [this](KeyEvent) {
                       send_deauth();
                       text_info.set(chsel_ == 0 ? "Deauth (lock a ch!)" : ("Deauth sent ch " + std::to_string((int)chsel_)));
                       text_info.set_dirty();
                   }});
    menu.add_item({"[ Clear ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       send_clear();
                       count_ = 0;
                       load_list();
                   }});
    menu.add_item({"[ Save to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       save_hc22000();
                   }});

    for (uint16_t i = 0; i < count_; i++) {
        hs_disp_t e;
        if (!fetch_entry(i, e)) continue;
        entries_.push_back(e);
        std::string name = ssid_of(e);
        if (name.empty()) name = "<unknown>";
        if (name.size() > 10) name = name.substr(0, 10);
        char row[40];
        snprintf(row, sizeof(row), "%c %-10s %-2d %02X%02X",
                 e.type == 2 ? 'H' : 'P', name.c_str(), e.channel, e.ap[4], e.ap[5]);
        hs_disp_t ec = e;
        menu.add_item({std::string(row), ui::Color::white(), nullptr,
                       [this, ec](KeyEvent) { nav_.push<PmkidDetailView>(ec); }});
    }
    loaded_ = true;
    set_dirty();
}

// Fetch each hashcat line in chunks and write WIFI/Handshakes/handshakes.hc22000.
void PmkidView::save_hc22000() {
    TCHAR dir[24];
    to_tchar("WIFI", dir, 24);
    _api->f_mkdir(dir);
    to_tchar("WIFI/Handshakes", dir, 24);
    _api->f_mkdir(dir);

    TCHAR path[48];
    to_tchar("WIFI/Handshakes/handshakes.hc22000", path, 48);
    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        text_info.set("SD save failed");
        text_info.set_dirty();
        return;
    }
    UINT bw = 0;
    uint16_t written = 0;
    for (uint16_t i = 0; i < count_; i++) {
        uint16_t len = line_len(i);
        if (len == 0 || len > 512) continue;
        uint16_t off = 0;
        uint8_t chunk[96];
        while (off < len) {
            int n = line_chunk(i, off, chunk);
            if (n <= 0) break;
            uint16_t take = (len - off) < (uint16_t)n ? (len - off) : (uint16_t)n;
            _api->f_write(&fil, chunk, take, &bw);
            off += take;
        }
        _api->f_write(&fil, "\r\n", 2, &bw);
        written++;
    }
    _api->f_close(&fil);
    text_info.set("Saved " + to_string_dec_uint(written) + " to SD");
    text_info.set_dirty();
}

void PmkidView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[7] = {0, 0, 0, 0, 0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 14) == false) return;
    uint16_t new_count = buf[0];
    capturing_ = (uint8_t)buf[1];

    bool count_changed = (new_count != count_);
    count_ = new_count;

    if (count_ == 0)
        text_info.set(paused_ ? "Paused" : "Listening...");
    else
        text_info.set("Captures: " + to_string_dec_uint(count_) + (paused_ ? " (paused)" : ""));
    text_info.set_dirty();

    if (!paused_ && (!loaded_ || count_changed)) load_list();
    _api->set_dirty();
}

void PmkidView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != PMKID_APP_ID) send_app(PMKID_APP_ID);
        } else {
            send_app(PMKID_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
