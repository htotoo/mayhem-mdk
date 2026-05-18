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

class LogicAnalyzerView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        LogicAnalyzerView* parent_view = nullptr;

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
    };

    struct Block {
        bool active = false;
        int lane = 0;
        int y = 0;
        int last_y = 0;
    };

    HiddenController hidden_pad;
    Block blocks[16];

    int lane_x[4] = {0, 0, 0, 0};
    int lane_w = 40;
    int target_y = 260;

    ui::Color lane_colors[4] = {ui::Color::yellow(), ui::Color::blue(), ui::Color::green(), ui::Color::red()};
    int lane_flash[4] = {0, 0, 0, 0};

    uint32_t score = 0;
    int health = 100;
    float speed = 2.0f;
    int spawn_timer = 0;
    int spawn_rate = 60;
    uint8_t game_status = 0;

    ui::Labels label_title{
        {{UI_POS_X_CENTER(14), 4}, "LOGIC ANALYZER", ui::Color::white()}};

    ui::Text text_score{{8, 20, 14 * 8, 16}};
    ui::Text text_health{{150, 20, 10 * 8, 16}};

    ui::Text text_status{{UI_POS_X_CENTER(12), 50, 12 * 8, 16}};
    ui::Text text_inst1{{8, 75, 28 * 8, 16}};
    ui::Text text_inst2{{8, 95, 28 * 8, 16}};
    ui::Text text_inst3{{8, 115, 28 * 8, 16}};
    ui::Text text_inst4{{8, 135, 28 * 8, 16}};

    ui::Button button_start{{UI_POS_X_CENTER(12), 165, 12 * 8, 32}, "START"};

   public:
    LogicAnalyzerView(ui::NavigationView& nav) : hidden_pad{}, blocks{} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        add_children({&label_title,
                      &text_score,
                      &text_health,
                      &text_status,
                      &text_inst1,
                      &text_inst2,
                      &text_inst3,
                      &text_inst4,
                      &button_start,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            button_start.hidden(true);
            text_status.hidden(true);
            text_inst1.hidden(true);
            text_inst2.hidden(true);
            text_inst3.hidden(true);
            text_inst4.hidden(true);
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_status.set("HOW TO PLAY:");
        text_inst1.set("LEFT  -> Catch YELLOW blocks");
        text_inst2.set("UP    -> Catch BLUE blocks");
        text_inst3.set("DOWN  -> Catch GREEN blocks");
        text_inst4.set("RIGHT -> Catch RED blocks");

        text_score.set("SCORE: 0");
        text_health.set("STAB: 100%");

        recalculate_dimensions();
    }

    ~LogicAnalyzerView() {
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
        if (game_status == 1) {
            draw_static_ui();
            draw_entities();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            int sh = *_api->screen_height;

            if (spawn_timer > 0) {
                spawn_timer--;
            } else {
                for (int i = 0; i < 16; i++) {
                    if (!blocks[i].active) {
                        blocks[i].active = true;
                        blocks[i].lane = rand() % 4;
                        blocks[i].y = 40;
                        blocks[i].last_y = 40;
                        break;
                    }
                }
                spawn_timer = spawn_rate;
                if (spawn_rate > 15) spawn_rate -= 1;
                speed += 0.05f;
            }

            for (int i = 0; i < 4; i++) {
                if (lane_flash[i] > 0) {
                    lane_flash[i]--;
                    if (lane_flash[i] == 0) {
                        draw_target_box(i, lane_colors[i]);
                    }
                }
            }

            erase_entities();

            for (int i = 0; i < 16; i++) {
                if (blocks[i].active) {
                    blocks[i].last_y = blocks[i].y;
                    blocks[i].y += (int)speed;

                    if (blocks[i].y > (sh - 20)) {
                        blocks[i].active = false;
                        health -= 10;
                        update_stats();
                        if (health <= 0) handle_game_over();
                    }
                }
            }

            draw_static_ui();
            draw_entities();
        }
    }

    void handle_game_key(const ui::KeyEvent event) {
        int target_lane = -1;
        if (event == ui::KeyEvent::Left)
            target_lane = 0;
        else if (event == ui::KeyEvent::Up)
            target_lane = 1;
        else if (event == ui::KeyEvent::Down)
            target_lane = 2;
        else if (event == ui::KeyEvent::Right)
            target_lane = 3;

        if (target_lane != -1) {
            process_hit(target_lane);
        }
    }

   private:
    void recalculate_dimensions() {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        lane_w = sw / 5;
        int total_lanes_w = lane_w * 4;
        int start_x = (sw - total_lanes_w) / 2;

        for (int i = 0; i < 4; i++) {
            lane_x[i] = start_x + (i * lane_w);
        }

        target_y = sh - 60;
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
        if (w > 0 && h > 0) {
            _api->fill_rectangle((ui::Coord)x, (ui::Coord)y, (ui::Dim)w, (ui::Dim)h, c.v);
        }
    }

    void draw_static_ui() {
        int sw = *_api->screen_width;
        safe_fill(0, target_y - 2, sw, 2, ui::Color::white());
        safe_fill(0, target_y + 20, sw, 2, ui::Color::white());
        for (int i = 0; i < 4; i++) {
            if (lane_flash[i] == 0) draw_target_box(i, lane_colors[i]);
        }
    }

    void draw_target_box(int lane, ui::Color c) {
        int x = lane_x[lane];
        safe_fill(x, target_y, lane_w, 4, c);
        safe_fill(x, target_y + 16, lane_w, 4, c);
        safe_fill(x, target_y, 4, 20, c);
        safe_fill(x + lane_w - 4, target_y, 4, 20, c);
        safe_fill(x + 4, target_y + 4, lane_w - 8, 12, ui::Color::dark_grey());
    }

    void erase_entities() {
        for (int i = 0; i < 16; i++) {
            if (blocks[i].active || blocks[i].y > blocks[i].last_y) {
                int eh = 15;
                int ey = blocks[i].last_y;
                if (ey < 40) {
                    eh -= (40 - ey);
                    ey = 40;
                }
                if (eh > 0) {
                    safe_fill(lane_x[blocks[i].lane], ey, lane_w, eh, ui::Color::dark_grey());
                }
            }
        }
    }

    void draw_entities() {
        for (int i = 0; i < 16; i++) {
            if (blocks[i].active) {
                int bh = 15;
                int by = blocks[i].y;
                if (by < 40) {
                    bh -= (40 - by);
                    by = 40;
                }
                if (bh > 0) {
                    safe_fill(lane_x[blocks[i].lane], by, lane_w, bh, lane_colors[blocks[i].lane]);
                }
            }
        }
    }

    void process_hit(int lane) {
        int best_idx = -1;
        int best_dist = 999;

        for (int i = 0; i < 16; i++) {
            if (blocks[i].active && blocks[i].lane == lane) {
                int dist = abs(blocks[i].y - target_y);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_idx = i;
                }
            }
        }

        if (best_idx != -1 && best_dist < 30) {
            int ey = blocks[best_idx].y;
            int eh = 15;
            if (ey < 40) {
                eh -= (40 - ey);
                ey = 40;
            }
            if (eh > 0) {
                safe_fill(lane_x[lane], ey, lane_w, eh, ui::Color::dark_grey());
            }
            blocks[best_idx].active = false;
            lane_flash[lane] = 10;
            draw_target_box(lane, ui::Color::white());

            if (best_dist < 10) {
                score += 100;
                health += 2;
            } else {
                score += 50;
                health += 1;
            }
            if (health > 100) health = 100;
        } else {
            health -= 5;
            lane_flash[lane] = 10;
            draw_target_box(lane, ui::Color::red());
        }

        update_stats();
        if (health <= 0) handle_game_over();
    }

    void update_stats() {
        text_score.set("SCORE: " + to_string_dec_uint(score));
        if (health < 0) health = 0;
        text_health.set("STAB: " + to_string_dec_uint(health) + "%");
    }

    void handle_game_over() {
        game_status = 2;
        text_status.set("SYSTEM FAILURE");
        text_status.hidden(false);
        button_start.set_text("REBOOT");
        button_start.hidden(false);
        button_start.focus();
    }

    void start_game() {
        score = 0;
        health = 100;
        speed = 2.0f;
        spawn_timer = 0;
        spawn_rate = 60;

        recalculate_dimensions();

        for (int i = 0; i < 16; i++) blocks[i].active = false;
        for (int i = 0; i < 4; i++) lane_flash[i] = 0;

        update_stats();
        game_status = 1;

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        _api->fill_rectangle((ui::Coord)0, (ui::Coord)40, (ui::Dim)sw, (ui::Dim)(sh - 40), ui::Color::dark_grey().v);
        draw_static_ui();
    }
};

}  // namespace ui