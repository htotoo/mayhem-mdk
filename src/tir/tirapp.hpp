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
#include "ui/ui_helper.hpp"
#include <string.h>
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

#include "pp_commands.hpp"
namespace ui {

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

class TIRAppView : public ui::View {
   public:
    TIRAppView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({
            &labels,
            &button_recv,
            &button_send,
            &text_filename,
            &text_irproto,
            &text_irdata,
        });

        button_send.on_select = [this](ui::Button&) {
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

        button_recv.on_select = [this](ui::Button&) {
            recv_mode_on = true;
            button_recv.set_text("Waiting...");
            text_irproto.set("-");
            text_irdata.set("-");
        };
    }

    ~TIRAppView() {
        ui::Theme::destroy();
    }

    void on_framesync() override {
        if (need_refresh()) {
            Command cmd = Command::PPCMD_IRTX_GETLASTRCVIR;
            std::vector<uint8_t> data(sizeof(ir_data_t));
            if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false) return;
            ir_data_t irdata = *(ir_data_t*)data.data();
            got_data(irdata);
        }
    }

    void focus() override {
        button_recv.focus();
    }

    bool need_refresh() {
        rfcnt++;
        if (!recv_mode_on) return false;
        if (rfcnt > 60) {  // 1 sec
            rfcnt = 0;
            return true;
        }
        return false;
    }

    void got_data(ir_data_t ir) {
        if (ir.protocol == irproto::UNK) return;
        recv_mode_on = false;  // no more, got a valid
        button_recv.set_text("Read ir");
        std::string proto = "UNK";
        switch (ir.protocol) {
            case irproto::NEC:
                proto = "NEC";
                break;
            case irproto::NECEXT:
                proto = "NECEXT";
                break;
            case irproto::SONY:
                proto = "SONY";
                break;
            case irproto::SAM:
                proto = "SAM";
                break;
            case irproto::RC5:
                proto = "RC5";
                break;
            default:
                break;
        }
        text_irproto.set(proto);
        text_irdata.set(to_string_hex(ir.data, 16));
    }

   private:
    ui::Button button_send{{1, 20, 7 * 16 + 1, 20}, "Send ir"};
    ui::Button button_recv{{1, 60, 7 * 16 + 1, 20}, "Read ir"};

    ui::Labels labels{
        {{1, 1}, "File:", ui::Theme::getInstance()->fg_light->foreground},
        {{1, 40}, "------------------------------", ui::Theme::getInstance()->fg_light->foreground},
        {{1, 80}, "IR received:", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Text text_filename{{7 * 8, 1, 22 * 8, 16}, "-NIY-"};
    ui::Text text_irproto{{1, 100, 22 * 8, 16}, "-"};
    ui::Text text_irdata{{1, 120, 22 * 8, 16}, "-"};

    uint8_t rfcnt = 0;
    bool recv_mode_on = false;
};

}  // namespace ui