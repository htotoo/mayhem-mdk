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
    PPCMD_WIFI_SET_STA = 0xa005,
    PPCMD_WIFI_SET_AP = 0xa006,
    PPCMD_WIFI_GET_CONFIG = 0xa007,
    PPCMD_WIFI_STARTSCAN = 0xa008,
    PPCMD_WIFI_STOPSCAN = 0xa009,
    PPCMD_WIFI_GETSCANRESULT = 0xa00a,
    // ESP manager app
    PPCMD_AIRPLANE_MODE = 0xa00b,
    // esp apps app
    PPCMD_APPMGR_APPMGR = 0xa00c,
    PPCMD_APPMGR_APPCMD = 0xa00d,
    // evil portal - dedicated portal AP (separate from the management AP)
    PPCMD_EVILPORTAL_SET_AP = 0xa00e,
    PPCMD_EVILPORTAL_GET_AP = 0xa00f,
    // evil portal - list fetch (index sent as uint16, one entry returned per read)
    PPCMD_EVILPORTAL_GET_CLIENT = 0xa010,
    PPCMD_EVILPORTAL_GET_CAPTURE = 0xa011,
    // evil portal - custom HTML upload ([uint16 offset][bytes]; 0=reset, 0xFFFF=commit)
    PPCMD_EVILPORTAL_SET_HTML = 0xa012,
    // evil portal - readback diagnostic ([uint16 len][uint8 useCustom][uint8 firstByte])
    PPCMD_EVILPORTAL_GET_HTMLLEN = 0xa013,
    // wifi analyzer - GET_ENTRY sends uint16 index / returns one wifi_ap_entry_t; RESCAN forces a scan
    PPCMD_WIFIANALYZER_GET_ENTRY = 0xa020,
    PPCMD_WIFIANALYZER_RESCAN = 0xa021,
    // wifi analyzer - enable/disable auto rescan (payload byte: 1=on, 0=off)
    PPCMD_WIFIANALYZER_SET_AUTO = 0xa022,
    // probe sniffer - GET_ENTRY (uint16 index -> probe_entry_t), SET_AUTO (1=capture,0=pause), CLEAR
    PPCMD_PROBESNIFFER_GET_ENTRY = 0xa030,
    PPCMD_PROBESNIFFER_SET_AUTO = 0xa031,
    PPCMD_PROBESNIFFER_CLEAR = 0xa032,
    // pmkid capture - GET_ENTRY (uint16 index -> pmkid_entry_t), SET_AUTO (1=capture,0=pause), CLEAR
    PPCMD_PMKID_GET_ENTRY = 0xa040,
    PPCMD_PMKID_SET_AUTO = 0xa041,
    PPCMD_PMKID_CLEAR = 0xa042,
    // pmkid capture - lock to a channel (payload byte: 0=hop all, 1-13=stay)
    PPCMD_PMKID_SET_CHANNEL = 0xa043,
    // pmkid capture - fetch hashcat line text in chunks
    PPCMD_PMKID_GET_LINELEN = 0xa044,
    PPCMD_PMKID_SET_LINEPOS = 0xa045,
    PPCMD_PMKID_GET_LINECHUNK = 0xa046,
    // pmkid capture - send a deauth burst on the current channel (force reconnect)
    PPCMD_PMKID_DEAUTH = 0xa047,
    // standalone deauth tool
    PPCMD_DEAUTH_GET_ENTRY = 0xa050,
    PPCMD_DEAUTH_RESCAN = 0xa051,
    PPCMD_DEAUTH_SET_TARGET = 0xa052,
    PPCMD_DEAUTH_SET_ATTACK = 0xa053,
    // BLE scanner
    PPCMD_BLESCAN_GET_ENTRY = 0xa060,
    PPCMD_BLESCAN_SET_AUTO = 0xa061,
    PPCMD_BLESCAN_CLEAR = 0xa062,
    // BLE spam
    PPCMD_BLESPAM_SET_MODE = 0xa070,
    PPCMD_BLESPAM_SET_ACTIVE = 0xa071,
    PPCMD_BLESPAM_ADD = 0xa072,
    PPCMD_BLESPAM_COMMIT = 0xa073,
    PPCMD_BLESPAM_CLEAR_CUSTOM = 0xa074,
    // drone detector
    PPCMD_DRONE_GET_ENTRY = 0xa080,
    PPCMD_DRONE_CLEAR = 0xa081
};