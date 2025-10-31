#pragma once

#include "ui_widget.hpp"

namespace ui {

class NavigationView : public ui::View {
   public:
    NavigationView(ui::Context& context, const ui::Rect parent_rect)
        : View{parent_rect}, context_(context) {
        set_style(ui::Theme::getInstance()->bg_dark);
    }

    template <class T, class... Args>
    T* push(Args&&... args) {
        return reinterpret_cast<T*>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
    }

    template <class T, class... Args>
    T* replace(Args&&... args) {
        pop();
        return reinterpret_cast<T*>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
    }

    void push(View* v);
    View* push_view(std::unique_ptr<View> new_view);
    void replace(View* v);
    void pop(bool trigger_update = true);
    bool set_on_pop(std::function<void()> on_pop);
    ui::Context& context() const override {
        return context_;
    }
    bool is_top() const;
    bool is_valid() const;

    std::function<void()> on_navigate_back{};

   protected:
    ui::Context& context_;
    struct ViewState {
        std::unique_ptr<View> view;
        std::function<void()> on_pop;
    };

    std::vector<ViewState> view_stack{};

    Widget* view() const;

    void free_view();
    void update_view();
};

}  // namespace ui