#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/string_format.hpp"
#include "ui/ui_helper.hpp"
#include <string.h>
#include <cmath>
#include "file.hpp"
#include "pp_commands.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"

namespace ui {

struct MinerState {
    double money = 0.0;
    double max_rock_hp = 10.0;
    double current_rock_hp = 10.0;
    uint32_t depth = 1;
    uint32_t lvl_power = 1;
    uint32_t lvl_rpm = 1;
    uint32_t lvl_crit_chance = 0;
    uint32_t lvl_crit_multi = 1;
    uint32_t lvl_acid = 0;
};

class JAbyssDrillerView : public ui::View {
   public:
    JAbyssDrillerView(ui::NavigationView& nav) {
        (void)nav;
        set_style(ui::Theme::getInstance()->bg_dark);

        add_children({&label_title,
                      &text_money,
                      &text_depth,
                      &button_overdrive,
                      &text_animation,
                      &progress_bar,
                      &text_upg0, &btn_buy0,
                      &text_upg1, &btn_buy1,
                      &text_upg2, &btn_buy2,
                      &text_upg3, &btn_buy3,
                      &text_upg4, &btn_buy4});

        reset_to_defaults();
        load_state();

        button_overdrive.on_select = [this](ui::Button&) {
            deal_damage((state.lvl_power * 2.0));
            text_animation.set(">>> OVERDRIVE HIT <<<");
            anim_timer = 10;
        };

        btn_buy0.on_select = [this](ui::Button&) { buy_upgrade(0); };
        btn_buy1.on_select = [this](ui::Button&) { buy_upgrade(1); };
        btn_buy2.on_select = [this](ui::Button&) { buy_upgrade(2); };
        btn_buy3.on_select = [this](ui::Button&) { buy_upgrade(3); };
        btn_buy4.on_select = [this](ui::Button&) { buy_upgrade(4); };

        update_upgrades_ui();
        update_stats_ui();

        int hp_percent = (int)((state.current_rock_hp / state.max_rock_hp) * 100.0);
        if (hp_percent < 0) hp_percent = 0;
        if (hp_percent > 100) hp_percent = 100;
        progress_bar.set_value(hp_percent);
        last_drawn_hp_percent = hp_percent;
    }

    ~JAbyssDrillerView() {
        save_state();
        ui::Theme::destroy();
    }

    void focus() override {
        button_overdrive.focus();
    }

    void on_framesync() override {
        frame_tick++;

        if (state.lvl_acid > 0) {
            double acid_dps = state.lvl_acid * 1.5;
            deal_damage(acid_dps / 60.0);
        }

        uint32_t hit_interval = 60 / state.lvl_rpm;
        if (hit_interval < 1) hit_interval = 1;

        double rpm_multi = (state.lvl_rpm > 60) ? (state.lvl_rpm / 60.0) : 1.0;

        if (frame_tick % hit_interval == 0) {
            double dmg = state.lvl_power * rpm_multi;

            if (state.lvl_crit_chance > 0 && (uint32_t)((rand() % 100)) < state.lvl_crit_chance) {
                dmg *= (state.lvl_crit_multi * 2.0);
                text_animation.set("* CRITICAL STRIKE *");
                anim_timer = 15;
            } else {
                if (anim_timer == 0) text_animation.set("v v v DRILLING v v v");
            }

            deal_damage(dmg);
        }

        if (frame_tick % 10800 == 0) {
            save_state();
            text_animation.set("--- GAME SAVED ---");
            anim_timer = 30;
        }

        if (anim_timer > 0) {
            anim_timer--;
            if (anim_timer == 0) text_animation.set("v v v DRILLING v v v");
        }

        if (frame_tick % 5 == 0) {
            update_stats_ui();

            int hp_percent = (int)((state.current_rock_hp / state.max_rock_hp) * 100.0);
            if (hp_percent < 0) hp_percent = 0;
            if (hp_percent > 100) hp_percent = 100;

            if (hp_percent != last_drawn_hp_percent) {
                progress_bar.set_value(hp_percent);
                last_drawn_hp_percent = hp_percent;
            }
        }
    }

   private:
    MinerState state{};
    uint32_t frame_tick = 0;
    uint8_t anim_timer = 0;
    double last_drawn_money = -1.0;
    uint32_t last_drawn_depth = 0;
    int last_drawn_hp_percent = -1;

    void reset_to_defaults() {
        state.money = 0.0;
        state.max_rock_hp = 10.0;
        state.current_rock_hp = 10.0;
        state.depth = 1;
        state.lvl_power = 1;
        state.lvl_rpm = 1;
        state.lvl_crit_chance = 0;
        state.lvl_crit_multi = 1;
        state.lvl_acid = 0;
    }

    void save_state() {
        File f;
        auto error = f.create(u"SETTINGS/iab.stat");
        if (!error) {
            f.write(&state, sizeof(state));
        }
    }

    void load_state() {
        File f;
        auto error = f.open(u"SETTINGS/iab.stat");
        if (!error) {
            MinerState temp_state{};
            auto read_result = f.read(&temp_state, sizeof(MinerState));

            if (read_result.is_ok() && read_result.value() == sizeof(MinerState)) {
                state = temp_state;

                if (state.lvl_power < 1) state.lvl_power = 1;
                if (state.lvl_rpm < 1) state.lvl_rpm = 1;
                if (state.lvl_crit_multi < 1) state.lvl_crit_multi = 1;
                if (state.max_rock_hp <= 0) state.max_rock_hp = 10.0;
                if (state.current_rock_hp <= 0 || state.current_rock_hp > state.max_rock_hp) {
                    state.current_rock_hp = state.max_rock_hp;
                }
                if (state.depth < 1) state.depth = 1;
            } else {
                reset_to_defaults();
            }
        } else {
            reset_to_defaults();
        }
    }

    void deal_damage(double amount) {
        if (amount < 0.0) return;
        state.current_rock_hp -= amount;

        if (state.current_rock_hp <= 0.0) {
            double reward = 5.0 * pow(1.12, state.depth);
            state.money += reward;

            state.depth++;
            state.max_rock_hp = 10.0 * pow(1.15, state.depth);
            state.current_rock_hp = state.max_rock_hp;

            text_animation.set("--- ROCK SHATTERED ---");
            anim_timer = 20;
            update_stats_ui();
        }
    }

    double get_upgrade_cost(int type) {
        switch (type) {
            case 0:
                return 10.0 * pow(1.5, state.lvl_power);
            case 1:
                return 25.0 * pow(1.6, state.lvl_rpm);
            case 2:
                return 100.0 * pow(1.8, state.lvl_crit_chance);
            case 3:
                return 500.0 * pow(2.0, state.lvl_crit_multi);
            case 4:
                return 1000.0 * pow(2.2, state.lvl_acid);
            default:
                return 999999999.0;
        }
    }

    int get_upgrade_level(int type) {
        switch (type) {
            case 0:
                return state.lvl_power;
            case 1:
                return state.lvl_rpm;
            case 2:
                return state.lvl_crit_chance;
            case 3:
                return state.lvl_crit_multi;
            case 4:
                return state.lvl_acid;
            default:
                return 0;
        }
    }

    void buy_upgrade(int type) {
        double cost = get_upgrade_cost(type);
        if (state.money >= cost) {
            state.money -= cost;
            switch (type) {
                case 0:
                    state.lvl_power++;
                    break;
                case 1:
                    state.lvl_rpm++;
                    break;
                case 2:
                    state.lvl_crit_chance++;
                    break;
                case 3:
                    state.lvl_crit_multi++;
                    break;
                case 4:
                    state.lvl_acid++;
                    break;
            }
            update_upgrades_ui();
            update_stats_ui();
        } else {
            text_animation.set("! NOT ENOUGH FUNDS !");
            anim_timer = 15;
        }
    }

    std::string format_large_money(double val) {
        if (val < 0.0) val = 0.0;
        if (val < 1000.0) return "$" + to_string_dec_uint((uint32_t)val);

        uint32_t whole = 0;
        uint32_t frac = 0;
        std::string suffix = "";

        if (val < 1000000.0) {
            whole = (uint32_t)(val / 1000.0);
            frac = (uint32_t)(val / 10.0) % 100;
            suffix = "K";
        } else if (val < 1000000000.0) {
            whole = (uint32_t)(val / 1000000.0);
            frac = (uint32_t)(val / 10000.0) % 100;
            suffix = "M";
        } else if (val < 1000000000000.0) {
            whole = (uint32_t)(val / 1000000000.0);
            frac = (uint32_t)(val / 10000000.0) % 100;
            suffix = "B";
        } else {
            whole = (uint32_t)(val / 1000000000000.0);
            frac = (uint32_t)(val / 10000000000.0) % 100;
            suffix = "T";
        }

        std::string frac_str = to_string_dec_uint(frac);
        if (frac < 10) frac_str = "0" + frac_str;

        return "$" + to_string_dec_uint(whole) + "." + frac_str + suffix;
    }

    std::string build_upgrade_string(int type, std::string name) {
        int lvl = get_upgrade_level(type);
        double cost = get_upgrade_cost(type);
        return name + " L" + to_string_dec_uint(lvl) + "  " + format_large_money(cost);
    }

    void update_stats_ui() {
        if (state.money != last_drawn_money) {
            text_money.set("FUNDS: " + format_large_money(state.money));
            last_drawn_money = state.money;
        }

        if (state.depth != last_drawn_depth) {
            text_depth.set("DEPTH: " + to_string_dec_uint(state.depth) + " m");
            last_drawn_depth = state.depth;
        }
    }

    void update_upgrades_ui() {
        text_upg0.set(build_upgrade_string(0, "POWER"));
        text_upg1.set(build_upgrade_string(1, "SPEED"));
        text_upg2.set(build_upgrade_string(2, "CRIT%"));
        text_upg3.set(build_upgrade_string(3, "CRITx"));
        text_upg4.set(build_upgrade_string(4, "ACID "));
    }

    ui::Labels label_title{
        {{UI_POS_X_CENTER(13), UI_POS_Y(0)}, "ABYSS DRILLER", ui::Theme::getInstance()->fg_light->foreground}};
    ui::Text text_money{{UI_POS_X_CENTER(26), UI_POS_Y(1), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::Text text_depth{{UI_POS_X_CENTER(26), UI_POS_Y(2), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};

    ui::Button button_overdrive{{UI_POS_X_CENTER(16), UI_POS_Y(4), UI_POS_WIDTH(16), UI_POS_HEIGHT(2)}, "MANUAL DRILL"};

    ui::Text text_animation{{UI_POS_X_CENTER(26), UI_POS_Y(7), UI_POS_WIDTH(26), UI_POS_HEIGHT(1)}};
    ui::ProgressBar progress_bar{{UI_POS_X_CENTER(28), UI_POS_Y(8), UI_POS_WIDTH(28), UI_POS_HEIGHT(1)}};

    ui::Text text_upg0{{UI_POS_X(0), UI_POS_Y(10), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};
    ui::Button btn_buy0{{UI_POS_X(23), UI_POS_Y(10), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)}, "BUY"};

    ui::Text text_upg1{{UI_POS_X(0), UI_POS_Y(12), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};
    ui::Button btn_buy1{{UI_POS_X(23), UI_POS_Y(12), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)}, "BUY"};

    ui::Text text_upg2{{UI_POS_X(0), UI_POS_Y(14), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};
    ui::Button btn_buy2{{UI_POS_X(23), UI_POS_Y(14), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)}, "BUY"};

    ui::Text text_upg3{{UI_POS_X(0), UI_POS_Y(16), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};
    ui::Button btn_buy3{{UI_POS_X(23), UI_POS_Y(16), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)}, "BUY"};

    ui::Text text_upg4{{UI_POS_X(0), UI_POS_Y(18), UI_POS_WIDTH(22), UI_POS_HEIGHT(1)}};
    ui::Button btn_buy4{{UI_POS_X(23), UI_POS_Y(18), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)}, "BUY"};
};

}  // namespace ui