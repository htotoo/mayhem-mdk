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
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

#define USER_COMMANDS_START 0x7F01

namespace ui {

enum class Command : uint16_t {
    // UART specific commands
    COMMAND_UART_REQUESTDATA_SHORT = USER_COMMANDS_START,
    COMMAND_UART_REQUESTDATA_LONG,
    COMMAND_UART_BAUDRATE_INC,
    COMMAND_UART_BAUDRATE_DEC,
    COMMAND_UART_BAUDRATE_GET
};

class UartAPPView : public ui::View {
   public:
    UartAPPView(ui::NavigationView& nav) {
        (void)nav;
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

        Command cmd = Command::COMMAND_UART_BAUDRATE_GET;
        std::vector<uint8_t> data(4);

        if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false)
            return;

        uint32_t baudrate = *(uint32_t*)data.data();
        set_baudrate(baudrate);
    }

    ~UartAPPView() {
        ui::Theme::destroy();
    }

    void on_framesync() override {
        if (isBaudrateChanged()) {
            Command cmd = Command::COMMAND_UART_BAUDRATE_GET;
            std::vector<uint8_t> data(4);

            if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false)
                return;

            uint32_t baudrate = *(uint32_t*)data.data();
            set_baudrate(baudrate);
            return;
        }

        Command cmd = Command::COMMAND_UART_REQUESTDATA_SHORT;
        std::vector<uint8_t> data(5);

        uint8_t more_data_available;
        do {
            if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false)
                return;

            uint8_t data_len = data[0] & 0x7f;
            more_data_available = data[0] >> 7;
            uint8_t* data_ptr = data.data() + 1;

            if (data_len > 0) {
                get_console().write(std::string((char*)data_ptr, data_len));
            }

            if (more_data_available) {
                cmd = Command::COMMAND_UART_REQUESTDATA_LONG;
                data = std::vector<uint8_t>(128);
            }
        } while (more_data_available == 1);
    }

    ui::Console& get_console() {
        return console;
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

    ui::Console console{{0, 2 * 16, UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(4)}};

    uint32_t baudrate_{115200};
    bool baudrate_dirty_{true};
};

}  // namespace ui
