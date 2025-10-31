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
#include "ui/file_path.hpp"
#include "ui/ui_helper.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
namespace ui {

class MapAppView : public ui::View {
   public:
    MapAppView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&button_send, &geo_map});

        geo_map.init();
        geo_map.set_focusable(true);
        geo_map.set_mode(ui::GeoMapMode::DISPLAY);

        geo_map.move(19.0, 48.0);
        button_send.on_select = [this](ui::Button&) {
            geo_map.move(21.1, 48.1);
        };
    }

    ~MapAppView() {
        ui::Theme::destroy();
    }

    void focus() override {
        button_send.focus();
    }

   private:
    ui::Button button_send{{0, UI_POS_Y(0), UI_POS_WIDTH(5), UI_POS_HEIGHT(2)}, "Move", true};
    // ui::Button button_recv{{1, 60, 7 * 16 + 1, 20}, "Read ir"};
    ui::GeoMap geo_map{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - 60}};
    uint8_t rfcnt = 0;
};

}  // namespace ui