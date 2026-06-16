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

class ACyberSeaView : public ui::View {
   private:
    struct Point2D {
        int x, y, size;
        ui::Color c;
    };

    static const int GRID_X = 26;
    static const int GRID_Z = 24;
    static const int NUM_POINTS = GRID_X * GRID_Z;

    Point2D prev_pts[NUM_POINTS];

    float wave_time;
    float speed;

   public:
    ACyberSeaView(const ACyberSeaView&) = delete;
    ACyberSeaView& operator=(const ACyberSeaView&) = delete;

    ACyberSeaView(ui::NavigationView& nav) : prev_pts{},
                                             wave_time{0.0f},
                                             speed{0.18f} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);

        for (int i = 0; i < NUM_POINTS; i++) {
            prev_pts[i] = {0, 0, 0, ui::Color::black()};
        }
    }

    ~ACyberSeaView() {
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
        return false;
    }

    void paint(ui::Painter& painter) override {
        (void)painter;
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        _api->fill_rectangle(0, 0, sw, sh, ui::Color::black().v);

        for (int i = 0; i < 30; i++) {
            int sx = rand() % sw;
            int sy = rand() % 100;
            _api->fill_rectangle(sx, sy, 1, 1, ui::Color::white().v);
        }

        int cx = sw / 2;
        for (int y = -50; y <= 50; y += 2) {
            if (y > 10 && y < 14) continue;
            if (y > 20 && y < 26) continue;
            if (y > 34 && y < 42) continue;
            if (y > 46) continue;

            int val = 50 * 50 - y * y;
            if (val < 0) val = 0;
            int w = (int)(sqrt(val)) * 2;
            int x = cx - w / 2;

            ui::Color c;
            if (y < -20)
                c = ui::Color::yellow();
            else if (y < 10)
                c = ui::Color::orange();
            else
                c = ui::Color::red();

            _api->fill_rectangle(x, 100 + y, w, 2, c.v);
        }

        _api->fill_rectangle(0, 155, sw, 2, ui::Color::magenta().v);
    }

    void on_framesync() override {
        for (int i = 0; i < NUM_POINTS; i++) {
            if (prev_pts[i].size > 0) {
                safe_fill(prev_pts[i].x, prev_pts[i].y,
                          prev_pts[i].size, prev_pts[i].size, ui::Color::black());
            }
        }

        wave_time += speed;

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        float fov = 200.0f;

        float cam_y = 15.0f;

        int idx = 0;
        for (int zi = 0; zi < GRID_Z; zi++) {
            float bz = 8.0f + (zi * 3.5f);

            for (int xi = 0; xi < GRID_X; xi++) {
                float bx = (xi - (GRID_X / 2.0f)) * 4.0f;
                float y_wave = sinf(bx * 0.2f + wave_time * 0.8f) * 3.0f +
                               cosf(bz * 0.15f - wave_time * 1.2f) * 4.0f;

                float by = cam_y + y_wave;

                int px = (int)((bx * fov) / bz) + (sw / 2);
                int py = (int)((by * fov) / bz) + 155;

                int size = (int)(40.0f / bz);
                if (size < 1) size = 1;
                if (size > 6) size = 6;

                ui::Color c;
                if (y_wave > 2.5f)
                    c = ui::Color::white();
                else if (y_wave > 1.0f)
                    c = ui::Color::magenta();
                else if (y_wave > -1.5f)
                    c = ui::Color::cyan();
                else
                    c = ui::Color::blue();

                prev_pts[idx] = {px, py, size, c};

                safe_fill(px, py, size, size, c);
                idx++;
            }
        }
    }

   private:
    void safe_fill(int x, int y, int w, int h, ui::Color c) {
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        if (x >= sw || y >= sh || x + w <= 0 || y + h <= 157) return;

        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 157) {
            h -= (157 - y);
            y = 157;
        }
        if (x + w > sw) w = sw - x;
        if (y + h > sh) h = sh - y;

        if (w > 0 && h > 0) {
            _api->fill_rectangle(x, y, w, h, c.v);
        }
    }
};

}  // namespace ui