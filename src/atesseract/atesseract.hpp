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

class ATesseractView : public ui::View {
   private:
    struct Point4D {
        float x, y, z, w;
    };
    struct Point2D {
        int x, y, size;
        ui::Color c;
    };

    static const int NUM_POINTS = 320;
    Point4D base_pts[NUM_POINTS];
    Point2D prev_pts[NUM_POINTS];

    float angle_xy;
    float angle_xw;
    float angle_zw;

   public:
    ATesseractView(const ATesseractView&) = delete;
    ATesseractView& operator=(const ATesseractView&) = delete;

    ATesseractView(ui::NavigationView& nav) : prev_pts{},
                                              angle_xy{0.0f},
                                              angle_xw{0.0f},
                                              angle_zw{0.0f} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);

        int idx = 0;
        for (int i = 0; i < 16; i++) {
            for (int b = 0; b < 4; b++) {
                if (!(i & (1 << b))) {
                    int j = i | (1 << b);
                    Point4D p1 = {
                        (i & 1) ? 1.0f : -1.0f,
                        (i & 2) ? 1.0f : -1.0f,
                        (i & 4) ? 1.0f : -1.0f,
                        (i & 8) ? 1.0f : -1.0f};
                    Point4D p2 = {
                        (j & 1) ? 1.0f : -1.0f,
                        (j & 2) ? 1.0f : -1.0f,
                        (j & 4) ? 1.0f : -1.0f,
                        (j & 8) ? 1.0f : -1.0f};
                    for (int k = 0; k < 10; k++) {
                        float t = k / 10.0f;
                        base_pts[idx++] = {
                            p1.x + t * (p2.x - p1.x),
                            p1.y + t * (p2.y - p1.y),
                            p1.z + t * (p2.z - p1.z),
                            p1.w + t * (p2.w - p1.w)};
                    }
                }
            }
        }
    }

    ~ATesseractView() {
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

        angle_xy += 0.015f;
        angle_xw += 0.022f;
        angle_zw += 0.018f;

        float cxy = cosf(angle_xy), sxy = sinf(angle_xy);
        float cxw = cosf(angle_xw), sxw = sinf(angle_xw);
        float czw = cosf(angle_zw), szw = sinf(angle_zw);

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        for (int i = 0; i < NUM_POINTS; i++) {
            float x = base_pts[i].x;
            float y = base_pts[i].y;
            float z = base_pts[i].z;
            float w = base_pts[i].w;

            float nx = x * cxy - y * sxy;
            float ny = x * sxy + y * cxy;
            x = nx;
            y = ny;

            nx = x * cxw - w * sxw;
            float nw = x * sxw + w * cxw;
            x = nx;
            w = nw;

            float nz = z * czw - w * szw;
            nw = z * szw + w * czw;
            z = nz;
            w = nw;

            float w_dist = 2.4f;
            float w_factor = 1.0f / (w_dist - w);
            float x3 = x * w_factor;
            float y3 = y * w_factor;
            float z3 = z * w_factor;

            float z_dist = 3.2f;
            float z_factor = 200.0f / (z_dist - z3);

            int px = (int)(x3 * z_factor) + (sw / 2);
            int py = (int)(y3 * z_factor) + (sh / 2);

            int size = (int)(25.0f / ((w_dist - w) * (z_dist - z3)));
            if (size < 1) size = 1;
            if (size > 8) size = 8;

            ui::Color c;
            if (w > 0.6f)
                c = ui::Color::white();
            else if (w > 0.0f)
                c = ui::Color::green();
            else if (w > -0.6f)
                c = ui::Color::dark_green();
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