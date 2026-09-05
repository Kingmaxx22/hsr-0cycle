#include "Button.h"

Button::Button(Rectangle bounds_, std::string label_)
    : bounds(bounds_), label(std::move(label_))
{
}

bool Button::update()
{
    return CheckCollisionPointRec(GetMousePosition(), bounds)
        && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Button::draw(bool selected) const
{
    Color bg = selected ? Color{55, 62, 82, 255} : Color{30, 33, 43, 255};
    DrawRectangleRounded(bounds, 0.18f, 8, bg);
    DrawText(label.c_str(), static_cast<int>(bounds.x + 14),
             static_cast<int>(bounds.y + 10), 16, RAYWHITE);
}
