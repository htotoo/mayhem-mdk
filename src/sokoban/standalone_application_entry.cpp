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

#include "standalone_application.hpp"
#include <memory>

extern "C" {
__attribute__((section(".standalone_application_information"), used)) standalone_application_information_t _standalone_application_information = {
    /*.header_version = */ CURRENT_STANDALONE_APPLICATION_API_VERSION,

    /*.app_name = */ "Sokoban",
    /*.bitmap_data = */ {
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0x07,
        0xE0,
        0x0B,
        0xD0,
        0xF3,
        0xCF,
        0x33,
        0xCC,
        0x53,
        0xCA,
        0x93,
        0xC9,
        0x93,
        0xC9,
        0x53,
        0xCA,
        0x33,
        0xCC,
        0xF3,
        0xCF,
        0x0B,
        0xD0,
        0x07,
        0xE0,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    },
    /*.icon_color = 16 bit: 5R 6G 5B*/ 0x7E0,
    /*.menu_location = */ app_location_t::GAMES,

    /*.initialize_app = */ initialize,
    /*.on_event = */ on_event,
    /*.shutdown = */ shutdown,
    /*.PaintViewMirror = */ PaintViewMirror,
    /*.OnTouchEvent = */ OnTouchEvent,
    /*.OnFocus = */ OnFocus,
    /*.OnKeyEvent = */ OnKeyEvent,
    /*.OnEncoder = */ OnEncoder,
    /*.OnKeyboad = */ OnKeyboad,
};
}
