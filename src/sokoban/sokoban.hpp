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

class SokobanView : public ui::View {
   private:
    struct Pos {
        int x, y;
    };

    uint8_t grid[12][12];
    Pos boxes[4];
    Pos player;

    int level;
    bool won;

    ui::Text text_info;

   public:
    SokobanView(const SokobanView&) = delete;
    SokobanView& operator=(const SokobanView&) = delete;

    SokobanView(ui::NavigationView& nav) : level{1},
                                           won{false},
                                           text_info{{0, 16, 240, 16}} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);

        int sw = *_api->screen_width;
        text_info.set_parent_rect({0, 16, (ui::Dim)sw, 16});

        add_children({&text_info});

        generate();
        update_ui();
    }

    ~SokobanView() {
        ui::Theme::destroy();
    }

    bool on_key(const ui::KeyEvent event) override {
        if (event == ui::KeyEvent::Up)
            move(0, -1);
        else if (event == ui::KeyEvent::Down)
            move(0, 1);
        else if (event == ui::KeyEvent::Left)
            move(-1, 0);
        else if (event == ui::KeyEvent::Right)
            move(1, 0);
        else if (event == ui::KeyEvent::Select) {
            if (won) {
                level++;
            }
            generate();
            update_ui();
            set_dirty();
        }
        return true;
    }

    void paint(ui::Painter& painter) override {
        (void)painter;

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        _api->fill_rectangle(0, 48, sw, sh - 48, ui::Color::black().v);

        int avail_h = sh - 48;
        int cs = (sw < avail_h ? sw : avail_h) / 12;
        int off_x = (sw - (cs * 12)) / 2;
        int off_y = 48 + (avail_h - (cs * 12)) / 2;

        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 12; x++) {
                if (grid[y][x] == 1) {
                    _api->fill_rectangle(off_x + x * cs + 1, off_y + y * cs + 1, cs - 2, cs - 2, ui::Color::dark_grey().v);
                } else if (grid[y][x] == 2) {
                    int tgt_s = cs / 3;
                    int tgt_off = (cs - tgt_s) / 2;
                    _api->fill_rectangle(off_x + x * cs + tgt_off, off_y + y * cs + tgt_off, tgt_s, tgt_s, ui::Color::red().v);
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            ui::Color bc = ui::Color::orange();
            if (grid[boxes[i].y][boxes[i].x] == 2) {
                bc = ui::Color::green();
            }
            _api->fill_rectangle(off_x + boxes[i].x * cs + 1, off_y + boxes[i].y * cs + 1, cs - 2, cs - 2, bc.v);
            _api->fill_rectangle(off_x + boxes[i].x * cs + 3, off_y + boxes[i].y * cs + 3, cs - 6, cs - 6, ui::Color::black().v);

            int in_s = cs / 2;
            int in_off = (cs - in_s) / 2;
            _api->fill_rectangle(off_x + boxes[i].x * cs + in_off, off_y + boxes[i].y * cs + in_off, in_s, in_s, bc.v);
        }

        int p_s = cs - 4;
        int p_off = (cs - p_s) / 2;
        _api->fill_rectangle(off_x + player.x * cs + p_off, off_y + player.y * cs + p_off, p_s, p_s, ui::Color::cyan().v);
    }

   private:
    void update_ui() {
        if (won) {
            text_info.set("CLEARED! PRESS SELECT ->");
        } else {
            text_info.set("DUNGEON LEVEL: " + to_string_dec_int(level));
        }
    }

    bool check_win() {
        for (int i = 0; i < 4; i++) {
            if (grid[boxes[i].y][boxes[i].x] != 2) return false;
        }
        return true;
    }

    void move(int dx, int dy) {
        if (won) return;

        int nx = player.x + dx;
        int ny = player.y + dy;

        if (nx < 0 || nx >= 12 || ny < 0 || ny >= 12 || grid[ny][nx] == 1) return;

        int b_idx = -1;
        for (int i = 0; i < 4; i++) {
            if (boxes[i].x == nx && boxes[i].y == ny) {
                b_idx = i;
                break;
            }
        }

        if (b_idx != -1) {
            int nbx = nx + dx;
            int nby = ny + dy;

            if (nbx < 0 || nbx >= 12 || nby < 0 || nby >= 12 || grid[nby][nbx] == 1) return;

            for (int i = 0; i < 4; i++) {
                if (boxes[i].x == nbx && boxes[i].y == nby) return;
            }

            boxes[b_idx].x = nbx;
            boxes[b_idx].y = nby;
        }

        player.x = nx;
        player.y = ny;

        if (check_win()) {
            won = true;
        }

        update_ui();
        set_dirty();
    }

    void generate() {
        bool good = false;
        while (!good) {
            won = false;
            for (int y = 0; y < 12; y++) {
                for (int x = 0; x < 12; x++) {
                    if (x == 0 || x == 11 || y == 0 || y == 11)
                        grid[y][x] = 1;
                    else
                        grid[y][x] = 0;
                }
            }

            for (int i = 0; i < 15; i++) {
                grid[1 + rand() % 10][1 + rand() % 10] = 1;
            }

            for (int i = 0; i < 4; i++) {
                int bx, by;
                do {
                    bx = 1 + rand() % 10;
                    by = 1 + rand() % 10;
                } while (grid[by][bx] != 0);

                grid[by][bx] = 2;
                boxes[i] = {bx, by};
            }

            do {
                player.x = 1 + rand() % 10;
                player.y = 1 + rand() % 10;
            } while (grid[player.y][player.x] != 0);

            for (int i = 0; i < 2000; i++) {
                int dir = rand() % 4;
                int dx = 0, dy = 0;
                if (dir == 0)
                    dx = 1;
                else if (dir == 1)
                    dx = -1;
                else if (dir == 2)
                    dy = 1;
                else
                    dy = -1;

                int p_next_x = player.x + dx;
                int p_next_y = player.y + dy;

                if (p_next_x > 0 && p_next_x < 11 && p_next_y > 0 && p_next_y < 11 && grid[p_next_y][p_next_x] != 1) {
                    bool p_next_has_box = false;
                    for (int b = 0; b < 4; b++) {
                        if (boxes[b].x == p_next_x && boxes[b].y == p_next_y) {
                            p_next_has_box = true;
                            break;
                        }
                    }

                    if (!p_next_has_box) {
                        int bx = player.x - dx;
                        int by = player.y - dy;
                        int b_idx = -1;

                        for (int b = 0; b < 4; b++) {
                            if (boxes[b].x == bx && boxes[b].y == by) {
                                b_idx = b;
                                break;
                            }
                        }

                        if (b_idx != -1 && (rand() % 5 != 0)) {
                            boxes[b_idx].x = player.x;
                            boxes[b_idx].y = player.y;
                        }

                        player.x = p_next_x;
                        player.y = p_next_y;
                    }
                }
            }

            int boxes_on_target = 0;
            for (int i = 0; i < 4; i++) {
                if (grid[boxes[i].y][boxes[i].x] == 2) boxes_on_target++;
            }
            if (boxes_on_target == 0) {
                good = true;
            }
        }
    }
};

}  // namespace ui