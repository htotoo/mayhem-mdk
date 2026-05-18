#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include <cmath>
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

class RetroRacerView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        RetroRacerView* parent_view = nullptr;
        HiddenController() {}
        HiddenController(const HiddenController&) = delete;
        HiddenController& operator=(const HiddenController&) = delete;

        bool on_key(const ui::KeyEvent event) override {
            if (parent_view && parent_view->game_status == 1) {
                parent_view->handle_game_key(event);
                return true;
            }
            return Button::on_key(event);
        }
        bool on_encoder(const ui::EncoderEvent event) override {
            if (parent_view && parent_view->game_status == 1) {
                parent_view->handle_game_encoder(event);
                return true;
            }
            return Button::on_encoder(event);
        }
    };

    struct Obst {
        bool active = false;
        int lane = 0;
        int y = 0;
        int type = 0;
    };

    HiddenController hidden_pad{};
    Obst obstacles[8];
    int player_lane = 1;
    int player_y = 260;
    int lane_w = 80;
    int lane_centers[3] = {40, 120, 200};
    int line_x1 = 78;
    int line_x2 = 158;
    int road_offset = 0;
    uint32_t score = 0;
    int speed_level = 1;
    int speed_pixels = 4;
    int spawn_timer = 0;
    uint8_t game_status = 0;

    ui::Labels label_title{
        {{UI_POS_X_CENTER(11), 4}, "RETRO RACER", ui::Color::white()}};

    ui::Text text_score{{8, 20, 14 * 8, 16}};
    ui::Text text_speed{{150, 20, 10 * 8, 16}};
    ui::Text text_status{{UI_POS_X_CENTER(16), 110, 16 * 8, 16}};

    ui::Button button_start{{UI_POS_X_CENTER(12), 140, 12 * 8, 32}, "START"};

   public:
    RetroRacerView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        add_children({&label_title,
                      &text_score,
                      &text_speed,
                      &text_status,
                      &button_start,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            button_start.hidden(true);
            text_status.set("");
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_status.set("READY?");
        text_score.set("SCORE: 0");
        text_speed.set("SPEED: 1");

        recalculate_dimensions();
    }

    ~RetroRacerView() {
        ui::Theme::destroy();
    }

    void focus() override {
        if (game_status == 1) {
            hidden_pad.focus();
        } else {
            button_start.focus();
        }
    }

    void paint(ui::Painter& painter) override {
        (void)painter;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        _api->fill_rectangle(0, 40, sw, sh - 40, ui::Color::dark_grey().v);
        if (game_status == 1 || game_status == 2) {
            draw_entities();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            int sh = *_api->screen_height;
            score += speed_level;

            if (score > 1000) {
                speed_level = 2;
                speed_pixels = 6;
            }
            if (score > 3000) {
                speed_level = 3;
                speed_pixels = 8;
            }
            if (score > 6000) {
                speed_level = 4;
                speed_pixels = 10;
            }
            if (score > 12000) {
                speed_level = 5;
                speed_pixels = 13;
            }
            if (score > 20000) {
                speed_level = 6;
                speed_pixels = 16;
            }

            if (spawn_timer > 0) {
                spawn_timer--;
            } else {
                int lane = rand() % 3;
                bool clear = true;
                for (int i = 0; i < 8; i++) {
                    if (obstacles[i].active && obstacles[i].lane == lane && obstacles[i].y < 140) {
                        clear = false;
                    }
                }

                int next_rate = 40 - (speed_level * 5);
                if (next_rate < 12) next_rate = 12;
                spawn_timer = next_rate;

                if (clear) {
                    for (int i = 0; i < 8; i++) {
                        if (!obstacles[i].active) {
                            obstacles[i].active = true;
                            obstacles[i].lane = lane;
                            obstacles[i].y = 40;
                            obstacles[i].type = rand() % 2;
                            break;
                        }
                    }
                }
            }

            erase_entities();

            road_offset = (road_offset + speed_pixels) % 40;

            for (int i = 0; i < 8; i++) {
                if (obstacles[i].active) {
                    obstacles[i].y += speed_pixels;

                    if (obstacles[i].y - speed_pixels > sh) {
                        obstacles[i].active = false;
                        obstacles[i].y = 0;
                    } else {
                        if (obstacles[i].lane == player_lane && obstacles[i].y + 20 > player_y && obstacles[i].y < player_y + 30) {
                            draw_entities();
                            handle_crash();
                            return;
                        }
                    }
                }
            }

            draw_entities();

            if (score % 5 == 0) {
                text_score.set("SCORE: " + to_string_dec_uint(score));
                text_speed.set("SPEED: " + to_string_dec_uint(speed_level));
            }
        }
    }

    void handle_game_key(const ui::KeyEvent event) {
        if (event == ui::KeyEvent::Left) {
            if (game_status == 1 && player_lane > 0) {
                erase_entities();
                player_lane--;
                draw_entities();
            }
        } else if (event == ui::KeyEvent::Right) {
            if (game_status == 1 && player_lane < 2) {
                erase_entities();
                player_lane++;
                draw_entities();
            }
        }
    }

    void handle_game_encoder(const ui::EncoderEvent event) {
        if (game_status == 1) {
            if (event < 0 && player_lane > 0) {
                erase_entities();
                player_lane--;
                draw_entities();
            } else if (event > 0 && player_lane < 2) {
                erase_entities();
                player_lane++;
                draw_entities();
            }
        }
    }

   private:
    void recalculate_dimensions() {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        lane_w = sw / 3;
        for (int i = 0; i < 3; i++) {
            lane_centers[i] = (i * lane_w) + (lane_w / 2);
        }

        line_x1 = lane_w - 2;
        line_x2 = (lane_w * 2) - 2;
        player_y = sh - 60;
    }

    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 40) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 40) {
            h -= (40 - y);
            y = 40;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w > 0 && h > 0) _api->fill_rectangle((ui::Coord)x, (ui::Coord)y, (ui::Dim)w, (ui::Dim)h, c.v);
    }

    void draw_car(int x, int y, ui::Color body_color) {
        safe_fill(x - 2, y + 4, 4, 8, ui::Color::black());
        safe_fill(x + 18, y + 4, 4, 8, ui::Color::black());
        safe_fill(x - 2, y + 18, 4, 8, ui::Color::black());
        safe_fill(x + 18, y + 18, 4, 8, ui::Color::black());
        safe_fill(x, y, 20, 30, body_color);
        safe_fill(x + 2, y + 8, 16, 6, ui::Color::blue());
        safe_fill(x + 2, y + 20, 16, 4, ui::Color::blue());
        safe_fill(x + 2, y, 4, 3, ui::Color::yellow());
        safe_fill(x + 14, y, 4, 3, ui::Color::yellow());
        safe_fill(x + 2, y + 27, 5, 3, ui::Color::red());
        safe_fill(x + 13, y + 27, 5, 3, ui::Color::red());
    }

    void draw_barrel(int x, int y) {
        safe_fill(x + 2, y, 16, 20, ui::Color::red());
        safe_fill(x + 2, y + 4, 16, 3, ui::Color::light_grey());
        safe_fill(x + 2, y + 13, 16, 3, ui::Color::light_grey());
    }

    void draw_cone(int x, int y) {
        safe_fill(x + 8, y, 4, 6, ui::Color::orange());
        safe_fill(x + 6, y + 6, 8, 4, ui::Color::white());
        safe_fill(x + 4, y + 10, 12, 6, ui::Color::orange());
        safe_fill(x, y + 16, 20, 4, ui::Color::orange());
    }

    void erase_entities() {
        int sh = *_api->screen_height;

        int last_offset = (road_offset - speed_pixels + 40) % 40;
        for (int y = 40; y < sh; y += 40) {
            safe_fill(line_x1, y + last_offset, 4, 20, ui::Color::dark_grey());
            safe_fill(line_x2, y + last_offset, 4, 20, ui::Color::dark_grey());
        }

        int px = lane_centers[player_lane];
        safe_fill(px - 15, player_y, 30, 32, ui::Color::dark_grey());

        for (int i = 0; i < 8; i++) {
            if (obstacles[i].active) {
                int ox = lane_centers[obstacles[i].lane];
                int old_y = obstacles[i].y - speed_pixels;
                safe_fill(ox - 15, old_y, 30, 32, ui::Color::dark_grey());
            }
        }
    }

    void draw_entities() {
        int sh = *_api->screen_height;

        for (int y = 40; y < sh; y += 40) {
            int calculated_y = y + road_offset;
            safe_fill(line_x1, calculated_y, 4, 20, ui::Color::white());
            safe_fill(line_x2, calculated_y, 4, 20, ui::Color::white());
        }

        for (int i = 0; i < 8; i++) {
            if (obstacles[i].active && obstacles[i].y >= 40) {
                int ox = lane_centers[obstacles[i].lane];
                if (obstacles[i].type == 0)
                    draw_barrel(ox - 8, obstacles[i].y);
                else
                    draw_cone(ox - 10, obstacles[i].y);
            }
        }

        int px = lane_centers[player_lane];
        draw_car(px - 10, player_y, ui::Color::light_grey());
    }

    void handle_crash() {
        game_status = 2;
        text_status.set("CRASH!");
        button_start.set_text("RETRY");
        button_start.hidden(false);
        button_start.focus();
    }

    void start_game() {
        score = 0;
        player_lane = 1;
        road_offset = 0;
        speed_level = 1;
        speed_pixels = 4;
        spawn_timer = 0;

        recalculate_dimensions();

        for (int i = 0; i < 8; i++) {
            obstacles[i].active = false;
            obstacles[i].y = 0;
        }

        game_status = 1;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        _api->fill_rectangle(0, 40, sw, sh - 40, ui::Color::dark_grey().v);
        draw_entities();
        text_score.set("SCORE: 0");
        text_speed.set("SPEED: 1");
    }
};

}  // namespace ui