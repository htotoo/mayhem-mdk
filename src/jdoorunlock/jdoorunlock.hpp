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

class JDoorUnlockView : public ui::View {
   public:
    JDoorUnlockView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&label_title,
                      &option_target,
                      &button_hack,
                      &text_log1,
                      &text_log2,
                      &text_log3,
                      &progress_bar,
                      &text_status_title,
                      &text_status_pin,
                      &text_status_relay});

        button_hack.on_select = [this](ui::Button&) {
            if (hack_state == 0) {
                hack_state = 1;
                frame_tick = 0;
                button_hack.set_text("ABORTING...");
            } else if (hack_state == 4) {
                hack_state = 0;
                button_hack.set_text("INITIATE BREACH");
                reset_ui();
            }
        };

        reset_ui();
    }

    ~JDoorUnlockView() {
        ui::Theme::destroy();
    }

    void focus() override {
        option_target.focus();
    }

    void on_framesync() override {
        if (hack_state == 0) return;

        frame_tick++;

        switch (hack_state) {
            case 1:
                if (frame_tick % 30 == 0) {
                    text_log1.set("Scanning perimeter...");
                    text_log2.set("Target: " + option_target.options()[option_target.selected_index()].first);
                }
                if (frame_tick % 60 == 0) {
                    text_log3.set("Isolating RF signal [" + to_string_dec_uint(frame_tick / 60) + "s]");
                }
                if (frame_tick > 180) {
                    hack_state = 2;
                    frame_tick = 0;
                }
                break;

            case 2:
                text_log1.set("Timing Attack Started...");

                if (frame_tick % 5 == 0) {
                    std::string fake_pin = "[ ";
                    for (int i = 0; i < 6; i++) fake_pin += to_string_dec_uint(rand() % 10);
                    fake_pin += " ]";
                    text_log2.set("Bruteforce PIN: " + fake_pin);
                }

                if (frame_tick % 30 == 0) {
                    text_log3.set("Measuring CPU latency...");
                }

                if (frame_tick % 4 == 0) {
                    progress_bar.set_value(frame_tick / 4.2);
                }

                if (frame_tick > 420) {  // 7 másodperc
                    hack_state = 3;
                    frame_tick = 0;
                    progress_bar.set_value(100);
                }
                break;

            case 3:
                if (frame_tick == 1) {
                    text_log1.set("Hash collision success.");
                    text_log2.set("KEY FOUND: [ 8 3 0 1 9 4 ]");
                    text_log3.set("Transmitting payload...");
                }
                if (frame_tick > 120) {
                    hack_state = 4;
                    frame_tick = 0;
                }
                break;

            case 4:
                if (frame_tick == 1) {
                    text_log1.set(">> ACCESS BYPASSED <<");
                    text_log2.set("Security overridden.");
                    text_log3.set("");

                    text_status_title.set("STATUS: ACCESS GRANTED");
                    text_status_pin.set("STORED KEY: 830194");
                    text_status_relay.set("MagLock: DISABLED (OPEN)");

                    button_hack.set_text("CLOSE SESSION");
                }

                if (frame_tick % 60 == 0) {
                    text_status_relay.set("MagLock: DISABLED (OPEN)");
                } else if (frame_tick % 60 == 30) {
                    text_status_relay.set("MagLock: DISABLED       ");
                }
                break;
        }
    }

   private:
    uint32_t frame_tick = 0;
    uint8_t hack_state = 0;

    void reset_ui() {
        text_log1.set("Ready to scan perimeter.");
        text_log2.set("");
        text_log3.set("");
        progress_bar.set_value(0);
        text_status_title.set("");
        text_status_pin.set("");
        text_status_relay.set("");
    }

    ui::Labels label_title{
        {{UI_POS_X_CENTER(21), UI_POS_Y(1)}, "ACCESS CONTROL BYPASS", ui::Theme::getInstance()->fg_light->foreground}};

    ui::OptionsField option_target{
        {UI_POS_X_CENTER(15), UI_POS_Y(3)},
        15,
        {{"Wiegand RFID", 0},
         {"OSDP Keypad", 1},
         {"Z-Wave SmartLock", 2},
         {"OOK 433MHz Gate", 3}}};

    ui::Button button_hack{{UI_POS_X_CENTER(15), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(15), UI_POS_HEIGHT(2)}, "INITIATE BREACH"};

    ui::Text text_log1{{UI_POS_X_CENTER(30), UI_POS_Y(5), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};
    ui::Text text_log2{{UI_POS_X_CENTER(30), UI_POS_Y(6), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};
    ui::Text text_log3{{UI_POS_X_CENTER(30), UI_POS_Y(7), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};

    ui::ProgressBar progress_bar{{UI_POS_X_CENTER(30), UI_POS_Y(9), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};

    ui::Text text_status_title{{UI_POS_X_CENTER(30), UI_POS_Y(11), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};
    ui::Text text_status_pin{{UI_POS_X_CENTER(30), UI_POS_Y(12), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};
    ui::Text text_status_relay{{UI_POS_X_CENTER(30), UI_POS_Y(14), UI_POS_WIDTH(30), UI_POS_HEIGHT(1)}};
};

}  // namespace ui