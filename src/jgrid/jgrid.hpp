#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <utility>
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

class JGridHackView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        JGridHackView* parent_view = nullptr;

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
    int target_type;
    int city_idx;
    bool flash_state;
    uint32_t entropy_timer;

    std::vector<std::pair<std::string, int32_t>> grid_opts;
    std::vector<std::pair<std::string, int32_t>> city_opts;

    ui::Text text_title;
    ui::Text label_grid;
    ui::OptionsField options_grid;
    ui::Text label_city;
    ui::OptionsField options_city;

    ui::Text text_line1;
    ui::Text text_line2;
    ui::Text text_line3;
    ui::Text text_line4;
    ui::Text text_line5;
    ui::Text text_warning;
    ui::Button button_action;

   public:
    JGridHackView(const JGridHackView&) = delete;
    JGridHackView& operator=(const JGridHackView&) = delete;

    JGridHackView(ui::NavigationView& nav) : hidden_pad{},
                                             status{0},
                                             timer{0},
                                             target_type{0},
                                             city_idx{0},
                                             flash_state{false},
                                             entropy_timer{0},
                                             grid_opts{{"POWER GRID", 0}, {"WATER SUPPLY", 1}},
                                             city_opts{
                                                 {"BUDAPEST", 0},
                                                 {"NEW YORK", 1},
                                                 {"LONDON", 2},
                                                 {"TOKYO", 3},
                                                 {"BERLIN", 4},
                                                 {"LOS ANGELES", 5},
                                                 {"CHICAGO", 6},
                                                 {"HOUSTON", 7},
                                                 {"PHOENIX", 8},
                                                 {"PHILADELPHIA", 9},
                                                 {"SAN ANTONIO", 10},
                                                 {"SAN DIEGO", 11},
                                                 {"DALLAS", 12},
                                                 {"AUSTIN", 13},
                                                 {"SEATTLE", 14},
                                                 {"MIAMI", 15},
                                                 {"PARIS", 16},
                                                 {"ROME", 17},
                                                 {"MADRID", 18},
                                                 {"VIENNA", 19},
                                                 {"WARSAW", 20},
                                                 {"MOSCOW", 21}},
                                             text_title{{0, 0, 0, 0}},
                                             label_grid{{16, 50, 8 * 8, 16}, "TARGET: "},
                                             options_grid{{16 + 8 * 8, 50}, 14, grid_opts},
                                             label_city{{16, 80, 10 * 8, 16}, "LOCATION: "},
                                             options_city{{16 + 10 * 8, 80}, 14, city_opts},
                                             text_line1{{0, 0, 0, 0}},
                                             text_line2{{0, 0, 0, 0}},
                                             text_line3{{0, 0, 0, 0}},
                                             text_line4{{0, 0, 0, 0}},
                                             text_line5{{0, 0, 0, 0}},
                                             text_warning{{0, 0, 0, 0}},
                                             button_action{{0, 0, 0, 0}, "INITIALIZE EXPLOIT"} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        int sw = *_api->screen_width;

        text_title.set_parent_rect({(ui::Coord)((sw - 26 * 8) / 2), 20, (ui::Dim)(26 * 8), 16});

        text_line1.set_parent_rect({0, 20, (ui::Dim)(sw - 8), 16});
        text_line2.set_parent_rect({0, 40, (ui::Dim)(sw - 8), 16});
        text_line3.set_parent_rect({0, 60, (ui::Dim)(sw - 8), 16});
        text_line4.set_parent_rect({0, 80, (ui::Dim)(sw - 8), 16});
        text_line5.set_parent_rect({0, 100, (ui::Dim)(sw - 8), 16});

        text_warning.set_parent_rect({(ui::Coord)((sw - 20 * 8) / 2), 160, (ui::Dim)(20 * 8), 16});
        button_action.set_parent_rect({(ui::Coord)((sw - 18 * 8) / 2), 200, (ui::Dim)(18 * 8), 32});

        add_children({&text_title,
                      &label_grid,
                      &options_grid,
                      &label_city,
                      &options_city,
                      &text_line1,
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
                target_type = options_grid.selected_index_value();
                city_idx = options_city.selected_index_value();

                status = 1;
                timer = 0;

                text_title.hidden(true);
                label_grid.hidden(true);
                options_grid.hidden(true);
                label_city.hidden(true);
                options_city.hidden(true);
                button_action.hidden(true);

                text_line1.hidden(false);
                text_line2.hidden(false);
                text_line3.hidden(false);
                text_line4.hidden(false);
                text_line5.hidden(false);
                text_warning.hidden(false);

                set_dirty();
                hidden_pad.focus();
            } else if (status == 4) {
                status = 5;
                timer = 0;
                button_action.hidden(true);
                text_line1.set("");
                text_line2.set("");
                text_line3.set("");
                text_line4.set("");
                text_line5.set("");
                text_warning.set("");
                hidden_pad.focus();
            } else if (status == 6) {
                reset_ui();
                options_grid.focus();
                set_dirty();
            }
        };

        reset_ui();
    }

    ~JGridHackView() {
        ui::Theme::destroy();
    }

    void focus() override {
        if (status == 0) {
            options_grid.focus();
        } else if (status >= 1 && status <= 3) {
            hidden_pad.focus();
        } else if (status == 5 || status == 6) {
            button_action.focus();
        } else {
            button_action.focus();
        }
    }

    void paint(ui::Painter& painter) override {
        (void)painter;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        if (status == 5 && flash_state) {
            ui::Color flash_col = (target_type == 0) ? ui::Color::yellow() : ui::Color::cyan();
            _api->fill_rectangle(0, 16, sw, sh - 16, flash_col.v);
        } else if (status >= 1) {
            _api->fill_rectangle(0, 16, sw, sh - 16, ui::Color::black().v);
        } else {
            _api->fill_rectangle(0, 16, sw, sh - 16, ui::Color::dark_grey().v);
        }
    }

    void on_framesync() override {
        entropy_timer++;

        if (status >= 1 && status <= 3) {
            timer++;

            if (status == 1) {
                if (timer % 20 == 0) {
                    text_line1.set("SCANNING SENSOR MESH...");
                    text_line2.set("CITY NODE: 0x" + to_string_hex((uint32_t)rand(), 8));
                    text_line3.set("EXPLOITING ZERO-DAY IN ZIGBEE...");
                }
                draw_progress(timer);
                if (timer > 320) {
                    status = 2;
                    timer = 0;
                }
            } else if (status == 2) {
                if (timer % 20 == 0) {
                    const char* h0 = (target_type == 0) ? "ISOLATING SUBSTATION RELAYS..." : "ISOLATING PUMP STATIONS...";
                    text_line1.set("PIVOTING TO SCADA NETWORK...");
                    text_line2.set("PLC IP: 10." + to_string_dec_uint(rand() % 255) + ".42." + to_string_dec_uint(rand() % 255));
                    text_line3.set("BYPASSING AUTHENTICATION...");
                    text_line4.set(h0);
                }
                draw_progress(320 + timer);
                if (timer > 320) {
                    status = 3;
                    timer = 0;
                }
            } else if (status == 3) {
                if (timer % 20 == 0) {
                    const char* h1 = (target_type == 0) ? "OVERLOADING TRANSFORMER TAP..." : "OVERRIDING PRESSURE VALVES...";
                    const char* h2 = (target_type == 0) ? "DISABLING PROTECTIVE CIRCUITS" : "DISABLING CHLORINE SENSORS...";
                    text_line4.set(h1);
                    text_line5.set(h2);
                    text_warning.set("INJECTING STUXNET_V3...");
                }
                draw_progress(640 + timer);
                if (timer > 320) {
                    status = 4;
                    timer = 0;
                    set_dirty();
                    text_line1.set("ACCESS GRANTED.");
                    text_line2.set("MAIN CONTROLLER COMPROMISED.");
                    text_line3.set("");
                    text_line4.set("");
                    text_line5.set("");
                    text_warning.set("READY TO EXECUTE");

                    button_action.set_text("EXECUTE SHUTDOWN");
                    button_action.hidden(false);
                    button_action.focus();
                }
            }
        } else if (status == 5) {
            timer++;
            if (timer % 20 == 0) {
                flash_state = !flash_state;
                set_dirty();
            }
            if (timer > 240) {
                status = 6;
                flash_state = false;
                set_dirty();
                text_warning.set(target_type == 0 ? "BLACKOUT TRIGGERED" : "WATER SUPPLY HALTED");
                button_action.set_text("REBOOT SYSTEM");
                button_action.hidden(false);
                button_action.focus();
            }
        }
    }

    bool handle_game_key(const ui::KeyEvent) {
        return false;
    }

    bool handle_game_encoder(const ui::EncoderEvent) {
        return false;
    }

   private:
    void draw_progress(int current_tick) {
        int sw = *_api->screen_width - 32;
        int bar_width = (current_tick * sw) / 960;
        if (bar_width > sw) bar_width = sw;
        _api->fill_rectangle(16, 130, bar_width, 16, ui::Color::red().v);
    }

    void reset_ui() {
        status = 0;
        timer = 0;
        flash_state = false;

        text_title.set("GRID VULNERABILITY SCANNER");
        text_title.hidden(false);
        label_grid.hidden(false);
        options_grid.hidden(false);
        label_city.hidden(false);
        options_city.hidden(false);

        text_line1.set("");
        text_line1.hidden(true);
        text_line2.set("");
        text_line2.hidden(true);
        text_line3.set("");
        text_line3.hidden(true);
        text_line4.set("");
        text_line4.hidden(true);
        text_line5.set("");
        text_line5.hidden(true);
        text_warning.set("");
        text_warning.hidden(true);

        button_action.set_text("INITIALIZE EXPLOIT");
        button_action.hidden(false);

        set_dirty();
    }
};

}  // namespace ui