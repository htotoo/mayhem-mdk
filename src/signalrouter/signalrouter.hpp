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

class SignalRouterView : public ui::View {
   private:
    class HiddenController : public ui::Button {
       public:
        SignalRouterView* parent_view = nullptr;

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

    struct Cell {
        int type = 0;
        int rot = 0;
        bool filled = false;
    };

    HiddenController hidden_pad;
    Cell grid[6][8];

    int cursor_x;
    int cursor_y;

    int signal_x;
    int signal_y;
    int signal_dir;
    int initial_signal_dir;

    uint32_t timer_frames;
    int flow_tick;
    uint8_t game_status;
    uint32_t level;
    uint32_t entropy_timer;

    ui::Text text_title{{0, 0, 0, 0}};
    ui::Text text_info{{0, 0, 0, 0}};
    ui::Text text_status{{0, 0, 0, 0}};
    ui::Button button_start{{0, 0, 0, 0}, "BOOT OS"};

    ui::Text text_inst1{{0, 0, 0, 0}};
    ui::Text text_inst2{{0, 0, 0, 0}};
    ui::Text text_inst3{{0, 0, 0, 0}};

   public:
    SignalRouterView(ui::NavigationView& nav) : hidden_pad{},
                                                grid{},
                                                cursor_x{3},
                                                cursor_y{3},
                                                signal_x{0},
                                                signal_y{0},
                                                signal_dir{2},
                                                initial_signal_dir{2},
                                                timer_frames{900},
                                                flow_tick{0},
                                                game_status{0},
                                                level{1},
                                                entropy_timer{0} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        hidden_pad.parent_view = this;
        hidden_pad.set_parent_rect({500, 500, 10, 10});

        int sw = *_api->screen_width;

        text_title.set_parent_rect({(ui::Coord)((sw - 13 * 8) / 2), 4, (ui::Dim)(13 * 8), 16});
        text_info.set_parent_rect({(ui::Coord)((sw - 220) / 2), 20, 220, 16});
        text_status.set_parent_rect({(ui::Coord)((sw - 16 * 8) / 2), 140, (ui::Dim)(16 * 8), 16});
        button_start.set_parent_rect({(ui::Coord)((sw - 12 * 8) / 2), 170, (ui::Dim)(12 * 8), 32});

        text_inst1.set_parent_rect({(ui::Coord)((sw - 18 * 8) / 2), 60, (ui::Dim)(18 * 8), 16});
        text_inst2.set_parent_rect({(ui::Coord)((sw - 22 * 8) / 2), 80, (ui::Dim)(22 * 8), 16});
        text_inst3.set_parent_rect({(ui::Coord)((sw - 27 * 8) / 2), 100, (ui::Dim)(27 * 8), 16});

        add_children({&text_title,
                      &text_info,
                      &text_status,
                      &button_start,
                      &text_inst1,
                      &text_inst2,
                      &text_inst3,
                      &hidden_pad});

        button_start.on_select = [this](ui::Button&) {
            srand(entropy_timer);
            button_start.hidden(true);
            text_inst1.hidden(true);
            text_inst2.hidden(true);
            text_inst3.hidden(true);
            text_status.hidden(true);
            start_game();
            hidden_pad.focus();
        };

        game_status = 0;
        text_title.set("SIGNAL ROUTER");
        text_status.set("SYSTEM OFFLINE");
        text_info.set("");

        text_inst1.set("D-PAD: MOVE CURSOR");
        text_inst2.set("SELECT/ENCODER: ROTATE");
        text_inst3.set("CONNECT TX(RED) TO RX(BLUE)");
    }

    ~SignalRouterView() {
        ui::Theme::destroy();
    }

    void focus() override {
        if (game_status == 1 || game_status == 2) {
            hidden_pad.focus();
        } else {
            button_start.focus();
        }
    }

    void paint(ui::Painter& painter) override {
        (void)painter;
        _api->fill_rectangle(0, 56, *_api->screen_width, *_api->screen_height - 56, ui::Color::dark_grey().v);
        if (game_status != 0) {
            draw_full_grid();
        }
    }

    void on_framesync() override {
        entropy_timer++;

        if (game_status == 1) {
            if (timer_frames > 0) {
                timer_frames--;
                if (timer_frames % 60 == 0) {
                    text_info.set("LVL " + to_string_dec_uint(level) + " | TX DELAY: " + to_string_dec_uint(timer_frames / 60) + "s");
                }
            } else {
                game_status = 2;
                text_info.set("STATUS: TRANSMITTING...");
                flow_tick = 0;
            }
        } else if (game_status == 2) {
            flow_tick++;
            int current_speed = 90 - (level - 1) * 10;
            if (current_speed < 10) current_speed = 10;

            if (flow_tick >= current_speed) {
                flow_tick = 0;
                advance_signal();
            }
        }
    }

    bool handle_game_key(const ui::KeyEvent event) {
        if (game_status == 1 || game_status == 2) {
            int old_x = cursor_x;
            int old_y = cursor_y;
            bool moved = false;
            bool rotated = false;

            if (event == ui::KeyEvent::Left && cursor_x > 0) {
                cursor_x--;
                moved = true;
            } else if (event == ui::KeyEvent::Right && cursor_x < 5) {
                cursor_x++;
                moved = true;
            } else if (event == ui::KeyEvent::Up && cursor_y > 0) {
                cursor_y--;
                moved = true;
            } else if (event == ui::KeyEvent::Down && cursor_y < 7) {
                cursor_y++;
                moved = true;
            } else if (event == ui::KeyEvent::Select) {
                if (!grid[cursor_x][cursor_y].filled && grid[cursor_x][cursor_y].type != 3 && grid[cursor_x][cursor_y].type != 4) {
                    grid[cursor_x][cursor_y].rot = (grid[cursor_x][cursor_y].rot + 1) % 4;
                    rotated = true;
                }
            }

            if (moved) {
                draw_cell(old_x, old_y);
                draw_cell(cursor_x, cursor_y);
                draw_cursor();
                return true;
            } else if (rotated) {
                draw_cell(cursor_x, cursor_y);
                draw_cursor();
                return true;
            }
        }
        return false;
    }

    bool handle_game_encoder(const ui::EncoderEvent event) {
        if (game_status == 1 || game_status == 2) {
            if (!grid[cursor_x][cursor_y].filled && grid[cursor_x][cursor_y].type != 3 && grid[cursor_x][cursor_y].type != 4) {
                if (event > 0) {
                    grid[cursor_x][cursor_y].rot = (grid[cursor_x][cursor_y].rot + 1) % 4;
                } else if (event < 0) {
                    grid[cursor_x][cursor_y].rot = (grid[cursor_x][cursor_y].rot + 3) % 4;
                }
                draw_cell(cursor_x, cursor_y);
                draw_cursor();
            }
            return true;
        }
        return false;
    }

   private:
    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int phys_y = y + 16;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        if (x >= sw || phys_y >= sh || x + w <= 0 || phys_y + h <= 16) return;

        if (x < 0) {
            w += x;
            x = 0;
        }
        if (phys_y < 16) {
            h -= (16 - phys_y);
            phys_y = 16;
        }
        if (x + w > sw) w = sw - x;
        if (phys_y + h > sh) h = sh - phys_y;

        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, phys_y, w, h, c.v);
        }
    }

    bool connects(int type, int rot, int dir) {
        if (type == 1) {
            if (rot % 2 == 0)
                return (dir == 1 || dir == 3);
            else
                return (dir == 0 || dir == 2);
        } else if (type == 2) {
            if (rot == 0) return (dir == 0 || dir == 1);
            if (rot == 1) return (dir == 1 || dir == 2);
            if (rot == 2) return (dir == 2 || dir == 3);
            if (rot == 3) return (dir == 3 || dir == 0);
        } else if (type == 3) {
            return (dir == 2 || dir == 1);
        } else if (type == 4) {
            return true;
        }
        return false;
    }

    void advance_signal() {
        int dx[4] = {0, 1, 0, -1};
        int dy[4] = {-1, 0, 1, 0};

        if (grid[signal_x][signal_y].type == 4) {
            game_status = 3;
            level++;
            text_info.set("STATUS: SUCCESS");
            text_status.set("LEVEL " + to_string_dec_uint(level - 1) + " CLEARED!");
            text_status.hidden(false);
            button_start.set_text("NEXT LEVEL");
            button_start.hidden(false);
            button_start.focus();
            return;
        }

        int nx = signal_x + dx[signal_dir];
        int ny = signal_y + dy[signal_dir];

        if (nx < 0 || nx > 5 || ny < 0 || ny > 7) {
            crash();
            return;
        }

        int opposite_dir = (signal_dir + 2) % 4;

        if (!connects(grid[nx][ny].type, grid[nx][ny].rot, opposite_dir)) {
            crash();
            return;
        }

        grid[nx][ny].filled = true;
        draw_cell(nx, ny);

        if (nx == cursor_x && ny == cursor_y) {
            draw_cursor();
        }

        signal_x = nx;
        signal_y = ny;

        if (grid[nx][ny].type == 4) return;

        bool found_out = false;
        for (int d = 0; d < 4; d++) {
            if (d != opposite_dir && connects(grid[nx][ny].type, grid[nx][ny].rot, d)) {
                signal_dir = d;
                found_out = true;
                break;
            }
        }

        if (!found_out) crash();
    }

    void crash() {
        game_status = 4;
        text_info.set("STATUS: FAILED");
        text_status.set("CONNECTION LOST!");
        text_status.hidden(false);
        button_start.set_text("RETRY");
        button_start.hidden(false);
        button_start.focus();
    }

    void draw_cell(int x, int y) {
        int start_x = (*_api->screen_width - 180) / 2;
        int gx = start_x + x * 30;
        int gy = 40 + y * 30;

        safe_fill(gx - 2, gy - 2, 32, 32, ui::Color::dark_grey());
        safe_fill(gx, gy, 28, 28, ui::Color::black());

        Cell c = grid[x][y];
        ui::Color wc = c.filled ? ui::Color::green() : ui::Color::orange();

        if (c.type == 3) {
            safe_fill(gx + 4, gy + 4, 20, 20, ui::Color::red());
            if (initial_signal_dir == 1) {
                safe_fill(gx + 24, gy + 12, 4, 4, wc);
            } else {
                safe_fill(gx + 12, gy + 24, 4, 4, wc);
            }
        } else if (c.type == 4) {
            safe_fill(gx + 4, gy + 4, 20, 20, ui::Color::blue());
            if (c.filled) safe_fill(gx + 8, gy + 8, 12, 12, ui::Color::green());
        } else if (c.type == 1) {
            if (c.rot % 2 == 0)
                safe_fill(gx, gy + 12, 28, 4, wc);
            else
                safe_fill(gx + 12, gy, 4, 28, wc);
        } else if (c.type == 2) {
            if (c.rot == 0) {
                safe_fill(gx + 12, gy, 4, 16, wc);
                safe_fill(gx + 12, gy + 12, 16, 4, wc);
            }
            if (c.rot == 1) {
                safe_fill(gx + 12, gy + 12, 16, 4, wc);
                safe_fill(gx + 12, gy + 12, 4, 16, wc);
            }
            if (c.rot == 2) {
                safe_fill(gx + 12, gy + 12, 4, 16, wc);
                safe_fill(gx, gy + 12, 16, 4, wc);
            }
            if (c.rot == 3) {
                safe_fill(gx, gy + 12, 16, 4, wc);
                safe_fill(gx + 12, gy, 4, 16, wc);
            }
        }
    }

    void draw_cursor() {
        int start_x = (*_api->screen_width - 180) / 2;
        int gx = start_x + cursor_x * 30;
        int gy = 40 + cursor_y * 30;
        ui::Color c = ui::Color::white();
        safe_fill(gx - 2, gy - 2, 32, 2, c);
        safe_fill(gx - 2, gy + 28, 32, 2, c);
        safe_fill(gx - 2, gy - 2, 2, 32, c);
        safe_fill(gx + 28, gy - 2, 2, 32, c);
    }

    void draw_full_grid() {
        _api->fill_rectangle(0, 56, *_api->screen_width, *_api->screen_height - 56, ui::Color::dark_grey().v);
        for (int x = 0; x < 6; x++) {
            for (int y = 0; y < 8; y++) {
                draw_cell(x, y);
            }
        }
        draw_cursor();
    }

    void start_game() {
        if (game_status == 0 || game_status == 4) {
            level = 1;
        }

        _api->fill_rectangle(0, 56, *_api->screen_width, *_api->screen_height - 56, ui::Color::dark_grey().v);

        for (int x = 0; x < 6; x++) {
            for (int y = 0; y < 8; y++) {
                grid[x][y].type = (rand() % 10 < 6) ? 1 : 2;
                grid[x][y].rot = rand() % 4;
                grid[x][y].filled = false;
            }
        }

        int moves[12];
        for (int i = 0; i < 5; i++) moves[i] = 1;
        for (int i = 5; i < 12; i++) moves[i] = 2;

        for (int i = 0; i < 12; i++) {
            int swap_idx = i + (rand() % (12 - i));
            int temp = moves[i];
            moves[i] = moves[swap_idx];
            moves[swap_idx] = temp;
        }

        int px = 0;
        int py = 0;
        int current_dir = moves[0];

        for (int i = 0; i < 11; i++) {
            if (current_dir == 1)
                px++;
            else if (current_dir == 2)
                py++;

            int next_dir = moves[i + 1];
            if (current_dir == next_dir) {
                grid[px][py].type = 1;
            } else {
                grid[px][py].type = 2;
            }
            grid[px][py].rot = rand() % 4;
            current_dir = next_dir;
        }

        grid[0][0].type = 3;
        grid[0][0].rot = 0;
        grid[0][0].filled = true;

        grid[5][7].type = 4;
        grid[5][7].rot = 0;
        grid[5][7].filled = false;

        cursor_x = 3;
        cursor_y = 3;
        signal_x = 0;
        signal_y = 0;

        signal_dir = moves[0];
        initial_signal_dir = signal_dir;

        int initial_prep = 900 - (level - 1) * 60;
        if (initial_prep < 120) initial_prep = 120;
        timer_frames = initial_prep;

        flow_tick = 0;
        game_status = 1;
        draw_full_grid();
    }
};

}  // namespace ui