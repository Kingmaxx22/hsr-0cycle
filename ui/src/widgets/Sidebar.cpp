#include "Sidebar.h"

namespace
{
    const char* kNavItems[] = {
        "TEAM BUILDER",
        "CHARACTERS",
        "LIGHT CONES",
        "RELICS",
        "ENEMIES",
        "ROTATION",
        "SIMULATE",
        "RULES",
        "SCALING"
    };
    constexpr int kNavCount = 9;
}

int Sidebar::itemCount()
{
    return kNavCount;
}

Rectangle Sidebar::itemBounds(int index)
{
    return Rectangle{18.0f, 100.0f + index * 58.0f, 234.0f, 46.0f};
}

int Sidebar::updateAndDraw(int activeNav) const
{
    DrawRectangle(0, 0, 270, GetScreenHeight(), Color{12, 14, 19, 255});
    DrawRectangle(0, 0, 270, 72, Color{25, 28, 38, 255});

    DrawText("HSR", 28, 17, 34, RAYWHITE);
    DrawText("0-CYCLE", 87, 23, 20, GRAY);

    int clicked = -1;
    Vector2 mouse = GetMousePosition();
    bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    for (int i = 0; i < kNavCount; ++i)
    {
        Rectangle r = itemBounds(i);

        if (i == activeNav)
            DrawRectangleRounded(r, 0.25f, 8, Color{55, 62, 82, 255});

        DrawText(kNavItems[i], 34, static_cast<int>(r.y + 13), 17,
                 i == activeNav ? RAYWHITE : Color{155, 160, 174, 255});

        if (pressed && CheckCollisionPointRec(mouse, r))
            clicked = i;
    }

    DrawText("ENGINE", 28, GetScreenHeight() - 82, 13, GRAY);
    DrawText("C++ sim: connected", 28, GetScreenHeight() - 58, 13,
             Color{115, 120, 132, 255});

    return clicked;
}
