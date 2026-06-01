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

class AWormholeView : public ui::View {
   private:
    struct Point2D {
        int x, y, size;
        ui::Color c;
    };

    // Pontfelhő sűrűsége: több gyűrű, több pont, de NINCSENEK vonalak!
    static const int NUM_RINGS = 24;
    static const int SIDES = 12;
    static constexpr float Z_SPACING = 30.0f;

    Point2D prev_pts[NUM_RINGS][SIDES];

    float tunnel_offset;
    float global_time;
    float speed;

   public:
    AWormholeView(const AWormholeView&) = delete;
    AWormholeView& operator=(const AWormholeView&) = delete;

    AWormholeView(ui::NavigationView& nav) : prev_pts{},
                                             tunnel_offset{Z_SPACING},
                                             global_time{0.0f},
                                             speed{4.0f}  // Haladási sebesség a Z-tengelyen
    {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);

        for (int i = 0; i < NUM_RINGS; i++) {
            for (int j = 0; j < SIDES; j++) {
                prev_pts[i][j] = {0, 0, 0, ui::Color::black()};
            }
        }
    }

    ~AWormholeView() {
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
        // 1. Előző képkocka PONTJAINAK letörlése (Nagyságrendekkel gyorsabb, mint a vonalak!)
        for (int i = 0; i < NUM_RINGS; i++) {
            for (int j = 0; j < SIDES; j++) {
                if (prev_pts[i][j].size > 0) {
                    safe_fill(prev_pts[i][j].x, prev_pts[i][j].y,
                              prev_pts[i][j].size, prev_pts[i][j].size, ui::Color::black());
                }
            }
        }

        // 2. Idő és pozíció frissítése
        tunnel_offset -= speed;
        if (tunnel_offset < 0.0f) {
            tunnel_offset += Z_SPACING;
        }
        global_time += 0.05f;

        // 3. Új pontok kiszámítása
        int sw = *_api->screen_width;
        int sh = *_api->screen_height;
        float fov = 180.0f;
        float radius = 60.0f;

        for (int i = 0; i < NUM_RINGS; i++) {
            float z = (i * Z_SPACING) + tunnel_offset + 5.0f;

            // Kígyózás
            float cx = sin(z * 0.015f + global_time * 1.5f) * 60.0f;
            float cy = cos(z * 0.01f + global_time * 1.2f) * 60.0f;

            // Csavarodás
            float rot = z * 0.01f + global_time * 1.0f;

            ui::Color c = get_ring_color(i);

            for (int j = 0; j < SIDES; j++) {
                float angle = (j * 2.0f * 3.14159f / SIDES) + rot;
                float x = cx + cos(angle) * radius;
                float y = cy + sin(angle) * radius;

                // Projekció
                int px = (int)((x * fov) / z) + (sw / 2);
                int py = (int)((y * fov) / z) + (sh / 2);

                // Méret számítása távolság alapján (közel = nagy, távol = kicsi)
                int size = (int)(50.0f / z);
                if (size < 1) size = 1;
                if (size > 12) size = 12;

                prev_pts[i][j] = {px, py, size, c};

                // 4. Új pont kirajzolása
                safe_fill(px, py, size, size, c);
            }
        }
    }

   private:
    ui::Color get_ring_color(int depth_index) {
        if (depth_index < 3) return ui::Color::white();
        if (depth_index < 8) return ui::Color::cyan();
        if (depth_index < 16) return ui::Color::blue();
        return ui::Color::dark_blue();
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