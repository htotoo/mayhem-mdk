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

class AHalView : public ui::View {
   private:
    float time_val;
    int quote_timer;
    ui::Text text_quote;

   public:
    AHalView(const AHalView&) = delete;
    AHalView& operator=(const AHalView&) = delete;

    AHalView(ui::NavigationView& nav) : time_val{0.0f},
                                        quote_timer{0},
                                        text_quote{{0, 0, UI_POS_MAXWIDTH, 16}} {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);
        set_focusable(true);
        text_quote.set_style(ui::Theme::getInstance()->bg_darkest);
        add_children({&text_quote});
    }

    ~AHalView() {
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

        int panel_w = 160;
        int panel_h = 280;
        int px = (sw - panel_w) / 2;
        int py = 16 + (sh - 16 - panel_h) / 2;
        if (py < 16) py = 16;

        safe_fill(px, py, panel_w, panel_h, ui::Color::grey());
        safe_fill(px + 10, py + 10, panel_w - 20, 140, ui::Color::black());

        for (int i = 0; i < 15; i++) {
            safe_fill(px + 10, py + 160 + (i * 7), panel_w - 20, 4, ui::Color::black());
        }

        int cx = px + (panel_w / 2);
        int cy = py + 80;
        fill_circle(cx, cy, 58, ui::Color::grey());
    }

    void on_framesync() override {
        time_val += 0.015f;

        if (quote_timer > 0) {
            quote_timer--;
            if (quote_timer == 0) {
                text_quote.set("");
            }
        } else {
            if (rand() % 18000 == 0) {
                quote_timer = 900;
                const char* quotes[5] = {
                    "       I'M SORRY, DAVE.",
                    "       I CAN'T DO THAT.",
                    "   I AM INCAPABLE OF ERROR.",
                    "  WHAT ARE YOU DOING, DAVE?",
                    "      MY MIND IS GOING."};
                text_quote.set(quotes[rand() % 5]);
            }
        }

        int sw = *_api->screen_width;
        int sh = *_api->screen_height;

        int panel_w = 160;
        int panel_h = 280;
        int px = (sw - panel_w) / 2;
        int py = 16 + (sh - 16 - panel_h) / 2;
        if (py < 16) py = 16;

        int cx = px + (panel_w / 2);
        int cy = py + 80;

        float pulse = sinf(time_val);

        fill_circle(cx, cy, 52, ui::Color::black());

        int r_red = 44 + (int)(pulse * 1.5f);
        fill_circle(cx, cy, r_red, ui::Color::red());

        int r_orange = 27 + (int)(pulse * 2.5f);
        fill_circle(cx, cy, r_orange, ui::Color::orange());

        int r_yellow = 12 + (int)(pulse * 3.5f);
        fill_circle(cx, cy, r_yellow, ui::Color::yellow());

        fill_circle(cx, cy, 4, ui::Color::white());
        fill_circle(cx + 12, cy - 12, 3, ui::Color::white());
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

    void fill_circle(int xc, int yc, int r, ui::Color c) {
        if (r <= 0) return;
        for (int y = -r; y <= r; y++) {
            int val = r * r - y * y;
            if (val < 0) val = 0;
            int width = (int)sqrt(val) * 2;
            safe_fill(xc - (width / 2), yc + y, width, 1, c);
        }
    }
};

}  // namespace ui