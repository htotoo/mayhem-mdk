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
    PPCMD_SATTRACK_SETSAT = 0xa001,
    PPCMD_SATTRACK_SETMGPS = 0xa002
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

typedef struct
{
    float lat;
    float lon;
} sat_mgps_t;

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&labels,
                      &option_sat,
                      &text_fixtime,
                      &text_elevation,
                      &text_azi,
                      &button_set,
                      &mlatpn,
                      //&mlat3,
                      &mlat2,
                      &mlat1,
                      &mlats1,
                      &mlats2,
                      &mlats3,
                      &mlonpn,
                      &mlon3,
                      &mlon2,
                      &mlon1,
                      &mlons1,
                      &mlons2,
                      &mlons3});

        option_sat.on_change = [this](size_t, ui::OptionsField::value_t v) {
            if (v != -1) {
                std::string sn = "GOES 18";  // option_sat.options()[v].first;
                button_set.set_text("b");
                Command cmd = Command::PPCMD_SATTRACK_SETSAT;
                std::vector<uint8_t> data(sn.begin(), sn.end());
                if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false) return;
                button_set.set_text("bbb");
            }
        };
        button_set.on_select = [this](ui::Button&) {
            Command cmd = Command::PPCMD_SATTRACK_SETMGPS;
            button_set.set_text("a");
            sat_mgps_t mgps;
            mgps.lat = getLat();
            mgps.lon = getLon();
            if (_api->i2c_read((uint8_t*)&cmd, 2, (uint8_t*)&mgps, sizeof(sat_mgps_t)) == false) return;
            button_set.set_text("aaa");
        };
    }

    ui::Context& context() const override {
        return context_;
    }

    void focus() override {
        option_sat.focus();
    }

    bool need_refresh() {
        rfcnt++;
        if (rfcnt > 120) {  // 2 sec
            rfcnt = 0;
            return true;
        }
        return false;
    }

    void got_data(sattrackdata_t data) {
        if (data.lat != 0 && data.lon != 0) {
            setLat(data.lat);
            setLon(data.lon);
        }

        text_fixtime.set(to_string_dec_uint(data.hour) + ":" + to_string_dec_uint(data.minute) + ":" + to_string_dec_uint(data.second));
        text_dbtime.set(to_string_dec_uint(data.sat_year) + "-" + to_string_dec_uint(data.sat_month) + "-" + to_string_dec_uint(data.sat_day) + " " + to_string_dec_uint(data.sat_hour));
        // text_elevation.set(to_string_decimal(data.elevation, 2));
        // text_azi.set(to_string_decimal(data.azimuth, 2));
    }

   private:
    float getLat() {
        float tmp = /*mlat3.value() * 100 +*/ mlat2.value() * 10 + mlat1.value() + mlats1.value() * 0.1 + mlats2.value() * 0.01 + mlats3.value() * 0.001;
        if (mlatpn.selected_index_value() == 1) tmp = -tmp;
        return tmp;
    }
    float getLon() {
        float tmp = mlon3.value() * 100 + mlon2.value() * 10 + mlon1.value() + mlons1.value() * 0.1 + mlons2.value() * 0.01 + mlons3.value() * 0.001;
        if (mlonpn.selected_index_value() == 1) tmp = -tmp;
        return tmp;
    }
    void setLat(float lat) {
        if (lat < 0) {
            mlatpn.set_selected_index(1);
            lat = -lat;
        } else
            mlatpn.set_selected_index(0);
        // mlat3.set_value(lat / 100);
        // lat -= mlat3.value() * 100;
        mlat2.set_value(lat / 10);
        lat -= mlat2.value() * 10;
        mlat1.set_value(lat);
        lat -= mlat1.value();
        mlats1.set_value(lat * 10);
        lat -= mlats1.value() * 0.1;
        mlats2.set_value(lat * 100);
        lat -= mlats2.value() * 0.01;
        mlats3.set_value(lat * 1000);
    }
    void setLon(float lon) {
        if (lon < 0) {
            mlonpn.set_selected_index(1);
            lon = -lon;
        } else
            mlonpn.set_selected_index(0);
        mlon3.set_value(lon / 100);
        lon -= mlon3.value() * 100;
        mlon2.set_value(lon / 10);
        lon -= mlon2.value() * 10;
        mlon1.set_value(lon);
        lon -= mlon1.value();
        mlons1.set_value(lon * 10);
        lon -= mlons1.value() * 0.1;
        mlons2.set_value(lon * 100);
        lon -= mlons2.value() * 0.01;
        mlons3.set_value(lon * 1000);
    }
    uint8_t rfcnt = 0;  // refresh counter, inc with frame_sync
    ui::Text text_fixtime{{40, 4 + 2 * 16, 26 * 8, 16}};
    ui::Text text_dbtime{{40, 4 + 3 * 16, 26 * 8, 16}};
    ui::Text text_elevation{{90, 4 + 5 * 16, 20 * 8, 16}};
    ui::Text text_azi{{90, 4 + 6 * 16, 20 * 8, 16}};
    ui::Button button_set{{230 - 5 * 8, 320 - 3 * 16, 5 * 8, 16}, "Set GPS"};
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
            {"IIS", 9},

        }};
    ui::OptionsField mlatpn{
        {40 + 1 * 8, 4},
        1,
        {{" ", 0},
         {"-", 1}}};

    /* ui::NumberField mlat3{
        {40 + 1 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    */
    ui::NumberField mlat2{
        {40 + 2 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlat1{
        {40 + 3 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    // point
    // ap
    ui::NumberField mlats1{
        {40 + 5 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlats2{
        {40 + 6 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlats3{
        {40 + 7 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};

    ui::OptionsField mlonpn{
        {40 + 9 * 8, 4},
        1,
        {{" ", 0},
         {"-", 1}}};

    ui::NumberField mlon3{
        {40 + 10 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlon2{
        {40 + 11 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlon1{
        {40 + 12 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    // point
    // ap
    ui::NumberField mlons1{
        {40 + 14 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlons2{
        {40 + 15 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};
    ui::NumberField mlons3{
        {40 + 16 * 8, 4},
        1,
        {0, 9},
        1,
        '0',
        true};

    ui::Labels labels{
        {{40 + 5 * 8, 4}, ".", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4}, "GPS:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 1 * 16}, "Sat:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 2 * 16}, "Fix:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 3 * 16}, "DBT:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 5 * 16}, "Elevation:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 6 * 16}, "Azimuth  :", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Context& context_;
};
