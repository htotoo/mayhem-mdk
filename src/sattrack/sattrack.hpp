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

enum class Command : uint16_t {
    // UART specific commands
    PPCMD_SATTRACK_DATA = 0xa000,
    PPCMD_SATTRACK_SETSAT = 0xa001
};

typedef struct
{
    float azimuth;
    float elevation;
    uint8_t day;    // data time for setting when the data last updated, and to let user check if it is ok or not
    uint8_t month;  // lat lon won't sent with this, since it is queried by driver
    uint16_t year;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t sat_day;  // sat last data
    uint8_t sat_month;
    uint16_t sat_year;
    uint8_t sat_hour;
    float lat;  // where am i
    float lon;
} sattrackdata_t;

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&labels,
                      &text_gps,
                      &option_sat,
                      //&text_sat,
                      &text_fixtime,
                      &text_elevation,

                      &text_azi});

        text_gps.set("-");  // todo input box for it
        // text_sat.set("NOAA 15");  // todo input for it
        text_fixtime.set("0:00:00");
        text_elevation.set("??.?");
        text_azi.set("??.?");

        option_sat.on_change = [this](size_t, ui::OptionsField::value_t v) {
            if (v != -1) {
                std::string sn = option_sat.options()[v].first;
                Command cmd = Command::PPCMD_SATTRACK_SETSAT;
                std::vector<uint8_t> data(sn.begin(), sn.end());
                if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false)
                    return;
            }
        };
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
        text_gps.focus();
    }

    bool need_refresh() {
        rfcnt++;
        if (rfcnt > 120) {  // 2 sec
            rfcnt = 0;
            return true;
        }
        return false;
    }

    void got_data(sattrackdata_t* data) {
        // data->
        text_gps.set(to_string_decimal(data->lat, 5) + " " + to_string_decimal(data->lon, 5));
        // text_sat.set("NOAA 15");
        text_fixtime.set(to_string_dec_uint(data->hour) + ":" + to_string_dec_uint(data->minute) + ":" + to_string_dec_uint(data->second));
        text_dbtime.set(to_string_dec_uint(data->sat_year) + "-" + to_string_dec_uint(data->sat_month) + "-" + to_string_dec_uint(data->sat_day) + " " + to_string_dec_uint(data->sat_hour));
        text_elevation.set(to_string_decimal(data->elevation, 2));
        text_azi.set(to_string_decimal(data->azimuth, 2));
    }

   private:
    uint8_t rfcnt = 0;  // refresh counter, inc with frame_sync
    ui::Text text_gps{{40, 4, 30 * 8, 16}};
    // ui::Text text_sat{{40, 4 + 1 * 16, 30 * 8, 16}};
    ui::Text text_fixtime{{40, 4 + 2 * 16, 30 * 8, 16}};
    ui::Text text_dbtime{{40, 4 + 3 * 16, 30 * 8, 16}};
    ui::Text text_elevation{{90, 4 + 5 * 16, 30 * 8, 16}};
    ui::Text text_azi{{90, 4 + 6 * 16, 30 * 8, 16}};
    ui::OptionsField option_sat{
        {40, 4 + 1 * 16},
        20,
        {
            {"NOAA 15", 0},
            {"NOAA 18", 1},
            {"NOAA 19", 2},
            {"GOES 14", 3},
            {"GOES 15", 4},
            {"GOES 16", 5},
            {"GOES 17", 6},
            {"GOES 18", 7},
            {"GOES 19", 8},

        }};

    ui::Labels labels{
        {{0 * 8, 4}, "GPS:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 1 * 16}, "Sat:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 2 * 16}, "Fix:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 3 * 16}, "DBT:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 5 * 16}, "Elevation:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 6 * 16}, "Azimuth  :", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Context& context_;
};
