#pragma once
#include "raylib.h"
#include <string>

class Button
{
public:
    Button() = default;
    Button(Rectangle bounds, std::string label);

    bool update();
    void draw(bool selected = false) const;

private:
    Rectangle bounds{};
    std::string label;
};
