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
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "ui/ui_helper.hpp"

namespace ui {

class AboutRain {
   private:
    const std::vector<std::string> authors = {
        "jboone", "eried", "furrtek", "NotherNgineer", "gullradriel", "jLynx", "kallanreed", "Brumi-2021", "htotoo", "bernd-herzog", "zxkmm", "ArjanOnwezen", "euquiq", "u-foka", "iNetro"};
    //"heurist1", "dqs105", "teixeluis", "jwetzell", "jimilinuxguy", "gregoryfenton", "notpike", "strijar", "BehleZebub", "arneluehrs", "rascafr", "joyel24", "ImDroided", "zigad", "johnelder", "klockee", "nnesetto", "LupusE", "argilo", "dc2dc", "formtapez", "RocketGod-git", "mrmookie", "ITAxReal", "F33RNI", "F4GEV", "rusty-labs", "mjwaxios", "andrej-mk", "RedFox-Fr", "nemanjan00", "MichalLeonBorsuk", "MatiasFernandez", "Giorgiofox", "ckuethe"};

    ui::Painter painter{};
    int WIDTH = 240;
    int HEIGHT = 325;
    int MARGIN_TOP = 20;
    int CHAR_WIDTH = 5;
    int CHAR_HEIGHT = 8;
    int COLS = 0;
    int ROWS = 0;
    static const int MAX_DROPS = 36;

    const ui::Font& font = ui::font::fixed_5x8();

    struct Drop {
        uint8_t x;
        int16_t y;
        uint8_t length;
        uint8_t speed;
        uint8_t morph_counter[32];  // Increased size for longer names
        char chars[32];             // Increased size for longer names
        int16_t old_y;
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
        drops[index].old_y = drops[index].y;
        drops[index].length = random(5, 15);
        drops[index].speed = random(1, 2);
        drops[index].active = true;

        // Select a random author name
        std::string author = authors[random(0, authors.size() - 1)];

        // Fill the chars array with the author name
        size_t name_length = author.length();
        for (size_t i = 0; i < name_length; i++) {
            drops[index].chars[i] = author[i];
            drops[index].morph_counter[i] = random(10, 20);  // Slower morphing
        }

        // Set the drop length to match the author name length
        drops[index].length = name_length;
    }

    void clear_drop_trail(const Drop& drop) {
        // Convert to int16_t for consistent type comparison
        int16_t start_y = std::max<int16_t>(0, drop.old_y - drop.length + 1);
        int16_t end_y = std::min<int16_t>(ROWS - 1, drop.old_y);

        if (start_y <= end_y) {
            int16_t pixel_y = start_y * CHAR_HEIGHT + MARGIN_TOP;
            uint16_t height = (end_y - start_y + 1) * CHAR_HEIGHT;

            painter.fill_rectangle_unrolled8(
                {static_cast<int16_t>(drop.x * CHAR_WIDTH),
                 pixel_y,
                 CHAR_WIDTH,
                 height},
                ui::Color::black());
        }
    }

    // Remove or modify morph_characters to keep names stable
    void morph_characters(Drop& drop) {
        // Optional: Occasionally change to a different author name
        if (random(0, 100) < 2) {  // 2% chance to change
            std::string author = authors[random(0, authors.size() - 1)];
            for (size_t i = 0; i < author.length(); i++) {
                drop.chars[i] = author[i];
            }
            drop.length = author.length();
        }
    }

   public:
    AboutRain() {
        std::srand(0);
        WIDTH = UI_POS_MAXWIDTH;
        HEIGHT = UI_POS_MAXHEIGHT;
        COLS = WIDTH / CHAR_WIDTH;
        ROWS = (HEIGHT - MARGIN_TOP) / CHAR_HEIGHT;

        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            init_drop(i, true);
        }
    }

    void update() {
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            if (!drops[i].active) continue;

            // Store old position before updating
            drops[i].old_y = drops[i].y;

            // Update position
            drops[i].y += drops[i].speed;
            morph_characters(drops[i]);

            // Reset drop if off screen
            if (drops[i].y - drops[i].length > ROWS) {
                clear_drop_trail(drops[i]);  // Clear final position
                init_drop(i);
            }
        }
    }

    void render() {
        for (uint8_t i = 0; i < MAX_DROPS; ++i) {
            if (!drops[i].active) continue;

            clear_drop_trail(drops[i]);

            for (uint8_t j = 0; j < drops[i].length; ++j) {
                int y = drops[i].y - j;
                if (y >= 0 && y < ROWS) {
                    ui::Point p{
                        static_cast<int16_t>(drops[i].x * CHAR_WIDTH),
                        static_cast<int16_t>(y * CHAR_HEIGHT + MARGIN_TOP)};

                    ui::Color fg;
                    if (j == 0) {
                        fg = ui::Color::white();  // First character is white
                    } else if (j < 3) {
                        fg = ui::Color(0, 255, 0);  // Next few are bright green
                    } else {
                        uint8_t intensity = std::max(40, 180 - (j * 15));
                        fg = ui::Color(0, intensity, 0);  // Rest fade out
                    }

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

class AboutRainView : public ui::View {
   public:
    AboutRainView(NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_darkest);
    }

    ~AboutRainView() {
        ui::Theme::destroy();
    }

    void focus() override {
    }

    void on_framesync() override {
        need_refresh();
    }

    bool need_refresh() {
        update++;
        if (update % 5 == 0) {
            return false;
        }
        digitalRain.update();
        digitalRain.render();
        return true;
    }

   private:
    ui::Console console{{0, 0, UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 16}};
    AboutRain digitalRain{};
    uint8_t update = 0;
};

}  // namespace ui