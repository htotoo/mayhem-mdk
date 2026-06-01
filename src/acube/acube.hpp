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

class ACubeView : public ui::View {
   private:
    struct Point3D {
        float x, y, z;
    };
    struct Point2D {
        int x, y, size;
        ui::Color c;
    };

    static const int NUM_POINTS = 216;
    Point3D base_pts[NUM_POINTS];
    Point2D prev_pts[NUM_POINTS];

    float angle_x;
    float angle_y;
    float time_val;
    float speed_mult;

   public:
    ACubeView(const ACubeView&) = delete;
    ACubeView& operator=(const ACubeView&) = delete;

    ACubeView(ui::NavigationView& nav) : base_pts{},
                                         prev_pts{},
                                         angle_x{0.3f},
                                         angle_y{0.0f},
                                         time_val{0.0f},
                                         speed_mult{5.0f}  // Maximális sebesség
    {
        (void)nav;
        set_focusable(true);
        set_style(ui::Theme::getInstance()->bg_dark);

        int idx = 0;
        for (int x = 0; x < 6; x++) {
            for (int y = 0; y < 6; y++) {
                for (int z = 0; z < 6; z++) {
                    base_pts[idx++] = {(x - 2.5f) * 0.9f, (y - 2.5f) * 0.9f, (z - 2.5f) * 0.9f};
                }
            }
        }

        for (int i = 0; i < NUM_POINTS; i++) {
            prev_pts[i] = {0, 0, 0, ui::Color::black()};
        }
    }

    ~ACubeView() {
        ui::Theme::destroy();
    }

    void focus() override {
        View::focus();
    }

    // Bemenetek elkapása a kilépéshez
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
        for (int i = 0; i < NUM_POINTS; i++) {
            if (prev_pts[i].size > 0) {
                safe_fill(prev_pts[i].x, prev_pts[i].y, prev_pts[i].size, prev_pts[i].size, ui::Color::black());
            }
        }

        time_val += 0.05f * speed_mult;
        angle_y += 0.015f * speed_mult;
        angle_x += 0.005f * speed_mult;

        float sy = sin(angle_y), cy = cos(angle_y);
        float sx = sin(angle_x), cx = cos(angle_x);

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        float fov = 160.0f;

        for (int i = 0; i < NUM_POINTS; i++) {
            float bx = base_pts[i].x;
            float by = base_pts[i].y;
            float bz = base_pts[i].z;

            float wave = sin(bx * 2.0f + time_val) * cos(bz * 2.0f + time_val) * 0.4f;
            by += wave;

            float x1 = bx * cy + bz * sy;
            float z1 = -bx * sy + bz * cy;

            float y2 = by * cx - z1 * sx;
            float z2 = by * sx + z1 * cx;

            float z_trans = z2 + 6.0f;

            int px = (int)((x1 * fov) / z_trans) + (sw / 2);
            int py = (int)((y2 * fov) / z_trans) + (sh / 2);

            // MÉRET NÖVELÉSE: Nagyobb bázis szorzó (25.0f) és nagyobb maximum (8)
            int size = (int)(25.0f / z_trans);
            if (size < 1) size = 1;
            if (size > 8) size = 8;

            prev_pts[i].x = px;
            prev_pts[i].y = py;
            prev_pts[i].size = size;

            ui::Color c;
            if (wave > 0.15f)
                c = ui::Color::magenta();
            else if (wave < -0.15f)
                c = ui::Color::cyan();
            else
                c = ui::Color::blue();

            prev_pts[i].c = c;
        }

        for (int i = 0; i < NUM_POINTS; i++) {
            safe_fill(prev_pts[i].x, prev_pts[i].y, prev_pts[i].size, prev_pts[i].size, prev_pts[i].c);
        }
    }

   private:
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