#pragma once
#include "standalone_application.hpp"

#include "ui_navigation.hpp"

namespace ui {

class StandaloneViewMirror : public NavigationView {
   public:
    StandaloneViewMirror(ui::Context& context, const ui::Rect parent_rect)
        : ui::NavigationView(context, parent_rect) {
        set_style(ui::Theme::getInstance()->bg_dark);
    }
};

}  // namespace ui

extern ui::StandaloneViewMirror* standaloneViewMirror;
extern ui::Context* context;