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

#include "wifisettings.hpp"

#include <memory>
#include <string>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::WifiSettingsView>();
}

namespace ui {

WifiSettingsView::WifiSettingsView(NavigationView& nav) : nav_(nav) {
    add_children({&btn_ssid, &btn_password, &btn_send, &text_ssid, &text_password,
                  &text_ssid_ap, &text_password_ap, &btn_ssid_ap, &btn_password_ap, &btn_send_ap,
                  &text_ip, &labels});

    btn_ssid.on_select = [this](Button&) {
        text_prompt(nav_, ssid_, 30, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ssid_ = value;
            text_ssid.set("SSID: " + ssid_);
            text_ssid.set_dirty();
        });
    };
    btn_password.on_select = [this](Button&) {
        text_prompt(nav_, password_, 30, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            password_ = value;
            text_password.set("PWD: " + password_);
            text_password.set_dirty();
        });
    };
    btn_ssid_ap.on_select = [this](Button&) {
        text_prompt(nav_, ssid_ap_, 30, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ssid_ap_ = value;
            text_ssid_ap.set("SSID: " + ssid_ap_);
            text_ssid_ap.set_dirty();
        });
    };
    btn_password_ap.on_select = [this](Button&) {
        text_prompt(nav_, password_ap_, 30, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            password_ap_ = value;
            text_password_ap.set("PWD: " + password_ap_);
            text_password_ap.set_dirty();
        });
    };
    btn_send.on_select = [this](Button&) {
        wifi_config_comp_t config;
        memset(&config, 0, sizeof(config));
        strncpy(config.ssid, ssid_.c_str(), sizeof(config.ssid) - 1);
        strncpy(config.password, password_.c_str(), sizeof(config.password) - 1);
        Command cmd = Command::PPCMD_WIFI_SET_STA;
        std::vector<uint8_t> data(sizeof(wifi_config_comp_t));
        memcpy(data.data(), &config, sizeof(wifi_config_comp_t));
        data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
        if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
    };
    btn_send_ap.on_select = [this](Button&) {
        wifi_config_comp_t config;
        memset(&config, 0, sizeof(config));
        strncpy(config.ssid, ssid_ap_.c_str(), sizeof(config.ssid) - 1);
        strncpy(config.password, password_ap_.c_str(), sizeof(config.password) - 1);
        Command cmd = Command::PPCMD_WIFI_SET_AP;
        std::vector<uint8_t> data(sizeof(wifi_config_comp_t));
        memcpy(data.data(), &config, sizeof(wifi_config_comp_t));
        data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
        if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
    };
}

void WifiSettingsView::on_framesync() {
    if (config_loaded <= 10) config_loaded++;
    if (config_loaded == 10) {
        get_current_config();
    }
}

void WifiSettingsView::get_current_config() {
    Command cmd = Command::PPCMD_WIFI_GET_CONFIG;
    std::vector<uint8_t> data(sizeof(wifi_current_data_t));
    wifi_current_data_t current{};
    if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false) return;
    memcpy(&current, data.data(), sizeof(wifi_current_data_t));
    current.sta_ssid[29] = 0;
    current.sta_password[29] = 0;
    current.ap_ssid[29] = 0;
    current.ap_password[29] = 0;
    ssid_ = std::string(current.sta_ssid, 30);
    password_ = std::string(current.sta_password, 30);
    ssid_ap_ = std::string(current.ap_ssid, 30);
    password_ap_ = std::string(current.ap_password, 30);
    text_ssid.set("SSID: " + ssid_);
    text_ssid.set_dirty();
    text_password.set("PWD: " + password_);
    text_password.set_dirty();
    text_ssid_ap.set("SSID: " + ssid_ap_);
    text_ssid_ap.set_dirty();
    text_password_ap.set("PWD: " + password_ap_);
    text_password_ap.set_dirty();
    std::string ipstr = "IP: " + to_string_dec_uint(current.ip[0]) + "." + to_string_dec_uint(current.ip[1]) + "." +
                        to_string_dec_uint(current.ip[2]) + "." + to_string_dec_uint(current.ip[3]);
    text_ip.set(ipstr);
    text_ip.set_dirty();
}

void WifiSettingsView::focus() {
    btn_ssid.focus();
}

}  // namespace ui