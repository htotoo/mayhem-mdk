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
#include "ui/ui_helper.hpp"

#define USER_COMMANDS_START 0x7F01

enum class Command : uint16_t {
    // UART specific commands
    COMMAND_UART_REQUESTDATA_SHORT = USER_COMMANDS_START,
    COMMAND_UART_REQUESTDATA_LONG,
    COMMAND_UART_BAUDRATE_INC,
    COMMAND_UART_BAUDRATE_DEC,
    COMMAND_UART_BAUDRATE_GET
};

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect) : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&text,
                      &console,
                      &button_n,
                      &button_p

        });

        text.set("BR: " + std::to_string(baudrate_));

        button_n.on_select = [this](ui::Button&) {
            Command cmd_dec = Command::COMMAND_UART_BAUDRATE_DEC;
            _api->i2c_read((uint8_t*)&cmd_dec, 2, nullptr, 0);

            baudrate_dirty_ = true;
        };

        button_p.on_select = [this](ui::Button&) {
            Command cmd_inc = Command::COMMAND_UART_BAUDRATE_INC;
            _api->i2c_read((uint8_t*)&cmd_inc, 2, nullptr, 0);

            baudrate_dirty_ = true;
        };
    }

    ui::Console& get_console() {
        return console;
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
        button_n.focus();
    }

    void set_baudrate(uint32_t baudrate) {
        baudrate_ = baudrate;
        baudrate_dirty_ = false;

        text.set("BR: " + std::to_string(baudrate_));

        set_dirty();
    }

    bool isBaudrateChanged() {
        return baudrate_dirty_;
    }

   private:
    ui::Text text{{4, 4, 96, 16}};

    ui::Button button_n{{100, 4, 16, 24}, "-"};
    ui::Button button_p{{120, 4, 16, 24}, "+"};

    ui::Console console{{0, 2 * 16, 240, 272}};

    ui::Context& context_;

    uint32_t baudrate_{115200};
    bool baudrate_dirty_{true};
};
