/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 Mark Thompson
 *
 * (Optimized for 32-bit float standalone RAM compatibility with explicit paths)
 */

#include "ui_geomap.hpp"

#include <cstring>
#include <string_view>
#include <cmath>  // Only for 32-bit float funcs (sinf, cosf, etc)

#include "string_format.hpp"
#include "complex.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "file_path.hpp"

namespace ui {

static const float mercator_lut[86] = {
    0.000000f, 0.017455f, 0.034915f, 0.052402f, 0.069922f, 0.087489f, 0.105112f, 0.122802f,
    0.140569f, 0.158424f, 0.176378f, 0.194441f, 0.212627f, 0.230948f, 0.249416f, 0.268045f,
    0.286848f, 0.305840f, 0.325035f, 0.344449f, 0.364098f, 0.383998f, 0.404168f, 0.424625f,
    0.445388f, 0.466477f, 0.487913f, 0.509718f, 0.531913f, 0.554522f, 0.577570f, 0.601083f,
    0.625088f, 0.649613f, 0.674686f, 0.700340f, 0.726605f, 0.753512f, 0.781096f, 0.809393f,
    0.838446f, 0.868301f, 0.899003f, 0.930602f, 0.963148f, 0.996695f, 1.031301f, 1.067026f,
    1.103934f, 1.142095f, 1.181589f, 1.222501f, 1.264923f, 1.308953f, 1.354694f, 1.402256f,
    1.451756f, 1.503322f, 1.557088f, 1.613198f, 1.671804f, 1.733072f, 1.797178f, 1.864319f,
    1.934707f, 2.008568f, 2.086146f, 2.167705f, 2.253527f, 2.343924f, 2.439229f, 2.539803f,
    2.646039f, 2.758362f, 2.877242f, 3.003189f, 3.136773f, 3.278635f, 3.429497f, 3.590150f,
    3.761483f, 3.944481f, 4.140220f, 4.350005f, 4.575389f, 4.818048f};

// Gyors float abszolút érték beépített abs() helyett
static inline float abs_f(float v) {
    return (v < 0.0f) ? -v : v;
}

GeoPos::GeoPos(
    const Point pos,
    const alt_unit altitude_unit,
    const spd_unit speed_unit)
    : altitude_unit_(altitude_unit), speed_unit_(speed_unit) {
    set_parent_rect({pos, {UI_POS_MAXWIDTH, UI_POS_HEIGHT(3)}});

    add_children({&labels_position, &label_spd_position, &field_altitude, &field_speed,
                  &text_alt_unit, &text_speed_unit, &field_lat_degrees, &field_lat_minutes,
                  &field_lat_seconds, &text_lat_decimal, &field_lon_degrees, &field_lon_minutes,
                  &field_lon_seconds, &text_lon_decimal});

    set_altitude(0);
    set_speed(0);
    set_lat(0);
    set_lon(0);

    const auto changed_fn = [this](int32_t) {
        float lat_value = lat();
        float lon_value = lon();
        text_lat_decimal.set(to_string_decimal(lat_value, 5));
        text_lon_decimal.set(to_string_decimal(lon_value, 5));
        if (on_change && report_change)
            on_change(altitude(), lat_value, lon_value, speed());
    };

    field_altitude.on_change = changed_fn;
    field_speed.on_change = changed_fn;
    field_lat_degrees.on_change = changed_fn;
    field_lat_minutes.on_change = changed_fn;
    field_lat_seconds.on_change = changed_fn;
    field_lon_degrees.on_change = changed_fn;
    field_lon_minutes.on_change = changed_fn;
    field_lon_seconds.on_change = changed_fn;

    const auto wrapped_lat_seconds = [this](int32_t v) { field_lat_minutes.on_encoder(v); };
    const auto wrapped_lat_minutes = [this](int32_t v) { field_lat_degrees.on_encoder((field_lat_degrees.value() >= 0) ? v : -v); };
    const auto wrapped_lon_seconds = [this](int32_t v) { field_lon_minutes.on_encoder(v); };
    const auto wrapped_lon_minutes = [this](int32_t v) { field_lon_degrees.on_encoder((field_lon_degrees.value() >= 0) ? v : -v); };

    field_lat_seconds.on_wrap = wrapped_lat_seconds;
    field_lat_minutes.on_wrap = wrapped_lat_minutes;
    field_lon_seconds.on_wrap = wrapped_lon_seconds;
    field_lon_minutes.on_wrap = wrapped_lon_minutes;

    text_alt_unit.set(altitude_unit_ ? "m" : "ft");
    if (speed_unit_ == KMPH) text_speed_unit.set("kmph");
    if (speed_unit_ == MPH) text_speed_unit.set("mph");
    if (speed_unit_ == KNOTS) text_speed_unit.set("knots");
    if (speed_unit_ == HIDDEN) {
        text_speed_unit.hidden(true);
        label_spd_position.hidden(true);
        field_speed.hidden(true);
    }
}

void GeoPos::set_read_only(bool v) {
    field_altitude.set_focusable(!v);
    field_speed.set_focusable(!v);
}

void GeoPos::set_report_change(bool v) {
    report_change = v;
}

void GeoPos::focus() {
    if (field_altitude.focusable())
        field_altitude.focus();
    else
        field_lat_degrees.focus();
}

void GeoPos::hide_altandspeed() {
    field_altitude.set_style(Theme::getInstance()->fg_medium);
    field_speed.set_style(Theme::getInstance()->fg_medium);
}

void GeoPos::set_altitude(int32_t altitude) {
    field_altitude.set_value(altitude);
}
void GeoPos::set_speed(int32_t speed) {
    field_speed.set_value(speed);
}

void GeoPos::set_lat(float lat) {
    field_lat_degrees.set_value(lat);
    field_lat_minutes.set_value((uint32_t)abs_f(lat / (1.0f / 60.0f)) % 60);
    field_lat_seconds.set_value((uint32_t)abs_f(lat / (1.0f / 3600.0f)) % 60);
}

void GeoPos::set_lon(float lon) {
    field_lon_degrees.set_value(lon);
    field_lon_minutes.set_value((uint32_t)abs_f(lon / (1.0f / 60.0f)) % 60);
    field_lon_seconds.set_value((uint32_t)abs_f(lon / (1.0f / 3600.0f)) % 60);
}

float GeoPos::lat() {
    if (field_lat_degrees.value() < 0) {
        return -1.0f * ((float)-field_lat_degrees.value() + (field_lat_minutes.value() / 60.0f) + (field_lat_seconds.value() / 3600.0f));
    } else {
        return (float)field_lat_degrees.value() + (field_lat_minutes.value() / 60.0f) + (field_lat_seconds.value() / 3600.0f);
    }
};

float GeoPos::lon() {
    if (field_lon_degrees.value() < 0) {
        return -1.0f * ((float)-field_lon_degrees.value() + (field_lon_minutes.value() / 60.0f) + (field_lon_seconds.value() / 3600.0f));
    } else {
        return (float)field_lon_degrees.value() + (field_lon_minutes.value() / 60.0f) + (field_lon_seconds.value() / 3600.0f);
    }
};

int32_t GeoPos::altitude() {
    return field_altitude.value();
};
int32_t GeoPos::speed() {
    return field_speed.value();
};

GeoMap::GeoMap(Rect parent_rect) : Widget{parent_rect}, markerListLen(0) {
    has_osm = use_osm = find_osm_file_tile();
}

bool GeoMap::on_encoder(const EncoderEvent delta) {
    if (delta > 0) {
        if (map_zoom < MAX_MAP_ZOOM_IN) {
            if (map_zoom == -2)
                map_zoom = 1;
            else
                map_zoom += (map_zoom >= MAP_ZOOM_RESOLUTION_LIMIT) ? map_zoom : 1;
        }
        map_osm_zoom++;
        if (has_osm) set_osm_max_zoom(true);
    } else if (delta < 0) {
        if (map_zoom > -MAX_MAP_ZOOM_OUT) {
            if (map_zoom == 1)
                map_zoom = -2;
            else {
                if (map_zoom > MAP_ZOOM_RESOLUTION_LIMIT)
                    map_zoom /= 2;
                else
                    map_zoom--;
            }
        }
        if (map_osm_zoom > 0) map_osm_zoom--;
        if (has_osm) set_osm_max_zoom(true);
    } else {
        return false;
    }

    map_visible = map_opened && (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT);
    if (use_osm) {
        map_visible = true;
        zoom_pixel_offset = 0.0f;
    } else {
        zoom_pixel_offset = (map_visible && (map_zoom > 1)) ? (float)map_zoom / 2.0f : 0.0f;
    }
    redraw_map = true;
    set_dirty();
    return true;
}

void GeoMap::map_read_line_bin(ui::Color* buffer, uint16_t pixels) {
    const auto r = screen_rect();
    ui::Dim width = r.width();
    if (map_zoom == 1) {
        map_file.read(buffer, pixels << 1);
    } else if (map_zoom > 1) {
        uint16_t src_pixels_needed = (pixels + map_zoom - 1) / map_zoom;
        if (src_pixels_needed == 0) src_pixels_needed = 1;
        uint16_t src_offset = pixels - src_pixels_needed;
        map_file.read(&buffer[src_offset], src_pixels_needed << 1);
        for (uint16_t dst = 0; dst < pixels; ++dst) {
            uint16_t i0 = dst / map_zoom;
            uint16_t i1 = i0 + 1;
            if (i1 >= src_pixels_needed) i1 = src_pixels_needed - 1;
            uint16_t frac = dst % map_zoom;
            ui::Color c0 = buffer[src_offset + i0];
            ui::Color c1 = buffer[src_offset + i1];
            if (frac == 0) {
                buffer[dst] = c0;
            } else {
                uint32_t inv_frac = map_zoom - frac;
                uint8_t color_r = ((uint32_t)c0.r() * inv_frac + (uint32_t)c1.r() * frac) / map_zoom;
                uint8_t color_g = ((uint32_t)c0.g() * inv_frac + (uint32_t)c1.g() * frac) / map_zoom;
                uint8_t color_b = ((uint32_t)c0.b() * inv_frac + (uint32_t)c1.b() * frac) / map_zoom;
                buffer[dst] = ui::Color(color_r, color_g, color_b);
            }
        }
    } else {
        const int skip = -map_zoom;
        const int MAX_BUFFER_ELEMENTS = 256;
        ui::Color zoom_out_buffer[MAX_BUFFER_ELEMENTS];
        const int total_elements_needed = pixels * skip;
        const int chunk_size = total_elements_needed < MAX_BUFFER_ELEMENTS ? total_elements_needed : MAX_BUFFER_ELEMENTS;
        int target_i = 0;
        int current_file_offset = 0;
        while (target_i < width && current_file_offset < total_elements_needed) {
            int read_size = chunk_size < (total_elements_needed - current_file_offset) ? chunk_size : (total_elements_needed - current_file_offset);
            map_file.read(zoom_out_buffer, read_size << 1);
            int first_valid_index = 0;
            int offset_modulo = current_file_offset % skip;
            if (offset_modulo != 0) first_valid_index = skip - offset_modulo;
            for (int j = first_valid_index; j < read_size; j += skip) {
                if (target_i < width) {
                    buffer[target_i] = zoom_out_buffer[j];
                    target_i++;
                }
            }
            current_file_offset += read_size;
        }
    }
}

void GeoMap::draw_markers(Painter& painter) {
    for (int i = 0; i < markerListLen; ++i) {
        draw_marker_item(painter, markerList[i], markerList[i].color, markerList[i].color, Color::magenta());
    }
}

void GeoMap::draw_marker_item(Painter& painter, GeoMarker& item, const Color color, const Color fontColor, const Color backColor) {
    const auto r = screen_rect();
    const ui::Point itemPoint = item_rect_pixel(item);
    if ((itemPoint.x() >= 0) && (itemPoint.x() < r.width()) &&
        (itemPoint.y() > 10) && (itemPoint.y() < r.height())) {
        draw_marker(painter, {itemPoint.x() + r.left(), itemPoint.y() + r.top()}, item.angle, item.tag, color, fontColor, backColor);
    }
}

ui::Point GeoMap::item_rect_pixel(GeoMarker& item) {
    if (!use_osm) {
        const auto r = screen_rect();
        const auto geomap_rect_half_width = r.width() / 2;
        const auto geomap_rect_half_height = r.height() / 2;
        GeoPoint mapPoint = lat_lon_to_map_pixel(item.lat, item.lon);
        float x = mapPoint.x - x_pos;
        float y = mapPoint.y - y_pos;
        if (map_zoom > 1) {
            x = x * map_zoom + zoom_pixel_offset;
            y = y * map_zoom + zoom_pixel_offset;
        } else if (map_zoom < 0) {
            x = x / (float)(-map_zoom);
            y = y / (float)(-map_zoom);
        }
        x += geomap_rect_half_width;
        y += geomap_rect_half_height;
        return {(int16_t)x, (int16_t)y};
    }
    float y = lat_to_pixel_y_tile(item.lat, map_osm_real_zoom) - viewport_top_left_py;
    float x = lon_to_pixel_x_tile(item.lon, map_osm_real_zoom) - viewport_top_left_px;
    return {(int16_t)x, (int16_t)y};
}

int GeoMap::lon2tile(float lon, int zoom) {
    return (int)floorf((lon + 180.0f) / 360.0f * powf(2.0f, zoom));
}

int GeoMap::lat2tile(float lat, int zoom) {
    float lat_rad = lat * 3.14159265f / 180.0f;
    return (int)floorf((1.0f - logf(tanf(lat_rad) + 1.0f / cosf(lat_rad)) / 3.14159265f) / 2.0f * powf(2.0f, zoom));
}

void GeoMap::set_osm_max_zoom(bool changeboth) {
    if (map_osm_zoom > 20) map_osm_zoom = 20;
    for (uint8_t i = map_osm_zoom; i > 0; i--) {
        int tile_x = lon2tile(lon_, i);
        int tile_y = lat2tile(lat_, i);

        char path_buffer[64];
        char* p = path_buffer;

        // FIX: Remove leading slash for Standalone app FatFS
        strcpy(p, "OSM/");
        p += 4;
        p = append_int_geomap(p, i);
        *p++ = '/';
        p = append_int_geomap(p, tile_x);
        *p++ = '/';
        p = append_int_geomap(p, tile_y);
        strcpy(p, ".bmp");

        std::filesystem::path file_path(path_buffer);
        if (file_exists(file_path)) {
            map_osm_real_zoom = i;
            if (changeboth) map_osm_zoom = i;
            return;
        }
    }
    if (changeboth) map_osm_zoom = 0;
    map_osm_real_zoom = 0;
}

uint8_t GeoMap::find_osm_file_tile() {
    // FIX: Using explicit string literal and removed leading slash
    std::filesystem::path file_path(u"OSM/0/0/0.bmp");
    if (file_exists(file_path)) return 1;
    return 0;
}

GeoPoint GeoMap::lat_lon_to_map_pixel(float lat, float lon) {
    float x = (map_width * (lon + 180.0f) / 360.0f);
    float abs_lat = abs_f(lat);
    if (abs_lat > 85.0f) abs_lat = 85.0f;

    int lat_idx = (int)abs_lat;
    float frac = abs_lat - lat_idx;

    float mercator_val = mercator_lut[lat_idx] + frac * (mercator_lut[lat_idx + 1] - mercator_lut[lat_idx]);
    if (lat < 0.0f) mercator_val = -mercator_val;
    float y = (map_height - ((map_world_lon * mercator_val) - map_offset));
    return {x, y};
}

void GeoMap::draw_map_grid(ui::Rect r, Painter& painter) {
    int grid_spacing = map_zoom * 2;
    int x = (r.width() / 2) % grid_spacing;
    int y = (r.height() / 2) % grid_spacing;

    if (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT) return;

    painter.fill_rectangle({{0, r.top()}, {r.width(), r.height()}}, Theme::getInstance()->bg_darkest->background);

    for (uint16_t line = y; line < r.height(); line += grid_spacing) {
        painter.fill_rectangle({{0, r.top() + line}, {r.width(), 1}}, Theme::getInstance()->bg_darker->background);
    }
    for (uint16_t column = x; column < r.width(); column += grid_spacing) {
        painter.fill_rectangle({{column, r.top()}, {1, r.height()}}, Theme::getInstance()->bg_darker->background);
    }
}

float GeoMap::tile_pixel_x_to_lon(int x, int zoom) {
    float map_w = powf(2.0f, zoom) * TILE_SIZE;
    return ((float)x / map_w * 360.0f) - 180.0f;
}

float GeoMap::tile_pixel_y_to_lat(int y, int zoom) {
    float map_h = powf(2.0f, zoom) * TILE_SIZE;
    float n = 3.14159265f * (1.0f - 2.0f * (float)y / map_h);
    return atanf(sinhf(n)) * 180.0f / 3.14159265f;
}

float GeoMap::lon_to_pixel_x_tile(float lon, int zoom) {
    return ((lon + 180.0f) / 360.0f) * powf(2.0f, zoom) * TILE_SIZE;
}

float GeoMap::lat_to_pixel_y_tile(float lat, int zoom) {
    float lat_rad = lat * 3.14159265f / 180.0f;
    float sin_lat = sinf(lat_rad);
    return ((1.0f - logf((1.0f + sin_lat) / (1.0f - sin_lat)) / (2.0f * 3.14159265f)) / 2.0f) * powf(2.0f, zoom) * TILE_SIZE;
}

bool GeoMap::draw_osm_file(int zoom, int tile_x, int tile_y, int relative_x, int relative_y, Painter& painter) {
    const auto r = screen_rect();
    if (relative_x >= r.width() || relative_y >= r.height() ||
        relative_x + TILE_SIZE <= 0 || relative_y + TILE_SIZE <= 0) {
        return true;
    }

    BMPFile* bmp = bmp_cache.get(zoom, tile_x, tile_y);
    int src_x = 0;
    int src_y = 0;
    int dest_x = relative_x;
    int dest_y = relative_y;
    int clip_w = TILE_SIZE;
    int clip_h = TILE_SIZE;
    if (dest_x < 0) {
        src_x = -dest_x;
        clip_w += dest_x;
        dest_x = 0;
    }
    if (dest_y < 0) {
        src_y = -dest_y;
        clip_h += dest_y;
        dest_y = 0;
    }
    if (dest_x + clip_w > r.width()) {
        clip_w = r.width() - dest_x;
    }
    if (dest_y + clip_h > r.height()) {
        clip_h = r.height() - dest_y;
    }
    if (clip_w <= 0 || clip_h <= 0) return true;

    if (!bmp || !bmp->is_loaded()) {
        ui::Rect error_rect{{dest_x + r.left(), dest_y + r.top()}, {(ui::Dim)clip_w, (ui::Dim)clip_h}};
        painter.fill_rectangle(error_rect, Theme::getInstance()->bg_darkest->background);
        return false;
    }

    map_line_buffer.resize(clip_w);

    if (bmp->is_bottomup()) {
        for (int y = clip_h - 1; y >= 0; --y) {
            int source_row = src_y + y;
            int dest_row = dest_y + y;
            bmp->seek(src_x, source_row);
            bmp->read_next_px_cnt(map_line_buffer.data(), clip_w, false);
            painter.draw_pixels({dest_x + r.left(), dest_row + r.top(), (ui::Dim)clip_w, 1}, map_line_buffer);
        }
    } else {
        for (int y = 0; y < clip_h; ++y) {
            int source_row = src_y + y;
            int dest_row = dest_y + y;
            bmp->seek(src_x, source_row);
            bmp->read_next_px_cnt(map_line_buffer.data(), clip_w, false);
            painter.draw_pixels({dest_x + r.left(), dest_row + r.top(), (ui::Dim)clip_w, 1}, map_line_buffer);
        }
    }

#ifndef _USE_GEOMAP_BMP_CACHE
    delete bmp;
#endif
    return true;
}

void GeoMap::paint(Painter& painter) {
    const auto r = screen_rect();
    int16_t zoom_seek_x, zoom_seek_y;

    if (!use_osm) {
        map_line_buffer.resize(r.width());
        if (map_zoom <= 1) {
            const float min_diff = abs_f((float)map_zoom);
            if (abs_f(x_pos - prev_x_pos) >= min_diff)
                redraw_map = true;
            else if (abs_f(y_pos - prev_y_pos) >= min_diff)
                redraw_map = true;
        } else {
            if ((abs_f(x_pos - prev_x_pos) * map_zoom) >= 1.0f)
                redraw_map = true;
            else if ((abs_f(y_pos - prev_y_pos) * map_zoom) >= 1.0f)
                redraw_map = true;
        }
    }

    if (redraw_map) {
        redraw_map = false;
        if (map_visible) {
            if (!use_osm) {
                prev_x_pos = x_pos;
                prev_y_pos = y_pos;

                if (map_zoom > 1) {
                    zoom_seek_x = x_pos - (float)r.width() / (2.0f * map_zoom);
                    zoom_seek_y = y_pos - (float)r.height() / (2.0f * map_zoom);
                } else {
                    zoom_seek_x = x_pos - (r.width() * abs_f((float)map_zoom)) / 2.0f;
                    zoom_seek_y = y_pos - (r.height() * abs_f((float)map_zoom)) / 2.0f;
                }

                int duplicate_lines = (map_zoom < 0) ? 1 : map_zoom;
                for (uint16_t line = 0; line < (r.height() / duplicate_lines); line++) {
                    uint16_t seek_line = zoom_seek_y + ((map_zoom >= 0) ? line : line * (-map_zoom));
                    map_file.seek(4 + ((zoom_seek_x + (map_width * seek_line)) << 1));
                    map_read_line_bin(map_line_buffer.data(), r.width());
                    for (uint16_t j = 0; j < duplicate_lines; j++) {
                        painter.draw_pixels({0, r.top() + (line * duplicate_lines) + j, (ui::Dim)r.width(), 1}, map_line_buffer);
                    }
                }

            } else {
                float global_center_px = lon_to_pixel_x_tile(lon_, map_osm_real_zoom);
                float global_center_py = lat_to_pixel_y_tile(lat_, map_osm_real_zoom);

                viewport_top_left_px = global_center_px - (r.width() / 2.0f);
                viewport_top_left_py = global_center_py - (r.height() / 2.0f);

                int start_tile_x = (int)floorf(viewport_top_left_px / TILE_SIZE);
                int start_tile_y = (int)floorf(viewport_top_left_py / TILE_SIZE);

                float render_offset_x = -(viewport_top_left_px - (start_tile_x * TILE_SIZE));
                float render_offset_y = -(viewport_top_left_py - (start_tile_y * TILE_SIZE));

                int tiles_needed_x = (r.width() / TILE_SIZE) + 2;
                int tiles_needed_y = (r.height() / TILE_SIZE) + 2;

                for (int y = 0; y < tiles_needed_y; ++y) {
                    for (int x = 0; x < tiles_needed_x; ++x) {
                        int current_tile_x = start_tile_x + x;
                        int current_tile_y = start_tile_y + y;

                        int draw_pos_x = (int)roundf(render_offset_x + x * TILE_SIZE);
                        int draw_pos_y = (int)roundf(render_offset_y + y * TILE_SIZE);
                        draw_osm_file(map_osm_real_zoom, current_tile_x, current_tile_y, draw_pos_x, draw_pos_y, painter);
                    }
                }
            }

        } else {
            draw_map_grid(r, painter);
        }

        if (manual_panning_) {
            painter.fill_rectangle({r.center() - Point(16, 1) + Point(zoom_pixel_offset, zoom_pixel_offset), {32, 2}}, Color::red());
            painter.fill_rectangle({r.center() - Point(1, 16) + Point(zoom_pixel_offset, zoom_pixel_offset), {2, 32}}, Color::red());
        }

        draw_markers(painter);
        if (!use_osm) draw_scale(painter);
        draw_mypos(painter);
        if (has_osm) draw_switcher(painter);
        set_clean();
    }

    if (!manual_panning_ && !hide_center_marker_) {
        draw_marker(painter, r.center() + Point(zoom_pixel_offset, zoom_pixel_offset), angle_, tag_, Color::red(), Color::white(), Color::black());
    }
}

void GeoMap::draw_switcher(Painter& painter) {
    painter.fill_rectangle({screen_rect().left(), screen_rect().top(), 3 * 20, 20}, Theme::getInstance()->bg_darker->background);
    std::string_view txt = (use_osm) ? "B I N" : "O S M";
    painter.draw_string({screen_rect().left() + 5, screen_rect().top() + 2}, *Theme::getInstance()->fg_light, txt);
}

bool GeoMap::on_keyboard(KeyboardEvent key) {
    if (key == '+' || key == ' ') return on_encoder(1);
    if (key == '-') return on_encoder(-1);
    return false;
}

bool GeoMap::on_touch(const TouchEvent event) {
    if (has_osm && event.type == TouchEvent::Type::Start && event.point.x() < screen_rect().left() + 3 * 20 && event.point.y() < screen_rect().top() + 20) {
        use_osm = !use_osm;
        move(lon_, lat_);
        if (use_osm) set_osm_max_zoom();
        redraw_map = true;
        set_dirty();
        return false;
    }

    if ((event.type == TouchEvent::Type::Start) && (mode_ == PROMPT)) {
        Point p;
        set_highlighted(true);
        if (on_move) {
            if (!use_osm) {
                p = event.point - screen_rect().center();
                on_move(p.x() / 2.0f * lon_ratio, p.y() / 2.0f * lat_ratio, false);
            } else {
                p = event.point - screen_rect().location();
                on_move(tile_pixel_x_to_lon(p.x() + viewport_top_left_px, map_osm_real_zoom), tile_pixel_y_to_lat(p.y() + viewport_top_left_py, map_osm_real_zoom), true);
            }
            return true;
        }
    }
    return false;
}

void GeoMap::move(const float lon, const float lat) {
    const auto r = screen_rect();
    bool is_changed = (lon_ != lon || lat_ != lat);
    lon_ = lon;
    lat_ = lat;
    if (!use_osm) {
        GeoPoint mapPoint = lat_lon_to_map_pixel(lat_, lon_);
        x_pos = mapPoint.x;
        y_pos = mapPoint.y;
        if (x_pos > (map_width - r.width() / 2)) x_pos = map_width - r.width() / 2;
        if (y_pos > (map_height + r.height() / 2)) y_pos = map_height - r.height() / 2;

        float km_per_deg_lon = cosf(lat * 3.14159265f / 180.0f) * 111.321f;
        pixels_per_km = (r.width() / 2.0f) / km_per_deg_lon;
    } else {
        if (is_changed) {
            set_osm_max_zoom();
            float global_center_px = lon_to_pixel_x_tile(lon_, map_osm_real_zoom);
            float global_center_py = lat_to_pixel_y_tile(lat_, map_osm_real_zoom);
            if (abs_f(global_center_px - (r.width() / 2.0f) - viewport_top_left_px) >= 1.0f ||
                abs_f(global_center_py - (r.height() / 2.0f) - viewport_top_left_py) >= 1.0f) {
                redraw_map = true;
            }
        }
    }
}

bool GeoMap::init() {
    // FIX: Hardcoded path directly to ADSB/world_map.bin
    auto result = map_file.open(u"ADSB/world_map.bin");
    map_opened = !result.is_valid();

    if (map_opened) {
        map_file.read(&map_width, 2);
        map_file.read(&map_height, 2);
    } else {
        map_width = 32768;
        map_height = 32768;
    }

    map_visible = map_opened || has_osm;
    map_center_x = map_width >> 1;
    map_center_y = map_height >> 1;

    lon_ratio = 180.0f / map_center_x;
    lat_ratio = -90.0f / map_center_y;

    map_bottom = sinf(-85.05f * 3.14159265f / 180.0f);
    map_world_lon = map_width / (2.0f * 3.14159265f);
    map_offset = (map_world_lon / 2.0f * logf((1.0f + map_bottom) / (1.0f - map_bottom)));
    return map_opened;
}

void GeoMap::set_mode(GeoMapMode mode) {
    mode_ = mode;
}
void GeoMap::set_manual_panning(bool v) {
    manual_panning_ = v;
}
bool GeoMap::manual_panning() {
    return manual_panning_;
}

void GeoMap::draw_scale(Painter& painter) {
    const auto r = screen_rect();
    uint32_t m = 800000;
    uint32_t scale_width = (map_zoom > 0) ? (uint32_t)(m * map_zoom * pixels_per_km) : (uint32_t)(m * pixels_per_km / (float)(-map_zoom));
    ui::Color scale_color = (map_visible) ? Color::black() : Color::white();
    std::string km_string;

    while (scale_width > (uint32_t)r.width() * (1000 / 2)) {
        scale_width /= 2;
        m /= 2;
    }
    scale_width /= 1000;
    if (m < 1000) {
        km_string = to_string_dec_uint(m) + "m";
    } else {
        m += 50;
        uint32_t km = m / 1000;
        m -= km * 1000;
        if (m == 0) {
            km_string = to_string_dec_uint(km) + " km";
        } else {
            km_string = to_string_dec_uint(km) + "." + to_string_dec_uint(m / 100, 1) + "km";
        }
    }

    painter.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 4}, {(ui::Dim)scale_width, 2}}, scale_color);
    painter.fill_rectangle({{r.right() - 5, r.bottom() - 8}, {2, 6}}, scale_color);
    painter.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 8}, {2, 6}}, scale_color);
    std::string_view sw = km_string;
    ui::Point pos = {(uint16_t)(r.right() - 25 - scale_width - sw.length() * 5 / 2), r.bottom() - 10};
    painter.draw_string(pos, *Theme::getInstance()->fg_light, sw);
}

void GeoMap::draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color, Painter& painter) {
    Point arrow_a, arrow_b, arrow_c;
    for (size_t thickness = 0; thickness < 3; thickness++) {
        arrow_a = fast_polar_to_point((int)angle, size) + origin;
        arrow_b = fast_polar_to_point((int)(angle + 180 - 35), size) + origin;
        arrow_c = fast_polar_to_point((int)(angle + 180 + 35), size) + origin;
        painter.draw_line(arrow_a, arrow_b, color);
        painter.draw_line(arrow_b, arrow_c, color);
        painter.draw_line(arrow_c, arrow_a, color);
        size--;
    }
    painter.draw_pixel(origin, color);
}

void GeoMap::draw_marker(Painter& painter, const ui::Point itemPoint, const uint16_t itemAngle, const std::string& itemTag, const Color color, const Color fontColor, const Color backColor) {
    const auto r = screen_rect();
    int tagOffset = 10;
    if (mode_ == PROMPT) {
        painter.fill_rectangle({itemPoint - Point(16, 1), {32, 2}}, color);
        painter.fill_rectangle({itemPoint - Point(1, 16), {2, 32}}, color);
        tagOffset = 16;
    } else if (angle_ < 360) {
        draw_bearing(itemPoint, itemAngle, 10, color, painter);
        tagOffset = 10;
    } else {
        painter.fill_rectangle({itemPoint - Point(8, 1), {16, 2}}, color);
        painter.fill_rectangle({itemPoint - Point(1, 8), {2, 16}}, color);
        tagOffset = 8;
    }
    if ((itemPoint.y() - r.top() >= 32) && (itemTag.find_first_not_of(' ') != itemTag.npos)) {
        painter.draw_string(itemPoint - Point(((int)itemTag.length() * 8 / 2), 14 + tagOffset),
                            style().font, fontColor, backColor, itemTag);
    }
}

void GeoMap::draw_mypos(Painter& painter) {
    if ((my_pos.lat < INVALID_LAT_LON) && (my_pos.lon < INVALID_LAT_LON))
        draw_marker_item(painter, my_pos, Color::yellow());
}

void GeoMap::clear_markers() {
    markerListLen = 0;
}

MapMarkerStored GeoMap::store_marker(GeoMarker& marker) {
    const auto r = screen_rect();
    MapMarkerStored ret;
    bool can_see_on_map = true;
    if (!use_osm) {
        GeoPoint mapPoint = lat_lon_to_map_pixel(marker.lat, marker.lon);
        int x_dist = (int)abs_f(mapPoint.x - x_pos);
        int y_dist = (int)abs_f(mapPoint.y - y_pos);
        int zoom_out = (map_zoom < 0) ? -map_zoom : 1;
        if ((x_dist >= (zoom_out * r.width() / 2)) || (y_dist >= (zoom_out * r.height() / 2))) {
            can_see_on_map = false;
        }
    }

    if (!can_see_on_map) {
        ret = MARKER_NOT_STORED;
    } else if (markerListLen < NumMarkerListElements) {
        markerList[markerListLen] = marker;
        markerListLen++;
        redraw_map = true;
        ret = MARKER_STORED;
    } else {
        ret = MARKER_LIST_FULL;
    }
    return ret;
}

void GeoMap::update_my_position(float lat, float lon, int32_t altitude) {
    bool is_changed = (my_pos.lat != lat) || (my_pos.lon != lon);
    my_pos.lat = lat;
    my_pos.lon = lon;
    my_altitude = altitude;
    redraw_map |= is_changed;
    set_dirty();
}

void GeoMap::update_my_orientation(uint16_t angle, bool refresh) {
    bool is_changed = (my_pos.angle != angle);
    my_pos.angle = angle;
    if (refresh && is_changed) {
        redraw_map = true;
        set_dirty();
    }
}

MapType GeoMap::get_map_type() {
    return use_osm ? MAP_TYPE_OSM : MAP_TYPE_BIN;
}

void GeoMapView::focus() {
    geopos.focus();
    if (!geomap.map_file_opened()) {
        nav_.display_modal("No map", "No world_map.bin file in\nADSB directory", ui::ABORT);
    }
}

void GeoMapView::update_my_position(float lat, float lon, int32_t altitude) {
    geomap.update_my_position(lat, lon, altitude);
}
void GeoMapView::update_my_orientation(uint16_t angle, bool refresh) {
    geomap.update_my_orientation(angle, refresh);
}

void GeoMapView::update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed) {
    if (geomap.manual_panning()) {
        geomap.set_dirty();
        return;
    }
    bool is_changed = lat_ != lat || lon_ != lon || altitude_ != altitude || speed_ != speed || angle_ != angle;
    lat_ = lat;
    lon_ = lon;
    altitude_ = altitude;
    speed_ = speed;

    geopos.set_report_change(false);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);
    geopos.set_altitude(altitude_);
    geopos.set_speed(speed_);
    geopos.set_report_change(true);

    geomap.set_angle(angle);
    if (is_changed) {
        geomap.move(lon_, lat_);
        geomap.set_dirty();
    }
}

void GeoMapView::update_tag(const std::string tag) {
    geomap.set_tag(tag);
}

void GeoMapView::setup() {
    add_child(&geomap);
    geopos.set_altitude(altitude_);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);

    geopos.on_change = [this](int32_t altitude, float lat, float lon, int32_t speed) {
        bool is_changed = (altitude_ != altitude) || (lat_ != lat) || (lon_ != lon) || (speed_ != speed);
        altitude_ = altitude;
        lat_ = lat;
        lon_ = lon;
        speed_ = speed;
        geopos.hide_altandspeed();
        geomap.set_manual_panning(true);
        if (is_changed) geomap.move(lon_, lat_);
        geomap.set_dirty();
    };

    geomap.on_move = [this](float move_x, float move_y, bool absolute) {
        if (absolute) {
            lon_ = move_x;
            lat_ = move_y;
        } else {
            lon_ += move_x;
            lat_ += move_y;
        }

        geopos.set_report_change(false);
        geopos.set_lon(lon_);
        geopos.set_lat(lat_);
        geopos.set_report_change(true);

        geomap.move(lon_, lat_);
        geomap.set_dirty();
    };
}

GeoMapView::~GeoMapView() {
    if (on_close_) on_close_();
}

GeoMapView::GeoMapView(
    ui::NavigationView& nav,
    const std::string& tag,
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    uint16_t angle,
    const std::function<void(void)> on_close)
    : nav_(nav), altitude_(altitude), altitude_unit_(altitude_unit), speed_unit_(speed_unit), lat_(lat), lon_(lon), angle_(angle), on_close_(on_close) {
    mode_ = DISPLAY;
    add_child(&geopos);
    geomap.init();
    setup();
    geomap.set_mode(mode_);
    geomap.set_tag(tag);
    geomap.set_angle(angle);
    geomap.move(lon_, lat_);
    geomap.set_focusable(true);
    geopos.set_read_only(true);
}

GeoMapView::GeoMapView(
    NavigationView& nav,
    int32_t altitude,
    GeoPos::alt_unit altitude_unit,
    GeoPos::spd_unit speed_unit,
    float lat,
    float lon,
    const std::function<void(int32_t, float, float, int32_t)> on_done)
    : nav_(nav), altitude_(altitude), altitude_unit_(altitude_unit), speed_unit_(speed_unit), lat_(lat), lon_(lon) {
    mode_ = PROMPT;
    add_child(&geopos);
    geomap.init();
    setup();
    add_child(&button_ok);
    geomap.set_mode(mode_);
    geomap.move(lon_, lat_);
    geomap.set_focusable(true);

    button_ok.on_select = [this, on_done, &nav](Button&) {
        if (on_done) on_done(altitude_, lat_, lon_, speed_);
        nav.pop();
    };
}

void GeoMapView::clear_markers() {
    geomap.clear_markers();
}
MapMarkerStored GeoMapView::store_marker(GeoMarker& marker) {
    return geomap.store_marker(marker);
}
MapType GeoMapView::get_map_type() {
    return geomap.get_map_type();
}

} /* namespace ui */