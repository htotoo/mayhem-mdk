/*
This file contains the used command addresses. You should not reuse them.
You can define your own here, or in another file too. This is just a common container, to help them find in one place.
*/

#pragma once

#include <cstdint>

enum class Command : uint16_t {
    // Sat track app
    PPCMD_SATTRACK_DATA = 0xa000,
    PPCMD_SATTRACK_SETSAT = 0xa001,
    PPCMD_SATTRACK_SETMGPS = 0xa002,
    // IR send / receive app
    PPCMD_IRTX_SENDIR = 0xa003,
    PPCMD_IRTX_GETLASTRCVIR = 0xa004,
    // Wifi settings app
    PPCMD_WIFI_SET = 0xa005,
    PPCMD_WIFI_STARTSCAN = 0xa006,
    PPCMD_WIFI_STOPSCAN = 0xa007,
    PPCMD_WIFI_GETSCANRESULT = 0xa008
};