/*
 * Copyright (C) 2025 HTotoo
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

#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include "ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "pp_commands.hpp"
#include <string>
#include "ui_textentry.hpp"
namespace ui {

class ESPManagerView : public View {
   public:
    ESPManagerView(NavigationView& nav);
    ~ESPManagerView() {
        Theme::destroy();
    };

    std::string title() const override { return "ESP Manager"; };
    void on_framesync() override;

   private:
    void get_current_config();

    NavigationView& nav_;
    uint8_t config_loaded = 0;
    Button btn_airplane_on{{UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "ON"};
    Button btn_airplane_off{{UI_POS_X(15), UI_POS_Y(5), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)}, "OFF"};
    Labels labels{
        {{UI_POS_X_CENTER(12), UI_POS_Y(0)}, "ESP Manager", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "Airplane mode", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "To eliminate RF from it", ui::Theme::getInstance()->fg_yellow->foreground}};
};
}  // namespace ui