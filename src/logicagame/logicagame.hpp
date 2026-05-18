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
            if (parent_view && parent_view->handle_game_key(event)) return true;
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

    int lane_x[4] = {25, 75, 125, 175};
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
    ui::Text text_status{{UI_POS_X_CENTER(16), 120, 16 * 8, 16}};

    ui::Button button_start{{UI_POS_X_CENTER(12), 150, 12 * 8, 32}, "START"};

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
                      &button_start,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            button_start.hidden(true);
            text_status.set("");
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_status.set("SYSTEM READY");
        text_score.set("SCORE: 0");
        text_health.set("STAB: 100%");
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
        painter.fill_rectangle({0, 40, 240, 280}, ui::Color::dark_grey());
        if (game_status == 1) {
            draw_static_ui();
            draw_entities();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
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

                    if (blocks[i].y > 300) {
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

    bool handle_game_key(const ui::KeyEvent event) {
        if (game_status == 1) {
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
                return true;
            }
        }
        return false;
    }

   private:
    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        if (x >= 240 || y >= 320 || x + w <= 0 || y + h <= 40) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 40) {
            h -= (40 - y);
            y = 40;
        }
        if (x + w > 240) w = 240 - x;
        if (y + h > 320) h = 320 - y;
        if (w > 0 && h > 0) {
            _api->fill_rectangle((ui::Coord)x, (ui::Coord)y, (ui::Dim)w, (ui::Dim)h, c.v);
        }
    }

    void draw_static_ui() {
        safe_fill(0, 258, 240, 2, ui::Color::white());
        safe_fill(0, 280, 240, 2, ui::Color::white());
        for (int i = 0; i < 4; i++) {
            if (lane_flash[i] == 0) draw_target_box(i, lane_colors[i]);
        }
    }

    void draw_target_box(int lane, ui::Color c) {
        int x = lane_x[lane];
        safe_fill(x, 260, 40, 4, c);
        safe_fill(x, 276, 40, 4, c);
        safe_fill(x, 260, 4, 20, c);
        safe_fill(x + 36, 260, 4, 20, c);
        safe_fill(x + 4, 264, 32, 12, ui::Color::dark_grey());
    }

    void erase_entities() {
        for (int i = 0; i < 16; i++) {
            if (blocks[i].active || blocks[i].y > blocks[i].last_y) {
                safe_fill(lane_x[blocks[i].lane], blocks[i].last_y, 40, 15, ui::Color::dark_grey());
            }
        }
    }

    void draw_entities() {
        for (int i = 0; i < 16; i++) {
            if (blocks[i].active) {
                safe_fill(lane_x[blocks[i].lane], blocks[i].y, 40, 15, lane_colors[blocks[i].lane]);
            }
        }
    }

    void process_hit(int lane) {
        int best_idx = -1;
        int best_dist = 999;

        for (int i = 0; i < 16; i++) {
            if (blocks[i].active && blocks[i].lane == lane) {
                int dist = abs(blocks[i].y - 260);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_idx = i;
                }
            }
        }

        if (best_idx != -1 && best_dist < 30) {
            safe_fill(lane_x[lane], blocks[best_idx].y, 40, 15, ui::Color::dark_grey());
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

        for (int i = 0; i < 16; i++) blocks[i].active = false;
        for (int i = 0; i < 4; i++) lane_flash[i] = 0;

        update_stats();
        game_status = 1;
        _api->fill_rectangle((ui::Coord)0, (ui::Coord)40, (ui::Dim)240, (ui::Dim)280, ui::Color::dark_grey().v);
        draw_static_ui();
    }
};

}  // namespace ui