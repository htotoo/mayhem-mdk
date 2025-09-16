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

#include "tirapp.hpp"

#include <memory>
#include <string>

StandaloneViewMirror* standaloneViewMirror = nullptr;
ui::Context* context = nullptr;

extern "C" void initialize(const standalone_application_api_t& api) {
    _api = &api;
    screen_width = *_api->screen_width;
    screen_height = *_api->screen_height;
    context = new ui::Context();
    standaloneViewMirror = new StandaloneViewMirror(*context, {0, 16, screen_width, screen_height - 16});
}

// event 1 == frame sync. called each 1/60th of second, so 6 = 100ms

extern "C" void on_event(const uint32_t& events) {
    if (((events & 1) == 1) && (standaloneViewMirror->need_refresh())) {
        Command cmd = Command::PPCMD_IRTX_GETLASTRCVIR;
        std::vector<uint8_t> data(sizeof(ir_data_t));
        if (_api->i2c_read((uint8_t*)&cmd, 2, data.data(), data.size()) == false) return;
        ir_data_t irdata = *(ir_data_t*)data.data();
        standaloneViewMirror->got_data(irdata);
    }
}

extern "C" void shutdown() {
    delete standaloneViewMirror;
    delete context;
}

extern "C" void PaintViewMirror() {
    ui::Painter painter;
    if (standaloneViewMirror)
        painter.paint_widget_tree(standaloneViewMirror);
}

ui::Widget* touch_widget(ui::Widget* const w, ui::TouchEvent event) {
    if (!w->hidden()) {
        // To achieve reverse depth ordering (last object drawn is
        // considered "top"), descend first.
        for (const auto child : w->children()) {
            const auto touched_widget = touch_widget(child, event);
            if (touched_widget) {
                return touched_widget;
            }
        }

        const auto r = w->screen_rect();
        if (r.contains(event.point)) {
            if (w->on_touch(event)) {
                // This widget responded. Return it up the call stack.
                return w;
            }
        }
    }
    return nullptr;
}

ui::Widget* captured_widget{nullptr};

extern "C" void OnTouchEvent(int x, int y, uint32_t type) {
    if (standaloneViewMirror) {
        ui::TouchEvent event{{x, y}, static_cast<ui::TouchEvent::Type>(type)};

        if (event.type == ui::TouchEvent::Type::Start) {
            captured_widget = touch_widget(standaloneViewMirror, event);

            if (captured_widget) {
                captured_widget->focus();
                captured_widget->set_dirty();
            }
        }

        if (captured_widget)
            captured_widget->on_touch(event);
    }
}

extern "C" void OnFocus() {
    if (standaloneViewMirror)
        standaloneViewMirror->focus();
}

extern "C" bool OnKeyEvent(uint8_t key_val) {
    ui::KeyEvent key = (ui::KeyEvent)key_val;
    if (context) {
        auto focus_widget = context->focus_manager().focus_widget();

        if (focus_widget) {
            if (focus_widget->on_key(key))
                return true;

            context->focus_manager().update(standaloneViewMirror, key);

            if (focus_widget != context->focus_manager().focus_widget())
                return true;
            else {
                if (key == ui::KeyEvent::Up || key == ui::KeyEvent::Back || key == ui::KeyEvent::Left) {
                    focus_widget->blur();
                    return false;
                }
            }
        }
    }
    return false;
}

extern "C" bool OnEncoder(int32_t delta) {
    if (context) {
        auto focus_widget = context->focus_manager().focus_widget();

        if (focus_widget)
            return focus_widget->on_encoder((ui::EncoderEvent)delta);
    }

    return false;
}

extern "C" bool OnKeyboad(uint8_t key) {
    if (context) {
        auto focus_widget = context->focus_manager().focus_widget();

        if (focus_widget)
            return focus_widget->on_keyboard((ui::KeyboardEvent)key);
    }

    return false;
}
