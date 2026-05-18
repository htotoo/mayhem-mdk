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
            if (parent_view && parent_view->handle_game_key(event)) return true;
            return Button::on_key(event);
        }
        bool on_encoder(const ui::EncoderEvent event) override {
            if (parent_view && parent_view->handle_game_encoder(event)) return true;
            return Button::on_encoder(event);
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
        bool last_active = false;
    };

    HiddenController hidden_pad;
    static const int max_platforms = 14;
    Platform platforms[max_platforms];

    float player_x = 120;
    float player_y = 160;
    float last_player_x = 120;
    float last_player_y = 160;
    float player_vy = 0;
    float gravity = 0.35f;
    float jump_strength = -6.5f;

    uint32_t score = 0;
    uint32_t last_score = 0;
    uint8_t game_status = 0;

    ui::Text text_score{{8, 18, 14 * 8, 16}};
    ui::Text text_status{{UI_POS_X_CENTER(10), 120, 10 * 8, 16}};
    ui::Button button_start{{UI_POS_X_CENTER(10), 150, 10 * 8, 32}, "START"};

   public:
    InfiniteJumperView(ui::NavigationView& nav) : hidden_pad{}, platforms{} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        add_children({&text_score,
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
        text_status.set("READY");
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
        _api->fill_rectangle(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black().v);
        if (game_status == 1 || game_status == 2) {
            draw_entities();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            erase_entities();

            player_y += player_vy;
            player_vy += gravity;

            if (player_x < -10) player_x = *_api->screen_width;
            if (player_x > *_api->screen_width) player_x = -10;

            for (int i = 0; i < max_platforms; i++) {
                if (platforms[i].active && platforms[i].type == 1) {
                    platforms[i].x += platforms[i].dir * 2;
                    if (platforms[i].x < 0 || platforms[i].x > *_api->screen_width - 30) {
                        platforms[i].dir *= -1;
                    }
                }
            }

            if (player_vy > 0) {
                for (int i = 0; i < max_platforms; i++) {
                    if (platforms[i].active) {
                        if (player_y + 10 >= platforms[i].y && player_y + 10 <= platforms[i].y + 10) {
                            if (player_x + 10 > platforms[i].x && player_x < platforms[i].x + 30) {
                                player_vy = jump_strength;
                                if (platforms[i].type == 2) {
                                    platforms[i].active = false;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            int mid_y = 16 + (*_api->screen_height - 16) / 2;
            if (player_y < mid_y) {
                float diff = mid_y - player_y;
                player_y = mid_y;
                score += (int)diff;

                for (int i = 0; i < max_platforms; i++) {
                    platforms[i].y += diff;
                    if (platforms[i].y > *_api->screen_height) {
                        platforms[i].y = 16;
                        platforms[i].x = rand() % (*_api->screen_width - 30);

                        int r = rand() % 100;
                        if (r < 60)
                            platforms[i].type = 0;
                        else if (r < 85)
                            platforms[i].type = 1;
                        else
                            platforms[i].type = 2;

                        platforms[i].active = true;
                    }
                }
            }

            if (player_y > *_api->screen_height) {
                game_status = 2;
                button_start.set_text("RETRY");
                button_start.hidden(false);
                button_start.focus();
                text_status.set("GAME OVER");
            }

            if (game_status == 1) {
                draw_entities();
                if (score != last_score) {
                    text_score.set("SCORE: " + to_string_dec_uint(score));
                    last_score = score;
                }
            }
        }
    }

    bool handle_game_key(const ui::KeyEvent event) {
        if (game_status == 1) {
            if (event == ui::KeyEvent::Left) {
                player_x -= 14;
                return true;
            } else if (event == ui::KeyEvent::Right) {
                player_x += 14;
                return true;
            }
        }
        return false;
    }

    bool handle_game_encoder(const ui::EncoderEvent event) {
        if (game_status == 1) {
            player_x += event * 8;
            return true;
        }
        return false;
    }

   private:
    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        if (x >= *_api->screen_width || y >= *_api->screen_height || x + w <= 0 || y + h <= 16) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 16) {
            h -= (16 - y);
            y = 16;
        }
        if (x + w > *_api->screen_width) w = *_api->screen_width - x;
        if (y + h > *_api->screen_height) h = *_api->screen_height - y;
        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }

    void erase_entities() {
        safe_fill((int)last_player_x, (int)last_player_y, 10, 10, ui::Color::black());
        for (int i = 0; i < max_platforms; i++) {
            if (platforms[i].last_active) {
                safe_fill((int)platforms[i].last_x, (int)platforms[i].last_y, 30, 6, ui::Color::black());
            }
        }
    }

    void draw_entities() {
        for (int i = 0; i < max_platforms; i++) {
            if (platforms[i].active) {
                ui::Color c = ui::Color::green();
                if (platforms[i].type == 1)
                    c = ui::Color::blue();
                else if (platforms[i].type == 2)
                    c = ui::Color::red();
                safe_fill((int)platforms[i].x, (int)platforms[i].y, 30, 6, c);
            }
            platforms[i].last_x = platforms[i].x;
            platforms[i].last_y = platforms[i].y;
            platforms[i].last_active = platforms[i].active;
        }

        safe_fill((int)player_x, (int)player_y, 10, 10, ui::Color::cyan());
        last_player_x = player_x;
        last_player_y = player_y;
    }

    void start_game() {
        score = 0;
        last_score = 0;

        player_x = *_api->screen_width / 2;
        player_y = *_api->screen_height - 60;
        player_vy = jump_strength;
        last_player_x = player_x;
        last_player_y = player_y;

        int spacing = (*_api->screen_height - 16) / max_platforms;

        for (int i = 0; i < max_platforms; i++) {
            platforms[i].active = true;
            platforms[i].x = rand() % (*_api->screen_width - 30);
            platforms[i].y = *_api->screen_height - (i * spacing) - 10;
            platforms[i].type = 0;
            platforms[i].dir = (rand() % 2 == 0) ? 1 : -1;
            platforms[i].last_active = false;
        }

        platforms[0].x = player_x - 10;
        platforms[0].y = player_y + 20;

        text_score.set("SCORE: 0");
        game_status = 1;

        safe_fill(0, 16, *_api->screen_width, *_api->screen_height - 16, ui::Color::black());
    }
};

}  // namespace ui