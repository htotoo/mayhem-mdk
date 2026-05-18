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

class SpaceInvadersView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        SpaceInvadersView* parent_view = nullptr;
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

    struct Bullet {
        bool active = false;
        int x = 0;
        int y = 0;
        int last_y = 0;
    };

    struct Enemy {
        bool active = false;
        int x = 0;
        int y = 0;
        int last_x = 0;
        int last_y = 0;
        int type = 0;
        int hp = 1;
        int dir = 1;
    };

    struct Star {
        int x = 0;
        int y = 0;
        int last_y = 0;
    };

    HiddenController hidden_pad;
    Bullet bullets[8];
    Enemy enemies[6];
    Star stars[10];

    int player_x = 120;
    int last_player_x = 120;
    int player_y = 290;

    uint32_t score = 0;
    int shield = 100;
    int spawn_timer = 0;
    int speed_level = 1;
    uint8_t game_status = 0;
    uint32_t frame_count = 0;

    ui::Labels label_title{
        {{UI_POS_X_CENTER(14), 4}, "SPACE INVADERS", ui::Color::white()}};

    ui::Text text_score{{8, 20, 14 * 8, 16}};
    ui::Text text_shield{{140, 20, 12 * 8, 16}};
    ui::Text text_status{{UI_POS_X_CENTER(16), 120, 16 * 8, 16}};

    ui::Button button_start{{UI_POS_X_CENTER(12), 150, 12 * 8, 32}, "START"};

   public:
    SpaceInvadersView(ui::NavigationView& nav) : hidden_pad{}, bullets{}, enemies{}, stars{} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        add_children({&label_title,
                      &text_score,
                      &text_shield,
                      &text_status,
                      &button_start,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            button_start.hidden(true);
            text_status.hidden(true);
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_status.set("SYSTEM READY");
        text_status.hidden(false);
        text_score.set("SCORE: 0");
        text_shield.set("SHIELD: 100%");

        recalculate_dimensions();

        for (int i = 0; i < 10; i++) {
            stars[i].x = rand() % (*_api->screen_width - 2);
            stars[i].y = 54 + (rand() % (*_api->screen_height - 74));
            stars[i].last_y = stars[i].y;
        }
    }

    ~SpaceInvadersView() {
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
        _api->fill_rectangle(0, 52, sw, sh - 52, ui::Color::black().v);
        if (game_status == 1 || game_status == 2) {
            draw_entities();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            frame_count++;
            int sw = *_api->screen_width;
            int sh = *_api->screen_height;

            if (spawn_timer > 0) {
                spawn_timer--;
            } else {
                for (int i = 0; i < 6; i++) {
                    if (!enemies[i].active) {
                        enemies[i].active = true;
                        enemies[i].x = 20 + (rand() % (sw - 50));
                        enemies[i].y = 54;
                        enemies[i].last_x = enemies[i].x;
                        enemies[i].last_y = enemies[i].y;
                        enemies[i].type = rand() % 3;
                        enemies[i].dir = (rand() % 2 == 0) ? 1 : -1;
                        if (enemies[i].type == 0)
                            enemies[i].hp = 1;
                        else if (enemies[i].type == 1)
                            enemies[i].hp = 1;
                        else if (enemies[i].type == 2)
                            enemies[i].hp = 3;
                        break;
                    }
                }
                spawn_timer = 80 - (speed_level * 5);
                if (spawn_timer < 25) spawn_timer = 25;
            }

            erase_entities();

            if (frame_count % 3 == 0) {
                for (int i = 0; i < 10; i++) {
                    stars[i].last_y = stars[i].y;
                    stars[i].y += 1;
                    if (stars[i].y > (sh - 10)) {
                        stars[i].y = 54;
                        stars[i].x = rand() % (sw - 2);
                    }
                }
            }

            for (int i = 0; i < 8; i++) {
                if (bullets[i].active) {
                    bullets[i].last_y = bullets[i].y;
                    bullets[i].y -= 4;
                    if (bullets[i].y < 52) {
                        bullets[i].active = false;
                    }
                }
            }

            int espeed = 1;
            if (speed_level > 3 && frame_count % 2 == 0) espeed = 2;

            if (frame_count % 2 == 0) {
                for (int i = 0; i < 6; i++) {
                    if (enemies[i].active) {
                        enemies[i].last_x = enemies[i].x;
                        enemies[i].last_y = enemies[i].y;

                        enemies[i].y += espeed;

                        if (enemies[i].type == 1) {
                            enemies[i].x += enemies[i].dir * 1;
                            if (enemies[i].x < 5 || enemies[i].x > (sw - 25)) {
                                enemies[i].dir = -enemies[i].dir;
                            }
                        }

                        if (enemies[i].y > (player_y + 5)) {
                            enemies[i].active = false;
                            shield -= 15;
                            update_stats();
                            if (shield <= 0) handle_game_over();
                        }
                    }
                }
            }

            for (int b = 0; b < 8; b++) {
                if (!bullets[b].active) continue;
                for (int e = 0; e < 6; e++) {
                    if (!enemies[e].active) continue;

                    int ew = (enemies[e].type == 2) ? 20 : 12;
                    int eh = (enemies[e].type == 2) ? 16 : 12;

                    if (bullets[b].x >= enemies[e].x && bullets[b].x <= enemies[e].x + ew &&
                        bullets[b].y >= enemies[e].y && bullets[b].y <= enemies[e].y + eh) {
                        bullets[b].active = false;

                        int b_last_y = bullets[b].last_y;
                        int b_last_h = 6;
                        if (b_last_y < 52) {
                            b_last_h -= (52 - b_last_y);
                            b_last_y = 52;
                        }
                        if (b_last_h > 0) safe_fill(bullets[b].x, b_last_y, 2, b_last_h, ui::Color::black());

                        int b_y = bullets[b].y;
                        int b_h = 6;
                        if (b_y < 52) {
                            b_h -= (52 - b_y);
                            b_y = 52;
                        }
                        if (b_h > 0) safe_fill(bullets[b].x, b_y, 2, b_h, ui::Color::black());

                        enemies[e].hp--;
                        if (enemies[e].hp <= 0) {
                            enemies[e].active = false;

                            int e_y = enemies[e].y;
                            int e_h = eh;
                            if (e_y < 52) {
                                e_h -= (52 - e_y);
                                e_y = 52;
                            }
                            if (e_h > 0) safe_fill(enemies[e].x, e_y, ew, e_h, ui::Color::black());

                            int e_last_y = enemies[e].last_y;
                            int e_last_h = eh;
                            if (e_last_y < 52) {
                                e_last_h -= (52 - e_last_y);
                                e_last_y = 52;
                            }
                            if (e_last_h > 0) safe_fill(enemies[e].last_x, e_last_y, ew, e_last_h, ui::Color::black());

                            score += (enemies[e].type + 1) * 50;

                            if (score > 500) speed_level = 2;
                            if (score > 1500) speed_level = 3;
                            if (score > 3000) speed_level = 4;
                            if (score > 6000) speed_level = 5;

                            update_stats();
                        }
                        break;
                    }
                }
            }

            draw_entities();
        }
    }

    void handle_game_key(const ui::KeyEvent event) {
        if (event == ui::KeyEvent::Select || event == ui::KeyEvent::Up) {
            for (int i = 0; i < 8; i++) {
                if (!bullets[i].active) {
                    bullets[i].active = true;
                    bullets[i].x = player_x + 7;
                    bullets[i].y = player_y - 6;
                    bullets[i].last_y = bullets[i].y;
                    break;
                }
            }
        }
    }

    void handle_game_encoder(const ui::EncoderEvent event) {
        int sw = *_api->screen_width;
        last_player_x = player_x;
        player_x += event * 5;
        if (player_x < 5) player_x = 5;
        if (player_x > (sw - 21)) player_x = (sw - 21);
    }

   private:
    void recalculate_dimensions() {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        player_x = sw / 2 - 8;
        last_player_x = player_x;
        player_y = sh - 30;
    }

    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 52) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 52) {
            h -= (52 - y);
            y = 52;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }

    void erase_entities() {
        if (player_x != last_player_x) {
            safe_fill(last_player_x, player_y, 16, 16, ui::Color::black());
            last_player_x = player_x;
        }
        for (int i = 0; i < 10; i++) {
            if (stars[i].last_y >= 52) {
                safe_fill(stars[i].x, stars[i].last_y, 1, 1, ui::Color::black());
            }
        }
        for (int i = 0; i < 8; i++) {
            if (bullets[i].active || bullets[i].y < bullets[i].last_y) {
                int by = bullets[i].last_y;
                int bh = 6;
                if (by < 52) {
                    bh -= (52 - by);
                    by = 52;
                }
                if (bh > 0) {
                    safe_fill(bullets[i].x, by, 2, bh, ui::Color::black());
                }
            }
        }
        for (int i = 0; i < 6; i++) {
            if (enemies[i].active || enemies[i].y > enemies[i].last_y || enemies[i].x != enemies[i].last_x) {
                int ew = (enemies[i].type == 2) ? 20 : 12;
                int eh = (enemies[i].type == 2) ? 16 : 12;
                int ey = enemies[i].last_y;
                if (ey < 52) {
                    eh -= (52 - ey);
                    ey = 52;
                }
                if (eh > 0) {
                    safe_fill(enemies[i].last_x, ey, ew, eh, ui::Color::black());
                }
            }
        }
    }

    void draw_entities() {
        for (int i = 0; i < 10; i++) {
            if (stars[i].y >= 52) {
                safe_fill(stars[i].x, stars[i].y, 1, 1, ui::Color::grey());
            }
        }
        for (int i = 0; i < 8; i++) {
            if (bullets[i].active && bullets[i].y >= 52) {
                safe_fill(bullets[i].x, bullets[i].y, 2, 6, ui::Color::cyan());
            }
        }
        for (int i = 0; i < 6; i++) {
            if (enemies[i].active) {
                int ey = enemies[i].y;
                int eh = (enemies[i].type == 2) ? 16 : 12;
                int ew = (enemies[i].type == 2) ? 20 : 12;
                int offset_y = 0;
                if (ey < 52) {
                    offset_y = 52 - ey;
                    eh -= offset_y;
                    ey = 52;
                }
                if (eh > 0) {
                    if (enemies[i].type == 0) {
                        safe_fill(enemies[i].x, ey, ew, eh, ui::Color::green());
                        int core_y = enemies[i].y + 4;
                        int core_h = 4;
                        if (core_y < 52) {
                            core_h -= (52 - core_y);
                            core_y = 52;
                        }
                        if (core_h > 0) safe_fill(enemies[i].x + 4, core_y, 4, core_h, ui::Color::black());
                    } else if (enemies[i].type == 1) {
                        int top_y = enemies[i].y;
                        int top_h = 4;
                        if (top_y < 52) {
                            top_h -= (52 - top_y);
                            top_y = 52;
                        }
                        if (top_h > 0) safe_fill(enemies[i].x + 2, top_y, 8, top_h, ui::Color::yellow());

                        int mid_y = enemies[i].y + 4;
                        int mid_h = 4;
                        if (mid_y < 52) {
                            mid_h -= (52 - mid_y);
                            mid_y = 52;
                        }
                        if (mid_h > 0) safe_fill(enemies[i].x, mid_y, 12, mid_h, ui::Color::orange());

                        int bot_y = enemies[i].y + 8;
                        int bot_h = 4;
                        if (bot_y < 52) {
                            bot_h -= (52 - bot_y);
                            bot_y = 52;
                        }
                        if (bot_h > 0) safe_fill(enemies[i].x + 4, bot_y, 4, bot_h, ui::Color::yellow());
                    } else if (enemies[i].type == 2) {
                        ui::Color jc = ui::Color::red();
                        if (enemies[i].hp == 2)
                            jc = ui::Color::orange();
                        else if (enemies[i].hp == 1)
                            jc = ui::Color::yellow();
                        safe_fill(enemies[i].x, ey, ew, eh, jc);

                        int eye_y = enemies[i].y + 4;
                        int eye_h = 4;
                        if (eye_y < 52) {
                            eye_h -= (52 - eye_y);
                            eye_y = 52;
                        }
                        if (eye_h > 0) {
                            safe_fill(enemies[i].x + 4, eye_y, 4, eye_h, ui::Color::black());
                            safe_fill(enemies[i].x + 12, eye_y, 4, eye_h, ui::Color::black());
                        }
                    }
                }
            }
        }
        safe_fill(player_x + 6, player_y, 4, 6, ui::Color::light_grey());
        safe_fill(player_x + 2, player_y + 6, 12, 4, ui::Color::blue());
        safe_fill(player_x, player_y + 10, 16, 6, ui::Color::blue());
    }

    void update_stats() {
        text_score.set("SCORE: " + to_string_dec_uint(score));
        if (shield < 0) shield = 0;
        text_shield.set("SHIELD: " + to_string_dec_uint(shield) + "%");
    }

    void handle_game_over() {
        game_status = 2;
        text_status.set("GAME OVER");
        text_status.hidden(false);
        button_start.set_text("RETRY");
        button_start.hidden(false);
        button_start.focus();
    }

    void start_game() {
        score = 0;
        shield = 100;
        speed_level = 1;
        spawn_timer = 0;
        frame_count = 0;

        recalculate_dimensions();

        for (int i = 0; i < 8; i++) bullets[i].active = false;
        for (int i = 0; i < 6; i++) enemies[i].active = false;

        update_stats();
        game_status = 1;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        _api->fill_rectangle(0, 52, sw, sh - 52, ui::Color::black().v);
        draw_entities();
    }
};

}  // namespace ui