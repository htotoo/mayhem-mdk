#pragma once

#include "ui/ui_helper.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui_navigation.hpp"
#include "pp_commands.hpp"

class AppWifiSSIDSpam : public ui::View {
   public:
    AppWifiSSIDSpam() {
        add_children({&btn_standby, &btn_random, &btn_rick, &btn_emoji});
        btn_standby.on_select = [this](ui::Button&) {
            sendAppCommand(0);
        };
        btn_random.on_select = [this](ui::Button&) {
            sendAppCommand(1);
        };
        btn_rick.on_select = [this](ui::Button&) {
            sendAppCommand(2);
        };
        btn_emoji.on_select = [this](ui::Button&) {
            sendAppCommand(3);
        };
    };

    void sendAppCommand(uint16_t appcmd) {
        std::vector<uint16_t> data;
        data.push_back(static_cast<uint16_t>(Command::PPCMD_APPMGR_APPCMD));
        data.push_back(static_cast<uint16_t>(appcmd));
        _api->i2c_read((uint8_t*)data.data(), 4, nullptr, 0);
    }

    ui::Button btn_standby{{UI_POS_X(0), UI_POS_Y(0), UI_POS_WIDTH(10), UI_POS_HEIGHT(1.5)}, "Standby"};
    ui::Button btn_random{{UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(10), UI_POS_HEIGHT(1.5)}, "Random"};
    ui::Button btn_rick{{UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(10), UI_POS_HEIGHT(1.5)}, "Rickroll"};
    ui::Button btn_emoji{{UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH(10), UI_POS_HEIGHT(1.5)}, "Emoji"};
};