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

class BlastZoneView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        BlastZoneView* parent_view = nullptr;

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

    static const int COLS = 11;
    static const int ROWS = 13;
    static const int MAX_BOMBS = 6;
    static const int MAX_FIRES = 64;

    struct Bomb {
        int r = 0;
        int c = 0;
        int timer = 0;
        int owner = 0;  // 0 = Játékos, 1..5 = Ellenfelek
        bool active = false;
    };

    struct Fire {
        int r = 0;
        int c = 0;
        int timer = 0;
        int owner_idx = 0;  // Megjegyzi ki robbantott
        bool active = false;
    };

    struct Enemy {
        int r = 0;
        int c = 0;
        int dir_r = 0;
        int dir_c = 0;
        int timer = 0;
        bool active = false;
    };

    HiddenController hidden_pad;

    uint8_t grid[ROWS][COLS];
    bool dirty[ROWS][COLS];
    Fire fires[MAX_FIRES];
    Bomb bombs[MAX_BOMBS];
    Enemy enemies[5];

    int player_r = 0;
    int player_c = 0;
    int player_cooldown = 0;

    uint32_t score = 0;
    uint32_t last_score = 0;
    uint8_t game_status = 0;
    int tick_counter = 0;

    ui::Text text_score{{8, 0, 14 * 8, 16}};
    ui::Text text_status{{UI_POS_X_CENTER(10), 60, 10 * 8, 16}};
    ui::Text text_instructions{{UI_POS_X_CENTER(28), 90, 28 * 8, 64}};
    ui::Button button_start{{UI_POS_X_CENTER(10), 170, 10 * 8, 32}, "START"};

   public:
    BlastZoneView(ui::NavigationView& nav) : hidden_pad{}, grid{}, dirty{}, fires{}, bombs{}, enemies{} {
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

        // Javított, pontosabb leírás angolul
        text_instructions.set(
            "Dodge and blast enemies.\n"
            "Use bombs to clear boxes.\n"
            "Walls block your path.\n"
            "Avoid all explosions!");

        text_score.set("SCORE: 0");
    }

    ~BlastZoneView() {
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
        _api->fill_rectangle(0, 32, sw, sh - 32, ui::Color::black().v);
        if (game_status == 1 || game_status == 2 || game_status == 3) {
            for (int r = 0; r < ROWS; r++) {
                for (int c = 0; c < COLS; c++) {
                    dirty[r][c] = true;
                }
            }
            draw_dirty_cells();
        }
    }

    void on_framesync() override {
        if (game_status == 1) {
            tick_counter++;

            if (player_cooldown > 0) {
                player_cooldown--;
            }

            dirty[player_r][player_c] = true;

            if (tick_counter % 5 == 0) {
                for (int i = 0; i < MAX_FIRES; i++) {
                    if (fires[i].active) {
                        fires[i].timer--;
                        if (fires[i].timer <= 0) {
                            fires[i].active = false;
                            dirty[fires[i].r][fires[i].c] = true;
                        } else {
                            if (player_r == fires[i].r && player_c == fires[i].c) {
                                trigger_game_over();
                                return;
                            }
                            for (int e = 0; e < 5; e++) {
                                if (enemies[e].active && enemies[e].r == fires[i].r && enemies[e].c == fires[i].c) {
                                    enemies[e].active = false;
                                    dirty[enemies[e].r][enemies[e].c] = true;
                                    // CSAK akkor kapsz pontot, ha a tüzet te (0-s index) okoztad!
                                    if (fires[i].owner_idx == 0) {
                                        score += 100;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (tick_counter % 10 == 0) {
                for (int b = 0; b < MAX_BOMBS; b++) {
                    if (bombs[b].active) {
                        bombs[b].timer--;
                        dirty[bombs[b].r][bombs[b].c] = true;
                        if (bombs[b].timer <= 0) {
                            explode_bomb(b);
                        }
                    }
                }
            }

            if (tick_counter % 35 == 0) {
                bool any_enemy_alive = false;

                for (int e = 0; e < 5; e++) {
                    if (enemies[e].active) {
                        any_enemy_alive = true;

                        bool curr_danger = is_dangerous(enemies[e].r, enemies[e].c);

                        int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                        int valid_moves[4];
                        int valid_count = 0;
                        int safe_moves[4];
                        int safe_count = 0;

                        for (int d = 0; d < 4; d++) {
                            int tr = enemies[e].r + dirs[d][0];
                            int tc = enemies[e].c + dirs[d][1];
                            if (is_valid_move(tr, tc)) {
                                valid_moves[valid_count++] = d;
                                if (!is_dangerous(tr, tc)) {
                                    safe_moves[safe_count++] = d;
                                }
                            }
                        }

                        int nr = enemies[e].r + enemies[e].dir_r;
                        int nc = enemies[e].c + enemies[e].dir_c;
                        bool forward_valid = is_valid_move(nr, nc);
                        bool forward_safe = forward_valid && !is_dangerous(nr, nc);

                        dirty[enemies[e].r][enemies[e].c] = true;

                        if (curr_danger || !forward_safe) {
                            if (safe_count > 0) {
                                int pick = safe_moves[rand() % safe_count];
                                enemies[e].dir_r = dirs[pick][0];
                                enemies[e].dir_c = dirs[pick][1];
                            } else if (valid_count > 0) {
                                int pick = valid_moves[rand() % valid_count];
                                enemies[e].dir_r = dirs[pick][0];
                                enemies[e].dir_c = dirs[pick][1];
                            } else {
                                enemies[e].dir_r = 0;
                                enemies[e].dir_c = 0;
                            }
                        } else if (rand() % 100 < 20) {
                            if (safe_count > 0) {
                                int pick = safe_moves[rand() % safe_count];
                                enemies[e].dir_r = dirs[pick][0];
                                enemies[e].dir_c = dirs[pick][1];
                            }
                        }

                        if (enemies[e].dir_r != 0 || enemies[e].dir_c != 0) {
                            enemies[e].r += enemies[e].dir_r;
                            enemies[e].c += enemies[e].dir_c;
                        }

                        dirty[enemies[e].r][enemies[e].c] = true;

                        if (player_r == enemies[e].r && player_c == enemies[e].c) {
                            trigger_game_over();
                            return;
                        }

                        if (!curr_danger && !bombs[e + 1].active && (rand() % 100 < 15)) {
                            bool near_box = false;
                            for (int d = 0; d < 4; d++) {
                                int tr = enemies[e].r + dirs[d][0];
                                int tc = enemies[e].c + dirs[d][1];
                                if (tr >= 0 && tr < ROWS && tc >= 0 && tc < COLS && grid[tr][tc] == 2) {
                                    near_box = true;
                                    break;
                                }
                            }
                            if (near_box) {
                                bombs[e + 1].r = enemies[e].r;
                                bombs[e + 1].c = enemies[e].c;
                                bombs[e + 1].timer = 15;
                                bombs[e + 1].owner = e + 1;  // Beállítjuk az AI tulajdonost
                                bombs[e + 1].active = true;
                                dirty[enemies[e].r][enemies[e].c] = true;
                            }
                        }
                    }
                }

                if (!any_enemy_alive) {
                    trigger_game_win();
                    return;
                }
            }

            if (game_status == 1) {
                for (int e = 0; e < 5; e++) {
                    if (enemies[e].active && player_r == enemies[e].r && player_c == enemies[e].c) {
                        trigger_game_over();
                        return;
                    }
                }

                draw_dirty_cells();
                if (score != last_score) {
                    text_score.set("SCORE: " + to_string_dec_uint(score));
                    last_score = score;
                }
            }
        }
    }

    void handle_game_key(const ui::KeyEvent event) {
        int dr = 0;
        int dc = 0;

        if (event == ui::KeyEvent::Up)
            dr = -1;
        else if (event == ui::KeyEvent::Down)
            dr = 1;
        else if (event == ui::KeyEvent::Left)
            dc = -1;
        else if (event == ui::KeyEvent::Right)
            dc = 1;
        else if (event == ui::KeyEvent::Select) {
            if (!bombs[0].active) {
                if (!is_enemy_at(player_r, player_c)) {
                    bombs[0].r = player_r;
                    bombs[0].c = player_c;
                    bombs[0].timer = 15;
                    bombs[0].owner = 0;  // Játékosé a bomba
                    bombs[0].active = true;
                    dirty[player_r][player_c] = true;
                }
            }
            return;
        }

        if ((dr != 0 || dc != 0) && player_cooldown == 0) {
            int nr = player_r + dr;
            int nc = player_c + dc;

            if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
                if (is_enemy_at(nr, nc)) {
                    trigger_game_over();
                    return;
                }

                if (is_valid_move(nr, nc)) {
                    dirty[player_r][player_c] = true;
                    player_r = nr;
                    player_c = nc;
                    dirty[player_r][player_c] = true;
                    player_cooldown = 10;
                }
            }
        }
    }

    void handle_game_encoder(const ui::EncoderEvent event) {
        (void)event;
    }

   private:
    bool is_enemy_at(int r, int c) {
        for (int e = 0; e < 5; e++) {
            if (enemies[e].active && enemies[e].r == r && enemies[e].c == c) {
                return true;
            }
        }
        return false;
    }

    bool is_dangerous(int r, int c) {
        for (int f = 0; f < MAX_FIRES; f++) {
            if (fires[f].active && fires[f].r == r && fires[f].c == c) return true;
        }
        for (int b = 0; b < MAX_BOMBS; b++) {
            if (!bombs[b].active) continue;
            if (bombs[b].r == r && bombs[b].c == c) return true;

            if (bombs[b].r == r) {
                int dist = c - bombs[b].c;
                if (dist < 0) dist = -dist;
                if (dist <= 2) {
                    bool wall = false;
                    int min_c = c < bombs[b].c ? c : bombs[b].c;
                    int max_c = c > bombs[b].c ? c : bombs[b].c;
                    for (int i = min_c + 1; i < max_c; i++) {
                        if (grid[r][i] == 1 || grid[r][i] == 2) wall = true;
                    }
                    if (!wall) return true;
                }
            }
            if (bombs[b].c == c) {
                int dist = r - bombs[b].r;
                if (dist < 0) dist = -dist;
                if (dist <= 2) {
                    bool wall = false;
                    int min_r = r < bombs[b].r ? r : bombs[b].r;
                    int max_r = r > bombs[b].r ? r : bombs[b].r;
                    for (int i = min_r + 1; i < max_r; i++) {
                        if (grid[i][c] == 1 || grid[i][c] == 2) wall = true;
                    }
                    if (!wall) return true;
                }
            }
        }
        return false;
    }

    bool is_valid_move(int r, int c) {
        if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return false;
        if (grid[r][c] != 0) return false;
        for (int b = 0; b < MAX_BOMBS; b++) {
            if (bombs[b].active && bombs[b].r == r && bombs[b].c == c) return false;
        }
        return true;
    }

    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 32) return;
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 32) {
            h -= (32 - y);
            y = 32;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;
        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }

    void spawn_fire(int r, int c, int owner_idx) {
        dirty[r][c] = true;
        for (int i = 0; i < MAX_FIRES; i++) {
            if (!fires[i].active) {
                fires[i].r = r;
                fires[i].c = c;
                fires[i].timer = 6;
                fires[i].owner_idx = owner_idx;  // Átadjuk kié a tűz
                fires[i].active = true;
                break;
            }
        }
    }

    void explode_bomb(int b_idx) {
        bombs[b_idx].active = false;
        int attacker = bombs[b_idx].owner;  // Ki robbantott?
        spawn_fire(bombs[b_idx].r, bombs[b_idx].c, attacker);

        int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

        for (int d = 0; d < 4; d++) {
            for (int step = 1; step <= 2; step++) {
                int nr = bombs[b_idx].r + dirs[d][0] * step;
                int nc = bombs[b_idx].c + dirs[d][1] * step;

                if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) break;
                if (grid[nr][nc] == 1) break;
                if (grid[nr][nc] == 2) {
                    grid[nr][nc] = 0;
                    // Csak akkor kapsz pontot a dobozért, ha Te lőtted ki!
                    if (attacker == 0) {
                        score += 10;
                    }
                    spawn_fire(nr, nc, attacker);
                    break;
                }
                spawn_fire(nr, nc, attacker);
            }
        }
    }

    void draw_dirty_cells() {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        int cell_w = sw / COLS;
        int cell_h = (sh - 32) / ROWS;
        int offset_x = (sw - (COLS * cell_w)) / 2;
        int offset_y = 32 + ((sh - 32) - (ROWS * cell_h)) / 2;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (dirty[r][c]) {
                    dirty[r][c] = false;

                    int px = offset_x + c * cell_w;
                    int py = offset_y + r * cell_h;

                    safe_fill(px, py, cell_w, cell_h, ui::Color::black());

                    if (grid[r][c] == 1) {
                        safe_fill(px + 1, py + 1, cell_w - 2, cell_h - 2, ui::Color::grey());
                    } else if (grid[r][c] == 2) {
                        safe_fill(px + 1, py + 1, cell_w - 2, cell_h - 2, ui::Color::orange());
                    }

                    for (int b = 0; b < MAX_BOMBS; b++) {
                        if (bombs[b].active && bombs[b].r == r && bombs[b].c == c) {
                            ui::Color bc = (bombs[b].timer % 2 == 0) ? ui::Color::red() : ui::Color::white();
                            safe_fill(px + 2, py + 2, cell_w - 4, cell_h - 4, bc);
                        }
                    }

                    for (int i = 0; i < MAX_FIRES; i++) {
                        if (fires[i].active && fires[i].r == r && fires[i].c == c) {
                            safe_fill(px + 1, py + 1, cell_w - 2, cell_h - 2, ui::Color::yellow());
                            safe_fill(px + 3, py + 3, cell_w - 6, cell_h - 6, ui::Color::red());
                        }
                    }

                    for (int e = 0; e < 5; e++) {
                        if (enemies[e].active && enemies[e].r == r && enemies[e].c == c) {
                            safe_fill(px + 4, py + 4, cell_w - 8, cell_h - 8, ui::Color::green());
                        }
                    }

                    if (player_r == r && player_c == c) {
                        safe_fill(px + 5, py + 5, cell_w - 10, cell_h - 10, ui::Color::cyan());
                    }
                }
            }
        }
    }

    void trigger_game_over() {
        game_status = 2;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) dirty[r][c] = true;
        }
        draw_dirty_cells();

        button_start.set_text("RETRY");
        button_start.hidden(false);
        text_status.set("GAME OVER");
        text_status.hidden(false);
        text_instructions.hidden(true);
        button_start.focus();
    }

    void trigger_game_win() {
        game_status = 3;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) dirty[r][c] = true;
        }
        draw_dirty_cells();

        button_start.set_text("NEXT LEVEL");
        button_start.hidden(false);
        text_status.set("VICTORY!");
        text_status.hidden(false);
        text_instructions.hidden(true);
        button_start.focus();
    }

    void start_game() {
        score = 0;
        last_score = 0;
        tick_counter = 0;
        player_cooldown = 0;

        player_r = 0;
        player_c = 0;

        for (int b = 0; b < MAX_BOMBS; b++) bombs[b].active = false;
        for (int i = 0; i < MAX_FIRES; i++) fires[i].active = false;
        for (int e = 0; e < 5; e++) enemies[e].active = false;

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                dirty[r][c] = true;
                if (r % 2 == 1 && c % 2 == 1) {
                    grid[r][c] = 1;
                } else if (r + c > 2 && (rand() % 100) < 40) {
                    grid[r][c] = 2;
                } else {
                    grid[r][c] = 0;
                }
            }
        }

        int e_count = 0;
        while (e_count < 4) {
            int er = rand() % ROWS;
            int ec = rand() % COLS;
            if (grid[er][ec] == 0 && (er + ec > 5)) {
                enemies[e_count].r = er;
                enemies[e_count].c = ec;
                enemies[e_count].dir_r = 1;
                enemies[e_count].dir_c = 0;
                enemies[e_count].active = true;
                e_count++;
            }
        }

        text_score.set("SCORE: 0");
        game_status = 1;

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        _api->fill_rectangle(0, 32, sw, sh - 32, ui::Color::black().v);

        draw_dirty_cells();
    }
};

}  // namespace ui