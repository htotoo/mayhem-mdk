/*
 * Copyright (C) 2026 HTotoo
 */

#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

class JStarlinkView : public ui::View {
   public:
    JStarlinkView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&label_title,
                      &option_sat,
                      &button_hack,
                      &text_log1,
                      &text_log2,
                      &text_log3,
                      &progress_bar,
                      &text_wifi_ssid,
                      &text_wifi_pass,
                      &text_clients});

        button_hack.on_select = [this](ui::Button&) {
            if (hack_state == 0) {
                hack_state = 1;
                frame_tick = 0;
                button_hack.set_text("ABORTING...");
            } else if (hack_state == 5) {
                hack_state = 0;
                button_hack.set_text("INIT EXPLOIT");
                reset_ui();
            }
        };

        reset_ui();
    }

    ~JStarlinkView() {
        ui::Theme::destroy();
    }

    void focus() override {
        option_sat.focus();
    }

    void on_framesync() override {
        if (hack_state == 0) return;

        frame_tick++;

        switch (hack_state) {
            case 1:
                if (frame_tick % 30 == 0) {
                    text_log1.set("Aligning Phased Array...");
                    text_log2.set("Targeting: " + option_sat.options()[option_sat.selected_index()].first);
                }
                if (frame_tick % 60 == 0) {
                    text_log3.set("Bypassing Geofence [" + to_string_dec_uint(frame_tick / 60) + "s]");
                }
                if (frame_tick > 240) {
                    hack_state = 2;
                    frame_tick = 0;
                }
                break;

            case 2:
                text_log1.set("Ku-Band Handshake captured.");

                if (frame_tick % 10 == 0) {
                    const char* hex_chars = "0123456789ABCDEF";
                    std::string fake_hex = "0x";
                    for (int i = 0; i < 8; i++) fake_hex += hex_chars[rand() % 16];
                    text_log2.set("Bruteforce AES: " + fake_hex);
                }

                if (frame_tick % 6 == 0) {
                    progress_bar.set_value(frame_tick / 6);
                }

                if (frame_tick > 600) {
                    hack_state = 3;
                    frame_tick = 0;
                    progress_bar.set_value(100);
                }
                break;

            case 3:
                if (frame_tick == 1) {
                    text_log1.set("ROOT ACCESS GRANTED.");
                    text_log2.set("Injecting DHCP Payload...");
                    text_log3.set("Starting Rogue AP...");
                }
                if (frame_tick > 180) {
                    hack_state = 4;
                    frame_tick = 0;
                }
                break;

            case 4:
                text_log1.set(">> SATELLITE LINK ACTIVE <<");
                text_log2.set("Bandwidth: 2.4 Gbps Down");
                text_log3.set("Routing traffic via Tor...");

                text_wifi_ssid.set("SSID: Starlink_Public_Free");
                text_wifi_pass.set("PASS: elonmusk123");

                text_clients.set("Clients: 0 | RX: 0 Mbps");

                button_hack.set_text("STOP HOTSPOT");
                hack_state = 5;
                frame_tick = 0;
                break;

            case 5:
                if (frame_tick < 420) {
                    if (frame_tick % 60 == 0) {
                        text_clients.set("Clients: 0 | RX: 0 Mbps");
                    }
                } else {
                    if (frame_tick % 60 == 0) {
                        int rx_speed = 50 + (rand() % 400);
                        text_clients.set("Clients: 1 | RX: " + to_string_dec_uint(rx_speed) + " Mbps");
                    }
                }
                break;
        }
    }

   private:
    uint32_t frame_tick = 0;
    uint8_t hack_state = 0;

    void reset_ui() {
        text_log1.set("Terminal Ready.");
        text_log2.set("");
        text_log3.set("");
        progress_bar.set_value(0);
        text_wifi_ssid.set("");
        text_wifi_pass.set("");
        text_clients.set("");
    }
    ui::Labels label_title{
        {{UI_POS_X_CENTER(21), UI_POS_Y(1)}, "STARLINK EXPLOIT TOOL", ui::Theme::getInstance()->fg_light->foreground}};

    ui::OptionsField option_sat{
        {UI_POS_X_CENTER(12), UI_POS_Y(3)},
        12,
        {{"Constell. v1", 0},
         {"Constell. v2", 1},
         {"Polar Orbit", 2},
         {"Direct2Cell", 3}}};

    ui::Button button_hack{{UI_POS_X_CENTER(14), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)}, "INIT EXPLOIT"};

    ui::Text text_log1{{UI_POS_X_CENTER(26), UI_POS_Y(5), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_log2{{UI_POS_X_CENTER(26), UI_POS_Y(6), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_log3{{UI_POS_X_CENTER(26), UI_POS_Y(7), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::ProgressBar progress_bar{{UI_POS_X_CENTER(26), UI_POS_Y(9), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::Text text_wifi_ssid{{UI_POS_X_CENTER(26), UI_POS_Y(11), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_wifi_pass{{UI_POS_X_CENTER(26), UI_POS_Y(12), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_clients{{UI_POS_X_CENTER(26), UI_POS_Y(14), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
};

}  // namespace ui