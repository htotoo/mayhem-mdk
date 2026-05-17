#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include <cmath>
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

struct SafeState {
    int level = 1;
    int locks_total = 3;
    int locks_cracked = 0;
    int target_val = 0;
    int frames_left = 3600;
    int max_frames = 3600;
};

class SafeCrackerView : public ui::View {
   public:
    SafeCrackerView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&label_title,
                      &text_level,
                      &text_locks,
                      &bar_time,
                      &text_time,
                      &text_left,
                      &field_dial,
                      &text_right,
                      &bar_signal,
                      &text_signal,
                      &text_anim,
                      &btn_action});

        btn_action.on_select = [this](ui::Button&) {
            if (game_status == 0 || game_status == 2) {
                start_level(1);
                field_dial.focus();
            } else if (game_status == 1) {
                game_status = 2;
                text_anim.set("SYSTEM LOCKOUT!");
                btn_action.set_text("RETRY");
            }
        };

        reset_ui();
    }

    ~SafeCrackerView() {
        ui::Theme::destroy();
    }

    void focus() override {
        btn_action.focus();
    }

    void on_framesync() override {
        frame_tick++;

        if (game_status == 1) {
            if (state.frames_left > 0) {
                state.frames_left--;
                if (state.frames_left == 0) {
                    game_status = 2;
                    text_anim.set("SYSTEM LOCKOUT!");
                    btn_action.set_text("RETRY");
                }
            }

            int current_val = field_dial.value();
            int dist = abs(state.target_val - current_val);
            if (dist > 50) dist = 100 - dist;
            int raw_signal = 100 - (dist * 2);

            if (frame_tick % 5 == 0) {
                int noise_amp = 0;
                if (state.level >= 4) {
                    noise_amp = (state.level - 3) * 5;
                    if (noise_amp > 30) noise_amp = 30;
                }
                current_noise = (noise_amp > 0) ? ((rand() % (noise_amp * 2)) - noise_amp) : 0;
            }

            int final_signal = raw_signal + current_noise;
            if (final_signal < 0) final_signal = 0;
            if (final_signal > 100) final_signal = 100;

            if (dist == 0) {
                hack_timer++;
                if (hack_timer >= 60) {
                    state.locks_cracked++;
                    hack_timer = 0;
                    if (state.locks_cracked >= state.locks_total) {
                        game_status = 3;
                        text_anim.set("ACCESS GRANTED");
                        anim_timer = 120;
                        btn_action.set_text("STANDBY...");
                    } else {
                        state.target_val = rand() % 100;
                        text_anim.set("NODE CRACKED");
                        anim_timer = 60;
                    }
                } else {
                    int dots = hack_timer / 15;
                    std::string s = "CRACKING";
                    for (int i = 0; i < dots; i++) s += ".";
                    text_anim.set(s);
                }
            } else {
                hack_timer = 0;
                if (anim_timer > 0) {
                    anim_timer--;
                } else {
                    text_anim.set(">>> SEARCHING <<<");
                }
            }

            if (frame_tick % 5 == 0) {
                bar_time.set_value((state.frames_left * 100) / state.max_frames);
                text_time.set("TRACE: " + to_string_dec_uint(state.frames_left / 60) + "s");

                bar_signal.set_value(final_signal);
                text_signal.set("SIGNAL: " + to_string_dec_uint(final_signal) + "%");

                std::string locks_str = "LOCKS: ";
                for (int i = 0; i < state.locks_total && i < 5; i++) {
                    locks_str += (i < state.locks_cracked) ? "[X]" : "[ ]";
                }
                text_locks.set(locks_str);
                text_level.set("LEVEL: " + to_string_dec_uint(state.level));
            }
        } else if (game_status == 3) {
            if (anim_timer > 0) {
                anim_timer--;
                if (anim_timer == 0) {
                    start_level(state.level + 1);
                }
            }
        }
    }

   private:
    SafeState state{};
    uint32_t frame_tick = 0;
    uint8_t game_status = 0;
    uint8_t hack_timer = 0;
    uint8_t anim_timer = 0;
    int current_noise = 0;

    void start_level(int lvl) {
        state.level = lvl;
        state.locks_total = 2 + lvl;
        if (state.locks_total > 5) state.locks_total = 5;
        state.locks_cracked = 0;

        state.max_frames = (20 + state.locks_total * 10) * 60;
        state.frames_left = state.max_frames;
        state.target_val = rand() % 100;

        hack_timer = 0;
        game_status = 1;

        field_dial.set_value(0);
        text_anim.set("NODE ACQUIRED");
        anim_timer = 60;
        btn_action.set_text("ABORT HACK");
    }

    void reset_ui() {
        text_level.set("LEVEL: -");
        text_locks.set("LOCKS: -");
        bar_time.set_value(100);
        text_time.set("TRACE: OFFLINE");
        bar_signal.set_value(0);
        text_signal.set("SIGNAL: 0%");
        text_anim.set("PRESS START TO BEGIN");
    }

    ui::Labels label_title{
        {{UI_POS_X_CENTER(18), UI_POS_Y(1)}, "SIGNAL SAFECRACKER", ui::Theme::getInstance()->fg_light->foreground}};

    ui::Text text_level{{UI_POS_X_CENTER(10), UI_POS_Y(3), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)}};
    ui::Text text_locks{{UI_POS_X_CENTER(22), UI_POS_Y(4), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};

    ui::ProgressBar bar_time{{UI_POS_X_CENTER(26), UI_POS_Y(6), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_time{{UI_POS_X_CENTER(26), UI_POS_Y(7), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::Text text_left{{UI_POS_X(10), UI_POS_Y(10), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)}, "<<<"};
    ui::NumberField field_dial{{UI_POS_X(14), UI_POS_Y(10)}, 2, {0, 99}, 1, '0', true};
    ui::Text text_right{{UI_POS_X(17), UI_POS_Y(10), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)}, ">>>"};

    ui::ProgressBar bar_signal{{UI_POS_X_CENTER(26), UI_POS_Y(13), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_signal{{UI_POS_X_CENTER(26), UI_POS_Y(14), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::Text text_anim{{UI_POS_X_CENTER(26), UI_POS_Y(16), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::Button btn_action{{UI_POS_X_CENTER(14), UI_POS_Y_BOTTOM(3), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)}, "START HACK"};
};

}  // namespace ui