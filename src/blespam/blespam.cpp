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

#include "blespam.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::BleSpamView>();
}

namespace ui {

static const char* mode_name(uint8_t m) {
    switch (m) {
        case 0: return "Apple";
        case 1: return "FastPair";
        case 2: return "SwiftPair";
        case 3: return "All";
        default: return "SD custom";
    }
}

static void to_tchar(const char* s, TCHAR* out, size_t maxlen) {
    size_t i = 0;
    for (; s[i] != 0 && i + 1 < maxlen; i++) out[i] = (TCHAR)(unsigned char)s[i];
    out[i] = 0;
}
static void tchar_to_char(const TCHAR* s, char* out, size_t maxlen) {
    size_t i = 0;
    for (; s[i] != 0 && i + 1 < maxlen; i++) out[i] = (char)(s[i] & 0xFF);
    out[i] = 0;
}
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// ============================ SD file picker ============================
BleSpamFilesView::BleSpamFilesView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &menu});
    menu.on_left = [this]() { nav_.pop(); };
}

void BleSpamFilesView::on_framesync() {
    if (!loaded_) {
        loaded_ = true;
        load_dir();
    }
}

void BleSpamFilesView::load_dir() {
    menu.clear();
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    TCHAR dirp[24];
    to_tchar("BLUETOOTH/BLESPAM", dirp, 24);
    DIR dir;
    int n = 0;
    if (_api->f_opendir(&dir, dirp) == FR_OK) {
        FILINFO fno;
        while (_api->f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            if (fno.fattrib & AM_DIR) continue;
            char name[64];
            tchar_to_char(fno.fname, name, sizeof(name));
            std::string s(name);
            if (s.size() > 4 && (s.compare(s.size() - 4, 4, ".txt") == 0 || s.compare(s.size() - 4, 4, ".TXT") == 0)) {
                std::string fn = s;
                menu.add_item({s.substr(0, 26), ui::Color::white(), nullptr,
                               [this, fn](KeyEvent) { upload_file(fn); }});
                n++;
            }
        }
        _api->f_closedir(&dir);
    }
    text_info.set(n ? "pick a spam set:" : "no .txt in BLUETOOTH/BLESPAM");
    text_info.set_dirty();
    set_dirty();
}

// Read hex lines from the file and upload them to the ESP as custom payloads.
void BleSpamFilesView::upload_file(const std::string& name) {
    char pathc[96];
    snprintf(pathc, sizeof(pathc), "BLUETOOTH/BLESPAM/%s", name.c_str());
    TCHAR path[96];
    to_tchar(pathc, path, 96);
    FIL fil;
    if (_api->f_open(&fil, path, FA_READ) != FR_OK) {
        text_info.set("open failed");
        text_info.set_dirty();
        return;
    }
    // clear existing custom set
    uint16_t clr = (uint16_t)Command::PPCMD_BLESPAM_CLEAR_CUSTOM;
    _api->i2c_read((uint8_t*)&clr, 2, nullptr, 0);

    std::string buf;
    char chunk[128];
    UINT br = 0;
    while (_api->f_read(&fil, chunk, sizeof(chunk), &br) == FR_OK && br > 0)
        buf.append(chunk, br);
    _api->f_close(&fil);

    int idx = 0;
    size_t pos = 0;
    while (pos <= buf.size() && idx < 24) {
        size_t nl = buf.find('\n', pos);
        std::string line = buf.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? buf.size() + 1 : nl + 1;
        // trim + skip comments
        if (!line.empty() && (line[0] == '#' || line[0] == ';')) continue;
        uint8_t payload[31];
        int plen = 0, hi = -1;
        for (char c : line) {
            int v = hexval(c);
            if (v < 0) continue;  // skip spaces/colons/CR
            if (hi < 0) {
                hi = v;
            } else {
                if (plen < 31) payload[plen++] = (uint8_t)((hi << 4) | v);
                hi = -1;
            }
        }
        if (plen < 1) continue;
        // upload this payload in <=24-byte chunks (I2C write must stay < 32B)
        int off = 0;
        while (off < plen) {
            int take = plen - off;
            if (take > 24) take = 24;
            uint8_t tx[2 + 2 + 24];
            uint16_t cmd = (uint16_t)Command::PPCMD_BLESPAM_ADD;
            memcpy(tx, &cmd, 2);
            tx[2] = (uint8_t)idx;
            tx[3] = (uint8_t)off;
            memcpy(tx + 4, payload + off, take);
            _api->i2c_read(tx, 4 + take, nullptr, 0);
            off += take;
        }
        idx++;
    }

    uint8_t ctx[3];
    uint16_t ccmd = (uint16_t)Command::PPCMD_BLESPAM_COMMIT;
    memcpy(ctx, &ccmd, 2);
    ctx[2] = (uint8_t)idx;
    _api->i2c_read(ctx, 3, nullptr, 0);

    // switch to custom mode
    uint8_t mtx[3];
    uint16_t mcmd = (uint16_t)Command::PPCMD_BLESPAM_SET_MODE;
    memcpy(mtx, &mcmd, 2);
    mtx[2] = 4;
    _api->i2c_read(mtx, 3, nullptr, 0);

    text_info.set("Loaded " + to_string_dec_uint(idx) + " -> custom");
    text_info.set_dirty();
}

BleSpamView::BleSpamView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &warn, &menu});
    menu.on_left = [this]() {};  // physical back exits the app
    build_menu();
}

BleSpamView::~BleSpamView() {
    send_active(false);  // never leave it spamming
    send_app(0);
    Theme::destroy();
}

void BleSpamView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void BleSpamView::send_mode(uint8_t m) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_BLESPAM_SET_MODE;
    memcpy(tx, &cmd, 2);
    tx[2] = m;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void BleSpamView::send_active(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_BLESPAM_SET_ACTIVE;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

void BleSpamView::build_menu() {
    menu.clear();
    menu.add_item({std::string("[ Mode: ") + mode_name(mode_) + " ]", ui::Theme::getInstance()->fg_yellow->foreground,
                   nullptr, [this](KeyEvent) {
                       mode_ = (mode_ + 1) % 4;
                       send_mode(mode_);
                       build_menu();
                   }});
    menu.add_item({active_ ? "[ STOP spam ]" : "[ Start spam ]",
                   active_ ? ui::Theme::getInstance()->fg_green->foreground : ui::Theme::getInstance()->fg_red->foreground,
                   nullptr, [this](KeyEvent) {
                       active_ = !active_;
                       send_active(active_);
                       build_menu();
                   }});
    menu.add_item({"[ Load from SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr,
                   [this](KeyEvent) { nav_.push<BleSpamFilesView>(); }});
    set_dirty();
}

void BleSpamView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    active_ = (buf[0] != 0);
    uint8_t new_mode = (uint8_t)buf[1];
    sent_ = buf[2];
    if (new_mode != mode_) {  // e.g. switched to custom by the SD picker
        mode_ = new_mode;
        build_menu();
    }
    if (active_)
        text_info.set("SPAMMING " + std::string(mode_name(mode_)) + "  #" + to_string_dec_uint(sent_));
    else
        text_info.set(std::string("Idle - ") + mode_name(mode_));
    text_info.set_dirty();
}

void BleSpamView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != BLESPAM_APP_ID) send_app(BLESPAM_APP_ID);
        } else {
            send_app(BLESPAM_APP_ID);
        }
        send_mode(mode_);
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
