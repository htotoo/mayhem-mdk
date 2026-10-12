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
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include <vector>
#include "ui_textentry.hpp"

namespace ui {

// Must match AppList::EVILPORTAL in the ESP32 firmware (appmanager.hpp).
static constexpr uint16_t EVILPORTAL_APP_ID = 4;

typedef struct wifi_config_comp_t {
    char ssid[30];
    char password[30];
} wifi_config_comp_t;

// Must match ep_client_t in the ESP32 firmware (ep_app_evilportal.hpp).
typedef struct __attribute__((packed)) {
    uint8_t mac[6];
    uint8_t ip[4];
    char hostname[32];
} ep_client_t;

#define EP_CAP_LEN 96

// Helper: write a list of text lines to /EvilPortal/<filename> on the SD card.
// Returns true on success. Implemented in evilportal.cpp.
bool ep_save_to_sd(const char* filename, const std::vector<std::string>& lines);

// ---- Connected-clients list (MAC + IP + hostname) ----
// Full-screen detail: shows one entry's complete value, wrapped over lines.
class EvilPortalDetailView : public View {
   public:
    EvilPortalDetailView(NavigationView& nav, std::string title, std::string full);
    std::string title() const override { return title_; };
    void focus() override { menu.focus(); };

   private:
    NavigationView& nav_;
    std::string title_;
    MenuView menu{{0, UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(1)}, true};
};

class EvilPortalClientsView : public View {
   public:
    EvilPortalClientsView(NavigationView& nav, uint16_t count);
    std::string title() const override { return "EP Clients"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void load();
    NavigationView& nav_;
    uint16_t count_;
    bool loaded_ = false;
    std::vector<std::string> lines_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Clients"};
    MenuView menu{{0, UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(2)}, true};
};

// ---- Captured credentials list (user | pass) ----
class EvilPortalCapturesView : public View {
   public:
    EvilPortalCapturesView(NavigationView& nav, uint16_t count);
    std::string title() const override { return "EP Captured"; };
    void on_framesync() override;
    void focus() override { menu.focus(); };

   private:
    void load();
    NavigationView& nav_;
    uint16_t count_;
    bool loaded_ = false;
    std::vector<std::string> lines_;

    Text text_info{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Captured"};
    MenuView menu{{0, UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(2)}, true};
};

// ---- Main view ----
class EvilPortalView : public View {
   public:
    EvilPortalView(NavigationView& nav);
    EvilPortalView(const EvilPortalView&) = delete;
    EvilPortalView& operator=(const EvilPortalView&) = delete;
    ~EvilPortalView() {
        Theme::destroy();
    };

    std::string title() const override { return "Evil Portal"; };
    void on_framesync() override;

   private:
    void get_status();
    void get_ap_config();
    void update_running_ui();
    void update_security_ui();
    void send_start_stop();
    void send_template(uint16_t tpl);
    bool send_ap();  // returns false if WPA2 password invalid (not sent)
    void scan_sd_html();                        // enumerate EvilPortal/html/*.html on SD
    void upload_sd_html(const std::string& name);  // stream a custom page to the ESP over I2C

    NavigationView& nav_;

    uint16_t current_app = 0;
    uint8_t config_loaded = 0;
    uint8_t poll_counter = 0;
    uint16_t client_count_ = 0;
    uint16_t capture_count_ = 0;

    // Custom SD page chosen before the portal is running: upload it shortly
    // after Start. -1 = nothing pending.
    int pending_html_idx_ = -1;
    uint8_t pending_delay_ = 0;

    std::vector<std::string> sd_html_;  // custom portal files found on the SD card

    std::string ap_ssid_ = "Free WiFi";
    std::string ap_pass_ = "";

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(3)}, "Security:", ui::Color::white()},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Portal:", ui::Color::white()}};

    Text text_ap_ssid{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "SSID: Free WiFi"};
    Button button_edit_ssid{{UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(9), UI_POS_HEIGHT(2)}, "Edit SSID"};

    // Security selector on its own labeled row under Edit SSID
    OptionsField options_security{{UI_POS_X(10), UI_POS_Y(3)}, 6, {{"Open", 0}, {"WPA2", 1}}};

    Text text_pwd{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)}, "PWD: (open)"};
    Button button_edit_pwd{{UI_POS_X(15), UI_POS_Y(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "Edit PWD"};

    // portal chooser on its own labeled row; scroll with the encoder
    OptionsField options_template{{UI_POS_X(8), UI_POS_Y(6)}, 18, {{"Captive Test", 0}}};

    Button button_startstop{{UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH(11), UI_POS_HEIGHT(2)}, "Start"};
    // sits next to Start/Stop, shown only while the portal runs
    Button button_reapply{{UI_POS_X(14), UI_POS_Y(8), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)}, "Re-apply"};

    Text text_clients{{UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH(16), UI_POS_HEIGHT(1)}, "Clients: 0"};
    Button button_clients{{UI_POS_X(17), UI_POS_Y(11), UI_POS_WIDTH(9), UI_POS_HEIGHT(1.5)}, "View"};

    Text text_captures{{UI_POS_X(0), UI_POS_Y(13), UI_POS_WIDTH(16), UI_POS_HEIGHT(1)}, "Captured: 0"};
    Button button_captures{{UI_POS_X(17), UI_POS_Y(13), UI_POS_WIDTH(9), UI_POS_HEIGHT(1.5)}, "View"};

    Text text_status{{UI_POS_X(0), UI_POS_Y(15), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, "Stopped"};
};
}  // namespace ui
