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
#include "ui/ui_geomap.hpp"

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&labels,
                      &button_recv,
                      &button_send /*, &geo_map*/});

        button_send.on_select = [this](ui::Button&) {
            /*Command cmd = Command::PPCMD_IRTX_SENDIR;
            ir_data_t ir;
            ir.protocol = irproto::NECEXT;
            ir.data = 0x20DF10EF;
            ir.repeat = 1;
            std::vector<uint8_t> data(sizeof(ir_data_t));
            memcpy(data.data(), &ir, sizeof(ir_data_t));
            data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
            if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;*/
        };

        button_recv.on_select = [this](ui::Button&) {
            /*recv_mode_on = true;
            button_recv.set_text("Waiting...");
            text_irproto.set("-");
            text_irdata.set("-");*/
        };
        // geo_map.move(48, 19);
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
        button_recv.focus();
    }

    bool need_refresh() {
        /*rfcnt++;
        if (!recv_mode_on) return false;
        if (rfcnt > 60) {  // 1 sec
            rfcnt = 0;
            return true;
        }
        return false;*/
        return true;
    }

   private:
    ui::Button button_send{{1, 20, 7 * 16 + 1, 20}, "Send ir"};
    ui::Button button_recv{{1, 60, 7 * 16 + 1, 20}, "Read ir"};
    // ui::GeoMap geo_map{{0, 0, 7 * 16, 240}};
    ui::Labels labels{
        {{1, 1}, "File:", ui::Theme::getInstance()->fg_light->foreground},
        {{1, 40}, "------------------------------", ui::Theme::getInstance()->fg_light->foreground},
        {{1, 80}, "IR received:", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Context& context_;
    uint8_t rfcnt = 0;
    bool recv_mode_on = false;
};
