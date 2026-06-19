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

// Memóriakímélő string -> float konverter (atof helyett, hogy férjünk a RAM-ba)
static float parse_float_mini(const char* s) {
    float res = 0.0f;
    float sign = 1.0f;
    if (*s == '-') {
        sign = -1.0f;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        res = res * 10.0f + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        float frac = 1.0f;
        while (*s >= '0' && *s <= '9') {
            frac *= 0.1f;
            res += (*s - '0') * frac;
            s++;
        }
    }
    return res * sign;
}

class MapAppView : public ui::GeoMapView {
   public:
    MapAppView(ui::NavigationView& nav)
        : ui::GeoMapView(
              nav,
              "",                            // tag
              0,                             // altitude
              ui::GeoPos::alt_unit::METERS,  // alt_unit
              ui::GeoPos::spd_unit::NONE,    // spd_unit
              48.0f,                         // kezdő lat (Magyarország)
              19.0f,                         // kezdő lon
              0,                             // angle
              nullptr)                       // on_close callback
    {
        // Hozzáadjuk a számláló szöveget a GeoMapView beépített elemei mellé
        add_children({&text_poi_count});
        load_poi_markers();
    }

   private:
    // A képernyő aljára tesszük (320 - 16 = 304)
    ui::Text text_poi_count{{0, 304, 240, 16}};

    void load_poi_markers() {
        clear_markers();
        File f;
        int loaded_count = 0;
        auto error = f.open(u"SETTINGS/poi.txt");

        if (!error) {
            char line[64];
            int line_len = 0;
            char c;
            auto res = f.read(&c, 1);

            while (res.is_ok() && res.value() == 1) {
                if (c == '\n') {
                    line[line_len] = 0;
                    if (add_marker_from_line(line)) {
                        loaded_count++;
                    }
                    line_len = 0;
                } else if (c != '\r' && line_len < 63) {
                    line[line_len++] = c;
                }
                res = f.read(&c, 1);
            }
            if (line_len > 0) {
                line[line_len] = 0;
                if (add_marker_from_line(line)) {
                    loaded_count++;
                }
            }
        }

        // Memóriakímélő "szám -> szöveg" összerakás snprintf() és std::string nélkül
        char buf[32] = "LOADED POIs: ";
        int num = loaded_count;
        int i = 13;
        if (num == 0) {
            buf[i++] = '0';
        } else {
            char tmp[10];
            int ti = 0;
            while (num > 0) {
                tmp[ti++] = (num % 10) + '0';
                num /= 10;
            }
            while (ti > 0) {
                buf[i++] = tmp[--ti];
            }
        }
        buf[i] = 0;

        text_poi_count.set(buf);
    }

    bool add_marker_from_line(const char* line) {
        const char* p1 = strchr(line, ':');
        if (!p1) return false;
        const char* p2 = strchr(p1 + 1, ':');
        if (!p2) return false;

        float lat = parse_float_mini(p1 + 1);
        float lon = parse_float_mini(p2 + 1);

        char name[16];
        int name_len = p1 - line;
        if (name_len > 15) name_len = 15;
        memcpy(name, line, name_len);
        name[name_len] = 0;

        GeoMarker m;
        m.lat = lat;
        m.lon = lon;
        m.tag = std::string(name);  // A GeoMarker struct miatt itt az az 1 db string konverzió maradhat
        m.color = ui::Color::red();

        store_marker(m);
        return true;
    }
};

}  // namespace ui