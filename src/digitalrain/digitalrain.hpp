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

#include <string>
#include <random>
#include <cstdlib>  // for std::rand() and std::srand()
#include <ctime>    // for std::time()

class DigitalRain {
   private:
    static const int WIDTH = 30;
    static const int HEIGHT = 19;
    static const int MAX_DROPS = 8;  // Reduced for Portapack performance

    struct Drop {
        uint8_t x;       // Column position
        int8_t y;        // Row position (signed for above-screen positions)
        uint8_t length;  // Length of the drop
        uint8_t speed;   // Drop speed
        bool active;     // Drop state
    };

    std::string buffer;
    Drop drops[MAX_DROPS];

    // Helper function for random range
    inline int random(int min, int max) {
        return min + (std::rand() % (max - min + 1));
    }

    void init_drop(uint8_t index, bool force_top = false) {
        drops[index].x = random(0, WIDTH - 1);
        drops[index].y = force_top ? -random(0, 5) : -5;
        drops[index].length = random(3, 6);
        drops[index].speed = random(1, 2);
        drops[index].active = true;
    }

   public:
    DigitalRain()
        : buffer(WIDTH * HEIGHT, ' ') {
        std::srand(std::time(nullptr));

        // Initialize all drops
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            init_drop(i, true);
        }
    }

    void update() {
        // Clear buffer efficiently
        std::fill(buffer.begin(), buffer.end(), ' ');

        // Update and render each drop
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            if (!drops[i].active) continue;

            // Move drop
            drops[i].y += drops[i].speed;

            // Draw drop characters
            for (uint8_t j = 0; j < drops[i].length; ++j) {
                int y = drops[i].y - j;
                if (y >= 0 && y < HEIGHT) {
                    int pos = y * WIDTH + drops[i].x;
                    if (pos >= 0 && pos < WIDTH * HEIGHT) {
                        // Head of drop
                        if (j == 0) {
                            buffer[pos] = '@';
                        }
                        // Trail characters (binary style)
                        else {
                            buffer[pos] = (std::rand() & 1) ? '1' : '0';
                        }
                    }
                }
            }

            // Reset drop when it goes off screen
            if (drops[i].y - drops[i].length > HEIGHT) {
                init_drop(i);
            }
        }
    }

    std::string render() {
        std::string output;
        // Draw frame
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                output += buffer[y * WIDTH + x];
            }
            output += '\n';
        }
        return output;
    }
};

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&console});
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
    }

    bool need_refresh() {
        digitalRain.update();
        console.clear(true);
        console.write(digitalRain.render());
        return true;
    }

   private:
    ui::Context& context_;
    ui::Console console{{0, 0, 240, 320}};
    DigitalRain digitalRain{};
};
