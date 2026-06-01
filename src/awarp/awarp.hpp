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

class AWarpView : public ui::View {
   private:
    struct Star {
        float x, y, z;
        int px1, py1, px2, py2;
        bool has_prev;
    };

    static const int NUM_STARS = 120;
    Star stars[NUM_STARS];

    float speed;
    float streak_length;

   public:
    AWarpView(const AWarpView&) = delete;
    AWarpView& operator=(const AWarpView&) = delete;

    AWarpView(ui::NavigationView& nav) : stars{},
                                         speed{12.0f},
                                         streak_length{3.0f} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        for (int i = 0; i < NUM_STARS; i++) {
            reset_star(i, true);
        }
        set_focusable(true);
    }

    ~AWarpView() {
        ui::Theme::destroy();
    }

    bool on_key(const ui::KeyEvent) override {
        _api->exit_app();
        return true;
    }

    bool on_encoder(const ui::EncoderEvent) override {
        _api->exit_app();
        return true;
    }

    bool on_touch(const ui::TouchEvent) override {
        _api->exit_app();
        return true;
    }

    void paint(ui::Painter& painter) override {
        (void)painter;

        _api->fill_rectangle(0, 0, *_api->screen_width, *_api->screen_height, ui::Color::black().v);
    }

    void on_framesync() override {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        int cx = sw / 2;
        int cy = sh / 2;
        float fov = 120.0f;

        for (int i = 0; i < NUM_STARS; i++) {
            if (stars[i].has_prev) {
                draw_line(stars[i].px1, stars[i].py1, stars[i].px2, stars[i].py2, ui::Color::black());
            }

            stars[i].z -= speed;

            if (stars[i].z <= 1.0f) {
                reset_star(i, false);
            }

            float z1 = stars[i].z;
            float z2 = stars[i].z + (speed * streak_length);

            int nx1 = (int)((stars[i].x * fov) / z1) + cx;
            int ny1 = (int)((stars[i].y * fov) / z1) + cy;

            int nx2 = (int)((stars[i].x * fov) / z2) + cx;
            int ny2 = (int)((stars[i].y * fov) / z2) + cy;

            if ((nx1 < 0 || nx1 > sw || ny1 < 0 || ny1 > sh) &&
                (nx2 < 0 || nx2 > sw || ny2 < 0 || ny2 > sh)) {
                reset_star(i, false);
                continue;
            }

            ui::Color c;
            if (z1 < 100.0f)
                c = ui::Color::white();
            else if (z1 < 300.0f)
                c = ui::Color::cyan();
            else if (z1 < 600.0f)
                c = ui::Color::blue();
            else
                c = ui::Color::dark_grey();

            draw_line(nx1, ny1, nx2, ny2, c);

            stars[i].px1 = nx1;
            stars[i].py1 = ny1;
            stars[i].px2 = nx2;
            stars[i].py2 = ny2;
            stars[i].has_prev = true;
        }
    }

   private:
    void reset_star(int idx, bool full_random_z) {
        stars[idx].x = (float)((rand() % 2000) - 1000);
        stars[idx].y = (float)((rand() % 2000) - 1000);

        if (full_random_z) {
            stars[idx].z = (float)((rand() % 800) + 10);
        } else {
            stars[idx].z = 800.0f + (float)(rand() % 200);
        }

        stars[idx].has_prev = false;
    }

    void draw_line(int x0, int y0, int x1, int y1, ui::Color c) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            safe_fill(x0, y0, 1, 1, c);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 0) return;

        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h -= (0 - y);
            y = 0;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;

        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }
};

}  // namespace ui