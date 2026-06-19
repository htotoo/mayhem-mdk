#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/ui_menu.hpp"
#include "ui/ui_geomap.hpp"
#include "ui/ui_textentry.hpp"
#include "file.hpp"
#include <string.h>

namespace ui {

class POIEditorAppView : public ui::View {
   public:
    POIEditorAppView(NavigationView& nav) : nav_(nav) {
        set_style(Theme::getInstance()->bg_dark);
        add_children({&btn_add, &btn_del, &menu_pois});

        btn_add.on_select = [this](Button&) {
            nav_.push<GeoMapView>(0, GeoPos::METERS, GeoPos::HIDDEN, 48.0f, 19.0f,
                                  [this](int32_t, float lat, float lon, int32_t) {
                                      this->pending_lat = lat;
                                      this->pending_lon = lon;
                                      this->needs_prompt = true;
                                  });
        };

        btn_del.on_select = [this](Button&) {
            int selected = menu_pois.highlighted_index();
            if (selected >= 0) {
                auto items = menu_pois.get_menu_items();
                menu_pois.clear();
                for (size_t i = 0; i < items.size(); i++) {
                    if ((int)i != selected) menu_pois.add_item(items[i]);
                }
                save_pois();
            }
        };

        load_pois();
    }

    void focus() override {
        if (needs_prompt) {
            needs_prompt = false;
            static std::string n_buf;
            n_buf = "";
            text_prompt(nav_, n_buf, 15, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string& s) {
                if (!s.empty()) {
                    char line[64];
                    char* ptr = line;
                    int nlen = (s.length() > 20) ? 20 : (int)s.length();
                    memcpy(ptr, s.c_str(), nlen);
                    ptr += nlen;
                    *ptr++ = ':';
                    ptr += float_to_str(ptr, pending_lat);
                    *ptr++ = ':';
                    ptr += float_to_str(ptr, pending_lon);
                    *ptr = '\0';
                    menu_pois.add_item({std::string(line), Color::white(), nullptr, nullptr});
                    save_pois();
                }
            });
        } else {
            menu_pois.focus();
        }
    }

   private:
    NavigationView& nav_;
    Button btn_add{{0, 0, 120, UI_POS_HEIGHT(2)}, "[+] ADD"};
    Button btn_del{{120, 0, 120, UI_POS_HEIGHT(2)}, "[-] DEL"};
    MenuView menu_pois{{0, UI_POS_Y(2), UI_POS_MAXWIDTH, UI_POS_MAXHEIGHT - UI_POS_HEIGHT(2)}};

    float pending_lat = 0;
    float pending_lon = 0;
    bool needs_prompt = false;

    static int float_to_str(char* p, float v) {
        char* start = p;
        if (v < 0) {
            *p++ = '-';
            v = -v;
        }
        int int_part = (int)v;
        int frac_part = (int)((v - (float)int_part) * 10000.0f);
        if (frac_part < 0) frac_part = -frac_part;
        char tmp[10];
        int i = 0;
        if (int_part == 0) {
            tmp[i++] = '0';
        } else {
            while (int_part > 0) {
                tmp[i++] = (int_part % 10) + '0';
                int_part /= 10;
            }
        }
        while (i > 0) *p++ = tmp[--i];
        *p++ = '.';
        for (int j = 3; j >= 0; j--) {
            tmp[j] = (frac_part % 10) + '0';
            frac_part /= 10;
        }
        for (int j = 0; j < 4; j++) *p++ = tmp[j];
        return p - start;
    }

    void load_pois() {
        File f;
        if (!f.open(u"SETTINGS/poi.txt", true, false).is_valid()) {
            char line[64];
            int pos = 0;
            char c;
            auto res = f.read(&c, 1);
            while (res.is_ok() && res.value() == 1) {
                if (c == '\n' || c == '\r') {
                    if (pos > 0) {
                        line[pos] = '\0';
                        menu_pois.add_item({std::string(line), Color::white(), nullptr, nullptr});
                        pos = 0;
                    }
                } else if (pos < 63) {
                    line[pos++] = c;
                }
                // A következő olvasás a ciklus végén
                res = f.read(&c, 1);
            }

            // Maradvány kezelése
            if (pos > 0) {
                line[pos] = '\0';
                menu_pois.add_item({std::string(line), Color::white(), nullptr, nullptr});
            }
        }
    }

    void save_pois() {
        File f;
        if (!f.create(u"SETTINGS/poi.txt")) {
            for (const auto& item : menu_pois.get_menu_items()) {
                f.write_line(item.text);
            }
        }
    }
};

}  // namespace ui