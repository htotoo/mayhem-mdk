#pragma once

#include "standalone_application.hpp"
#include "ui/ui_widget.hpp"
#include "ui/theme.hpp"
#include "ui/ui_geomap.hpp"
#include "ui/ui_helper.hpp"
#include "ui/ui_navigation.hpp"
#include "standaloneviewmirror.hpp"
#include "file.hpp"
#include <string.h>

namespace ui {

// Ultra-lightweight float parser a RAM kímélése érdekében
static float parse_float_mini(const char* s) {
    int val = 0, sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    int div = 1;
    if (*s == '.') {
        s++;
        int frac_len = 0;
        while (*s >= '0' && *s <= '9' && frac_len < 5) {
            val = val * 10 + (*s - '0');
            div *= 10;
            s++;
            frac_len++;
        }
    }
    return (float)(val * sign) / (float)div;
}

class MapAppView : public ui::GeoMapView {
   public:
    MapAppView(ui::NavigationView& nav)
        : ui::GeoMapView(
              nav,
              "",
              0,
              ui::GeoPos::alt_unit::METERS,
              ui::GeoPos::spd_unit::NONE,
              48.0f,
              19.0f,
              0,
              nullptr) {
        add_child(&text_poi_count);
        load_poi_markers();
    }

   private:
    // A UI_POS_Y_BOTTOM(2) garantálja, hogy egy sorral feljebb kerül, és biztosan a látható képernyőn marad
    ui::Text text_poi_count{{0,
                             UI_POS_Y_BOTTOM(2),
                             UI_POS_MAXWIDTH,
                             UI_POS_HEIGHT(1)}};

    void load_poi_markers() {
        clear_markers();
        File f;
        if (f.open(u"SETTINGS/poi.txt")) {
            text_poi_count.set("NO POI FILE");
            return;
        }

        char buf[512];
        auto res = f.read(buf, sizeof(buf) - 1);
        if (!res.is_ok()) {
            text_poi_count.set("READ ERROR");
            return;
        }

        int len = res.value();
        buf[len] = '\0';

        int loaded_count = 0;
        char* p = buf;

        while (*p && loaded_count < 30) {
            char* line_end = strchr(p, '\n');
            if (line_end)
                *line_end = '\0';
            else if (strchr(p, '\r'))
                *strchr(p, '\r') = '\0';

            char* p1 = strchr(p, ':');
            if (p1) {
                *p1 = '\0';
                char* p2 = strchr(p1 + 1, ':');
                if (p2) {
                    *p2 = '\0';
                    GeoMarker m;
                    m.tag = std::string(p);
                    m.lat = parse_float_mini(p1 + 1);
                    m.lon = parse_float_mini(p2 + 1);
                    m.angle = 361;
                    m.color = ui::Color::red();
                    store_marker(m);
                    loaded_count++;
                }
            }
            if (!line_end) break;
            p = line_end + 1;
        }

        // Szám konvertálása stringgé a snprintf mellőzésével (RAM spórolás)
        char cnt_str[32] = "LOADED POIS: ";
        int i = 13;
        if (loaded_count == 0) {
            cnt_str[i++] = '0';
        } else {
            char tmp[10];
            int ti = 0;
            int num = loaded_count;
            while (num > 0) {
                tmp[ti++] = (num % 10) + '0';
                num /= 10;
            }
            while (ti > 0) {
                cnt_str[i++] = tmp[--ti];
            }
        }
        cnt_str[i] = '\0';

        text_poi_count.set(cnt_str);
    }
};

}  // namespace ui