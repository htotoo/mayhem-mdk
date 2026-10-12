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

#include "deauth.hpp"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::DeauthView>();
}

namespace ui {

static std::string ssid_of(const deauth_ap_t& e) {
    char s[33];
    memcpy(s, e.ssid, 32);
    s[32] = 0;
    if (s[0] == 0) return "<hidden>";
    return std::string(s);
}

DeauthView::DeauthView(NavigationView& nav)
    : nav_(nav) {
    add_children({&text_info, &text_header, &menu});
    text_header.set_style(Theme::getInstance()->fg_light);
    menu.on_left = [this]() {};  // physical back exits the app
}

DeauthView::~DeauthView() {
    send_attack(false);  // never leave the attack running
    send_app(0);
    Theme::destroy();
}

void DeauthView::send_app(uint16_t appid) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appid);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void DeauthView::send_rescan() {
    uint16_t cmd = (uint16_t)Command::PPCMD_DEAUTH_RESCAN;
    _api->i2c_read((uint8_t*)&cmd, 2, nullptr, 0);
}

void DeauthView::send_target(uint16_t idx) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_DEAUTH_SET_TARGET, idx};
    _api->i2c_read((uint8_t*)tx, 4, nullptr, 0);
}

void DeauthView::send_attack(bool on) {
    uint8_t tx[3];
    uint16_t cmd = (uint16_t)Command::PPCMD_DEAUTH_SET_ATTACK;
    memcpy(tx, &cmd, 2);
    tx[2] = on ? 1 : 0;
    _api->i2c_read(tx, 3, nullptr, 0);
}

bool DeauthView::fetch_entry(uint16_t idx, deauth_ap_t& out) {
    uint16_t tx[2] = {(uint16_t)Command::PPCMD_DEAUTH_GET_ENTRY, idx};
    memset(&out, 0, sizeof(out));
    return _api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&out, sizeof(out));
}

void DeauthView::load_list() {
    entries_.clear();
    menu.clear();
    menu.add_item({"[ Rescan ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       if (attacking_) return;  // can't scan while attacking
                       send_rescan();
                       text_info.set("Scanning...");
                       text_info.set_dirty();
                   }});
    menu.add_item({attacking_ ? "[ STOP attack ]" : "[ Start attack ]",
                   attacking_ ? ui::Theme::getInstance()->fg_green->foreground : ui::Theme::getInstance()->fg_red->foreground,
                   nullptr, [this](KeyEvent) {
                       if (!have_target_ && !attacking_) {
                           text_info.set("Pick a target first");
                           text_info.set_dirty();
                           return;
                       }
                       send_attack(!attacking_);
                   }});

    for (uint16_t i = 0; i < ap_count_; i++) {
        deauth_ap_t e;
        if (!fetch_entry(i, e)) continue;
        entries_.push_back(e);
        std::string name = ssid_of(e);
        if (name.size() > 12) name = name.substr(0, 12);
        char row[40];
        snprintf(row, sizeof(row), "%-12s %-2d %d", name.c_str(), e.channel, (int)e.rssi);
        uint16_t idx = i;
        std::string full = ssid_of(e);
        uint8_t ch = e.channel;
        menu.add_item({std::string(row), ui::Color::white(), nullptr, [this, idx, full, ch](KeyEvent) {
                           send_target(idx);
                           have_target_ = true;
                           target_name_ = full;
                           target_ch_ = ch;
                           load_list();
                       }});
    }
    loaded_ = true;
    set_dirty();
}

void DeauthView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[4] = {0, 0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 8) == false) return;
    uint16_t new_count = buf[0];
    uint8_t new_attack = (uint8_t)buf[1];
    scanning_ = (uint8_t)buf[2];

    bool count_changed = (new_count != ap_count_);
    bool attack_changed = (new_attack != attacking_);
    ap_count_ = new_count;
    attacking_ = new_attack;

    if (attacking_)
        text_info.set("ATTACKING " + target_name_.substr(0, 10) + " ch" + to_string_dec_uint(target_ch_));
    else if (have_target_)
        text_info.set("Target: " + target_name_.substr(0, 12) + " ch" + to_string_dec_uint(target_ch_));
    else
        text_info.set("APs: " + to_string_dec_uint(ap_count_) + (scanning_ ? " (scan)" : ""));
    text_info.set_dirty();

    if (!attacking_ && (!loaded_ || count_changed)) load_list();
    if (attack_changed) load_list();  // refresh the toggle label
    _api->set_dirty();
}

void DeauthView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            if (capp != DEAUTH_APP_ID) send_app(DEAUTH_APP_ID);
        } else {
            send_app(DEAUTH_APP_ID);
        }
    }
    if (config_loaded < 20) return;

    if (++poll_counter >= 15) {
        poll_counter = 0;
        get_status();
    }
}

}  // namespace ui
