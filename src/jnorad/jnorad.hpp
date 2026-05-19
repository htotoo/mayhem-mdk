#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include <cmath>
#include <cstdlib>
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

class JNoradView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        JNoradView* parent_view = nullptr;

        HiddenController() {}
        HiddenController(const HiddenController&) = delete;
        HiddenController& operator=(const HiddenController&) = delete;

        bool on_key(const ui::KeyEvent event) override {
            if (parent_view && parent_view->handle_game_key(event)) return true;
            return Button::on_key(event);
        }
        bool on_encoder(const ui::EncoderEvent event) override {
            if (parent_view && parent_view->handle_game_encoder(event)) return true;
            return Button::on_encoder(event);
        }
    };

    HiddenController hidden_pad;

    uint8_t status;
    uint32_t timer;
    int target_idx;
    bool flash_state;

    const char* targets[16] = {
        "MOSCOW, RU",
        "BEIJING, CN",
        "PYONGYANG, KP",
        "TEHRAN, IR",
        "AREA 51, US",
        "WASHINGTON D.C., US",
        "PENTAGON, US",
        "NEW YORK CITY, US",
        "LOS ANGELES, US",
        "CHICAGO, US",
        "LONDON, UK",
        "PARIS, FR",
        "BERLIN, DE",
        "BRUSSELS, BE",
        "ROME, IT",
        "WARSAW, PL"};

    ui::Text text_line1{{0, 0, 0, 0}};
    ui::Text text_line2{{0, 0, 0, 0}};
    ui::Text text_line3{{0, 0, 0, 0}};
    ui::Text text_line4{{0, 0, 0, 0}};
    ui::Text text_line5{{0, 0, 0, 0}};
    ui::Text text_warning{{0, 0, 0, 0}};
    ui::Button button_action{{0, 0, 0, 0}, "INITIATE LINK"};

   public:
    JNoradView(const JNoradView&) = delete;
    JNoradView& operator=(const JNoradView&) = delete;

    JNoradView(ui::NavigationView& nav) : hidden_pad{},
                                          status{0},
                                          timer{0},
                                          target_idx{0},
                                          flash_state{false} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        int sw = *_api->screen_width;

        text_line1.set_parent_rect({8, 20, (ui::Dim)(sw - 16), 16});
        text_line2.set_parent_rect({8, 40, (ui::Dim)(sw - 16), 16});
        text_line3.set_parent_rect({8, 60, (ui::Dim)(sw - 16), 16});
        text_line4.set_parent_rect({8, 80, (ui::Dim)(sw - 16), 16});
        text_line5.set_parent_rect({8, 100, (ui::Dim)(sw - 16), 16});

        text_warning.set_parent_rect({(ui::Coord)((sw - 18 * 8) / 2), 160, (ui::Dim)(18 * 8), 16});
        button_action.set_parent_rect({(ui::Coord)((sw - 16 * 8) / 2), 200, (ui::Dim)(16 * 8), 32});

        add_children({&text_line1,
                      &text_line2,
                      &text_line3,
                      &text_line4,
                      &text_line5,
                      &text_warning,
                      &button_action,
                      &hidden_pad});

        button_action.on_select = [this](ui::Button&) {
            if (status == 0) {
                status = 1;
                timer = 0;
                button_action.hidden(true);
                text_line1.set("CONNECTING TO 104.28.14.73...");
                text_line2.set("ESTABLISHING SECURE TUNNEL...");
                hidden_pad.focus();
            } else if (status == 2) {
                status = 3;
                timer = 0;
                text_warning.set("");
                button_action.set_text("LAUNCH STRIKE");
                hidden_pad.focus();
            } else if (status == 3) {
                status = 4;
                timer = 0;
                button_action.hidden(true);
                text_line1.set("");
                text_line2.set("");
                text_line3.set("");
                text_line4.set("");
                text_line5.set("");
                text_warning.set("");
                hidden_pad.focus();
            }
        };

        reset_ui();
    }

    ~JNoradView() {
        ui::Theme::destroy();
    }

    void focus() override {
        if (status == 1 || status == 3 || status == 4) {
            hidden_pad.focus();
        } else {
            button_action.focus();
        }
    }

    void paint(ui::Painter& painter) override {
        (void)painter;
        if (status == 4 && flash_state) {
            _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::red().v);
        } else {
            _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);
        }
    }

    void on_framesync() override {
        if (status == 1) {
            timer++;

            if (timer < 90) {
                text_line1.set("CONNECTING TO 104.28.14.73...");
                text_line2.set("ESTABLISHING SECURE TUNNEL...");
            } else if (timer < 180) {
                text_line1.set("BYPASSING DOD FIREWALL...");
                text_line2.set("INJECTING PAYLOAD...");
            } else {
                if (timer % 4 == 0) {
                    text_line1.set("BRUTE_FORCE: " + to_string_hex((uint32_t)rand(), 8) + to_string_hex((uint32_t)rand(), 8));
                    text_line2.set("MEM_DUMP [" + to_string_hex((uint32_t)rand(), 4) + "]: " + to_string_hex((uint32_t)rand(), 8));
                    text_line3.set("RSA-4096 CRACK: " + to_string_hex((uint32_t)rand(), 8) + to_string_hex((uint32_t)rand(), 8));
                    text_line4.set("NODE HASH: " + to_string_hex((uint32_t)rand(), 8));
                    text_line5.set("OVERRIDE_KEY: " + to_string_hex((uint32_t)rand(), 8));
                }
            }

            int progress = (timer * 100) / 480;
            text_warning.set("DECRYPTING... " + to_string_dec_uint(progress) + "%");

            int bar_width = (timer * (*_api->screen_width - 32)) / 480;
            if (bar_width > *_api->screen_width - 32) bar_width = *_api->screen_width - 32;

            _api->fill_rectangle(16, 130, bar_width, 16, ui::Color::green().v);

            if (timer > 480) {
                status = 2;
                timer = 0;
                _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);
                text_line1.set("CONNECTION ESTABLISHED.");
                text_line2.set("SECURITY OVERRIDE SUCCESSFUL.");
                text_line3.set("");
                text_line4.set("");
                text_line5.set("");
                text_warning.set("ACCESS GRANTED");

                button_action.set_text("ENTER COMMAND");
                button_action.hidden(false);
                button_action.focus();
            }
        } else if (status == 2) {
            timer++;
            if (timer % 30 == 0) {
                if (timer % 60 == 0)
                    text_warning.set("ACCESS GRANTED");
                else
                    text_warning.set("");
            }
        } else if (status == 3) {
            text_line1.set("NORAD STRATEGIC COMMAND");
            text_line2.set("DEFCON 1 - GLOBAL THREAT");
            text_line3.set("-------------------------");
            text_line4.set("SELECT TARGET: ");
            text_line5.set(targets[target_idx]);
        } else if (status == 4) {
            timer++;
            if (timer % 15 == 0) {
                flash_state = !flash_state;

                if (flash_state) {
                    text_warning.set("LAUNCH INITIATED");
                } else {
                    text_warning.set("T-MINUS " + to_string_dec_uint(10 - (timer / 60)) + " SECONDS");
                }

                set_dirty();
            }
            if (timer > 600) {
                status = 5;
                flash_state = false;
                set_dirty();
                text_warning.set("COMMUNICATION LOST.");
            }
        }
    }

    bool handle_game_key(const ui::KeyEvent event) {
        if (status == 3 && event == ui::KeyEvent::Select) {
            button_action.on_select(button_action);
            return true;
        }
        return false;
    }

    bool handle_game_encoder(const ui::EncoderEvent event) {
        if (status == 3) {
            if (event > 0) {
                target_idx++;
                if (target_idx > 15) target_idx = 0;
            } else if (event < 0) {
                target_idx--;
                if (target_idx < 0) target_idx = 15;
            }
            return true;
        }
        return false;
    }

   private:
    void reset_ui() {
        text_line1.set("DEPARTMENT OF DEFENSE");
        text_line2.set("NETWORK TERMINAL V4.2");
        text_line3.set("");
        text_line4.set("");
        text_line5.set("");
        text_warning.set("UNAUTHORIZED ACCESS");
        button_action.hidden(false);
    }
};

}  // namespace ui