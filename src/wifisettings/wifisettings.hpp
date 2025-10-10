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

class WifiSettingsView : public View {
   public:
    WifiSettingsView(NavigationView& nav);
    ~WifiSettingsView() {};

    void focus() override;
    std::string title() const override { return "WiFi Settings"; };

   private:
    std::string ssid_ = "MyWiFi";
    NavigationView& nav_;
    Button btn_set{{0, 0, 200, 40}, "Set WiFi"};
    Text text_ssid{{0, 50, UI_POS_MAXWIDTH, 16}, "SSID: MyWiFi"};
};

}  // namespace ui