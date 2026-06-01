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

class ATorusView : public ui::View {
   private:
    struct Point3D {
        float x, y, z;
    };
    struct Point2D {
        int x, y, size;
        ui::Color c;
    };

    static const int NUM_POINTS = 200;
    Point3D base_pts[NUM_POINTS];
    Point2D prev_pts[NUM_POINTS];

    float angle_x;
    float angle_y;
    float angle_z;

   public:
    ATorusView(const ATorusView&) = delete;
    ATorusView& operator=(const ATorusView&) = delete;

    ATorusView(ui::NavigationView& nav) : prev_pts{},
                                          angle_x{0.0f},
                                          angle_y{0.0f},
                                          angle_z{0.0f} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);

        int idx = 0;
        float R = 2.2f;
        float r = 0.9f;
        for (int i = 0; i < 20; i++) {
            float phi = (i * 2.0f * 3.14159f) / 20.0f;
            for (int j = 0; j < 10; j++) {
                float theta = (j * 2.0f * 3.14159f) / 10.0f;
                base_pts[idx].x = (R + r * cosf(theta)) * cosf(phi);
                base_pts[idx].y = (R + r * cosf(theta)) * sinf(phi);
                base_pts[idx].z = r * sinf(theta);
                prev_pts[idx] = {0, 0, 0, ui::Color::black()};
                idx++;
            }
        }
    }

    ~ATorusView() {
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
        for (int i = 0; i < NUM_POINTS; i++) {
            if (prev_pts[i].size > 0) {
                safe_fill(prev_pts[i].x, prev_pts[i].y, prev_pts[i].size, prev_pts[i].size, ui::Color::black());
            }
        }

        angle_x += 0.025f;
        angle_y += 0.035f;
        angle_z += 0.015f;

        float sx = sinf(angle_x), cx = cosf(angle_x);
        float sy = sinf(angle_y), cy = cosf(angle_y);
        float sz = sinf(angle_z), cz = cosf(angle_z);

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        float fov = 160.0f;

        for (int i = 0; i < NUM_POINTS; i++) {
            float bx = base_pts[i].x;
            float by = base_pts[i].y;
            float bz = base_pts[i].z;

            float xy = cx * by - sx * bz;
            float xz = sx * by + cx * bz;

            float yx = cy * bx + sy * xz;
            float yz = -sy * bx + cy * xz;

            float zx = cz * yx - sz * xy;
            float zy = sz * yx + cz * xy;

            float z_trans = yz + 6.5f;

            int px = (int)((zx * fov) / z_trans) + (sw / 2);
            int py = (int)((zy * fov) / z_trans) + (sh / 2);

            int size = (int)(22.0f / z_trans);
            if (size < 1) size = 1;
            if (size > 6) size = 6;

            ui::Color c;
            if (z_trans < 5.0f)
                c = ui::Color::white();
            else if (z_trans < 6.2f)
                c = ui::Color::cyan();
            else if (z_trans < 7.5f)
                c = ui::Color::blue();
            else
                c = ui::Color::dark_grey();

            prev_pts[i] = {px, py, size, c};

            safe_fill(px, py, size, size, c);
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