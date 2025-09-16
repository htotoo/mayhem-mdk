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
    uint8_t time_method;
} sattrackdata_t;

typedef struct
{
    float lat;
    float lon;
} sat_mgps_t;

extern uint16_t screen_width;
extern uint16_t screen_height;

class StandaloneViewMirror : public ui::View {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&labels,
                      &option_sat,
                      &text_fixtime,
                      &text_dbtime,
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
                std::string sn = option_sat.selected_index_name();
                Command cmd = Command::PPCMD_SATTRACK_SETSAT;
                std::vector<uint8_t> data(sn.begin(), sn.end());
                data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));  // hacky way to i2c_transfer
                text_elevation.set("?");
                text_azi.set("?");
                if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
            }
        };
        button_set.on_select = [this](ui::Button&) {
            Command cmd = Command::PPCMD_SATTRACK_SETMGPS;
            sat_mgps_t mgps;
            mgps.lat = getLat();
            mgps.lon = getLon();
            std::vector<uint8_t> data(sizeof(sat_mgps_t));
            memcpy(data.data(), &mgps, sizeof(sat_mgps_t));
            data.insert(data.begin(), reinterpret_cast<uint8_t*>(&cmd), reinterpret_cast<uint8_t*>(&cmd) + sizeof(cmd));
            if (_api->i2c_read(data.data(), data.size(), nullptr, 0) == false) return;
            option_sat.focus();
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
        if (data.lat != 0 || data.lon != 0) {
            setLat(data.lat);
            setLon(data.lon);
        } else {
            text_elevation.set("Please set");
            text_azi.set("your location");
            text_fixtime.set("");
            return;
        }
        if (data.time_method != 0) {
            if (data.time_method == 1) {
                text_fixtime.set("GPS, " + to_string_dec_uint(data.hour) + ":" + to_string_dec_uint(data.minute) + ":" + to_string_dec_uint(data.second));
            } else {
                text_fixtime.set("NTP, " + to_string_dec_uint(data.hour) + ":" + to_string_dec_uint(data.minute) + ":" + to_string_dec_uint(data.second));
            }
        } else {
            text_fixtime.set("NO TIME");
            text_elevation.set("GPS or WIFI");
            text_azi.set("is needed");
            return;
        }
        text_dbtime.set(to_string_dec_uint(data.sat_year) + "-" + to_string_dec_uint(data.sat_month) + "-" + to_string_dec_uint(data.sat_day) + " " + to_string_dec_uint(data.sat_hour));
        text_elevation.set(to_string_decimal_my(data.elevation, 2));
        text_azi.set(to_string_decimal_my(data.azimuth, 2));
    }

   private:
    float getLat() {
        float tmp = /*mlat3.value() * 100 +*/ mlat2.value() * 10 + mlat1.value() + mlats1.value() * 0.1 + mlats2.value() * 0.01 + mlats3.value() * 0.001;
        if (mlatpn.selected_index_value() == 1) tmp = -tmp;
        if (tmp > 90) tmp = 90;
        if (tmp < -90) tmp = -90;
        return tmp;
    }
    float getLon() {
        float tmp = mlon3.value() * 100 + mlon2.value() * 10 + mlon1.value() + mlons1.value() * 0.1 + mlons2.value() * 0.01 + mlons3.value() * 0.001;
        if (mlonpn.selected_index_value() == 1) tmp = -tmp;
        if (tmp > 180) tmp = 180;
        if (tmp < -180) tmp = -180;
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
    ui::Text text_dbtime{{40, 4 + 3 * 16, 26 * 8, 16}, "?"};
    ui::Text text_elevation{{90, 4 + 5 * 16, 20 * 8, 16}};
    ui::Text text_azi{{90, 4 + 6 * 16, 20 * 8, 16}};
    ui::Button button_set{{230 - 9 * 8, 320 - 4 * 16, 9 * 8, 20}, "Set GPS"};
    ui::OptionsField option_sat{
        {40, 4 + 1 * 16},
        8,
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
            {"ISS", 9},

        }};
    ui::OptionsField mlatpn{
        {40 + 1 * 8, 4},
        1,
        {{" ", 0},
         {"-", 1}}};

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
        {0, 1},
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
        {{40 + 4 * 8, 4}, ".", ui::Theme::getInstance()->fg_light->foreground},
        {{40 + 13 * 8, 4}, ".", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4}, "GPS:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 1 * 16}, "Sat:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 2 * 16}, "Tim:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 3 * 16}, "DBT:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 5 * 16}, "Elevation:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 + 6 * 16}, "Azimuth  :", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Context& context_;

    std::string to_string_decimal_my(float decimal, int8_t precision) {
        if (precision < 0) precision = 0;
        if (precision > 7) precision = 7;  // Limit precision to avoid overflow

        // Handle special cases
        if (decimal != decimal) return "NaN";  // Check for NaN
        if (decimal == 1.0f / 0.0f) return "Inf";
        if (decimal == -1.0f / 0.0f) return "-Inf";

        char buffer[16];  // Buffer for string conversion
        int idx = 0;
        bool negative = decimal < 0;
        if (negative) {
            decimal = -decimal;
            buffer[idx++] = '-';
        }

        int32_t intPart = (int32_t)decimal;
        float fracPart = decimal - intPart;

        // Convert integer part
        if (intPart == 0) {
            buffer[idx++] = '0';
        } else {
            char temp[10];
            int tempIdx = 0;
            while (intPart > 0) {
                temp[tempIdx++] = '0' + (intPart % 10);
                intPart /= 10;
            }
            while (tempIdx > 0) {
                buffer[idx++] = temp[--tempIdx];
            }
        }

        // Add fractional part if precision > 0
        if (precision > 0) {
            buffer[idx++] = '.';
            for (int i = 0; i < precision; ++i) {
                fracPart *= 10;
                int digit = (int)fracPart;
                buffer[idx++] = '0' + digit;
                fracPart -= digit;
            }
        }

        buffer[idx] = '\0';
        return std::string(buffer);
    }
};
