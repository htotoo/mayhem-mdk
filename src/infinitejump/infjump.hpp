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

class InfiniteJumperView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        InfiniteJumperView* parent_view = nullptr;

        HiddenController() {}
        HiddenController(const HiddenController&) = delete;
        HiddenController& operator=(const HiddenController&) = delete;

        bool on_key(const ui::KeyEvent event) override {
            if (parent_view && parent_view->game_status == 1) {
                parent_view->handle_game_key(event);
            }
            return true;
        }
        bool on_encoder(const ui::EncoderEvent event) override {
            if (parent_view && parent_view->game_status == 1) {
                parent_view->handle_game_encoder(event);
            }
            return true;
        }
    };

    struct Platform {
        bool active = true;
        float x = 0;
        float y = 0;
        float last_x = 0;
        float last_y = 0;
        int type = 0;
        int dir = 1;
        int hits_left = 0;
        bool last_active = false;
    };

    HiddenController hidden_pad;
    static const int max_platforms = 8;
    Platform platforms[max_platforms];

    float player_x = 0;
    float player_y = 0;
    float player_vy = 0;
    float gravity = 0.25f;
    float jump_strength = -6.5f;

    float last_player_x = 0;
    float last_player_y = 0;

    int score = 0;
    int last_score = 0;
    uint8_t game_status = 0;

    ui::Text text_score{{0, 0, 240, 16}};
    ui::Text text_status{{UI_POS_X_CENTER(10), 80, 10 * 8, 16}};
    ui::Text text_instructions{{UI_POS_X_CENTER(24), 110, 24 * 8, 16}};
    ui::Button button_start{{UI_POS_X_CENTER(10), 150, 10 * 8, 32}, "START"};

   public:
    InfiniteJumperView(ui::NavigationView& nav) : hidden_pad{} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        add_children({&text_score,
                      &text_status,
                      &text_instructions,
                      &button_start,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            button_start.hidden(true);
            text_status.hidden(true);
            text_instructions.hidden(true);
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_status.set("READY");
        text_instructions.set("Left/Right or Encoder");
        text_score.set("SCORE: 0");
    }

    ~InfiniteJumperView() {
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
        _api->fill_rectangle(0, 16, sw, sh - 16, ui::Color::black().v);
        if (game_status == 1 || game_status == 2) {
            for (int i = 0; i < max_platforms; i++) {
                if (platforms[i].active) {
                    ui::Color c = ui::Color::green();
                    if (platforms[i].type == 1)
                        c = ui::Color::blue();
                    else if (platforms[i].type == 2)
                        c = ui::Color::red();
                    safe_fill((int)platforms[i].x, (int)platforms[i].y + 16, 30, 6, c);
                }
            }
            safe_fill((int)player_x, (int)player_y + 16, 10, 10, ui::Color::cyan());
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            player_vy += gravity;
            player_y += player_vy;

            int sw = *_api->screen_width;
            int sh = *_api->screen_height - 16;

            if (player_x < 0) player_x = 0;
            if (player_x > sw - 10) player_x = sw - 10;

            if (player_vy > 0) {
                for (int i = 0; i < max_platforms; i++) {
                    if (platforms[i].active) {
                        if (player_x + 10 >= platforms[i].x && player_x <= platforms[i].x + 30) {
                            if (last_player_y + 10 <= platforms[i].y && player_y + 10 >= platforms[i].y) {
                                if (platforms[i].type == 2) {
                                    platforms[i].hits_left--;
                                    if (platforms[i].hits_left <= 0) {
                                        platforms[i].active = false;
                                        safe_fill((int)platforms[i].x, (int)platforms[i].y + 16, 30, 6, ui::Color::black());
                                    }
                                }
                                player_vy = jump_strength;
                                player_y = platforms[i].y - 10;
                                break;
                            }
                        }
                    }
                }
            }

            if (player_y < sh / 2) {
                float diff = (sh / 2) - player_y;
                player_y = sh / 2;
                score += (int)diff;

                for (int i = 0; i < max_platforms; i++) {
                    platforms[i].y += diff;
                    if (platforms[i].y > sh) {
                        platforms[i].active = true;
                        platforms[i].x = rand() % (sw - 30);
                        platforms[i].y = 0;
                        platforms[i].type = (rand() % 100 < 20) ? 1 : ((rand() % 100 < 15) ? 2 : 0);
                        platforms[i].dir = (rand() % 2 == 0) ? 1 : -1;
                        platforms[i].hits_left = (platforms[i].type == 2) ? 2 : 0;
                    }
                }
            }

            for (int i = 0; i < max_platforms; i++) {
                if (platforms[i].active && platforms[i].type == 1) {
                    platforms[i].x += platforms[i].dir * 0.8f;
                    if (platforms[i].x < 0) {
                        platforms[i].x = 0;
                        platforms[i].dir = 1;
                    }
                    if (platforms[i].x > sw - 30) {
                        platforms[i].x = sw - 30;
                        platforms[i].dir = -1;
                    }
                }
            }

            if (player_y > sh) {
                game_status = 2;
                button_start.set_text("RETRY");
                button_start.hidden(false);
                text_status.set("GAME OVER");
                text_status.hidden(false);
                button_start.focus();
                return;
            }

            if (score != last_score) {
                text_score.set("SCORE: " + to_string_dec_uint(score));
                last_score = score;
            }

            update_screen_elements();
        }
    }

    void handle_game_key(const ui::KeyEvent event) {
        if (event == ui::KeyEvent::Left) {
            player_x -= 8;
        } else if (event == ui::KeyEvent::Right) {
            player_x += 8;
        }
    }

    void handle_game_encoder(const ui::EncoderEvent event) {
        player_x += event * 4;
    }

   private:
    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 16) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 16) {
            h -= (16 - y);
            y = 16;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }

    void update_screen_elements() {
        for (int i = 0; i < max_platforms; i++) {
            if (platforms[i].last_active && (!platforms[i].active || platforms[i].x != platforms[i].last_x || platforms[i].y != platforms[i].last_y)) {
                safe_fill((int)platforms[i].last_x, (int)platforms[i].last_y + 16, 30, 6, ui::Color::black());
            }
        }
        for (int i = 0; i < max_platforms; i++) {
            if (platforms[i].active) {
                ui::Color c = ui::Color::green();
                if (platforms[i].type == 1)
                    c = ui::Color::blue();
                else if (platforms[i].type == 2)
                    c = ui::Color::red();
                safe_fill((int)platforms[i].x, (int)platforms[i].y + 16, 30, 6, c);
            }
            platforms[i].last_x = platforms[i].x;
            platforms[i].last_y = platforms[i].y;
            platforms[i].last_active = platforms[i].active;
        }

        if (player_x != last_player_x || player_y != last_player_y) {
            safe_fill((int)last_player_x, (int)last_player_y + 16, 10, 10, ui::Color::black());
        }
        safe_fill((int)player_x, (int)player_y + 16, 10, 10, ui::Color::cyan());
        last_player_x = player_x;
        last_player_y = player_y;
    }

    void start_game() {
        score = 0;
        last_score = 0;

        int sw = *_api->screen_width;
        int sh = *_api->screen_height - 16;

        player_x = sw / 2;
        player_y = sh - 60;
        player_vy = jump_strength;
        last_player_x = player_x;
        last_player_y = player_y;

        int spacing = sh / max_platforms;

        for (int i = 0; i < max_platforms; i++) {
            platforms[i].active = true;
            if (i == 0) {
                platforms[i].x = player_x - 10;
            } else {
                platforms[i].x = rand() % (sw - 30);
            }
            platforms[i].y = sh - (i * spacing) - 10;
            platforms[i].type = 0;
            platforms[i].dir = 1;
            platforms[i].hits_left = 0;
            platforms[i].last_x = platforms[i].x;
            platforms[i].last_y = platforms[i].y;
            platforms[i].last_active = true;
        }

        text_score.set("SCORE: 0");
        game_status = 1;

        _api->fill_rectangle(0, 16, sw, sh, ui::Color::black().v);
        update_screen_elements();
    }
};

}  // namespace ui