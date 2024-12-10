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

enum class Command : uint16_t {
    PPCMD_IRTX_SENDIR = 0xa003
};

enum irproto : uint8_t {
    UNK,
    NEC,
    NECEXT,
    SONY,
    SAM,
    RC5,
    PROTO_COUNT
};

typedef struct ir_data {
    irproto protocol;
    uint64_t data;
    uint8_t repeat;
} ir_data_t;

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({
            &labels,

            &button_set,
        });

        button_set.on_select = [this](ui::Button&) {
            Command cmd = Command::PPCMD_IRTX_SENDIR;
            ir_data_t ir;
            ir.protocol = irproto::NECEXT;
            ir.data = 0x20DF10EF;
            ir.repeat = 1;
            std::vector<uint8_t> data(sizeof(ir_data_t));
            memcpy(data.data(), &ir, sizeof(ir_data_t));
            data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
            if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
        };
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
        button_set.focus();
    }

   private:
    ui::Button button_set{{230 - 9 * 8, 320 - 4 * 16, 9 * 8, 20}, "Sed ir"};

    ui::Labels labels{
        {{40 + 4 * 8, 4}, ".", ui::Theme::getInstance()->fg_light->foreground},
        {{40 + 13 * 8, 4}, ".", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Context& context_;
};
