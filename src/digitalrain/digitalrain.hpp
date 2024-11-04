/*
 * Copyright (C) 2024 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "standalone_application.hpp"

#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include <string.h>
#include "ui/ui_font_fixed_5x8.hpp"
#include "ui/ui_painter.hpp"
#include <string>
#include <random>
#include <cstdlib>  // for std::rand() and std::srand()
#include <ctime>    // for std::time()

class DigitalRain {
   private:
    ui::Painter painter{};
    static const int WIDTH = 240;
    static const int HEIGHT = 320;
    static const int MARGIN_TOP = 20;  // Added top margin
    static const int CHAR_WIDTH = 5;
    static const int CHAR_HEIGHT = 8;
    static const int COLS = WIDTH / CHAR_WIDTH;
    static const int ROWS = (HEIGHT - MARGIN_TOP) / CHAR_HEIGHT;  // Adjusted for margin
    static const int MAX_DROPS = 24;

    const ui::Font& font = ui::font::fixed_5x8();

    struct Drop {
        uint8_t x;
        int16_t y;
        uint8_t length;
        uint8_t speed;
        uint8_t morph_counter[16];
        char chars[16];
        bool active;
    };

    Drop drops[MAX_DROPS];
    const char char_set[16] = {
        '@', '#', '$', '0', '1', '2', '>', '<',
        '/', '\\', '[', ']', '{', '}', '.', ' '};

    inline int random(int min, int max) {
        return min + (std::rand() % (max - min + 1));
    }

    void init_drop(uint8_t index, bool force_top = false) {
        drops[index].x = random(0, COLS - 1);
        drops[index].y = force_top ? -random(0, 5) : -5;
        drops[index].length = random(5, 15);
        drops[index].speed = random(1, 3);
        drops[index].active = true;

        for (uint8_t i = 0; i < 16; i++) {
            drops[index].chars[i] = char_set[random(0, 15)];
            drops[index].morph_counter[i] = random(2, 6);
        }
    }

    void morph_characters(Drop& drop) {
        for (uint8_t i = 0; i < drop.length; i++) {
            if (--drop.morph_counter[i] == 0) {
                drop.chars[i] = char_set[random(0, 15)];
                drop.morph_counter[i] = random(2, 6);
            }
        }
    }

   public:
    DigitalRain() {
        std::srand(0);
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            init_drop(i, true);
        }
    }

    void update() {
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            if (!drops[i].active) continue;

            drops[i].y += drops[i].speed;
            morph_characters(drops[i]);

            if (drops[i].y - drops[i].length > ROWS) {
                init_drop(i);
            }
        }
    }

    void render() {
        // Clear screen starting from margin_top
        painter.fill_rectangle_unrolled8(
            {0, MARGIN_TOP, WIDTH, HEIGHT - MARGIN_TOP},
            ui::Color::black());

        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            if (!drops[i].active) continue;

            for (uint8_t j = 0; j < drops[i].length; ++j) {
                int y = drops[i].y - j;
                if (y >= 0 && y < ROWS) {
                    // Calculate position in pixels, adding MARGIN_TOP
                    ui::Point p{
                        static_cast<int16_t>(drops[i].x * CHAR_WIDTH),
                        static_cast<int16_t>(y * CHAR_HEIGHT + MARGIN_TOP)  // Add margin
                    };

                    // Color calculation
                    ui::Color fg;
                    if (j == 0) {
                        fg = ui::Color::white();
                    } else if (j < 3) {
                        fg = ui::Color(0, 255, 0);
                    } else {
                        uint8_t intensity = std::max(40, 180 - (j * 15));
                        fg = ui::Color(0, intensity, 0);
                    }

                    // Render character
                    std::string ch(1, drops[i].chars[j]);
                    painter.draw_string(
                        p,
                        font,
                        fg,
                        ui::Color::black(),
                        ch);
                }
            }
        }
    }
};
class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        // add_children({&console});
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
    }

    bool need_refresh() {
        digitalRain.update();
        digitalRain.render();
        return true;
    }

   private:
    ui::Context& context_;
    ui::Console console{{0, 0, 240, 320}};
    DigitalRain digitalRain{};
};
