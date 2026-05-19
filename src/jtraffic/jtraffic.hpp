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

class JTrafficView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        JTrafficView* parent_view = nullptr;

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
    uint32_t mesh_id;
    uint32_t signal_id;
    bool flash_state;
    uint32_t entropy_timer;

    ui::Text text_line1{{0, 0, 0, 0}};
    ui::Text text_line2{{0, 0, 0, 0}};
    ui::Text text_line3{{0, 0, 0, 0}};
    ui::Text text_line4{{0, 0, 0, 0}};
    ui::Text text_line5{{0, 0, 0, 0}};
    ui::Text text_warning{{0, 0, 0, 0}};
    ui::Button button_action{{0, 0, 0, 0}, "CONNECT MESH"};

   public:
    JTrafficView(const JTrafficView&) = delete;
    JTrafficView& operator=(const JTrafficView&) = delete;

    JTrafficView(ui::NavigationView& nav) : hidden_pad{},
                                            status{0},
                                            timer{0},
                                            mesh_id{0},
                                            signal_id{0},
                                            flash_state{false},
                                            entropy_timer{0} {
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
        button_action.set_parent_rect({(ui::Coord)((sw - 18 * 8) / 2), 200, (ui::Dim)(18 * 8), 32});

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
                srand(entropy_timer);
                status = 1;
                timer = 0;
                button_action.hidden(true);
                hidden_pad.focus();
            } else if (status == 2) {
                status = 3;
                timer = 0;
                button_action.hidden(true);
                text_warning.set("");
                hidden_pad.focus();
            } else if (status == 4) {
                signal_id = ((uint32_t)rand() << 16) ^ rand();
                status = 2;
                timer = 0;
                flash_state = false;
                set_dirty();

                text_line1.set("CONNECTION ESTABLISHED");
                text_line2.set("MESH NODE: 0x" + to_string_hex(mesh_id, 8));
                text_line3.set("");
                text_line4.set("NEAREST SIGNAL DETECTED:");
                text_line5.set("SIGNAL ID: 0x" + to_string_hex(signal_id, 8));
                text_warning.set("STATUS: RED");

                button_action.set_text("SET TO GREEN");
                button_action.focus();
            }
        };

        reset_ui();
    }

    ~JTrafficView() {
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
            _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::dark_green().v);
        } else {
            _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);
        }
    }

    void on_framesync() override {
        entropy_timer++;

        if (status == 1) {
            timer++;
            if (timer % 3 == 0) {
                text_line1.set("SCANNING 5.9GHz DSRC BAND...");
                text_line2.set("HANDSHAKE: 0x" + to_string_hex((uint32_t)rand(), 8));
                text_line3.set("DECRYPTING WPA3 PMK...");
                text_line4.set("MESH_TX: " + to_string_hex((uint32_t)rand(), 8));
                text_line5.set("MESH_RX: " + to_string_hex((uint32_t)rand(), 8));
            }

            int sw = *_api->screen_width - 32;
            int bar_width = (timer * sw) / 90;
            if (bar_width > sw) bar_width = sw;
            _api->fill_rectangle(16, 130, bar_width, 16, ui::Color::blue().v);

            if (timer > 90) {
                status = 2;
                timer = 0;
                mesh_id = ((uint32_t)rand() << 16) ^ rand();
                signal_id = ((uint32_t)rand() << 16) ^ rand();
                _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);

                text_line1.set("CONNECTION ESTABLISHED");
                text_line2.set("MESH NODE: 0x" + to_string_hex(mesh_id, 8));
                text_line3.set("");
                text_line4.set("NEAREST SIGNAL DETECTED:");
                text_line5.set("SIGNAL ID: 0x" + to_string_hex(signal_id, 8));
                text_warning.set("STATUS: RED");

                button_action.set_text("SET TO GREEN");
                button_action.hidden(false);
                button_action.focus();
            }
        } else if (status == 3) {
            timer++;
            if (timer % 3 == 0) {
                text_line1.set("AUTH: MAINT_TECH_OVERRIDE");
                text_line2.set("BYPASSING NEMA TS2 CONTROLLER...");
                text_line3.set("INJECTING CAN-BUS FRAME...");
                text_line4.set("TX: 0x" + to_string_hex((uint32_t)rand(), 8));
                text_line5.set("");
                text_warning.set("OVERRIDING...");
            }

            int sw = *_api->screen_width - 32;
            int bar_width = (timer * sw) / 60;
            if (bar_width > sw) bar_width = sw;
            _api->fill_rectangle(16, 130, bar_width, 16, ui::Color::orange().v);

            if (timer > 60) {
                status = 4;
                timer = 0;
                _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);

                text_line1.set("OVERRIDE SUCCESSFUL");
                text_line2.set("SIGNAL ID: 0x" + to_string_hex(signal_id, 8));
                text_line3.set("");
                text_line4.set("");
                text_line5.set("");
                text_warning.set("STATUS: GREEN");

                button_action.set_text("SCAN NEXT SIGNAL");
                button_action.hidden(false);
                button_action.focus();
            }
        } else if (status == 4) {
            timer++;
            // Pontosan 3 alkalommal villan (60 frame alatt)
            if (timer <= 60 && timer % 10 == 0) {
                flash_state = !flash_state;
                set_dirty();
            }
        }
    }

    bool handle_game_key(const ui::KeyEvent event) {
        if ((status == 2 || status == 4) && event == ui::KeyEvent::Select) {
            button_action.on_select(button_action);
            return true;
        }
        return false;
    }

    bool handle_game_encoder(const ui::EncoderEvent) {
        return false;
    }

   private:
    void reset_ui() {
        text_line1.set("TRAFFIC MESH NETWORK TOOL");
        text_line2.set("V2.8.4 - ROOT ACCESS");
        text_line3.set("");
        text_line4.set("READY TO SCAN");
        text_line5.set("");
        text_warning.set("NOT CONNECTED");
        button_action.set_text("CONNECT MESH");
        button_action.hidden(false);
    }
};

}  // namespace ui